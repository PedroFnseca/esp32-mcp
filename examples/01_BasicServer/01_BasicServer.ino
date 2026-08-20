#include <WiFi.h>
#include <ESP32MCP.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

MCPServer server("ESP32-Basic", "1.0.0");

void setup() {
    Serial.begin(115200);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());

    server.setInstructions("ESP32 basic device providing system diagnostics.");

    server.tool(
        "get_uptime",
        "Returns the system uptime in seconds",
        [](const MCPArguments& args) {
            uint32_t sec = millis() / 1000;
            return MCPResult::text("Uptime: " + String(sec) + " seconds");
        }
    );

    server.tool(
        "get_free_heap",
        "Returns available free heap memory in bytes",
        [](const MCPArguments& args) {
            uint32_t freeHeap = ESP.getFreeHeap();
            return MCPResult::text("Free heap: " + String(freeHeap) + " bytes");
        }
    );

    server.begin();
    Serial.println("MCP Server listening on /mcp");
}

void loop() {
    server.handleClient();
}
