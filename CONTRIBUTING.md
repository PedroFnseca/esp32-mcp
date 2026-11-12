# Contributing to ESP32-MCP

Thank you for your interest in contributing to **ESP32-MCP**! We welcome bug reports, feature requests, documentation improvements, and code contributions from the community.

---

## Code of Conduct

Please be respectful and constructive in all discussions, issues, and pull requests. We are committed to providing a welcoming and inclusive environment for everyone.

---

## Development Workflow

### Prerequisites

- [PlatformIO Core (CLI)](https://docs.platformio.org/en/latest/core/index.html) or [PlatformIO for VS Code](https://platformio.org/install/ide?install=vscode)
- Python 3.10+ (for helper scripts and test orchestration)
- (Optional) [Arduino Lint](https://github.com/arduino/arduino-lint) for metadata compliance checking

### Running Tests Locally

ESP32-MCP includes a comprehensive native test suite with Unity that runs directly on your development machine (Linux, macOS, or Windows) without needing physical ESP32 hardware connected.

To execute the unit tests:

```bash
# Run unit test suite on native host platform
pio test -e native
```

To run Arduino Lint locally:

```bash
arduino-lint --compliance strict
```

---

## Commit Guidelines (Conventional Commits)

This repository enforces **[Conventional Commits v1.0.0](https://www.conventionalcommits.org/)**. Our release pipeline ([Release Please](https://github.com/googleapis/release-please)) relies entirely on commit messages to calculate semantic versions (`MAJOR.MINOR.PATCH`) and generate entries in `CHANGELOG.md`.

### Format

```text
<type>(<optional scope>): <description>

[optional body]

[optional footer(s)]
```

### Supported Types

| Type | Release Bump | Purpose | Example |
| :--- | :--- | :--- | :--- |
| `feat` | **Minor** (`0.x.0`) | A new feature or user-facing functionality | `feat(tool): add support for double-precision float parameters` |
| `fix` | **Patch** (`0.0.x`) | A bug fix | `fix(json): handle escaped unicode characters in scanner` |
| `perf` | **Patch** (`0.0.x`) | A code change that improves performance | `perf(writer): optimize integer serialization buffer writes` |
| `docs` | None | Documentation updates only | `docs(readme): clarify reverse proxy configuration` |
| `refactor` | None | Code change that neither fixes a bug nor adds a feature | `refactor(server): clean up method dispatch table` |
| `test` | None | Adding or correcting tests | `test(native): add edge-case tests for max request size` |
| `chore` | None | Build tasks, tool configurations, dependencies | `chore(ci): update github action versions` |

### Breaking Changes

Any commit that introduces a breaking API change must include `BREAKING CHANGE:` in the footer or append an exclamation mark (`!`) after the type/scope:

```text
feat(core)!: restructure transport interface signature

BREAKING CHANGE: MCPTransport now requires a buffer capacity argument.
```

---

## Automated Releases & Version Bumping

Releases are fully automated via GitHub Actions:

1. When conventional commits are pushed to `main`, **Release Please** creates or updates an open release pull request.
2. The release PR contains the updated `CHANGELOG.md` and bumps versions across metadata files (`library.json`, `library.properties`, `.release-please-manifest.json`, `src/mcp/MCPServer.h`).
3. Merging the release PR automatically tags the repository and publishes a GitHub Release.

### Manual Version Bump Utility

For testing or manual maintenance, the project includes `bump.py`:

```bash
python bump.py <new_version>
# Example:
python bump.py 1.1.0
```

---

## Architectural & Coding Guidelines

ESP32 microcontrollers operate in constrained memory environments. When writing code for ESP32-MCP, follow these rules:

1. **Zero Dynamic Allocations in Hot Paths**:
   - Never use `malloc`, `free`, `new`, `delete`, `std::string`, or Arduino `String` during request handling, JSON parsing, or response formatting.
   - Use stack-allocated fixed buffers or the internal zero-copy `MCPJsonScanner` and `MCPJsonWriter`.
2. **Deterministic Memory Bounds**:
   - Always enforce limits using configured macros (`MCP_MAX_REQUEST_SIZE`, `MCP_MAX_RESPONSE_SIZE`, `MCP_MAX_PARAMS`, `MCP_MAX_TOOLS`).
   - Gracefully reject payloads exceeding limits with standard HTTP or JSON-RPC errors rather than overflowing.
3. **Constant-Time Security**:
   - Secrets, tokens, and authorization credentials must always be compared in constant time (XOR-based) to guard against timing side-channel attacks.
4. **FreeRTOS Awareness**:
   - The server may execute inside a dedicated FreeRTOS background task on Core 1. Keep shared mutable state thread-safe or document concurrency expectations for tool callbacks.

---

## Pull Request Process

1. Fork the repository and create a descriptive branch:
   ```bash
   git checkout -b feat/add-my-awesome-feature
   ```
2. Write clean, commented code adhering to the architecture rules.
3. Add unit tests in `test/` for any new logic or bug fixes.
4. Ensure all unit tests pass locally (`pio test -e native`).
5. Commit using Conventional Commits.
6. Submit a Pull Request targeting `main`. Our CI workflow will verify test suites across Linux, Windows, and macOS, as well as Arduino Lint compliance.
