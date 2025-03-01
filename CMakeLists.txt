cmake_minimum_required(VERSION 3.14)
project(clangnewotel)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ensure vcpkg integration
set(CMAKE_TOOLCHAIN_FILE "$ENV{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# Manually set OpenTelemetry paths
set(OpenTelemetry_DIR "$ENV{HOME}/vcpkg/installed/arm64-osx/share/opentelemetry-cpp")
set(CMAKE_PREFIX_PATH "${OpenTelemetry_DIR}")
set(CMAKE_MODULE_PATH "${OpenTelemetry_DIR}")

# Find OpenTelemetry
find_package(opentelemetry-cpp CONFIG REQUIRED)

# Find required dependencies
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MICROHTTPD REQUIRED libmicrohttpd)

# Ensure correct linking
if(MICROHTTPD_FOUND)
    message(STATUS "Found libmicrohttpd: ${MICROHTTPD_LIBRARIES}")
else()
    message(FATAL_ERROR "libmicrohttpd not found")
endif()

# Include directories
include_directories(
    ${OpenTelemetry_DIR}/include
    ${Protobuf_INCLUDE_DIRS}
    ${gRPC_INCLUDE_DIRS}
    ${MICROHTTPD_INCLUDE_DIRS}
    /Users/xuzhengzhi/vcpkg/installed/arm64-osx/include
)

# Explicitly set library path for libmicrohttpd
link_directories(/Users/xuzhengzhi/vcpkg/installed/arm64-osx/lib)

# ✅ Fix: Compile `otel_wrapper` as a static library
add_library(otel_wrapper STATIC otel_wrapper.cpp)

# Link OpenTelemetry, gRPC, and Protobuf
target_link_libraries(otel_wrapper
    PRIVATE
    opentelemetry-cpp::api
    opentelemetry-cpp::sdk
    opentelemetry-cpp::logs
    opentelemetry-cpp::metrics
    opentelemetry-cpp::trace
    opentelemetry-cpp::otlp_grpc_exporter
    opentelemetry-cpp::otlp_grpc_log_record_exporter
    opentelemetry-cpp::otlp_grpc_metrics_exporter
    gRPC::grpc++
    gRPC::grpc
    protobuf::libprotobuf
)

# ✅ Fix: Ensure `web_server` links to `otel_wrapper` with C++ linkage
add_executable(web_server web_server.c)

# find curl added 03/01
set(CURL_ROOT "/opt/homebrew/opt/curl")
set(CURL_INCLUDE_DIR "${CURL_ROOT}/include")
set(CURL_LIBRARY "${CURL_ROOT}/lib/libcurl.dylib")

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})
link_directories(${CURL_LIBRARY})

#target_link_libraries(web_server PRIVATE curl)


# ✅ Force C++ linking by using `target_link_libraries`
set_target_properties(web_server PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(web_server 
    PRIVATE 
    otel_wrapper
    microhttpd
    curl
    "-framework CoreFoundation"    # ✅ Required for SecureTransport
    "-framework Security"          # ✅ Required for SSL/TLS
    "-framework SystemConfiguration" # ✅ Fix for `_SCDynamicStoreCopyProxies`
)


# ✅ Ensure `otel_wrapper` is compiled before linking to `web_server`
add_dependencies(web_server otel_wrapper)

# Copy cataloglist.txt to the build directory
configure_file(${CMAKE_SOURCE_DIR}/cataloglist.txt ${CMAKE_BINARY_DIR}/cataloglist.txt COPYONLY)

# Install cataloglist.txt along with the executable
install(FILES ${CMAKE_SOURCE_DIR}/cataloglist.txt DESTINATION bin)


