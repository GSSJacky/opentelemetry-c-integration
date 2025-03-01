#include "otel_wrapper.h"
#include <iostream>
#include <memory>

// OpenTelemetry Core
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/metrics/provider.h>
#include <opentelemetry/logs/provider.h>

// OpenTelemetry SDK Components
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/sdk/trace/simple_processor.h>
#include <opentelemetry/sdk/metrics/meter_provider.h>
#include <opentelemetry/sdk/logs/logger_provider.h>

// OpenTelemetry Exporters
#include <opentelemetry/exporters/otlp/otlp_grpc_exporter.h>
#include <opentelemetry/exporters/otlp/otlp_grpc_metric_exporter.h>
#include <opentelemetry/exporters/otlp/otlp_grpc_log_record_exporter.h>

#include <opentelemetry/sdk/logs/batch_log_record_processor.h>

namespace trace_sdk = opentelemetry::sdk::trace;
namespace metrics_sdk = opentelemetry::sdk::metrics;
namespace logs_sdk = opentelemetry::sdk::logs;
namespace trace_api = opentelemetry::trace;
namespace metrics_api = opentelemetry::metrics;
namespace logs_api = opentelemetry::logs;
namespace nostd = opentelemetry::nostd;

// Tracer initialization
void InitTracer()
{

    opentelemetry::exporter::otlp::OtlpGrpcExporterOptions options;
    options.endpoint = std::getenv("OTEL_EXPORTER_OTLP_TRACES_ENDPOINT") ? std::getenv("OTEL_EXPORTER_OTLP_TRACES_ENDPOINT") : "http://localhost:4317";
    
    auto exporter = std::make_unique<opentelemetry::exporter::otlp::OtlpGrpcExporter>(options);
    // auto exporter = std::make_unique<opentelemetry::exporter::otlp::OtlpGrpcExporter>();
    
    auto processor = std::make_unique<trace_sdk::SimpleSpanProcessor>(std::move(exporter));

    std::vector<std::unique_ptr<trace_sdk::SpanProcessor>> processors;
    processors.emplace_back(std::move(processor));

    auto context = std::make_unique<trace_sdk::TracerContext>(std::move(processors));
    auto provider = nostd::shared_ptr<trace_api::TracerProvider>(
        new trace_sdk::TracerProvider(std::move(context)));

    trace_api::Provider::SetTracerProvider(provider);
    std::cout << "[OTEL] Tracer initialized.\n";
}

// Metrics initialization
void InitMeter()
{
    auto exporter = std::make_unique<opentelemetry::exporter::otlp::OtlpGrpcMetricExporter>();
    auto meter_provider = std::make_shared<metrics_sdk::MeterProvider>();

    opentelemetry::metrics::Provider::SetMeterProvider(
        nostd::shared_ptr<metrics_sdk::MeterProvider>(meter_provider));

    std::cout << "[OTEL] Meter initialized.\n";
}

// Logger initialization
void InitLogger()
{
    auto exporter = std::make_unique<opentelemetry::exporter::otlp::OtlpGrpcLogRecordExporter>();
    auto processor = std::make_unique<logs_sdk::BatchLogRecordProcessor>(std::move(exporter));

    auto provider = std::make_shared<logs_sdk::LoggerProvider>();
    provider->AddProcessor(std::move(processor));

    logs_api::Provider::SetLoggerProvider(nostd::shared_ptr<logs_sdk::LoggerProvider>(provider));

    std::cout << "[OTEL] Logger initialized.\n";
}

// Initialize all OpenTelemetry components
void InitOpenTelemetry()
{
    InitTracer();
    InitMeter();
    InitLogger();
}

// Cleanup function
void CleanupOpenTelemetry()
{
    std::cout << "[OTEL] Cleaning up OpenTelemetry...\n";
}

void StartTrace(const char *span_name)
{
    auto tracer = trace_api::Provider::GetTracerProvider()->GetTracer("example_tracer");

    // ✅ Correct way to specify SpanKind
    trace_api::StartSpanOptions options;
    options.kind = trace_api::SpanKind::kServer;  // ✅ Set ENTRY SpanKind properly

    auto span = tracer->StartSpan(span_name, {{"otel.service.name", 
                  std::getenv("OTEL_SERVICE_NAME") ? std::getenv("OTEL_SERVICE_NAME") : "DefaultOTELService"}}, options);

    auto scope = tracer->WithActiveSpan(span);
    
    std::cout << "[OTEL] Started trace: " << span_name << " with ENTRY kind\n";
}

// End a trace (optional)
void EndTrace()
{
    auto span = trace_api::Provider::GetTracerProvider()->GetTracer("example_tracer")->GetCurrentSpan();
    span->End();
    std::cout << "[OTEL] Trace ended.\n";
}

// Record a metric
void RecordMetric(const char *metric_name, int value)
{
    std::cout << "[OTEL] Metric recorded: " << metric_name << " = " << value << "\n";
}

// Log a message
void LogMessage(const char *message)
{
    std::cout << "[OTEL] Log: " << message << "\n";
}
