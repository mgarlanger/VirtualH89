#if defined(__GUIimgui__)
///
/// \file GUIimgui.cpp
///
/// ImGui-based GUI implementation using SDL2 backend.
/// Provides modern cross-platform rendering.
///
/// \date 2025
/// \author Mark Fisher
///

#include "GUIimgui.h"
#include "h19.h"        // For H19::GetH19() and screen data
#include "logger.h"
#include "GUIProfiler.h"  // Clean profiling system

#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include <SDL_syswm.h>  // For SDL_GetWindowWMInfo

#ifndef __APPLE__
#include <GL/gl.h>
#endif

// RENDER THROTTLING: Prevent Wayland resource leaks by limiting SDL_RenderPresent() frequency
static Uint32 lastPresentMs = 0;

static inline bool shouldPresentNow(Uint32 minIntervalMs) {
    Uint32 now = SDL_GetTicks();
    if (now - lastPresentMs < minIntervalMs) return false;
    lastPresentMs = now;
    return true;
}

// Dear ImGui implementation files
// Note: These are compiled separately as part of the build system

using namespace std;

// Static callback function pointers
tKeyboardFunc GUIimgui::GUIKeyboardFunc = nullptr;
tDisplayFunc  GUIimgui::GUITimerFunc     = nullptr;
tDisplayFunc  GUIimgui::GUIDisplayFunc   = nullptr;
SDL_TimerID   GUIimgui::timerID          = 0;
bool          GUIimgui::screenNeedsRedraw = true;  // Initial render needed

//
// SDL Timer callback function (runs independently of main loop)
//
Uint32 GUIimgui::SDLTimerCallback(Uint32 interval, void* param)
{
    if (GUITimerFunc) {
        GUITimerFunc();
    }

    if (H19::GetH19() && H19::GetH19()->checkUpdated()) {
        screenNeedsRedraw = true;  // Signal main loop to render
    }

    // Handle automation (profiling system manages this when enabled)
    GUIProfiler::getInstance().handleTimerTick(interval);

    return interval;
}

//
// Constructor - Set up SDL2 and Dear ImGui
//
GUIimgui::GUIimgui()
    : window(nullptr)
    , renderer(nullptr)
    , showAbout(false)
    , showImGuiDemo(false)
    , showConfig(false)
    , running(true)
    , maintainAspectRatio(true)
    , colorScheme(0)  // Default: Amber
    , linearTextureScale(8)   // Default: 8x oversampling (sweet spot)
    , cachedWindowWidth(0)
    , cachedWindowHeight(0)
    , windowSizeChanged(true)
    , configDirty(false)
    , cachedCharScaleX(1.0f)
    , cachedCharScaleY(1.0f)
    , cachedOffsetX(0)
    , cachedOffsetY(0)
    , lastUIEventTime(0)
    , pendingWindowScale(0)
    , savedWindowWidth(1360)  // Default 2x scale
    , savedWindowHeight(1080)
{
    // Use inverted font table (same as GLUT version)
    fontTable = (unsigned char*) fontTableInverted;

    // Initialize font texture array
    memset(fontTextures, 0, sizeof(fontTextures));

    // Initialize color settings - Amber on black (default)
    foregroundColor[0] = 1.0f; // Red
    foregroundColor[1] = 1.0f; // Green  
    foregroundColor[2] = 0.0f; // Blue (amber)
    backgroundColor[0] = 0.0f; // Black background
    backgroundColor[1] = 0.0f; 
    backgroundColor[2] = 0.0f;


    debugss(ssH19, INFO, "GUIimgui constructor - SDL2 Renderer backend (cross-platform!)\n");
}

//
// Destructor - Clean up SDL2 and Dear ImGui resources  
//
GUIimgui::~GUIimgui()
{
    debugss(ssH19, INFO, "GUIimgui destructor\n");

    // Save configuration to disk
    saveConfig();

    // Stop SDL timer if running
    if (timerID != 0) {
        SDL_RemoveTimer(timerID);
        timerID = 0;
    }

    // Cleanup font textures
    for (int i = 0; i < 256; i++) {
        if (fontTextures[i]) {
            SDL_DestroyTexture(fontTextures[i]);
            fontTextures[i] = nullptr;
        }
    }

    // Stop SDL text input
    SDL_StopTextInput();

    // Cleanup Dear ImGui
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    // Cleanup SDL2 resources
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }

    // Cleanup SDL2
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

//
// Initialize the GUI system
//
void GUIimgui::InitGUI(void)
{
    debugss(ssH19, INFO, "Initializing SDL2 + Dear ImGui GUI with SDL2 Renderer backend\n");

    // Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        cerr << "Error: SDL_Init(): " << SDL_GetError() << endl;
        return;
    }

    // Load configuration from disk (before creating window so size is applied correctly)
    loadConfig();

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Set hints for proper window decorations on tiling window managers like Hyprland
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
    SDL_SetHint(SDL_HINT_VIDEO_X11_WINDOW_VISUALID, "");
    
    // Enable linear filtering for smooth text rendering
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    // Create window with SDL_Renderer graphics context  
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE);

    window = SDL_CreateWindow("VirtualH89 - Heathkit H-89 Emulator (SDL2 Renderer)", 
                             SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                             savedWindowWidth, savedWindowHeight,  // Use saved window size
                             window_flags);

    if (!window) {
        cerr << "Error: SDL_CreateWindow(): " << SDL_GetError() << endl;
        return;
    }

    // Create SDL2 renderer - DISABLE VSYNC, otherwise performance is poor, we protect this with render throttling
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    printf("SDL2 Renderer: VSync DISABLED (affects performance when set true)\n");
    if (!renderer) {
        cerr << "Error: SDL_CreateRenderer(): " << SDL_GetError() << endl;
        return;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Configure ImGui display size
    int window_width, window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);
    io.DisplaySize = ImVec2((float)window_width, (float)window_height);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends - SDL2 renderer is truly cross-platform!
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // Enable SDL text input for proper character handling (including shift modifiers)
    SDL_StartTextInput();

#ifdef __APPLE__
    // Set up event filter for macOS resize handling
    // This helps capture resize events during dragging on macOS
    SDL_SetEventFilter([](void* userdata, SDL_Event* event) -> int {
        if (event->type == SDL_WINDOWEVENT && 
            (event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED || 
             event->window.event == SDL_WINDOWEVENT_RESIZED)) {

            GUIimgui* gui = static_cast<GUIimgui*>(userdata);
            if (gui) {
                // Handle user-initiated resizes
                gui->windowSizeChanged = true;

                // Render immediately from event filter (macOS blocks main loop during resize)
                if (GUIDisplayFunc) {
                    GUIDisplayFunc();
                }
            }

            // Return 1 to allow event to continue to main loop for proper cleanup
            return 1;
        }
        return 1; // Allow other events to continue processing
    }, this);
#endif

    setupH19Font();

    // Apply loaded colors to font textures (config was loaded earlier)
    updateFontTextures();

    cout << "SDL2 + Dear ImGui GUI with SDL2 Renderer backend initialized successfully" << endl;
}

//
// Setup the authentic H19 terminal font for ImGui
//
void GUIimgui::setupH19Font()
{
    debugss(ssH19, INFO, "Setting up H19 bitmap font textures for ImGui rendering\n");

    createFontTextures();

    debugss(ssH19, INFO, "H19 font textures created successfully\n");
}

//
// Create ImGui textures from H19 bitmap font data
//
void GUIimgui::createFontTextures()
{
    const int FONT_WIDTH = 8;
    const int FONT_HEIGHT = 20;
    const int BYTES_PER_CHAR = 20;  // 20 bytes per character in font table


    // Create RGBA texture data for each character
    for (int i = 0; i < 256; i++)
    {
        // Allocate RGBA texture data (4 bytes per pixel)
        unsigned char* textureData = new unsigned char[FONT_WIDTH * FONT_HEIGHT * 4];
        memset(textureData, 0, FONT_WIDTH * FONT_HEIGHT * 4);

        // Convert 1-bit bitmap to RGBA texture
        // Note: Flip Y-axis for ImGui (ImGui Y=0 at top, font data Y=0 at bottom)
        for (int row = 0; row < FONT_HEIGHT; row++)
        {
            // Flip the row: read from bottom to top of font data
            int flipped_row = (FONT_HEIGHT - 1) - row;
            unsigned char fontByte = fontTable[i * BYTES_PER_CHAR + flipped_row];

            for (int col = 0; col < FONT_WIDTH; col++)
            {
                int pixelIndex = (row * FONT_WIDTH + col) * 4;

                // Check if this pixel is set (bit testing)
                if (fontByte & (1 << (7 - col)))
                {
                    // White/amber pixel (foreground)
                    textureData[pixelIndex + 0] = 255;  // Red
                    textureData[pixelIndex + 1] = 255;  // Green  
                    textureData[pixelIndex + 2] = 0;    // Blue (0 for amber)
                    textureData[pixelIndex + 3] = 255;  // Alpha
                }
                else
                {
                    // Transparent pixel (background)
                    textureData[pixelIndex + 0] = 0;    // Red
                    textureData[pixelIndex + 1] = 0;    // Green
                    textureData[pixelIndex + 2] = 0;    // Blue
                    textureData[pixelIndex + 3] = 0;    // Alpha (transparent)
                }
            }
        }

        // Create SDL2 texture safely - avoid referencing external memory
        SDL_Surface* surface = SDL_CreateRGBSurface(0, FONT_WIDTH, FONT_HEIGHT, 32, 
                                                    0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
        if (!surface) {
            cerr << "Error creating SDL surface for character " << i << ": " << SDL_GetError() << endl;
            fontTextures[i] = nullptr;
            delete[] textureData;
            continue;
        }

        // Copy our texture data into the surface
        SDL_LockSurface(surface);
        memcpy(surface->pixels, textureData, FONT_WIDTH * FONT_HEIGHT * 4);
        SDL_UnlockSurface(surface);

        fontTextures[i] = SDL_CreateTextureFromSurface(renderer, surface);
        if (!fontTextures[i]) {
            cerr << "Error creating SDL texture for character " << i << ": " << SDL_GetError() << endl;
        }

        SDL_FreeSurface(surface);

        // Clean up temporary texture data
        delete[] textureData;
    }
}

//
// Render the main menu bar
//
void GUIimgui::renderMenuBar()
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save Config", "ALT+S")) {
                saveConfig();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "ALT+Q")) {
                running = false;  // Signal to exit main loop
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Config...", "ALT+C")) {
                showConfig = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About VirtualH89")) {
                showAbout = true;
            }
            if (ImGui::MenuItem("Show ImGui Demo")) {
                showImGuiDemo = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

//
// Render dialogs and popups
//
void GUIimgui::renderDialogs()
{
    // About dialog
    if (showAbout) {
        if (ImGui::Begin("About VirtualH89", &showAbout, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("VirtualH89 - Heathkit H-89 Emulator");
            ImGui::Text("SDL2 + Dear ImGui Version");
            ImGui::Separator();
            ImGui::Text("Based on Virtual H89");
            ImGui::Text("Copyright (C) 2009-2016 by Mark Garlanger");
            ImGui::Text("Release 1.93");
            ImGui::Separator();
            ImGui::Text("Portions derived from Z80Pack Release 1.17");
            ImGui::Text("Copyright (C) 1987-2008 by Udo Munk");
            ImGui::Separator();
            ImGui::Text("Built with:");
            ImGui::BulletText("SDL2 for cross-platform windowing");
            ImGui::BulletText("Dear ImGui for modern UI");
            ImGui::BulletText("SDL2 version by Mark Fisher");
            ImGui::Separator();
            ImGui::Text("Display Settings:");
            ImGui::BulletText("Aspect Ratio: %s", maintainAspectRatio ? "Maintained" : "Stretch to Fit");

            if (ImGui::Button("OK")) {
                showAbout = false;
            }
        }
        ImGui::End();
    }

    // ImGui demo window
    if (showImGuiDemo) {
        ImGui::ShowDemoWindow(&showImGuiDemo);
    }

    // Config window
    if (showConfig) {
        renderConfigWindow();
    }
}

//
// Main display function - renders the terminal
//
void GUIimgui::GUIDisplay(void)
{
    PROFILE_FRAME_START();

    // Handle automation profiling (cursor stability detection)
    H19* h19 = H19::GetH19();
    if (h19) {
        PROFILE_CURSOR_STABILITY(h19->posX_m, h19->posY_m, SDL_GetTicks());
    }

    // Clear screen with background color
    SDL_SetRenderDrawColor(renderer, 
                          (Uint8)(backgroundColor[0] * 255),
                          (Uint8)(backgroundColor[1] * 255), 
                          (Uint8)(backgroundColor[2] * 255), 255);
    SDL_RenderClear(renderer);
    PROFILE_FRAME_STAGE("clear");

    // Render terminal
    renderTerminal();
    PROFILE_FRAME_STAGE("terminal");

    // Setup ImGui frame
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    PROFILE_FRAME_STAGE("imgui_start");

    // Render menu and dialogs
    renderMenuBar();
    renderDialogs();

    // Render ImGui
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
    PROFILE_FRAME_STAGE("imgui_end");

    // Note: Pending window scaling is processed in the main loop, not here

    // Present frame
    SDL_RenderPresent(renderer);

    // Complete frame profiling
    PROFILE_FRAME_END();
    PROFILE_REPORT_STATS();
}

//
// Render the H19 terminal display using ImGui
//
void GUIimgui::renderTerminal()
{
    PROFILE_MUTEX_ACQUIRED();

    // Get H19 instance for screen data
    H19* h19 = H19::GetH19();
    if (!h19) {
        return;
    }


    // SIMPLE: Only get window size when we receive resize events
    // The main render throttling prevents resource leaks
    int window_width = cachedWindowWidth;
    int window_height = cachedWindowHeight;

    if (windowSizeChanged) {
        // Get current window size
        SDL_GetWindowSize(window, &window_width, &window_height);
        windowSizeChanged = false; // Reset the flag
    } else {
        // On macOS, we need to continuously update during resize to avoid "snapshot" behavior
        // Check if we're currently resizing by polling the window size
        int current_width, current_height;
        SDL_GetWindowSize(window, &current_width, &current_height);
        if (current_width != window_width || current_height != window_height) {
            window_width = current_width;
            window_height = current_height;
            // Force recalculation of scaling
            cachedCharScaleX = 0.0f;
            cachedCharScaleY = 0.0f;

            // Continuous updates during resize (macOS)
        }
    }

    // Use full window size for terminal rendering
    int available_width = window_width;
    int available_height = window_height;


    // Track aspect ratio changes and forced recalculations
    static bool lastAspectRatioMode = maintainAspectRatio; 
    bool sizeChanged = (window_width != cachedWindowWidth || window_height != cachedWindowHeight);
    bool aspectRatioChanged = (maintainAspectRatio != lastAspectRatioMode);
    bool forceRecalc = (cachedCharScaleX == 0.0f || cachedCharScaleY == 0.0f);  // Forced recalc from scaling

    if (sizeChanged || aspectRatioChanged || forceRecalc) {
        if (sizeChanged) {
            SDL_RenderSetViewport(renderer, NULL);  // NULL = full render target
            cachedWindowWidth = window_width;
            cachedWindowHeight = window_height;
        }

        // Recalculate scaling when size or aspect ratio changes
        float terminal_width = H19_TERMINAL_COLS * H19_CHAR_WIDTH + 2 * H19_BORDER_SIZE;
        float terminal_height = H19_TERMINAL_ROWS * H19_CHAR_HEIGHT + 2 * H19_BORDER_SIZE;
        float scale_x = (float)window_width / terminal_width;
        float scale_y = (float)window_height / terminal_height;

        // Choose scaling based on aspect ratio setting
        if (maintainAspectRatio) {
            // Maintain aspect ratio - use smaller scale to fit entirely
            float scale = (scale_x < scale_y) ? scale_x : scale_y;
            float scaled_width = terminal_width * scale;
            float scaled_height = terminal_height * scale;
            cachedOffsetX = (int)((window_width - scaled_width) * 0.5f);
            cachedOffsetY = (int)((window_height - scaled_height) * 0.5f);
            cachedCharScaleX = cachedCharScaleY = scale;
        } else {
            // Stretch to fill window with separate X/Y scaling
            cachedOffsetX = cachedOffsetY = 0;
            cachedCharScaleX = scale_x;
            cachedCharScaleY = scale_y;
        }

        lastAspectRatioMode = maintainAspectRatio;
    }

    // FAST: Render each character using SDL2 (batched automatically!)
    // Note: screen_m is column-major: [col][row]
    for (int row = 0; row < H19_TERMINAL_ROWS; row++) {
        for (int col = 0; col < h19->cols_c; col++) {
            unsigned int charIndex = h19->screen_m[col][row];

            // Skip empty/null characters
            if (charIndex == 0 || charIndex >= 256 || !fontTextures[charIndex]) continue;

            // Calculate character position with cached scaling values
            SDL_Rect dest_rect;
            dest_rect.x = cachedOffsetX + (int)((H19_BORDER_SIZE + col * H19_CHAR_WIDTH) * cachedCharScaleX);
            dest_rect.y = cachedOffsetY + (int)((H19_BORDER_SIZE + row * H19_CHAR_HEIGHT) * cachedCharScaleY);
            dest_rect.w = (int)(H19_CHAR_WIDTH * cachedCharScaleX);
            dest_rect.h = (int)(H19_CHAR_HEIGHT * cachedCharScaleY);

            // FAST: Single SDL2 call - automatically batched!
            SDL_RenderCopy(renderer, fontTextures[charIndex], nullptr, &dest_rect);
        }
    }

    // Render cursor if visible
    if (!h19->cursorOff_m && h19->curCursor_m) {
        unsigned int cursor = h19->cursorBlock_m ? (128 + 32) : 27;

        if (cursor < 256 && fontTextures[cursor]) {
            SDL_Rect cursor_rect;
            cursor_rect.x = cachedOffsetX + (int)((H19_BORDER_SIZE + (h19->posX_m % h19->cols_c) * H19_CHAR_WIDTH) * cachedCharScaleX);
            cursor_rect.y = cachedOffsetY + (int)((H19_BORDER_SIZE + h19->posY_m * H19_CHAR_HEIGHT) * cachedCharScaleY);
            cursor_rect.w = (int)(H19_CHAR_WIDTH * cachedCharScaleX);
            cursor_rect.h = (int)(H19_CHAR_HEIGHT * cachedCharScaleY);

            // FAST: Render cursor with SDL2  
            SDL_RenderCopy(renderer, fontTextures[cursor], nullptr, &cursor_rect);
        }
    }
}

//
// Handle SDL2 events and convert to H19 key codes
//
void GUIimgui::handleEvents()
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);

        // Mark UI events that might need responsive rendering
        if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN || 
            event.type == SDL_MOUSEBUTTONUP || event.type == SDL_KEYDOWN ||
            event.type == SDL_KEYUP) {
            lastUIEventTime = SDL_GetTicks();
        }

        switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;

// disable text input for profiling, this is handled by ascii characters in processKeyboard
#ifndef ENABLE_PROFILING
            case SDL_TEXTINPUT:
                // Handle text input (characters with shift modifiers already applied)
                processTextInput(event.text);
                break;
#endif

            case SDL_WINDOWEVENT:
                if (event.window.windowID == SDL_GetWindowID(window)) {
                    if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                        running = false;
                    } else if (event.window.event == SDL_WINDOWEVENT_RESIZED || 
                               event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        // Mark the cached sizes dirty like you already do
                        windowSizeChanged = true;
                        // Mark config as dirty when window is manually resized
                        markConfigDirty();

                        // Save new size
                        SDL_GetWindowSize(window, &savedWindowWidth, &savedWindowHeight);

                        // IMPORTANT: treat this as a "hot" UI event so StartGUI renders
                        lastUIEventTime = SDL_GetTicks();


#ifdef __APPLE__
                        // On macOS, present *now* so the OS doesn't just stretch the last frame.
                        // Render immediately in response to the resize event.
                        if (GUIDisplayFunc) {
                            GUIDisplayFunc();           // draws + SDL_RenderPresent()
                            screenNeedsRedraw = false;  // we've just drawn
                        }
#endif
                    } else if (event.window.event == SDL_WINDOWEVENT_EXPOSED) {
                        // Some WMs/OSes fire this during live-resize; render on it too.
                        if (GUIDisplayFunc) {
                            GUIDisplayFunc();
                            screenNeedsRedraw = false;
                        }
                    }
                }
                break;

            case SDL_KEYDOWN:
                processKeyboard(event.key);
                break;
        }
    }
}

//
// Process text input (characters with shift modifiers already applied)
//
void GUIimgui::processTextInput(SDL_TextInputEvent& textEvent)
{
    // Get the first character from the text input
    unsigned char h19_key = (unsigned char)textEvent.text[0];

    // Send to emulation core if we have a callback and it's a valid character
    if (GUIKeyboardFunc && h19_key != 0) {
        GUIKeyboardFunc(h19_key);
    }
}

bool GUIimgui::isScalingDisabled()
{
    // Check if scaling should be disabled (Wayland tiled windows)
    const char* videoDriver = SDL_GetCurrentVideoDriver();
    if (videoDriver && strcmp(videoDriver, "wayland") == 0) {
        Uint32 currentFlags = SDL_GetWindowFlags(window);
        return (currentFlags & SDL_WINDOW_MAXIMIZED) != 0;
    }
    return false;
}

//
// Process keyboard input and send to emulation core
//
void GUIimgui::processKeyboard(SDL_KeyboardEvent& key)
{
    // Convert SDL key codes to H19 key codes
    unsigned char h19_key = 0;

    // Handle special key combinations first
    SDL_Keymod keymod = SDL_GetModState();

    // Use ALT key for application shortcuts
    if (keymod & KMOD_ALT) {
        // even though each platform has a "kill app" key, we use ALT+Q for consistency and to close resources cleanly
        if (key.keysym.sym == SDLK_q) {
            running = false;
            return;
        }

        if (key.keysym.sym == SDLK_s) {
            saveConfig();
            return;
        }

        if (key.keysym.sym == SDLK_c) {
            showConfig = true;
            return;
        }

        if (key.keysym.sym >= SDLK_1 && key.keysym.sym <= SDLK_4) {
            // Handle scaling shortcuts
            if (!isScalingDisabled()) {
                setWindowScale(key.keysym.sym - SDLK_1 + 1);
                return;
            }
        }
    }

    // Handle Ctrl key combinations - convert to ASCII control characters
    if (keymod & KMOD_CTRL) {
        // Convert Ctrl+letter to ASCII control character (Ctrl+A=1, Ctrl+B=2, etc.)
        if (key.keysym.sym >= SDLK_a && key.keysym.sym <= SDLK_z) {
            h19_key = (unsigned char)(key.keysym.sym - SDLK_a + 1);
        }
        // Ctrl+@ = 0, Ctrl+[ = 27, Ctrl+\ = 28, Ctrl+] = 29, Ctrl+^ = 30, Ctrl+_ = 31
        else if (key.keysym.sym == SDLK_AT) {
            h19_key = 0;
        }
        else if (key.keysym.sym == SDLK_LEFTBRACKET) {
            h19_key = 27; // ESC
        }
        else if (key.keysym.sym == SDLK_BACKSLASH) {
            h19_key = 28;
        }
        else if (key.keysym.sym == SDLK_RIGHTBRACKET) {
            h19_key = 29;
        }
        else if (key.keysym.sym == SDLK_CARET) {
            h19_key = 30;
        }
        else if (key.keysym.sym == SDLK_UNDERSCORE) {
            h19_key = 31;
        }
    }

    // Handle special keys first
    switch (key.keysym.sym) {
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            h19_key = 13; // CR (Carriage Return)
            break;

        case SDLK_BACKSPACE:
            h19_key = 8; // BS (Backspace)
            break;

        case SDLK_TAB:
            h19_key = 9; // HT (Horizontal Tab)
            break;

        case SDLK_ESCAPE:
            h19_key = 27; // ESC
            break;

        case SDLK_DELETE:
            h19_key = 127; // DEL
            break;

        // Function keys (match GLUT encoding)
        case SDLK_F1:
            h19_key = 'S' | 0x80;
            break;
        case SDLK_F2:
            h19_key = 'T' | 0x80;
            break;
        case SDLK_F3:
            h19_key = 'U' | 0x80;
            break;
        case SDLK_F4:
            h19_key = 'V' | 0x80;
            break;
        case SDLK_F5:
            h19_key = 'W' | 0x80;
            break;
        case SDLK_F6:
            h19_key = 'P' | 0x80;
            break;
        case SDLK_F7:
            h19_key = 'Q' | 0x80;
            break;
        case SDLK_F8:
            h19_key = 'R' | 0x80;
            break;

        // Arrow keys and HOME (match GLUT encoding exactly)
        case SDLK_HOME:
            h19_key = 'H' | 0x80;
            break;
        case SDLK_UP:
            h19_key = 'A' | 0x80;
            break;
        case SDLK_DOWN:
            h19_key = 'B' | 0x80;
            break;
        case SDLK_LEFT:
            h19_key = 'D' | 0x80;
            break;
        case SDLK_RIGHT:
            h19_key = 'C' | 0x80;
            break;

        default:
// Only handle printable characters when profiling
// Otherwise we don't process SHIFT modifiers correctly
#ifdef ENABLE_PROFILING
            if (key.keysym.sym >= 32 && key.keysym.sym <= 126) {
                h19_key = (unsigned char)key.keysym.sym;
            }
#endif
            break;
    }

    // Key mapping complete (debug output removed for performance)

    // Send to emulation core if we have a callback
    if (GUIKeyboardFunc && h19_key != 0) {
        GUIKeyboardFunc(h19_key);
    }
}

//
// Start the main GUI loop
//
void GUIimgui::StartGUI(void)
{
    debugss(ssH19, INFO, "Starting SDL2 + Dear ImGui main loop\n");

    // Main loop (timer now runs independently via SDL_AddTimer)
    while (running) {

        handleEvents();

        // Process any pending window scaling (safe to do in main loop)
        if (pendingWindowScale > 0) {
            setWindowScale(pendingWindowScale);
            pendingWindowScale = 0;  // Clear the pending request
        }

        // **SMART RENDERING**: Render when screen changes OR when UI needs responsiveness
        // Timer sets screenNeedsRedraw flag via checkUpdated(), we check flag here

        // Check if we recently had UI events that might need responsive rendering
        Uint32 currentTime = SDL_GetTicks();
        bool uiNeedsRender = (currentTime - lastUIEventTime) < UI_EVENT_RENDER_DURATION;
        if (screenNeedsRedraw || uiNeedsRender) {
            // RENDER THROTTLING: Choose tighter cap when UI is "hot"
            const bool uiHot = uiNeedsRender; 
            const Uint32 minInterval = uiHot ? 8 /*~120 FPS*/ : 16 /*~60 FPS for normal operation*/;

            if (!shouldPresentNow(minInterval)) {
                SDL_Delay(1);         // yield a little so we don't tight-spin
            } else {
                if (GUIDisplayFunc) {
                    GUIDisplayFunc();  // This acquires h19_mutex for screen read
                }
                screenNeedsRedraw = false;  // Clear flag after rendering
            }
        } else {
            // No updates needed - sleep to avoid busy-waiting and reduce CPU usage  
            SDL_Delay(1);
        }
    }
}

//
// Set keyboard callback function
//
void GUIimgui::SetKeyboardFunc(tKeyboardFunc KeyboardFunc)
{
    GUIKeyboardFunc = KeyboardFunc;
}

//
// Set display callback function  
//
void GUIimgui::SetDisplayFunc(tDisplayFunc DisplayFunc)
{
    GUIDisplayFunc = DisplayFunc;
}

//
// Set timer callback function
//
void GUIimgui::SetTimerFunc(unsigned int ms, tTimerFunc TimerFunc)
{
    GUITimerFunc = TimerFunc;

    // Stop existing timer if running
    if (timerID != 0) {
        SDL_RemoveTimer(timerID);
        timerID = 0;
    }

    // Start SDL timer (runs independently of main loop like glutTimerFunc)
    if (ms > 0 && TimerFunc != nullptr) {
        timerID = SDL_AddTimer(ms, SDLTimerCallback, nullptr);
        if (timerID == 0) {
            printf("Error: Failed to create SDL timer: %s\n", SDL_GetError());
        }
    }
}

//
// Render the configuration window
//
void GUIimgui::renderConfigWindow()
{
    if (ImGui::Begin("Configuration", &showConfig, ImGuiWindowFlags_AlwaysAutoResize)) {
        // Save original values when config window first opens
        static bool configWindowJustOpened = false;
        if (showConfig && !configWindowJustOpened) {
            saveOriginalConfig();
            configWindowJustOpened = true;
        } else if (!showConfig) {
            configWindowJustOpened = false;
        }
        ImGui::Text("Display Settings");
        ImGui::Separator();

        // Aspect ratio setting
        if (ImGui::Checkbox("Maintain Aspect Ratio", &maintainAspectRatio)) {
            // Aspect ratio change will be handled in renderTerminal()
            markConfigDirty();
        }

        // Quick scaling options
        ImGui::Spacing();
        ImGui::Text("Quick Scale (680x540 base):");

        bool scalingDisabled = isScalingDisabled();

        if (scalingDisabled) {
            // Show disabled buttons with explanation
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("1x (680x540)##scale")) {
            // Defer window scaling to avoid ImGui event conflicts
            pendingWindowScale = 1;
            markConfigDirty();
        }
        ImGui::SameLine();
        if (ImGui::Button("2x (1360x1080)##scale")) {
            pendingWindowScale = 2;
            markConfigDirty();
        }
        ImGui::SameLine();
        if (ImGui::Button("3x (2040x1620)##scale")) {
            pendingWindowScale = 3;
            markConfigDirty();
        }
        ImGui::SameLine();
        if (ImGui::Button("4x (2720x2160)##scale")) {
            pendingWindowScale = 4;
            markConfigDirty();
        }

        if (scalingDisabled) {
            ImGui::EndDisabled();
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Scaling disabled: Window is tiled/maximized in Wayland");
        }

        // Show current window info and potential issues
        int currentWidth, currentHeight;
        SDL_GetWindowSize(window, &currentWidth, &currentHeight);
        int drawableWidth, drawableHeight;
        SDL_GetRendererOutputSize(renderer, &drawableWidth, &drawableHeight);

        ImGui::Text("Current window: %dx%d", currentWidth, currentHeight);

        const char* videoDriver = SDL_GetCurrentVideoDriver();
        if (videoDriver) {
            ImGui::Text("Video driver: %s", videoDriver);
            if (strcmp(videoDriver, "wayland") == 0) {
                // Check if window appears to be tiled/maximized
                Uint32 currentFlags = SDL_GetWindowFlags(window);
                if (currentFlags & SDL_WINDOW_MAXIMIZED) {
                    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "🚫 WAYLAND LIMITATION: Scaling disabled");
                    ImGui::TextWrapped("Your window is in tiled/maximized mode. Wayland prevents programmatic resizing of tiled windows for security reasons.");
                    ImGui::Separator();
                    ImGui::Text("Solutions:");
                    ImGui::BulletText("Switch to floating mode in your window manager");
                    ImGui::BulletText("Use: SDL_VIDEODRIVER=x11 ./v89-imgui");
                    ImGui::BulletText("Manually resize window to desired size");
                    if (ImGui::Button("Copy X11 Launch Command")) {
                        ImGui::SetClipboardText("SDL_VIDEODRIVER=x11 ./v89-imgui");
                    }
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "⚠ Wayland: Limited scaling support");
                    ImGui::TextWrapped("Scaling may move window to tiled mode");
                    ImGui::BulletText("First use may lose floating status");
                    ImGui::BulletText("For reliable scaling: Use X11 mode");
                }
            } else if (strcmp(videoDriver, "x11") == 0) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ X11: Full scaling support available");
                ImGui::TextDisabled("Perfect scaling with floating windows maintained");
            }
        }

        ImGui::Spacing();
        ImGui::Text("Rendering Settings");
        ImGui::Separator();

        // Oversampling setting
        if (ImGui::SliderInt("Oversampling", &linearTextureScale, 1, 8, "%dx")) {
            updateFontTextures();
            markConfigDirty();
        }

        ImGui::Spacing();
        ImGui::Text("Color Settings");
        ImGui::Separator();

        // Color scheme selection
        const char* schemes[] = { "Amber on Black", "Green on Black", "White on Black", "Black on White" };
        if (ImGui::Combo("Color Scheme", &colorScheme, schemes, 4)) {
            applyColorScheme();
            updateFontTextures();
            markConfigDirty();
        }

        ImGui::Spacing();

        // Custom color pickers
        bool colorChanged = false;
        colorChanged |= ImGui::ColorEdit3("Foreground Color", foregroundColor);
        colorChanged |= ImGui::ColorEdit3("Background Color", backgroundColor);

        if (colorChanged) {
            colorScheme = -1; // Mark as custom
            updateFontTextures();
            markConfigDirty();
        }

        ImGui::Spacing();
        ImGui::Separator();

        // Reset to Amber button (above other buttons)
        if (ImGui::Button("Reset to Amber")) {
            colorScheme = 0;
            applyColorScheme();
            updateFontTextures();
            markConfigDirty();
        }
        
        ImGui::Spacing();
        ImGui::Separator();

        // Save button - greyed out when no changes
        bool saveDisabled = !configDirty;
        if (saveDisabled) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Save Config")) {
            saveConfig();  // Save current settings
            configDirty = false;
        }
        if (saveDisabled) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        
        // Reload Config button - restores original settings from file
        if (ImGui::Button("Reload Config")) {
            restoreOriginalConfig();
        }
    }
    ImGui::End();
}

//
// Apply a predefined color scheme
//
void GUIimgui::applyColorScheme()
{
    switch (colorScheme) {
        case 0: // Amber on black
            foregroundColor[0] = 1.0f; foregroundColor[1] = 1.0f; foregroundColor[2] = 0.0f;
            backgroundColor[0] = 0.0f; backgroundColor[1] = 0.0f; backgroundColor[2] = 0.0f;
            break;
        case 1: // Green on black
            foregroundColor[0] = 0.0f; foregroundColor[1] = 1.0f; foregroundColor[2] = 0.0f;
            backgroundColor[0] = 0.0f; backgroundColor[1] = 0.0f; backgroundColor[2] = 0.0f;
            break;
        case 2: // White on black
            foregroundColor[0] = 1.0f; foregroundColor[1] = 1.0f; foregroundColor[2] = 1.0f;
            backgroundColor[0] = 0.0f; backgroundColor[1] = 0.0f; backgroundColor[2] = 0.0f;
            break;
        case 3: // Black on white (reverse)
            foregroundColor[0] = 0.0f; foregroundColor[1] = 0.0f; foregroundColor[2] = 0.0f;
            backgroundColor[0] = 1.0f; backgroundColor[1] = 1.0f; backgroundColor[2] = 1.0f;
            break;
        default:
            // Custom - don't change colors
            break;
    }
}

//
// Update font textures with new colors
//
void GUIimgui::updateFontTextures()
{
    const int FONT_WIDTH = 8;
    const int FONT_HEIGHT = 20;
    const int BYTES_PER_CHAR = 20;


    // Convert float colors to byte values (no brightness boost needed - oversampling handles brightness)
    unsigned char fg_r = (unsigned char)(foregroundColor[0] * 255);
    unsigned char fg_g = (unsigned char)(foregroundColor[1] * 255);
    unsigned char fg_b = (unsigned char)(foregroundColor[2] * 255);

    // Recreate all font textures with new colors
    for (int i = 0; i < 256; i++)
    {
        // Destroy old texture if it exists
        if (fontTextures[i]) {
            SDL_DestroyTexture(fontTextures[i]);
            fontTextures[i] = nullptr;
        }

        // Create higher resolution textures with configurable oversampling
        int textureWidth = FONT_WIDTH * linearTextureScale;
        int textureHeight = FONT_HEIGHT * linearTextureScale;
        
        // Allocate RGBA texture data (4 bytes per pixel)
        unsigned char* textureData = new unsigned char[textureWidth * textureHeight * 4];
        memset(textureData, 0, textureWidth * textureHeight * 4);

        // Convert 1-bit bitmap to RGBA texture with new colors
        for (int row = 0; row < FONT_HEIGHT; row++)
        {
            int flipped_row = (FONT_HEIGHT - 1) - row;
            unsigned char fontByte = fontTable[i * BYTES_PER_CHAR + flipped_row];

            for (int col = 0; col < FONT_WIDTH; col++)
            {
                bool pixel = fontByte & (1 << (7 - col));
                
                // Fill the scaled texture block
                for (int sy = 0; sy < linearTextureScale; sy++) {
                    for (int sx = 0; sx < linearTextureScale; sx++) {
                        int textureRow = row * linearTextureScale + sy;
                        int textureCol = col * linearTextureScale + sx;
                        int pixelIndex = (textureRow * textureWidth + textureCol) * 4;

                        if (pixel)
                        {
                            // Foreground pixel
                            textureData[pixelIndex + 0] = fg_r;  // Red
                            textureData[pixelIndex + 1] = fg_g;  // Green  
                            textureData[pixelIndex + 2] = fg_b;  // Blue
                            textureData[pixelIndex + 3] = 255;   // Alpha
                        }
                        else
                        {
                            // Transparent pixel (background handled by clear color)
                            textureData[pixelIndex + 0] = 0;
                            textureData[pixelIndex + 1] = 0;
                            textureData[pixelIndex + 2] = 0;
                            textureData[pixelIndex + 3] = 0; // Transparent
                        }
                    }
                }
            }
        }

        // Create new SDL2 texture
        SDL_Surface* surface = SDL_CreateRGBSurface(0, textureWidth, textureHeight, 32, 
                                                    0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
        if (surface) {
            SDL_LockSurface(surface);
            memcpy(surface->pixels, textureData, textureWidth * textureHeight * 4);
            SDL_UnlockSurface(surface);

            fontTextures[i] = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);
        }

        delete[] textureData;
    }
}

//
// Mark config as dirty when changes are made
//
void GUIimgui::markConfigDirty()
{
    configDirty = true;
}

//
// Save original config values when config window opens
//
void GUIimgui::saveOriginalConfig()
{
    originalMaintainAspectRatio = maintainAspectRatio;
    originalColorScheme = colorScheme;
    originalLinearTextureScale = linearTextureScale;
    originalForegroundColor[0] = foregroundColor[0];
    originalForegroundColor[1] = foregroundColor[1];
    originalForegroundColor[2] = foregroundColor[2];
    originalBackgroundColor[0] = backgroundColor[0];
    originalBackgroundColor[1] = backgroundColor[1];
    originalBackgroundColor[2] = backgroundColor[2];
    originalWindowWidth = savedWindowWidth;
    originalWindowHeight = savedWindowHeight;
    configDirty = false;
}

//
// Restore original config values (cancel functionality)
//
void GUIimgui::restoreOriginalConfig()
{
    maintainAspectRatio = originalMaintainAspectRatio;
    colorScheme = originalColorScheme;
    linearTextureScale = originalLinearTextureScale;
    foregroundColor[0] = originalForegroundColor[0];
    foregroundColor[1] = originalForegroundColor[1];
    foregroundColor[2] = originalForegroundColor[2];
    backgroundColor[0] = originalBackgroundColor[0];
    backgroundColor[1] = originalBackgroundColor[1];
    backgroundColor[2] = originalBackgroundColor[2];
    savedWindowWidth = originalWindowWidth;
    savedWindowHeight = originalWindowHeight;
    
    // Apply the restored settings
    applyColorScheme();
    updateFontTextures();
    
    // Restore window size directly without triggering scale calculation
    SDL_SetWindowSize(window, savedWindowWidth, savedWindowHeight);
    
    configDirty = false;
}

//
// Apply texture filtering mode
//

//
// Set window to a specific scale multiple
//
void GUIimgui::setWindowScale(int scale)
{
    // Calculate base terminal size (without any scaling)
    int baseWidth = H19_TERMINAL_COLS * H19_CHAR_WIDTH + 2 * H19_BORDER_SIZE;   // 680 pixels
    int baseHeight = H19_TERMINAL_ROWS * H19_CHAR_HEIGHT + 2 * H19_BORDER_SIZE; // 540 pixels

    // Calculate desired window size
    int desiredWidth = baseWidth * scale;
    int desiredHeight = baseHeight * scale;

#ifdef __APPLE__
    // On macOS, detect OS constraints and apply desktop limits
    static int maxAllowedWidth = -1;
    static int maxAllowedHeight = -1;

    if (maxAllowedWidth == -1 || maxAllowedHeight == -1) {
        // First, test with 9999 to see what the OS will actually allow
        SDL_SetWindowSize(window, 9999, 9999);
        SDL_Delay(10);
        int osMaxWidth, osMaxHeight;
        SDL_GetWindowSize(window, &osMaxWidth, &osMaxHeight);

        // Then get the desktop size
        SDL_Rect displayBounds;
        int displayIndex = SDL_GetWindowDisplayIndex(window);
        if (SDL_GetDisplayBounds(displayIndex, &displayBounds) == 0) {
            // Use the smaller of OS constraint or desktop size
            maxAllowedWidth = (osMaxWidth < displayBounds.w) ? osMaxWidth : displayBounds.w;
            maxAllowedHeight = (osMaxHeight < displayBounds.h) ? osMaxHeight : displayBounds.h;
        } else {
            // Fallback to OS constraint
            maxAllowedWidth = osMaxWidth;
            maxAllowedHeight = osMaxHeight;
        }
    }

    // Use the smaller of desired size or combined maximum
    int newWidth = (desiredWidth < maxAllowedWidth) ? desiredWidth : maxAllowedWidth;
    int newHeight = (desiredHeight < maxAllowedHeight) ? desiredHeight : maxAllowedHeight;

#else
    // On Linux, use the desired size directly
    int newWidth = desiredWidth;
    int newHeight = desiredHeight;
#endif

    // Set the window size
    SDL_SetWindowSize(window, newWidth, newHeight);

#ifdef __APPLE__
    // Center the window on macOS to prevent it from extending off-screen
    SDL_Rect displayBounds;
    int displayIndex = SDL_GetWindowDisplayIndex(window);
    if (SDL_GetDisplayBounds(displayIndex, &displayBounds) == 0) {
        int centerX = displayBounds.x + (displayBounds.w - newWidth) / 2;
        int centerY = displayBounds.y + (displayBounds.h - newHeight) / 2;
        SDL_SetWindowPosition(window, centerX, centerY);
    }
#endif

    // Update cached window sizes
    cachedWindowWidth = newWidth;
    cachedWindowHeight = newHeight;
    windowSizeChanged = true;


    // Brief delay for window manager processing
    SDL_Delay(20);

    // Reset size constraints to reasonable values
    SDL_SetWindowMinimumSize(window, 340, 270);  // 0.5x minimum

    // Force complete render state refresh
    SDL_RenderSetViewport(renderer, NULL);

    // Update cached values and force terminal rescaling
    // Use logical window size since we disabled high DPI scaling
    SDL_GetWindowSize(window, &cachedWindowWidth, &cachedWindowHeight);
    windowSizeChanged = true;
    cachedCharScaleX = 0.0f;  // Force recalculation
    cachedCharScaleY = 0.0f;
    screenNeedsRedraw = true;

    // Update saved window size for persistence
    savedWindowWidth = cachedWindowWidth;
    savedWindowHeight = cachedWindowHeight;

    // Force immediate render update
    if (GUIDisplayFunc) {
        GUIDisplayFunc();
    }

    // printf("Window scaling complete\n");
}

//
// Get platform-independent configuration file path
//
std::string GUIimgui::getConfigPath()
{
    // Use SDL's built-in function to get the user's config directory
    char* configDir = SDL_GetPrefPath("VirtualH89", "config");
    if (!configDir) {
        // Fallback to current directory if SDL can't determine config path
        return "virtualh89.conf";
    }

    std::string configPath = std::string(configDir) + "virtualh89.conf";
    SDL_free(configDir);
    return configPath;
}

//
// Load configuration from disk
//
void GUIimgui::loadConfig()
{
    std::string configPath = getConfigPath();
    std::ifstream configFile(configPath);

    if (!configFile.is_open()) {
        printf("No config file found at %s, using defaults\n", configPath.c_str());
        return;
    }

    printf("Loading configuration from %s\n", configPath.c_str());

    std::string line;
    while (std::getline(configFile, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        // Parse key=value pairs
        size_t equalsPos = line.find('=');
        if (equalsPos == std::string::npos) continue;

        std::string key = line.substr(0, equalsPos);
        std::string value = line.substr(equalsPos + 1);

        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        // Parse configuration values
        if (key == "maintainAspectRatio") {
            maintainAspectRatio = (value == "true" || value == "1");
        }
        else if (key == "windowWidth") {
            savedWindowWidth = std::stoi(value);
        }
        else if (key == "windowHeight") {
            savedWindowHeight = std::stoi(value);
        }
        else if (key == "colorScheme") {
            colorScheme = std::stoi(value);
            // Don't apply color scheme here - wait until after we load custom colors
        }
        else if (key == "linearTextureScale") {
            linearTextureScale = std::stoi(value);
        }
        else if (key == "foregroundColor") {
            // Parse "r,g,b" format
            std::istringstream iss(value);
            std::string token;
            int i = 0;
            while (std::getline(iss, token, ',') && i < 3) {
                foregroundColor[i] = std::stof(token);
                i++;
            }
        }
        else if (key == "backgroundColor") {
            // Parse "r,g,b" format
            std::istringstream iss(value);
            std::string token;
            int i = 0;
            while (std::getline(iss, token, ',') && i < 3) {
                backgroundColor[i] = std::stof(token);
                i++;
            }
        }
    }

    configFile.close();

    printf("Configuration loaded successfully\n");
}

//
// Save configuration to disk
//
void GUIimgui::saveConfig()
{
    std::string configPath = getConfigPath();

    // Create config directory if it doesn't exist
    size_t lastSlash = configPath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        std::string configDir = configPath.substr(0, lastSlash);
        // Note: SDL_GetPrefPath already creates the directory, but let's be safe
    }

    std::ofstream configFile(configPath);
    if (!configFile.is_open()) {
        printf("Warning: Could not save configuration to %s\n", configPath.c_str());
        return;
    }

    printf("Saving configuration to %s\n", configPath.c_str());

    // Write configuration header
    configFile << "# VirtualH89 Configuration File\n";
    configFile << "# Generated automatically - edit with caution\n\n";

    // Write display settings
    configFile << "[Display]\n";
    configFile << "maintainAspectRatio=" << (maintainAspectRatio ? "true" : "false") << "\n";
    configFile << "windowWidth=" << savedWindowWidth << "\n";
    configFile << "windowHeight=" << savedWindowHeight << "\n";
    configFile << "\n";

    // Write rendering settings
    configFile << "[Rendering]\n";
    configFile << "linearTextureScale=" << linearTextureScale << "\n";
    configFile << "\n";

    // Write color settings
    configFile << "[Colors]\n";
    configFile << "colorScheme=" << colorScheme << "\n";
    configFile << "foregroundColor=" << foregroundColor[0] << "," << foregroundColor[1] << "," << foregroundColor[2] << "\n";
    configFile << "backgroundColor=" << backgroundColor[0] << "," << backgroundColor[1] << "," << backgroundColor[2] << "\n";
    configFile << "\n";

    configFile.close();
}

#endif /* __GUIimgui__ */
