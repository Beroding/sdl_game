// ============================================================================
// loading_screen.c – Loading screen between menu and gameplay
// ============================================================================
//
// This file implements a visually appealing loading screen that appears
// after the menu and before the game starts. It provides feedback during
// asset loading and features a smooth fade transition.
// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "../include/loading_screen.h"

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
#define FADE_DURATION 0.5f          // Fade in/out duration in seconds
#define MIN_LOADING_TIME 1.5f       // Minimum time to show loading screen
#define PARTICLE_COUNT 50
#define TIP_ROTATION_INTERVAL 3.0f  // Seconds between tip changes

// ----------------------------------------------------------------------------
// Loading tips to entertain the player
// ----------------------------------------------------------------------------
static const char* LOADING_TIPS[] = {
    "Tip: Press E to talk to NPCs",
    "Tip: Use WASD or Arrow Keys to move",
    "Tip: Press ESC to pause the game",
    "Tip: Save your progress often",
    "Tip: Different NPCs offer different dialogues",
    "Tip: Follow the arrow trail for guidance",
    "Tip: Your choices affect the story",
    "Tip: Explore every corner of the map",
    "Tip: Music sets the mood - adjust volume in settings",
    "Did you know? The game uses SDL3 for graphics!"
};
#define TIP_COUNT (sizeof(LOADING_TIPS) / sizeof(LOADING_TIPS[0]))

// ----------------------------------------------------------------------------
// Create a gradient background texture
// ----------------------------------------------------------------------------
static SDL_Texture* create_gradient_background(SDL_Renderer *renderer, int w, int h) {
    SDL_Surface *surf = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA8888);
    if (!surf) return NULL;
    
    Uint32 *pixels = (Uint32*)surf->pixels;
    
    for (int y = 0; y < h; y++) {
        float t = (float)y / h;
        // Dark blue to deep purple gradient
        Uint8 r = (Uint8)(20 * (1.0f - t) + 40 * t);
        Uint8 g = (Uint8)(20 * (1.0f - t) + 20 * t);
        Uint8 b = (Uint8)(60 * (1.0f - t) + 80 * t);
        
        for (int x = 0; x < w; x++) {
            // Add subtle vignette effect
            float vignette = 1.0f - (float)((x - w/2)*(x - w/2) + (y - h/2)*(y - h/2)) / (w*h/3);
            if (vignette < 0.5f) vignette = 0.5f;
            
            pixels[y * w + x] = SDL_MapRGBA(SDL_GetPixelFormatDetails(surf->format), NULL,
                                            (Uint8)(r * vignette),
                                            (Uint8)(g * vignette),
                                            (Uint8)(b * vignette),
                                            255);
        }
    }
    
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_DestroySurface(surf);
    return tex;
}

// ----------------------------------------------------------------------------
// Initialize particles with random positions
// ----------------------------------------------------------------------------
static void init_particles(LoadingScreen *ls) {
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        ls->particles[i].active = true;
        ls->particles[i].x = (float)(rand() % ls->window_w);
        ls->particles[i].y = (float)(rand() % ls->window_h);
        ls->particles[i].vx = ((float)(rand() % 100) - 50) * 20.0f;
        ls->particles[i].vy = ((float)(rand() % 100) - 50) * 20.0f;
        ls->particles[i].life = (float)(rand() % 100) / 100.0f;
    }
}

// ----------------------------------------------------------------------------
// Update particles for animation
// ----------------------------------------------------------------------------
static void update_particles(LoadingScreen *ls, float delta_time) {
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        if (!ls->particles[i].active) continue;
        
        ls->particles[i].x += ls->particles[i].vx * delta_time;
        ls->particles[i].y += ls->particles[i].vy * delta_time;
        
        // Reset particles that go off screen
        if (ls->particles[i].x < -50 || ls->particles[i].x > ls->window_w + 50 ||
            ls->particles[i].y < -50 || ls->particles[i].y > ls->window_h + 50) {
            ls->particles[i].x = (float)(rand() % ls->window_w);
            ls->particles[i].y = (float)(rand() % ls->window_h);
            ls->particles[i].vx = ((float)(rand() % 100) - 50) * 20.0f;
            ls->particles[i].vy = ((float)(rand() % 100) - 50) * 20.0f;
        }
        
        // Pulsing life
        ls->particles[i].life += delta_time * 2.0f;
        if (ls->particles[i].life > 1.0f) ls->particles[i].life = 0.0f;
    }
}

// ----------------------------------------------------------------------------
// Create loading screen
// ----------------------------------------------------------------------------
LoadingScreen* loading_screen_create(SDL_Renderer *renderer, int window_width, int window_height) {
    LoadingScreen *ls = calloc(1, sizeof(LoadingScreen));
    if (!ls) return NULL;
    
    ls->window_w = window_width;
    ls->window_h = window_height;
    ls->active = false;
    ls->phase = LOADING_PHASE_FADE_IN;
    ls->phase_timer = 0.0f;
    ls->fade_alpha = 0.0f;
    ls->progress = 0.0f;
    ls->progress_target = 0.0f;
    ls->loading_speed = 0.5f;  // Fill in about 2 seconds
    
    // Create gradient background
    ls->bg_texture = create_gradient_background(renderer, window_width, window_height);
    
    // Load fonts
    ls->font_title = TTF_OpenFont("game_assets/MedievalSharp-Regular.ttf", 72);
    if (!ls->font_title) {
        printf("Loading screen title font not found, using default\n");
        ls->font_title = TTF_OpenFont("game_assets/Roboto_Medium.ttf", 72);
    }
    
    ls->font_small = TTF_OpenFont("game_assets/Roboto_Medium.ttf", 24);
    if (!ls->font_small) {
        ls->font_small = TTF_OpenFont("game_assets/Roboto_Medium.ttf", 24);
    }
    
    // Setup loading tips
    ls->tips = LOADING_TIPS;
    ls->tip_count = TIP_COUNT;
    ls->current_tip = 0;
    ls->tip_timer = 0.0f;
    ls->tip_duration = TIP_ROTATION_INTERVAL;
    
    // Initialize particle system
    srand((unsigned int)time(NULL));
    init_particles(ls);
    
    printf("Loading screen created\n");
    return ls;
}

// ----------------------------------------------------------------------------
// Destroy loading screen
// ----------------------------------------------------------------------------
void loading_screen_destroy(LoadingScreen *loading_screen) {
    if (!loading_screen) return;
    
    if (loading_screen->bg_texture) SDL_DestroyTexture(loading_screen->bg_texture);
    if (loading_screen->font_title) TTF_CloseFont(loading_screen->font_title);
    if (loading_screen->font_small) TTF_CloseFont(loading_screen->font_small);
    
    free(loading_screen);
    printf("Loading screen destroyed\n");
}

// ----------------------------------------------------------------------------
// Start loading screen sequence
// ----------------------------------------------------------------------------
void loading_screen_start(LoadingScreen *loading_screen, void (*on_complete)(void *userdata), void *userdata) {
    if (!loading_screen) return;
    
    loading_screen->active = true;
    loading_screen->phase = LOADING_PHASE_FADE_IN;
    loading_screen->phase_timer = 0.0f;
    loading_screen->fade_alpha = 0.0f;
    loading_screen->progress = 0.0f;
    loading_screen->progress_target = 0.0f;
    loading_screen->on_complete = on_complete;
    loading_screen->userdata = userdata;
    loading_screen->start_time = SDL_GetTicks();
    
    printf("Loading screen started\n");
}

// ----------------------------------------------------------------------------
// Set loading progress
// ----------------------------------------------------------------------------
void loading_screen_set_progress(LoadingScreen *loading_screen, float progress) {
    if (!loading_screen) return;
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;
    loading_screen->progress_target = progress;
}

// ----------------------------------------------------------------------------
// Update loading screen
// ----------------------------------------------------------------------------
void loading_screen_update(LoadingScreen *ls, float delta_time, SDL_Renderer *renderer) {
    if (!ls || !ls->active) return;
    
    // Smoothly interpolate progress toward target
    if (ls->progress < ls->progress_target) {
        ls->progress += delta_time * ls->loading_speed;
        if (ls->progress > ls->progress_target) ls->progress = ls->progress_target;
    }
    
    // Update phase transitions
    ls->phase_timer += delta_time;
    
    switch (ls->phase) {
        case LOADING_PHASE_FADE_IN:
            ls->fade_alpha = ls->phase_timer / FADE_DURATION;
            if (ls->fade_alpha > 1.0f) {
                ls->fade_alpha = 1.0f;
                ls->phase = LOADING_PHASE_ACTIVE;
                ls->phase_timer = 0.0f;
            }
            break;
            
        case LOADING_PHASE_ACTIVE:
            // Update tip rotation
            ls->tip_timer += delta_time;
            if (ls->tip_timer >= ls->tip_duration) {
                ls->tip_timer = 0.0f;
                ls->current_tip = (ls->current_tip + 1) % ls->tip_count;
            }
            
            // Check if loading is complete AND minimum time has passed
            if (ls->progress >= 1.0f) {
                Uint64 elapsed = SDL_GetTicks() - ls->start_time;
                float elapsed_sec = elapsed / 1000.0f;
                
                if (elapsed_sec >= MIN_LOADING_TIME) {
                    ls->phase = LOADING_PHASE_FADE_OUT;
                    ls->phase_timer = 0.0f;
                }
            }
            break;
            
        case LOADING_PHASE_FADE_OUT:
            ls->fade_alpha = 1.0f - (ls->phase_timer / FADE_DURATION);
            if (ls->fade_alpha < 0.0f) {
                ls->fade_alpha = 0.0f;
                ls->active = false;
                
                // Call completion callback
                if (ls->on_complete) {
                    ls->on_complete(ls->userdata);
                }
            }
            break;
    }
    
    // Update particles for visual flair
    update_particles(ls, delta_time);
}

// ----------------------------------------------------------------------------
// Render loading screen
// ----------------------------------------------------------------------------
void loading_screen_render(SDL_Renderer *renderer, LoadingScreen *ls) {
    if (!ls || !ls->active) return;
    
    // Draw background
    if (ls->bg_texture) {
        SDL_FRect full = {0, 0, ls->window_w, ls->window_h};
        SDL_RenderTexture(renderer, ls->bg_texture, NULL, &full);
    } else {
        // Fallback solid color
        SDL_SetRenderDrawColor(renderer, 20, 20, 40, 255);
        SDL_RenderClear(renderer);
    }
    
    // Draw particles (subtle stars)
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        if (!ls->particles[i].active) continue;
        Uint8 alpha = (Uint8)(150 * (1.0f - ls->particles[i].life));
        SDL_SetRenderDrawColor(renderer, 200, 200, 255, alpha);
        SDL_FRect particle_rect = {
            ls->particles[i].x, ls->particles[i].y,
            2, 2
        };
        SDL_RenderFillRect(renderer, &particle_rect);
    }
    
    // Apply fade overlay if fading
    if (ls->fade_alpha < 1.0f) {
        Uint8 fade_alpha_val = (Uint8)((1.0f - ls->fade_alpha) * 255);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, fade_alpha_val);
        SDL_FRect fade_rect = {0, 0, ls->window_w, ls->window_h};
        SDL_RenderFillRect(renderer, &fade_rect);
    }
    
    // "LOADING" title with pulsing effect
    if (ls->font_title) {
        float pulse = 0.8f + 0.2f * sinf(SDL_GetTicks() / 400.0f);
        SDL_Color title_color = {
            (Uint8)(255 * pulse),
            (Uint8)(215 * pulse),
            (Uint8)(100 * pulse),
            (Uint8)(255 * ls->fade_alpha)
        };
        
        SDL_Surface *title_surf = TTF_RenderText_Blended(ls->font_title, "LOADING", 0, title_color);
        if (title_surf) {
            SDL_Texture *title_tex = SDL_CreateTextureFromSurface(renderer, title_surf);
            float tw, th;
            SDL_GetTextureSize(title_tex, &tw, &th);
            
            SDL_FRect title_rect = {
                (ls->window_w - tw) / 2,
                ls->window_h / 3,
                tw, th
            };
            SDL_RenderTexture(renderer, title_tex, NULL, &title_rect);
            SDL_DestroyTexture(title_tex);
            SDL_DestroySurface(title_surf);
        }
    }
    
    // Loading bar background
    int bar_width = 600;
    int bar_height = 30;
    int bar_x = (ls->window_w - bar_width) / 2;
    int bar_y = ls->window_h / 2;
    
    SDL_FRect bar_bg = {bar_x, bar_y, bar_width, bar_height};
    SDL_SetRenderDrawColor(renderer, 40, 40, 60, 200);
    SDL_RenderFillRect(renderer, &bar_bg);
    
    SDL_SetRenderDrawColor(renderer, 100, 80, 50, 255);
    SDL_RenderRect(renderer, &bar_bg);
    
    // Loading bar fill (with gradient effect based on progress)
    int fill_width = (int)(bar_width * ls->progress);
    if (fill_width > 0) {
        SDL_FRect bar_fill = {bar_x, bar_y, fill_width, bar_height};
        
        // Animated gradient within the fill bar
        float hue_shift = (SDL_GetTicks() / 20.0f);
        Uint8 r = (Uint8)(100 + 155 * ls->progress);
        Uint8 g = (Uint8)(150 + 105 * ls->progress);
        Uint8 b = (Uint8)(200 + 55 * ls->progress);
        
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderFillRect(renderer, &bar_fill);
    }
    
    // Percentage text
    if (ls->font_small) {
        char percent_str[16];
        snprintf(percent_str, sizeof(percent_str), "%d%%", (int)(ls->progress * 100));
        SDL_Color percent_color = {200, 200, 200, (Uint8)(255 * ls->fade_alpha)};
        SDL_Surface *percent_surf = TTF_RenderText_Blended(ls->font_small, percent_str, 0, percent_color);
        if (percent_surf) {
            SDL_Texture *percent_tex = SDL_CreateTextureFromSurface(renderer, percent_surf);
            float pw, ph;
            SDL_GetTextureSize(percent_tex, &pw, &ph);
            
            SDL_FRect percent_rect = {
                bar_x + bar_width + 20,
                bar_y + (bar_height - ph) / 2,
                pw, ph
            };
            SDL_RenderTexture(renderer, percent_tex, NULL, &percent_rect);
            SDL_DestroyTexture(percent_tex);
            SDL_DestroySurface(percent_surf);
        }
    }
    
    // Loading tip
    if (ls->font_small && ls->tips && ls->tip_count > 0) {
        SDL_Color tip_color = {180, 180, 220, (Uint8)(200 * ls->fade_alpha)};
        const char *tip = ls->tips[ls->current_tip];
        
        SDL_Surface *tip_surf = TTF_RenderText_Blended_Wrapped(ls->font_small, tip, 0, tip_color, ls->window_w - 200);
        if (tip_surf) {
            SDL_Texture *tip_tex = SDL_CreateTextureFromSurface(renderer, tip_surf);
            float tw, th;
            SDL_GetTextureSize(tip_tex, &tw, &th);
            
            SDL_FRect tip_rect = {
                (ls->window_w - tw) / 2,
                bar_y + bar_height + 50,
                tw, th
            };
            SDL_RenderTexture(renderer, tip_tex, NULL, &tip_rect);
            SDL_DestroyTexture(tip_tex);
            SDL_DestroySurface(tip_surf);
        }
    }
}

// ----------------------------------------------------------------------------
// Check if loading screen is active
// ----------------------------------------------------------------------------
bool loading_screen_is_active(LoadingScreen *loading_screen) {
    return loading_screen && loading_screen->active;
}

// ----------------------------------------------------------------------------
// Force complete loading
// ----------------------------------------------------------------------------
void loading_screen_complete(LoadingScreen *loading_screen) {
    if (!loading_screen) return;
    loading_screen->progress_target = 1.0f;
    loading_screen->progress = 1.0f;
}