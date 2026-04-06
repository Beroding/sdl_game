#ifndef COLLISION_SYSTEM_H
#define COLLISION_SYSTEM_H

#include <stdbool.h>
#include <SDL3/SDL.h>

#define MAX_WALLS 50
#define MAX_COLLISION_LAYERS 4  // ground, walls, water, triggers

typedef struct {
    float x, y, w, h;
    bool active;
    int layer;           // 0=ground(walkable), 1=wall, 2=water, etc.
    void *userdata;      // optional: for trigger events
} Collider;

typedef struct {
    Collider colliders[MAX_WALLS];
    int count;
    float map_width;
    float map_height;
} CollisionWorld;

// Core functions
CollisionWorld* collision_world_create(float map_w, float map_h);
void collision_world_destroy(CollisionWorld *world);
void collision_world_clear(CollisionWorld *world);

// Add different types of colliders
int collision_add_wall(CollisionWorld *world, float x, float y, float w, float h);
int collision_add_trigger(CollisionWorld *world, float x, float y, float w, float h, void *userdata);

// Queries
bool collision_check_point(CollisionWorld *world, float x, float y, float radius, int layer_mask);
bool collision_check_rect(CollisionWorld *world, float x, float y, float w, float h, int layer_mask);


bool collision_load_from_file(CollisionWorld *world, const char *filename);

// Movement with sliding (returns actual position after collision)
void collision_resolve_movement(CollisionWorld *world, 
                                float *x, float *y, 
                                float target_x, float target_y,
                                float radius);

// Debug
void collision_render_debug(CollisionWorld *world, SDL_Renderer *renderer, 
                            float cam_x, float cam_y, float zoom);

#endif