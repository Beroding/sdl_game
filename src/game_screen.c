#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "game_screen.h"

GameScreen *game_screen_create(SDL_Renderer *renderer, int window_width, int window_height) {
    GameScreen *gs = calloc(1, sizeof(GameScreen));
    if (!gs) {
        fprintf(stderr, "Failed to allocate game screen\n");
        return NULL;
    }

    // Load background
    SDL_Surface *surface = IMG_Load("assets/tiles/floor/floor_stair.png");
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

    // LOAD PLAYER SPRITE SHEET
    SDL_Surface *player_surface = IMG_Load("knight_idle_spritesheet.png");
    if (!player_surface) {
        fprintf(stderr, "Failed to load player sprite: %s\n", SDL_GetError());
        // Continue without player texture - will just not render player
    } else {
        gs->player_texture = SDL_CreateTextureFromSurface(renderer, player_surface);
        SDL_SetTextureScaleMode(gs->player_texture, SDL_SCALEMODE_NEAREST); // Set once here
        SDL_DestroySurface(player_surface);
        
        if (!gs->player_texture) {
            fprintf(stderr, "Failed to create player texture: %s\n", SDL_GetError());
        }
    }

    // Load font for HUD
    gs->font = TTF_OpenFont("game_assets/Roboto_Medium.ttf", 36);
    if (!gs->font) {
        fprintf(stderr, "Game font load error: %s\n", SDL_GetError());
        SDL_DestroyTexture(gs->bg_texture);
        SDL_DestroyTexture(gs->player_texture); // Clean up player texture too
        free(gs);
        return NULL;
    }

    gs->frame_counter = 0;
    gs->next_frame = 0;

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

    // Player rectangle (placeholder for sprite)
    gs->player_rect.w = 50;
    gs->player_rect.h = 50;
    gs->player_rect.x = gs->player_x - 25;
    gs->player_rect.y = gs->player_y - 25;

    printf("Game screen created\n");
    return gs;
}

void game_screen_update(GameScreen *gs, float delta_time) {
    if (!gs) return;
    
    // Update rect position based on player position
    gs->player_rect.x = gs->player_x - 25;
    gs->player_rect.y = gs->player_y - 25;
}

void game_screen_render(SDL_Renderer *renderer, GameScreen *gs) {
    if (!gs) return;

    // Draw background
    SDL_RenderTexture(renderer, gs->bg_texture, NULL, NULL);

    // Animate player sprite
    gs->frame_counter++;
    if (gs->frame_counter >= 10) // change every ~160ms (16ms loop)
    {
        gs->frame_counter = 0;
        gs->next_frame += 16;
        if (gs->next_frame > 81) // wrap at last frame x=81
        {
            gs->next_frame = 0;
        }
    }

    // Draw player (only if texture exists)
    if (gs->player_texture) {
        SDL_FRect char_sprite = {1 + gs->next_frame, 0, 15, 16}; // Sprite sheet source rect
        SDL_FRect char_position = {gs->player_x - 32, gs->player_y - 32, 64, 64}; // Use player position!
        
        // Remove SDL_SetTextureScaleMode from here - already set in create
        SDL_RenderTexture(renderer, gs->player_texture, &char_sprite, &char_position);
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
    if (!gs || key->repeat) return;

    const float speed = 10.0f;

    switch (key->scancode) {
        case SDL_SCANCODE_W:
        case SDL_SCANCODE_UP:
            gs->player_y -= speed;
            break;
        case SDL_SCANCODE_S:
        case SDL_SCANCODE_DOWN:
            gs->player_y += speed;
            break;
        case SDL_SCANCODE_A:
        case SDL_SCANCODE_LEFT:
            gs->player_x -= speed;
            break;
        case SDL_SCANCODE_D:
        case SDL_SCANCODE_RIGHT:
            gs->player_x += speed;
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
        if (gs->player_texture) SDL_DestroyTexture(gs->player_texture); // Clean up player texture
        free(gs);
        printf("Game screen destroyed\n");
    }
}