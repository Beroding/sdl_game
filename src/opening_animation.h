#ifndef OPENING_ANIMATION_H
#define OPENING_ANIMATION_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

#define MAX_SHARDS 32
#define MAX_CRACKS 16

typedef enum {
    OPENING_PHASE_TRIDENT_RISE,    // Trident rises from below with fire
    OPENING_PHASE_THRUST,          // Violent thrust toward screen
    OPENING_PHASE_IMPACT,          // Screen shatters on impact
    OPENING_PHASE_SHATTER,         // Shards fall and scatter
    OPENING_PHASE_DARKEN,          // Fade to black through cracks
    OPENING_PHASE_COMPLETE
} OpeningPhase;

typedef struct {
    // Position and physics
    float x, y;
    float vx, vy;
    float rotation;
    float vrotation;
    float size;
    float alpha;
    
    // Shard shape (triangle)
    float px[3], py[3];  // Relative points
} ScreenShard;

typedef struct {
    // Animation state
    OpeningPhase phase;
    float phase_timer;
    float total_timer;
    
    // Window dimensions
    int window_w, window_h;
    
    // Trident properties
    float trident_x, trident_y;
    float trident_scale;
    float trident_rotation;
    float trident_glow;
    bool trident_quaking;
    
    // Crack system
    int crack_count;
    float crack_points[MAX_CRACKS][4];  // x1,y1 to x2,y2
    float crack_width[MAX_CRACKS];
    float crack_glow[MAX_CRACKS];
    float impact_point_x, impact_point_y;
    
    // Shatter system
    ScreenShard shards[MAX_SHARDS];
    int shard_count;
    float shatter_progress;
    
    // Visual effects
    float screen_shake;
    float red_flash;
    float darkness;
    
    // Fire particles
    float fire_particles[20][5];  // x, y, vx, vy, life
    
    // Textures
    SDL_Texture *dark_overlay;
    SDL_Texture *trident_glow_texture;
    
    // Font
    TTF_Font *font;
    
    bool complete;
    
} OpeningAnimation;

OpeningAnimation* opening_create(SDL_Renderer *renderer, int window_w, int window_h);
void opening_destroy(OpeningAnimation *anim);
void opening_update(OpeningAnimation *anim, float delta_time);
void opening_render(SDL_Renderer *renderer, OpeningAnimation *anim);
bool opening_is_complete(OpeningAnimation *anim);

#endif