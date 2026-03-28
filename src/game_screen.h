#ifndef GAME_SCREEN_H
#define GAME_SCREEN_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "dialogue_loader.h"

#define MAX_LINE_LENGTH 256

typedef struct {
    float x, y;
    float zoom;
    int viewport_w;
    int viewport_h;
} Camera;

typedef struct {
    // Game world data
    float player_x;
    float player_y;
    float npc_x;
    float npc_y;

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
    TTF_Font *font_dialogue;
    
    // Visuals
    SDL_Texture *bg_texture;
    SDL_Texture *idle_texture;
    SDL_Texture *run_texture;
    SDL_Texture *title_text;
    SDL_Texture *npc_idle_texture;
    SDL_Texture *npc_portrait;
    
    // === SCRIPT-BASED DIALOGUE SYSTEM ===
    bool in_dialogue;
    bool dialogue_text_complete;
    bool dialogue_skip_requested;
    bool waiting_for_player_choice;
    bool player_speaking;
    
    // Script-based dialogue
    DialogueScript *current_script;
    int current_line_index;
    
    // UI state
    int dialogue_char_index;
    float dialogue_timer;
    float dialogue_char_delay;
    SDL_Texture *dialogue_box_texture;
    SDL_Texture *dialogue_name_texture;
    
    // Choice buttons (NEW - two separate buttons)
    SDL_FRect choice_a_rect;
    SDL_FRect choice_b_rect;
    bool choice_a_hovered;
    bool choice_b_hovered;
    
    // Dialogue NPC animation
    int dialogue_npc_frame;
    float dialogue_npc_timer;
    float dialogue_npc_frame_duration;
    
    // Dialogue box clickable area
    SDL_FRect dialogue_box_rect;
    bool dialogue_area_hovered;

    // Battle System
    bool battle_triggered;
    
} GameScreen;

GameScreen *game_screen_create(SDL_Renderer *renderer, int window_width, int window_height);
void game_screen_update(GameScreen *game_screen, float delta_time);
void game_screen_render(SDL_Renderer *renderer, GameScreen *game_screen);
void game_screen_handle_input(GameScreen *game_screen, SDL_KeyboardEvent *key);
void game_screen_handle_mouse(GameScreen *game_screen, SDL_MouseButtonEvent *mouse, SDL_Renderer *renderer);
void game_screen_destroy(GameScreen *game_screen);

// === SCRIPT-BASED DIALOGUE FUNCTIONS ===
void dialogue_start_script(GameScreen *gs, SDL_Renderer *renderer, const char *filename);
void dialogue_advance_script(GameScreen *gs, SDL_Renderer *renderer);
void dialogue_select_choice(GameScreen *gs, int choice);
void dialogue_end(GameScreen *gs);
void dialogue_update(GameScreen *gs, float delta_time, SDL_Renderer *renderer);
void dialogue_render(SDL_Renderer *renderer, GameScreen *gs);
bool dialogue_handle_mouse(GameScreen *gs, SDL_MouseButtonEvent *mouse, SDL_Renderer *renderer);
bool dialogue_handle_key(GameScreen *gs, SDL_KeyboardEvent *key, SDL_Renderer *renderer);

#endif