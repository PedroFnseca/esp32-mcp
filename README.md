# ESP32-MCP

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![CI Tests](https://github.com/PedroFnseca/esp32-mcp/actions/workflows/ci.yml/badge.svg)](https://github.com/PedroFnseca/esp32-mcp/actions/workflows/ci.yml)
[![Release Please](https://github.com/PedroFnseca/esp32-mcp/actions/workflows/release-please.yml/badge.svg)](https://github.com/PedroFnseca/esp32-mcp/actions/workflows/release-please.yml)
[![Framework](https://img.shields.io/badge/Framework-Arduino%20%7C%20PlatformIO-blue.svg)](https://platformio.org/)
[![Target](https://img.shields.io/badge/Target-ESP32-green.svg)](https://espressif.com/)

**ESP32-MCP** is an open-source, lightweight, and modular C++ library implementing a **Model Context Protocol (MCP) Server** for ESP32 microcontrollers.

It allows AI assistants (such as Claude Desktop, Gemini, Cursor, and autonomous AI agents) to discover and execute hardware functions, control actuators, read sensor telemetry, and trigger embedded workflows on an ESP32 over standard JSON-RPC.

---

## Features

- **Standard MCP 2026-07-28 Protocol**: Full support for the stateless protocol specification including `server/discover`, `initialize`, `ping`, `notifications/initialized`, `tools/list`, and `tools/call`.
- **JSON-RPC 2.0 Compliance**: Proper handling of IDs, request verification, method routing, and standard JSON-RPC error codes.
- **Embedded-First Design**: Optimized for ESP32 memory constraints using [ArduinoJson v7](https://arduinojson.org/) with minimal allocations.
- **Clean & Intuitive API**: Simple lambda-based tool registration with optional JSON Schema parameter definitions.
- **Decoupled Architecture**: Protocol engine is fully separated from network transports.
- **Built-in HTTP Transport**: Ready-to-use HTTP server with CORS support.
- **Host Unit Tested**: Pure protocol and JSON-RPC layers can be validated on host machines without hardware in the loop.

---

## Architecture

```
ESP32-MCP Architecture
┌─────────────────────────────────────────────────────────────┐
│                     User Application                        │
│         (Registers Tools, Controls GPIO, Reads ADC)         │
└──────────────────────────────┬──────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                        MCPServer                            │
│           (Server Configuration & Tool Registry)            │
└──────────────────────────────┬──────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                    MCP Protocol Engine                      │
│     (initialize, ping, tools/list, tools/call dispatch)     │
└──────────────────────────────┬──────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                     JSON-RPC 2.0 Layer                      │
│        (Request Parsing, Validation & Serialization)        │
└──────────────────────────────┬──────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                      Transport Layer                        │
│            MCPHttpTransport (Default: POST /mcp)            │
└─────────────────────────────────────────────────────────────┘
```

---

## Installation

### PlatformIO
Add `ESP32-MCP` and `ArduinoJson` to your `platformio.ini`:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
    bblanchon/ArduinoJson @ ^7.0.0
    https://github.com/PedroFnseca/esp32-mcp.git
```

### Arduino IDE
1. Download this repository as a `.zip` archive.
2. In Arduino IDE, navigate to **Sketch** -> **Include Library** -> **Add .ZIP Library...**
3. Install **ArduinoJson** (version 7.x or newer) via the **Library Manager** (`Ctrl+Shift+I`).

---

## Quick Start

```cpp
#include <WiFi.h>
#include <ESP32MCP.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

MCPServer server("ESP32-Node", "1.0.0");

void setup() {
    Serial.begin(115200);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());

    // Register a tool without parameters
    server.tool(
        "get_uptime",
        "Returns the system uptime in seconds",
        [](const MCPArguments& args) {
            uint32_t uptimeSec = millis() / 1000;
            return MCPResult::text(String(uptimeSec) + "s");
        }
    );

    // Register a tool with JSON Schema parameters
    server.tool(
        "set_led",
        "Turns the onboard LED ON or OFF",
        "{\"type\":\"object\",\"properties\":{\"state\":{\"type\":\"boolean\",\"description\":\"true for ON, false for OFF\"}},\"required\":[\"state\"]}",
        [](const MCPArguments& args) {
            if (!args.has("state")) {
                return MCPResult::error("MISSING_PARAM", "Parameter 'state' is required");
            }
            bool state = args.getBool("state");
            digitalWrite(2, state ? HIGH : LOW);
            return MCPResult::text(state ? "LED turned ON" : "LED turned OFF");
        }
    );

    // Start MCP Server on HTTP port 80 at endpoint /mcp
    server.begin();
}

void loop() {
    server.handleClient();
}
```

---

## API Reference

### `MCPServer`
- `MCPServer(const String& name = "ESP32-MCP", const String& version = "0.1.0")`: Creates server instance.
- `bool tool(const String& name, const String& description, MCPToolHandler handler)`: Registers tool without schema.
- `bool tool(const String& name, const String& description, const String& schemaJson, MCPToolHandler handler)`: Registers tool with custom JSON Schema.
- `void setInstructions(const String& instructions)`: Sets system instructions returned during initialization.
- `bool begin(MCPTransport* transport = nullptr)`: Initializes the transport (defaults to `MCPHttpTransport(80, "/mcp")`).
- `void handleClient()`: Processes pending client connections in `loop()`.
- `String processMessage(const String& message)`: Low-level JSON-RPC message processor.

### `MCPArguments`
Provides type-safe extraction of tool arguments:
- `bool has(const String& key)`: Checks if a key was supplied.
- `String getString(const String& key, const String& defaultVal = "")`
- `int getInt(const String& key, int defaultVal = 0)`
- `long getLong(const String& key, long defaultVal = 0)`
- `float getFloat(const String& key, float defaultVal = 0.0f)`
- `double getDouble(const String& key, double defaultVal = 0.0)`
- `bool getBool(const String& key, bool defaultVal = false)`
- `JsonObjectConst getObject(const String& key)`
- `JsonArrayConst getArray(const String& key)`

### `MCPResult`
Response builder returned by tool handlers:
- `MCPResult::text(const String& text)`: Standard text response.
- `MCPResult::json(const String& jsonString)`: Formatted JSON response.
- `MCPResult::error(const String& message)`: Tool execution failure.
- `MCPResult::error(const String& code, const String& message)`: Tool execution failure with error code.

---

## Protocol Testing via HTTP

You can test your running ESP32 directly using `curl` or any HTTP client:

### 1. Server Discovery (`server/discover` - MCP 2026-07-28)
```bash
curl -X POST http://<ESP32_IP>/mcp \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"server/discover","params":{}}'
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "supportedVersions": ["2026-07-28"],
    "capabilities": { "tools": { "listChanged": false } },
    "_meta": {
      "io.modelcontextprotocol/serverInfo": { "name": "ESP32-Node", "version": "1.0.0" }
    },
    "serverInfo": { "name": "ESP32-Node", "version": "1.0.0" }
  }
}
```

### 2. Initialize Handshake
```bash
curl -X POST http://<ESP32_IP>/mcp \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":2,"method":"initialize","params":{"protocolVersion":"2026-07-28","capabilities":{},"clientInfo":{"name":"test-client","version":"1.0"}}}'
```

### 3. Discover Tools (`tools/list`)
```bash
curl -X POST http://<ESP32_IP>/mcp \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":3,"method":"tools/list"}'
```

### 4. Call a Tool (`tools/call`)
```bash
curl -X POST http://<ESP32_IP>/mcp \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":4,"method":"tools/call","params":{"name":"set_led","arguments":{"state":true}}}'
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "result": {
    "content": [
      { "type": "text", "text": "LED turned ON" }
    ],
    "isError": false
  }
}
```

---

## Running Unit Tests

The library includes automated unit tests that validate JSON-RPC formatting, error handling, argument extraction, tool registration, and protocol handshakes on host machines without requiring physical hardware:

```bash
g++ -std=c++14 -Isrc -Itest/mock -Itest/include src/mcp/MCPToolRegistry.cpp src/mcp/MCPJsonRpc.cpp src/mcp/MCPServer.cpp src/mcp/transports/MCPHttpTransport.cpp test/test_main.cpp -o test/test_runner
./test/test_runner
```

---

## Examples

Check the `examples/` directory for ready-to-flash projects:
- **`01_BasicServer`**: Uptime and free heap reporting.
- **`02_LedControl`**: Digital GPIO actuation with parameter schema.
- **`03_SensorReading`**: Analog sensor ADC conversion and pin validation.
- **`04_StructuredData`**: Comprehensive JSON system telemetry reporting.

---

## Contributing

Contributions are welcome! Please check our [Contributing Guide](CONTRIBUTING.md) for details on our commit conventions (Conventional Commits for Release Please), clean code rules, and testing workflow.

---

## License

This library is licensed under the [MIT License](LICENSE).
