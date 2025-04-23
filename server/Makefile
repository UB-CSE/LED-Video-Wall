# Config File Locations
SRC_DIRS := src ../protocol/src
INC_DIRS := inc ../protocol/src
OBJ_DIR   := obj

# Source and object files
SRCS := $(shell find $(SRC_DIRS) -name '*.cpp')
OBJS := $(foreach src, $(SRCS), $(OBJ_DIR)/$(src).o)
LOCAL_INC_DIRS := $(shell find $(INC_DIRS) -type d)

CXX := g++
CPPFLAGS :=
CXXFLAGS := -O0 -g -Wall -std=c++17

# Include flags
INCFLAGS = $(addprefix -I,$(LOCAL_INC_DIRS)) \
        `pkg-config --cflags opencv4` \
	`pkg-config --cflags yaml-cpp`

LDFLAGS = `pkg-config --libs opencv4` \
	`pkg-config --libs yaml-cpp`

all: led-wall-server

led-wall-server: $(OBJS)
	$(CXX) $^ -o $@ $(INCFLAGS) $(LDFLAGS)

$(OBJ_DIR)/%.cpp.o: %.cpp | $(OBJ_DIR)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)
