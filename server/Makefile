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
CXXFLAGS := -O0 -g -Wall -std=c++20

# Include flags
INCFLAGS = $(addprefix -I,$(LOCAL_INC_DIRS)) \
	`pkg-config --cflags opencv4` \
	`pkg-config --cflags yaml-cpp`\
	`pkg-config --cflags freetype2`\
	`pkg-config --cflags librtmp`\
	`pkg-config --cflags libavcodec`\
	`pkg-config --cflags libavutil`\
	`pkg-config --cflags libswscale`\
	-Icef -Icef/include
  

LDFLAGS = `pkg-config --libs opencv4` \
	`pkg-config --libs yaml-cpp` \
	`pkg-config --libs freetype2`\
	`pkg-config --libs librtmp`\
	`pkg-config --libs libavcodec`\
	`pkg-config --libs libavutil`\
	`pkg-config --libs libswscale`

ifeq ($(shell uname), Darwin)
	LDFLAGS += -Fcef/lib -framework 'Chromium Embedded Framework' cef/lib/libcef_dll_wrapper.a
else
	LDFLAGS += -Wl,-rpath=cef/lib -Lcef/lib -lcef cef/lib/libcef_dll_wrapper.a
endif

.PHONY: all
all: cef/.success led-wall-server

cef/.success:
	./scripts/get-cef.bash
	@touch cef/.success # Create .success file to indicate that script was run successfully

led-wall-server: $(OBJS)
	$(CXX) $^ -o $@ $(INCFLAGS) $(LDFLAGS)
ifeq ($(shell uname), Darwin)
	./scripts/make-macos-bundle.bash
endif

$(OBJ_DIR)/%.cpp.o: %.cpp $(INCS) | $(OBJ_DIR)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) cef led-wall-server*
