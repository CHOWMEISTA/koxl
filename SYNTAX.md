KOXL language syntax (minimal)

- Style: C-like, statements terminated by semicolons `;` (mandatory).
- Comments: `//` single-line comments.
- Blocks: `{` and `}` open/close blocks (ignored by the minimal compiler).
- Builtins:
  - `print(<string_literal>);` — prints the string followed by a newline.

Examples:

// print example
print("Hello, KOXL");

Notes:
- The current compiler is minimal and only supports `print` statements; other statements are ignored but must end with `;`.
- Strings use single or double quotes.
