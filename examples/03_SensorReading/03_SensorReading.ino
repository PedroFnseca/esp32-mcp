#include <WiFi.h>
#include <ESP32MCP.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

const int SENSOR_PIN = 34;

MCPServer server("ESP32-Sensors", "1.0.0");

static int targetPin = 0;

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

    server.tool("read_temperature", "Reads ambient temperature in Celsius")
          .readOnly()
          .handle([](MCPResult& r) {
              float temp = readSimulatedTemperature();
              r.text("%.2f °C", temp);
          });

    server.tool("read_analog_pin", "Reads ADC voltage from a specified analog pin")
          .readOnly()
          .param("pin", &targetPin, 0, 39) // Automatic range validation
          .handle([](MCPResult& r) {
              int raw = analogRead(targetPin);
              float voltage = (raw / 4095.0f) * 3.3f;
              r.text("Pin %d raw=%d voltage=%.3fV", targetPin, raw, voltage);
          });

    server.begin();
    Serial.println("MCP Server listening on /mcp");
}

void loop() {
    // Handled in FreeRTOS background task
    delay(1000);
}
