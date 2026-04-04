#ifndef LOADING_SCREEN_H
#define LOADING_SCREEN_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

// ============================================================================
// Loading Screen – Transition between menu and gameplay
// ============================================================================
//
// This screen displays a loading animation with progress indication
// while assets are being loaded and the game world is being initialized.
// It provides visual feedback to the player during the transition.
//
// Features:
// - Animated loading bar with particle effects
// - Background image or gradient
// - Subtle pulsing text effect
// - Fade in/out transitions
// ============================================================================

typedef enum {
    LOADING_PHASE_FADE_IN,      // Fading into loading screen
    LOADING_PHASE_ACTIVE,       // Loading animation playing
    LOADING_PHASE_FADE_OUT      // Fading out to game
} LoadingPhase;

typedef struct {
    bool active;                 // Is loading screen visible
    LoadingPhase phase;
    float phase_timer;           // Time in current phase
    float fade_alpha;            // Current alpha for fade effects
    
    // Animation state
    float progress;              // 0.0 to 1.0 loading progress
    float progress_target;       // Target progress (increments as assets load)
    float loading_speed;         // How fast progress fills
    
    // Visual elements
    SDL_Texture *bg_texture;     // Background (can be NULL for gradient)
    TTF_Font *font_title;        // Large font for "LOADING"
    TTF_Font *font_small;        // Small font for tips
    
    // Particle system for visual flair
    struct {
        float x, y;
        float vx, vy;
        float life;
        bool active;
    } particles[50];
    
    // Loading tips (rotating tips to entertain player)
    const char **tips;
    int tip_count;
    int current_tip;
    float tip_timer;
    float tip_duration;
    
    // Callback for when loading completes
    void (*on_complete)(void *userdata);
    void *userdata;
    
    // Window dimensions
    int window_w;
    int window_h;
    
    // Timing
    Uint64 start_time;
    
} LoadingScreen;

// ----------------------------------------------------------------------------
// Functions
// ----------------------------------------------------------------------------

/**
 * Create and initialize the loading screen.
 *
 * @param renderer        SDL renderer for creating textures
 * @param window_width    Width of the game window
 * @param window_height   Height of the game window
 * @return                Pointer to new LoadingScreen, or NULL on failure
 */
LoadingScreen* loading_screen_create(SDL_Renderer *renderer, int window_width, int window_height);

/**
 * Destroy the loading screen and free all resources.
 *
 * @param loading_screen   The loading screen to destroy
 */
void loading_screen_destroy(LoadingScreen *loading_screen);

/**
 * Start the loading screen sequence.
 *
 * @param loading_screen   Loading screen instance
 * @param on_complete      Callback when loading finishes
 * @param userdata         User data passed to callback
 */
void loading_screen_start(LoadingScreen *loading_screen, void (*on_complete)(void *userdata), void *userdata);

/**
 * Update the loading screen (called every frame).
 *
 * @param loading_screen   Loading screen instance
 * @param delta_time       Time since last frame in seconds
 * @param renderer         SDL renderer for dynamic texture creation
 */
void loading_screen_update(LoadingScreen *loading_screen, float delta_time, SDL_Renderer *renderer);

/**
 * Render the loading screen.
 *
 * @param renderer         SDL renderer
 * @param loading_screen   Loading screen instance
 */
void loading_screen_render(SDL_Renderer *renderer, LoadingScreen *loading_screen);

/**
 * Set loading progress (0.0 to 1.0).
 *
 * @param loading_screen   Loading screen instance
 * @param progress         Progress value (0.0 to 1.0)
 */
void loading_screen_set_progress(LoadingScreen *loading_screen, float progress);

/**
 * Check if loading screen is still active.
 *
 * @param loading_screen   Loading screen instance
 * @return                 True if still loading/animating
 */
bool loading_screen_is_active(LoadingScreen *loading_screen);

/**
 * Force complete the loading screen (skip remaining animation).
 *
 * @param loading_screen   Loading screen instance
 */
void loading_screen_complete(LoadingScreen *loading_screen);

#endif // LOADING_SCREEN_H