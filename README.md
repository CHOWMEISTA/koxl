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

The compiler emits object files into the `dist/bin/1.0` folder under the project containing the `.kxproj` (or the source folder if no project is found). The `.kxproj` may include `<OutputPath>` and `<OutputName>` elements to override the output directory and output file name.

Notes:
- Semicolons are mandatory; the compiler will error if a non-empty, non-comment line does not end with `;`.
- The current translator supports `print("...\")` statements and emits a C file that is assembled with `clang -c` into an object. `clang` must be on PATH.
- This is a minimal scaffold — expand the lexer/parser and codegen as needed.
