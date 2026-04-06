#ifndef SPAWN_ANIMATION_H
#define SPAWN_ANIMATION_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#define SPAWN_PARTICLE_COUNT 60
#define SPAWN_RING_COUNT 5

/* Phases of the spawn animation */
typedef enum {
    SPAWN_PHASE_BEAM_DESCEND,    /* Light beam comes down from sky */
    SPAWN_PHASE_MATERIALIZE,     /* Player fades in with particles */
    SPAWN_PHASE_RING_EXPAND,     /* Shockwave rings spread outward */
    SPAWN_PHASE_COMPLETE
} SpawnPhase;

/* A single particle in the swirl effect */
typedef struct {
    float x, y;
    float vx, vy;
    float life;
    float max_life;
    float size;
    Uint8 r, g, b;
    bool active;
} SpawnParticle;

/* Expanding ring/shockwave effect */
typedef struct {
    float radius;
    float max_radius;
    float width;
    float alpha;
    bool active;
} SpawnRing;

/* Spawn animation controller */
typedef struct {
    SpawnPhase phase;
    float phase_timer;
    float total_timer;
    
    int window_w, window_h;
    
    /* Player position (world coordinates) */
    float player_x, player_y;
    
    /* Camera for coordinate conversion */
    float cam_x, cam_y;
    float zoom;
    
    /* Beam properties */
    float beam_width;
    float beam_intensity;
    float beam_core_glow;
    float beam_particles[30][4]; /* x, y, speed, life */
    
    /* Materialization */
    float player_opacity;
    float swirl_angle;
    
    /* Effects */
    SpawnParticle particles[SPAWN_PARTICLE_COUNT];
    SpawnRing rings[SPAWN_RING_COUNT];
    
    /* Screen flash */
    float flash_intensity;
    
    bool complete;
} SpawnAnimation;

/* Create/destroy */
SpawnAnimation* spawn_animation_create(float player_x, float player_y, 
                                        float cam_x, float cam_y, float zoom);
void spawn_animation_destroy(SpawnAnimation *anim);

/* Update/render */
void spawn_animation_update(SpawnAnimation *anim, float delta_time);
void spawn_animation_render(SDL_Renderer *renderer, SpawnAnimation *anim, 
                            int window_w, int window_h);

/* Check if finished */
bool spawn_animation_is_complete(SpawnAnimation *anim);

/* Skip animation */
void spawn_animation_skip(SpawnAnimation *anim);

#endif