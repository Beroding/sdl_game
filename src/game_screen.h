#ifndef GAME_SCREEN_H
#define GAME_SCREEN_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

typedef struct {
    // Game world data
    float player_x;
    float player_y;
    int score;
    int level;
    int frame_counter;
    int next_frame;
    
    // Visuals
    SDL_Texture *bg_texture;
    TTF_Font *font;
    SDL_Texture *title_text;
    
    // Layout
    SDL_FRect player_rect;
    SDL_Texture *player_texture;
} GameScreen;

GameScreen *game_screen_create(SDL_Renderer *renderer, int window_width, int window_height);
void game_screen_update(GameScreen *game_screen, float delta_time);
void game_screen_render(SDL_Renderer *renderer, GameScreen *game_screen);
void game_screen_handle_input(GameScreen *game_screen, SDL_KeyboardEvent *key);
void game_screen_destroy(GameScreen *game_screen);

#endif