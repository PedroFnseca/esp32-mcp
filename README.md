# ESP32-MCP: Model Context Protocol (MCP) Server Library for ESP32 (Arduino & PlatformIO)

<div align="center">

[![CI](https://github.com/PedroFnseca/esp32-mcp/actions/workflows/ci.yml/badge.svg)](https://github.com/PedroFnseca/esp32-mcp/actions/workflows/ci.yml)
[![GitHub Release](https://img.shields.io/github/v/release/PedroFnseca/esp32-mcp?include_prereleases&style=flat-square&color=blue)](https://github.com/PedroFnseca/esp32-mcp/releases)
[![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg?style=flat-square)](LICENSE)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-red.svg?style=flat-square&logo=espressif)](https://www.espressif.com/en/products/socs/esp32)
[![Framework: Arduino](https://img.shields.io/badge/Framework-Arduino-00979C.svg?style=flat-square&logo=arduino)](https://www.arduino.cc/)
[![Dependencies: 0](https://img.shields.io/badge/Dependencies-0-brightgreen.svg?style=flat-square)](#)
[![MCP Spec](https://img.shields.io/badge/MCP%20Spec-2026--07--28%20%7C%202024--11--05-blueviolet.svg?style=flat-square)](https://modelcontextprotocol.io)

**Turn an ESP32 into a native [Model Context Protocol](https://modelcontextprotocol.io) (MCP) server, so AI agents such as Claude, Cursor, Gemini, and custom LLM workflows can read sensors and control hardware over plain HTTP.**

[Quick Start](#quick-start) · [How It Works](#how-it-works) · [Defining Tools](#defining-tools) · [Client Setup](#connecting-mcp-clients) · [Security](#security-and-authentication) · [FAQ](#faq) · [Contributing](CONTRIBUTING.md)

</div>

---

## Overview

**ESP32-MCP** is a lightweight C++ library that lets an ESP32 microcontroller act as an **MCP server**. It exposes your hardware (GPIO pins, sensors, relays, motors, LEDs) as **MCP tools** that any MCP-compatible AI client can discover and call through a single HTTP endpoint (`/mcp`).

It is built for the Arduino framework and works with **PlatformIO** and the **Arduino IDE**. It has **zero external dependencies** and performs **no dynamic memory allocation** while handling requests, which keeps the heap stable on long-running devices.

**Typical use cases**

- Control IoT devices and home automation with natural language through an AI assistant.
- Give an LLM agent safe, typed access to sensors and actuators.
- Build AI-driven robotics, lab equipment, and industrial monitoring prototypes.
- Prototype agentic hardware workflows without writing a custom JSON-RPC server.

---

## Table of Contents

- [Features](#features)
- [How It Works](#how-it-works)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Defining Tools](#defining-tools)
- [Security and Authentication](#security-and-authentication)
- [Connecting MCP Clients](#connecting-mcp-clients)
- [Testing with cURL](#testing-with-curl)
- [Configuration and Memory Tuning](#configuration-and-memory-tuning)
- [Performance and Memory Benchmark](#performance-and-memory-benchmark)
- [FAQ](#faq)
- [Contributing](#contributing)
- [License](#license)

---

## Features

| Feature | Description |
| :--- | :--- |
| **Zero dependencies** | Custom in-place JSON scanner and writer. No `ArduinoJson` or third-party libraries required. |
| **No heap allocation in hot paths** | Uses pre-allocated static buffers. No `malloc`, `new`, or heap string reallocation during request dispatch, which avoids fragmentation and out-of-memory crashes. |
| **Fluent, declarative API** | Define tools with direct variable binding, for example `.param("state", &myVar)`. JSON Schema generation, type validation, and range enforcement are automatic. |
| **Asynchronous FreeRTOS execution** | A dedicated background task pinned to Core 1 handles network I/O and JSON-RPC dispatch, so your `loop()` is never blocked. |
| **Authentication and CORS** | Constant-time bearer token verification (resistant to timing attacks) and a closed-by-default CORS policy. |
| **Dual MCP protocol support** | Implements the stateless MCP specification `2026-07-28` (with discovery and caching hints) and falls back to legacy clients on `2024-11-05`. |
| **Built-in diagnostics** | Optional `get_status`, `read_pin`, and `write_pin` tools with GPIO allow-listing. |

---

## How It Works

1. **Zero-copy in-place scanner (`MCPJsonScanner`)**
   Instead of building an AST on the heap like typical JSON parsers, the scanner reads the HTTP body directly in the input buffer. Strings, numbers, and booleans are sliced by index and converted on the fly, with no intermediate heap strings.

2. **Direct variable binding**
   When you register `.param("speed", &motorSpeed, 0, 100)`, the registry stores the address of `motorSpeed`. When an LLM calls the tool, the engine validates the argument, enforces the `[0, 100]` bounds, and writes the value into the variable before your handler runs.

3. **Deterministic memory footprint**
   Request (`MCP_MAX_REQUEST_SIZE`) and response (`MCP_MAX_RESPONSE_SIZE`) buffers are statically allocated, so network bursts cannot exhaust free heap.

4. **Non-blocking FreeRTOS concurrency**
   `server.begin()` spawns a FreeRTOS task on Core 1 (stack: 4096 bytes, priority: 1). Web traffic and JSON-RPC execution do not delay time-critical code in your main `loop()`.

---

## Installation

### PlatformIO

Add the library to your `platformio.ini`:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

; Pull directly from Git or a local workspace
lib_deps =
    https://github.com/PedroFnseca/esp32-mcp.git
```

### Arduino IDE

1. Download the latest `.zip` from [Releases](https://github.com/PedroFnseca/esp32-mcp/releases).
2. In the Arduino IDE, go to **Sketch** → **Include Library** → **Add .ZIP Library...**.
3. Add `#include <ESP32MCP.h>` to your sketch.

---

## Quick Start

A complete example that exposes an LED toggle tool to any MCP-compatible AI client:

```cpp
#include <WiFi.h>
#include <ESP32MCP.h>

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Initialize server: Name, Version, Port (default: 80)
MCPServer server("ESP32-Node", "1.0.0");

static bool ledState = false;

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\nConnected! MCP endpoint: http://%s/mcp\n", WiFi.localIP().toString().c_str());

    // Optional: protect the endpoint with a bearer token
    server.bearer("secret-device-key");

    // Declare an MCP tool with direct variable binding
    server.tool("set_led", "Turns the onboard LED ON or OFF")
          .destructive()
          .param("state", &ledState, "true for ON, false for OFF")
          .handle([](MCPResult& r) {
              digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
              r.text("LED is now %s", ledState ? "ON" : "OFF");
          });

    // Starts the HTTP server and the background FreeRTOS task on Core 1
    server.begin();
}

void loop() {
    // loop() stays free for sensor readings and control logic
    delay(1000);
}
```

Once the board is connected, point your MCP client at `http://<device-ip>/mcp` (see [Connecting MCP Clients](#connecting-mcp-clients)).

---

## Defining Tools

### Fluent Declarative API

Tools are declared with method chaining. Parameters are validated against their types and ranges before your handler runs:

```cpp
static bool power = false;
static int brightness = 100;
static float frequency = 50.0f;
static char mode[32] = "auto";

server.tool("configure_light", "Controls power, brightness, and operational mode")
      .destructive()
      .title("Smart Lighting System")
      .param("power", &power, "Master power state: true or false")
      .param("brightness", &brightness, 0, 255)            // Auto-validated [min, max]
      .param("frequency", &frequency, 10.0f, 120.0f)       // Float with bounds
      .param("mode", mode, sizeof(mode), "Operating mode") // Bounded string buffer
      .handle([](MCPResult& r) {
          // All bound variables are already validated and assigned
          r.text("Light set to %s | Brightness: %d | Freq: %.1f Hz | Mode: %s",
                 power ? "ON" : "OFF", brightness, frequency, mode);
      });
```

### Supported Parameter Types

| Type | Syntax | Notes |
| :--- | :--- | :--- |
| **Boolean** | `.param("state", &boolVar, "description")` | Coerced from a boolean or the strings `"true"` / `"false"`. |
| **Integer** | `.param("val", &intVar, minVal, maxVal)` | Range checked. Out-of-range values return error `-32602`. |
| **Float / Double** | `.param("freq", &floatVar, 0.0f, 100.0f)` | Floating-point conversion with bounds validation. |
| **String** | `.param("str", buf, sizeof(buf), "desc")` | Zero-copy string slice with buffer overflow guard. |
| **Optional** | `.param("opt", &var, ...).optional()` | If the LLM omits it, the variable keeps its default value. |

### Tool Annotations

Annotations tell LLMs what a tool does, which improves safety and how clients render it:

| Annotation | Meaning |
| :--- | :--- |
| `.readOnly()` | The tool has no side effects (`readOnly: true`). Safe for autonomous use. |
| `.destructive()` | The tool changes hardware state or performs irreversible actions (`destructive: true`). |
| `.idempotent()` | Repeating the call with identical inputs produces the same outcome. |
| `.title("...")` | Human-friendly name shown in agent interfaces. |

### Formatted Responses (`MCPResult`)

| Method | Purpose |
| :--- | :--- |
| `r.text("Format: %d", value)` | printf-style text response with no heap allocation. |
| `r.json("{\"key\":\"value\"}")` | Direct structured JSON response. |
| `r.error("Sensor not ready")` | Reports an execution failure to the LLM (`isError: true`). |
| `r.error(code, "Custom error")` | Reports a failure with a machine-readable error code. |

### Built-in Diagnostic Tools

Get hardware introspection without writing boilerplate tools:

```cpp
// Exposes the 'get_status' tool (heap stats, uptime, WiFi RSSI, chip revision)
server.enableStatus();

// Exposes 'read_pin' and 'write_pin', strictly restricted to the listed GPIOs
server.enableGpio({2, 4, 15});
```

---

## Security and Authentication

### Constant-Time Bearer Authentication

```cpp
server.bearer("my-super-secret-token");
```

- Unauthorized requests are rejected with `HTTP 401 Unauthorized` and JSON-RPC error `-32600`.
- Token comparison uses constant-time XOR operations to prevent timing side-channel attacks.
- If no token is set, a warning is printed to `Serial` on boot:

  ```text
  [ESP32-MCP] WARNING: Server running with NO authentication (bearer token not set).
  ```

### Origin Isolation (CORS)

Browser-based LLM clients are blocked by default. Allow only the origins you trust:

```cpp
server.allowOrigin("https://agent.mydomain.com");
```

> **Security note:** The server speaks plain HTTP. Run it only on trusted networks, or place it behind a TLS-terminating reverse proxy or VPN when exposing it beyond your LAN. Never expose an unauthenticated device to the internet.

---

## Connecting MCP Clients

The ESP32 serves MCP on the `/mcp` HTTP endpoint, for example `http://192.168.1.150/mcp`.

### Claude Desktop

Add the server to your `claude_desktop_config.json`:

```json
{
  "mcpServers": {
    "esp32-node": {
      "url": "http://192.168.1.150/mcp",
      "headers": {
        "Authorization": "Bearer my-super-secret-token"
      }
    }
  }
}
```

> **Tip:** If you use mDNS, you can target `http://esp32-node.local/mcp`.

### Other MCP Clients

Any MCP client that supports HTTP transport can connect using the same endpoint URL and `Authorization` header.

---

## Testing with cURL

You can exercise the server directly from a terminal.

**1. Discover the server (`server/discover`)**

```bash
curl -X POST http://192.168.1.150/mcp \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer my-super-secret-token" \
  -H "MCP-Protocol-Version: 2026-07-28" \
  -H "Mcp-Method: server/discover" \
  -d '{"jsonrpc":"2.0","id":1,"method":"server/discover"}'
```

**2. List registered tools (`tools/list`)**

```bash
curl -X POST http://192.168.1.150/mcp \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer my-super-secret-token" \
  -H "MCP-Protocol-Version: 2026-07-28" \
  -H "Mcp-Method: tools/list" \
  -d '{"jsonrpc":"2.0","id":2,"method":"tools/list"}'
```

**3. Call a tool (`tools/call`)**

```bash
curl -X POST http://192.168.1.150/mcp \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer my-super-secret-token" \
  -H "MCP-Protocol-Version: 2026-07-28" \
  -H "Mcp-Method: tools/call" \
  -H "Mcp-Name: set_led" \
  -d '{"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"set_led","arguments":{"state":true}}}'
```

---

## Configuration and Memory Tuning

All memory usage is statically bounded. Override the defaults with `build_flags` in `platformio.ini`:

| Macro | Default | Description |
| :--- | :--- | :--- |
| `MCP_MAX_REQUEST_SIZE` | `4096` bytes | Maximum request body. Larger payloads return `HTTP 413 Payload Too Large`. |
| `MCP_MAX_RESPONSE_SIZE` | `4096` bytes | Maximum JSON-RPC response buffer. Overflows abort safely with code `-32603`. |
| `MCP_MAX_TOOLS` | `16` | Maximum registered tools in the static table (~12 KB RAM). |
| `MCP_MAX_PARAMS` | `16` | Maximum parameters per tool. |
| `MCP_MAX_JSON_DEPTH` | `16` | Maximum JSON nesting depth (guards against parser stack exhaustion). |

Example:

```ini
build_flags =
    -DMCP_MAX_REQUEST_SIZE=8192
    -DMCP_MAX_TOOLS=24
```

### Synchronous Mode (Without the FreeRTOS Task)

For single-threaded, polling-based projects:

```cpp
server.setUseBackgroundTask(false);
server.begin();

void loop() {
    server.handleClient(); // Manually process incoming requests
}
```

---

## Performance and Memory Benchmark

Measured with `examples/06_PerformanceTest/06_PerformanceTest.ino` over **10,000 consecutive JSON-RPC requests**:

```text
--- Heap Stability Benchmark (10,000 requests) ---
Free heap before: 295,000 bytes
Free heap after:  295,000 bytes
Heap delta:       0 bytes (0 bytes leaked)
--- Benchmark Complete ---
```

| Metric | Result |
| :--- | :--- |
| **Heap leak** | 0 bytes after 10,000 requests |
| **Flash footprint** | ~824 KB (including the ESP32 WiFi stack, TCP/IP, and FreeRTOS kernel) |
| **Library RAM** | Under 8 KB statically allocated |

## Contributing

Contributions are welcome. Please read the [Contributing Guide](CONTRIBUTING.md), which covers:

- Conventional Commit message standards (`feat:`, `fix:`, `docs:`)
- Running the native unit test suite (`pio test -e native`)
- Architectural rules for embedded C++ development

If you are upgrading from an earlier version, see the [Migration Guide](MIGRATION.md).

---

## License

Released under the [MIT License](LICENSE).

---

<sub>**Keywords:** ESP32 MCP server, Model Context Protocol ESP32, ESP32 AI agent, ESP32 LLM control, Arduino MCP library, PlatformIO MCP, IoT AI integration, microcontroller MCP, Claude ESP32, JSON-RPC ESP32, FreeRTOS HTTP server.</sub>