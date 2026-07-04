# KOXL — minimal language scaffold

This workspace contains a minimal scaffold for the KOXL language (C-like, semicolons mandatory).

Build the compiler with CMake (generates Visual Studio/msbuild solution):

```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

This produces the `koxl` executable.

Basic usage:

```powershell
# Compile a .kx; output path and output name are taken from the nearest .kxproj if present
koxl sample\Hello.kx
# or scaffold a minimal project
koxl --scaffold myproject
```

The compiler emits MSVC COFF object files (`.obj`) into the `dist/bin/1.0` folder under the project containing the nearest `.kxproj` (or the source folder if no project is found). The `.kxproj` may include `<OutputPath>` and `<OutputName>` elements to override the output directory and output file name.

Notes:
- Semicolons are mandatory; the compiler will error if a non-empty, non-comment line does not end with `;`.
- The compiler prefers the Microsoft `cl` compiler (MSVC) to produce x86-64 COFF `.obj` files. Ensure you run it from a Visual Studio Developer Command Prompt (or otherwise run `vcvarsall.bat`) so `cl` is on `PATH` and configured for x64. If `cl` is not available, the compiler will try `clang` as a fallback.
- The current translator supports `print("...")` statements and emits a temporary C file which is compiled to an object. This is a one-pass minimal compiler; expand the lexer/parser and codegen for more language features.
