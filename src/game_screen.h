#ifndef GAME_SCREEN_H
#define GAME_SCREEN_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

typedef struct {
    float x, y;        // Camera position (top-left corner)
    float zoom;        // Zoom level (2.0 = 2x zoom)
    int viewport_w;    // Screen width
    int viewport_h;    // Screen height
} Camera;

typedef struct {
    // Game world data
    float player_x;
    float player_y;
    float npc_x;       // first npc
    float npc_y;       // first npc

    int score;
    int level;

    // NPC data
    int npc_frame;
    float npc_frame_timer;
    float npc_frame_duration;
    
    // Map data
    int map_width;
    int map_height;
    
    // Movement state
    bool moving_left;
    bool moving_right;
    bool moving_up;
    bool moving_down;
    
    // Animation
    int frame_counter;
    int current_frame;
    bool facing_right;
    bool is_moving;
    
    // NPC Animation
    bool npc_facing_right;

    // NPC Interaction
    bool player_near_npc;

    // Camera
    Camera camera;
    
    // Fonts
    TTF_Font *font;
    TTF_Font *font_bold;
    
    // Visuals
    SDL_Texture *bg_texture;
    SDL_Texture *idle_texture;
    SDL_Texture *run_texture;
    SDL_Texture *title_text;
    SDL_Texture *npc_idle_texture;  // first npc
} GameScreen;

GameScreen *game_screen_create(SDL_Renderer *renderer, int window_width, int window_height);
void game_screen_update(GameScreen *game_screen, float delta_time);
void game_screen_render(SDL_Renderer *renderer, GameScreen *game_screen);
void game_screen_handle_input(GameScreen *game_screen, SDL_KeyboardEvent *key);
void game_screen_destroy(GameScreen *game_screen);

#endif