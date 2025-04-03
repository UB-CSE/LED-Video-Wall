# Config File Locations
SRC_DIRS := src
INC_DIRS := inc
OBJ_DIR   := obj

#Overwrite because I have it installed differently
OPENCV_INCLUDE_DIR ?= $(OPENCV_DIR)
YAML_CPP_INCLUDE_DIR ?= $(YAML_DIR)

OPENCV_LIB_DIR     ?= /usr/lib/x86_64-linux-gnu
YAML_CPP_LIB_DIR   ?= /usr/lib/x86_64-linux-gnu

# Source and object files
SRCS := $(shell find $(SRC_DIRS) -name '*.cpp')
OBJS := $(foreach src, $(SRCS), $(OBJ_DIR)/$(src).o)
LOCAL_INC_DIRS := $(shell find $(INC_DIRS) -type d)


CXX := g++
CPPFLAGS :=
CXXFLAGS := -O0 -g -Wall -std=c++17

# Include flags
INCFLAGS = $(addprefix -I,$(LOCAL_INC_DIRS)) \
           -I$(OPENCV_INCLUDE_DIR) \
           -I$(YAML_CPP_INCLUDE_DIR) \
		   -I/usr/include/freetype2 \
           `pkg-config --cflags opencv4`


LDFLAGS = -L$(YAML_CPP_LIB_DIR) -lyaml-cpp \
          -L$(OPENCV_LIB_DIR) \
		  -L/usr/local/lib/ \
		   -lfreetype \
		  `pkg-config --libs opencv4`

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
