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

    server.tool(
        "get_system_telemetry",
        "Returns comprehensive JSON telemetry report of ESP32 system state",
        [](const MCPArguments& args) {
            JsonDocument doc;
            doc["chip_model"] = ESP.getChipModel();
            doc["chip_revision"] = ESP.getChipRevision();
            doc["cpu_freq_mhz"] = ESP.getCpuFreqMHz();
            doc["free_heap"] = ESP.getFreeHeap();
            doc["min_free_heap"] = ESP.getMinFreeHeap();
            doc["heap_size"] = ESP.getHeapSize();
            doc["uptime_ms"] = millis();
            doc["wifi_rssi"] = WiFi.RSSI();
            doc["ip_address"] = WiFi.localIP().toString();

            String jsonOutput;
            serializeJson(doc, jsonOutput);

            return MCPResult::json(jsonOutput);
        }
    );

    server.begin();
}

void loop() {
    server.handleClient();
}
