# gumcpp — Shell Scripting TUI Toolkit (C++ port of Gum)

A zero-dependency C++ port of [gum](https://github.com/charmbracelet/gum) — a collection of terminal UI components for shell scripts: choose, confirm, filter, format, and more.

## Why gumcpp?

The original [gum](https://github.com/charmbracelet/gum) requires the Go toolchain plus dozens of modules. gumcpp compiles with a single `make` using only C++17 and standard Linux headers.

## Quick Start

```bash
make
./gumcpp choose "option a" "option b" "option c"
./gumcpp confirm "Are you sure?"
./gumcpp filter < file.txt
```

## Features

- **choose** — Fuzzy-select from a list of options
- **confirm** — Yes/no prompt
- **filter** — Interactive text filter
- **format** — Style and color text output
- **input** — Single-line text input
- **write** — Multi-line text editor
- **spin** — Show a spinner while running a command
- **style** — Apply ANSI styles to strings
- **join** — Combine components

## Build

```bash
make
```
Requires: GCC 10+ or Clang 12+, GNU Make
