#include <WiFi.h>
#include <ESP32MCP.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

const int LED_PIN = 2;

MCPServer server("ESP32-LED", "1.0.0");

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

    server.tool(
        "set_led",
        "Turns the onboard LED ON or OFF",
        "{\"type\":\"object\",\"properties\":{\"state\":{\"type\":\"boolean\",\"description\":\"true for ON, false for OFF\"}},\"required\":[\"state\"]}",
        [](const MCPArguments& args) {
            if (!args.has("state")) {
                return MCPResult::error("MISSING_ARG", "Parameter 'state' is required");
            }
            bool state = args.getBool("state");
            digitalWrite(LED_PIN, state ? HIGH : LOW);
            return MCPResult::text(state ? "LED turned ON" : "LED turned OFF");
        }
    );

    server.tool(
        "toggle_led",
        "Toggles current state of the LED",
        [](const MCPArguments& args) {
            int current = digitalRead(LED_PIN);
            int next = (current == HIGH) ? LOW : HIGH;
            digitalWrite(LED_PIN, next);
            return MCPResult::text(next == HIGH ? "LED toggled ON" : "LED toggled OFF");
        }
    );

    server.begin();
}

void loop() {
    server.handleClient();
}
