#if defined(__GUIimgui__)
///
/// \name GUIimgui.h
///
/// A GUI implementation based on SDL2 + Dear ImGui.
/// Provides modern cross-platform rendering with excellent performance on all platforms.
///
/// \date 2024
/// \author Assistant & Mark Fisher
///

#ifndef GUIIMGUI_H_
#define GUIIMGUI_H_

#include "GUI.h"
#include "h19.h"

// SDL2 headers
#include <SDL.h>

// Dear ImGui headers
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"

/// \cond
#include <memory>
/// \endcond

typedef void (* tGUIImGuiKeyboardFunc)(unsigned char Key, int x, int y);

class GUIimgui: public GUI
{
  public:
    GUIimgui();
    virtual ~GUIimgui() override;

    // Interface functions.
    virtual void GUIDisplay(void) override;
    virtual void InitGUI(void) override;
    virtual void StartGUI(void) override;

    // Callback functions.
    virtual void SetKeyboardFunc(tKeyboardFunc KeyboardFunc) override;
    virtual void SetDisplayFunc(tDisplayFunc DisplayFunc) override;
    virtual void SetTimerFunc(unsigned int ms, tTimerFunc TimerFunc) override;

  private:
    // SDL2 window and renderer (cross-platform!)
    SDL_Window* window;
    SDL_Renderer* renderer;

    // ImGui state
    bool showAbout;
    bool showImGuiDemo;
    bool showConfig;
    bool running;

    // Display settings
    bool maintainAspectRatio;

    // Color settings
    float foregroundColor[3];  // RGB values 0-1
    float backgroundColor[3];  // RGB values 0-1
    int colorScheme;           // 0=Amber, 1=Green, 2=White, 3=Reverse

    // Rendering settings
    int linearTextureScale;    // Oversampling factor for linear filtering (1-8)

    // Window size caching (to avoid excessive SDL calls)
    int cachedWindowWidth;
    int cachedWindowHeight;
    bool windowSizeChanged;
    
    // Config dirty state tracking
    bool configDirty;
    
    // Original config values for cancel functionality
    bool originalMaintainAspectRatio;
    int originalColorScheme;
    int originalLinearTextureScale;
    float originalForegroundColor[3];
    float originalBackgroundColor[3];
    int originalWindowWidth;
    int originalWindowHeight;

    // Cached scaling values (avoid recalculating when window size unchanged)
    float cachedCharScaleX;
    float cachedCharScaleY;
    int cachedOffsetX;
    int cachedOffsetY;

    // UI responsiveness
    Uint32 lastUIEventTime;
    static const Uint32 UI_EVENT_RENDER_DURATION = 100; // Render for 100ms after UI events

    // Deferred window scaling (to avoid ImGui event conflicts)
    int pendingWindowScale;

    // Window size persistence
    int savedWindowWidth;
    int savedWindowHeight;

    // Configuration management
    void loadConfig();
    void saveConfig();
    std::string getConfigPath();

    // Input handling
    void processTextInput(SDL_TextInputEvent& textEvent);
    bool isScalingDisabled();

    // Callback function pointers
    static tKeyboardFunc GUIKeyboardFunc;
    static tDisplayFunc  GUITimerFunc;
    static tDisplayFunc  GUIDisplayFunc;
    static SDL_TimerID   timerID;

    // SDL Timer callback function
    static Uint32 SDLTimerCallback(Uint32 interval, void* param);

    // Font rendering
    unsigned char* fontTable;
    SDL_Texture* fontTextures[256];  // SDL2 textures (cross-platform!)
    void setupH19Font();
    void renderTerminal();
    void createFontTextures();

    // Screen redraw flag (shared between timer and main loop)
    static bool screenNeedsRedraw;

    // Terminal display constants
    static const int H19_CHAR_WIDTH = 8;
    static const int H19_CHAR_HEIGHT = 20;
    static const int H19_TERMINAL_COLS = 80;
    static const int H19_TERMINAL_ROWS = 25;
    static const int H19_BORDER_SIZE = 20;

    // Event handling  
    void handleEvents();
    void processKeyboard(SDL_KeyboardEvent& key);

    // Menu and UI rendering
    void renderMenuBar();
    void renderDialogs();
    void renderConfigWindow();

    // Color management
    void applyColorScheme();
    void updateFontTextures();
    
    // Config management
    void markConfigDirty();
    void saveOriginalConfig();
    void restoreOriginalConfig();

    // Rendering settings

    // Window scaling
    void setWindowScale(int scale);
};

#endif /* GUIIMGUI_H_ */
#endif
