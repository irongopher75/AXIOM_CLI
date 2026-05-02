# =============================================================================
# Axiom — Professional Build System
# =============================================================================

CXX         := clang++
CC          := clang
CXXFLAGS    := -std=c++20 -O2 -Wall -Wextra -Wpedantic -Iinternal -Ipkg -Iconfigs -MMD -MP
CFLAGS      := -O2 -Wall -MMD -MP
LDFLAGS     := -lcurl -lsqlite3

# Paths
OBJ         := build/obj
BIN         := axiom
PREFIX      := /usr/local
INSTALL_DIR := $(PREFIX)/bin

# Sources
ENGINE_SRCS  := internal/engine/data_engine.cpp \
                internal/engine/analysis_engine.cpp \
                internal/engine/sqlite_store.cpp
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

# Dependencies
DEPS := $(ENGINE_OBJS:.o=.d) $(SERVICE_OBJS:.o=.d) $(UI_OBJS:.o=.d) $(MAIN_OBJ:.o=.d) $(C_OBJ:.o=.d)

.PHONY: all clean install uninstall

all: $(BIN)

$(BIN): $(ENGINE_OBJS) $(SERVICE_OBJS) $(UI_OBJS) $(MAIN_OBJ) $(C_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "  ✓  $@ binary ready"

# Test Targets
TEST_SRCS := tests/test_main.cpp tests/test_ui.cpp
TEST_OBJS := $(patsubst tests/%.cpp, $(OBJ)/tests/%.o, $(TEST_SRCS))

test: $(ENGINE_OBJS) $(SERVICE_OBJS) $(UI_OBJS) $(C_OBJ) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o build/run_tests $(LDFLAGS)
	@echo "  ✓  Test binary ready"
	./build/run_tests

$(OBJ)/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -Itests -c $< -o $@

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

-include $(DEPS)

install: $(BIN)
	@mkdir -p $(INSTALL_DIR)
	@cp $(BIN) $(INSTALL_DIR)/$(BIN)
	@chmod +x $(INSTALL_DIR)/$(BIN)
	@echo "  ✓  Installed to $(INSTALL_DIR)"

uninstall:
	@rm -f $(INSTALL_DIR)/$(BIN)
	@echo "  ✓  Uninstalled from $(INSTALL_DIR)"

clean:
	rm -rf build $(BIN)
