#ifndef BATTLE_SYSTEM_H
#define BATTLE_SYSTEM_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

#define MAX_TEAM_SIZE 3
#define MAX_SKILLS 4
#define BATTLE_BG_WIDTH 1920
#define BATTLE_BG_HEIGHT 1080

typedef enum {
    BATTLE_STATE_INTRO,      // Entry animation
    BATTLE_STATE_PLAYER_TURN,  // Player selecting action
    BATTLE_STATE_ENEMY_TURN,   // Enemy AI turn
    BATTLE_STATE_ANIMATION,    // Skill animation playing
    BATTLE_STATE_VICTORY,      // Win screen
    BATTLE_STATE_DEFEAT,       // Lose screen
    BATTLE_STATE_ESCAPE        // Ran away
} BattleState;

typedef enum {
    SKILL_TYPE_ATTACK,
    SKILL_TYPE_DEFENSE,
    SKILL_TYPE_HEAL,
    SKILL_TYPE_SPECIAL
} SkillType;

typedef struct {
    char name[32];
    char description[128];
    SkillType type;
    int chakra_cost;
    int damage;
    int heal_amount;
    float animation_duration;
    SDL_Texture *icon;
} Skill;

typedef struct {
    // Stats
    char name[32];
    int max_hp;
    int current_hp;
    int max_chakra;
    int current_chakra;
    int attack;
    int defense;
    int speed;
    
    // Visuals
    SDL_Texture *idle_sprite;
    SDL_Texture *attack_sprite;
    SDL_Texture *hurt_sprite;
    SDL_Texture *portrait;
    float x, y;  // Screen position
    float scale;
    bool is_player;  // true = player team, false = enemy
    
    // Animation
    int current_frame;
    float anim_timer;
    bool is_animating;
    float target_x, target_y;  // For movement animation
    
    // Skills
    Skill skills[MAX_SKILLS];
    int skill_count;
} BattleCharacter;

typedef struct {
    // State
    BattleState state;
    int turn_count;
    float state_timer;
    
    // Teams
    BattleCharacter player_team[MAX_TEAM_SIZE];
    BattleCharacter enemy_team[MAX_TEAM_SIZE];
    int player_count;
    int enemy_count;
    int current_actor_index;  // Whose turn it is
    int last_actor;
    
    // UI
    TTF_Font *font;
    TTF_Font *font_bold;
    SDL_Texture *bg_texture;
    SDL_Texture *ui_panel;
    SDL_Texture *chakra_icon;
    SDL_Texture *hp_icon;
    
    // Skill selection
    int selected_skill;
    int selected_target;
    bool skill_menu_open;
    
    // Battle result
    bool player_won;
    int escape_chance;
    
    // Visual effects
    SDL_Texture *effect_textures[5];  // Hit, fire, etc.
    float effect_timer;
    int current_effect;
    float shake_screen;
    
} BattleSystem;

// Core functions
BattleSystem* battle_create(SDL_Renderer *renderer, int window_w, int window_h);
void battle_destroy(BattleSystem *battle);
void battle_update(BattleSystem *battle, float delta_time);
void battle_render(SDL_Renderer *renderer, BattleSystem *battle);

// Input handling
void battle_handle_mouse(BattleSystem *battle, SDL_MouseButtonEvent *mouse);
void battle_handle_key(BattleSystem *battle, SDL_KeyboardEvent *key);

// Battle flow
void battle_start(BattleSystem *battle, SDL_Renderer *renderer);
void battle_end(BattleSystem *battle);
void battle_next_turn(BattleSystem *battle);
void battle_execute_skill(BattleSystem *battle, int skill_index, int target_index);
void battle_enemy_ai(BattleSystem *battle);

// Animation
void battle_play_animation(BattleSystem *battle, const char *anim_name, float duration);
void battle_shake_screen(BattleSystem *battle, float intensity);

// Utils
bool battle_is_active(BattleSystem *battle);
bool battle_check_victory(BattleSystem *battle);

#endif