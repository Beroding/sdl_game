#ifndef OPENING_ANIMATION_H
#define OPENING_ANIMATION_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

#define MAX_SHARDS 32
#define MAX_CRACKS 16

/* Boss entrance animation phases - dramatic trident strike before battle */
typedef enum {
    OPENING_PHASE_TRIDENT_RISE,    /* Trident rises from bottom with fire */
    OPENING_PHASE_THRUST,           /* Violent thrust toward player */
    OPENING_PHASE_IMPACT,           /* Screen cracks on impact */
    OPENING_PHASE_SHATTER,          /* Glass shards fall */
    OPENING_PHASE_DARKEN,           /* Fade to black */
    OPENING_PHASE_COMPLETE          /* Animation done, start battle */
} OpeningPhase;

/* Screen shard with physics for shatter effect */
typedef struct {
    float x, y;
    float vx, vy;       /* Velocity with gravity */
    float rotation;
    float vrotation;    /* Rotation speed */
    float size;
    float alpha;        /* Fade out over time */
    float px[3], py[3]; /* Triangle vertices relative to center */
} ScreenShard;

/* Boss entrance animation - plays before battle starts */
typedef struct {
    OpeningPhase phase;
    float phase_timer;   /* Time in current phase */
    float total_timer;   /* For synced effects */
    
    int window_w, window_h;
    
    /* Hell trident - boss weapon */
    float trident_x, trident_y;
    float trident_scale;
    float trident_rotation;
    float trident_glow;      /* Pulsing intensity */
    bool trident_quaking;    /* Shake when rising */
    
    /* Screen crack effect from impact */
    int crack_count;
    float crack_points[MAX_CRACKS][4];  /* Line segments: x1,y1,x2,y2 */
    float crack_width[MAX_CRACKS];
    float crack_glow[MAX_CRACKS];       /* Pulsing crack brightness */
    float impact_point_x, impact_point_y;
    
    /* Shattered glass pieces */
    ScreenShard shards[MAX_SHARDS];
    int shard_count;
    float shatter_progress;
    
    /* Screen effects */
    float screen_shake;   /* Camera shake intensity */
    float red_flash;      /* Impact flash (0-1) */
    float darkness;       /* Fade to black (0-255) */
    
    /* Fire particles trailing the trident */
    float fire_particles[20][5];  /* x, y, vx, vy, life */
    
    SDL_Texture *dark_overlay;
    SDL_Texture *trident_glow_texture;
    TTF_Font *font;       /* For "PERISH!" text */
    
    bool complete;
    
} OpeningAnimation;

OpeningAnimation* opening_create(SDL_Renderer *renderer, int window_w, int window_h);
void opening_destroy(OpeningAnimation *anim);

void opening_update(OpeningAnimation *anim, float delta_time);
void opening_render(SDL_Renderer *renderer, OpeningAnimation *anim);

bool opening_is_complete(OpeningAnimation *anim);

#endif