# CODING_STANDARDS.md

Minimal C++/Arduino guidelines for this repo:
- Naming: classes/structs `PascalCase`, functions/methods `lowerCamelCase`, variables/members `lowerCamelCase` (no trailing underscores); constants `SCREAMING_SNAKE_CASE` using `constexpr` when possible.
- Braces on the same line as the header (`if (...) {`), always use braces even for single statements.
- Prefer `const`/`constexpr` and references over pointers when `nullptr` is not expected.
- Avoid dynamic allocation; use static/stack buffers sized for the MCU constraints.
- Includes: project/local headers in `""`, platform/stdlib in `<...>`; include only what you need.
- Formatting: 2-space indent, keep lines short (< 100 chars), use spaces after commas and around operators.
