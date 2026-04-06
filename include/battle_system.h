#ifndef BATTLE_SYSTEM_H
#define BATTLE_SYSTEM_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

/*
 * These limits are arbitrary but sufficient for a simple RPG.
 * MAX_TEAM_SIZE > 1 allows for future party expansion without rewriting logic.
 */
#define MAX_TEAM_SIZE 3
#define MAX_SKILLS 4

/*
 * Background dimensions are fixed at 1080p for consistent UI layout.
 * The actual window can be any size - we scale or letterbox as needed.
 */
#define BATTLE_BG_WIDTH 1920
#define BATTLE_BG_HEIGHT 1080

/*
 * State machine for battle flow. Each state has distinct input handling
 * and rendering. Transitions are mostly automatic (timed) except
 * PLAYER_TURN which waits for input.
 */
typedef enum {
    BATTLE_STATE_INTRO,         /* VS screen - builds tension, 2 second minimum */
    BATTLE_STATE_PLAYER_TURN,   /* Blocking - waits for skill selection */
    BATTLE_STATE_ENEMY_TURN,    /* 1.5s delay then AI acts (lets player read) */
    BATTLE_STATE_ANIMATION,     /* Attack plays, 1.5s before result check */
    BATTLE_STATE_VICTORY,       /* 3s display then auto-return to world */
    BATTLE_STATE_DEFEAT,        /* 3s display then game over or retry */
    BATTLE_STATE_ESCAPE         /* Immediate exit, no rewards */
} BattleState;

/*
 * Skill categories determine targeting, damage calculation, and UI color.
 * HEAL and DEFENSE both use heal_amount field - we don't distinguish
 * HP recovery vs chakra recovery at this level (UI handles labeling).
 */
typedef enum {
    SKILL_TYPE_ATTACK,      /* Damages target, standard formula */
    SKILL_TYPE_DEFENSE,     /* Usually recovers chakra or buffs */
    SKILL_TYPE_HEAL,        /* Restores HP, cannot overheal */
    SKILL_TYPE_SPECIAL      /* High damage + high cost, visual flair */
} SkillType;

/*
 * Skills are data-only - no function pointers. This keeps them serializable
 * for save files and editable in data files. Damage is base value before
 * the attack/defense ratio and random variance (90-110%) are applied.
 */
typedef struct {
    char name[32];
    char description[128];  /* Shown on hover/selection */
    SkillType type;
    int chakra_cost;        /* 0 = free (basic attack) */
    int damage;             /* Base before stats and variance */
    int heal_amount;        /* For HEAL or DEFENSE types */
    float animation_duration;
    SDL_Texture *icon;      /* 64x64, generated or loaded */
} Skill;

/*
 * Characters are positioned on screen for visual effect - the x,y values
 * are in pixels and animate during attacks. We use colored rectangles
 * for now (placeholder art) but keep texture fields for future sprites.
 * 
 * target_x/y are used for lerp animation - we don't teleport characters.
 */
typedef struct {
    /* Core stats - attack/defense ratio is the damage formula key */
    char name[32];
    int max_hp;
    int current_hp;
    int max_chakra;
    int current_chakra;
    int attack;         /* Higher = more damage dealt */
    int defense;        /* Higher = less damage taken */
    int speed;          /* Determines turn order at battle start */
    
    /* Visuals - all placeholders currently using colored rectangles */
    SDL_Texture *idle_sprite;
    SDL_Texture *attack_sprite;
    SDL_Texture *hurt_sprite;
    SDL_Texture *portrait;
    float x, y;         /* Current screen position (animates) */
    float scale;        /* Size multiplier - bosses are bigger */
    bool is_player;     /* Affects AI targeting and UI layout */
    
    /* Animation state - 0.3 second lerp for all movements */
    int current_frame;
    float anim_timer;
    bool is_animating;
    float target_x, target_y;
    
    /* Skills - skill_count may be less than MAX_SKILLS for enemies */
    Skill skills[MAX_SKILLS];
    int skill_count;
} BattleCharacter;

/*
 * Main battle controller. We track last_actor to know whose turn comes next
 * without recalculating speed (speed only matters for initial order).
 * 
 * escape_chance is flat percentage - no level or stat modifiers yet.
 * shake_screen decays exponentially (0.9x per frame) for impact feel.
 */
typedef struct {
    BattleState state;
    int turn_count;         /* Starts at 1, increments per action */
    float state_timer;      /* Seconds in current state, drives transitions */
    
    /* Teams - we support up to 3 but currently only use 1v1 */
    BattleCharacter player_team[MAX_TEAM_SIZE];
    BattleCharacter enemy_team[MAX_TEAM_SIZE];
    int player_count;
    int enemy_count;
    int current_actor_index;
    int last_actor;         /* 0 = player, 1 = enemy, -1 = none yet */
    
    /* UI resources - loaded once at battle creation */
    TTF_Font *font;
    TTF_Font *font_bold;
    SDL_Texture *bg_texture;
    SDL_Texture *ui_panel;
    SDL_Texture *chakra_icon;
    SDL_Texture *hp_icon;
    
    /* Player input state */
    int selected_skill;     /* -1 until chosen, 0-3 during execution */
    int selected_target;    /* Always 0 for now (single enemy) */
    bool skill_menu_open;   /* Could be false during target selection */
    
    /* Results - player_won is redundant with state but convenient */
    bool player_won;
    int escape_chance;      /* Base 30%, no modifiers implemented */
    
    /* Visual effects - effect_textures[0] is hit, [1] is fire, etc. */
    SDL_Texture *effect_textures[5];
    float effect_timer;
    int current_effect;
    float shake_screen;     /* Pixels of random offset, decays over time */
    
} BattleSystem;

/* 
 * Creates battle with hardcoded player "Vermin Soul" vs enemy "Naberius".
 * Speed check determines initial state (PLAYER_TURN or ENEMY_TURN).
 * Returns NULL if font loading fails (renderer error also returns NULL).
 */
BattleSystem* battle_create(SDL_Renderer *renderer, int window_w, int window_h);

/* 
 * Destroys all textures and fonts. Safe to call with NULL.
 * Does NOT free character sprites (those are managed elsewhere).
 */
void battle_destroy(BattleSystem *battle);

/*
 * Updates timers, animations, and state machine. Call at ~60 FPS.
 * delta_time is seconds (0.016 for 60 FPS). State transitions happen here.
 */
void battle_update(BattleSystem *battle, float delta_time);

/*
 * Renders entire battle scene. shake_screen is applied to all elements
 * for consistent earthquake effect during impacts.
 */
void battle_render(SDL_Renderer *renderer, BattleSystem *battle);

/* 
 * Mouse clicks on skill buttons. Only works during PLAYER_TURN.
 * Checks chakra cost before executing - insufficient chakra is ignored.
 */
void battle_handle_mouse(BattleSystem *battle, SDL_MouseButtonEvent *mouse);

/*
 * Keyboard: 1-4 for skills, ESC to attempt escape (30% chance).
 * Numpad keys also bound for accessibility.
 */
void battle_handle_key(BattleSystem *battle, SDL_KeyboardEvent *key);

/*
 * These are called by input handlers or AI. target_index is 0 for now
 * (single enemy) but parameter exists for future multi-enemy battles.
 */
void battle_execute_skill(BattleSystem *battle, int skill_index, int target_index);

/*
 * Simple AI: uses ultimate if HP < 50% and chakra >= 80,
 * medium attack 40% of time if affordable, else basic.
 * Called automatically after 1.5s delay in ENEMY_TURN state.
 */
void battle_enemy_ai(BattleSystem *battle);

/*
 * Switches turns and adds 5 chakra to both sides (prevents stalemates).
 * Checks HP to determine if defeated side can still act.
 */
void battle_next_turn(BattleSystem *battle);

/* 
 * Plays effect animation. Currently only screen shake is implemented.
 * Future: fire, ice, hit effects using effect_textures[].
 */
void battle_play_animation(BattleSystem *battle, const char *anim_name, float duration);

/* 
 * Adds intensity to shake_screen. Decay is automatic in update().
 * Special attacks should shake more than basic attacks.
 */
void battle_shake_screen(BattleSystem *battle, float intensity);

/*
 * Victory/Defeat/Escape states are considered inactive.
 * Main loop checks this to know when to destroy battle and return to world.
 */
bool battle_is_active(BattleSystem *battle);

/*
 * Convenience check - same logic as state == VICTORY but more readable.
 * Not currently used but kept for potential victory rewards screen.
 */
bool battle_check_victory(BattleSystem *battle);

void battle_end(BattleSystem *battle);

#endif