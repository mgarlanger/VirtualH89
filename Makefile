# Base sources (all except GUI-specific files)
BASE_SOURCES = $(filter-out VirtualH89/Src/GUIglut.cpp VirtualH89/Src/GUIimgui.cpp VirtualH89/Src/GUIProfiler.cpp VirtualH89/Src/GUIwxWidgets.cpp VirtualH89/Src/VirtualH89%.cpp, $(wildcard VirtualH89/Src/*.cpp))

# GUI-specific sources
GLUT_SOURCES = VirtualH89/Src/GUIglut.cpp
IMGUI_GUI_SOURCES = VirtualH89/Src/GUIimgui.cpp VirtualH89/Src/GUIProfiler.cpp
IMGUI_LIB_SOURCES = VirtualH89/Src/imgui/imgui.cpp VirtualH89/Src/imgui/imgui_demo.cpp VirtualH89/Src/imgui/imgui_draw.cpp VirtualH89/Src/imgui/imgui_tables.cpp VirtualH89/Src/imgui/imgui_widgets.cpp VirtualH89/Src/imgui/imgui_impl_sdl2.cpp VirtualH89/Src/imgui/imgui_impl_sdlrenderer2.cpp
WXWIDGETS_SOURCES = VirtualH89/Src/GUIwxWidgets.cpp VirtualH89/Src/VirtualH89App.cpp VirtualH89/Src/VirtualH89Frame.cpp

# SDL2 Renderer approach - truly cross-platform, no OpenGL/Metal needed!

CHECK = `which scan-build`
UNCRUSTIFY = uncrustify

# Object file definitions
BASE_OBJECTS = $(subst .cpp,.o,$(BASE_SOURCES))
GLUT_OBJECTS = $(subst .cpp,.o,$(GLUT_SOURCES))
IMGUI_GUI_OBJECTS = $(subst .cpp,.o,$(IMGUI_GUI_SOURCES))
IMGUI_LIB_OBJECTS = $(subst .cpp,.o,$(IMGUI_LIB_SOURCES))
WXWIDGETS_OBJECTS = $(subst .cpp,.o,$(WXWIDGETS_SOURCES))

.PHONY: clean check uncrust

all: v89

CXXFLAGS = -g -std=c++11

# Default build with GLUT GUI
v89: CXXFLAGS += -D__GUIglut__
v89: $(BASE_OBJECTS) $(GLUT_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(BASE_OBJECTS) $(GLUT_OBJECTS) -lpthread -lGL -lglut

# Build with ImGui GUI using SDL2 Renderer - CLEAN VERSION (no profiling overhead)
v89-imgui: CXXFLAGS += -D__GUIimgui__ $(shell pkg-config --cflags sdl2)
v89-imgui: $(BASE_OBJECTS) $(IMGUI_GUI_OBJECTS) $(IMGUI_LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(BASE_OBJECTS) $(IMGUI_GUI_OBJECTS) $(IMGUI_LIB_OBJECTS) -lpthread $(shell pkg-config --libs sdl2)

# Build with ImGui GUI using SDL2 Renderer - PROFILING VERSION (with detailed diagnostics)
v89-imgui-profile: CXXFLAGS += -D__GUIimgui__ -DENABLE_PROFILING $(shell pkg-config --cflags sdl2)
v89-imgui-profile: $(BASE_OBJECTS) $(IMGUI_GUI_OBJECTS) $(IMGUI_LIB_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(BASE_OBJECTS) $(IMGUI_GUI_OBJECTS) $(IMGUI_LIB_OBJECTS) -lpthread $(shell pkg-config --libs sdl2)

# Build with wxWidgets GUI
v89-wx: CXXFLAGS += -D__GUIwx__ $(shell wx-config --cxxflags)
v89-wx: $(BASE_OBJECTS) $(WXWIDGETS_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(BASE_OBJECTS) $(WXWIDGETS_OBJECTS) -lpthread $(shell wx-config --libs)

# No special platform-specific compilation needed with SDL2 Renderer!

clean:
	rm -f *.o *.orig VirtualH89/Src/*.o VirtualH89/Src/imgui/*.o v89 v89-imgui v89-wx

check:
	$(CHECK) -stats -load-plugin alpha.cplusplus.VirtualCall -load-plugin alpha.deadcode.UnreachableCode -maxloop 20 -k --use-analyzer Xcode -o check xcodebuild

uncrust:
	$(UNCRUSTIFY) -c VirtualH89/uncrust.cfg --no-backup VirtualH89/Src/*.cpp VirtualH89/Src/*.h
