include .env

CFLAGS = -std=c++17 -O2 -I$(TINYOBJ_PATH) -I$(STB_IMAGE_PATH) -I$(IMGUI_PATH) -I$(IMGUI_PATH)/backends -I. -Isrc
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

# create list of all spv files and set as dependency
vertSources = $(shell find ./shaders -type f -name "*.vert")
vertObjFiles = $(patsubst %.vert, %.vert.spv, $(vertSources))
fragSources = $(shell find ./shaders -type f -name "*.frag")
fragObjFiles = $(patsubst %.frag, %.frag.spv, $(fragSources))
compSources = $(shell find ./shaders -type f -name "*.comp")
compObjFiles = $(patsubst %.comp, %.comp.spv, $(compSources))

TARGET = VulkanOut.out
BUILD_DIR = build

SOURCES = $(shell find src -name "*.cpp") main.cpp

IMGUI_SOURCES = $(IMGUI_PATH)/imgui.cpp \
                $(IMGUI_PATH)/imgui_draw.cpp \
                $(IMGUI_PATH)/imgui_tables.cpp \
                $(IMGUI_PATH)/imgui_widgets.cpp \
                $(IMGUI_PATH)/imgui_demo.cpp \
                $(IMGUI_PATH)/backends/imgui_impl_glfw.cpp \
                $(IMGUI_PATH)/backends/imgui_impl_vulkan.cpp

ALL_SOURCES = $(SOURCES) $(IMGUI_SOURCES)
OBJECTS = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(ALL_SOURCES))
HEADERS = $(shell find src -name "*.hpp")

$(TARGET): $(vertObjFiles) $(fragObjFiles) $(compObjFiles) $(OBJECTS)
	g++ $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(patsubst %.cpp, $(BUILD_DIR)/%.o, $(SOURCES)): $(HEADERS)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	g++ $(CFLAGS) -c $< -o $@

# make shader targets
%.spv: %
	glslc $< -o $@

.PHONY: test clean shaders

test: VulkanOut.out
	./VulkanOut.out

shaders: $(vertObjFiles) $(fragObjFiles) $(compObjFiles)

clean:
	rm -f VulkanOut.out
	rm -rf $(BUILD_DIR)
