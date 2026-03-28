// ============================================================================
// opening_animation.c – Boss entrance cinematic (trident strike)
// ============================================================================
//
// This file implements a dramatic pre‑battle animation: a hellish trident
// rises from below, lunges at the player, shatters the screen, and fades to
// black. It uses particle effects, screen shake, cracks, and shards.
//
// Assumptions:
// - The window dimensions are fixed and known.
// - Font "Roboto_Medium.ttf" is available in game_assets.
// - The animation is played once before a boss battle.
// - Randomness is used for cracks, shards, and fire particles.
// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "opening_animation.h"

// ----------------------------------------------------------------------------
// Animation timing constants (seconds per phase)
// ----------------------------------------------------------------------------
#define PHASE_TRIDENT_RISE_DURATION 1.2f
#define PHASE_THRUST_DURATION 0.4f
#define PHASE_IMPACT_DURATION 0.3f
#define PHASE_SHATTER_DURATION 1.5f
#define PHASE_DARKEN_DURATION 0.8f

// Trident dimensions (in world units, before scaling/rotation)
#define TRIDENT_HEAD_W 120
#define TRIDENT_HEAD_H 200
#define TRIDENT_HANDLE_W 30
#define TRIDENT_HANDLE_H 400

// ----------------------------------------------------------------------------
// Helper: random float between 0 and 1
// ----------------------------------------------------------------------------
static float randf(void) {
    return (float)rand() / RAND_MAX;
}

// ----------------------------------------------------------------------------
// Rotate a point (px, py) around (x, y) by angle with given cos/sin.
// Used for trident drawing and shards.
// ----------------------------------------------------------------------------
static void rotate_point(float px, float py, float x, float y, float cos_r, float sin_r, float *rx, float *ry) {
    float tx = px;
    float ty = py;
    *rx = x + (tx * cos_r - ty * sin_r);
    *ry = y + (tx * sin_r + ty * cos_r);
}

// ----------------------------------------------------------------------------
// Generate radial cracks emanating from impact point.
// Cracks are stored as line segments (x1,y1,x2,y2) with varying width and glow.
// ----------------------------------------------------------------------------
static void generate_radial_cracks(OpeningAnimation *anim, float cx, float cy) {
    srand((unsigned int)SDL_GetTicks());   // Seed based on time for variety
    anim->impact_point_x = cx;
    anim->impact_point_y = cy;
    
    for (int i = 0; i < MAX_CRACKS; i++) {
        // Evenly spaced angles plus a little randomness for organic look
        float angle = (i / (float)MAX_CRACKS) * 2 * 3.14159f + (randf() * 0.3f - 0.15f);
        float length = 100 + randf() * 300;   // Crack length varies
        
        anim->crack_points[i][0] = cx;
        anim->crack_points[i][1] = cy;
        anim->crack_points[i][2] = cx + cosf(angle) * length;
        anim->crack_points[i][3] = cy + sinf(angle) * length;
        anim->crack_width[i] = 2 + randf() * 4;      // Thickness
        anim->crack_glow[i] = 0;                    // Will glow during impact
    }
}

// ----------------------------------------------------------------------------
// Initialize shards (shattered glass pieces) after impact.
// Each shard is a triangle with random shape, position, and explosive velocity.
// ----------------------------------------------------------------------------
static void init_shards(OpeningAnimation *anim) {
    anim->shard_count = MAX_SHARDS;
    float cx = anim->impact_point_x;
    float cy = anim->impact_point_y;
    
    for (int i = 0; i < MAX_SHARDS; i++) {
        ScreenShard *s = &anim->shards[i];
        
        // Start shards scattered around the impact point
        float angle = (i / (float)MAX_SHARDS) * 2 * 3.14159f + randf() * 0.5f;
        float dist = 50 + randf() * 100;
        s->x = cx + cosf(angle) * dist;
        s->y = cy + sinf(angle) * dist;
        
        // Outward explosion velocity with upward bias (flying upward)
        float speed = 300 + randf() * 400;
        s->vx = cosf(angle) * speed;
        s->vy = sinf(angle) * speed - 100;
        
        s->rotation = randf() * 360;
        s->vrotation = (randf() - 0.5f) * 720;   // Random rotation speed
        s->size = 0.5f + randf() * 1.0f;
        s->alpha = 1.0f;
        
        // Generate three random points for a triangle shape
        for (int j = 0; j < 3; j++) {
            s->px[j] = (randf() - 0.5f) * 80 * s->size;
            s->py[j] = (randf() - 0.5f) * 80 * s->size;
        }
    }
}

// ----------------------------------------------------------------------------
// Initialize fire particles trailing the trident (static positions, will respawn)
// ----------------------------------------------------------------------------
static void init_fire_particles(OpeningAnimation *anim) {
    for (int i = 0; i < 20; i++) {
        float *p = anim->fire_particles[i];
        p[0] = anim->trident_x + (randf() - 0.5f) * 60;
        p[1] = anim->trident_y - 100 + randf() * 50;
        p[2] = (randf() - 0.5f) * 100;      // Horizontal drift
        p[3] = -150 - randf() * 200;        // Upward velocity
        p[4] = 0.5f + randf() * 0.5f;       // Lifetime
    }
}

// ----------------------------------------------------------------------------
// Create the animation state and all required textures.
// ----------------------------------------------------------------------------
OpeningAnimation* opening_create(SDL_Renderer *renderer, int window_w, int window_h) {
    OpeningAnimation *anim = calloc(1, sizeof(OpeningAnimation));
    if (!anim) return NULL;
    
    anim->window_w = window_w;
    anim->window_h = window_h;
    anim->phase = OPENING_PHASE_TRIDENT_RISE;
    anim->phase_timer = 0;
    anim->total_timer = 0;
    
    // Trident starts below screen, slightly right (enemy side)
    anim->trident_x = window_w * 0.75f;
    anim->trident_y = window_h + 300;
    anim->trident_scale = 1.5f;
    anim->trident_rotation = -15;            // Tilted for drama
    anim->trident_glow = 0;
    anim->trident_quaking = false;
    
    anim->screen_shake = 0;
    anim->red_flash = 0;
    anim->darkness = 0;
    anim->shatter_progress = 0;
    
    // Create a solid black texture for the final fade‑out
    SDL_Surface *surf = SDL_CreateSurface(window_w, window_h, SDL_PIXELFORMAT_RGBA8888);
    if (surf) {
        SDL_FillSurfaceRect(surf, NULL, SDL_MapRGBA(SDL_GetPixelFormatDetails(surf->format), NULL, 0, 0, 0, 255));
        anim->dark_overlay = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_SetTextureBlendMode(anim->dark_overlay, SDL_BLENDMODE_BLEND);
        SDL_DestroySurface(surf);
    }
    
    // Create a radial glow texture for the trident (soft additive glow)
    SDL_Surface *glow_surf = SDL_CreateSurface(256, 256, SDL_PIXELFORMAT_RGBA8888);
    if (glow_surf) {
        Uint32 *pixels = (Uint32*)glow_surf->pixels;
        int cx = 128, cy = 128;
        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 256; x++) {
                float dist = sqrtf((x-cx)*(x-cx) + (y-cy)*(y-cy));
                float intensity = fmaxf(0, 1.0f - dist/128.0f);
                Uint8 r = (Uint8)(255 * intensity);
                Uint8 g = (Uint8)(100 * intensity * intensity);   // Red‑orange bias
                Uint8 b = 0;
                Uint8 a = (Uint8)(200 * intensity);
                pixels[y*256 + x] = SDL_MapRGBA(SDL_GetPixelFormatDetails(glow_surf->format), NULL, r, g, b, a);
            }
        }
        anim->trident_glow_texture = SDL_CreateTextureFromSurface(renderer, glow_surf);
        SDL_SetTextureBlendMode(anim->trident_glow_texture, SDL_BLENDMODE_ADD);
        SDL_DestroySurface(glow_surf);
    }
    
    anim->font = TTF_OpenFont("game_assets/Roboto_Medium.ttf", 72);
    
    init_fire_particles(anim);
    
    printf("Hell trident opening created\n");
    return anim;
}

// ----------------------------------------------------------------------------
// Free all resources.
// ----------------------------------------------------------------------------
void opening_destroy(OpeningAnimation *anim) {
    if (!anim) return;
    if (anim->dark_overlay) SDL_DestroyTexture(anim->dark_overlay);
    if (anim->trident_glow_texture) SDL_DestroyTexture(anim->trident_glow_texture);
    if (anim->font) TTF_CloseFont(anim->font);
    free(anim);
}

// ----------------------------------------------------------------------------
// Update animation state based on delta time.
// ----------------------------------------------------------------------------
void opening_update(OpeningAnimation *anim, float delta_time) {
    if (!anim || anim->complete) return;
    
    anim->phase_timer += delta_time;
    anim->total_timer += delta_time;
    
    // Screen shake decays exponentially
    if (anim->screen_shake > 0) {
        anim->screen_shake *= 0.85f;
        if (anim->screen_shake < 0.5f) anim->screen_shake = 0;
    }
    
    // Red flash fades quickly
    if (anim->red_flash > 0) {
        anim->red_flash *= 0.9f;
    }
    
    // Update fire particles (they move upward and respawn during rise phase)
    for (int i = 0; i < 20; i++) {
        float *p = anim->fire_particles[i];
        if (p[4] > 0) {
            p[0] += p[2] * delta_time;
            p[1] += p[3] * delta_time;
            p[4] -= delta_time * 0.5f;
        } else {
            // Respawn only during the rise phase (continuous fire trail)
            if (anim->phase == OPENING_PHASE_TRIDENT_RISE) {
                p[0] = anim->trident_x + (randf() - 0.5f) * 80;
                p[1] = anim->trident_y - 150;
                p[2] = (randf() - 0.5f) * 150;
                p[3] = -200 - randf() * 300;
                p[4] = 0.8f + randf() * 0.4f;
            }
        }
    }
    
    // Phase state machine
    switch (anim->phase) {
        case OPENING_PHASE_TRIDENT_RISE: {
            float t = anim->phase_timer / PHASE_TRIDENT_RISE_DURATION;
            if (t > 1) t = 1;
            
            // Ease‑out‑back curve for dramatic rise (overshoots slightly)
            float ease = 1 + 2.70158f * powf(t - 1, 3) + 1.70158f * powf(t - 1, 2);
            
            float start_y = anim->window_h + 300;
            float end_y = anim->window_h * 0.6f;
            anim->trident_y = start_y + (end_y - start_y) * ease;
            
            // Glow grows as it approaches
            anim->trident_glow = t;
            
            // Quake when nearly up (tension)
            if (t > 0.7f) {
                anim->trident_quaking = true;
                anim->screen_shake = 5.0f * (t - 0.7f) / 0.3f;
            }
            
            if (anim->phase_timer >= PHASE_TRIDENT_RISE_DURATION) {
                anim->phase = OPENING_PHASE_THRUST;
                anim->phase_timer = 0;
                anim->trident_quaking = false;
            }
            break;
        }
            
        case OPENING_PHASE_THRUST: {
            float t = anim->phase_timer / PHASE_THRUST_DURATION;
            if (t > 1) t = 1;
            
            // Violent lunge toward player (left side of screen)
            float start_x = anim->window_w * 0.75f;
            float end_x = anim->window_w * 0.25f;
            float start_y = anim->window_h * 0.6f;
            float end_y = anim->window_h * 0.5f;
            
            // Cubic ease‑in for acceleration (fast start, slower end)
            float ease = t * t * t;
            
            anim->trident_x = start_x + (end_x - start_x) * ease;
            anim->trident_y = start_y + (end_y - start_y) * ease;
            anim->trident_rotation = -15 + (-45 * ease);   // Rotate to point at player
            
            // Intense screen shake during thrust
            anim->screen_shake = 15.0f * sinf(t * 3.14159f);
            
            if (anim->phase_timer >= PHASE_THRUST_DURATION) {
                anim->phase = OPENING_PHASE_IMPACT;
                anim->phase_timer = 0;
                anim->screen_shake = 30.0f;          // Massive impact shake
                
                // Cracks appear at the impact point (where player would be)
                generate_radial_cracks(anim, anim->window_w * 0.3f, anim->window_h * 0.5f);
                anim->red_flash = 1.0f;
            }
            break;
        }
            
        case OPENING_PHASE_IMPACT: {
            float t = anim->phase_timer / PHASE_IMPACT_DURATION;
            if (t > 1) t = 1;
            
            // Hold trident at impact point
            anim->trident_x = anim->window_w * 0.25f;
            anim->trident_y = anim->window_h * 0.5f;
            
            // Cracks glow and pulse
            for (int i = 0; i < MAX_CRACKS; i++) {
                anim->crack_glow[i] = t * (1.0f + sinf(anim->total_timer * 20 + i) * 0.3f);
            }
            
            anim->screen_shake = 20.0f * (1 - t);   // Shake fades
            
            if (anim->phase_timer >= PHASE_IMPACT_DURATION) {
                anim->phase = OPENING_PHASE_SHATTER;
                anim->phase_timer = 0;
                init_shards(anim);   // Shatter into pieces
            }
            break;
        }
            
        case OPENING_PHASE_SHATTER: {
            float t = anim->phase_timer / PHASE_SHATTER_DURATION;
            if (t > 1) t = 1;
            
            anim->shatter_progress = t;
            
            // Update each shard: gravity, movement, rotation, fade
            for (int i = 0; i < anim->shard_count; i++) {
                ScreenShard *s = &anim->shards[i];
                
                s->vy += 800 * delta_time;                // Gravity
                s->x += s->vx * delta_time;
                s->y += s->vy * delta_time;
                s->rotation += s->vrotation * delta_time;
                
                // Fade out after half the shatter duration
                if (t > 0.5f) {
                    s->alpha = 1.0f - (t - 0.5f) * 2;
                }
                
                // Remove if it falls off screen
                if (s->y > anim->window_h + 100) s->alpha = 0;
            }
            
            // Trident recedes slightly as shards fall
            anim->trident_x += 100 * delta_time;
            
            if (anim->phase_timer >= PHASE_SHATTER_DURATION) {
                anim->phase = OPENING_PHASE_DARKEN;
                anim->phase_timer = 0;
            }
            break;
        }
            
        case OPENING_PHASE_DARKEN: {
            float t = anim->phase_timer / PHASE_DARKEN_DURATION;
            if (t > 1) t = 1;
            
            // Fade to black
            anim->darkness = t * 255;
            
            // Shards continue falling in the darkness
            for (int i = 0; i < anim->shard_count; i++) {
                ScreenShard *s = &anim->shards[i];
                s->vy += 800 * delta_time;
                s->x += s->vx * delta_time;
                s->y += s->vy * delta_time;
                s->rotation += s->vrotation * delta_time;
                s->alpha *= 0.95f;   // Fast fade
            }
            
            if (anim->phase_timer >= PHASE_DARKEN_DURATION) {
                anim->phase = OPENING_PHASE_COMPLETE;
                anim->complete = true;
            }
            break;
        }
            
        case OPENING_PHASE_COMPLETE:
            anim->complete = true;
            break;
    }
}

// ----------------------------------------------------------------------------
// Draw the trident with glow, shaft, and prongs.
// ----------------------------------------------------------------------------
static void draw_trident(SDL_Renderer *renderer, OpeningAnimation *anim, float shake_x, float shake_y) {
    float x = anim->trident_x + shake_x;
    float y = anim->trident_y + shake_y;
    float scale = anim->trident_scale;
    float rot = anim->trident_rotation * 3.14159f / 180.0f;
    
    float cos_r = cosf(rot);
    float sin_r = sinf(rot);
    
    // Hellish colours
    SDL_Color shaft_color = {180, 40, 20, 255};
    SDL_Color head_color = {220, 60, 30, 255};
    SDL_Color prong_tip_color = {255, 150, 50, 255};
    
    // Glow behind the trident
    if (anim->trident_glow_texture && anim->trident_glow > 0) {
        SDL_SetTextureAlphaMod(anim->trident_glow_texture, (Uint8)(200 * anim->trident_glow));
        float glow_size = 300 * scale * (0.8f + sinf(anim->total_timer * 10) * 0.2f);   // Pulsing
        SDL_FRect glow_rect = {
            x - glow_size/2, y - glow_size/2,
            glow_size, glow_size
        };
        SDL_RenderTexture(renderer, anim->trident_glow_texture, NULL, &glow_rect);
    }
    
    // Shaft (handle) – draw as a rotated rectangle using lines
    float sx[4], sy[4];
    rotate_point(-TRIDENT_HANDLE_W/2 * scale, 0, x, y, cos_r, sin_r, &sx[0], &sy[0]);
    rotate_point( TRIDENT_HANDLE_W/2 * scale, 0, x, y, cos_r, sin_r, &sx[1], &sy[1]);
    rotate_point( TRIDENT_HANDLE_W/2 * scale,  TRIDENT_HANDLE_H * scale, x, y, cos_r, sin_r, &sx[2], &sy[2]);
    rotate_point(-TRIDENT_HANDLE_W/2 * scale,  TRIDENT_HANDLE_H * scale, x, y, cos_r, sin_r, &sx[3], &sy[3]);
    
    SDL_SetRenderDrawColor(renderer, shaft_color.r, shaft_color.g, shaft_color.b, 255);
    // Draw four sides
    for (int i = 0; i < 4; i++) {
        SDL_RenderLine(renderer, sx[i], sy[i], sx[(i+1)%4], sy[(i+1)%4]);
    }
    // Fill by drawing diagonals (simple but works)
    for (int i = 0; i < 4; i++) {
        SDL_RenderLine(renderer, sx[i], sy[i], sx[(i+2)%4], sy[(i+2)%4]);
    }
    
    // Trident head: three prongs
    // Center prong (longest)
    float cx1, cy1, cx2, cy2;
    rotate_point(0, -TRIDENT_HEAD_H * scale, x, y, cos_r, sin_r, &cx1, &cy1);
    rotate_point(0, 0, x, y, cos_r, sin_r, &cx2, &cy2);
    
    SDL_SetRenderDrawColor(renderer, head_color.r, head_color.g, head_color.b, 255);
    SDL_RenderLine(renderer, cx1, cy1, cx2, cy2);
    
    // Left prong
    float sp1x, sp1y, sp2x, sp2y;
    rotate_point(-TRIDENT_HEAD_W/2 * scale, -TRIDENT_HEAD_H * 0.7f * scale, x, y, cos_r, sin_r, &sp1x, &sp1y);
    rotate_point(-TRIDENT_HEAD_W/3 * scale, 0, x, y, cos_r, sin_r, &sp2x, &sp2y);
    SDL_RenderLine(renderer, sp1x, sp1y, sp2x, sp2y);
    
    // Right prong
    rotate_point( TRIDENT_HEAD_W/2 * scale, -TRIDENT_HEAD_H * 0.7f * scale, x, y, cos_r, sin_r, &sp1x, &sp1y);
    rotate_point( TRIDENT_HEAD_W/3 * scale, 0, x, y, cos_r, sin_r, &sp2x, &sp2y);
    SDL_RenderLine(renderer, sp1x, sp1y, sp2x, sp2y);
    
    // Crossbar connecting prongs
    float bx1, by1, bx2, by2;
    rotate_point(-TRIDENT_HEAD_W/2 * scale, -TRIDENT_HEAD_H * 0.3f * scale, x, y, cos_r, sin_r, &bx1, &by1);
    rotate_point( TRIDENT_HEAD_W/2 * scale, -TRIDENT_HEAD_H * 0.3f * scale, x, y, cos_r, sin_r, &bx2, &by2);
    SDL_RenderLine(renderer, bx1, by1, bx2, by2);
    
    // Glowing tips
    SDL_SetRenderDrawColor(renderer, prong_tip_color.r, prong_tip_color.g, prong_tip_color.b, 255);
    float tip_size = 8 * scale;
    for (int i = -1; i <= 1; i++) {
        float tx, ty;
        float offset = i * TRIDENT_HEAD_W/3 * scale;
        if (i == 0) offset = 0;
        rotate_point(offset, (-TRIDENT_HEAD_H - tip_size) * scale, x, y, cos_r, sin_r, &tx, &ty);
        SDL_FRect tip = {tx - tip_size/2, ty - tip_size/2, tip_size, tip_size};
        SDL_RenderFillRect(renderer, &tip);
    }
}

// ----------------------------------------------------------------------------
// Draw fire particles as glowing rectangles.
// ----------------------------------------------------------------------------
static void draw_fire_particles(SDL_Renderer *renderer, OpeningAnimation *anim) {
    for (int i = 0; i < 20; i++) {
        float *p = anim->fire_particles[i];
        if (p[4] <= 0) continue;
        
        float size = 15 * p[4];
        Uint8 r = 255;
        Uint8 g = (Uint8)(100 + 155 * p[4]);   // Brightens with life
        Uint8 b = 0;
        Uint8 a = (Uint8)(255 * p[4]);
        
        SDL_SetRenderDrawColor(renderer, r, g, b, a);
        SDL_FRect fire = {p[0] - size/2, p[1] - size/2, size, size};
        SDL_RenderFillRect(renderer, &fire);
        
        // Inner core
        SDL_SetRenderDrawColor(renderer, 255, 255, 200, a);
        SDL_FRect core = {p[0] - size/4, p[1] - size/4, size/2, size/2};
        SDL_RenderFillRect(renderer, &core);
    }
}

// ----------------------------------------------------------------------------
// Draw cracks with glowing, pulsing lines.
// ----------------------------------------------------------------------------
static void draw_cracks(SDL_Renderer *renderer, OpeningAnimation *anim, float shake_x, float shake_y) {
    if (anim->phase < OPENING_PHASE_IMPACT) return;
    
    for (int i = 0; i < MAX_CRACKS; i++) {
        float glow = anim->crack_glow[i];
        if (glow <= 0) continue;
        
        Uint8 r = 255;
        Uint8 g = (Uint8)(100 * (1 - glow));   // More red when glowing
        Uint8 b = 0;
        Uint8 a = (Uint8)(255 * fminf(1, glow));
        
        SDL_SetRenderDrawColor(renderer, r, g, b, a);
        
        float x1 = anim->crack_points[i][0] + shake_x;
        float y1 = anim->crack_points[i][1] + shake_y;
        float x2 = anim->crack_points[i][2] + shake_x;
        float y2 = anim->crack_points[i][3] + shake_y;
        
        // Thick glowing line (draw multiple offsets)
        for (int w = -2; w <= 2; w++) {
            SDL_RenderLine(renderer, x1 + w, y1, x2 + w, y2);
            SDL_RenderLine(renderer, x1, y1 + w, x2, y2 + w);
        }
        
        // Branch cracks when glow is strong (secondary cracks)
        if (glow > 0.5f) {
            float mid_x = (x1 + x2) / 2;
            float mid_y = (y1 + y2) / 2;
            float angle = atan2f(y2 - y1, x2 - x1) + 0.5f;
            float branch_len = 30 * glow;
            SDL_RenderLine(renderer, mid_x, mid_y, 
                          mid_x + cosf(angle) * branch_len,
                          mid_y + sinf(angle) * branch_len);
        }
    }
}

// ----------------------------------------------------------------------------
// Draw shattered glass shards (triangles).
// ----------------------------------------------------------------------------
static void draw_shards(SDL_Renderer *renderer, OpeningAnimation *anim) {
    if (anim->phase < OPENING_PHASE_SHATTER) return;
    
    for (int i = 0; i < anim->shard_count; i++) {
        ScreenShard *s = &anim->shards[i];
        if (s->alpha <= 0) continue;
        
        // Transform triangle points by rotation
        float cos_r = cosf(s->rotation * 3.14159f / 180);
        float sin_r = sinf(s->rotation * 3.14159f / 180);
        
        float tx[3], ty[3];
        for (int j = 0; j < 3; j++) {
            float rx = s->px[j] * cos_r - s->py[j] * sin_r;
            float ry = s->px[j] * sin_r + s->py[j] * cos_r;
            tx[j] = s->x + rx;
            ty[j] = s->y + ry;
        }
        
        Uint8 alpha = (Uint8)(255 * s->alpha);
        SDL_SetRenderDrawColor(renderer, 200, 220, 255, alpha);   // Icy blue shards
        
        // Draw triangle outline
        for (int j = 0; j < 3; j++) {
            SDL_RenderLine(renderer, tx[j], ty[j], tx[(j+1)%3], ty[(j+1)%3]);
        }
        // Fill by drawing lines to centroid
        float center_x = (tx[0] + tx[1] + tx[2]) / 3;
        float center_y = (ty[0] + ty[1] + ty[2]) / 3;
        for (int j = 0; j < 3; j++) {
            SDL_RenderLine(renderer, tx[j], ty[j], center_x, center_y);
        }
        
        // Highlight one edge with white
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, alpha);
        SDL_RenderLine(renderer, tx[0], ty[0], tx[1], ty[1]);
    }
}

// ----------------------------------------------------------------------------
// Draw a red screen flash.
// ----------------------------------------------------------------------------
static void draw_red_flash(SDL_Renderer *renderer, OpeningAnimation *anim, int w, int h) {
    if (anim->red_flash <= 0) return;
    
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, (Uint8)(100 * anim->red_flash));
    SDL_FRect rect = {0, 0, w, h};
    SDL_RenderFillRect(renderer, &rect);
}

// ----------------------------------------------------------------------------
// Render the entire animation, applying shake and overlays.
// ----------------------------------------------------------------------------
void opening_render(SDL_Renderer *renderer, OpeningAnimation *anim) {
    if (!anim || !renderer) return;
    
    int w = anim->window_w;
    int h = anim->window_h;
    
    // Shake offset (random each frame)
    float shake_x = 0, shake_y = 0;
    if (anim->screen_shake > 0) {
        shake_x = (randf() - 0.5f) * anim->screen_shake * 2;
        shake_y = (randf() - 0.5f) * anim->screen_shake * 2;
    }
    
    // Draw in order: background (not shown here – caller draws background),
    // fire, trident, cracks, shards, flash, darkness.
    draw_fire_particles(renderer, anim);
    
    if (anim->phase <= OPENING_PHASE_SHATTER) {
        draw_trident(renderer, anim, shake_x, shake_y);
    }
    
    draw_cracks(renderer, anim, shake_x, shake_y);
    draw_shards(renderer, anim);
    draw_red_flash(renderer, anim, w, h);
    
    if (anim->darkness > 0 && anim->dark_overlay) {
        SDL_SetTextureAlphaMod(anim->dark_overlay, (Uint8)anim->darkness);
        SDL_FRect rect = {shake_x, shake_y, w, h};   // Darkness also shakes slightly
        SDL_RenderTexture(renderer, anim->dark_overlay, NULL, &rect);
    }
    
    // Dramatic "PERISH!" text during impact phase
    if (anim->phase == OPENING_PHASE_IMPACT && anim->font) {
        const char *text = "PERISH!";
        SDL_Color color = {255, 50, 50, 255};
        SDL_Surface *surf = TTF_RenderText_Blended(anim->font, text, 0, color);
        if (surf) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
            float tw, th;
            SDL_GetTextureSize(tex, &tw, &th);
            
            // Pulsing scale and fade‑out
            float scale = 1.5f + sinf(anim->total_timer * 30) * 0.2f;
            float alpha = 1.0f - (anim->phase_timer / PHASE_IMPACT_DURATION);
            
            SDL_SetTextureAlphaMod(tex, (Uint8)(255 * alpha));
            SDL_FRect rect = {
                (w - tw * scale) / 2 + shake_x,
                (h - th * scale) / 2 + shake_y - 100,
                tw * scale, th * scale
            };
            SDL_RenderTexture(renderer, tex, NULL, &rect);
            SDL_DestroyTexture(tex);
            SDL_DestroySurface(surf);
        }
    }
}

// ----------------------------------------------------------------------------
// Return true when the animation is finished.
// ----------------------------------------------------------------------------
bool opening_is_complete(OpeningAnimation *anim) {
    return anim && anim->complete;
}