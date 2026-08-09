# QEditNA — MinGW makefile
#
# Targets:
#   make            same as `make release`
#   make release    optimized build         -> build/QEditNA.exe
#   make debug      debug symbols, no opt   -> build/QEditNA.exe
#   make clean      remove build artifacts
#   make run        build and launch
#
# Disk writing is OFF by default (data-safety rule for v0.x). To produce a
# build that can actually save files:  make release WRITE=1

CXX      := g++
CXXFLAGS := -std=c++17 -municode -Wall -Wextra -Wpedantic
LDFLAGS  := -municode -mwindows -static-libgcc -static-libstdc++
LDLIBS   := -lgdi32 -luser32 -lshell32

ifeq ($(WRITE),1)
CXXFLAGS += -DQEDITNA_ENABLE_WRITE
endif

BUILD_DIR := build
TARGET    := $(BUILD_DIR)/QEditNA.exe

SOURCES := src/main.cpp \
           src/editor/editor_core.cpp \
           src/editor/command_engine.cpp \
           src/editor/macro_engine.cpp \
           src/io/file_io.cpp \
           src/ui/window.cpp \
           src/ui/status_bar.cpp \
           src/ui/command_menu.cpp \
           src/ui/input_box.cpp \
           src/ui/notice_box.cpp

OBJECTS := $(SOURCES:%.cpp=$(BUILD_DIR)/%.o)
DEPS    := $(OBJECTS:.o=.d)

.PHONY: all release debug clean run

all: release

release: CXXFLAGS += -O2 -DNDEBUG
release: $(TARGET)

debug: CXXFLAGS += -O0 -g -DDEBUG
debug: LDFLAGS := -municode
debug: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

run: release
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
