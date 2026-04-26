# =============================================================================
# Axiom — Professional Build System
# =============================================================================

CXX         := clang++
CC          := clang
CXXFLAGS    := -std=c++20 -O2 -Wall -Wextra -Wpedantic -Iinternal -Ipkg -Iconfigs
CFLAGS      := -O2 -Wall
LDFLAGS     := -lcurl

# Paths
OBJ         := build/obj
BIN         := axiom
PREFIX      := /usr/local
INSTALL_DIR := $(PREFIX)/bin

# Sources
ENGINE_SRCS  := internal/engine/data_engine.cpp \
                internal/engine/analysis_engine.cpp
SERVICE_SRCS := internal/service/analysis_service.cpp \
                internal/service/config_service.cpp \
                internal/service/repl_service.cpp
UI_SRCS      := internal/ui/heatmap.cpp
MAIN_SRC     := cmd/axiom/main.cpp
C_SRC        := internal/linenoise.c

# Objects
ENGINE_OBJS  := $(patsubst internal/%.cpp, $(OBJ)/%.o, $(ENGINE_SRCS))
SERVICE_OBJS := $(patsubst internal/%.cpp, $(OBJ)/%.o, $(SERVICE_SRCS))
UI_OBJS      := $(patsubst internal/%.cpp, $(OBJ)/%.o, $(UI_SRCS))
MAIN_OBJ     := $(OBJ)/cmd/axiom/main.o
C_OBJ        := $(OBJ)/linenoise.o

.PHONY: all clean install uninstall

all: $(BIN)

$(BIN): $(ENGINE_OBJS) $(SERVICE_OBJS) $(UI_OBJS) $(MAIN_OBJ) $(C_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "  ✓  $@ binary ready"

# Compile rules
$(OBJ)/%.o: internal/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/cmd/%.o: cmd/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/%.o: internal/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

install: $(BIN)
	@mkdir -p $(INSTALL_DIR)
	@cp $(BIN) $(INSTALL_DIR)/$(BIN)
	@chmod +x $(INSTALL_DIR)/$(BIN)
	@echo "  ✓  Installed to $(INSTALL_DIR)"

clean:
	rm -rf build $(BIN)
