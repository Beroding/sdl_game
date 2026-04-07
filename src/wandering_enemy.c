#include "../include/wandering_enemy.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void wandering_enemies_init(WanderingEnemy enemies[], int *count, int map_w, int map_h) {
    *count = 3;
    
    for (int i = 0; i < *count; i++) {
        WanderingEnemy *enemy = &enemies[i];
        
        enemy->x = 500 + i * 300 + (rand() % 200);
        enemy->y = 300 + (rand() % 400);
        enemy->vx = 0;
        enemy->vy = 0;
        enemy->speed = 0.8f + (rand() % 100) / 100.0f * 0.5f;
        enemy->max_hp = 50 + (rand() % 30);
        enemy->hp = enemy->max_hp;
        enemy->level = 1 + (rand() % 3);
        enemy->state = ENEMY_STATE_PATROL;
        enemy->state_timer = 0;
        enemy->anim_timer = 0;
        enemy->current_frame = 0;
        enemy->facing_right = (rand() % 2) == 0;
        enemy->hit_flash = 0;
        enemy->dead_timer = 0;
        enemy->active = true;
        
        enemy->target_x = enemy->x + (rand() % 200 - 100);
        enemy->target_y = enemy->y + (rand() % 200 - 100);
    }
}

void wandering_enemies_update(WanderingEnemy enemies[], int count, 
                               float player_x, float player_y, bool player_in_dialogue,
                               float delta_time, int map_w, int map_h) {
    for (int i = 0; i < count; i++) {
        WanderingEnemy *enemy = &enemies[i];
        if (!enemy->active) continue;
        
        if (enemy->hit_flash > 0) {
            enemy->hit_flash -= delta_time * 5.0f;
            if (enemy->hit_flash < 0) enemy->hit_flash = 0;
        }
        
        switch (enemy->state) {
            case ENEMY_STATE_PATROL: {
                float dx = enemy->target_x - enemy->x;
                float dy = enemy->target_y - enemy->y;
                float dist = sqrtf(dx*dx + dy*dy);
                
                if (dist < 5.0f) {
                    enemy->target_x = enemy->x + (rand() % 300 - 150);
                    enemy->target_y = enemy->y + (rand() % 300 - 150);
                    enemy->target_x = fmaxf(50, fminf(map_w - 50, enemy->target_x));
                    enemy->target_y = fmaxf(50, fminf(map_h - 50, enemy->target_y));
                } else {
                    enemy->x += (dx / dist) * enemy->speed;
                    enemy->y += (dy / dist) * enemy->speed;
                    enemy->facing_right = (dx > 0);
                }
                
                enemy->anim_timer += delta_time;
                if (enemy->anim_timer >= 0.3f) {
                    enemy->anim_timer = 0;
                    enemy->current_frame = (enemy->current_frame + 1) % 2;
                }
                
                float player_dx = player_x - enemy->x;
                float player_dy = player_y - enemy->y;
                float player_dist = sqrtf(player_dx*player_dx + player_dy*player_dy);
                
                if (player_dist < 150.0f && !player_in_dialogue) {
                    enemy->state = ENEMY_STATE_CHASE;
                }
                break;
            }
            
            case ENEMY_STATE_CHASE: {
                float dx = player_x - enemy->x;
                float dy = player_y - enemy->y;
                float dist = sqrtf(dx*dx + dy*dy);
                
                if (dist > 200.0f) {
                    enemy->state = ENEMY_STATE_PATROL;
                    enemy->target_x = enemy->x;
                    enemy->target_y = enemy->y;
                } else if (dist > 10.0f) {
                    enemy->x += (dx / dist) * enemy->speed * 1.5f;
                    enemy->y += (dy / dist) * enemy->speed * 1.5f;
                    enemy->facing_right = (dx > 0);
                }
                
                enemy->anim_timer += delta_time;
                if (enemy->anim_timer >= 0.15f) {
                    enemy->anim_timer = 0;
                    enemy->current_frame = (enemy->current_frame + 1) % 2;
                }
                break;
            }
            
            case ENEMY_STATE_HIT: {
                enemy->x += enemy->vx * delta_time;
                enemy->y += enemy->vy * delta_time;
                enemy->vx *= 0.9f;
                enemy->vy *= 0.9f;
                
                enemy->state_timer -= delta_time;
                if (enemy->state_timer <= 0) {
                    if (enemy->hp <= 0) {
                        enemy->state = ENEMY_STATE_DEAD;
                        enemy->dead_timer = 3.0f;
                    } else {
                        enemy->state = ENEMY_STATE_PATROL;
                    }
                }
                break;
            }
            
            case ENEMY_STATE_DEAD:
                enemy->dead_timer -= delta_time;
                if (enemy->dead_timer <= 0) {
                    enemy->active = false;
                }
                break;
                
            default:
                break;
        }
    }
}

void wandering_enemies_render(SDL_Renderer *renderer, WanderingEnemy enemies[], int count,
                               float cam_x, float cam_y, float zoom,
                               TTF_Font *font_small) {
    for (int i = 0; i < count; i++) {
        WanderingEnemy *enemy = &enemies[i];
        if (!enemy->active) continue;
        
        float screen_x = (enemy->x * zoom) - cam_x;
        float screen_y = (enemy->y * zoom) - cam_y;
        float sprite_size = 35 * zoom;
        
        SDL_Color enemy_color = {200, 50, 50, 255};
        
        if (enemy->hit_flash > 0) {
            enemy_color.r = 255;
            enemy_color.g = 255;
            enemy_color.b = 255;
        }
        
        Uint8 alpha = 255;
        if (enemy->state == ENEMY_STATE_DEAD) {
            alpha = (Uint8)(255 * (enemy->dead_timer / 3.0f));
        }
        
        SDL_SetRenderDrawColor(renderer, enemy_color.r, enemy_color.g, enemy_color.b, alpha);
        
        SDL_FRect enemy_rect = {
            screen_x - sprite_size / 2,
            screen_y - sprite_size / 2,
            sprite_size,
            sprite_size
        };
        SDL_RenderFillRect(renderer, &enemy_rect);
        
        // HP bar
        if (enemy->state != ENEMY_STATE_DEAD) {
            float hp_pct = enemy->hp / enemy->max_hp;
            float bar_w = sprite_size;
            float bar_h = 6 * zoom;
            
            SDL_SetRenderDrawColor(renderer, 100, 0, 0, alpha);
            SDL_FRect hp_bg = {screen_x - bar_w/2, screen_y - sprite_size/2 - bar_h - 5, bar_w, bar_h};
            SDL_RenderFillRect(renderer, &hp_bg);
            
            Uint8 r = (Uint8)(255 * (1.0f - hp_pct));
            Uint8 g = (Uint8)(255 * hp_pct);
            SDL_SetRenderDrawColor(renderer, r, g, 0, alpha);
            SDL_FRect hp_fill = {screen_x - bar_w/2, screen_y - sprite_size/2 - bar_h - 5, bar_w * hp_pct, bar_h};
            SDL_RenderFillRect(renderer, &hp_fill);
            
            // Level
            if (font_small) {
                char level_str[16];
                snprintf(level_str, sizeof(level_str), "Lv.%d", enemy->level);
                SDL_Color level_color = {255, 255, 100, alpha};
                SDL_Surface *level_surf = TTF_RenderText_Blended(font_small, level_str, 0, level_color);
                if (level_surf) {
                    SDL_Texture *level_tex = SDL_CreateTextureFromSurface(renderer, level_surf);
                    float lw, lh;
                    SDL_GetTextureSize(level_tex, &lw, &lh);
                    SDL_SetTextureAlphaMod(level_tex, alpha);
                    SDL_FRect level_rect = {
                        screen_x - lw/2,
                        screen_y - sprite_size/2 - bar_h - lh - 10,
                        lw, lh
                    };
                    SDL_RenderTexture(renderer, level_tex, NULL, &level_rect);
                    SDL_DestroyTexture(level_tex);
                    SDL_DestroySurface(level_surf);
                }
            }
        }
    }
}

void player_attack(PlayerCombatStats *player, WanderingEnemy enemies[], int enemy_count) {
    if (!player || !player->has_club || player->attacking || player->attack_cooldown > 0) return;
    
    player->attacking = true;
    player->attack_timer = 0.0f;
    player->attack_cooldown = 0.5f;
    
    printf("Player attacks with club!\n");
    
    for (int i = 0; i < enemy_count; i++) {
        WanderingEnemy *enemy = &enemies[i];
        if (!enemy->active || enemy->state == ENEMY_STATE_DEAD) continue;
        
        float dx = enemy->x - player->x;
        float dy = enemy->y - player->y;
        float dist = sqrtf(dx*dx + dy*dy);
        
        bool in_front = (player->facing_right && dx > 0) || (!player->facing_right && dx < 0);
        
        if (dist < player->attack_range && in_front) {
            float damage = 15 + (player->level * 2);
            enemy->hp -= damage;
            enemy->hit_flash = 1.0f;
            enemy->state = ENEMY_STATE_HIT;
            enemy->state_timer = 0.3f;
            
            float knockback_x = (enemy->x - player->x) / dist;
            float knockback_y = (enemy->y - player->y) / dist;
            enemy->vx = knockback_x * 200.0f;
            enemy->vy = knockback_y * 200.0f;
            
            printf("Hit enemy %d for %.1f damage! HP: %.1f/%.1f\n", i, damage, enemy->hp, enemy->max_hp);
        }
    }
}