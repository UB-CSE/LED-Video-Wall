# Config File Locations
SRC_DIRS := src ../protocol/src
INC_DIRS := inc inc_dummy ../protocol/src
VPATH+=$(SRC_DIRS)
VPATH+=$(INC_DIRS)
OBJ_DIR   := obj

# Source and object files
SRCS := $(shell find $(SRC_DIRS) -name '*.cpp')
INCS := $(shell find $(INC_DIRS) -name '*.hpp')
OBJS := $(foreach src, $(SRCS), $(OBJ_DIR)/$(notdir $(src).o))
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

.PHONY: all
all: led-wall-server

led-wall-server: $(OBJS) $(INCS)
	$(CXX) $^ -o $@ $(INCFLAGS) $(LDFLAGS)

$(OBJ_DIR)/%.cpp.o: %.cpp | $(OBJ_DIR)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR)
