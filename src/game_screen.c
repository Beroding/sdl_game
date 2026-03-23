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
#define MOVE_SPEED 4.0f

GameScreen *game_screen_create(SDL_Renderer *renderer, int window_width, int window_height) {
    GameScreen *gs = calloc(1, sizeof(GameScreen));
    if (!gs) {
        fprintf(stderr, "Failed to allocate game screen\n");
        return NULL;
    }

    // Load background
    SDL_Surface *surface = IMG_Load("game_assets/map_1.jpg");
    if (!surface) {
        surface = SDL_CreateSurface(window_width, window_height, SDL_PIXELFORMAT_RGBA8888);
        SDL_FillSurfaceRect(surface, NULL, SDL_MapRGBA(SDL_GetPixelFormatDetails(surface->format), NULL, 20, 40, 60, 255));
    }
    
    gs->bg_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_SetTextureScaleMode(gs->bg_texture, SDL_SCALEMODE_PIXELART);
    SDL_DestroySurface(surface);

    if (!gs->bg_texture) {
        fprintf(stderr, "Failed to create game bg texture: %s\n", SDL_GetError());
        free(gs);
        return NULL;
    }

    // LOAD IDLE SPRITE SHEET
    SDL_Surface *idle_surface = IMG_Load("assets/heroes/knight/knight_idle_spritesheet.png");
    if (!idle_surface) {
        fprintf(stderr, "Failed to load idle sprite: %s\n", SDL_GetError());
    } else {
        gs->idle_texture = SDL_CreateTextureFromSurface(renderer, idle_surface);
        SDL_SetTextureScaleMode(gs->idle_texture, SDL_SCALEMODE_NEAREST);
        SDL_DestroySurface(idle_surface);
    }

    // LOAD RUN SPRITE SHEET
    SDL_Surface *run_surface = IMG_Load("assets/heroes/knight/knight_run_spritesheet.png");
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

    // Create "GAME RUNNING" title
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *title_surf = TTF_RenderText_Blended(gs->font, "GAME RUNNING - Press ESC for Menu", 0, white);
    gs->title_text = SDL_CreateTextureFromSurface(renderer, title_surf);
    SDL_DestroySurface(title_surf);

    // Initialize player position (center of screen)
    gs->player_x = window_width / 2.0f;
    gs->player_y = window_height / 2.0f;
    gs->score = 0;
    gs->level = 1;

    printf("Game screen created\n");
    return gs;
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
}

void game_screen_render(SDL_Renderer *renderer, GameScreen *gs) {
    if (!gs) return;

    // Draw background
    SDL_RenderTexture(renderer, gs->bg_texture, NULL, NULL);

    // Use run texture when moving, idle when standing still
    SDL_Texture *current_texture = gs->is_moving ? gs->run_texture : gs->idle_texture;
    
    if (current_texture) {
        // Calculate source rect for current frame
        SDL_FRect src_rect = {
            .x = 1.0f + (gs->current_frame * 16.0f),
            .y = 0,
            .w = FRAME_WIDTH,
            .h = FRAME_HEIGHT
        };
        
        // Destination rect on screen
        SDL_FRect dst_rect = {
            .x = gs->player_x - 32,
            .y = gs->player_y - 32,
            .w = 64,
            .h = 64
        };
        
        // Flip based on last horizontal direction faced
        SDL_FlipMode flip = gs->facing_right ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
        
        SDL_RenderTextureRotated(renderer, current_texture, &src_rect, &dst_rect, 0.0, NULL, flip);
    }

    // Draw title text at top
    float w, h;
    SDL_GetTextureSize(gs->title_text, &w, &h);
    SDL_FRect title_rect = {
        .x = (1500 - w) / 2,
        .y = 30,
        .w = w,
        .h = h
    };
    SDL_RenderTexture(renderer, gs->title_text, NULL, &title_rect);
}

void game_screen_handle_input(GameScreen *gs, SDL_KeyboardEvent *key) {
    if (!gs) return;

    bool is_pressed = (key->type == SDL_EVENT_KEY_DOWN);

    switch (key->scancode) {
        // Horizontal movement - updates facing direction
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
            
        // Vertical movement - does NOT change facing direction
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