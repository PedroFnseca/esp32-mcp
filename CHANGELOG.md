# Changelog

## [1.1.1](https://github.com/PedroFnseca/esp32-mcp/compare/esp32-mcp-v1.1.0...esp32-mcp-v1.1.1) (2026-11-12)


### Documentation

* refactor readme apresentation ([deccf8e](https://github.com/PedroFnseca/esp32-mcp/commit/deccf8e36dc8ec2a837e7944e94fcc5a108677a5))

## [1.1.0](https://github.com/PedroFnseca/esp32-mcp/compare/esp32-mcp-v1.0.0...esp32-mcp-v1.1.0) (2026-11-12)


### Features

* **api:** add fluent and declarative tool builder with auto schema generation ([e8c0f98](https://github.com/PedroFnseca/esp32-mcp/commit/e8c0f988865c22655dd9bb10a17aaef90ab7609e))
* **core:** add FreeRTOS background task, payload limits, bearer auth and strict CORS ([d79e215](https://github.com/PedroFnseca/esp32-mcp/commit/d79e21539428d998b7de519299b928ed238493dc))
* **core:** implement ESP32-MCP server with MCP 2026-07-28 and HTTP transport ([e02c665](https://github.com/PedroFnseca/esp32-mcp/commit/e02c665d9dce4bfbd077f71f5673e8604e36fe15))
* **json:** add minimal DOM-less JSON scanner and fixed-buffer writer ([a421cbd](https://github.com/PedroFnseca/esp32-mcp/commit/a421cbdf18a46c0b37b305bee560bbd4bdf20a16))
* **protocol:** implement MCP 2026-07-28 discovery, cache hints, header validation and legacy mode ([17d7fe7](https://github.com/PedroFnseca/esp32-mcp/commit/17d7fe7f910930e178023c60ffe7cd68abcad305))
* **tools:** add built-in status/gpio tools, mDNS and rewrite examples with bearer demo ([e0e8785](https://github.com/PedroFnseca/esp32-mcp/commit/e0e87851739b6fa365523f67cf936f0251819ea6))


### Bug Fixes

* **ci:** update arduino-lint flag to update, add matching header and requirements.txt for pip cache ([7c0fa94](https://github.com/PedroFnseca/esp32-mcp/commit/7c0fa94dbf01c2ec96567e4fc9e2abb5bd65a512))
* **linter:** initialize allowedPins array and resolve variable shadow in handleToolsCall ([67818fa](https://github.com/PedroFnseca/esp32-mcp/commit/67818fac3a1b56dcf4de5f816dec1d372223a21c))


### Performance

* **memory:** eliminate String from hot path and use fixed buffers ([0606187](https://github.com/PedroFnseca/esp32-mcp/commit/0606187286a548094ad1f87a2361e10c3765a5ba))


### Documentation

* add project documentation with API reference and quick start ([c76bbb4](https://github.com/PedroFnseca/esp32-mcp/commit/c76bbb4acef4cc643ccb0da48ddc06978d9215b8))
* **examples:** add basic, led control, sensor, and telemetry sketches ([e46d693](https://github.com/PedroFnseca/esp32-mcp/commit/e46d6931e8287253b9b38ad00039d0bd2f202085))
* **readme:** rewrite comprehensive single documentation and remove contributing guide ([936d25d](https://github.com/PedroFnseca/esp32-mcp/commit/936d25d86ded348af3b19ffeeaca9dfd5ea6bfcb))


### Refactoring

* **mcp:** switch server, arguments and jsonrpc to internal scanner and writer ([851ecc5](https://github.com/PedroFnseca/esp32-mcp/commit/851ecc5ebbbcfb901fa11b106462d977f1cd5ae9))


### Tests

* add unit test suite and native test mock ([2085fe6](https://github.com/PedroFnseca/esp32-mcp/commit/2085fe62c8013623f4de7d2f6de0d3866d8dff76))
* **unity:** migrate host tests to Unity runner and update CI workflow ([5c4f634](https://github.com/PedroFnseca/esp32-mcp/commit/5c4f634b11e074a09a5f6c3edff5e97a986f59bc))

## [0.1.0](https://github.com/PedroFnseca/esp32-mcp/compare/esp32-mcp-v0.0.1...esp32-mcp-v0.1.0) (2026-08-20)


### Features

* **core:** implement ESP32-MCP server with MCP 2026-07-28 and HTTP transport ([e02c665](https://github.com/PedroFnseca/esp32-mcp/commit/e02c665d9dce4bfbd077f71f5673e8604e36fe15))
