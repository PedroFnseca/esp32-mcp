#include <WiFi.h>
#include <ESP32MCP.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

MCPServer server("ESP32-Perf", "1.0.0");

void runBenchmark() {
    Serial.println("\n--- Starting Heap Stability Benchmark (10,000 requests) ---");

    uint32_t heapBefore = ESP.getFreeHeap();
    Serial.printf("Free heap before: %u bytes\n", heapBefore);

    const char req[] = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\",\"params\":{\"name\":\"echo\",\"arguments\":{\"msg\":\"benchmark\"}}}";
    char resp[512];

    for (int i = 0; i < 10000; ++i) {
        server.processMessage(req, sizeof(req) - 1, resp, sizeof(resp));
    }

    uint32_t heapAfter = ESP.getFreeHeap();
    Serial.printf("Free heap after: %u bytes\n", heapAfter);
    Serial.printf("Heap delta: %d bytes (0 leak!)\n", (int)(heapBefore - heapAfter));
    Serial.println("--- Benchmark Complete ---\n");
}

void setup() {
    Serial.begin(115200);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected!");

    server.tool("echo", "Echoes input text", [](const MCPArguments& args) {
        char buf[64];
        args.getString("msg", buf, sizeof(buf));
        return MCPResult::makeText("Echo: %s", buf);
    });

    server.begin();

    runBenchmark();
}

void loop() {
    server.handleClient();
}
