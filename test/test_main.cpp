#include <iostream>
#include <cassert>
#include "Arduino.h"
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
    TEST_ASSERT(response.indexOf("\"jsonrpc\":\"2.0\"") != -1, "jsonrpc must be 2.0");
    TEST_ASSERT(response.indexOf("\"code\":-32700") != -1, "Error code must be -32700");
}

void test_json_rpc_empty_message() {
    MCPServer server;
    String response = server.processMessage("");
    TEST_ASSERT(response.indexOf("\"jsonrpc\":\"2.0\"") != -1, "jsonrpc must be 2.0");
    TEST_ASSERT(response.indexOf("\"code\":-32700") != -1, "Error code must be -32700");
}

void test_json_rpc_invalid_request() {
    MCPServer server;
    String response = server.processMessage("{\"hello\":\"world\"}");
    TEST_ASSERT(response.indexOf("\"code\":-32600") != -1, "Error code must be -32600");
}

void test_json_rpc_unknown_method() {
    MCPServer server;
    String req = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"unknown/method\",\"params\":{}}";
    String response = server.processMessage(req);
    TEST_ASSERT(response.indexOf("\"id\":1") != -1, "ID must match");
    TEST_ASSERT(response.indexOf("\"code\":-32601") != -1, "Error code must be -32601");
}

void test_mcp_server_discover() {
    MCPServer server("ESP32-DiscoverNode", "2.0.0");
    server.setInstructions("Autonomous IoT Node");
    String req = "{\"jsonrpc\":\"2.0\",\"id\":\"disc-1\",\"method\":\"server/discover\",\"params\":{}}";
    String response = server.processMessage(req);

    TEST_ASSERT(response.indexOf("\"id\":\"disc-1\"") != -1, "ID must match disc-1");
    TEST_ASSERT(response.indexOf("\"supportedVersions\":[\"2026-07-28\",\"2024-11-05\"]") != -1, "Supported versions must match");
    TEST_ASSERT(response.indexOf("\"listChanged\":false") != -1, "Capabilities tools mismatch");
    TEST_ASSERT(response.indexOf("\"instructions\":\"Autonomous IoT Node\"") != -1, "Instructions mismatch");
    TEST_ASSERT(response.indexOf("\"name\":\"ESP32-DiscoverNode\"") != -1, "ServerInfo name mismatch");
}

void test_mcp_initialize_2026() {
    MCPServer server("TestESP32", "1.2.3");
    server.setInstructions("Test instructions for AI");
    String req = "{\"jsonrpc\":\"2.0\",\"id\":42,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2026-07-28\",\"capabilities\":{},\"clientInfo\":{\"name\":\"TestClient\",\"version\":\"1.0\"}}}";
    String response = server.processMessage(req);

    TEST_ASSERT(response.indexOf("\"id\":42") != -1, "ID must be 42");
    TEST_ASSERT(response.indexOf("\"protocolVersion\":\"2026-07-28\"") != -1, "Protocol version must be 2026-07-28");
    TEST_ASSERT(response.indexOf("\"name\":\"TestESP32\"") != -1, "Server name mismatch");
    TEST_ASSERT(response.indexOf("\"version\":\"1.2.3\"") != -1, "Server version mismatch");
    TEST_ASSERT(response.indexOf("\"instructions\":\"Test instructions for AI\"") != -1, "Instructions mismatch");
    TEST_ASSERT(response.indexOf("\"listChanged\":false") != -1, "Capabilities tools mismatch");
}

void test_mcp_ping() {
    MCPServer server;
    String req = "{\"jsonrpc\":\"2.0\",\"id\":\"ping-123\",\"method\":\"ping\"}";
    String response = server.processMessage(req);

    TEST_ASSERT(response.indexOf("\"id\":\"ping-123\"") != -1, "ID must match");
    TEST_ASSERT(response.indexOf("\"result\":{}") != -1, "Must contain result object");
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
        return MCPResult::makeText("25.5 C");
    });
    server.tool(
        "set_led",
        "Control LED",
        "{\"type\":\"object\",\"properties\":{\"state\":{\"type\":\"boolean\"}},\"required\":[\"state\"]}",
        [](const MCPArguments& args) {
            return MCPResult::makeText("OK");
        }
    );

    String req = "{\"jsonrpc\":\"2.0\",\"id\":\"req-list\",\"method\":\"tools/list\"}";
    String response = server.processMessage(req);

    TEST_ASSERT(response.indexOf("\"id\":\"req-list\"") != -1, "ID must match");
    TEST_ASSERT(response.indexOf("\"name\":\"get_temp\"") != -1, "First tool name mismatch");
    TEST_ASSERT(response.indexOf("\"name\":\"set_led\"") != -1, "Second tool name mismatch");
    TEST_ASSERT(response.indexOf("\"type\":\"boolean\"") != -1, "Schema property type mismatch");
}

void test_tools_call_success() {
    MCPServer server;
    server.tool("add_numbers", "Adds two numbers", [](const MCPArguments& args) {
        int a = args.getInt("a");
        int b = args.getInt("b");
        int sum = a + b;
        return MCPResult::makeText("%d", sum);
    });

    String req = "{\"jsonrpc\":\"2.0\",\"id\":10,\"method\":\"tools/call\",\"params\":{\"name\":\"add_numbers\",\"arguments\":{\"a\":15,\"b\":27}}}";
    String response = server.processMessage(req);

    TEST_ASSERT(response.indexOf("\"id\":10") != -1, "ID must be 10");
    TEST_ASSERT(response.indexOf("\"isError\":false") != -1, "isError must be false");
    TEST_ASSERT(response.indexOf("\"type\":\"text\"") != -1, "Content type must be text");
    TEST_ASSERT(response.indexOf("\"text\":\"42\"") != -1, "Addition result must be 42");
}

void test_tools_call_error_handling() {
    MCPServer server;
    server.tool("failing_tool", "Tool that fails", [](const MCPArguments& args) {
        return MCPResult::makeError("DEVICE_BUSY", "Sensor currently busy");
    });

    String req = "{\"jsonrpc\":\"2.0\",\"id\":11,\"method\":\"tools/call\",\"params\":{\"name\":\"failing_tool\",\"arguments\":{}}}";
    String response = server.processMessage(req);

    TEST_ASSERT(response.indexOf("\"id\":11") != -1, "ID must be 11");
    TEST_ASSERT(response.indexOf("\"isError\":true") != -1, "isError must be true");
    TEST_ASSERT(response.indexOf("\"errorCode\":\"DEVICE_BUSY\"") != -1, "Error code mismatch");
    TEST_ASSERT(response.indexOf("\"text\":\"Sensor currently busy\"") != -1, "Error message mismatch");
}

void test_tools_call_unknown_tool() {
    MCPServer server;
    String req = "{\"jsonrpc\":\"2.0\",\"id\":12,\"method\":\"tools/call\",\"params\":{\"name\":\"non_existent\",\"arguments\":{}}}";
    String response = server.processMessage(req);

    TEST_ASSERT(response.indexOf("\"id\":12") != -1, "ID must be 12");
    TEST_ASSERT(response.indexOf("\"code\":-32602") != -1, "Error code must be -32602 for unknown tool");
}

void test_tools_overwrite_registration() {
    MCPServer server;
    server.tool("sensor", "V1 description", [](const MCPArguments& args) {
        return MCPResult::makeText("V1");
    });
    server.tool("sensor", "V2 description", [](const MCPArguments& args) {
        return MCPResult::makeText("V2");
    });

    TEST_ASSERT(server.getToolRegistry().count() == 1, "Must have only 1 tool after overwrite");
    const MCPTool* t = server.getToolRegistry().findTool("sensor");
    TEST_ASSERT(t != nullptr, "Tool must exist");
    TEST_ASSERT(strcmp(t->description, "V2 description") == 0, "Description must be updated");

    String req = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\",\"params\":{\"name\":\"sensor\",\"arguments\":{}}}";
    String response = server.processMessage(req);
    TEST_ASSERT(response.indexOf("\"text\":\"V2\"") != -1, "Result must come from updated handler");
}

void test_mcp_arguments_types() {
    const char* rawJson = "{\"str_key\":\"hello\",\"int_key\":123,\"float_key\":45.67,\"bool_key\":true}";
    MCPArguments args(rawJson, strlen(rawJson));

    TEST_ASSERT(args.has("str_key"), "Must have str_key");
    TEST_ASSERT(!args.has("missing_key"), "Must not have missing_key");
    TEST_ASSERT(args.getString("str_key") == "hello", "String value mismatch");
    TEST_ASSERT(args.getInt("int_key") == 123, "Int value mismatch");
    TEST_ASSERT(args.getFloat("float_key") > 45.0f, "Float value mismatch");
    TEST_ASSERT(args.getBool("bool_key") == true, "Bool value mismatch");
    TEST_ASSERT(args.getInt("missing_key", 999) == 999, "Default value mismatch");

    char dest[32];
    TEST_ASSERT(args.getString("str_key", dest, sizeof(dest)), "Must copy to char buffer");
    TEST_ASSERT(strcmp(dest, "hello") == 0, "Buffer value must be hello");
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

void test_tool_schema_validation_and_max_tools() {
    MCPServer server;
    // Malformed JSON schema
    bool okMalformed = server.tool("bad1", "desc", "{malformed:", [](const MCPArguments&) { return MCPResult::makeText("ok"); });
    TEST_ASSERT(!okMalformed, "Malformed schema must be rejected");

    // Valid JSON but not an object
    bool okNonObject = server.tool("bad2", "desc", "[1, 2, 3]", [](const MCPArguments&) { return MCPResult::makeText("ok"); });
    TEST_ASSERT(!okNonObject, "Non-object schema must be rejected");

    // Valid object schema
    bool okGood = server.tool("good", "desc", "{\"type\":\"object\"}", [](const MCPArguments&) { return MCPResult::makeText("ok"); });
    TEST_ASSERT(okGood, "Valid object schema must be accepted");

    // Fill up to MCP_MAX_TOOLS
    server.getToolRegistry().clear();
    for (int i = 0; i < MCP_MAX_TOOLS; ++i) {
        char name[16];
        snprintf(name, sizeof(name), "t_%d", i);
        bool regOk = server.tool(name, "desc", [](const MCPArguments&) { return MCPResult::makeText("ok"); });
        TEST_ASSERT(regOk, "Registering within MCP_MAX_TOOLS limit must succeed");
    }
    // Next tool registration must fail
    bool overflowOk = server.tool("t_overflow", "desc", [](const MCPArguments&) { return MCPResult::makeText("ok"); });
    TEST_ASSERT(!overflowOk, "Registering beyond MCP_MAX_TOOLS must return false");
}

void test_id_preservation_string_and_number() {
    MCPServer server;
    // String id
    String reqStr = "{\"jsonrpc\":\"2.0\",\"id\":\"req-xyz-987\",\"method\":\"ping\"}";
    String respStr = server.processMessage(reqStr);
    TEST_ASSERT(respStr.indexOf("\"id\":\"req-xyz-987\"") != -1, "String ID must be preserved exactly");

    // Numeric id
    String reqNum = "{\"jsonrpc\":\"2.0\",\"id\":987654321,\"method\":\"ping\"}";
    String respNum = server.processMessage(reqNum);
    TEST_ASSERT(respNum.indexOf("\"id\":987654321") != -1, "Numeric ID must be preserved exactly");
}

void test_arguments_get_raw() {
    const char* raw = "{\"nested\":{\"foo\":\"bar\",\"count\":42}}";
    MCPArguments args(raw, strlen(raw));
    MCPJsonSlice slice = args.getRaw("nested");
    TEST_ASSERT(slice.isValid(), "Slice must be valid");
    TEST_ASSERT(slice.isObject(), "Slice must be object");
    TEST_ASSERT(strncmp(slice.ptr, "{\"foo\":", 7) == 0, "Slice must point to raw nested object");
}

void test_response_buffer_overflow_error() {
    MCPServer server;
    server.tool("huge_tool", "Generates huge output", [](const MCPArguments&) {
        static char huge[MCP_MAX_RESPONSE_SIZE + 500];
        memset(huge, 'A', sizeof(huge) - 1);
        huge[sizeof(huge) - 1] = '\0';
        return MCPResult::makeText(huge);
    });

    String req = "{\"jsonrpc\":\"2.0\",\"id\":\"overflow-test\",\"method\":\"tools/call\",\"params\":{\"name\":\"huge_tool\",\"arguments\":{}}}";
    String resp = server.processMessage(req);
    TEST_ASSERT(resp.indexOf("\"code\":-32603") != -1, "Must return JSON-RPC -32603 internal error");
    TEST_ASSERT(resp.indexOf("response too large") != -1, "Must report 'response too large'");
    TEST_ASSERT(resp.indexOf("\"id\":\"overflow-test\"") != -1, "Must preserve request ID in overflow response");
}

void test_fluent_tool_builder_and_validation() {
    MCPServer server;
    static bool ledState = false;
    static int brightness = 0;
    static char mode[16] = {0};

    server.tool("set_led", "Turns the onboard LED ON or OFF")
          .title("LED Controller")
          .readOnly()
          .param("state", &ledState, "true for ON, false for OFF")
          .param("brightness", &brightness, 0, 255, "PWM brightness 0-255")
          .param("mode", mode, sizeof(mode), "Operation mode").optional()
          .handle([&](MCPResult& r) {
              r.text("LED %s brightness %d", ledState ? "ON" : "OFF", brightness);
          });

    // 1. Check tools/list generated schema and annotations
    String listReq = "{\"jsonrpc\":\"2.0\",\"id\":\"list-1\",\"method\":\"tools/list\"}";
    String listResp = server.processMessage(listReq);
    TEST_ASSERT(listResp.indexOf("\"name\":\"set_led\"") != -1, "Tool name must be set_led");
    TEST_ASSERT(listResp.indexOf("\"title\":\"LED Controller\"") != -1, "Title must be present");
    TEST_ASSERT(listResp.indexOf("\"readOnly\":true") != -1, "Annotation readOnly must be true");
    TEST_ASSERT(listResp.indexOf("\"minimum\":0") != -1, "Minimum must be 0");
    TEST_ASSERT(listResp.indexOf("\"maximum\":255") != -1, "Maximum must be 255");
    TEST_ASSERT(listResp.indexOf("\"required\":[\"state\",\"brightness\"]") != -1, "Only state and brightness must be required");

    // 2. Missing required parameter 'brightness' -> -32602
    String missingReq = "{\"jsonrpc\":\"2.0\",\"id\":\"call-1\",\"method\":\"tools/call\",\"params\":{\"name\":\"set_led\",\"arguments\":{\"state\":true}}}";
    String missingResp = server.processMessage(missingReq);
    TEST_ASSERT(missingResp.indexOf("\"code\":-32602") != -1, "Missing param must return JSON-RPC -32602");
    TEST_ASSERT(missingResp.indexOf("Missing required parameter: brightness") != -1, "Error message must name missing param");

    // 3. Out of range value (brightness = 500) -> -32602
    String outOfRangeReq = "{\"jsonrpc\":\"2.0\",\"id\":\"call-2\",\"method\":\"tools/call\",\"params\":{\"name\":\"set_led\",\"arguments\":{\"state\":true,\"brightness\":500}}}";
    String outOfRangeResp = server.processMessage(outOfRangeReq);
    TEST_ASSERT(outOfRangeResp.indexOf("\"code\":-32602") != -1, "Out of range value must return JSON-RPC -32602");

    // 4. Invalid parameter type (state = 123 instead of boolean) -> -32602
    String badTypeReq = "{\"jsonrpc\":\"2.0\",\"id\":\"call-3\",\"method\":\"tools/call\",\"params\":{\"name\":\"set_led\",\"arguments\":{\"state\":123,\"brightness\":100}}}";
    String badTypeResp = server.processMessage(badTypeReq);
    TEST_ASSERT(badTypeResp.indexOf("\"code\":-32602") != -1, "Bad param type must return JSON-RPC -32602");

    // 5. Successful call with binding verification
    String goodReq = "{\"jsonrpc\":\"2.0\",\"id\":\"call-4\",\"method\":\"tools/call\",\"params\":{\"name\":\"set_led\",\"arguments\":{\"state\":true,\"brightness\":200,\"mode\":\"blink\"}}}";
    String goodResp = server.processMessage(goodReq);
    TEST_ASSERT(goodResp.indexOf("\"isError\":false") != -1, "Call must succeed");
    TEST_ASSERT(goodResp.indexOf("LED ON brightness 200") != -1, "Handler output formatted correctly");
    TEST_ASSERT(ledState == true, "Target variable ledState must be updated to true");
    TEST_ASSERT(brightness == 200, "Target variable brightness must be updated to 200");
    TEST_ASSERT(strcmp(mode, "blink") == 0, "Target variable mode must be updated to blink");
}

void test_step4_protocol_and_headers() {
    MCPServer server("Node-2026", "2.0.0");
    server.setCacheTtl(120000);

    // 1. Discover full check with ttlMs
    String discReq = "{\"jsonrpc\":\"2.0\",\"id\":\"disc-42\",\"method\":\"server/discover\",\"params\":{}}";
    String discResp = server.processMessage(discReq);
    TEST_ASSERT(discResp.indexOf("\"supportedVersions\":[\"2026-07-28\",\"2024-11-05\"]") != -1, "Must list all supported versions");
    TEST_ASSERT(discResp.indexOf("\"ttlMs\":120000") != -1, "Must include configured ttlMs in _meta");

    // 2. Legacy initialize negotiation (client requests 2024-11-05)
    String legInitReq = "{\"jsonrpc\":\"2.0\",\"id\":100,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2024-11-05\"}}";
    String legInitResp = server.processMessage(legInitReq);
    TEST_ASSERT(legInitResp.indexOf("\"protocolVersion\":\"2024-11-05\"") != -1, "Must negotiate 2024-11-05 when requested");

    // 3. Header validation
    const char* rawReq = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\",\"params\":{\"name\":\"set_led\",\"arguments\":{}}}";
    MCPRpcRequestView view;
    MCPScanError err = MCPScanError::None;
    bool parseOk = MCPJsonScanner::parseRpcRequest(rawReq, strlen(rawReq), view, &err);
    TEST_ASSERT(parseOk, "Parse must succeed");

    char errBuf[128];
    // Matching headers
    bool hOk1 = server.validateHeaders("2026-07-28", "tools/call", "set_led", view, errBuf, sizeof(errBuf));
    TEST_ASSERT(hOk1, "Matching headers must pass validation");

    // Unsupported version
    bool hOk2 = server.validateHeaders("1999-01-01", "tools/call", "set_led", view, errBuf, sizeof(errBuf));
    TEST_ASSERT(!hOk2, "Unsupported version header must fail");

    // Method mismatch
    bool hOk3 = server.validateHeaders("2026-07-28", "tools/list", "set_led", view, errBuf, sizeof(errBuf));
    TEST_ASSERT(!hOk3, "Mismatched method header must fail");

    // Name mismatch
    bool hOk4 = server.validateHeaders("2026-07-28", "tools/call", "wrong_name", view, errBuf, sizeof(errBuf));
    TEST_ASSERT(!hOk4, "Mismatched name header must fail");
}

void test_security_bearer_and_cors() {
    MCPServer server;
    // Without bearer configured: any request passes checkBearer
    TEST_ASSERT(server.checkBearer(nullptr), "Without bearer configured, checkBearer must be true");

    server.bearer("my-secret-token-123");
    TEST_ASSERT(!server.checkBearer(nullptr), "Missing header must fail when bearer is set");
    TEST_ASSERT(!server.checkBearer("Basic dXNlcjpwYXNz"), "Non-bearer auth must fail");
    TEST_ASSERT(!server.checkBearer("Bearer wrong-token"), "Wrong bearer token must fail");
    TEST_ASSERT(server.checkBearer("Bearer my-secret-token-123"), "Correct bearer token must pass");

    // CORS closed by default
    TEST_ASSERT(!server.checkOrigin("https://example.com"), "CORS must be closed by default");
    server.allowOrigin("https://trusted.ai");
    TEST_ASSERT(!server.checkOrigin("https://malicious.com"), "Untrusted origin must be rejected");
    TEST_ASSERT(server.checkOrigin("https://trusted.ai"), "Configured allowed origin must be accepted");
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

    RUN_TEST(test_tool_schema_validation_and_max_tools);
    RUN_TEST(test_id_preservation_string_and_number);
    RUN_TEST(test_arguments_get_raw);
    RUN_TEST(test_response_buffer_overflow_error);
    RUN_TEST(test_fluent_tool_builder_and_validation);
    RUN_TEST(test_step4_protocol_and_headers);
    RUN_TEST(test_security_bearer_and_cors);

    std::cout << "========================================" << std::endl;
    std::cout << " Test Summary: " << tests_passed << " passed, " << tests_failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;

    return tests_failed == 0 ? 0 : 1;
}
