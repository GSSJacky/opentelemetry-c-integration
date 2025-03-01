#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "otel_wrapper.h"  // OpenTelemetry wrapper header file

#define PORT 8080
#define FILE_PATH "cataloglist.txt"
#define POST_BUFFER_SIZE 512

// Struct to store POST data
struct PostData
{
    char buffer[POST_BUFFER_SIZE];
    size_t length;
};

// Struct to store HTTP response
struct MemoryStruct {
    char *memory;
    size_t size;
};

// Function prototypes
static char *get_catalog_list();
static char *get_catalog_by_id(const char *id);
static char *insert_catalog(const char *id, char *catalog_name);
static void record_metrics(const char *endpoint);
static void record_logs(const char *message, const char *level);
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
static char *search_from_url(const char *query);

// Function to handle API response
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + real_size + 1);
    if (!ptr) {
        printf("Memory allocation failed\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, real_size);
    mem->size += real_size;
    mem->memory[mem->size] = 0;

    return real_size;
}

// Implement OpenTelemetry Logging
static void record_logs(const char *message, const char *level)
{
    LogMessage(message);
    printf("[Logs] %s: %s\n", level, message);
}

// Implement OpenTelemetry Metrics
static void record_metrics(const char *endpoint)
{
    RecordMetric("http_requests_total", 1);
    printf("[Metrics] Recorded request for %s\n", endpoint);
}

// Function to fetch content from an API
static char *search_from_url(const char *query) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl) {
        char url[512];
        snprintf(url, sizeof(url), "https://api.duckduckgo.com/?q=%s&format=json", query);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            free(chunk.memory);
            curl_easy_cleanup(curl);
            return strdup("Error: Failed to fetch data.");
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return chunk.memory;
}

// Function to insert a new catalog entry
static char *insert_catalog(const char *id, char *catalog_name)
{
    if (!id || !catalog_name || strlen(id) == 0 || strlen(catalog_name) == 0) {
        record_logs("Invalid catalog insert request", "WARNING");
        return strdup("Error: Missing ID or catalog name.\n");
    }

    FILE *file = fopen(FILE_PATH, "a");
    if (!file) {
        record_logs("Failed to open catalog list file for writing", "ERROR");
        return strdup("Error: Could not open catalog list file.\n");
    }

    fprintf(file, "%s,%s\n", id, catalog_name);
    fclose(file);

    record_logs("Catalog inserted successfully", "INFO");
    return strdup("Success: Catalog inserted.\n");
}

// HTTP Request Handler
static enum MHD_Result request_handler(void *cls, struct MHD_Connection *connection, 
                                       const char *url, const char *method, 
                                       const char *version, const char *upload_data, 
                                       size_t *upload_data_size, void **con_cls)
{
    struct MHD_Response *response;
    enum MHD_Result ret;
    char *response_text = NULL;

    // OpenTelemetry Trace Start
    StartTrace(url);
    record_metrics(url);
    record_logs("Processing request", "INFO");

    printf("Received request: %s %s\n", method, url); // Debugging Output

    if (strcmp(url, "/getCataloglist") == 0 && strcmp(method, "GET") == 0)
    {
        response_text = get_catalog_list();
    }
    else if (strcmp(url, "/getCatalog") == 0 && strcmp(method, "GET") == 0)
    {
        const char *id = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "id");
        response_text = id ? get_catalog_by_id(id) : strdup("Error: Missing catalog ID.\n");
    }
    else if (strcmp(url, "/insertCatalog") == 0 && strcmp(method, "POST") == 0)
    {
        struct PostData *post_data = *con_cls;

        if (!post_data)
        {
            post_data = calloc(1, sizeof(struct PostData));
            if (!post_data)
                return MHD_NO;
            *con_cls = post_data;
            return MHD_YES;
        }

        if (*upload_data_size > 0)
        {
            printf("Received chunk: %s\n", upload_data);
            if ((post_data->length + *upload_data_size) < POST_BUFFER_SIZE)
            {
                strncat(post_data->buffer, upload_data, *upload_data_size);
                post_data->length += *upload_data_size;
            }
            *upload_data_size = 0;
            return MHD_YES;
        }

        char id[50] = {0};
        char catalog_name[200] = {0};
        sscanf(post_data->buffer, "id=%49[^&]&catalogname=%199[^\"]", id, catalog_name);

        printf("Extracted ID: %s\n", id);
        printf("Extracted Catalog Name: %s\n", catalog_name);

        response_text = insert_catalog(id, catalog_name);

        free(post_data);
        *con_cls = NULL;
    }
    else if (strcmp(url, "/searchfromURL") == 0 && strcmp(method, "GET") == 0)
    {
        const char *query = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "q");
        response_text = query ? search_from_url(query) : strdup("Error: Missing query parameter.");
    }
    else
    {
        response_text = strdup("Error: Invalid request.\n");
    }

    response = MHD_create_response_from_buffer(strlen(response_text), response_text, MHD_RESPMEM_MUST_FREE);
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    EndTrace(); // ✅ End the trace after response is sent

    return ret;
}


// Function to retrieve the catalog list
static char *get_catalog_list()
{
    FILE *file = fopen(FILE_PATH, "r");
    if (!file) {
        record_logs("Failed to open catalog list file", "ERROR");
        return strdup("Error: Could not open catalog list file.\n");
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(file_size + 1);
    if (!buffer)
    {
        fclose(file);
        record_logs("Memory allocation failed", "ERROR");
        return strdup("Error: Memory allocation failed.\n");
    }

    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';
    fclose(file);

    record_logs("Successfully retrieved catalog list", "INFO");
    return buffer;
}

// Function to retrieve a catalog by ID
static char *get_catalog_by_id(const char *id)
{
    FILE *file = fopen(FILE_PATH, "r");
    if (!file) {
        record_logs("Failed to open catalog list file", "ERROR");
        return strdup("Error: Could not open catalog list file.\n");
    }

    char line[256];
    while (fgets(line, sizeof(line), file))
    {
        char file_id[50], catalog_name[200];
        if (sscanf(line, "%49[^,],%199[^\n]", file_id, catalog_name) == 2)
        {
            if (strcmp(file_id, id) == 0)
            {
                fclose(file);
                record_logs("Catalog found for given ID", "INFO");
                return strdup(line);
            }
        }
    }

    fclose(file);
    record_logs("Catalog not found for given ID", "WARNING");
    return strdup("Error: Catalog not found.\n");
}


// Main Function - Start Web Server
int main()
{
    struct MHD_Daemon *daemon;

    // OpenTelemetry Initialization
    InitTracer();
    InitMeter();
    InitLogger();

    // Start HTTP Server
    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL, &request_handler, NULL, MHD_OPTION_END);

    if (!daemon)
    {
        printf("Failed to start server\n");
        return 1;
    }

    printf("Server is running on port %d...\n", PORT);
    getchar();

    MHD_stop_daemon(daemon);
    return 0;
}
