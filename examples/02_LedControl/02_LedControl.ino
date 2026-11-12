#include <WiFi.h>
#include <ESP32MCP.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

const int LED_PIN = 2;

MCPServer server("ESP32-LED", "1.0.0");

// Direct variable binding for fluent tools
static bool ledState = false;
static int ledBrightness = 255;

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected! IP: ");
    Serial.println(WiFi.localIP());

    // Fluent tool with automatic inputSchema generation & validation
    server.tool("set_led", "Turns the onboard LED ON or OFF with optional brightness")
          .destructive()
          .param("state", &ledState, "true for ON, false for OFF")
          .param("brightness", &ledBrightness, 0, 255) // validated min/max
          .handle([](MCPResult& r) {
              digitalWrite(LED_PIN, ledState ? HIGH : LOW);
              r.text("LED %s (brightness: %d)", ledState ? "ON" : "OFF", ledBrightness);
          });

    server.tool("toggle_led", "Toggles current state of the LED")
          .handle([](MCPResult& r) {
              int current = digitalRead(LED_PIN);
              int next = (current == HIGH) ? LOW : HIGH;
              digitalWrite(LED_PIN, next);
              ledState = (next == HIGH);
              r.text(next == HIGH ? "LED toggled ON" : "LED toggled OFF");
          });

    server.begin();
    Serial.println("MCP Server listening on /mcp");
}

void loop() {
    // Handled in FreeRTOS background task
    delay(1000);
}
