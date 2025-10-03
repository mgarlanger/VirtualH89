#pragma once

#include <cstdint>

// Compile-time profiling control
// Define helper macro for conditional compilation
#ifdef ENABLE_PROFILING
#define PROFILING_ENABLED 1
#else 
#define PROFILING_ENABLED 0
#endif

/**
 * Clean profiling system for GUI performance analysis
 * - Singleton pattern for global access
 * - Compiles to empty stubs when ENABLE_PROFILING not defined
 * - All profiling logic contained in this class
 */
class GUIProfiler {
public:
    static GUIProfiler& getInstance();

    // Frame profiling
    void frameStart();
    void frameStage(const char* stage);
    void frameEnd();
    void reportStats(uint32_t frameEnd = 0);

    // Automation profiling  
    void automationStart(int run);
    void automationEnd(int run, uint32_t duration);
    void checkCursorStability(uint32_t cursorX, uint32_t cursorY, uint32_t currentTime);
    void handleTimerTick(uint32_t interval);

    // Mutex profiling
    void mutexAcquired();

    // Memory profiling
    static long getMemoryUsageKB();

private:
    GUIProfiler() = default;
    ~GUIProfiler() = default;
    GUIProfiler(const GUIProfiler&) = delete;
    GUIProfiler& operator=(const GUIProfiler&) = delete;

#if PROFILING_ENABLED
    // All profiling state variables here
    uint32_t profileStartTime = 0;
    int totalFrames = 0;
    uint32_t mutexAcquisitions = 0;
    uint32_t frameStartTime = 0;
    uint32_t afterClear = 0;
    uint32_t afterTerminal = 0;
    uint32_t afterImGuiStart = 0;
    uint32_t afterImGui = 0;

    // Automation state
    uint32_t viewTimes[3] = {0};
    int profileCounter = 0;

    // Cursor stability tracking
    uint32_t lastCursorX = 0;
    uint32_t lastCursorY = 0;
    uint32_t cursorStableTime = 0;
    bool cursorInitialized = false;
    
    // Automation completion and exit
    uint32_t exitTime = 0;
    bool exitRequested = false;

    void doFrameStart();
    void doFrameStage(const char* stage);
    void doFrameEnd();
    void doReportStats(uint32_t frameEnd);
    void doAutomationStart(int run);
    void doAutomationEnd(int run, uint32_t duration);
    void doCheckCursorStability(uint32_t cursorX, uint32_t cursorY, uint32_t currentTime);
    void doHandleTimerTick(uint32_t interval);
    void doMutexAcquired();
#endif
};

// No inline implementations needed - handled in .cpp file with #if/#else

// Convenience macros for clean integration
#define PROFILE_FRAME_START()           GUIProfiler::getInstance().frameStart()
#define PROFILE_FRAME_STAGE(stage)      GUIProfiler::getInstance().frameStage(stage)
#define PROFILE_FRAME_END()             GUIProfiler::getInstance().frameEnd()
#define PROFILE_REPORT_STATS()          GUIProfiler::getInstance().reportStats(SDL_GetTicks())
#define PROFILE_MUTEX_ACQUIRED()        GUIProfiler::getInstance().mutexAcquired()
#define PROFILE_AUTOMATION_START(run)   GUIProfiler::getInstance().automationStart(run)
#define PROFILE_AUTOMATION_END(r, d)    GUIProfiler::getInstance().automationEnd(r, d)
#define PROFILE_CURSOR_STABILITY(x,y,t) GUIProfiler::getInstance().checkCursorStability(x,y,t)
