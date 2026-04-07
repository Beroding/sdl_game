#ifndef PLAYER_HUD_H
#define PLAYER_HUD_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

typedef struct {
    float hp;
    float max_hp;
    int level;
    bool has_club;
    float attack_cooldown;  // For visual feedback
} PlayerStats;

void player_hud_render(SDL_Renderer *renderer, PlayerStats *stats, 
                       TTF_Font *font_bold, TTF_Font *font,
                       int window_w, int window_h);

#endif