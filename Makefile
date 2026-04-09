CXX := g++
TARGET := worldcup2026
BUILD_DIR := build
SRC_DIR := src
INC_DIR := include

SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

FREEGLUT_CFLAGS := $(shell pkg-config --cflags freeglut)
FREEGLUT_LIBS := $(shell pkg-config --libs freeglut gl)
OPENAL_CFLAGS := $(shell pkg-config --cflags openal 2>/dev/null)
OPENAL_LIBS := $(shell pkg-config --libs openal 2>/dev/null)

CPPFLAGS := -I$(INC_DIR) $(FREEGLUT_CFLAGS)
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pedantic
LDLIBS := $(FREEGLUT_LIBS) -lm

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
	@echo "Building $(TARGET) - $(AUDIO_MODE)"

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
