include .env

CFLAGS = -std=c++17 -O2 -I$(TINYOBJ_PATH) -I. -Isrc
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

# create list of all spv files and set as dependency
vertSources = $(shell find ./shaders -type f -name "*.vert")
vertObjFiles = $(patsubst %.vert, %.vert.spv, $(vertSources))
fragSources = $(shell find ./shaders -type f -name "*.frag")
fragObjFiles = $(patsubst %.frag, %.frag.spv, $(fragSources))

TARGET = VulkanOut.out
# Recursive search for cpp files in src
SOURCES = $(shell find src -name "*.cpp") main.cpp
HEADERS = $(shell find src -name "*.hpp")

$(TARGET): $(vertObjFiles) $(fragObjFiles) $(SOURCES) $(HEADERS)
	g++ $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

# make shader targets
%.spv: %
	glslc $< -o $@

.PHONY: test clean shaders

test: VulkanOut.out
	./VulkanOut.out

shaders: $(vertObjFiles) $(fragObjFiles)

clean:
	rm -f VulkanOut.out