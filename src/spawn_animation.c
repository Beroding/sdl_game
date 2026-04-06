// ============================================================================
// spawn_animation.c – Teleport spawn animation with beam of light
// ============================================================================
//
// A dramatic player spawn effect featuring:
// - Beam of light descending from above
// - Swirling energy particles
// - Player materializing from transparent to solid
// - Expanding shockwave rings
// - Screen flash on completion
// ============================================================================

#include "../include/spawn_animation.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PHASE_BEAM_DURATION 1.5f
#define PHASE_MATERIALIZE_DURATION 0.8f
#define PHASE_RING_DURATION 0.6f

static float randf(void) {
    return (float)rand() / RAND_MAX;
}

// Initialize beam particles that fall down the light column
static void init_beam_particles(SpawnAnimation *anim) {
    for (int i = 0; i < 30; i++) {
        anim->beam_particles[i][0] = (randf() - 0.5f) * 40; /* x offset */
        anim->beam_particles[i][1] = -200 - randf() * 500;  /* y start (above screen) */
        anim->beam_particles[i][2] = 200 + randf() * 300;   /* fall speed */
        anim->beam_particles[i][3] = 0.5f + randf() * 1.0f;   /* life */
    }
}

// Initialize swirling particles around spawn point
static void init_swirl_particles(SpawnAnimation *anim) {
    for (int i = 0; i < SPAWN_PARTICLE_COUNT; i++) {
        SpawnParticle *p = &anim->particles[i];
        float angle = (i / (float)SPAWN_PARTICLE_COUNT) * 2 * 3.14159f;
        float dist = 20 + randf() * 60;
        
        p->x = anim->player_x + cosf(angle) * dist;
        p->y = anim->player_y + sinf(angle) * dist;
        p->vx = cosf(angle + 1.57f) * (50 + randf() * 100); /* tangent velocity */
        p->vy = sinf(angle + 1.57f) * (50 + randf() * 100);
        p->life = 0;
        p->max_life = 0.5f + randf() * 0.5f;
        p->size = 2 + randf() * 6;
        p->r = 200 + randf() * 55;
        p->g = 220 + randf() * 35;
        p->b = 255;
        p->active = false;
    }
}

// Initialize expanding rings
static void init_rings(SpawnAnimation *anim) {
    for (int i = 0; i < SPAWN_RING_COUNT; i++) {
        anim->rings[i].radius = 0;
        anim->rings[i].max_radius = 100 + i * 80;
        anim->rings[i].width = 3 + i * 2;
        anim->rings[i].alpha = 1.0f;
        anim->rings[i].active = false;
    }
}

SpawnAnimation* spawn_animation_create(float player_x, float player_y,
                                        float cam_x, float cam_y, float zoom) {
    SpawnAnimation *anim = calloc(1, sizeof(SpawnAnimation));
    if (!anim) return NULL;
    
    anim->phase = SPAWN_PHASE_BEAM_DESCEND;
    anim->phase_timer = 0;
    anim->total_timer = 0;
    anim->player_x = player_x;
    anim->player_y = player_y;
    anim->cam_x = cam_x;
    anim->cam_y = cam_y;
    anim->zoom = zoom;
    
    anim->beam_width = 0;
    anim->beam_intensity = 0;
    anim->beam_core_glow = 0;
    anim->player_opacity = 0;
    anim->swirl_angle = 0;
    anim->flash_intensity = 0;
    anim->complete = false;
    
    srand((unsigned int)SDL_GetTicks());
    init_beam_particles(anim);
    init_swirl_particles(anim);
    init_rings(anim);
    
    printf("Spawn animation created at (%.1f, %.1f)\n", player_x, player_y);
    return anim;
}

void spawn_animation_destroy(SpawnAnimation *anim) {
    free(anim);
}

// Convert world coordinates to screen coordinates
static void world_to_screen(SpawnAnimation *anim, float wx, float wy, 
                           float *sx, float *sy) {
    *sx = (wx * anim->zoom) - anim->cam_x;
    *sy = (wy * anim->zoom) - anim->cam_y;
}

void spawn_animation_update(SpawnAnimation *anim, float delta_time) {
    if (!anim || anim->complete) return;
    
    anim->phase_timer += delta_time;
    anim->total_timer += delta_time;
    
    switch (anim->phase) {
        case SPAWN_PHASE_BEAM_DESCEND: {
            float t = anim->phase_timer / PHASE_BEAM_DURATION;
            if (t > 1.0f) t = 1.0f;
            
            // Original size
            anim->beam_width = 20 + t * 60;  // 20 to 80 - ORIGINAL
            // Higher starting opacity
            anim->beam_intensity = 0.7f + t * 0.3f;  // Start at 70% instead of 0%
            anim->beam_core_glow = 0.5f + t * 0.5f;  // Start at 50% instead of 0%
            
            // Update falling beam particles
            for (int i = 0; i < 30; i++) {
                anim->beam_particles[i][1] += anim->beam_particles[i][2] * delta_time;
                anim->beam_particles[i][3] -= delta_time;
                if (anim->beam_particles[i][3] <= 0) {
                    // Reset particle to top
                    anim->beam_particles[i][0] = (randf() - 0.5f) * anim->beam_width;
                    anim->beam_particles[i][1] = -100;
                    anim->beam_particles[i][3] = 0.5f + randf() * 1.0f;
                }
            }
            
            if (anim->phase_timer >= PHASE_BEAM_DURATION) {
                anim->phase = SPAWN_PHASE_MATERIALIZE;
                anim->phase_timer = 0;
                // Activate swirl particles
                for (int i = 0; i < SPAWN_PARTICLE_COUNT; i++) {
                    anim->particles[i].active = true;
                }
            }
            break;
        }
        
        case SPAWN_PHASE_MATERIALIZE: {
            float t = anim->phase_timer / PHASE_MATERIALIZE_DURATION;
            if (t > 1.0f) t = 1.0f;
            
            // Player fades in
            anim->player_opacity = t;
            
            // Swirl animation
            anim->swirl_angle += delta_time * 5.0f; /* rotation speed */
            
            // Update particles - spiral inward then outward
            for (int i = 0; i < SPAWN_PARTICLE_COUNT; i++) {
                SpawnParticle *p = &anim->particles[i];
                if (!p->active) continue;
                
                // Spiral motion
                float center_x = anim->player_x;
                float center_y = anim->player_y;
                float dx = p->x - center_x;
                float dy = p->y - center_y;
                float dist = sqrtf(dx*dx + dy*dy);
                float angle = atan2f(dy, dx) + delta_time * (3.0f - t * 2.0f);
                
                // Spiral in then out
                float target_dist = t < 0.5f ? dist * 0.5f : dist * 1.5f;
                p->x = center_x + cosf(angle) * target_dist;
                p->y = center_y + sinf(angle) * target_dist;
                
                p->life += delta_time;
                if (p->life >= p->max_life) {
                    p->active = false;
                }
            }
            
            // Beam fades slightly
            anim->beam_intensity = 1.0f - t * 0.3f;
            
            if (anim->phase_timer >= PHASE_MATERIALIZE_DURATION) {
                anim->phase = SPAWN_PHASE_RING_EXPAND;
                anim->phase_timer = 0;
                // Activate rings with staggered timing
                for (int i = 0; i < SPAWN_RING_COUNT; i++) {
                    anim->rings[i].active = true;
                }
                // Flash screen
                anim->flash_intensity = 1.0f;
            }
            break;
        }
        
        case SPAWN_PHASE_RING_EXPAND: {
            float t = anim->phase_timer / PHASE_RING_DURATION;
            if (t > 1.0f) t = 1.0f;
            
            // Expand rings outward
            for (int i = 0; i < SPAWN_RING_COUNT; i++) {
                SpawnRing *r = &anim->rings[i];
                if (!r->active) continue;
                
                float stagger = i * 0.1f; /* each ring starts slightly later */
                float rt = (t - stagger) / (1.0f - stagger);
                if (rt < 0) rt = 0;
                if (rt > 1) rt = 1;
                
                r->radius = r->max_radius * rt;
                r->alpha = 1.0f - rt;
            }
            
            // Fade out beam
            anim->beam_intensity = 0.7f * (1.0f - t);
            anim->player_opacity = 1.0f;
            
            // Fade flash
            anim->flash_intensity *= 0.9f;
            
            if (anim->phase_timer >= PHASE_RING_DURATION) {
                anim->phase = SPAWN_PHASE_COMPLETE;
                anim->complete = true;
            }
            break;
        }
        
        case SPAWN_PHASE_COMPLETE:
            anim->complete = true;
            break;
    }
}

void spawn_animation_render(SDL_Renderer *renderer, SpawnAnimation *anim,
                            int window_w, int window_h) {
    if (!anim || !renderer) return;
    
    // Get player screen position
    float px, py;
    world_to_screen(anim, anim->player_x, anim->player_y, &px, &py);
    
    // Screen flash overlay
    if (anim->flash_intensity > 0.01f) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 
                              (Uint8)(80 * anim->flash_intensity));
        SDL_FRect flash = {0, 0, window_w, window_h};
        SDL_RenderFillRect(renderer, &flash);
    }
    
    // Render beam of light (original size, higher opacity to cover player)
    if (anim->beam_intensity > 0.01f) {
        // MUCH higher opacity - nearly opaque to hide player
        Uint8 alpha = (Uint8)(240 * anim->beam_intensity);  // Was ~200, now 240
        
        // Get player screen position
        float px, py;
        world_to_screen(anim, anim->player_x, anim->player_y, &px, &py);
        
        // Outer glow - original size, higher opacity
        for (int i = 5; i >= 0; i--) {
            float w = anim->beam_width * (1.5f + i * 0.5f);  // Original size
            SDL_SetRenderDrawColor(renderer, 
                (Uint8)(100 + i * 20), 
                (Uint8)(180 + i * 10), 
                255, 
                (Uint8)(alpha * 0.15f));  // Slightly higher opacity per layer
            SDL_FRect glow = {
                px - w/2, 
                -100,  // Original start
                w, 
                py + 200  // Original end
            };
            SDL_RenderFillRect(renderer, &glow);
        }
        
        // Core beam - original size, nearly opaque
        SDL_SetRenderDrawColor(renderer, 220, 240, 255, alpha);
        SDL_FRect core = {
            px - anim->beam_width/2,  // Original width
            -100,
            anim->beam_width,           // Original
            py + 200
        };
        SDL_RenderFillRect(renderer, &core);
        
        // Center glow - original size, higher opacity
        float core_glow = anim->beam_core_glow * 40;  // Original 40
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 
                              (Uint8)(250 * anim->beam_intensity));  // Was 150, now 250
        SDL_FRect center_glow = {
            px - core_glow, py - core_glow/2,
            core_glow * 2, core_glow
        };
        SDL_RenderFillRect(renderer, &center_glow);
        
        // Ground glow - original size, higher opacity
        SDL_SetRenderDrawColor(renderer, 200, 220, 255, 
                              (Uint8)(220 * anim->beam_intensity));  // Was 150, now 220
        SDL_FRect ground_glow = {
            px - 60, py + 20,  // Original size
            120, 40
        };
        SDL_RenderFillRect(renderer, &ground_glow);
        
        // Beam particles - more opaque
        for (int i = 0; i < 30; i++) {
            float *p = anim->beam_particles[i];
            if (p[3] <= 0) continue;
            
            float sx = px + p[0];
            float sy = p[1];
            Uint8 pa = (Uint8)(255 * p[3] * anim->beam_intensity);  // Full particle opacity
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, pa);
            float size = 2 + p[3] * 3;
            SDL_FRect particle = {sx - size/2, sy - size/2, size, size};
            SDL_RenderFillRect(renderer, &particle);
        }
    }
    
    // Render expanding rings
    for (int i = 0; i < SPAWN_RING_COUNT; i++) {
        SpawnRing *r = &anim->rings[i];
        if (!r->active || r->alpha <= 0.01f) continue;
        
        Uint8 ra = (Uint8)(255 * r->alpha);
        SDL_SetRenderDrawColor(renderer, 200, 220, 255, ra);
        
        // Draw ring as a circle outline (approximated with lines)
        int segments = 32;
        float prev_x = px + r->radius;
        float prev_y = py;
        
        for (int j = 1; j <= segments; j++) {
            float angle = (j / (float)segments) * 2 * 3.14159f;
            float cx = px + cosf(angle) * r->radius;
            float cy = py + sinf(angle) * r->radius * 0.6f; /* oval for perspective */
            
            // Draw thick line
            for (int w = -r->width/2; w <= r->width/2; w++) {
                SDL_RenderLine(renderer, prev_x + w, prev_y, cx + w, cy);
            }
            
            prev_x = cx;
            prev_y = cy;
        }
    }
    
    // Render swirl particles
    for (int i = 0; i < SPAWN_PARTICLE_COUNT; i++) {
        SpawnParticle *p = &anim->particles[i];
        if (!p->active) continue;
        
        float sx, sy;
        world_to_screen(anim, p->x, p->y, &sx, &sy);
        
        float life_ratio = 1.0f - (p->life / p->max_life);
        Uint8 pa = (Uint8)(255 * life_ratio);
        
        SDL_SetRenderDrawColor(renderer, p->r, p->g, p->b, pa);
        
        float size = p->size * life_ratio;
        SDL_FRect rect = {sx - size/2, sy - size/2, size, size};
        SDL_RenderFillRect(renderer, &rect);
    }
}

bool spawn_animation_is_complete(SpawnAnimation *anim) {
    return anim && anim->complete;
}

void spawn_animation_skip(SpawnAnimation *anim) {
    if (anim) {
        anim->phase = SPAWN_PHASE_COMPLETE;
        anim->complete = true;
        anim->player_opacity = 1.0f;
    }
}