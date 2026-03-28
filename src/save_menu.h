#ifndef SAVE_MENU_H
#define SAVE_MENU_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "save_system.h"

typedef enum {
    SAVE_MENU_MODE_SAVE,    // Saving game
    SAVE_MENU_MODE_LOAD,    // Loading game
    SAVE_MENU_MODE_DELETE   // Deleting saves
} SaveMenuMode;

typedef struct {
    bool is_open;
    SaveMenuMode mode;
    int selected_slot;      // -1 if none
    int hovered_slot;
    
    SaveSystem *save_system;
    
    // Visuals
    SDL_Texture *bg_overlay;
    SDL_Texture *panel_texture;
    TTF_Font *font;
    TTF_Font *font_title;
    TTF_Font *font_small;
    
    float panel_x, panel_y;
    float panel_w, panel_h;
    float anim_alpha;
    float target_alpha;
    
    // Slot rectangles for clicking
    SDL_FRect slot_rects[MAX_SAVE_SLOTS];
    SDL_FRect confirm_button;
    SDL_FRect back_button;
    SDL_FRect delete_button;  // For delete mode
    
    bool show_confirm_dialog;
    bool confirm_delete;      // For delete confirmation
    
    // Callbacks
    void (*on_save_completed)(int slot);
    void (*on_load_selected)(int slot, SaveSlotData *data);
    void (*on_delete_completed)(int slot);
    void (*on_cancelled)(void);
    
} SaveMenu;

SaveMenu* save_menu_create(SDL_Renderer *renderer, int window_w, int window_h, 
                           SaveSystem *save_sys);
void save_menu_destroy(SaveMenu *menu);
void save_menu_open(SaveMenu *menu, SaveMenuMode mode);
void save_menu_close(SaveMenu *menu);
bool save_menu_is_open(SaveMenu *menu);

void save_menu_update(SaveMenu *menu, float delta_time);
void save_menu_render(SDL_Renderer *renderer, SaveMenu *menu);
bool save_menu_handle_mouse(SaveMenu *menu, SDL_MouseButtonEvent *mouse);
bool save_menu_handle_key(SaveMenu *menu, SDL_KeyboardEvent *key);

void save_menu_set_callbacks(SaveMenu *menu,
                             void (*on_save)(int),
                             void (*on_load)(int, SaveSlotData*),
                             void (*on_delete)(int),
                             void (*on_cancel)(void));

#endif