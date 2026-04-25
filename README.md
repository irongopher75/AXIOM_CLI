# Axiom CLI (C++ Edition)

A high-performance, interactive institutional terminal for market analysis.

## Features
- **Interactive REPL**: A persistent session similar to Claude Code.
- **Markov Engine**: Predict market regimes using high-performance C++ math.
- **Premium UI**: Custom ANSI graphics and box-drawing elements.

## Installation
Ensure you have `libcurl` installed, then run:
```bash
make
sudo make install
```

## Usage
Start the interactive terminal from anywhere:
```bash
axiom
```

Inside the terminal:
- `predict AAPL`: Run Markov Chain prediction.
- `help`: See all commands.
- `clear`: Clear the screen.
- `quit`: Exit the terminal.
