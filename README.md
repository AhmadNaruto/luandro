# Luandro — Native Runtime Platform (NRP)

A modular, high-performance **Native Runtime Platform** for Android ARM64.

## Architecture

- **Kotlin** — thin public API only
- **C++20** — all business logic lives here
- **Luau** — second frontend sharing the same native implementation
- **JNI** — minimal bridge (type conversion only)

## Modules

| Package | Description |
|---------|-------------|
| `io.github.luandro.lexsoup` | JSoup-compatible HTML parser backed by Lexbor |
| `io.github.luandro.regex` | JavaScript-compatible regex via jsregexp |
| `io.github.luandro.js` | QuickJS JavaScript engine |
| `io.github.luandro.luau` | Luau scripting VM |

## Dependencies (Git Submodules)

- [Luau](https://github.com/luau-lang/luau)
- [Lexbor](https://github.com/lexbor/lexbor)
- [QuickJS-NG](https://github.com/quickjs-ng/quickjs)
- [jsregexp](https://github.com/kmarius/jsregexp)

## Development

See [PLANNING.md](PLANNING.md) for the full phase plan and progress tracker.

## Requirements

- Android NDK 28.2
- CMake 3.22+
- C++20 compiler
- Kotlin 2.x
