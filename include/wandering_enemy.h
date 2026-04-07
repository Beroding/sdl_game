#ifndef WANDERING_ENEMY_H
#define WANDERING_ENEMY_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

#define MAX_WANDERING_ENEMIES 5

typedef enum {
    ENEMY_STATE_IDLE,
    ENEMY_STATE_PATROL,
    ENEMY_STATE_CHASE,
    ENEMY_STATE_HIT,
    ENEMY_STATE_DEAD
} EnemyState;

typedef struct {
    float x, y;
    float vx, vy;
    float target_x, target_y;
    float speed;
    float hp;
    float max_hp;
    int level;
    EnemyState state;
    float state_timer;
    float anim_timer;
    int current_frame;
    bool facing_right;
    float hit_flash;
    float dead_timer;
    bool active;
} WanderingEnemy;

typedef struct {
    float x, y;
    float hp;
    float max_hp;
    int level;
    bool has_club;
    float attack_range;
    bool attacking;
    float attack_timer;
    float attack_cooldown;
    bool facing_right;
} PlayerCombatStats;

void wandering_enemies_init(WanderingEnemy enemies[], int *count, int map_w, int map_h);
void wandering_enemies_update(WanderingEnemy enemies[], int count, 
                               float player_x, float player_y, bool player_in_dialogue,
                               float delta_time, int map_w, int map_h);
void wandering_enemies_render(SDL_Renderer *renderer, WanderingEnemy enemies[], int count,
                               float cam_x, float cam_y, float zoom,
                               TTF_Font *font_small);
void player_attack(PlayerCombatStats *player, WanderingEnemy enemies[], int enemy_count);

#endif