#ifndef SAVE_MENU_H
#define SAVE_MENU_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include "save_system.h"

/* 
 * The three modes share one UI because they need identical slot selection 
 * and visual presentation - only the confirm button action differs.
 */
typedef enum {
    SAVE_MENU_MODE_SAVE,    /* Writes current game state to disk */
    SAVE_MENU_MODE_LOAD,    /* Restores previous game state */
    SAVE_MENU_MODE_DELETE   /* Permanently removes save data */
} SaveMenuMode;

/*
 * This struct acts as a self-contained UI module. The callback design 
 * keeps save logic in main.c while this handles only presentation - 
 * this prevents the UI from knowing too much about game internals.
 * 
 * Assumption: The game will set up callbacks before opening the menu.
 */
typedef struct {
    bool is_open;
    SaveMenuMode mode;
    
    /*
     * selected_slot vs hovered_slot distinction:
     * - selected_slot persists after mouse leaves (for keyboard nav)
     * - hovered_slot is transient (for visual feedback only)
     * -1 means "nothing" in both cases to match array indexing
     */
    int selected_slot;      
    int hovered_slot;       /* -2 and -3 are magic values for non-slot buttons */
    
    SaveSystem *save_system; /* Borrowed pointer - don't free, owner is main.c */
    
    SDL_Texture *bg_overlay;    /* Fullscreen darken to focus attention on menu */
    SDL_Texture *panel_texture; /* Cached to avoid recreating every frame */
    TTF_Font *font;             /* For slot numbers and button text */
    TTF_Font *font_title;       /* Larger - creates visual hierarchy */
    TTF_Font *font_small;       /* For save metadata (date, play time) */
    
    /* 
     * Panel position is recalculated on window resize.
     * All button positions are derived from these base values.
     */
    float panel_x, panel_y;
    float panel_w, panel_h;
    
    /*
     * Alpha animation prevents jarring pop-in. We animate toward
     * target_alpha rather than setting immediately for smooth fade.
     */
    float anim_alpha;
    float target_alpha;
    
    /* 
     * Slot rectangles are calculated once at creation and reused.
     * Assumption: Window size doesn't change while menu is open.
     */
    SDL_FRect slot_rects[MAX_SAVE_SLOTS];
    SDL_FRect confirm_button;   /* Changes label based on mode (Save/Load/Delete) */
    SDL_FRect back_button;      /* Always "BACK", positioned consistently for muscle memory */
    
    /*
     * Delete needs confirmation because it's irreversible.
     * We pause all other interaction while this is showing.
     */
    bool show_confirm_dialog;
    bool confirm_delete;        /* true = delete confirmed, false = cancelled */
    
    /* 
     * Callbacks fire AFTER the action completes successfully.
     * The menu doesn't close automatically - caller decides based on result.
     * 
     * Why callbacks instead of return values? Because save/load is async 
     * (disk I/O) and may need to show error messages before closing.
     */
    void (*on_save_completed)(int slot);
    void (*on_load_selected)(int slot, SaveSlotData *data);
    void (*on_delete_completed)(int slot);
    void (*on_cancelled)(void);     /* Fires on Back button or ESC */
    
} SaveMenu;

/*
 * save_system is borrowed (not copied) because the menu needs live
 * updates to slot occupancy after saves/deletes within the same session.
 */
SaveMenu* save_menu_create(SDL_Renderer *renderer, int window_w, int window_h, 
                           SaveSystem *save_sys);
void save_menu_destroy(SaveMenu *menu);

/*
 * Opening immediately starts fade-in animation.
 * Mode determines which callbacks are valid to trigger.
 */
void save_menu_open(SaveMenu *menu, SaveMenuMode mode);

/* 
 * Closing is a request, not immediate - animation must complete.
 * Check is_open() to know when it's safe to free resources.
 */
void save_menu_close(SaveMenu *menu);
bool save_menu_is_open(SaveMenu *menu);

/*
 * Must be called before open() - otherwise button clicks do nothing.
 * NULL callbacks are safe (buttons just do nothing).
 */
void save_menu_set_callbacks(SaveMenu *menu,
                             void (*on_save)(int),
                             void (*on_load)(int, SaveSlotData*),
                             void (*on_delete)(int),
                             void (*on_cancel)(void));

/* 
 * Delta time should be from main loop (usually ~16ms for 60 FPS).
 * Handles fade animation and hover effects.
 */
void save_menu_update(SaveMenu *menu, float delta_time);

/* 
 * Render assumes alpha blending is enabled on renderer.
 * Menu is drawn centered regardless of window size.
 */
void save_menu_render(SDL_Renderer *renderer, SaveMenu *menu);

/*
 * Returns true if input was consumed (caller should skip other processing).
 * Handles slot selection, button clicks, and confirmation dialog.
 */
bool save_menu_handle_mouse(SaveMenu *menu, SDL_MouseButtonEvent *mouse);

/*
 * Keyboard navigation: UP/DOWN moves selection, ENTER confirms, ESC cancels.
 * Also returns true if input was consumed.
 */
bool save_menu_handle_key(SaveMenu *menu, SDL_KeyboardEvent *key);

#endif