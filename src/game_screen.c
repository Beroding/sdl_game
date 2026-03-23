#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "game_screen.h"

#define FRAME_WIDTH 15
#define FRAME_HEIGHT 16
#define FRAME_COUNT 6
#define ANIMATION_SPEED 6
#define MOVE_SPEED 1.0f
#define ZOOM_LEVEL 5.0f  // 4x zoom
#define CAMERA_SMOOTH 0.1f  // Lower = smoother but more lag (0.0-1.0)

// Helper to clamp value between min and max
static float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

GameScreen *game_screen_create(SDL_Renderer *renderer, int window_width, int window_height) {
    GameScreen *gs = calloc(1, sizeof(GameScreen));
    if (!gs) {
        fprintf(stderr, "Failed to allocate game screen\n");
        return NULL;
    }

    // Load background/map
    SDL_Surface *surface = IMG_Load("game_assets/map_1.jpg");
    if (!surface) {
        fprintf(stderr, "Failed to load map: %s\n", SDL_GetError());
        surface = SDL_CreateSurface(window_width, window_height, SDL_PIXELFORMAT_RGBA8888);
        SDL_FillSurfaceRect(surface, NULL, SDL_MapRGBA(SDL_GetPixelFormatDetails(surface->format), NULL, 20, 40, 60, 255));
        gs->map_width = window_width;
        gs->map_height = window_height;
    } else {
        gs->map_width = surface->w;
        gs->map_height = surface->h;
    }
    
    gs->bg_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_SetTextureScaleMode(gs->bg_texture, SDL_SCALEMODE_NEAREST);
    SDL_DestroySurface(surface);

    if (!gs->bg_texture) {
        fprintf(stderr, "Failed to create game bg texture: %s\n", SDL_GetError());
        free(gs);
        return NULL;
    }

    printf("Map size: %dx%d\n", gs->map_width, gs->map_height);

    // LOAD IDLE SPRITE SHEET
    SDL_Surface *idle_surface = IMG_Load("game_assets/knight_idle_spritesheet.png");
    if (!idle_surface) {
        fprintf(stderr, "Failed to load idle sprite: %s\n", SDL_GetError());
    } else {
        gs->idle_texture = SDL_CreateTextureFromSurface(renderer, idle_surface);
        SDL_SetTextureScaleMode(gs->idle_texture, SDL_SCALEMODE_NEAREST);
        SDL_DestroySurface(idle_surface);
    }

    // LOAD RUN SPRITE SHEET
    SDL_Surface *run_surface = IMG_Load("game_assets/knight_run_spritesheet.png");
    if (!run_surface) {
        fprintf(stderr, "Failed to load run sprite: %s\n", SDL_GetError());
    } else {
        gs->run_texture = SDL_CreateTextureFromSurface(renderer, run_surface);
        SDL_SetTextureScaleMode(gs->run_texture, SDL_SCALEMODE_NEAREST);
        SDL_DestroySurface(run_surface);
    }

    // Load font for HUD
    gs->font = TTF_OpenFont("game_assets/Roboto_Medium.ttf", 36);
    if (!gs->font) {
        fprintf(stderr, "Game font load error: %s\n", SDL_GetError());
        SDL_DestroyTexture(gs->bg_texture);
        SDL_DestroyTexture(gs->idle_texture);
        SDL_DestroyTexture(gs->run_texture);
        free(gs);
        return NULL;
    }

    // Animation state
    gs->frame_counter = 0;
    gs->current_frame = 0;
    gs->facing_right = true;
    gs->is_moving = false;
    
    // Movement flags
    gs->moving_left = false;
    gs->moving_right = false;
    gs->moving_up = false;
    gs->moving_down = false;

    // Initialize camera
    gs->camera.zoom = ZOOM_LEVEL;
    gs->camera.viewport_w = window_width;
    gs->camera.viewport_h = window_height;
    gs->camera.x = 0;
    gs->camera.y = 0;

    // Create "GAME RUNNING" title
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *title_surf = TTF_RenderText_Blended(gs->font, "GAME RUNNING - Press ESC for Menu", 0, white);
    gs->title_text = SDL_CreateTextureFromSurface(renderer, title_surf);
    SDL_DestroySurface(title_surf);

    // Initialize player position (center of map)
    gs->player_x = gs->map_width / 2.0f;
    gs->player_y = gs->map_height / 2.0f;
    gs->score = 0;
    gs->level = 1;

    printf("Game screen created\n");
    return gs;
}

void update_camera(GameScreen *gs) {
    Camera *cam = &gs->camera;
    
    // Calculate desired camera position (centered on player)
    float target_x = gs->player_x * cam->zoom - cam->viewport_w / 2.0f;
    float target_y = gs->player_y * cam->zoom - cam->viewport_h / 2.0f;
    
    // Smooth camera movement (lerp)
    cam->x += (target_x - cam->x) * CAMERA_SMOOTH;
    cam->y += (target_y - cam->y) * CAMERA_SMOOTH;
    
    // Calculate max camera bounds (so we don't show beyond the map)
    float max_x = gs->map_width * cam->zoom - cam->viewport_w;
    float max_y = gs->map_height * cam->zoom - cam->viewport_h;
    
    // Clamp camera to map bounds (don't show black space)
    cam->x = clamp(cam->x, 0, max_x > 0 ? max_x : 0);
    cam->y = clamp(cam->y, 0, max_y > 0 ? max_y : 0);
}

void game_screen_update(GameScreen *gs, float delta_time) {
    if (!gs) return;
    
    // Check if moving in any direction
    gs->is_moving = gs->moving_left || gs->moving_right || 
                    gs->moving_up || gs->moving_down;
    
    // Update animation frame when moving
    if (gs->is_moving) {
        gs->frame_counter++;
        if (gs->frame_counter >= ANIMATION_SPEED) {
            gs->frame_counter = 0;
            gs->current_frame = (gs->current_frame + 1) % FRAME_COUNT;
        }
    } else {
        // Reset to idle frame when standing still
        gs->current_frame = 0;
        gs->frame_counter = 0;
    }
    
    // Apply movement
    if (gs->moving_right) {
        gs->player_x += MOVE_SPEED;
    }
    if (gs->moving_left) {
        gs->player_x -= MOVE_SPEED;
    }
    if (gs->moving_up) {
        gs->player_y -= MOVE_SPEED;
    }
    if (gs->moving_down) {
        gs->player_y += MOVE_SPEED;
    }
    
    // Clamp player to map bounds
    gs->player_x = clamp(gs->player_x, 0, gs->map_width);
    gs->player_y = clamp(gs->player_y, 0, gs->map_height);
    
    // Update camera to follow player
    update_camera(gs);
}

void game_screen_render(SDL_Renderer *renderer, GameScreen *gs) {
    if (!gs) return;

    Camera *cam = &gs->camera;
    
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw background/map with zoom and camera offset
    // Source rect: which part of the original map to show
    SDL_FRect src_rect = {
        .x = cam->x / cam->zoom,
        .y = cam->y / cam->zoom,
        .w = cam->viewport_w / cam->zoom,
        .h = cam->viewport_h / cam->zoom
    };
    
    // Clamp source rect to map bounds
    if (src_rect.x + src_rect.w > gs->map_width) {
        src_rect.w = gs->map_width - src_rect.x;
    }
    if (src_rect.y + src_rect.h > gs->map_height) {
        src_rect.h = gs->map_height - src_rect.y;
    }
    
    // Destination: full screen
    SDL_FRect dst_rect = {
        .x = 0,
        .y = 0,
        .w = cam->viewport_w,
        .h = cam->viewport_h
    };
    
    SDL_RenderTexture(renderer, gs->bg_texture, &src_rect, &dst_rect);

    // Calculate player screen position (relative to camera)
    float screen_x = (gs->player_x * cam->zoom) - cam->x;
    float screen_y = (gs->player_y * cam->zoom) - cam->y;

    // Draw player
    SDL_Texture *current_texture = gs->is_moving ? gs->run_texture : gs->idle_texture;
    
    if (current_texture) {
        // Source rect for animation frame
        SDL_FRect src_sprite = {
            .x = 1.0f + (gs->current_frame * 16.0f),
            .y = 0,
            .w = FRAME_WIDTH,
            .h = FRAME_HEIGHT
        };
        
        // Destination rect (scaled by zoom)
        float sprite_size = 16 * cam->zoom;  // Scale sprite with zoom
        SDL_FRect dst_sprite = {
            .x = screen_x - sprite_size / 2,
            .y = screen_y - sprite_size / 2,
            .w = sprite_size,
            .h = sprite_size
        };
        
        // Flip based on facing direction
        SDL_FlipMode flip = gs->facing_right ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
        
        SDL_RenderTextureRotated(renderer, current_texture, &src_sprite, &dst_sprite, 0.0, NULL, flip);
    }

    // Draw title text at top (fixed to screen, not world)
    float w, h;
    SDL_GetTextureSize(gs->title_text, &w, &h);
    SDL_FRect title_rect = {
        .x = (cam->viewport_w - w) / 2,
        .y = 30,
        .w = w,
        .h = h
    };
    SDL_RenderTexture(renderer, gs->title_text, NULL, &title_rect);
    
    // Debug: draw player position
    // printf("Player: %.1f, %.1f | Camera: %.1f, %.1f | Screen: %.1f, %.1f\n",
    //        gs->player_x, gs->player_y, cam->x, cam->y, screen_x, screen_y);
}

void game_screen_handle_input(GameScreen *gs, SDL_KeyboardEvent *key) {
    if (!gs) return;

    bool is_pressed = (key->type == SDL_EVENT_KEY_DOWN);

    switch (key->scancode) {
        case SDL_SCANCODE_D:
        case SDL_SCANCODE_RIGHT:
            gs->moving_right = is_pressed;
            if (is_pressed) gs->facing_right = true;
            break;
            
        case SDL_SCANCODE_A:
        case SDL_SCANCODE_LEFT:
            gs->moving_left = is_pressed;
            if (is_pressed) gs->facing_right = false;
            break;
            
        case SDL_SCANCODE_W:
        case SDL_SCANCODE_UP:
            gs->moving_up = is_pressed;
            break;
            
        case SDL_SCANCODE_S:
        case SDL_SCANCODE_DOWN:
            gs->moving_down = is_pressed;
            break;
            
        default:
            break;
    }
}

void game_screen_destroy(GameScreen *gs) {
    if (gs) {
        if (gs->title_text) SDL_DestroyTexture(gs->title_text);
        if (gs->font) TTF_CloseFont(gs->font);
        if (gs->bg_texture) SDL_DestroyTexture(gs->bg_texture);
        if (gs->idle_texture) SDL_DestroyTexture(gs->idle_texture);
        if (gs->run_texture) SDL_DestroyTexture(gs->run_texture);
        free(gs);
        printf("Game screen destroyed\n");
    }
}