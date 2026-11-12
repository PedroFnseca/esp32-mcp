#include <WiFi.h>
#include <ESP32MCP.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

MCPServer server("ESP32-Secure", "1.0.0");

static bool relayState = false;

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

    // 1. Configure Bearer Token (constant-time verification)
    server.bearer("my-super-secret-mcp-token-2026");

    // 2. Configure CORS Origin (closed by default; allowed only from this origin)
    server.allowOrigin("https://my-dashboard.example.com");

    // 3. Register tools
    server.tool("toggle_relay", "Switch relay state securely")
          .destructive()
          .param("state", &relayState, "true for energized, false for de-energized")
          .handle([](MCPResult& r) {
              r.text("Relay is now %s", relayState ? "ON" : "OFF");
          });

    // 4. Enable status diagnostics
    server.enableStatus();

    // 5. Start server (runs in FreeRTOS task)
    // Note: On local networks, place behind a TLS reverse proxy (e.g. Caddy/Nginx) for HTTPS.
    server.begin();
    Serial.println("Secure MCP Server ready on /mcp");
}

void loop() {
    delay(1000);
}
