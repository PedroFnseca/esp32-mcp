#include "MCPJsonScanner.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>

static inline bool isHexChar(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static inline uint16_t parseHex4(const char* p) {
    uint16_t v = 0;
    for (int i = 0; i < 4; ++i) {
        char c = p[i];
        v <<= 4;
        if (c >= '0' && c <= '9') v |= (c - '0');
        else if (c >= 'a' && c <= 'f') v |= (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') v |= (c - 'A' + 10);
    }
    return v;
}

static size_t encodeUtf8(uint32_t cp, char* out, size_t outCapacity) {
    if (cp <= 0x7F) {
        if (outCapacity < 1) return 0;
        out[0] = (char)cp;
        return 1;
    } else if (cp <= 0x7FF) {
        if (outCapacity < 2) return 0;
        out[0] = (char)(0xC0 | ((cp >> 6) & 0x1F));
        out[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    } else if (cp <= 0xFFFF) {
        if (outCapacity < 3) return 0;
        out[0] = (char)(0xE0 | ((cp >> 12) & 0x0F));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    } else if (cp <= 0x10FFFF) {
        if (outCapacity < 4) return 0;
        out[0] = (char)(0xF0 | ((cp >> 18) & 0x07));
        out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[3] = (char)(0x80 | (cp & 0x3F));
        return 4;
    }
    return 0;
}

bool MCPJsonSlice::asBool(bool defaultValue) const {
    if (!isValid() || type != MCPJsonType::Boolean || len == 0 || ptr == nullptr) {
        return defaultValue;
    }
    if (len == 4 && memcmp(ptr, "true", 4) == 0) return true;
    if (len == 5 && memcmp(ptr, "false", 5) == 0) return false;
    return defaultValue;
}

long MCPJsonSlice::asLong(long defaultValue, bool* ok) const {
    if (!isValid() || type != MCPJsonType::Number || len == 0 || ptr == nullptr) {
        if (ok) *ok = false;
        return defaultValue;
    }
    const char* p = ptr;
    const char* end = ptr + len;
    bool neg = false;
    if (*p == '-') {
        neg = true;
        p++;
    }
    long long val = 0;
    while (p < end && *p >= '0' && *p <= '9') {
        int d = *p - '0';
        if (neg) {
            if (-val < (LONG_MIN + d) / 10) {
                if (ok) *ok = false;
                return defaultValue;
            }
        } else {
            if (val > (LONG_MAX - d) / 10) {
                if (ok) *ok = false;
                return defaultValue;
            }
        }
        val = val * 10 + d;
        p++;
    }
    if (ok) *ok = true;
    return neg ? (long)(-val) : (long)val;
}

int MCPJsonSlice::asInt(int defaultValue, bool* ok) const {
    bool longOk = false;
    long v = asLong(defaultValue, &longOk);
    if (!longOk || v < INT_MIN || v > INT_MAX) {
        if (ok) *ok = false;
        return defaultValue;
    }
    if (ok) *ok = true;
    return (int)v;
}

float MCPJsonSlice::asFloat(float defaultValue, bool* ok) const {
    if (!isValid() || type != MCPJsonType::Number || len == 0 || ptr == nullptr) {
        if (ok) *ok = false;
        return defaultValue;
    }
    char buf[64];
    size_t copyLen = len < 63 ? len : 63;
    memcpy(buf, ptr, copyLen);
    buf[copyLen] = '\0';
    char* endp = nullptr;
    float v = strtof(buf, &endp);
    if (endp == buf) {
        if (ok) *ok = false;
        return defaultValue;
    }
    if (ok) *ok = true;
    return v;
}

double MCPJsonSlice::asDouble(double defaultValue, bool* ok) const {
    if (!isValid() || type != MCPJsonType::Number || len == 0 || ptr == nullptr) {
        if (ok) *ok = false;
        return defaultValue;
    }
    char buf[64];
    size_t copyLen = len < 63 ? len : 63;
    memcpy(buf, ptr, copyLen);
    buf[copyLen] = '\0';
    char* endp = nullptr;
    double v = strtod(buf, &endp);
    if (endp == buf) {
        if (ok) *ok = false;
        return defaultValue;
    }
    if (ok) *ok = true;
    return v;
}

bool MCPJsonSlice::decodeString(char* out, size_t outCapacity, size_t* outLength) const {
    if (!isValid() || type != MCPJsonType::String || ptr == nullptr || outCapacity == 0) {
        return false;
    }
    const char* cur = ptr;
    const char* end = ptr + len;
    if (cur < end && *cur == '"') cur++;
    if (end > cur && *(end - 1) == '"') end--;

    size_t written = 0;
    while (cur < end) {
        if (*cur == '\\') {
            cur++;
            if (cur >= end) return false;
            char esc = *cur++;
            char decodedChar = '\0';
            switch (esc) {
                case '"':  decodedChar = '"'; break;
                case '\\': decodedChar = '\\'; break;
                case '/':  decodedChar = '/'; break;
                case 'b':  decodedChar = '\b'; break;
                case 'f':  decodedChar = '\f'; break;
                case 'n':  decodedChar = '\n'; break;
                case 'r':  decodedChar = '\r'; break;
                case 't':  decodedChar = '\t'; break;
                case 'u': {
                    if (cur + 4 > end) return false;
                    for (int i = 0; i < 4; ++i) {
                        if (!isHexChar(cur[i])) return false;
                    }
                    uint16_t u1 = parseHex4(cur);
                    cur += 4;
                    uint32_t codepoint = u1;
                    if (u1 >= 0xD800 && u1 <= 0xDBFF) {
                        if (cur + 6 > end || cur[0] != '\\' || cur[1] != 'u') {
                            return false;
                        }
                        cur += 2;
                        for (int i = 0; i < 4; ++i) {
                            if (!isHexChar(cur[i])) return false;
                        }
                        uint16_t u2 = parseHex4(cur);
                        cur += 4;
                        if (u2 < 0xDC00 || u2 > 0xDFFF) return false;
                        codepoint = 0x10000 + (((uint32_t)(u1 - 0xD800) << 10) | (uint32_t)(u2 - 0xDC00));
                    } else if (u1 >= 0xDC00 && u1 <= 0xDFFF) {
                        return false;
                    }

                    if (written >= outCapacity) return false;
                    size_t uLen = encodeUtf8(codepoint, out + written, outCapacity - written);
                    if (uLen == 0) return false;
                    written += uLen;
                    continue;
                }
                default:
                    return false;
            }
            if (written + 1 >= outCapacity) return false;
            out[written++] = decodedChar;
        } else {
            if (written + 1 >= outCapacity) return false;
            out[written++] = *cur++;
        }
    }

    if (written < outCapacity) {
        out[written] = '\0';
    } else {
        return false;
    }
    if (outLength) *outLength = written;
    return true;
}

bool MCPJsonSlice::equalsString(const char* str) const {
    if (!str) return false;
    char decoded[128];
    if (!decodeString(decoded, sizeof(decoded))) {
        return false;
    }
    return strcmp(decoded, str) == 0;
}

const char* MCPJsonScanner::skipWhitespace(const char* cur, const char* end) {
    while (cur < end && (*cur == ' ' || *cur == '\t' || *cur == '\r' || *cur == '\n')) {
        cur++;
    }
    return cur;
}

const char* MCPJsonScanner::scanString(const char* cur, const char* end, MCPScanError* errOut) {
    if (cur >= end || *cur != '"') {
        if (errOut) *errOut = MCPScanError::UnexpectedChar;
        return nullptr;
    }
    cur++;
    while (cur < end) {
        unsigned char c = (unsigned char)*cur;
        if (c < 0x20) {
            if (errOut) *errOut = MCPScanError::UnexpectedChar;
            return nullptr;
        }
        if (c == '"') {
            return cur + 1;
        }
        if (c == '\\') {
            cur++;
            if (cur >= end) {
                if (errOut) *errOut = MCPScanError::Truncated;
                return nullptr;
            }
            char esc = *cur;
            if (esc == '"' || esc == '\\' || esc == '/' || esc == 'b' || esc == 'f' || esc == 'n' || esc == 'r' || esc == 't') {
                cur++;
            } else if (esc == 'u') {
                cur++;
                if (cur + 4 > end) {
                    if (errOut) *errOut = MCPScanError::Truncated;
                    return nullptr;
                }
                for (int i = 0; i < 4; ++i) {
                    if (!isHexChar(cur[i])) {
                        if (errOut) *errOut = MCPScanError::InvalidEscape;
                        return nullptr;
                    }
                }
                uint16_t u1 = parseHex4(cur);
                cur += 4;
                if (u1 >= 0xD800 && u1 <= 0xDBFF) {
                    if (cur + 2 > end) {
                        if (errOut) *errOut = MCPScanError::Truncated;
                        return nullptr;
                    }
                    if (cur[0] != '\\' || cur[1] != 'u') {
                        if (errOut) *errOut = MCPScanError::InvalidUnicode;
                        return nullptr;
                    }
                    cur += 2;
                    if (cur + 4 > end) {
                        if (errOut) *errOut = MCPScanError::Truncated;
                        return nullptr;
                    }
                    for (int i = 0; i < 4; ++i) {
                        if (!isHexChar(cur[i])) {
                            if (errOut) *errOut = MCPScanError::InvalidEscape;
                            return nullptr;
                        }
                    }
                    uint16_t u2 = parseHex4(cur);
                    cur += 4;
                    if (u2 < 0xDC00 || u2 > 0xDFFF) {
                        if (errOut) *errOut = MCPScanError::InvalidUnicode;
                        return nullptr;
                    }
                } else if (u1 >= 0xDC00 && u1 <= 0xDFFF) {
                    if (errOut) *errOut = MCPScanError::InvalidUnicode;
                    return nullptr;
                }
            } else {
                if (errOut) *errOut = MCPScanError::InvalidEscape;
                return nullptr;
            }
        } else {
            cur++;
        }
    }
    if (errOut) *errOut = MCPScanError::Truncated;
    return nullptr;
}

const char* MCPJsonScanner::scanNumber(const char* cur, const char* end, MCPScanError* errOut) {
    const char* start = cur;
    if (cur < end && *cur == '-') {
        cur++;
    }
    if (cur >= end) {
        if (errOut) *errOut = MCPScanError::Truncated;
        return nullptr;
    }
    if (*cur == '0') {
        cur++;
        if (cur < end && *cur >= '0' && *cur <= '9') {
            if (errOut) *errOut = MCPScanError::UnexpectedChar;
            return nullptr;
        }
    } else if (*cur >= '1' && *cur <= '9') {
        while (cur < end && *cur >= '0' && *cur <= '9') {
            cur++;
        }
    } else {
        if (errOut) *errOut = MCPScanError::UnexpectedChar;
        return nullptr;
    }

    bool hasFraction = false;
    if (cur < end && *cur == '.') {
        hasFraction = true;
        cur++;
        if (cur >= end || *cur < '0' || *cur > '9') {
            if (errOut) *errOut = MCPScanError::UnexpectedChar;
            return nullptr;
        }
        while (cur < end && *cur >= '0' && *cur <= '9') {
            cur++;
        }
    }

    bool hasExp = false;
    if (cur < end && (*cur == 'e' || *cur == 'E')) {
        hasExp = true;
        cur++;
        if (cur < end && (*cur == '+' || *cur == '-')) {
            cur++;
        }
        if (cur >= end || *cur < '0' || *cur > '9') {
            if (errOut) *errOut = MCPScanError::UnexpectedChar;
            return nullptr;
        }
        while (cur < end && *cur >= '0' && *cur <= '9') {
            cur++;
        }
    }

    if (!hasFraction && !hasExp) {
        MCPJsonSlice slice(start, (size_t)(cur - start), MCPJsonType::Number);
        bool ok = false;
        slice.asLong(0, &ok);
        if (!ok) {
            if (errOut) *errOut = MCPScanError::IntegerOverflow;
            return nullptr;
        }
    }

    return cur;
}

const char* MCPJsonScanner::scanObject(const char* cur, const char* end, MCPScanError* errOut, int depth, int maxDepth) {
    if (depth > maxDepth) {
        if (errOut) *errOut = MCPScanError::DepthExceeded;
        return nullptr;
    }
    cur++;
    cur = skipWhitespace(cur, end);
    if (cur < end && *cur == '}') {
        return cur + 1;
    }

    struct KeyEntry {
        char keyStr[64];
    };
    KeyEntry seenKeys[MCP_MAX_PARAMS];
    size_t seenCount = 0;

    while (cur < end) {
        cur = skipWhitespace(cur, end);
        if (cur >= end) {
            if (errOut) *errOut = MCPScanError::Truncated;
            return nullptr;
        }
        if (*cur != '"') {
            if (errOut) *errOut = MCPScanError::UnexpectedChar;
            return nullptr;
        }
        const char* keyStart = cur;
        const char* keyEnd = scanString(cur, end, errOut);
        if (!keyEnd) return nullptr;

        MCPJsonSlice keySlice(keyStart, (size_t)(keyEnd - keyStart), MCPJsonType::String);
        char currentKeyDecoded[64];
        if (keySlice.decodeString(currentKeyDecoded, sizeof(currentKeyDecoded))) {
            for (size_t i = 0; i < seenCount; ++i) {
                if (strcmp(seenKeys[i].keyStr, currentKeyDecoded) == 0) {
                    if (errOut) *errOut = MCPScanError::DuplicateKey;
                    return nullptr;
                }
            }
            if (seenCount < MCP_MAX_PARAMS) {
                strncpy(seenKeys[seenCount].keyStr, currentKeyDecoded, sizeof(seenKeys[seenCount].keyStr) - 1);
                seenKeys[seenCount].keyStr[sizeof(seenKeys[seenCount].keyStr) - 1] = '\0';
                seenCount++;
            }
        }

        cur = skipWhitespace(keyEnd, end);
        if (cur >= end) {
            if (errOut) *errOut = MCPScanError::Truncated;
            return nullptr;
        }
        if (*cur != ':') {
            if (errOut) *errOut = MCPScanError::UnexpectedChar;
            return nullptr;
        }
        cur++;
        cur = skipWhitespace(cur, end);
        if (cur >= end) {
            if (errOut) *errOut = MCPScanError::Truncated;
            return nullptr;
        }

        cur = scanValue(cur, end, nullptr, errOut, depth + 1, maxDepth);
        if (!cur) return nullptr;

        cur = skipWhitespace(cur, end);
        if (cur >= end) {
            if (errOut) *errOut = MCPScanError::Truncated;
            return nullptr;
        }
        if (*cur == ',') {
            cur++;
            cur = skipWhitespace(cur, end);
            if (cur < end && *cur == '}') {
                if (errOut) *errOut = MCPScanError::UnexpectedChar;
                return nullptr;
            }
        } else if (*cur == '}') {
            return cur + 1;
        } else {
            if (errOut) *errOut = MCPScanError::UnexpectedChar;
            return nullptr;
        }
    }
    if (errOut) *errOut = MCPScanError::Truncated;
    return nullptr;
}

const char* MCPJsonScanner::scanArray(const char* cur, const char* end, MCPScanError* errOut, int depth, int maxDepth) {
    if (depth > maxDepth) {
        if (errOut) *errOut = MCPScanError::DepthExceeded;
        return nullptr;
    }
    cur++;
    cur = skipWhitespace(cur, end);
    if (cur < end && *cur == ']') {
        return cur + 1;
    }

    while (cur < end) {
        cur = skipWhitespace(cur, end);
        if (cur >= end) {
            if (errOut) *errOut = MCPScanError::Truncated;
            return nullptr;
        }

        cur = scanValue(cur, end, nullptr, errOut, depth + 1, maxDepth);
        if (!cur) return nullptr;

        cur = skipWhitespace(cur, end);
        if (cur >= end) {
            if (errOut) *errOut = MCPScanError::Truncated;
            return nullptr;
        }
        if (*cur == ',') {
            cur++;
            cur = skipWhitespace(cur, end);
            if (cur < end && *cur == ']') {
                if (errOut) *errOut = MCPScanError::UnexpectedChar;
                return nullptr;
            }
        } else if (*cur == ']') {
            return cur + 1;
        } else {
            if (errOut) *errOut = MCPScanError::UnexpectedChar;
            return nullptr;
        }
    }
    if (errOut) *errOut = MCPScanError::Truncated;
    return nullptr;
}

const char* MCPJsonScanner::scanValue(const char* cur, const char* end, MCPJsonSlice* sliceOut, MCPScanError* errOut, int depth, int maxDepth) {
    cur = skipWhitespace(cur, end);
    if (cur >= end) {
        if (errOut) *errOut = MCPScanError::Truncated;
        return nullptr;
    }

    const char* start = cur;
    MCPJsonType type = MCPJsonType::Invalid;
    const char* next = nullptr;

    char c = *cur;
    if (c == '"') {
        type = MCPJsonType::String;
        next = scanString(cur, end, errOut);
    } else if (c == '{') {
        type = MCPJsonType::Object;
        next = scanObject(cur, end, errOut, depth, maxDepth);
    } else if (c == '[') {
        type = MCPJsonType::Array;
        next = scanArray(cur, end, errOut, depth, maxDepth);
    } else if (c == 't') {
        if (end - cur >= 4 && memcmp(cur, "true", 4) == 0) {
            type = MCPJsonType::Boolean;
            next = cur + 4;
        } else {
            if (errOut) *errOut = MCPScanError::UnexpectedChar;
            return nullptr;
        }
    } else if (c == 'f') {
        if (end - cur >= 5 && memcmp(cur, "false", 5) == 0) {
            type = MCPJsonType::Boolean;
            next = cur + 5;
        } else {
            if (errOut) *errOut = MCPScanError::UnexpectedChar;
            return nullptr;
        }
    } else if (c == 'n') {
        if (end - cur >= 4 && memcmp(cur, "null", 4) == 0) {
            type = MCPJsonType::Null;
            next = cur + 4;
        } else {
            if (errOut) *errOut = MCPScanError::UnexpectedChar;
            return nullptr;
        }
    } else if (c == '-' || (c >= '0' && c <= '9')) {
        type = MCPJsonType::Number;
        next = scanNumber(cur, end, errOut);
    } else {
        if (errOut) *errOut = MCPScanError::UnexpectedChar;
        return nullptr;
    }

    if (next && sliceOut) {
        sliceOut->ptr = start;
        sliceOut->len = (size_t)(next - start);
        sliceOut->type = type;
    }
    return next;
}

bool MCPJsonScanner::validate(const char* json, size_t length, MCPScanError* errOut, int maxDepth) {
    if (!json || length == 0) {
        if (errOut) *errOut = MCPScanError::Truncated;
        return false;
    }
    if (length > MCP_MAX_REQUEST_SIZE) {
        if (errOut) *errOut = MCPScanError::BufferOverflow;
        return false;
    }
    const char* cur = json;
    const char* end = json + length;
    cur = skipWhitespace(cur, end);
    if (cur >= end) {
        if (errOut) *errOut = MCPScanError::Truncated;
        return false;
    }

    MCPScanError err = MCPScanError::None;
    cur = scanValue(cur, end, nullptr, &err, 1, maxDepth);
    if (!cur) {
        if (errOut) *errOut = err;
        return false;
    }

    cur = skipWhitespace(cur, end);
    if (cur < end) {
        if (errOut) *errOut = MCPScanError::TrailingTrash;
        return false;
    }
    if (errOut) *errOut = MCPScanError::None;
    return true;
}

MCPJsonSlice MCPJsonScanner::findKey(const char* objJson, size_t objLength, const char* key, MCPScanError* errOut, int maxDepth) {
    if (!objJson || objLength == 0 || !key) {
        if (errOut) *errOut = MCPScanError::Truncated;
        return MCPJsonSlice();
    }
    const char* cur = objJson;
    const char* end = objJson + objLength;
    cur = skipWhitespace(cur, end);
    if (cur >= end || *cur != '{') {
        if (errOut) *errOut = MCPScanError::UnexpectedChar;
        return MCPJsonSlice();
    }
    cur++;

    while (cur < end) {
        cur = skipWhitespace(cur, end);
        if (cur >= end) {
            if (errOut) *errOut = MCPScanError::Truncated;
            return MCPJsonSlice();
        }
        if (*cur == '}') {
            return MCPJsonSlice();
        }
        if (*cur != '"') {
            if (errOut) *errOut = MCPScanError::UnexpectedChar;
            return MCPJsonSlice();
        }
        const char* keyStart = cur;
        const char* keyEnd = scanString(cur, end, errOut);
        if (!keyEnd) return MCPJsonSlice();

        MCPJsonSlice keySlice(keyStart, (size_t)(keyEnd - keyStart), MCPJsonType::String);
        bool match = keySlice.equalsString(key);

        cur = skipWhitespace(keyEnd, end);
        if (cur >= end || *cur != ':') {
            if (errOut) *errOut = MCPScanError::UnexpectedChar;
            return MCPJsonSlice();
        }
        cur++;
        cur = skipWhitespace(cur, end);
        if (cur >= end) {
            if (errOut) *errOut = MCPScanError::Truncated;
            return MCPJsonSlice();
        }

        MCPJsonSlice valSlice;
        cur = scanValue(cur, end, &valSlice, errOut, 1, maxDepth);
        if (!cur) return MCPJsonSlice();

        if (match) {
            if (errOut) *errOut = MCPScanError::None;
            return valSlice;
        }

        cur = skipWhitespace(cur, end);
        if (cur < end && *cur == ',') {
            cur++;
        } else if (cur < end && *cur == '}') {
            return MCPJsonSlice();
        } else {
            if (errOut) *errOut = MCPScanError::UnexpectedChar;
            return MCPJsonSlice();
        }
    }
    return MCPJsonSlice();
}

bool MCPJsonScanner::parseRpcRequest(const char* json, size_t length, MCPRpcRequestView& outView, MCPScanError* errOut) {
    if (!validate(json, length, errOut)) {
        return false;
    }

    MCPJsonSlice rootSlice;
    const char* end = json + length;
    const char* cur = skipWhitespace(json, end);
    scanValue(cur, end, &rootSlice, errOut, 1, MCP_MAX_JSON_DEPTH);
    if (!rootSlice.isObject()) {
        if (errOut) *errOut = MCPScanError::UnexpectedChar;
        return false;
    }

    outView.jsonrpc = findKey(rootSlice.ptr, rootSlice.len, "jsonrpc", errOut);
    outView.id = findKey(rootSlice.ptr, rootSlice.len, "id", errOut);
    outView.method = findKey(rootSlice.ptr, rootSlice.len, "method", errOut);
    outView.params = findKey(rootSlice.ptr, rootSlice.len, "params", errOut);

    if (outView.params.isObject()) {
        outView.paramsMeta = findKey(outView.params.ptr, outView.params.len, "_meta", errOut);
        outView.paramsName = findKey(outView.params.ptr, outView.params.len, "name", errOut);
        outView.paramsArguments = findKey(outView.params.ptr, outView.params.len, "arguments", errOut);
    } else {
        outView.paramsMeta = MCPJsonSlice();
        outView.paramsName = MCPJsonSlice();
        outView.paramsArguments = MCPJsonSlice();
    }

    outView.isNotification = !outView.id.isValid() || outView.id.isNull();
    return true;
}
