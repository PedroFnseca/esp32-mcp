#ifndef ESP32_MCP_JSON_CONFIG_H
#define ESP32_MCP_JSON_CONFIG_H

#include <stddef.h>
#include <stdint.h>

#ifndef MCP_MAX_REQUEST_SIZE
#define MCP_MAX_REQUEST_SIZE 4096
#endif

#ifndef MCP_MAX_RESPONSE_SIZE
#define MCP_MAX_RESPONSE_SIZE 4096
#endif

#ifndef MCP_MAX_TOOLS
#define MCP_MAX_TOOLS 16
#endif

#ifndef MCP_MAX_PARAMS
#define MCP_MAX_PARAMS 16
#endif

#ifndef MCP_MAX_JSON_DEPTH
#define MCP_MAX_JSON_DEPTH 16
#endif

#endif
