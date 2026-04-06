#ifndef GAME_SCREEN_H
#define GAME_SCREEN_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "dialogue_loader.h"
#include "collision_system.h"
#include "spawn_animation.h"

#define MAX_LINE_LENGTH 256
#define MAX_NPCS 4
#define MAX_TRAIL_POINTS 100

/* Camera tracks player position with smooth interpolation */
typedef struct {
    float x, y;
    float zoom;
    int viewport_w;
    int viewport_h;
} Camera;

typedef enum {
    FACING_DOWN,
    FACING_UP, 
    FACING_LEFT,
    FACING_RIGHT
} FacingDirection;

/* NPC structure – must be defined before GameScreen */
typedef struct {
    float x, y;                  
    float target_x, target_y;    
    float speed;                 
    float move_timer;            
    int move_pattern;            
    int current_frame;           
    float anim_timer;            
    bool facing_right;           
    bool is_moving;              
    bool is_active;              
    bool trigger_trail;          
    char dialogue_file[256];     
    int dialogue_line;           
    
    // For patrol pattern
    float patrol_min_x, patrol_max_x;  // boundaries for patrol
    float patrol_min_y, patrol_max_y;  // for vertical patrol (optional)
} NPC;

/* TrailPoint structure – also defined before GameScreen */
typedef struct {
    float x, y;                  // Trail waypoint position
    float lifetime;              // Time to live (seconds) – for fading
    bool active;
} TrailPoint;

/* Main gameplay state - handles world exploration and dialogue */
typedef struct {
    /* World positions */
    float player_x;
    float player_y;
    float npc_x;
    float npc_y;

    CollisionWorld *collision;  // NEW: collision system

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

    bool is_running;        // True when player is moving fast (shift key?)
    float move_speed;  

    float footstep_timer;

    // New NPC system
    NPC npcs[MAX_NPCS];
    int npc_count;
    int active_npc_index;        // Which NPC the player is currently interacting with
    
    // Trail system
    TrailPoint trail_points[MAX_TRAIL_POINTS];
    int trail_count;
    float trail_activation_time;  // Time when trail was activated
    bool trail_active;
    
    /* Player sprite animation */
    int frame_counter;
    int current_frame;
    bool facing_right;
    bool is_moving;
    FacingDirection facing_dir;
    
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
    SDL_Texture *guide_idle_texture;     // Guide NPC only
    SDL_Texture *npc_portrait;

    SpawnAnimation *spawn_anim;
    bool spawn_complete;
    
    /* Dialogue system state */
    bool in_dialogue;
    bool dialogue_text_complete;
    bool dialogue_skip_requested;
    bool waiting_for_player_choice;
    bool player_speaking;
    
    DialogueScript *current_script;
    int current_line_index;
    
    /* Typewriter effect */
    int dialogue_char_index;
    float dialogue_timer;
    float dialogue_char_delay;
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

    bool play_laugh_requested;
    
    SDL_FRect dialogue_box_rect;
    bool dialogue_area_hovered;

    float total_play_time;

    bool play_battle_music_requested;
    
    /* Set by dialogue script to trigger battle after closing */
    bool battle_triggered;
    
} GameScreen;

/* Function prototypes (remain unchanged) */
GameScreen *game_screen_create(SDL_Renderer *renderer, int window_width, int window_height);
void game_screen_destroy(GameScreen *game_screen);
void game_screen_update(GameScreen *game_screen, float delta_time);
void game_screen_render(SDL_Renderer *renderer, GameScreen *game_screen);
void game_screen_handle_input(GameScreen *game_screen, SDL_KeyboardEvent *key);
void game_screen_handle_mouse(GameScreen *game_screen, SDL_MouseButtonEvent *mouse, SDL_Renderer *renderer);

/* Dialogue functions */
void dialogue_start_script(GameScreen *gs, SDL_Renderer *renderer, const char *filename);
void dialogue_advance_script(GameScreen *gs, SDL_Renderer *renderer);
void dialogue_select_choice(GameScreen *gs, int choice);
void dialogue_end(GameScreen *gs);
void dialogue_update(GameScreen *gs, float delta_time, SDL_Renderer *renderer);
void dialogue_render(SDL_Renderer *renderer, GameScreen *gs);
bool dialogue_handle_mouse(GameScreen *gs, SDL_MouseButtonEvent *mouse, SDL_Renderer *renderer);
bool dialogue_handle_key(GameScreen *gs, SDL_KeyboardEvent *key, SDL_Renderer *renderer);

#endif