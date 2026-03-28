#ifndef GAME_SCREEN_H
#define GAME_SCREEN_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "dialogue_loader.h"

#define MAX_LINE_LENGTH 256

/* Camera tracks player position with smooth interpolation */
typedef struct {
    float x, y;
    float zoom;          /* Higher = closer to player */
    int viewport_w;
    int viewport_h;
} Camera;

/* Main gameplay state - handles world exploration and dialogue */
typedef struct {
    /* World positions */
    float player_x;
    float player_y;
    float npc_x;
    float npc_y;

    /* Progress tracking */
    int score;
    int level;

    /* Animation timing */
    int npc_frame;
    float npc_frame_timer;
    float npc_frame_duration;
    
    int map_width;
    int map_height;
    
    /* Input state - true while key held */
    bool moving_left;
    bool moving_right;
    bool moving_up;
    bool moving_down;
    
    /* Player sprite animation */
    int frame_counter;      /* Frames until next sprite */
    int current_frame;      /* 0-6 for 7-frame animation */
    bool facing_right;
    bool is_moving;
    
    bool npc_facing_right;
    bool player_near_npc;   /* True when close enough to talk */

    Camera camera;
    
    TTF_Font *font;
    TTF_Font *font_bold;
    TTF_Font *font_dialogue;
    
    SDL_Texture *bg_texture;
    SDL_Texture *idle_texture;
    SDL_Texture *run_texture;
    SDL_Texture *title_text;
    SDL_Texture *npc_idle_texture;
    SDL_Texture *npc_portrait;
    
    /* Dialogue system state */
    bool in_dialogue;
    bool dialogue_text_complete;  /* All characters displayed */
    bool dialogue_skip_requested;   /* Player pressed skip key */
    bool waiting_for_player_choice; /* Showing A/B buttons */
    bool player_speaking;         /* Speaker is "You" */
    
    DialogueScript *current_script;
    int current_line_index;
    
    /* Typewriter effect */
    int dialogue_char_index;        /* Characters revealed so far */
    float dialogue_timer;           /* Time until next character */
    float dialogue_char_delay;      /* Seconds per character */
    SDL_Texture *dialogue_box_texture;
    SDL_Texture *dialogue_name_texture;
    
    /* Choice button interaction */
    SDL_FRect choice_a_rect;
    SDL_FRect choice_b_rect;
    bool choice_a_hovered;
    bool choice_b_hovered;
    
    /* NPC animation during dialogue (separate from world) */
    int dialogue_npc_frame;
    float dialogue_npc_timer;
    float dialogue_npc_frame_duration;
    
    SDL_FRect dialogue_box_rect;
    bool dialogue_area_hovered;

    /* Set by dialogue script to trigger battle after closing */
    bool battle_triggered;
    
} GameScreen;

GameScreen *game_screen_create(SDL_Renderer *renderer, int window_width, int window_height);
void game_screen_destroy(GameScreen *game_screen);

void game_screen_update(GameScreen *game_screen, float delta_time);
void game_screen_render(SDL_Renderer *renderer, GameScreen *game_screen);

void game_screen_handle_input(GameScreen *game_screen, SDL_KeyboardEvent *key);
void game_screen_handle_mouse(GameScreen *game_screen, SDL_MouseButtonEvent *mouse, SDL_Renderer *renderer);

/* Dialogue functions - script-driven conversation system */
void dialogue_start_script(GameScreen *gs, SDL_Renderer *renderer, const char *filename);
void dialogue_advance_script(GameScreen *gs, SDL_Renderer *renderer);
void dialogue_select_choice(GameScreen *gs, int choice);
void dialogue_end(GameScreen *gs);
void dialogue_update(GameScreen *gs, float delta_time, SDL_Renderer *renderer);
void dialogue_render(SDL_Renderer *renderer, GameScreen *gs);
bool dialogue_handle_mouse(GameScreen *gs, SDL_MouseButtonEvent *mouse, SDL_Renderer *renderer);
bool dialogue_handle_key(GameScreen *gs, SDL_KeyboardEvent *key, SDL_Renderer *renderer);

#endif