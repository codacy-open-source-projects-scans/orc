---
short-description: Orc - The Oil Runtime Compiler
...

# Orc Reference Manual

Welcome to the Orc reference manual. Orc is a library and toolset for compiling and executing very simple programs that operate on arrays of data.

## What is Orc?

Orc is a compiler for a simple assembly-like language. Unlike most compilers, Orc is primarily a library, which means that all its features can be controlled from any application that uses it. Also unlike most compilers, Orc creates code that can be immediately executed by the application.

Orc is mainly useful for generating code that performs simple mathematical operations on contiguous arrays. It is primarily targeted toward generating code for vector CPU extensions such as SSE, AVX, Altivec, NEON, and RISC-V Vector extensions.

## Key Features

- **Runtime Code Generation**: Compile and execute code at runtime
- **Multiple Backends**: Support for SSE, AVX, MMX, NEON, Altivec, MIPS, RISC-V, and more
- **Simple Language**: Easy-to-learn assembly-like syntax
- **Optimized**: Generates highly optimized SIMD code
- **Portable**: Falls back to C implementation when SIMD is not available

## Getting Started

- [Building Orc](building.md) - How to build and install Orc
- [Running Applications](running.md) - Environment variables and runtime behavior
- [Orc Concepts](concepts.md) - Understanding Orc's architecture
- [Tutorial](tutorial.md) - Step-by-step guide to using Orc

## Project Information

- **Version**: 0.4.41.1
- **License**: BSD-2-Clause and BSD-3-Clause
- **Homepage**: https://gstreamer.freedesktop.org/modules/orc.html
- **Source**: https://gitlab.freedesktop.org/gstreamer/orc

## Support

For bug reports and feature requests, please use the [GitLab issue tracker](https://gitlab.freedesktop.org/gstreamer/orc/-/issues).
