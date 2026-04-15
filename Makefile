CXX := g++
TARGET := worldcup2026
BUILD_DIR := build
SRC_DIR := src
INC_DIR := include

SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

	# Use 2>/dev/null to suppress pkg-config error messages if package is not found
FREEGLUT_CFLAGS := $(shell pkg-config --cflags freeglut 2>/dev/null)
FREEGLUT_LIBS_PC := $(shell pkg-config --libs freeglut gl 2>/dev/null)
OPENAL_CFLAGS := $(shell pkg-config --cflags openal 2>/dev/null)
OPENAL_LIBS := $(shell pkg-config --libs openal 2>/dev/null)

CPPFLAGS := -I$(INC_DIR) $(FREEGLUT_CFLAGS)
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pedantic
LDFLAGS :=
LDLIBS := -lm

# Add FreeGLUT/OpenGL libraries. Prioritize pkg-config, fallback to explicit.
ifeq ($(strip $(FREEGLUT_LIBS_PC)),)
	# If pkg-config fails, find the library path manually and add it to LDFLAGS.
	# This is more robust than relying on default system paths.
	FREEGLUT_LIB_PATH := $(shell find /usr/lib* -name libfreeglut.so -exec dirname {} \; | head -n 1)
	ifneq ($(strip $(FREEGLUT_LIB_PATH)),)
		LDFLAGS += -L$(FREEGLUT_LIB_PATH)
	endif
	LDLIBS += -lfreeglut -lGL
	FREEGLUT_STATUS := FreeGLUT not found by pkg-config, using fallback.
else
	LDLIBS += $(FREEGLUT_LIBS_PC)
	FREEGLUT_STATUS := FreeGLUT found by pkg-config.
endif

ifneq ($(strip $(OPENAL_LIBS)),)
CPPFLAGS += -DHAS_OPENAL=1 $(OPENAL_CFLAGS)
LDLIBS += $(OPENAL_LIBS)
AUDIO_MODE := OpenAL enabled
else
AUDIO_MODE := OpenAL not found (terminal bell fallback)
endif

.PHONY: all clean info

all: info $(TARGET)

info:
	@echo "FreeGLUT Status: $(FREEGLUT_STATUS)"
	@echo "Building $(TARGET) - $(AUDIO_MODE)"

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
