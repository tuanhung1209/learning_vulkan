include .env

CFLAGS = -std=c++17 -O2 -I$(TINYOBJ_PATH) -I.
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

# create list of all spv files and set as dependency
vertSources = $(shell find ./shaders -type f -name "*.vert")
vertObjFiles = $(patsubst %.vert, %.vert.spv, $(vertSources))
fragSources = $(shell find ./shaders -type f -name "*.frag")
fragObjFiles = $(patsubst %.frag, %.frag.spv, $(fragSources))

TARGET = VulkanOut.out
SOURCES = $(wildcard *.cpp) $(wildcard systems/*.cpp)
HEADERS = $(wildcard *.hpp) $(wildcard systems/*.hpp)

$(TARGET): $(vertObjFiles) $(fragObjFiles) $(SOURCES) $(HEADERS)
	g++ $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

# make shader targets
%.spv: %
	glslc $< -o $@

.PHONY: test clean

test: VulkanOut.out
	./VulkanOut.out

clean:
	rm -f VulkanOut.out
	rm -f shaders/*.spv