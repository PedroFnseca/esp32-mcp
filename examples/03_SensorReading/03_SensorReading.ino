#include <WiFi.h>
#include <ESP32MCP.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

const int SENSOR_PIN = 34;

MCPServer server("ESP32-Sensors", "1.0.0");

float readSimulatedTemperature() {
    int raw = analogRead(SENSOR_PIN);
    return 20.0f + (raw / 4095.0f) * 15.0f;
}

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
        "read_temperature",
        "Reads ambient temperature in Celsius",
        [](const MCPArguments& args) {
            float temp = readSimulatedTemperature();
            return MCPResult::text(String(temp, 2) + " °C");
        }
    );

    server.tool(
        "read_analog_pin",
        "Reads ADC voltage from a specified analog pin",
        "{\"type\":\"object\",\"properties\":{\"pin\":{\"type\":\"integer\",\"description\":\"ADC GPIO pin number\"}},\"required\":[\"pin\"]}",
        [](const MCPArguments& args) {
            if (!args.has("pin")) {
                return MCPResult::error("INVALID_PIN", "Missing pin argument");
            }
            int pin = args.getInt("pin");
            if (pin < 0 || pin > 39) {
                return MCPResult::error("INVALID_PIN", "Pin out of valid range (0-39)");
            }
            int raw = analogRead(pin);
            float voltage = (raw / 4095.0f) * 3.3f;
            return MCPResult::text("Pin " + String(pin) + " raw=" + String(raw) + " voltage=" + String(voltage, 3) + "V");
        }
    );

    server.begin();
}

void loop() {
    server.handleClient();
}
