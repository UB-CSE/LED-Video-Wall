# Config File Locations
SRC_DIRS:=src
INC_DIRS:=inc
OBJ_DIR:=obj
# File Locations Variables
SRCS:=$(shell find $(SRC_DIR) -name '*.cpp')
OBJS:=$(foreach src, $(SRCS), $(OBJ_DIR)/$(src).o)
INC_DIRS:=$(shell find $(INC_DIRS) -type d)

# Config Compiler
CXX:=g++
CPPFLAGS:=
CXXFLAGS:=-O0 -g -Wall -std=c++17
INCFLAGS = $(addprefix -I,$(INC_DIRS)) `pkg-config --cflags opencv4`
LDFLAGS:=-lyaml-cpp `pkg-config --libs opencv4`

# Build
all: led-wall-server

led-wall-server: $(OBJS)
	$(CXX) $(INCFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.cpp.o: %.cpp obj
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCFLAGS) -c $< -o $@

clean:
	rm $(OBJS)
