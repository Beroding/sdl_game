#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "game_screen.h"

// PC (Player Character) settings from bayemgame_screen
#define PC_FRAME_WIDTH 38
#define PC_FRAME_HEIGHT 49
#define PC_FRAME_COUNT 7
#define ANIMATION_SPEED 4.5
#define MOVE_SPEED 1.2f
#define ZOOM_LEVEL 5.0f
#define CAMERA_SMOOTH 0.1f

// NPC Animation settings from game_screen
#define NPC_FRAME_WIDTH 65
#define NPC_FRAME_HEIGHT 60
#define NPC_FRAME_COUNT_ANIM 2    // Number of frames in NPC sprite sheet
#define NPC_FRAME_DURATION 0.3f   // Seconds per frame

static float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return value;
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

    // LOAD PC IDLE SPRITE SHEET (bayemgame_screen)
    SDL_Surface *idle_surface = IMG_Load("game_assets/pc_idle.png");
    if (!idle_surface) {
        fprintf(stderr, "Failed to load idle sprite: %s\n", SDL_GetError());
    } else {
        gs->idle_texture = SDL_CreateTextureFromSurface(renderer, idle_surface);
        SDL_SetTextureScaleMode(gs->idle_texture, SDL_SCALEMODE_NEAREST);
        SDL_DestroySurface(idle_surface);
    }

    // LOAD PC RUN SPRITE SHEET (bayemgame_screen)
    SDL_Surface *run_surface = IMG_Load("game_assets/pc_run.png");
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

    // Load NPC (from game_screen)
    SDL_Surface *npc_surface = IMG_Load("game_assets/npc1_idle.png");
    if (!npc_surface) {
        fprintf(stderr, "Failed to load NPC sprite: %s\n", SDL_GetError());
    } else {
        gs->npc_idle_texture = SDL_CreateTextureFromSurface(renderer, npc_surface);
        SDL_SetTextureScaleMode(gs->npc_idle_texture, SDL_SCALEMODE_NEAREST);
        SDL_DestroySurface(npc_surface);
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

    // Initialize player position (center of map) - no Y offset (from bayemgame_screen)
    gs->player_x = gs->map_width / 2.0f;
    gs->player_y = gs->map_height / 2.0f + 17;
    gs->score = 0;
    gs->level = 1;

    // Set NPC spawn (near player) - from game_screen
    gs->npc_x = gs->player_x + 35;
    gs->npc_y = gs->player_y - 10;

    // NPC animation setup
    gs->npc_frame = 0;
    gs->npc_frame_timer = 0.0f;
    gs->npc_frame_duration = NPC_FRAME_DURATION;
    gs->npc_facing_right = false;

    printf("Game screen created\n");
    return gs;
}

void update_camera(GameScreen *gs) {
    Camera *cam = &gs->camera;
    
    float target_x = gs->player_x * cam->zoom - cam->viewport_w / 2.0f;
    float target_y = gs->player_y * cam->zoom - cam->viewport_h / 2.0f;
    
    cam->x += (target_x - cam->x) * CAMERA_SMOOTH;
    cam->y += (target_y - cam->y) * CAMERA_SMOOTH;
    
    float max_x = gs->map_width * cam->zoom - cam->viewport_w;
    float max_y = gs->map_height * cam->zoom - cam->viewport_h;
    
    cam->x = clamp(cam->x, 0, max_x > 0 ? max_x : 0);
    cam->y = clamp(cam->y, 0, max_y > 0 ? max_y : 0);
}

void game_screen_update(GameScreen *gs, float delta_time) {
    if (!gs) return;
    
    // Check if moving in any direction
    gs->is_moving = gs->moving_left || gs->moving_right || 
                    gs->moving_up || gs->moving_down;
    
    // Update player animation frame when moving (PC settings)
    if (gs->is_moving) {
        gs->frame_counter++;
        if (gs->frame_counter >= ANIMATION_SPEED) {
            gs->frame_counter = 0;
            gs->current_frame = (gs->current_frame + 1) % PC_FRAME_COUNT;
        }
    } else {
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

    // Update NPC Animation (from game_screen)
    gs->npc_frame_timer += delta_time;
    if (gs->npc_frame_timer >= gs->npc_frame_duration) {
        gs->npc_frame_timer = 0.0f;
        gs->npc_frame = (gs->npc_frame + 1) % NPC_FRAME_COUNT_ANIM;
    }

    // Update NPC facing direction - FACE TOWARD PLAYER
    if (gs->player_x > gs->npc_x) {
        gs->npc_facing_right = true;
    } else {
        gs->npc_facing_right = false;
    }
    
    // NPC Interaction Range
    float dx = gs->player_x - gs->npc_x;
    float dy = gs->player_y - gs->npc_y;
    float distance_sq = dx*dx + dy*dy;

    gs->player_near_npc = (distance_sq < 40 * 40);

    // Update camera to follow player
    update_camera(gs);
}

void game_screen_render(SDL_Renderer *renderer, GameScreen *gs) {
    if (!gs) return;

    Camera *cam = &gs->camera;
    
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw background/map
    SDL_FRect src_rect = {
        .x = cam->x / cam->zoom,
        .y = cam->y / cam->zoom,
        .w = cam->viewport_w / cam->zoom,
        .h = cam->viewport_h / cam->zoom
    };
    
    if (src_rect.x + src_rect.w > gs->map_width) {
        src_rect.w = gs->map_width - src_rect.x;
    }
    if (src_rect.y + src_rect.h > gs->map_height) {
        src_rect.h = gs->map_height - src_rect.y;
    }
    
    SDL_FRect dst_rect = {
        .x = 0,
        .y = 0,
        .w = cam->viewport_w,
        .h = cam->viewport_h
    };
    
    SDL_RenderTexture(renderer, gs->bg_texture, &src_rect, &dst_rect);

    // Calculate positions
    float screen_x = (gs->player_x * cam->zoom) - cam->x;
    float screen_y = (gs->player_y * cam->zoom) - cam->y;

    float npc_screen_x = (gs->npc_x * cam->zoom) - cam->x;
    float npc_screen_y = (gs->npc_y * cam->zoom) - cam->y;

    float sprite_size = 16 * cam->zoom;  // PC sprite size
    float npc_sprite_size = 35 * cam->zoom;

    // =========================
    // DRAW NPC (BEFORE PLAYER)
    // =========================
    if (gs->npc_idle_texture) {
        int row = gs->npc_facing_right ? 3 : 1;  // Fixed: was 3.5 (invalid)
        
        SDL_FRect src_sprite = {
            .x = (float)(gs->npc_frame * NPC_FRAME_WIDTH),
            .y = (float)(row * (NPC_FRAME_HEIGHT + 4)),
            .w = NPC_FRAME_WIDTH,
            .h = NPC_FRAME_HEIGHT
        };

        SDL_FRect dst_sprite = {
            .x = npc_screen_x - npc_sprite_size / 2,
            .y = npc_screen_y - npc_sprite_size / 2,
            .w = npc_sprite_size,
            .h = npc_sprite_size
        };

        SDL_RenderTexture(renderer, gs->npc_idle_texture, &src_sprite, &dst_sprite);
    }

    // =========================
    // DRAW PLAYER (PC)
    // =========================
    SDL_Texture *current_texture = gs->is_moving ? gs->run_texture : gs->idle_texture;
    
    if (current_texture) {
        // PC sprite sheet uses Y offset 207 (from bayemgame_screen)
        SDL_FRect src_sprite = {
            .x = 2.0f + (gs->current_frame * PC_FRAME_WIDTH),
            .y = 207,  // PC sprite sheet row
            .w = PC_FRAME_WIDTH,
            .h = PC_FRAME_HEIGHT
        };
        
        SDL_FRect dst_sprite = {
            .x = screen_x - sprite_size / 2,
            .y = screen_y - sprite_size / 2,
            .w = sprite_size,
            .h = sprite_size
        };
        
        SDL_FlipMode flip = gs->facing_right ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
        
        SDL_RenderTextureRotated(renderer, current_texture, &src_sprite, &dst_sprite, 0.0, NULL, flip);
    }

    // NPC INTERACTION PROMPT (from game_screen)
    if (gs->player_near_npc) {
        SDL_Surface *surf = TTF_RenderText_Blended(
            gs->font,
            "Press E to talk",
            0,
            (SDL_Color){255,255,0,255}
        );

        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_DestroySurface(surf);

        float w, h;
        SDL_GetTextureSize(tex, &w, &h);

        SDL_FRect rect = {
            .x = npc_screen_x - w / 2,
            .y = npc_screen_y - npc_sprite_size - h,
            .w = w,
            .h = h
        };

        SDL_RenderTexture(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
    }
    else {
        SDL_Surface *surf = TTF_RenderText_Blended(
            gs->font,
            "Come Here Tarnished",
            0,  // Removed offensive language
            (SDL_Color){255,0,0,125}
        );

        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_DestroySurface(surf);

        float w, h;
        SDL_GetTextureSize(tex, &w, &h);

        SDL_FRect rect = {
            .x = npc_screen_x - w / 2,
            .y = npc_screen_y - npc_sprite_size,
            .w = w,
            .h = h
        };

        SDL_RenderTexture(renderer, tex, NULL, &rect);
        SDL_DestroyTexture(tex);
    }

    // Draw title text at top
    float w, h;
    SDL_GetTextureSize(gs->title_text, &w, &h);
    SDL_FRect title_rect = {
        .x = (cam->viewport_w - w) / 2,
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

        case SDL_SCANCODE_E:
            if (is_pressed && gs->player_near_npc) {
                printf("Talking to NPC!\n");
            }
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
        if (gs->npc_idle_texture) SDL_DestroyTexture(gs->npc_idle_texture);
        free(gs);
        printf("Game screen destroyed\n");
    }
}