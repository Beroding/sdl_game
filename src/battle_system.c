#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "battle_system.h"

#define SKILL_MENU_X 50
#define SKILL_MENU_Y 400
#define SKILL_BUTTON_H 60
#define SKILL_BUTTON_W 300
#define TARGET_OFFSET_X 400
#define TARGET_SPACING_Y 150

// Ninja Heroes style positions
#define PLAYER_BASE_X 200
#define PLAYER_BASE_Y 500
#define ENEMY_BASE_X 1400
#define ENEMY_BASE_Y 300

static SDL_Texture* create_skill_icon(SDL_Renderer *renderer, SDL_Color color, const char *label) {
    SDL_Surface *surf = SDL_CreateSurface(64, 64, SDL_PIXELFORMAT_RGBA8888);
    if (!surf) return NULL;
    
    // Fill with color
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            Uint32 pixel = SDL_MapRGBA(SDL_GetPixelFormatDetails(surf->format), NULL, 
                                      color.r, color.g, color.b, 255);
            ((Uint32*)surf->pixels)[y * surf->w + x] = pixel;
        }
    }
    
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_DestroySurface(surf);
    return tex;
}

BattleSystem* battle_create(SDL_Renderer *renderer, int window_w, int window_h) {
    BattleSystem *battle = calloc(1, sizeof(BattleSystem));
    if (!battle) return NULL;
    
    battle->state = BATTLE_STATE_INTRO;
    battle->state_timer = 0;
    battle->turn_count = 1;
    battle->escape_chance = 30; // 30% base escape chance
    
    // Load fonts
    battle->font = TTF_OpenFont("game_assets/Roboto_Medium.ttf", 28);
    battle->font_bold = TTF_OpenFont("game_assets/Roboto_Medium.ttf", 32);
    if (battle->font_bold) TTF_SetFontStyle(battle->font_bold, TTF_STYLE_BOLD);
    
    // Create battle background (dark arena)
    SDL_Surface *bg = SDL_CreateSurface(window_w, window_h, SDL_PIXELFORMAT_RGBA8888);
    if (bg) {
        // Dark gradient background
        for (int y = 0; y < window_h; y++) {
            Uint8 darkness = 20 + (y * 30 / window_h);
            for (int x = 0; x < window_w; x++) {
                Uint32 pixel = SDL_MapRGBA(SDL_GetPixelFormatDetails(bg->format), NULL,
                                          darkness/2, darkness/3, darkness, 255);
                ((Uint32*)bg->pixels)[y * bg->w + x] = pixel;
            }
        }
        battle->bg_texture = SDL_CreateTextureFromSurface(renderer, bg);
        SDL_DestroySurface(bg);
    }
    
    // Setup Player (You - the Tarnished/Vermin Soul)
    BattleCharacter *player = &battle->player_team[0];
    strcpy(player->name, "Vermin Soul");
    player->max_hp = 1000;
    player->current_hp = 1000;
    player->max_chakra = 100;
    player->current_chakra = 100;
    player->attack = 150;
    player->defense = 80;
    player->speed = 120;
    player->x = PLAYER_BASE_X;
    player->y = PLAYER_BASE_Y;
    player->scale = 2.5f;
    player->is_player = true;
    player->skill_count = 4;
    
    // Player Skills (Ninja Heroes style)
    strcpy(player->skills[0].name, "Basic Attack");
    strcpy(player->skills[0].description, "A swift strike with your weapon");
    player->skills[0].type = SKILL_TYPE_ATTACK;
    player->skills[0].chakra_cost = 0;
    player->skills[0].damage = 100;
    player->skills[0].icon = create_skill_icon(renderer, (SDL_Color){200, 200, 200, 255}, "ATK");
    
    strcpy(player->skills[1].name, "Shadow Strike");
    strcpy(player->skills[1].description, "A powerful attack from the shadows");
    player->skills[1].type = SKILL_TYPE_ATTACK;
    player->skills[1].chakra_cost = 25;
    player->skills[1].damage = 250;
    player->skills[1].icon = create_skill_icon(renderer, (SDL_Color){100, 100, 150, 255}, "SPC");
    
    strcpy(player->skills[2].name, "Chakra Focus");
    strcpy(player->skills[2].description, "Recover chakra and boost defense");
    player->skills[2].type = SKILL_TYPE_DEFENSE;
    player->skills[2].chakra_cost = 0;
    player->skills[2].heal_amount = 30; // Chakra recovery
    player->skills[2].icon = create_skill_icon(renderer, (SDL_Color){100, 150, 100, 255}, "DEF");
    
    strcpy(player->skills[3].name, "Soul Reaper");
    strcpy(player->skills[3].description, "Ultimate technique! Massive damage");
    player->skills[3].type = SKILL_TYPE_SPECIAL;
    player->skills[3].chakra_cost = 60;
    player->skills[3].damage = 500;
    player->skills[3].icon = create_skill_icon(renderer, (SDL_Color){255, 50, 50, 255}, "ULT");
    
    // Setup Enemy (Naberius - Gate Keeper)
    BattleCharacter *enemy = &battle->enemy_team[0];
    strcpy(enemy->name, "Naberius");
    enemy->max_hp = 2500;
    enemy->current_hp = 2500;
    enemy->max_chakra = 150;
    enemy->current_chakra = 150;
    enemy->attack = 200;
    enemy->defense = 100;
    enemy->speed = 90;
    enemy->x = ENEMY_BASE_X;
    enemy->y = ENEMY_BASE_Y;
    enemy->scale = 3.0f; // Bigger boss
    enemy->is_player = false;
    enemy->skill_count = 3;
    
    // Naberius Skills
    strcpy(enemy->skills[0].name, "Crow Peck");
    enemy->skills[0].type = SKILL_TYPE_ATTACK;
    enemy->skills[0].chakra_cost = 0;
    enemy->skills[0].damage = 120;
    
    strcpy(enemy->skills[1].name, "Shadow Claw");
    enemy->skills[1].type = SKILL_TYPE_ATTACK;
    enemy->skills[1].chakra_cost = 30;
    enemy->skills[1].damage = 280;
    
    strcpy(enemy->skills[2].name, "Gate of Despair");
    enemy->skills[2].type = SKILL_TYPE_SPECIAL;
    enemy->skills[2].chakra_cost = 80;
    enemy->skills[2].damage = 450;
    
    battle->player_count = 1;
    battle->enemy_count = 1;
    battle->current_actor_index = 0;
    
    // Determine first turn by speed
    if (player->speed >= enemy->speed) {
        battle->state = BATTLE_STATE_PLAYER_TURN;
    } else {
        battle->state = BATTLE_STATE_ENEMY_TURN;
    }
    
    printf("Battle created: %s vs %s\n", player->name, enemy->name);
    return battle;
}

void battle_destroy(BattleSystem *battle) {
    if (!battle) return;
    
    // Clean up textures
    if (battle->font) TTF_CloseFont(battle->font);
    if (battle->font_bold) TTF_CloseFont(battle->font_bold);
    if (battle->bg_texture) SDL_DestroyTexture(battle->bg_texture);
    
    for (int i = 0; i < battle->player_team[0].skill_count; i++) {
        if (battle->player_team[0].skills[i].icon) {
            SDL_DestroyTexture(battle->player_team[0].skills[i].icon);
        }
    }
    
    free(battle);
    printf("Battle destroyed\n");
}

void battle_update(BattleSystem *battle, float delta_time) {
    if (!battle) return;
    
    battle->state_timer += delta_time;
    
    // Update animations
    for (int i = 0; i < battle->player_count; i++) {
        BattleCharacter *c = &battle->player_team[i];
        if (c->is_animating) {
            c->anim_timer += delta_time;
            float t = c->anim_timer / 0.3f;
            if (t >= 1.0f) {
                t = 1.0f;
                c->is_animating = false;
                // Return to base position after animation
                if (c->is_player) {
                    c->target_x = PLAYER_BASE_X;
                    c->target_y = PLAYER_BASE_Y;
                }
            }
            c->x = c->x + (c->target_x - c->x) * 0.2f;
            c->y = c->y + (c->target_y - c->y) * 0.2f;
        }
    }
    
    for (int i = 0; i < battle->enemy_count; i++) {
        BattleCharacter *c = &battle->enemy_team[i];
        if (c->is_animating) {
            c->anim_timer += delta_time;
            float t = c->anim_timer / 0.3f;
            if (t >= 1.0f) {
                t = 1.0f;
                c->is_animating = false;
                // Return to base position
                c->target_x = ENEMY_BASE_X;
                c->target_y = ENEMY_BASE_Y;
            }
            c->x = c->x + (c->target_x - c->x) * 0.2f;
            c->y = c->y + (c->target_y - c->y) * 0.2f;
        }
    }
    
    // Screen shake decay
    if (battle->shake_screen > 0) {
        battle->shake_screen *= 0.9f;
        if (battle->shake_screen < 0.5f) battle->shake_screen = 0;
    }
    
    // State machine
    switch (battle->state) {
        case BATTLE_STATE_INTRO:
            if (battle->state_timer > 2.0f) {
                // Determine first turn by speed
                if (battle->player_team[0].speed >= battle->enemy_team[0].speed) {
                    battle->state = BATTLE_STATE_PLAYER_TURN;
                    battle->current_actor_index = 0;
                } else {
                    battle->state = BATTLE_STATE_ENEMY_TURN;
                    battle->current_actor_index = 1;
                }
                battle->state_timer = 0;
                battle->last_actor = -1; // No one has acted yet
            }
            break;
            
        case BATTLE_STATE_PLAYER_TURN:
            // Wait for player input (handled by mouse/keyboard)
            // No automatic timer here - player must choose
            break;
            
        case BATTLE_STATE_ENEMY_TURN:
            // Enemy AI acts after delay
            if (battle->state_timer > 1.5f) {
                battle_enemy_ai(battle);
            }
            break;
            
        case BATTLE_STATE_ANIMATION:
            // Wait for animation to complete
            if (battle->state_timer > 1.5f) {
                battle->state_timer = 0;
                
                // Check win/lose before switching turns
                if (battle->enemy_team[0].current_hp <= 0) {
                    battle->state = BATTLE_STATE_VICTORY;
                    battle->state_timer = 0;
                }
                else if (battle->player_team[0].current_hp <= 0) {
                    battle->state = BATTLE_STATE_DEFEAT;
                    battle->state_timer = 0;
                }
                else {
                    // Switch turns based on who just acted
                    battle_next_turn(battle);
                }
            }
            break;
            
        case BATTLE_STATE_VICTORY:
            if (battle->state_timer > 3.0f) {
                battle_end(battle);
            }
            break;
            
        case BATTLE_STATE_DEFEAT:
            if (battle->state_timer > 3.0f) {
                battle_end(battle);
            }
            break;
            
        case BATTLE_STATE_ESCAPE:
            battle_end(battle);
            break;
            
        default:
            break;
    }
}

void battle_render(SDL_Renderer *renderer, BattleSystem *battle) {
    if (!battle || !renderer) return;
    
    int window_w, window_h;
    SDL_GetWindowSize(SDL_GetWindowFromID(1), &window_w, &window_h); // Get from current window
    
    // Screen shake offset
    float shake_x = 0, shake_y = 0;
    if (battle->shake_screen > 0) {
        shake_x = (rand() % 100 / 100.0f - 0.5f) * battle->shake_screen;
        shake_y = (rand() % 100 / 100.0f - 0.5f) * battle->shake_screen;
    }
    
    // Background
    if (battle->bg_texture) {
        SDL_FRect bg_rect = {shake_x, shake_y, window_w, window_h};
        SDL_RenderTexture(renderer, battle->bg_texture, NULL, &bg_rect);
    }
    
    // Draw VS text in center
    if (battle->state == BATTLE_STATE_INTRO) {
        SDL_Color vs_color = {255, 200, 100, 255};
        SDL_Surface *vs_surf = TTF_RenderText_Blended(battle->font_bold, "VS", 0, vs_color);
        if (vs_surf) {
            SDL_Texture *vs_tex = SDL_CreateTextureFromSurface(renderer, vs_surf);
            float vs_w, vs_h;
            SDL_GetTextureSize(vs_tex, &vs_w, &vs_h);
            float scale = 1.0f + sinf(battle->state_timer * 3.0f) * 0.2f;
            SDL_FRect vs_rect = {
                (window_w - vs_w * scale) / 2 + shake_x,
                (window_h - vs_h * scale) / 2 + shake_y,
                vs_w * scale,
                vs_h * scale
            };
            SDL_RenderTexture(renderer, vs_tex, NULL, &vs_rect);
            SDL_DestroyTexture(vs_tex);
            SDL_DestroySurface(vs_surf);
        }
    }
    
    // Draw Player Team (Left side)
    for (int i = 0; i < battle->player_count; i++) {
        BattleCharacter *c = &battle->player_team[i];
        
        // Shadow
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100);
        SDL_FRect shadow = {c->x + shake_x - 50, c->y + shake_y + 200, 200, 40};
        SDL_RenderFillRect(renderer, &shadow);
        
        // Character placeholder (colored rectangle for now)
        SDL_Color player_color = {100, 150, 255, 255};
        SDL_SetRenderDrawColor(renderer, player_color.r, player_color.g, player_color.b, 255);
        SDL_FRect char_rect = {c->x + shake_x, c->y + shake_y, 150 * c->scale, 200 * c->scale};
        SDL_RenderFillRect(renderer, &char_rect);
        
        // Name
        SDL_Surface *name_surf = TTF_RenderText_Blended(battle->font_bold, c->name, 0, player_color);
        if (name_surf) {
            SDL_Texture *name_tex = SDL_CreateTextureFromSurface(renderer, name_surf);
            float name_w, name_h;
            SDL_GetTextureSize(name_tex, &name_w, &name_h);
            SDL_FRect name_rect = {c->x + shake_x, c->y + shake_y - 40, name_w, name_h};
            SDL_RenderTexture(renderer, name_tex, NULL, &name_rect);
            SDL_DestroyTexture(name_tex);
            SDL_DestroySurface(name_surf);
        }
        
        // HP Bar
        float hp_pct = (float)c->current_hp / c->max_hp;
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_FRect hp_bg = {c->x + shake_x, c->y + shake_y - 70, 150, 20};
        SDL_RenderFillRect(renderer, &hp_bg);
        
        SDL_Color hp_color = hp_pct > 0.5f ? (SDL_Color){0, 255, 0, 255} : 
                            hp_pct > 0.25f ? (SDL_Color){255, 255, 0, 255} : 
                            (SDL_Color){255, 0, 0, 255};
        SDL_SetRenderDrawColor(renderer, hp_color.r, hp_color.g, hp_color.b, 255);
        SDL_FRect hp_fill = {c->x + shake_x, c->y + shake_y - 70, 150 * hp_pct, 20};
        SDL_RenderFillRect(renderer, &hp_fill);
        
        // Chakra Bar
        float chakra_pct = (float)c->current_chakra / c->max_chakra;
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_FRect chakra_bg = {c->x + shake_x, c->y + shake_y - 45, 150, 15};
        SDL_RenderFillRect(renderer, &chakra_bg);
        
        SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
        SDL_FRect chakra_fill = {c->x + shake_x, c->y + shake_y - 45, 150 * chakra_pct, 15};
        SDL_RenderFillRect(renderer, &chakra_fill);
    }
    
    // Draw Enemy Team (Right side)
    for (int i = 0; i < battle->enemy_count; i++) {
        BattleCharacter *c = &battle->enemy_team[i];
        
        // Shadow
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100);
        SDL_FRect shadow = {c->x + shake_x - 50, c->y + shake_y + 250, 250, 50};
        SDL_RenderFillRect(renderer, &shadow);
        
        // Boss glow effect
        float glow = 0.5f + 0.5f * sinf(SDL_GetTicks() / 500.0f);
        SDL_SetRenderDrawColor(renderer, 255 * glow, 100 * glow, 100 * glow, 50);
        SDL_FRect glow_rect = {c->x + shake_x - 20, c->y + shake_y - 20, 
                               200 * c->scale + 40, 250 * c->scale + 40};
        SDL_RenderFillRect(renderer, &glow_rect);
        
        // Character (demon red)
        SDL_Color enemy_color = {255, 80, 80, 255};
        SDL_SetRenderDrawColor(renderer, enemy_color.r, enemy_color.g, enemy_color.b, 255);
        SDL_FRect char_rect = {c->x + shake_x, c->y + shake_y, 180 * c->scale, 250 * c->scale};
        SDL_RenderFillRect(renderer, &char_rect);
        
        // Name
        SDL_Surface *name_surf = TTF_RenderText_Blended(battle->font_bold, c->name, 0, enemy_color);
        if (name_surf) {
            SDL_Texture *name_tex = SDL_CreateTextureFromSurface(renderer, name_surf);
            float name_w, name_h;
            SDL_GetTextureSize(name_tex, &name_w, &name_h);
            SDL_FRect name_rect = {c->x + shake_x, c->y + shake_y - 50, name_w, name_h};
            SDL_RenderTexture(renderer, name_tex, NULL, &name_rect);
            SDL_DestroyTexture(name_tex);
            SDL_DestroySurface(name_surf);
        }
        
        // HP Bar (boss style - bigger)
        float hp_pct = (float)c->current_hp / c->max_hp;
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_FRect hp_bg = {c->x + shake_x - 50, c->y + shake_y - 90, 280, 30};
        SDL_RenderFillRect(renderer, &hp_bg);
        
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_FRect hp_fill = {c->x + shake_x - 50, c->y + shake_y - 90, 280 * hp_pct, 30};
        SDL_RenderFillRect(renderer, &hp_fill);
        
        // HP text
        char hp_text[32];
        snprintf(hp_text, sizeof(hp_text), "%d/%d", c->current_hp, c->max_hp);
        SDL_Surface *hp_surf = TTF_RenderText_Blended(battle->font, hp_text, 0, (SDL_Color){255, 255, 255, 255});
        if (hp_surf) {
            SDL_Texture *hp_tex = SDL_CreateTextureFromSurface(renderer, hp_surf);
            float hp_w, hp_h;
            SDL_GetTextureSize(hp_tex, &hp_w, &hp_h);
            SDL_FRect hp_text_rect = {c->x + shake_x + 90 - hp_w/2, c->y + shake_y - 88, hp_w, hp_h};
            SDL_RenderTexture(renderer, hp_tex, NULL, &hp_text_rect);
            SDL_DestroyTexture(hp_tex);
            SDL_DestroySurface(hp_surf);
        }
    }
    
    // Draw Skill Menu (Player Turn)
    if (battle->state == BATTLE_STATE_PLAYER_TURN) {
        BattleCharacter *player = &battle->player_team[0];
        
        // Menu background
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
        SDL_FRect menu_bg = {SKILL_MENU_X - 20, SKILL_MENU_Y - 20, SKILL_BUTTON_W + 40, 
                             player->skill_count * (SKILL_BUTTON_H + 10) + 40};
        SDL_RenderFillRect(renderer, &menu_bg);
        
        for (int i = 0; i < player->skill_count; i++) {
            Skill *skill = &player->skills[i];
            
            int btn_x = SKILL_MENU_X;
            int btn_y = SKILL_MENU_Y + i * (SKILL_BUTTON_H + 10);
            
            // Button background
            bool can_use = skill->chakra_cost <= player->current_chakra;
            SDL_Color btn_color = can_use ? 
                (i == battle->selected_skill ? (SDL_Color){255, 200, 100, 255} : (SDL_Color){80, 80, 120, 255}) :
                (SDL_Color){50, 50, 50, 255}; // Gray if can't afford
            
            SDL_SetRenderDrawColor(renderer, btn_color.r, btn_color.g, btn_color.b, 255);
            SDL_FRect btn_rect = {btn_x, btn_y, SKILL_BUTTON_W, SKILL_BUTTON_H};
            SDL_RenderFillRect(renderer, &btn_rect);
            
            // Border
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
            SDL_RenderRect(renderer, &btn_rect);
            
            // Skill icon
            if (skill->icon) {
                SDL_FRect icon_rect = {btn_x + 10, btn_y + 10, 40, 40};
                SDL_RenderTexture(renderer, skill->icon, NULL, &icon_rect);
            }
            
            // Skill name
            SDL_Surface *name_surf = TTF_RenderText_Blended(battle->font_bold, skill->name, 0, 
                                                             can_use ? (SDL_Color){255, 255, 255, 255} : 
                                                             (SDL_Color){150, 150, 150, 255});
            if (name_surf) {
                SDL_Texture *name_tex = SDL_CreateTextureFromSurface(renderer, name_surf);
                float name_w, name_h;
                SDL_GetTextureSize(name_tex, &name_w, &name_h);
                SDL_FRect name_rect = {btn_x + 60, btn_y + 10, name_w, name_h};
                SDL_RenderTexture(renderer, name_tex, NULL, &name_rect);
                SDL_DestroyTexture(name_tex);
                SDL_DestroySurface(name_surf);
            }
            
            // Chakra cost
            char chakra_text[16];
            snprintf(chakra_text, sizeof(chakra_text), "%d CP", skill->chakra_cost);
            SDL_Surface *chakra_surf = TTF_RenderText_Blended(battle->font, chakra_text, 0, 
                                                               (SDL_Color){100, 200, 255, 255});
            if (chakra_surf) {
                SDL_Texture *chakra_tex = SDL_CreateTextureFromSurface(renderer, chakra_surf);
                float chakra_w, chakra_h;
                SDL_GetTextureSize(chakra_tex, &chakra_w, &chakra_h);
                SDL_FRect chakra_rect = {btn_x + SKILL_BUTTON_W - chakra_w - 10, btn_y + 30, chakra_w, chakra_h};
                SDL_RenderTexture(renderer, chakra_tex, NULL, &chakra_rect);
                SDL_DestroyTexture(chakra_tex);
                SDL_DestroySurface(chakra_surf);
            }
        }
        
        // Turn indicator
        SDL_Surface *turn_surf = TTF_RenderText_Blended(battle->font_bold, "YOUR TURN", 0, 
                                                         (SDL_Color){100, 255, 100, 255});
        if (turn_surf) {
            SDL_Texture *turn_tex = SDL_CreateTextureFromSurface(renderer, turn_surf);
            float turn_w, turn_h;
            SDL_GetTextureSize(turn_tex, &turn_w, &turn_h);
            SDL_FRect turn_rect = {50, 50, turn_w, turn_h};
            SDL_RenderTexture(renderer, turn_tex, NULL, &turn_rect);
            SDL_DestroyTexture(turn_tex);
            SDL_DestroySurface(turn_surf);
        }
    }
    
    // Enemy turn indicator
    if (battle->state == BATTLE_STATE_ENEMY_TURN) {
        SDL_Surface *turn_surf = TTF_RenderText_Blended(battle->font_bold, "ENEMY TURN", 0, 
                                                         (SDL_Color){255, 100, 100, 255});
        if (turn_surf) {
            SDL_Texture *turn_tex = SDL_CreateTextureFromSurface(renderer, turn_surf);
            float turn_w, turn_h;
            SDL_GetTextureSize(turn_tex, &turn_w, &turn_h);
            SDL_FRect turn_rect = {window_w - turn_w - 50, 50, turn_w, turn_h};
            SDL_RenderTexture(renderer, turn_tex, NULL, &turn_rect);
            SDL_DestroyTexture(turn_tex);
            SDL_DestroySurface(turn_surf);
        }
    }
    
    // Victory/Defeat overlay
    if (battle->state == BATTLE_STATE_VICTORY || battle->state == BATTLE_STATE_DEFEAT) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
        SDL_FRect overlay = {0, 0, window_w, window_h};
        SDL_RenderFillRect(renderer, &overlay);
        
        const char *result_text = (battle->state == BATTLE_STATE_VICTORY) ? "VICTORY!" : "DEFEAT...";
        SDL_Color result_color = (battle->state == BATTLE_STATE_VICTORY) ? 
            (SDL_Color){255, 215, 0, 255} : (SDL_Color){255, 50, 50, 255};
        
        SDL_Surface *result_surf = TTF_RenderText_Blended(battle->font_bold, result_text, 0, result_color);
        if (result_surf) {
            SDL_Texture *result_tex = SDL_CreateTextureFromSurface(renderer, result_surf);
            float result_w, result_h;
            SDL_GetTextureSize(result_tex, &result_w, &result_h);
            float scale = 2.0f;
            SDL_FRect result_rect = {
                (window_w - result_w * scale) / 2,
                (window_h - result_h * scale) / 2,
                result_w * scale,
                result_h * scale
            };
            SDL_RenderTexture(renderer, result_tex, NULL, &result_rect);
            SDL_DestroyTexture(result_tex);
            SDL_DestroySurface(result_surf);
        }
    }
}

void battle_handle_mouse(BattleSystem *battle, SDL_MouseButtonEvent *mouse) {
    if (!battle || battle->state != BATTLE_STATE_PLAYER_TURN) return;
    
    if (mouse->type == SDL_EVENT_MOUSE_BUTTON_DOWN && mouse->button == SDL_BUTTON_LEFT) {
        float mx = mouse->x;
        float my = mouse->y;
        
        // Check skill buttons
        BattleCharacter *player = &battle->player_team[0];
        for (int i = 0; i < player->skill_count; i++) {
            int btn_x = SKILL_MENU_X;
            int btn_y = SKILL_MENU_Y + i * (SKILL_BUTTON_H + 10);
            
            if (mx >= btn_x && mx <= btn_x + SKILL_BUTTON_W &&
                my >= btn_y && my <= btn_y + SKILL_BUTTON_H) {
                
                Skill *skill = &player->skills[i];
                if (skill->chakra_cost <= player->current_chakra) {
                    battle->selected_skill = i;
                    battle_execute_skill(battle, i, 0); // Target first enemy
                }
                return;
            }
        }
    }
}

void battle_handle_key(BattleSystem *battle, SDL_KeyboardEvent *key) {
    if (!battle || battle->state != BATTLE_STATE_PLAYER_TURN) return;
    
    if (key->type == SDL_EVENT_KEY_DOWN) {
        BattleCharacter *player = &battle->player_team[0];
        
        switch (key->scancode) {
            case SDL_SCANCODE_1:
            case SDL_SCANCODE_KP_1:
                if (player->skills[0].chakra_cost <= player->current_chakra) {
                    battle_execute_skill(battle, 0, 0);
                }
                break;
            case SDL_SCANCODE_2:
            case SDL_SCANCODE_KP_2:
                if (player->skills[1].chakra_cost <= player->current_chakra) {
                    battle_execute_skill(battle, 1, 0);
                }
                break;
            case SDL_SCANCODE_3:
            case SDL_SCANCODE_KP_3:
                if (player->skills[2].chakra_cost <= player->current_chakra) {
                    battle_execute_skill(battle, 2, 0);
                }
                break;
            case SDL_SCANCODE_4:
            case SDL_SCANCODE_KP_4:
                if (player->skills[3].chakra_cost <= player->current_chakra) {
                    battle_execute_skill(battle, 3, 0);
                }
                break;
            case SDL_SCANCODE_ESCAPE:
                // Try to escape
                if ((rand() % 100) < battle->escape_chance) {
                    battle->state = BATTLE_STATE_ESCAPE;
                } else {
                    printf("Failed to escape!\n");
                    battle_next_turn(battle);
                }
                break;
        }
    }
}

void battle_execute_skill(BattleSystem *battle, int skill_index, int target_index) {
    if (!battle) return;
    
    BattleCharacter *actor = (battle->state == BATTLE_STATE_PLAYER_TURN) ? 
        &battle->player_team[0] : &battle->enemy_team[0];
    BattleCharacter *target = (battle->state == BATTLE_STATE_PLAYER_TURN) ? 
        &battle->enemy_team[target_index] : &battle->player_team[target_index];
    
    Skill *skill = &actor->skills[skill_index];
    
    // Track who acted
    battle->last_actor = actor->is_player ? 0 : 1;
    
    // Deduct chakra
    actor->current_chakra -= skill->chakra_cost;
    if (actor->current_chakra < 0) actor->current_chakra = 0;
    
    // Animate actor moving toward target
    actor->is_animating = true;
    actor->anim_timer = 0;
    // Move toward target
    actor->target_x = (actor->x + target->x) / 2; // Halfway to target
    actor->target_y = (actor->y + target->y) / 2;
    
    // Calculate damage
    int damage = skill->damage;
    if (skill->type == SKILL_TYPE_ATTACK || skill->type == SKILL_TYPE_SPECIAL) {
        damage = damage * actor->attack / target->defense;
        damage = damage * (90 + rand() % 20) / 100; // Random variance 90-110%
        
        target->current_hp -= damage;
        if (target->current_hp < 0) target->current_hp = 0;
        
        printf("%s uses %s on %s for %d damage!\n", actor->name, skill->name, target->name, damage);
        
        // Screen shake for impact
        battle_shake_screen(battle, skill->type == SKILL_TYPE_SPECIAL ? 20.0f : 10.0f);
    }
    else if (skill->type == SKILL_TYPE_HEAL) {
        actor->current_hp += skill->heal_amount;
        if (actor->current_hp > actor->max_hp) actor->current_hp = actor->max_hp;
        printf("%s heals for %d HP!\n", actor->name, skill->heal_amount);
    }
    else if (skill->type == SKILL_TYPE_DEFENSE) {
        actor->current_chakra += skill->heal_amount;
        if (actor->current_chakra > actor->max_chakra) actor->current_chakra = actor->max_chakra;
        printf("%s recovers %d chakra!\n", actor->name, skill->heal_amount);
    }
    
    // Move to animation state
    battle->state = BATTLE_STATE_ANIMATION;
    battle->state_timer = 0;
}

void battle_enemy_ai(BattleSystem *battle) {
    if (!battle || battle->state_timer < 1.5f) return;
    
    BattleCharacter *enemy = &battle->enemy_team[0];
    BattleCharacter *player = &battle->player_team[0];
    
    // Simple AI: Use special if enough chakra and HP is low, otherwise attack
    int skill_choice = 0;
    
    if (enemy->current_chakra >= 80 && (float)enemy->current_hp / enemy->max_hp < 0.5f) {
        skill_choice = 2; // Gate of Despair (ultimate)
    }
    else if (enemy->current_chakra >= 30 && rand() % 100 < 40) {
        skill_choice = 1; // Shadow Claw
    }
    else {
        skill_choice = 0; // Basic attack
    }
    
    battle_execute_skill(battle, skill_choice, 0);
}

void battle_next_turn(BattleSystem *battle) {
    if (!battle) return;
    
    battle->turn_count++;
    
    // Chakra regeneration each turn
    for (int i = 0; i < battle->player_count; i++) {
        BattleCharacter *c = &battle->player_team[i];
        c->current_chakra += 5;
        if (c->current_chakra > c->max_chakra) c->current_chakra = c->max_chakra;
    }
    for (int i = 0; i < battle->enemy_count; i++) {
        BattleCharacter *c = &battle->enemy_team[i];
        c->current_chakra += 5;
        if (c->current_chakra > c->max_chakra) c->current_chakra = c->max_chakra;
    }
    
    // Switch turns based on who just acted
    if (battle->last_actor == 0) {
        // Player just acted, now enemy's turn (if alive)
        if (battle->enemy_team[0].current_hp > 0) {
            battle->state = BATTLE_STATE_ENEMY_TURN;
            battle->current_actor_index = 1;
            battle->state_timer = 0;
        }
    } else {
        // Enemy just acted, now player's turn (if alive)
        if (battle->player_team[0].current_hp > 0) {
            battle->state = BATTLE_STATE_PLAYER_TURN;
            battle->current_actor_index = 0;
            battle->state_timer = 0;
        }
    }
    
    printf("Turn %d - %s's turn\n", battle->turn_count, 
           battle->state == BATTLE_STATE_PLAYER_TURN ? "PLAYER" : "ENEMY");
}

void battle_shake_screen(BattleSystem *battle, float intensity) {
    if (!battle) return;
    battle->shake_screen = intensity;
}

void battle_end(BattleSystem *battle) {
    if (!battle) return;
    printf("Battle ended! Result: %s\n", 
           battle->state == BATTLE_STATE_VICTORY ? "VICTORY" : 
           battle->state == BATTLE_STATE_DEFEAT ? "DEFEAT" : "ESCAPE");
    // Return to game will be handled by main
}

bool battle_is_active(BattleSystem *battle) {
    return battle && (battle->state != BATTLE_STATE_VICTORY && 
                      battle->state != BATTLE_STATE_DEFEAT &&
                      battle->state != BATTLE_STATE_ESCAPE);
}