#ifndef GAME_SCREEN_H
#define GAME_SCREEN_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "dialogue_loader.h"
#include "collision_system.h"
#include "spawn_animation.h"
#include "club_notification.h"    // ADD THIS
#include "wandering_enemy.h"       // ADD THIS
#include "player_hud.h"            // ADD THIS

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

/* NPC structure */
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
    float patrol_min_x, patrol_max_x;
    float patrol_min_y, patrol_max_y;
} NPC;

/* TrailPoint structure */
typedef struct {
    float x, y;
    float lifetime;
    bool active;
} TrailPoint;

/* Main gameplay state */
typedef struct {
    /* World positions */
    float player_x;
    float player_y;
    float npc_x;
    float npc_y;

    CollisionWorld *collision;

    /* Progress tracking */
    int score;
    int level;

    /* Animation timing */
    int npc_frame;
    float npc_frame_timer;
    float npc_frame_duration;
    
    int map_width;
    int map_height;
    
    /* Input state */
    bool moving_left;
    bool moving_right;
    bool moving_up;
    bool moving_down;

    bool is_running;
    float move_speed;  

    float footstep_timer;

    // NPC system
    NPC npcs[MAX_NPCS];
    int npc_count;
    int active_npc_index;
    
    // Trail system
    TrailPoint trail_points[MAX_TRAIL_POINTS];
    int trail_count;
    float trail_activation_time;
    bool trail_active;
    
    /* Player sprite animation */
    int frame_counter;
    int current_frame;
    bool facing_right;
    bool is_moving;
    FacingDirection facing_dir;
    
    bool npc_facing_right;
    bool player_near_npc;

    Camera camera;
    
    TTF_Font *font;
    TTF_Font *font_bold;
    TTF_Font *font_dialogue;
    TTF_Font *font_small;
    
    SDL_Renderer *renderer;

    SDL_Texture *bg_texture;
    SDL_Texture *idle_texture;
    SDL_Texture *run_texture;
    SDL_Texture *title_text;
    SDL_Texture *npc_idle_texture;
    SDL_Texture *guide_idle_texture;
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
    
    bool knight_first_talk_done;
    
    /* Club and combat system */
    bool has_club;
    float club_notify_timer;
    
    /* Enhanced club notification */
    ClubNotification club_notify;
    
    /* Wandering enemies */
    WanderingEnemy wandering_enemies[MAX_WANDERING_ENEMIES];
    int wandering_enemy_count;
    
    /* Player combat stats (also used by HUD) */
    float player_hp;
    float player_max_hp;
    int player_level;
    
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
    
    /* NPC animation during dialogue */
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

/* Function prototypes */
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