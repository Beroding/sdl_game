#ifndef PAUSE_MENU_H
#define PAUSE_MENU_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

typedef enum {
    PAUSE_BUTTON_CONTINUE,
    PAUSE_BUTTON_SAVE,
    PAUSE_BUTTON_SETTINGS,  /* Not implemented */
    PAUSE_BUTTON_EXIT,
    PAUSE_BUTTON_COUNT
} PauseButtonType;

typedef struct {
    SDL_FRect rect;
    SDL_Texture *text_texture;
    bool hovered;
    PauseButtonType type;
} PauseButton;

/* Pause menu - ESC overlay during gameplay */
typedef struct {
    bool is_open;
    bool should_close;      /* Fade-out requested */
    bool exit_to_menu;      /* Return to main menu */
    bool save_requested;    /* Open save menu */
    bool settings_requested;
    
    SDL_Texture *bg_overlay;    /* Darken game behind menu */
    SDL_Texture *panel_texture;
    
    TTF_Font *font;
    TTF_Font *font_title;
    TTF_Font *font_small;   /* Hint text */
    
    PauseButton buttons[PAUSE_BUTTON_COUNT];
    float panel_x, panel_y;
    float panel_w, panel_h;
    
    /* Fade animation */
    float anim_alpha;
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