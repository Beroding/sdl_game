#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "video_player.h"

// The animation file we attempt to load. SDL3 does not have built-in video decoding,
// so we provide a fallback animated sequence that creates a dramatic intro effect.
#define ANIMATION_FILE "game_assets/opening_animation.avi"
#define FALLBACK_DURATION 4.0f

/*
 * Creates a video player instance.
 * We try to open the video file; if it's missing or SDL can't decode it,
 * we fall back to a procedurally generated animation. This ensures the game
 * always shows something instead of crashing.
 *
 * Assumption: The renderer is valid and will exist for the player's lifetime.
 */
VideoPlayer* video_player_create(SDL_Renderer *renderer, const char *filename) {
    VideoPlayer *player = calloc(1, sizeof(VideoPlayer));
    if (!player) return NULL;
    
    player->filename = strdup(filename ? filename : ANIMATION_FILE);
    player->finished = false;
    player->playing = true;
    player->current_time = 0;
    
    // Attempt to open the file as an SDL_IOStream. Even if it exists,
    // SDL3 does not decode video, so we will always use the fallback for now.
    player->io = SDL_IOFromFile(player->filename, "rb");
    
    if (!player->io) {
        printf("Video file not found: %s, using fallback animation\n", player->filename);
        printf("Please place your opening video at: %s\n", player->filename);
        player->use_fallback = true;
        player->duration = FALLBACK_DURATION;
        
        // Create a surface for the fallback frames. The actual content is drawn
        // procedurally in video_player_render, so we only need a placeholder texture.
        player->frame_surface = SDL_CreateSurface(1920, 1080, SDL_PIXELFORMAT_RGBA8888);
        if (player->frame_surface) {
            SDL_FillSurfaceRect(player->frame_surface, NULL, 
                SDL_MapRGBA(SDL_GetPixelFormatDetails(player->frame_surface->format), NULL, 0, 0, 0, 255));
            player->frame_texture = SDL_CreateTextureFromSurface(renderer, player->frame_surface);
        }
    } else {
        // In a full implementation, we would use FFmpeg or another library to decode frames.
        // For now, we log that the file exists but still fall back to the procedural animation.
        printf("Video file found but requires FFmpeg integration\n");
        printf("Using high-quality fallback animation\n");
        player->use_fallback = true;
        player->duration = FALLBACK_DURATION;
        
        SDL_CloseIO(player->io);
        player->io = NULL;
        
        player->frame_surface = SDL_CreateSurface(1920, 1080, SDL_PIXELFORMAT_RGBA8888);
        if (player->frame_surface) {
            player->frame_texture = SDL_CreateTextureFromSurface(renderer, player->frame_surface);
        }
    }
    
    return player;
}

void video_player_destroy(VideoPlayer *player) {
    if (!player) return;
    
    if (player->frame_texture) SDL_DestroyTexture(player->frame_texture);
    if (player->frame_surface) SDL_DestroySurface(player->frame_surface);
    if (player->io) SDL_CloseIO(player->io);
    free(player->filename);
    free(player);
}

/*
 * Advances the playback time and marks the video as finished when duration is reached.
 * The fallback animation timer is also incremented so that the rendering code can
 * show the appropriate frame based on progress.
 */
void video_player_update(VideoPlayer *player, float delta_time) {
    if (!player || !player->playing || player->finished) return;
    
    player->current_time += delta_time;
    player->fallback_timer += delta_time;
    
    if (player->current_time >= player->duration) {
        player->finished = true;
        player->playing = false;
    }
    
    // For fallback, we could decode frames here if we had actual video data.
    // Currently, all the animation logic lives in the render function.
}

/*
 * Renders the video frame. If using the fallback, we draw a multi‑phase
 * procedural animation that builds a dramatic "shattering" effect.
 * The phases are time‑based (progress from 0 to 1) to create a coherent story.
 */
void video_player_render(SDL_Renderer *renderer, VideoPlayer *player) {
    if (!player || !renderer || !player->frame_texture) return;
    
    int window_w, window_h;
    SDL_GetWindowSize(SDL_GetWindowFromID(1), &window_w, &window_h);
    
    float progress = player->current_time / player->duration;
    
    if (player->use_fallback) {
        // Clear to black as a base.
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Phase 1: Red glow builds from the center (0‑20%).
        if (progress < 0.2f) {
            float intensity = progress / 0.2f;
            SDL_SetRenderDrawColor(renderer, 
                (Uint8)(255 * intensity), 
                (Uint8)(50 * intensity), 
                0, 
                (Uint8)(200 * intensity));
            SDL_FRect glow = {
                window_w * 0.5f - 200 * intensity, 
                window_h * 0.5f - 200 * intensity,
                400 * intensity, 400 * intensity
            };
            SDL_RenderFillRect(renderer, &glow);
        }
        // Phase 2: White flash and a trident silhouette appears (20‑40%).
        else if (progress < 0.4f) {
            float t = (progress - 0.2f) / 0.2f;
            // Flash: starts bright, fades out.
            Uint8 brightness = (Uint8)(255 * (1.0f - t));
            SDL_SetRenderDrawColor(renderer, brightness, brightness, brightness, 255);
            SDL_RenderClear(renderer);
            
            // Draw a red trident shape that moves across the screen.
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            float x = window_w * (0.8f - t * 0.6f);
            float y = window_h * 0.5f;
            SDL_FRect shaft = {x - 10, y - 200, 20, 400};
            SDL_RenderFillRect(renderer, &shaft);
            SDL_RenderLine(renderer, x - 60, y - 200, x, y - 100);
            SDL_RenderLine(renderer, x + 60, y - 200, x, y - 100);
            SDL_RenderLine(renderer, x, y - 250, x, y - 100);
        }
        // Phase 3: Screen cracks outward from the center (40‑60%).
        else if (progress < 0.6f) {
            float t = (progress - 0.4f) / 0.2f;
            SDL_SetRenderDrawColor(renderer, 20, 0, 0, 255);
            SDL_RenderClear(renderer);
            
            SDL_SetRenderDrawColor(renderer, 255, 200, 100, 255);
            float cx = window_w * 0.3f;
            float cy = window_h * 0.5f;
            for (int i = 0; i < 12; i++) {
                float angle = (i / 12.0f) * 2 * 3.14159f;
                float len = 100 + t * 400;
                SDL_RenderLine(renderer, cx, cy, 
                    cx + cosf(angle) * len, 
                    cy + sinf(angle) * len);
            }
        }
        // Phase 4: Screen shatters into pieces that fall (60‑80%).
        else if (progress < 0.8f) {
            float t = (progress - 0.6f) / 0.2f;
            SDL_SetRenderDrawColor(renderer, 10, 10, 20, 255);
            SDL_RenderClear(renderer);
            
            // Draw 20 shards that move and rotate.
            for (int i = 0; i < 20; i++) {
                // Use integer positions to avoid floating‑point modulo complexity.
                int sx = (i * 100 + (int)(t * 500)) % window_w;
                int sy = (i * 80 + (int)(t * 600)) % window_h;
                float rot = t * 360 + i * 20;
                
                SDL_SetRenderDrawColor(renderer, 150, 150, 200, (Uint8)(255 * (1 - t)));
                SDL_FRect shard = {(float)sx, (float)sy, 40, 40};
                SDL_RenderFillRect(renderer, &shard);
            }
        }
        // Phase 5: Fade to black (80‑100%).
        else {
            float t = (progress - 0.8f) / 0.2f;
            SDL_SetRenderDrawColor(renderer, 
                (Uint8)(10 * (1 - t)), 
                (Uint8)(10 * (1 - t)), 
                (Uint8)(20 * (1 - t)), 
                255);
            SDL_RenderClear(renderer);
        }
        
        // A simple "PERISH!" text would appear here if we had a font renderer.
        // Currently it's omitted because adding font dependencies would complicate the example.
    } else {
        // Real video texture rendering – would stretch the decoded frame to fit the window.
        SDL_FRect dst = {0, 0, window_w, window_h};
        SDL_RenderTexture(renderer, player->frame_texture, NULL, &dst);
    }
}

bool video_player_is_finished(VideoPlayer *player) {
    return player && player->finished;
}

void video_player_skip(VideoPlayer *player) {
    if (player) {
        player->finished = true;
        player->playing = false;
    }
}