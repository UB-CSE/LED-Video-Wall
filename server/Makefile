# Config
CXX:=g++
CPPFLAGS:=
CXXFLAGS:=-O0 -g -Wall -std=c++17
LDFLAGS:=-lyaml-cpp
SOURCES:=main tcp client config-parser
OBJ_DIR:=obj

# Variables
CXX_FILES:=$(foreach src, $(SOURCES), $(src).cpp)
OBJ_FILES:=$(foreach src, $(SOURCES), $(OBJ_DIR)/$(src).o)

# Build
all: led-wall-server

led-wall-server: $(OBJ_FILES)
	$(CXX) $^ -o $@ $(LDFLAGS)

$(OBJ_FILES): $(OBJ_DIR)/%.o: %.cpp obj
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

obj:
	mkdir obj

clean:
	rm $(OBJ_DIR)/*
