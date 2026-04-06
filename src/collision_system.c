#include "../include/collision_system.h"
#include <stdlib.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

bool collision_load_from_file(CollisionWorld *world, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Failed to open collision file: %s\n", filename);
        return false;
    }
    
    char line[256];
    int line_num = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        
        char type[32];
        float x, y, w, h;
        
        // Parse: wall x y w h
        if (sscanf(line, "%s %f %f %f %f", type, &x, &y, &w, &h) == 5) {
            if (strcmp(type, "wall") == 0) {
                collision_add_wall(world, x, y, w, h);
                printf("Loaded wall: %.0f,%.0f %.0fx%.0f\n", x, y, w, h);
            }
            else if (strcmp(type, "trigger") == 0) {
                // For now, triggers are loaded as walls without userdata
                // You can extend this later
                collision_add_trigger(world, x, y, w, h, NULL);
                printf("Loaded trigger: %.0f,%.0f %.0fx%.0f\n", x, y, w, h);
            }
        } else {
            printf("Warning: Invalid line %d in %s: %s", line_num, filename, line);
        }
    }
    
    fclose(file);
    printf("Loaded %d colliders from %s\n", world->count, filename);
    return true;
}

CollisionWorld* collision_world_create(float map_w, float map_h) {
    CollisionWorld *world = calloc(1, sizeof(CollisionWorld));
    world->map_width = map_w;
    world->map_height = map_h;
    world->count = 0;
    return world;
}

void collision_world_destroy(CollisionWorld *world) {
    free(world);
}

void collision_world_clear(CollisionWorld *world) {
    world->count = 0;
}

int collision_add_wall(CollisionWorld *world, float x, float y, float w, float h) {
    if (world->count >= MAX_WALLS) return -1;
    
    int id = world->count++;
    Collider *c = &world->colliders[id];
    c->x = x; c->y = y; c->w = w; c->h = h;
    c->active = true;
    c->layer = 1;  // wall layer
    c->userdata = NULL;
    
    return id;
}

int collision_add_trigger(CollisionWorld *world, float x, float y, float w, float h, void *userdata) {
    if (world->count >= MAX_WALLS) return -1;
    
    int id = world->count++;
    Collider *c = &world->colliders[id];
    c->x = x; c->y = y; c->w = w; c->h = h;
    c->active = true;
    c->layer = 3;  // trigger layer
    c->userdata = userdata;
    
    return id;
}

bool collision_check_rect(CollisionWorld *world, float x, float y, float w, float h, int layer_mask) {
    // Check map boundaries
    if (x < 0 || x + w > world->map_width || y < 0 || y + h > world->map_height) {
        return true;  // collide with map edge
    }
    
    // Check all colliders
    for (int i = 0; i < world->count; i++) {
        Collider *c = &world->colliders[i];
        if (!c->active) continue;
        if (!(layer_mask & (1 << c->layer))) continue;
        
        // AABB overlap check
        if (x < c->x + c->w && x + w > c->x &&
            y < c->y + c->h && y + h > c->y) {
            return true;
        }
    }
    return false;
}

bool collision_check_point(CollisionWorld *world, float x, float y, float radius, int layer_mask) {
    return collision_check_rect(world, x - radius, y - radius, radius * 2, radius * 2, layer_mask);
}

void collision_resolve_movement(CollisionWorld *world,
                                float *x, float *y,
                                float target_x, float target_y,
                                float radius) {
    // Try X movement first
    if (!collision_check_point(world, target_x, *y, radius, 1 << 1)) {
        *x = target_x;
    }
    // Then Y movement (allows sliding along walls)
    if (!collision_check_point(world, *x, target_y, radius, 1 << 1)) {
        *y = target_y;
    }
}

void collision_render_debug(CollisionWorld *world, SDL_Renderer *renderer,
                            float cam_x, float cam_y, float zoom) {
    for (int i = 0; i < world->count; i++) {
        Collider *c = &world->colliders[i];
        if (!c->active) continue;
        
        // Color by layer: red=wall, green=trigger, blue=water
        SDL_Color color = c->layer == 1 ? (SDL_Color){255, 0, 0, 100} :
                          c->layer == 3 ? (SDL_Color){0, 255, 0, 100} :
                                          (SDL_Color){0, 0, 255, 100};
        
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        
        SDL_FRect rect = {
            (c->x * zoom) - cam_x,
            (c->y * zoom) - cam_y,
            c->w * zoom,
            c->h * zoom
        };
        SDL_RenderFillRect(renderer, &rect);
    }
}