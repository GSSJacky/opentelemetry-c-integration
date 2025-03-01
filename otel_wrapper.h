#ifndef OTEL_WRAPPER_H
#define OTEL_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

// OpenTelemetry initialization functions
void InitTracer();
void InitMeter();
void InitLogger();
void InitOpenTelemetry();
void CleanupOpenTelemetry();

// Tracing functions
void StartTrace(const char *span_name);
void EndTrace();

// Metrics functions
void RecordMetric(const char *metric_name, int value);

// Logging functions
void LogMessage(const char *message);

#ifdef __cplusplus
}
#endif

#endif // OTEL_WRAPPER_H
