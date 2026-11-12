#include <iostream>
#include <cassert>
#include "Arduino.h"
#include "ArduinoJson.h"
#include "../src/mcp/MCPTypes.h"
#include "../src/mcp/MCPTool.h"
#include "../src/mcp/MCPToolRegistry.h"
#include "../src/mcp/MCPJsonRpc.h"
#include "../src/mcp/MCPServer.h"
#include "../src/mcp/json/MCPJsonScanner.h"
#include "../src/mcp/json/MCPJsonWriter.h"

int tests_passed = 0;
int tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[FAIL] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
            tests_failed++; \
            return; \
        } \
    } while(0)

#define RUN_TEST(fn) \
    do { \
        int before = tests_failed; \
        std::cout << "[RUNNING] " << #fn << "..."; \
        fn(); \
        if (tests_failed == before) { \
            tests_passed++; \
            std::cout << " [PASS]" << std::endl; \
        } \
    } while(0)

void test_json_rpc_parse_error() {
    MCPServer server;
    String response = server.processMessage("invalid json {");
    JsonDocument doc;
    deserializeJson(doc, response);
    TEST_ASSERT(doc["jsonrpc"] == "2.0", "jsonrpc must be 2.0");
    TEST_ASSERT(doc["error"]["code"] == -32700, "Error code must be -32700");
}

void test_json_rpc_empty_message() {
    MCPServer server;
    String response = server.processMessage("");
    JsonDocument doc;
    deserializeJson(doc, response);
    TEST_ASSERT(doc["jsonrpc"] == "2.0", "jsonrpc must be 2.0");
    TEST_ASSERT(doc["error"]["code"] == -32700, "Error code must be -32700");
}

void test_json_rpc_invalid_request() {
    MCPServer server;
    String response = server.processMessage("{\"hello\":\"world\"}");
    JsonDocument doc;
    deserializeJson(doc, response);
    TEST_ASSERT(doc["error"]["code"] == -32600, "Error code must be -32600");
}

void test_json_rpc_unknown_method() {
    MCPServer server;
    String req = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"unknown/method\",\"params\":{}}";
    String response = server.processMessage(req);
    JsonDocument doc;
    deserializeJson(doc, response);
    TEST_ASSERT(doc["id"] == 1, "ID must match");
    TEST_ASSERT(doc["error"]["code"] == -32601, "Error code must be -32601");
}

void test_mcp_server_discover() {
    MCPServer server("ESP32-DiscoverNode", "2.0.0");
    server.setInstructions("Autonomous IoT Node");
    String req = "{\"jsonrpc\":\"2.0\",\"id\":\"disc-1\",\"method\":\"server/discover\",\"params\":{}}";
    String response = server.processMessage(req);
    JsonDocument doc;
    deserializeJson(doc, response);

    TEST_ASSERT(doc["id"] == "disc-1", "ID must match");
    TEST_ASSERT(doc["result"]["supportedVersions"][0] == "2026-07-28", "Supported version must be 2026-07-28");
    TEST_ASSERT(doc["result"]["capabilities"]["tools"]["listChanged"] == false, "Capabilities tools mismatch");
    TEST_ASSERT(doc["result"]["instructions"] == "Autonomous IoT Node", "Instructions mismatch");
    TEST_ASSERT(doc["result"]["_meta"]["io.modelcontextprotocol/serverInfo"]["name"] == "ESP32-DiscoverNode", "Meta server name mismatch");
    TEST_ASSERT(doc["result"]["serverInfo"]["name"] == "ESP32-DiscoverNode", "ServerInfo name mismatch");
}

void test_mcp_initialize_2026() {
    MCPServer server("TestESP32", "1.2.3");
    server.setInstructions("Test instructions for AI");
    String req = "{\"jsonrpc\":\"2.0\",\"id\":42,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2026-07-28\",\"capabilities\":{},\"clientInfo\":{\"name\":\"TestClient\",\"version\":\"1.0\"}}}";
    String response = server.processMessage(req);
    JsonDocument doc;
    deserializeJson(doc, response);

    TEST_ASSERT(doc["id"] == 42, "ID must be 42");
    TEST_ASSERT(doc["result"]["protocolVersion"] == "2026-07-28", "Protocol version must be 2026-07-28");
    TEST_ASSERT(doc["result"]["serverInfo"]["name"] == "TestESP32", "Server name mismatch");
    TEST_ASSERT(doc["result"]["serverInfo"]["version"] == "1.2.3", "Server version mismatch");
    TEST_ASSERT(doc["result"]["instructions"] == "Test instructions for AI", "Instructions mismatch");
    TEST_ASSERT(doc["result"]["capabilities"]["tools"]["listChanged"] == false, "Capabilities tools mismatch");
}

void test_mcp_ping() {
    MCPServer server;
    String req = "{\"jsonrpc\":\"2.0\",\"id\":\"ping-123\",\"method\":\"ping\"}";
    String response = server.processMessage(req);
    JsonDocument doc;
    deserializeJson(doc, response);

    TEST_ASSERT(doc["id"] == "ping-123", "ID must match");
    TEST_ASSERT(!doc["result"].isNull(), "Must contain result object");
}

void test_mcp_notification_no_response() {
    MCPServer server;
    String req = "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}";
    String response = server.processMessage(req);
    TEST_ASSERT(response.length() == 0, "Notifications must return empty response");
}

void test_tools_list() {
    MCPServer server;
    server.tool("get_temp", "Reads temperature", [](const MCPArguments& args) {
        return MCPResult::text("25.5 C");
    });
    server.tool(
        "set_led",
        "Control LED",
        "{\"type\":\"object\",\"properties\":{\"state\":{\"type\":\"boolean\"}},\"required\":[\"state\"]}",
        [](const MCPArguments& args) {
            return MCPResult::text("OK");
        }
    );

    String req = "{\"jsonrpc\":\"2.0\",\"id\":\"req-list\",\"method\":\"tools/list\"}";
    String response = server.processMessage(req);
    JsonDocument doc;
    deserializeJson(doc, response);

    TEST_ASSERT(doc["id"] == "req-list", "ID must match");
    JsonArray tools = doc["result"]["tools"].as<JsonArray>();
    TEST_ASSERT(tools.size() == 2, "Must have 2 tools");
    TEST_ASSERT(tools[0]["name"] == "get_temp", "First tool name mismatch");
    TEST_ASSERT(tools[0]["inputSchema"]["type"] == "object", "Default schema type must be object");
    TEST_ASSERT(tools[1]["name"] == "set_led", "Second tool name mismatch");
    TEST_ASSERT(tools[1]["inputSchema"]["type"] == "object", "Schema type must be object");
    TEST_ASSERT(tools[1]["inputSchema"]["properties"]["state"]["type"] == "boolean", "Schema property type mismatch");
}

void test_tools_call_success() {
    MCPServer server;
    server.tool("add_numbers", "Adds two numbers", [](const MCPArguments& args) {
        int a = args.getInt("a");
        int b = args.getInt("b");
        int sum = a + b;
        return MCPResult::text(String(sum));
    });

    String req = "{\"jsonrpc\":\"2.0\",\"id\":10,\"method\":\"tools/call\",\"params\":{\"name\":\"add_numbers\",\"arguments\":{\"a\":15,\"b\":27}}}";
    String response = server.processMessage(req);
    JsonDocument doc;
    deserializeJson(doc, response);

    TEST_ASSERT(doc["id"] == 10, "ID must be 10");
    TEST_ASSERT(doc["result"]["isError"] == false, "isError must be false");
    JsonArray content = doc["result"]["content"].as<JsonArray>();
    TEST_ASSERT(content.size() == 1, "Content array size must be 1");
    TEST_ASSERT(content[0]["type"] == "text", "Content type must be text");
    TEST_ASSERT(content[0]["text"] == "42", "Addition result must be 42");
}

void test_tools_call_error_handling() {
    MCPServer server;
    server.tool("failing_tool", "Tool that fails", [](const MCPArguments& args) {
        return MCPResult::error("DEVICE_BUSY", "Sensor currently busy");
    });

    String req = "{\"jsonrpc\":\"2.0\",\"id\":11,\"method\":\"tools/call\",\"params\":{\"name\":\"failing_tool\",\"arguments\":{}}}";
    String response = server.processMessage(req);
    JsonDocument doc;
    deserializeJson(doc, response);

    TEST_ASSERT(doc["id"] == 11, "ID must be 11");
    TEST_ASSERT(doc["result"]["isError"] == true, "isError must be true");
    TEST_ASSERT(doc["result"]["errorCode"] == "DEVICE_BUSY", "Error code mismatch");
    TEST_ASSERT(doc["result"]["content"][0]["text"] == "Sensor currently busy", "Error message mismatch");
}

void test_tools_call_unknown_tool() {
    MCPServer server;
    String req = "{\"jsonrpc\":\"2.0\",\"id\":12,\"method\":\"tools/call\",\"params\":{\"name\":\"non_existent\",\"arguments\":{}}}";
    String response = server.processMessage(req);
    JsonDocument doc;
    deserializeJson(doc, response);

    TEST_ASSERT(doc["id"] == 12, "ID must be 12");
    TEST_ASSERT(doc["error"]["code"] == -32602, "Error code must be -32602 for unknown tool");
}

void test_tools_overwrite_registration() {
    MCPServer server;
    server.tool("sensor", "V1 description", [](const MCPArguments& args) {
        return MCPResult::text("V1");
    });
    server.tool("sensor", "V2 description", [](const MCPArguments& args) {
        return MCPResult::text("V2");
    });

    TEST_ASSERT(server.getToolRegistry().count() == 1, "Must have only 1 tool after overwrite");
    const MCPTool* t = server.getToolRegistry().findTool("sensor");
    TEST_ASSERT(t != nullptr, "Tool must exist");
    TEST_ASSERT(t->description == "V2 description", "Description must be updated");

    String req = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\",\"params\":{\"name\":\"sensor\",\"arguments\":{}}}";
    String response = server.processMessage(req);
    JsonDocument doc;
    deserializeJson(doc, response);
    TEST_ASSERT(doc["result"]["content"][0]["text"] == "V2", "Result must come from updated handler");
}

void test_mcp_arguments_types() {
    JsonDocument doc;
    doc["str_key"] = "hello";
    doc["int_key"] = 123;
    doc["float_key"] = 45.67;
    doc["bool_key"] = true;

    MCPArguments args(doc.as<JsonObjectConst>());
    TEST_ASSERT(args.has("str_key"), "Must have str_key");
    TEST_ASSERT(!args.has("missing_key"), "Must not have missing_key");
    TEST_ASSERT(args.getString("str_key") == "hello", "String value mismatch");
    TEST_ASSERT(args.getInt("int_key") == 123, "Int value mismatch");
    TEST_ASSERT(args.getFloat("float_key") > 45.0f, "Float value mismatch");
    TEST_ASSERT(args.getBool("bool_key") == true, "Bool value mismatch");
    TEST_ASSERT(args.getInt("missing_key", 999) == 999, "Default value mismatch");
}

void test_json_scanner_escapes_and_surrogates() {
    const char* json = "{\"escapes\":\"quote:\\\" slash:\\/ backslash:\\\\ newline:\\n tab:\\t return:\\r formfeed:\\f backspace:\\b unicode:\\u0041 smiley:\\uD83D\\uDE00\"}";
    MCPScanError err = MCPScanError::None;
    bool ok = MCPJsonScanner::validate(json, strlen(json), &err);
    TEST_ASSERT(ok, "JSON with escapes and surrogate pairs must be valid");
    TEST_ASSERT(err == MCPScanError::None, "No scan error expected");

    MCPJsonSlice val = MCPJsonScanner::findKey(json, strlen(json), "escapes");
    TEST_ASSERT(val.isValid(), "Key escapes must be found");
    TEST_ASSERT(val.isString(), "Value must be string");

    char decoded[256];
    size_t dLen = 0;
    TEST_ASSERT(val.decodeString(decoded, sizeof(decoded), &dLen), "Decoding escapes must succeed");
    TEST_ASSERT(strstr(decoded, "quote:\"") != nullptr, "Escaped quote must be decoded");
    TEST_ASSERT(strstr(decoded, "slash:/") != nullptr, "Escaped slash must be decoded");
    TEST_ASSERT(strstr(decoded, "backslash:\\") != nullptr, "Escaped backslash must be decoded");
    TEST_ASSERT(strstr(decoded, "unicode:A") != nullptr, "Escaped unicode \\u0041 must decode to A");
    // UTF-8 representation of U+1F600 (smiley) is \xF0\x9F\x98\x80
    const char smileyUtf8[] = "\xF0\x9F\x98\x80";
    TEST_ASSERT(strstr(decoded, smileyUtf8) != nullptr, "Surrogate pair must decode to UTF-8 smiley");
}

void test_json_scanner_truncated_unicode() {
    const char* json1 = "{\"str\":\"\\u00\"}";
    MCPScanError err = MCPScanError::None;
    TEST_ASSERT(!MCPJsonScanner::validate(json1, strlen(json1), &err), "Truncated \\u must fail validation");

    const char* json2 = "{\"str\":\"\\uD83D\"}";
    TEST_ASSERT(!MCPJsonScanner::validate(json2, strlen(json2), &err), "Unpaired surrogate must fail validation");

    const char* json3 = "{\"str\":\"\\uD83D\\u0041\"}";
    TEST_ASSERT(!MCPJsonScanner::validate(json3, strlen(json3), &err), "Invalid low surrogate must fail validation");
}

void test_json_scanner_integer_overflow() {
    const char* json = "{\"val\":999999999999999999999999999999}";
    MCPScanError err = MCPScanError::None;
    bool ok = MCPJsonScanner::validate(json, strlen(json), &err);
    TEST_ASSERT(!ok, "Integer overflowing long must fail validation");
    TEST_ASSERT(err == MCPScanError::IntegerOverflow, "Error must be IntegerOverflow");
}

void test_json_scanner_depth_limit() {
    const char* jsonDeep = "[[[[[[[[[[[[[[[[[[[[1]]]]]]]]]]]]]]]]]]]]";
    MCPScanError err = MCPScanError::None;
    bool ok = MCPJsonScanner::validate(jsonDeep, strlen(jsonDeep), &err, 5);
    TEST_ASSERT(!ok, "Nesting deeper than maxDepth must fail");
    TEST_ASSERT(err == MCPScanError::DepthExceeded, "Error must be DepthExceeded");
}

void test_json_scanner_duplicate_keys() {
    const char* json = "{\"key\":1, \"key\":2}";
    MCPScanError err = MCPScanError::None;
    bool ok = MCPJsonScanner::validate(json, strlen(json), &err);
    TEST_ASSERT(!ok, "Duplicate keys must fail validation");
    TEST_ASSERT(err == MCPScanError::DuplicateKey, "Error must be DuplicateKey");
}

void test_json_scanner_truncated_json() {
    const char* json1 = "{\"key\":\"value";
    MCPScanError err = MCPScanError::None;
    TEST_ASSERT(!MCPJsonScanner::validate(json1, strlen(json1), &err), "Unclosed string must fail");

    const char* json2 = "{\"key\":";
    TEST_ASSERT(!MCPJsonScanner::validate(json2, strlen(json2), &err), "Missing value must fail");

    const char* json3 = "[1, 2,";
    TEST_ASSERT(!MCPJsonScanner::validate(json3, strlen(json3), &err), "Unclosed array must fail");
}

void test_json_scanner_trailing_trash() {
    const char* json = "{\"a\":1} trailing";
    MCPScanError err = MCPScanError::None;
    bool ok = MCPJsonScanner::validate(json, strlen(json), &err);
    TEST_ASSERT(!ok, "Trailing non-whitespace must fail");
    TEST_ASSERT(err == MCPScanError::TrailingTrash, "Error must be TrailingTrash");
}

void test_json_scanner_buffer_overflow() {
    char bigBuffer[MCP_MAX_REQUEST_SIZE + 10];
    memset(bigBuffer, ' ', sizeof(bigBuffer));
    bigBuffer[0] = '{';
    bigBuffer[1] = '}';
    MCPScanError err = MCPScanError::None;
    bool ok = MCPJsonScanner::validate(bigBuffer, sizeof(bigBuffer), &err);
    TEST_ASSERT(!ok, "Payload exceeding MCP_MAX_REQUEST_SIZE must fail");
    TEST_ASSERT(err == MCPScanError::BufferOverflow, "Error must be BufferOverflow");
}

void test_json_writer_escapes_and_controls() {
    char buf[256];
    MCPJsonWriter writer(buf, sizeof(buf));
    writer.startObject();
    // Test string containing quote, backslash, slash, and control chars (\n, \r, \t, 0x01)
    const char rawInput[] = "quote:\" backslash:\\ slash:/ newline:\n byte1:\x01";
    writer.addString("data", rawInput);
    writer.endObject();

    TEST_ASSERT(!writer.isOverflow(), "Writer must not overflow");
    const char* out = writer.c_str();

    TEST_ASSERT(strstr(out, "\\\"") != nullptr, "Quote must be escaped as \\\"");
    TEST_ASSERT(strstr(out, "\\\\") != nullptr, "Backslash must be escaped as \\\\");
    TEST_ASSERT(strstr(out, "\\/") != nullptr, "Slash must be escaped as \\/");
    TEST_ASSERT(strstr(out, "\\n") != nullptr, "Newline must be escaped as \\n");
    TEST_ASSERT(strstr(out, "\\u0001") != nullptr, "Control char < 0x20 must be escaped as \\u0001");
}

void test_json_writer_overflow() {
    char smallBuf[20];
    MCPJsonWriter writer(smallBuf, sizeof(smallBuf));
    writer.startObject();
    bool r1 = writer.addString("long_key_name", "this value is definitely too long to fit in 20 bytes");
    writer.endObject();

    TEST_ASSERT(!r1 || writer.isOverflow(), "Write must indicate failure or overflow");
    TEST_ASSERT(writer.isOverflow(), "isOverflow must return true");
    TEST_ASSERT(writer.hasError(), "hasError must return true");
    TEST_ASSERT(strlen(writer.c_str()) < sizeof(smallBuf), "Must remain null-terminated within buffer");
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << " Running ESP32-MCP Unit Test Suite      " << std::endl;
    std::cout << "========================================" << std::endl;

    RUN_TEST(test_json_rpc_parse_error);
    RUN_TEST(test_json_rpc_empty_message);
    RUN_TEST(test_json_rpc_invalid_request);
    RUN_TEST(test_json_rpc_unknown_method);
    RUN_TEST(test_mcp_server_discover);
    RUN_TEST(test_mcp_initialize_2026);
    RUN_TEST(test_mcp_ping);
    RUN_TEST(test_mcp_notification_no_response);
    RUN_TEST(test_tools_list);
    RUN_TEST(test_tools_call_success);
    RUN_TEST(test_tools_call_error_handling);
    RUN_TEST(test_tools_call_unknown_tool);
    RUN_TEST(test_tools_overwrite_registration);
    RUN_TEST(test_mcp_arguments_types);

    RUN_TEST(test_json_scanner_escapes_and_surrogates);
    RUN_TEST(test_json_scanner_truncated_unicode);
    RUN_TEST(test_json_scanner_integer_overflow);
    RUN_TEST(test_json_scanner_depth_limit);
    RUN_TEST(test_json_scanner_duplicate_keys);
    RUN_TEST(test_json_scanner_truncated_json);
    RUN_TEST(test_json_scanner_trailing_trash);
    RUN_TEST(test_json_scanner_buffer_overflow);
    RUN_TEST(test_json_writer_escapes_and_controls);
    RUN_TEST(test_json_writer_overflow);

    std::cout << "========================================" << std::endl;
    std::cout << " Test Summary: " << tests_passed << " passed, " << tests_failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;

    return tests_failed == 0 ? 0 : 1;
}
