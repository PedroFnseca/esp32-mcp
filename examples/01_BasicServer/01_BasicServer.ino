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

    // Fluent declarative tools with printf-style formatting
    server.tool("get_uptime", "Returns the system uptime in seconds")
          .readOnly()
          .handle([](MCPResult& r) {
              r.text("Uptime: %lu seconds", (unsigned long)(millis() / 1000));
          });

    server.tool("get_free_heap", "Returns available free heap memory in bytes")
          .readOnly()
          .handle([](MCPResult& r) {
              r.text("Free heap: %lu bytes", (unsigned long)ESP.getFreeHeap());
          });

    // Or enable built-in status tool:
    // server.enableStatus();

    server.begin();
    Serial.println("MCP Server listening on /mcp (mDNS: esp32-basic.local)");
}

void loop() {
    // Server runs in a background FreeRTOS task, keeping loop() free!
    delay(1000);
}
