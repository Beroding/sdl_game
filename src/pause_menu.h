#ifndef PAUSE_MENU_H
#define PAUSE_MENU_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

typedef enum {
    PAUSE_BUTTON_CONTINUE,
    PAUSE_BUTTON_SAVE,
    PAUSE_BUTTON_SETTINGS,
    PAUSE_BUTTON_EXIT,
    PAUSE_BUTTON_COUNT
} PauseButtonType;

typedef struct {
    SDL_FRect rect;
    SDL_Texture *text_texture;
    SDL_Texture *hover_texture;
    bool hovered;
    PauseButtonType type;
} PauseButton;

typedef struct {
    bool is_open;
    bool should_close;
    bool exit_to_menu;
    bool save_requested;
    bool settings_requested;
    
    SDL_Texture *bg_overlay;
    SDL_Texture *panel_texture;
    
    // Fonts - ONLY DECLARE ONCE
    TTF_Font *font;
    TTF_Font *font_title;
    TTF_Font *font_small;  // Small font for hints
    
    PauseButton buttons[PAUSE_BUTTON_COUNT];
    float panel_x, panel_y;
    float panel_w, panel_h;
    
    float anim_alpha;      // For fade-in animation
    float target_alpha;
    
} PauseMenu;

PauseMenu* pause_menu_create(SDL_Renderer *renderer, int window_w, int window_h);
void pause_menu_destroy(PauseMenu *menu);
void pause_menu_update(PauseMenu *menu, float delta_time);
void pause_menu_render(SDL_Renderer *renderer, PauseMenu *menu);
bool pause_menu_handle_mouse(PauseMenu *menu, SDL_MouseButtonEvent *mouse);
bool pause_menu_handle_key(PauseMenu *menu, SDL_KeyboardEvent *key);
void pause_menu_open(PauseMenu *menu);
void pause_menu_close(PauseMenu *menu);
bool pause_menu_is_open(PauseMenu *menu);

#endif