# ESP32-MCP

[![Automated Tests & Verification](https://github.com/pedro-pedrosa/esp32-mcp/actions/workflows/ci.yml/badge.svg)](https://github.com/pedro-pedrosa/esp32-mcp/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Release](https://img.shields.io/github/v/release/pedro-pedrosa/esp32-mcp)](https://github.com/pedro-pedrosa/esp32-mcp/releases)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/pedro-pedrosa/library/ESP32-MCP.svg)](https://registry.platformio.org/libraries/pedro-pedrosa/ESP32-MCP)

**ESP32-MCP** is a lightweight, zero-dependency C++ library that turns an ESP32 into a high-performance Model Context Protocol (MCP) server over HTTP with a fluent declarative tool API, direct variable binding, and zero dynamic memory allocations in the request path.

---

## 1. Quick Start in 30 Seconds

No external libraries or JSON parsers required. Just plug into PlatformIO or Arduino IDE:

```cpp
#include <WiFi.h>
#include <ESP32MCP.h>

MCPServer server("ESP32-Node", "1.0.0");
static bool ledState = false;

void setup() {
    Serial.begin(115200);
    WiFi.begin("YOUR_SSID", "YOUR_PASSWORD");
    while (WiFi.status() != WL_CONNECTED) delay(500);

    pinMode(LED_BUILTIN, OUTPUT);

    // Fluent declarative tool with direct variable binding
    server.tool("set_led", "Turns onboard LED ON or OFF")
          .destructive()
          .param("state", &ledState, "true for ON, false for OFF")
          .handle([](MCPResult& r) {
              digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
              r.text("LED is now %s", ledState ? "ON" : "OFF");
          });

    server.begin(); // Starts HTTP server & background FreeRTOS task
}

void loop() {
    // loop() remains completely free for your application!
    delay(1000);
}
```

---

## 2. Connecting an MCP Client

### Claude Desktop
In your `claude_desktop_config.json`:

```json
{
  "mcpServers": {
    "esp32": {
      "url": "http://192.168.1.150/mcp"
    }
  }
}
```
*Tip: If using mDNS, you can reach the device via `http://esp32-node.local/mcp`.*

### Cursor & Other HTTP Clients
Point your MCP client configuration directly to:
```text
http://<ESP32_IP_OR_MDNS>/mcp
```
If bearer authentication is enabled, pass the Authorization header:
```text
Authorization: Bearer <YOUR_TOKEN>
```

### TLS & Reverse Proxy (Production / Public Networks)
The ESP32 serves plaintext HTTP over local WiFi. When connecting from remote clients or across public networks, place the ESP32 behind a lightweight reverse proxy (such as Caddy or Nginx) to terminate TLS/HTTPS.

**Example `Caddyfile` (~10 lines):**
```caddy
mcp.example.com {
    reverse_proxy 192.168.1.150:80 {
        header_up Host {upstream_hostport}
        header_up X-Real-IP {remote_host}
    }
}
```

---

## 3. Defining Tools

### Fluent Declarative API with Variable Binding
Define inputs, validate ranges, and bind directly to variables without manually parsing JSON or writing schemas:

```cpp
static bool state = false;
static int brightness = 255;
static char mode[32] = "auto";

server.tool("configure_light", "Controls lighting state and brightness")
      .destructive()
      .title("Smart Light Controller")
      .param("state", &state, "Power state: true or false")
      .param("brightness", &brightness, 0, 255)          // Validated min / max
      .param("mode", mode, sizeof(mode), "Operating mode") // String with buffer limit
      .handle([](MCPResult& r) {
          // Variables are already validated and populated!
          r.text("Light set to %s (%d) in mode %s", state ? "ON" : "OFF", brightness, mode);
      });
```

#### Supported Parameter Types
- `bool*`
- `int*`, `long*` (with optional `min`, `max` validation)
- `float*`, `double*` (with optional `min`, `max` validation)
- `char*`, `size_t capacity` (zero-copy string slicing)
- `.optional()`: makes the parameter optional (missing keys keep existing variable values).

#### Tool Annotations
Declare tool behavior for LLMs following the MCP specification:
- `.readOnly()`: Tool has no side-effects (`readOnly: true`).
- `.destructive()`: Tool alters hardware state (`destructive: true`).
- `.idempotent()`: Tool produces the same effect if called repeatedly (`idempotent: true`).
- `.title("...")`: Human-readable title for UI presentation.

#### MCPResult Responses
- `r.text(fmt, ...)`: Printf-style formatted text without dynamic heap allocation.
- `r.json(jsonString)`: Structured JSON response.
- `r.error(msg)`: Tool execution error (`isError: true`).
- `r.error(code, msg)`: Tool execution error with custom machine-readable code.

### Built-in Diagnostic Tools
Enable instant hardware inspection with a single call:

```cpp
// Exposes 'get_status' tool (heap, uptime, WiFi RSSI, chip model, firmware version)
server.enableStatus();

// Exposes 'read_pin' and 'write_pin' restricted STRICTLY to allowed GPIOs
server.enableGpio({2, 4, 15});
```

### Advanced Raw Schema Escape Hatch
If you need custom complex schemas, pass the JSON Schema string directly with zero copying:

```cpp
const char* customSchema = "{\"type\":\"object\",\"properties\":{\"raw\":{\"type\":\"string\"}}}";

server.tool("advanced_tool", "Custom raw schema", customSchema, [](const MCPArguments& args) {
    char rawBuf[64];
    args.getString("raw", rawBuf, sizeof(rawBuf));
    return MCPResult::makeText("Echo: %s", rawBuf);
});
```

---

## 4. Configuration & Memory Limits

ESP32-MCP uses fixed static buffers to guarantee deterministic memory usage. Configure buffer sizes via build flags or header defines:

| Macro | Default | Description & RAM Impact |
| :--- | :--- | :--- |
| `MCP_MAX_REQUEST_SIZE` | `4096` bytes | Maximum allowed HTTP request body. Larger payloads return HTTP 413. |
| `MCP_MAX_RESPONSE_SIZE` | `4096` bytes | Maximum JSON-RPC response buffer. Overflows return protocol error `-32603`. |
| `MCP_MAX_TOOLS` | `16` | Maximum registered tools in fixed-size registry array (~12 KB RAM). |
| `MCP_MAX_PARAMS` | `16` | Maximum parameters per fluent tool. |
| `MCP_MAX_JSON_DEPTH` | `16` | Maximum JSON parser nesting depth (guards against recursion stack overflow). |

### Cache TTL
Set cache expiration hint for tools and server capabilities:
```cpp
server.setCacheTtl(120000); // 2 minutes in milliseconds
```

### FreeRTOS Background Task
By default, `server.begin()` automatically spawns a dedicated FreeRTOS task on Core 1 (Stack: 4096 bytes, Priority: 1). This ensures requests are processed asynchronously without blocking user logic in `loop()`.

To manage request polling manually instead:
```cpp
server.setUseBackgroundTask(false);
server.begin();

void loop() {
    server.handleClient(); // Manual dispatch
}
```

---

## 5. Security

### Bearer Token Authentication
Protect your hardware from unauthorized invocations using constant-time verified tokens:

```cpp
server.bearer("my-super-secret-token");
```
- Unauthorized requests receive `HTTP 401 Unauthorized` with JSON-RPC `-32600`.
- Comparison uses constant-time XOR to prevent timing side-channel attacks.
- If no token is set, a serial warning is logged on startup:
  `[ESP32-MCP] WARNING: Server running with NO authentication (bearer token not set). Accessible to anyone on the network!`

### Strict CORS (Closed by Default)
Browser-based LLM clients running across web origins are blocked by default:
```cpp
// Allow only your trusted frontend origin
server.allowOrigin("https://dashboard.example.com");
```
- Requests from untrusted origins or CORS preflight OPTIONS from unknown origins receive `HTTP 403 Forbidden`.

> [!WARNING]
> Never expose an unauthenticated ESP32 server to public or shared networks without a bearer token and a TLS reverse proxy.

---

## 6. Protocol & Compatibility

- **Native Version (`2026-07-28`)**: Stateless MCP protocol.
  - Mandatory `server/discover` endpoint.
  - HTTP headers validation: `MCP-Protocol-Version`, `Mcp-Method`, `Mcp-Name`. Mismatches between headers and body return `HTTP 400 Bad Request`.
  - Cache TTL hints (`ttlMs`) included in discover and tool listing.
  - Deterministic tool ordering in `tools/list`.
  - `id` preservation (numbers and strings echoed verbatim).
- **Legacy Compatibility (`2024-11-05`)**:
  - Automatically negotiates and supports legacy clients sending `initialize`, `notifications/initialized`, and `ping`.

---

## 7. Performance & Memory Footprint

### Memory Benchmark (Zero Heap Leaks)
Tested with `examples/06_PerformanceTest/06_PerformanceTest.ino` running 10,000 consecutive JSON-RPC calls on ESP32:

```text
--- Heap Stability Benchmark (10,000 requests) ---
Free heap before: 295,000 bytes
Free heap after:  295,000 bytes
Heap delta:       0 bytes (0 bytes leaked!)
--- Benchmark Complete ---
```

### Build Footprint (Minimal Build)
- **RAM**: ~100 KB total used (including FreeRTOS, WiFi stack, and WebServer; library data footprint is < 8 KB statically allocated).
- **Flash**: ~824 KB (62.9% on standard 4MB ESP32 module, including full WiFi and networking stack).
- **Zero Allocations**: Zero calls to `malloc` / `new` / `free` during request parsing, dispatch, or response generation.

---

## 8. Testing with cURL

### 1. Discover Capabilities (`server/discover`)
```bash
curl -X POST http://esp32-node.local/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2026-07-28" \
  -H "Mcp-Method: server/discover" \
  -d '{"jsonrpc":"2.0","id":1,"method":"server/discover"}'
```

### 2. List Tools (`tools/list`)
```bash
curl -X POST http://esp32-node.local/mcp \
  -H "Content-Type: application/json" \
  -H "MCP-Protocol-Version: 2026-07-28" \
  -H "Mcp-Method: tools/list" \
  -d '{"jsonrpc":"2.0","id":"req-01","method":"tools/list"}'
```

### 3. Call Tool (`tools/call`) with Bearer Auth
```bash
curl -X POST http://esp32-node.local/mcp \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer my-super-secret-token" \
  -H "MCP-Protocol-Version: 2026-07-28" \
  -H "Mcp-Method: tools/call" \
  -H "Mcp-Name: set_led" \
  -d '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"set_led","arguments":{"state":true}}}'
```

---

## 9. Migration from v0.1.0

| Feature | v0.1.0 (Old) | v0.2.0 (New) |
| :--- | :--- | :--- |
| **Dependencies** | Requires `ArduinoJson` v7 | **Zero dependencies** (internal zero-alloc scanner & writer) |
| **Hot Path Strings** | Heavy `String` allocations & copies | `const char*` + size, fixed buffers, `snprintf` |
| **Tool Declaration** | Manual JSON schema string + manual `args.has()` | Fluent `.param(&var)` with auto-generated schema |
| **Validation** | Manual checks inside handlers | Automatic type and range validation (error `-32602`) |
| **Dispatch Model** | Blocking in `loop()` via `server.handleClient()` | Automatic FreeRTOS background task (Core 1) |
| **Protocol** | `2024-11-05` (stateful handshake) | `2026-07-28` (stateless + discovery + legacy fallback) |
| **Security** | Open CORS (`*`), no authentication | Bearer auth + constant-time check + closed CORS |
| **Payload Guard** | Unbounded | HTTP 413 on request overflow |

---

## 10. Contributing & License

### Running Tests Locally
Tests use PlatformIO native environment with Unity:

```bash
# Run unit test suite on native host
pio test -e native
```

All contributions must follow [Conventional Commits](https://www.conventionalcommits.org/) (`feat:`, `fix:`, `docs:`, `test:`, `refactor:`) to support automated releases via Release Please.

### License
Released under the [MIT License](LICENSE).
