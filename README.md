# OpenTelemetry C++ SDK Integration for C-based Applications

## **Overview**
This project demonstrates how to integrate **OpenTelemetry C++ SDK** into a **C-based application** to enable distributed tracing, metrics, and logging. The application exports telemetry data using the **OTLP (gRPC/HTTP) protocol** either through an **OpenTelemetry Agent** or directly to **Instana Backend**.

For more details, refer to the official documentation: [IBM Instana OpenTelemetry C SDK](https://www.ibm.com/docs/en/instana-observability/current?topic=sdks-c)

The implementation includes:
- **Tracing**: Capturing application requests and tracking execution across services.
- **Metrics**: Collecting performance data for monitoring.
- **Logging**: Structured logging with OpenTelemetry for better observability.

This project provides a simple **C-based web server** that supports **basic catalog management APIs** and a **web content search API** using OpenTelemetry instrumentation.

---

## **📌 Build & Run on macOS**

### **1️⃣ Prerequisites**
Ensure you have the following installed:

```bash
brew install cmake curl vcpkg microhttpd
```

Set up **vcpkg**:
```bash
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
export VCPKG_ROOT=$(pwd)
export CMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
```

Install required libraries:
```bash
$VCPKG_ROOT/vcpkg install opentelemetry-cpp microhttpd protobuf grpc
```

---

### **2️⃣ Clone and Build**

```bash
git clone https://github.com/GSSJacky/opentelemetry-c-integration
cd opentelemetry-c-integration
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE
make -j$(sysctl -n hw.logicalcpu)
```

---

### **3️⃣ Set Up OpenTelemetry & Instana**

Before running the server, set the **OpenTelemetry environment variables**:
```bash
export OTEL_SERVICE_NAME="ClangOTELServiceJacky"
export OTEL_EXPORTER_OTLP_INSECURE=true
export OTEL_EXPORTER_OTLP_TRACES_ENDPOINT="http://localhost:4317"
```

---

### **4️⃣ Run the Web Server**
```bash
./web_server
```

---

## **📌 API Usage (Test via `curl`)**

### **Insert Catalog**
```bash
curl -X POST http://localhost:8080/insertCatalog -d "id=8&catalogname=Product8D"
```

### **Get Catalog by ID**
```bash
curl "http://localhost:8080/getCatalog?id=2"
```

### **Search Online Content**
```bash
curl "http://localhost:8080/searchfromURL?q=openai"
```

---

## **📌 Instana Setup**

### **1️⃣ Install Instana Agent**
For Linux, follow the installation documentation.

### **2️⃣ Enable OpenTelemetry in Instana**

Edit **Instana Agent Configuration**:
```yaml
com.instana.plugin.opentelemetry:
  enabled: true
```
Restart the Instana Agent:
```bash
sudo systemctl restart instana-agent.service
netstat -ano | grep 4317
```
If you see **0.0.0.0:4317**, the setup is correct!

### **3️⃣ Verify in Instana UI**
After running `./web_server`, open **Instana Dashboard**:
- Navigate to **Applications > Services > ClangOTELServiceJacky**
- Check "Call Analysis" for traces

---

## **📌 Check Installed Libraries & Dependencies in vcpkg**

To verify whether all necessary OpenTelemetry and gRPC-related libraries are installed, run the following commands:

### **1️⃣ Check OpenTelemetry Installation**
```bash
ls ~/vcpkg/installed/arm64-osx/lib | grep opentelemetry
```
✅ **Expected Output:**
```
libopentelemetry_trace.a
libopentelemetry_metrics.a
libopentelemetry_logs.a
```

### **2️⃣ Check gRPC Installation**
```bash
find ~/vcpkg/installed/arm64-osx -name '*grpc*.a'
```
✅ **Expected Output:**
```
~/vcpkg/installed/arm64-osx/lib/libgrpc.a
~/vcpkg/installed/arm64-osx/lib/libgrpc++_reflection.a
~/vcpkg/installed/arm64-osx/lib/libgrpc++.a
```

### **3️⃣ Check Installed Packages**
```bash
$VCPKG_ROOT/vcpkg list | grep -E 'opentelemetry|grpc|protobuf|microhttpd'
```
✅ **Expected Output:**
```
opentelemetry-cpp:x64-osx        1.10.0       OpenTelemetry C++ SDK
grpc:x64-osx                      1.58.0       Open-source RPC framework
protobuf:x64-osx                   3.21.0      Protocol Buffers (protobuf)
microhttpd:x64-osx                 0.9.76      Small HTTP server library
```

---
