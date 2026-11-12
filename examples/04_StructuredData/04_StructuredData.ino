#include <WiFi.h>
#include <ESP32MCP.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

MCPServer server("ESP32-Telemetry", "1.0.0");

void setup() {
    Serial.begin(115200);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected! IP: ");
    Serial.println(WiFi.localIP());

    server.tool("get_system_telemetry", "Returns comprehensive JSON telemetry report of ESP32 system state")
          .readOnly()
          .handle([](MCPResult& r) {
              char buf[512];
              MCPJsonWriter writer(buf, sizeof(buf));
              writer.startObject();
              writer.addString("chip_model", ESP.getChipModel());
              writer.addLong("chip_revision", (long)ESP.getChipRevision());
              writer.addLong("cpu_freq_mhz", (long)ESP.getCpuFreqMHz());
              writer.addLong("free_heap", (long)ESP.getFreeHeap());
              writer.addLong("min_free_heap", (long)ESP.getMinFreeHeap());
              writer.addLong("heap_size", (long)ESP.getHeapSize());
              writer.addLong("uptime_ms", (long)millis());
              writer.addLong("wifi_rssi", (long)WiFi.RSSI());
              writer.addString("ip_address", WiFi.localIP().toString().c_str());
              writer.endObject();

              r.json(writer.c_str());
          });

    server.begin();
    Serial.println("MCP Server listening on /mcp");
}

void loop() {
    // Handled in FreeRTOS background task
    delay(1000);
}
