#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "video_player.h"

// Try to use SDL3's built-in AVI animation support
#define ANIMATION_FILE "game_assets/opening_animation.avi"
#define FALLBACK_DURATION 4.0f

VideoPlayer* video_player_create(SDL_Renderer *renderer, const char *filename) {
    VideoPlayer *player = calloc(1, sizeof(VideoPlayer));
    if (!player) return NULL;
    
    player->filename = strdup(filename ? filename : ANIMATION_FILE);
    player->finished = false;
    player->playing = true;
    player->current_time = 0;
    
    // Try to load as SDL Animation (AVI format)
    player->io = SDL_IOFromFile(player->filename, "rb");
    
    if (!player->io) {
        printf("Video file not found: %s, using fallback animation\n", player->filename);
        printf("Please place your opening video at: %s\n", player->filename);
        player->use_fallback = true;
        player->duration = FALLBACK_DURATION;
        
        // Create a placeholder texture for fallback
        player->frame_surface = SDL_CreateSurface(1920, 1080, SDL_PIXELFORMAT_RGBA8888);
        if (player->frame_surface) {
            // Fill with black initially
            SDL_FillSurfaceRect(player->frame_surface, NULL, 
                SDL_MapRGBA(SDL_GetPixelFormatDetails(player->frame_surface->format), NULL, 0, 0, 0, 255));
            player->frame_texture = SDL_CreateTextureFromSurface(renderer, player->frame_surface);
        }
    } else {
        // SDL3 doesn't have built-in video decoding, so we'll use a simple approach
        // For actual video support, you'd integrate FFmpeg here
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
    if (player->io) SDL_CloseIO(player->io);  // FIXED: removed extra player->
    free(player->filename);
    free(player);
}

void video_player_update(VideoPlayer *player, float delta_time) {
    if (!player || !player->playing || player->finished) return;
    
    player->current_time += delta_time;
    player->fallback_timer += delta_time;
    
    if (player->current_time >= player->duration) {
        player->finished = true;
        player->playing = false;
    }
    
    // Update fallback animation frames if needed
    if (player->use_fallback && player->frame_surface) {
        // This is where you'd decode the next video frame
        // For now, we just keep the surface as-is
    }
}

void video_player_render(SDL_Renderer *renderer, VideoPlayer *player) {
    if (!player || !renderer || !player->frame_texture) return;
    
    int window_w, window_h;
    SDL_GetWindowSize(SDL_GetWindowFromID(1), &window_w, &window_h);
    
    // Calculate progress
    float progress = player->current_time / player->duration;
    
    if (player->use_fallback) {
        // Render dramatic fallback animation based on progress
        
        // Clear to black
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Phase 1: Red glow builds (0-20%)
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
        // Phase 2: Flash and trident strike (20-40%)
        else if (progress < 0.4f) {
            float t = (progress - 0.2f) / 0.2f;
            // White flash
            Uint8 brightness = (Uint8)(255 * (1.0f - t));
            SDL_SetRenderDrawColor(renderer, brightness, brightness, brightness, 255);
            SDL_RenderClear(renderer);
            
            // Red trident silhouette
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            float x = window_w * (0.8f - t * 0.6f); // Move left
            float y = window_h * 0.5f;
            // Simple trident shape
            SDL_FRect shaft = {x - 10, y - 200, 20, 400};
            SDL_RenderFillRect(renderer, &shaft);
            // Prongs
            SDL_RenderLine(renderer, x - 60, y - 200, x, y - 100);
            SDL_RenderLine(renderer, x + 60, y - 200, x, y - 100);
            SDL_RenderLine(renderer, x, y - 250, x, y - 100);
        }
        // Phase 3: Screen crack effect (40-60%)
        else if (progress < 0.6f) {
            float t = (progress - 0.4f) / 0.2f;
            SDL_SetRenderDrawColor(renderer, 20, 0, 0, 255);
            SDL_RenderClear(renderer);
            
            // Crack lines from center
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
        // Phase 4: Shatter and fall (60-80%)
        else if (progress < 0.8f) {
            float t = (progress - 0.6f) / 0.2f;
            SDL_SetRenderDrawColor(renderer, 10, 10, 20, 255);
            SDL_RenderClear(renderer);
            
            // Falling shards - FIXED: use fmod for float modulo or cast to int
            for (int i = 0; i < 20; i++) {
                // Option 1: Cast to int for modulo
                int sx = (i * 100 + (int)(t * 500)) % window_w;
                int sy = (i * 80 + (int)(t * 600)) % window_h;
                
                // Option 2: Use fmod for floating point (if you need float positions)
                // float sx = fmodf(i * 100.0f + t * 500.0f, (float)window_w);
                // float sy = fmodf(i * 80.0f + t * 600.0f, (float)window_h);
                
                float rot = t * 360 + i * 20;
                
                SDL_SetRenderDrawColor(renderer, 150, 150, 200, (Uint8)(255 * (1-t)));
                SDL_FRect shard = {(float)sx, (float)sy, 40, 40};
                SDL_RenderFillRect(renderer, &shard);
            }
        }
        // Phase 5: Fade to black (80-100%)
        else {
            float t = (progress - 0.8f) / 0.2f;
            SDL_SetRenderDrawColor(renderer, 
                (Uint8)(10 * (1-t)), 
                (Uint8)(10 * (1-t)), 
                (Uint8)(20 * (1-t)), 
                255);
            SDL_RenderClear(renderer);
        }
        
        // "PERISH!" text at 35-45%
        if (progress > 0.35f && progress < 0.5f) {
            // Text rendering would go here if font available
        }
    } else {
        // Render actual video texture
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