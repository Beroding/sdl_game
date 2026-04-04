#ifndef OPENING_ANIMATION_H
#define OPENING_ANIMATION_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include <SDL3_mixer/SDL_mixer.h>
#include "sfx_system.h"          // <-- ADD THIS LINE

#define MAX_SHARDS 32
#define MAX_CRACKS 16

/* Boss entrance animation phases */
typedef enum {
    OPENING_PHASE_TRIDENT_RISE,
    OPENING_PHASE_THRUST,
    OPENING_PHASE_IMPACT,
    OPENING_PHASE_SHATTER,
    OPENING_PHASE_DARKEN,
    OPENING_PHASE_COMPLETE
} OpeningPhase;

/* Screen shard with physics */
typedef struct {
    float x, y;
    float vx, vy;
    float rotation;
    float vrotation;
    float size;
    float alpha;
    float px[3], py[3];
} ScreenShard;

/* Boss entrance animation */
typedef struct {
    OpeningPhase phase;
    float phase_timer;
    float total_timer;
    
    int window_w, window_h;
    
    float trident_x, trident_y;
    float trident_scale;
    float trident_rotation;
    float trident_glow;
    bool trident_quaking;
    
    int crack_count;
    float crack_points[MAX_CRACKS][4];
    float crack_width[MAX_CRACKS];
    float crack_glow[MAX_CRACKS];
    float impact_point_x, impact_point_y;
    
    ScreenShard shards[MAX_SHARDS];
    int shard_count;
    float shatter_progress;
    
    float screen_shake;
    float red_flash;
    float darkness;
    
    float fire_particles[20][5];
    
    SDL_Texture *dark_overlay;
    SDL_Texture *trident_glow_texture;
    TTF_Font *font;
    
    bool complete;
    
    SFXSystem *sfx;                // for playing glass break sound
    bool glass_break_played;       // ensure it plays only once
} OpeningAnimation;

OpeningAnimation* opening_create(SDL_Renderer *renderer, int window_w, int window_h, SFXSystem *sfx);
void opening_destroy(OpeningAnimation *anim);
void opening_update(OpeningAnimation *anim, float delta_time);
void opening_render(SDL_Renderer *renderer, OpeningAnimation *anim);
bool opening_is_complete(OpeningAnimation *anim);

#endif