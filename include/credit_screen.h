#ifndef CREDIT_SCREEN_H
#define CREDIT_SCREEN_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

#define MAX_CREDIT_SCENES 5
#define CREDIT_SCENE_FADE_DURATION 0.5f
#define CREDIT_SCENE_DISPLAY_DURATION 4.0f

typedef enum {
    CREDIT_PHASE_FADE_IN,
    CREDIT_PHASE_DISPLAY,
    CREDIT_PHASE_FADE_OUT,
    CREDIT_PHASE_NEXT,
    CREDIT_PHASE_COMPLETE
} CreditPhase;

typedef struct {
    // Resources
    TTF_Font *font_title;
    TTF_Font *font_body;
    TTF_Font *font_header;
    
    // Scene content
    char *scene_texts[MAX_CREDIT_SCENES];
    int scene_count;
    int current_scene;
    
    // Animation state
    CreditPhase phase;
    float phase_timer;
    float alpha;
    
    // Text textures for current scene
    SDL_Texture **line_textures;
    int line_count;
    float *line_y_positions;
    
    // Background
    SDL_Texture *bg_texture;
    
    // Window dimensions
    int window_w;
    int window_h;
    
    bool complete;
    bool skip_requested;
    
} CreditScreen;

// Create and destroy
CreditScreen* credit_screen_create(SDL_Renderer *renderer, int window_w, int window_h);
void credit_screen_destroy(CreditScreen *credits);

// Load scene files from disk
bool credit_screen_load_scenes(CreditScreen *credits, const char *scene_files[], int count);

// Update and render
void credit_screen_update(CreditScreen *credits, float delta_time);
void credit_screen_render(SDL_Renderer *renderer, CreditScreen *credits);

// Input handling
void credit_screen_handle_key(CreditScreen *credits, SDL_KeyboardEvent *key);
void credit_screen_skip(CreditScreen *credits);

// Status
bool credit_screen_is_complete(CreditScreen *credits);

#endif