#include "../include/player_hud.h"
#include <stdio.h>
#include <math.h>

void player_hud_render(SDL_Renderer *renderer, PlayerStats *stats, 
                       TTF_Font *font_bold, TTF_Font *font,
                       int window_w, int window_h) {
    if (!renderer || !stats) return;
    
    float hud_x = 20;
    float hud_y = 20;
    
    // Level badge (circle)
    float badge_radius = 25;
    SDL_SetRenderDrawColor(renderer, 50, 50, 100, 200);
    for (int y = -badge_radius; y <= badge_radius; y++) {
        for (int x = -badge_radius; x <= badge_radius; x++) {
            if (x*x + y*y <= badge_radius*badge_radius) {
                SDL_RenderPoint(renderer, hud_x + badge_radius + x, hud_y + badge_radius + y);
            }
        }
    }
    
    // Level number
    if (font_bold) {
        char level_str[8];
        snprintf(level_str, sizeof(level_str), "%d", stats->level);
        SDL_Color level_color = {255, 255, 255, 255};
        SDL_Surface *level_surf = TTF_RenderText_Blended(font_bold, level_str, 0, level_color);
        if (level_surf) {
            SDL_Texture *level_tex = SDL_CreateTextureFromSurface(renderer, level_surf);
            float lw, lh;
            SDL_GetTextureSize(level_tex, &lw, &lh);
            SDL_FRect level_rect = {
                hud_x + badge_radius - lw/2,
                hud_y + badge_radius - lh/2,
                lw, lh
            };
            SDL_RenderTexture(renderer, level_tex, NULL, &level_rect);
            SDL_DestroyTexture(level_tex);
            SDL_DestroySurface(level_surf);
        }
    }
    
    // HP Bar
    float bar_x = hud_x + badge_radius * 2 + 15;
    float bar_y = hud_y + 10;
    float bar_w = 200;
    float bar_h = 30;
    
    SDL_SetRenderDrawColor(renderer, 50, 0, 0, 200);
    SDL_FRect hp_bg = {bar_x, bar_y, bar_w, bar_h};
    SDL_RenderFillRect(renderer, &hp_bg);
    
    float hp_pct = stats->hp / stats->max_hp;
    hp_pct = fmaxf(0.0f, fminf(1.0f, hp_pct));
    
    Uint8 r = (Uint8)(255 * (1.0f - hp_pct));
    Uint8 g = (Uint8)(255 * hp_pct);
    SDL_SetRenderDrawColor(renderer, r, g, 0, 255);
    SDL_FRect hp_fill = {bar_x, bar_y, bar_w * hp_pct, bar_h};
    SDL_RenderFillRect(renderer, &hp_fill);
    
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderRect(renderer, &hp_bg);
    
    // HP text
    if (font) {
        char hp_str[32];
        snprintf(hp_str, sizeof(hp_str), "%.0f / %.0f", stats->hp, stats->max_hp);
        SDL_Color hp_color = {255, 255, 255, 255};
        SDL_Surface *hp_surf = TTF_RenderText_Blended(font, hp_str, 0, hp_color);
        if (hp_surf) {
            SDL_Texture *hp_tex = SDL_CreateTextureFromSurface(renderer, hp_surf);
            float hw, hh;
            SDL_GetTextureSize(hp_tex, &hw, &hh);
            SDL_FRect hp_text_rect = {
                bar_x + (bar_w - hw) / 2,
                bar_y + (bar_h - hh) / 2,
                hw, hh
            };
            SDL_RenderTexture(renderer, hp_tex, NULL, &hp_text_rect);
            SDL_DestroyTexture(hp_tex);
            SDL_DestroySurface(hp_surf);
        }
    }
    
    // Club icon
    if (stats->has_club) {
        float club_x = bar_x + bar_w + 20;
        float club_y = bar_y;
        float club_size = 30;
        
        SDL_SetRenderDrawColor(renderer, 139, 90, 43, 200);
        SDL_FRect club_bg = {club_x, club_y, club_size, club_size};
        SDL_RenderFillRect(renderer, &club_bg);
        
        SDL_SetRenderDrawColor(renderer, 101, 67, 33, 255);
        SDL_FRect club_handle = {club_x + club_size/2 - 5, club_y + 5, 10, club_size - 10};
        SDL_RenderFillRect(renderer, &club_handle);
        
        SDL_SetRenderDrawColor(renderer, 120, 80, 40, 255);
        SDL_FRect club_head = {club_x + 2, club_y + 2, club_size - 4, 10};
        SDL_RenderFillRect(renderer, &club_head);
        
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
        SDL_RenderRect(renderer, &club_bg);
        
        // Attack cooldown indicator (red overlay when on cooldown)
        if (stats->attack_cooldown > 0) {
            float cd_pct = stats->attack_cooldown / 0.5f; // 0.5s is max cooldown
            SDL_SetRenderDrawColor(renderer, 100, 0, 0, 150);
            SDL_FRect cd_overlay = {
                club_x, 
                club_y + club_size * (1.0f - cd_pct), 
                club_size, 
                club_size * cd_pct
            };
            SDL_RenderFillRect(renderer, &cd_overlay);
        }
    }
}