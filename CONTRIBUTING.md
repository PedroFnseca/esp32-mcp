# Contributing to ESP32-MCP

Thank you for your interest in contributing to **ESP32-MCP**!

To ensure seamless collaboration, automated semantic versioning, and changelog generation via **Release Please**, all contributions must follow the guidelines outlined below.

---

## 1. Commit Message Convention (Conventional Commits)

This repository uses **[Release Please](https://github.com/googleapis/release-please)** to automate version bumping and changelog entries.

All commit messages **must** follow the [Conventional Commits v1.0.0](https://www.conventionalcommits.org/) specification:

```text
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

### Commit Types & Release Triggers

| Type | Description | Release Impact |
| :--- | :--- | :--- |
| `feat` | A new feature or capability | **Minor** release (`0.X.0` or `X.Y.0`) |
| `fix` | A bug fix | **Patch** release (`0.0.X` or `X.Y.Z`) |
| `perf` | A code change that improves performance | **Patch** release |
| `docs` | Documentation changes only | No release (or patch if configured) |
| `refactor` | Code refactoring without feature or bug changes | No release |
| `test` | Adding or updating tests | No release |
| `chore` | Maintenance, dependencies, or tooling | No release |
| `ci` | Changes to CI/CD workflows and configuration | No release |
| `style` | Formatting or whitespace changes | No release |

### Breaking Changes
To trigger a **Major** release (e.g. `1.0.0` -> `2.0.0` or `0.1.0` -> `0.2.0` in pre-1.0), include an exclamation mark `!` after the type/scope or add `BREAKING CHANGE:` in the footer:

```text
feat!: change tool callback signature to return MCPResult
```
or
```text
feat: update tool argument parser

BREAKING CHANGE: MCPArguments now requires getInt() instead of getInteger().
```

### Examples of Good Commit Messages
- `feat(tool): add support for tool schema validation`
- `fix(jsonrpc): handle empty parameters object gracefully`
- `docs(readme): add curl examples for server/discover`
- `test(mcp): add unit tests for ping notification`
- `ci: add esp32 compile verification matrix`

---

## 2. Development & Code Guidelines

### Clean Code Constraints
1. **No Code Comments in `src/`**: Source code in `src/` must remain clean, expressive, and self-documenting without inline or block explanatory comments.
2. **English Documentation**: All documentation, error messages, test names, and commit messages must be written in English.
3. **RAM & Embedded Constraints**: Avoid unnecessary heap allocations, large static JSON buffers, and duplicate string copies. Prefer references (`const MCPArguments&`) and ArduinoJson v7 idioms.

---

## 3. Running Unit Tests Locally

Before submitting a Pull Request, verify that all unit tests pass on your machine:

```bash
# Compile and run unit tests on Linux / macOS
g++ -std=c++14 -Isrc -Itest/mock -Itest/include \
  src/mcp/MCPToolRegistry.cpp \
  src/mcp/MCPJsonRpc.cpp \
  src/mcp/MCPServer.cpp \
  src/mcp/transports/MCPHttpTransport.cpp \
  test/test_main.cpp \
  -o test_runner
./test_runner
```

```powershell
# Compile and run unit tests on Windows
g++ -std=c++14 -Isrc -Itest/mock -Itest/include `
  src/mcp/MCPToolRegistry.cpp `
  src/mcp/MCPJsonRpc.cpp `
  src/mcp/MCPServer.cpp `
  src/mcp/transports/MCPHttpTransport.cpp `
  test/test_main.cpp `
  -o test_runner.exe
.\test_runner.exe
```

---

## 4. Pull Request Process

1. **Fork and Branch**: Fork the repository and create a descriptive branch:
   ```bash
   git checkout -b feat/my-new-feature
   ```
2. **Make Changes**: Follow the clean code guidelines and add unit tests in `test/test_main.cpp` for any new logic.
3. **Validate**: Run the local test suite and ensure all tests pass.
4. **Commit with Conventional Commits**: Ensure your commit messages follow the `feat:`, `fix:`, etc. syntax.
5. **Open a PR**: Submit a Pull Request targeting the `main` branch. CI checks will run automated multi-OS unit tests and ESP32 example builds.
