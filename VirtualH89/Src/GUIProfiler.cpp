#include "GUIProfiler.h"

#if PROFILING_ENABLED
#include <SDL2/SDL.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib> // For exit()
#include "h19.h"

// Define automation globals (when profiling enabled)
bool g_testTriggered = false;
int g_viewRun = 0;
int g_automationStep = 0;
uint32_t g_stepTime = 0;
uint32_t g_totalStartTime = 0;
uint32_t g_commandStartTime = 0;
#endif

GUIProfiler& GUIProfiler::getInstance() {
    static GUIProfiler instance;
    return instance;
}

long GUIProfiler::getMemoryUsageKB() {
#if PROFILING_ENABLED
    std::ifstream file("/proc/self/status");
    std::string line;
    while (std::getline(file, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            return std::stol(line.substr(7));
        }
    }
    return 0;
#else
    return 0;
#endif
}

#if PROFILING_ENABLED

void GUIProfiler::frameStart() { doFrameStart(); }
void GUIProfiler::frameStage(const char* stage) { doFrameStage(stage); }
void GUIProfiler::frameEnd() { doFrameEnd(); }
void GUIProfiler::reportStats(uint32_t frameEnd) { doReportStats(frameEnd); }
void GUIProfiler::automationStart(int run) { doAutomationStart(run); }
void GUIProfiler::automationEnd(int run, uint32_t duration) { doAutomationEnd(run, duration); }
void GUIProfiler::checkCursorStability(uint32_t cursorX, uint32_t cursorY, uint32_t currentTime) { doCheckCursorStability(cursorX, cursorY, currentTime); }
void GUIProfiler::handleTimerTick(uint32_t interval) { doHandleTimerTick(interval); }
void GUIProfiler::mutexAcquired() { doMutexAcquired(); }

void GUIProfiler::doFrameStart() {
    frameStartTime = SDL_GetTicks();
}

void GUIProfiler::doFrameStage(const char* stage) {
    uint32_t now = SDL_GetTicks();
    if (strcmp(stage, "clear") == 0) {
        afterClear = now;
    } else if (strcmp(stage, "terminal") == 0) {
        afterTerminal = now;
    } else if (strcmp(stage, "imgui_start") == 0) {
        afterImGuiStart = now;
    } else if (strcmp(stage, "imgui_end") == 0) {
        afterImGui = now;
    }
}

void GUIProfiler::doFrameEnd() {
    profileCounter++;
    totalFrames++;
    mutexAcquisitions++; // Count each GUIDisplay call
}

void GUIProfiler::doReportStats(uint32_t frameEnd) {
    if (frameEnd == 0) frameEnd = SDL_GetTicks();

    // Suppress stats output once automation is complete to prevent spam
    if (exitRequested) return;

    if (profileCounter >= 120 || (profileStartTime > 0 && (frameEnd - profileStartTime) >= 2000)) {
        uint32_t totalTime = frameEnd - (profileStartTime > 0 ? profileStartTime : frameEnd - 2000);
        long memoryKB = getMemoryUsageKB();

        // Get system info for diagnostics
        H19* h19_diag = H19::GetH19();

        // **RENDERING PERFORMANCE** - Fixed calculation
        if (totalTime > 100) {  // At least 100ms of data
            uint32_t clearTime = afterClear - frameStartTime;
            uint32_t terminalTime = afterTerminal - afterClear;  
            uint32_t imguiStartTime = afterImGuiStart - afterTerminal;
            uint32_t imguiRenderTime = afterImGui - afterImGuiStart;
            uint32_t presentTime = frameEnd - afterImGui;
            uint32_t currentFrameTime = frameEnd - frameStartTime;

            // Calculate actual frame rate over the measurement period
            float actualFPS = (float)totalFrames * 1000.0f / (float)totalTime;
            float avgFrameTime = (float)totalTime / (float)totalFrames;

            printf("RENDER STATS: %d frames in %.1fs, %.1f fps, avg %.2fms/frame (Present: %ums)\n", 
                   totalFrames, totalTime / 1000.0f, actualFPS, avgFrameTime, presentTime);

            if (h19_diag) {
                printf("RENDER STATS: Characters/frame: %d, Cursor: %s, Memory: %ld KB\n",
                       h19_diag->cols_c * 25, ((!h19_diag->cursorOff_m) && (h19_diag->curCursor_m)) ? "YES" : "NO", memoryKB);
            }

            printf("MUTEX STATS: %d h19_mutex acquisitions in %.1fs (%.1f/sec)\n", 
                   mutexAcquisitions, totalTime / 1000.0f, mutexAcquisitions * 1000.0f / totalTime);

            printf("RENDER TIMING: Clear=%ums Terminal=%ums ImGuiSetup=%ums ImGuiRender=%ums Present=%ums TOTAL=%ums\n",
                   clearTime, terminalTime, imguiStartTime, imguiRenderTime, presentTime, currentFrameTime);
        }

        // **SYSTEM PERFORMANCE DIAGNOSTICS**
        printf("SYSTEM STATS: SDL2 Renderer backend, Memory: %ld KB\n", memoryKB);

        // Reset counters for next measurement period
        profileCounter = 0;
        totalFrames = 0;
        mutexAcquisitions = 0;
        profileStartTime = frameEnd;
    }
}

void GUIProfiler::doAutomationStart(int run) {
    printf("PROFILE: View run #%d STARTED at %u ms\n", run + 1, SDL_GetTicks());
}

void GUIProfiler::doAutomationEnd(int run, uint32_t duration) {
    printf("PROFILE: View run #%d COMPLETED - Duration: %u ms (%.2f seconds)\n", 
           run + 1, duration, duration / 1000.0f);

    viewTimes[run] = duration;
}

void GUIProfiler::doCheckCursorStability(uint32_t cursorX, uint32_t cursorY, uint32_t currentTime) {
    // Initialize cursor tracking on first call
    if (!cursorInitialized) {
        lastCursorX = cursorX;
        lastCursorY = cursorY;
        cursorStableTime = currentTime;
        cursorInitialized = true;
        return;
    }

    // Track cursor movement
    if (cursorX != lastCursorX || cursorY != lastCursorY) {
        cursorStableTime = currentTime;  // Reset stability timer
        lastCursorX = cursorX;
        lastCursorY = cursorY;
    }

    // Detect command completion (cursor stable for 1000ms during automation)
    if (g_automationStep == 2 && g_commandStartTime > 0 && (currentTime - cursorStableTime) > 1000) {
        uint32_t duration = cursorStableTime - g_commandStartTime;
        viewTimes[g_viewRun] = duration;
        printf("PROFILE: View run #%d COMPLETED - Cursor stable for 1000ms at (%u,%u)\n", 
               g_viewRun + 1, cursorX, cursorY);
        printf("PROFILE: Duration: %u ms (%.2f seconds) [Start: %u, StableAt: %u]\n", 
               duration, duration / 1000.0f, g_commandStartTime, cursorStableTime);

        // Advance automation
        g_automationStep = 5;  // Go to next run setup
        g_stepTime = currentTime;

        // Check if all 3 runs completed
        if (g_viewRun >= 2) {
            printf("\n📊 **FINAL PERFORMANCE SUMMARY** 📊\n");
            printf("Run #1: %u ms (%.2f seconds)\n", viewTimes[0], viewTimes[0] / 1000.0f);
            printf("Run #2: %u ms (%.2f seconds)\n", viewTimes[1], viewTimes[1] / 1000.0f);
            printf("Run #3: %u ms (%.2f seconds)\n", viewTimes[2], viewTimes[2] / 1000.0f);

            float avgTime = (viewTimes[0] + viewTimes[1] + viewTimes[2]) / 3.0f;
            printf("Average: %.2f ms (%.3f seconds)\n", avgTime, avgTime / 1000.0f);
            printf("Total test duration: %u ms (%.2f seconds)\n", 
                   currentTime - g_totalStartTime, (currentTime - g_totalStartTime) / 1000.0f);

            printf("PROGRAMMATIC: All 3 View runs completed successfully!\n");
            // Note: Can't call running = false here since we don't have access to the main class
        }
    }

    // Start automation
    if (!g_testTriggered && currentTime > 3000) {
        g_testTriggered = true;
        g_totalStartTime = currentTime;
        printf("PROGRAMMATIC: Starting 3-run View command test...\n");
    }

    // Status debug
    static uint32_t lastStatusTime = 0;
    if (g_testTriggered && (currentTime - lastStatusTime) > 5000) {
        uint32_t stableFor = (g_commandStartTime > 0) ? (currentTime - cursorStableTime) : 0;
        printf("STATUS DEBUG: Time=%ums, Run=%d/3, Step=%d, Cursor=(%u,%u), StableFor=%ums, CmdStart=%u\n",
               currentTime, g_viewRun + 1, g_automationStep, cursorX, cursorY, stableFor, g_commandStartTime);
        lastStatusTime = currentTime;
    }
}

void GUIProfiler::doHandleTimerTick(uint32_t interval) {
    // Check for delayed exit after automation completion
    if (exitRequested && SDL_GetTicks() >= exitTime) {
        printf("PROGRAMMATIC: Profiling automation complete. Exiting now.\n");
        exit(0);
    }
    
    // Handle automation timing (when profiling enabled)
    if (g_testTriggered && g_viewRun < 3) {
        uint32_t currentTime = SDL_GetTicks();
        uint32_t stepAge = (g_stepTime > 0) ? (currentTime - g_stepTime) : 0;

        switch (g_automationStep) {
            case 0: // Send 'V'
                if (g_stepTime == 0) g_stepTime = currentTime;
                printf("PROGRAMMATIC: Run #%d - Sending 'V' via SDL event...\n", g_viewRun + 1);
                printf("TIMER DEBUG: Step 0→1, stepTime=%u\n", currentTime);

                // Send V via SDL event
                SDL_Event vEvent;
                vEvent.type = SDL_KEYDOWN;
                vEvent.key.keysym.sym = SDLK_v;
                vEvent.key.keysym.scancode = SDL_SCANCODE_V;
                vEvent.key.keysym.mod = KMOD_NONE;
                vEvent.key.state = SDL_PRESSED;
                vEvent.key.repeat = 0;
                SDL_PushEvent(&vEvent);

                g_automationStep = 1;
                g_stepTime = currentTime;
                break;

            case 1: // Wait 100ms, then send Enter
                if (stepAge > 100) {
                    printf("PROGRAMMATIC: Run #%d - Sending Enter via SDL event...\n", g_viewRun + 1);
                    printf("TIMER DEBUG: Step 1→2, stepAge=%ums\n", stepAge);

                    // Send Enter via SDL event
                    SDL_Event enterEvent;
                    enterEvent.type = SDL_KEYDOWN;
                    enterEvent.key.keysym.sym = SDLK_RETURN;
                    enterEvent.key.keysym.scancode = SDL_SCANCODE_RETURN;
                    enterEvent.key.keysym.mod = KMOD_NONE;
                    enterEvent.key.state = SDL_PRESSED;
                    enterEvent.key.repeat = 0;
                    SDL_PushEvent(&enterEvent);

                    g_automationStep = 2;
                    g_stepTime = currentTime;
                    g_commandStartTime = currentTime;
                    printf("PROFILE: View run #%d STARTED at %u ms\n", g_viewRun + 1, currentTime);
                }
                break;

            case 2: // Wait for cursor stability detection in display function
                break;

            case 5: // Advance to next run
                printf("PROGRAMMATIC: Run #%d completed, advancing to next run...\n", g_viewRun + 1);
                printf("TIMER DEBUG: Step 5→0, advancing viewRun %d→%d\n", g_viewRun, g_viewRun + 1);
                g_viewRun++;
                g_automationStep = 0;
                g_stepTime = 0;
                if (g_viewRun < 3) {
                    printf("PROGRAMMATIC: Starting run #%d...\n", g_viewRun + 1);
                } else {
                    printf("PROGRAMMATIC: All automation complete - exiting in 2 seconds...\n");
                    exitRequested = true;
                    exitTime = SDL_GetTicks() + 2000; // Exit after 2 seconds
                }
                break;
        }
    }
}

void GUIProfiler::doMutexAcquired() {
    // Already handled in doFrameEnd()
}

#else

// When profiling is disabled, provide empty function implementations  
void GUIProfiler::frameStart() {}
void GUIProfiler::frameStage(const char* stage) {}
void GUIProfiler::frameEnd() {}
void GUIProfiler::reportStats(uint32_t frameEnd) {}
void GUIProfiler::automationStart(int run) {}
void GUIProfiler::automationEnd(int run, uint32_t duration) {}
void GUIProfiler::checkCursorStability(uint32_t cursorX, uint32_t cursorY, uint32_t currentTime) {}
void GUIProfiler::handleTimerTick(uint32_t interval) {}
void GUIProfiler::mutexAcquired() {}

#endif
