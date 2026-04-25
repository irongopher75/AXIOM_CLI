# =============================================================================
# Axiom — Multi-TU Build System
# Compilers: clang++ (C++20) + clang (C, for linenoise)
# =============================================================================

CXX         := clang++
CC          := clang
CXXFLAGS    := -std=c++20 -O2 -Wall -Wextra -Wpedantic -Isrc/cpp
CFLAGS      := -O2 -Wall
LDFLAGS     := -lcurl

# Paths
SRC         := src/cpp
OBJ         := build/obj
BIN         := axiom
PREFIX      := /usr/local
INSTALL_DIR := $(PREFIX)/bin

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
ENGINE_SRCS := $(SRC)/engine/data_engine.cpp \
               $(SRC)/engine/analysis_engine.cpp

REPL_SRC    := $(SRC)/axiom_repl.cpp
C_SRC       := $(SRC)/linenoise.c

ENGINE_OBJS := $(patsubst $(SRC)/%.cpp, $(OBJ)/%.o, $(ENGINE_SRCS))
REPL_OBJ    := $(OBJ)/axiom_repl.o
C_OBJ       := $(OBJ)/linenoise.o

# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------
.PHONY: all clean test install uninstall

all: $(BIN)

# Final binary
$(BIN): $(ENGINE_OBJS) $(REPL_OBJ) $(C_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "  ✓  $@ binary ready"

# ---------------------------------------------------------------------------
# Compile rules
# ---------------------------------------------------------------------------
$(OBJ)/%.o: $(SRC)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# ---------------------------------------------------------------------------
# Installation
# ---------------------------------------------------------------------------
install: $(BIN)
	@echo "  ➔  Installing to $(INSTALL_DIR)..."
	@mkdir -p $(INSTALL_DIR)
	@cp $(BIN) $(INSTALL_DIR)/$(BIN)
	@chmod +x $(INSTALL_DIR)/$(BIN)
	@echo "  ✓  Done. You can now run '$(BIN)' from anywhere."

uninstall:
	@echo "  ➔  Removing from $(INSTALL_DIR)..."
	@rm -f $(INSTALL_DIR)/$(BIN)
	@echo "  ✓  Done."

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
test: build/test_price build/test_analysis
	@echo "\n--- test_price ---"
	./build/test_price
	@echo "\n--- test_analysis ---"
	./build/test_analysis

build/test_price: $(SRC)/tests/test_price.cpp | build
	$(CXX) $(CXXFLAGS) $< -o $@

build/test_analysis: $(SRC)/tests/test_analysis.cpp \
                     $(ENGINE_OBJS) $(C_OBJ) | build
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

build:
	mkdir -p $@

clean:
	rm -rf build $(BIN)
