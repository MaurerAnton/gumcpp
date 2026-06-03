# gumcpp — Shell Scripting TUI Toolkit (C++ port of Gum)

A zero-dependency C++ port of [gum](https://github.com/charmbracelet/gum) — composable terminal UI components for shell scripts.

## Why gumcpp?

The original [gum](https://github.com/charmbracelet/gum) requires Go plus dozens of modules from the charmbracelet ecosystem. gumcpp compiles with a single `make` using only C++17 and pthreads.

## Quick Start

```bash
make
echo -e "Option A\nOption B\nOption C" | ./gumcpp choose
./gumcpp confirm "Are you sure?"
./gumcpp input "Enter your name"
```

## Components

| Command | Description |
|---------|-------------|
| `choose` | Interactive fuzzy selection from stdin |
| `confirm` | Yes/no prompt with customizable labels |
| `input` | Single-line text input with placeholder |
| `filter` | Interactive text filter from stdin |
| `format` | Apply formatting to text (bold, color, etc.) |
| `join` | Join arguments with separator |
| `spin` | Show a spinner while running a command |
| `style` | Apply ANSI styles (bold, dim, italic, underline, colors) |
| `write` | Multi-line text input (basic editor) |
| `table` | Render tabular data from stdin |
| `log` | Styled log output with levels (info, warn, error, debug) |

## Build

```bash
make
```
Requires: GCC 10+ or Clang 12+, GNU Make
