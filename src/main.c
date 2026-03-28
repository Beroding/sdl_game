#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "menu_screen.h"
#include "game_screen.h"
#include "battle_system.h"
#include "opening_animation.h"
#include "dialogue_loader.h"
#include "pause_menu.h"
#include "save_system.h"
#include "save_menu.h"

#define SDL_FLAGS SDL_INIT_VIDEO

#define WINDOW_TITLE "something sumting"
#define WINDOW_WIDTH 1500
#define WINDOW_HEIGHT 900

typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_DIALOGUE,
    STATE_OPENING_ANIMATION,
    STATE_BATTLE,
    STATE_CREDITS,
    STATE_SAVE_MENU  // New state for save/load menu
} GameState;

struct Game 
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    Menu *menu;
    GameScreen *game_screen;
    SDL_Event event;
    bool isRunning;
    GameState state;
    OpeningAnimation *opening;
    BattleSystem *battle;
    PauseMenu *pause_menu;
    SaveSystem *save_system;  // Save system
    SaveMenu *save_menu;       // Save menu UI
    float play_time;           // Track play time
    time_t session_start;      // When current session started
    int current_save_slot;     // Which slot we're using (-1 = none)
};

bool game_init_sdl(struct Game *g);
bool game_new(struct Game **game);
void game_free(struct Game **game);
void game_events(struct Game *g);
void game_update(struct Game *g);
void game_draw(struct Game *g);
void game_run(struct Game *g);

void start_new_game(struct Game *g);
void return_to_menu(struct Game *g);
void continue_game(struct Game *g, SaveSlotData *data);
void save_current_game(struct Game *g, int slot);
void on_save_completed(int slot);
void on_load_selected(int slot, SaveSlotData *data);
void on_delete_completed(int slot);
void on_save_cancelled(void);

// Global pointer for callbacks
struct Game *g_game = NULL;

bool game_init_sdl(struct Game *g) 
{
    if(SDL_Init(SDL_FLAGS) < 0) 
    {
        fprintf(stderr, "Error initializing SDL3: %s\n", SDL_GetError());
        return false;
    }
    printf("SDL Initialized\n");

    if (TTF_Init() == false) 
    {
        fprintf(stderr, "TTF_Init failed: %s\n", SDL_GetError());
        return false;
    }
    printf("SDL_ttf Initialized\n");

    // Get display size for fullscreen
    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(display);
    
    int window_w = mode ? mode->w : WINDOW_WIDTH;
    int window_h = mode ? mode->h : WINDOW_HEIGHT;

    // Create fullscreen window
    g->window = SDL_CreateWindow(WINDOW_TITLE, window_w, window_h, 
                                  SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS);
    if(!g->window)
    {
        fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
        return false;
    }
    printf("Window created: %dx%d fullscreen\n", window_w, window_h);
    
    // Don't allow resizing in fullscreen
    SDL_SetWindowResizable(g->window, false);

    g->renderer = SDL_CreateRenderer(g->window, NULL);
    if(!g->renderer) 
    {
        printf("Error creating renderer: %s\n", SDL_GetError());
        return false;
    }
    printf("Renderer created\n");

    // Create menu with actual screen size
    g->menu = menu_create(g->renderer, window_w, window_h);
    if (!g->menu)
    {
        printf("menu_create failed\n");
        return false;
    }
    printf("Menu created\n");

    // Create save system
    g->save_system = save_system_create();
    if (!g->save_system)
    {
        printf("save_system_create failed\n");
        return false;
    }
    printf("Save system created\n");

    // Create save menu
    g->save_menu = save_menu_create(g->renderer, window_w, window_h, g->save_system);
    if (!g->save_menu)
    {
        printf("save_menu_create failed\n");
        return false;
    }
    printf("Save menu created\n");

    // Set up callbacks
    save_menu_set_callbacks(g->save_menu, 
                           on_save_completed, 
                           on_load_selected, 
                           on_delete_completed, 
                           on_save_cancelled);

    g->game_screen = NULL;
    g->battle = NULL;
    g->pause_menu = NULL;
    g->play_time = 0;
    g->current_save_slot = -1;

    return true;
}

bool game_new(struct Game **game)
{
    *game = calloc(1, sizeof(struct Game));
    if(*game == NULL)
    {
        fprintf(stderr, "Error allocating memory for game struct\n");
        return false;
    }
    printf("Allocated game struct\n");
    struct Game *g = *game;
    g_game = g;  // Set global for callbacks

    if(!game_init_sdl(g))
    {
        free(g);
        *game = NULL;
        return false;
    }
    printf("game_init_sdl SUCCESS\n");

    g->isRunning = true;
    g->state = STATE_MENU;
    g->session_start = time(NULL);

    return true;
}

void game_free(struct Game **game) 
{    
    if(*game)
    {
        struct Game *g = *game;

        menu_destroy(g->menu);
        game_screen_destroy(g->game_screen);
        opening_destroy(g->opening);
        battle_destroy(g->battle);
        pause_menu_destroy(g->pause_menu);
        save_menu_destroy(g->save_menu);
        save_system_destroy(g->save_system);

        if(g->renderer) 
        {
            SDL_DestroyRenderer(g->renderer);
            g->renderer = NULL;
        }

        if(g->window) 
        {
            SDL_DestroyWindow(g->window);
            g->window = NULL;
        }

        TTF_Quit();
        SDL_Quit();

        free(g);
        g = NULL;
        *game = NULL;
        g_game = NULL;

        printf("all clean\n");
    }
}

void start_new_game(struct Game *g)
{
    printf("Starting new game...\n");
    
    game_screen_destroy(g->game_screen);
    g->game_screen = NULL;
    
    // Get actual screen size
    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(display);
    int w = mode ? mode->w : WINDOW_WIDTH;
    int h = mode ? mode->h : WINDOW_HEIGHT;
    
    g->game_screen = game_screen_create(g->renderer, w, h);
    if (!g->game_screen) {
        fprintf(stderr, "Failed to create game screen\n");
        return;
    }
    
    // Create pause menu for this game session
    pause_menu_destroy(g->pause_menu);
    g->pause_menu = pause_menu_create(g->renderer, w, h);
    
    g->play_time = 0;
    g->current_save_slot = -1;  // New game, no save slot yet
    g->session_start = time(NULL);
    
    g->state = STATE_PLAYING;
}

void continue_game(struct Game *g, SaveSlotData *data)
{
    printf("Continuing game from save...\n");
    
    game_screen_destroy(g->game_screen);
    g->game_screen = NULL;
    
    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(display);
    int w = mode ? mode->w : WINDOW_WIDTH;
    int h = mode ? mode->h : WINDOW_HEIGHT;
    
    g->game_screen = game_screen_create(g->renderer, w, h);
    if (!g->game_screen) {
        fprintf(stderr, "Failed to create game screen\n");
        return;
    }
    
    // Restore game state from save
    g->game_screen->level = data->current_level;
    g->game_screen->score = data->player_score;
    g->game_screen->player_x = data->player_x;
    g->game_screen->player_y = data->player_y;
    
    // Restore dialogue progress if any
    if (strlen(data->current_dialogue_file) > 0 && data->current_dialogue_line >= 0) {
        // Optionally resume dialogue - for now just mark as met
        g->game_screen->player_near_npc = false;  // Will be recalculated
    }
    
    // Create pause menu
    pause_menu_destroy(g->pause_menu);
    g->pause_menu = pause_menu_create(g->renderer, w, h);
    
    g->play_time = data->play_time;
    g->session_start = time(NULL);
    
    g->state = STATE_PLAYING;
}

void save_current_game(struct Game *g, int slot)
{
    if (!g || !g->game_screen) return;
    
    // Calculate current play time
    time_t now = time(NULL);
    float session_time = difftime(now, g->session_start);
    float total_time = g->play_time + session_time;
    
    bool result = save_game(g->save_system, slot,
                           "Vermin Soul",                    // player name
                           g->game_screen->level,            // level
                           g->game_screen->score,            // score
                           g->game_screen->player_x,         // x position
                           g->game_screen->player_y,         // y position
                           "",                               // current dialogue file (optional)
                           -1,                               // current dialogue line
                           1000,                             // max HP (from battle system)
                           1000,                             // current HP
                           100,                              // max chakra
                           100,                              // current chakra
                           true,                             // has met naberius
                           false,                            // defeated naberius (update this)
                           false,                            // tutorial completed
                           total_time);                      // play time
    
    if (result) {
        g->current_save_slot = slot;
        g->play_time = total_time;  // Update stored play time
        g->session_start = time(NULL);  // Reset session timer
        printf("Game saved successfully to slot %d\n", slot);
    } else {
        printf("Failed to save game!\n");
    }
}

// Callbacks for save menu
void on_save_completed(int slot)
{
    if (g_game) {
        save_current_game(g_game, slot);
    }
}

void on_load_selected(int slot, SaveSlotData *data)
{
    if (g_game) {
        g_game->current_save_slot = slot;
        continue_game(g_game, data);
    }
}

void on_delete_completed(int slot)
{
    printf("Save slot %d deleted\n", slot);
    if (g_game && g_game->current_save_slot == slot) {
        g_game->current_save_slot = -1;
    }
}

void on_save_cancelled(void)
{
    printf("Save/Load operation cancelled\n");
}

void return_to_menu(struct Game *g)
{
    printf("Returning to menu...\n");
    g->state = STATE_MENU;
    if (g->pause_menu) {
        pause_menu_close(g->pause_menu);
    }
    if (g->save_menu) {
        save_menu_close(g->save_menu);
    }
}

void game_events(struct Game *g)
{
    while (SDL_PollEvent(&g->event)) 
    {
        // Handle save menu first if open
        if (g->state == STATE_SAVE_MENU && save_menu_is_open(g->save_menu)) {
            switch (g->event.type) {
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    save_menu_handle_mouse(g->save_menu, &g->event.button);
                    break;
                case SDL_EVENT_KEY_DOWN:
                    save_menu_handle_key(g->save_menu, &g->event.key);
                    break;
            }
            continue; // Block other events
        }
        
        // Handle pause menu if open
        if (g->pause_menu && pause_menu_is_open(g->pause_menu)) {
            switch (g->event.type) {
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (pause_menu_handle_mouse(g->pause_menu, &g->event.button)) {
                        // Check if save requested
                        if (g->pause_menu->save_requested) {
                            g->pause_menu->save_requested = false;
                            g->state = STATE_SAVE_MENU;
                            save_menu_open(g->save_menu, SAVE_MENU_MODE_SAVE);
                        }
                        // Check if exit was requested
                        else if (g->pause_menu->exit_to_menu) {
                            g->pause_menu->exit_to_menu = false;
                            return_to_menu(g);
                        }
                        continue;
                    }
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (pause_menu_handle_key(g->pause_menu, &g->event.key)) {
                        if (g->pause_menu->exit_to_menu) {
                            g->pause_menu->exit_to_menu = false;
                            return_to_menu(g);
                        }
                        continue;
                    }
                    break;
                case SDL_EVENT_KEY_UP:
                    continue;
            }
        }
        
        switch (g->event.type) 
        {
            case SDL_EVENT_KEY_UP:
                if (g->state == STATE_PLAYING && g->game_screen && 
                    (!g->pause_menu || !pause_menu_is_open(g->pause_menu)) &&
                    (!g->save_menu || !save_menu_is_open(g->save_menu))) {
                    game_screen_handle_input(g->game_screen, &g->event.key);
                }
                break;

            case SDL_EVENT_QUIT:
                g->isRunning = false;
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (g->state == STATE_BATTLE && g->battle) {
                    battle_handle_mouse(g->battle, &g->event.button);
                }
                // Handle mouse in game screen (for dialogue)
                if (g->state == STATE_PLAYING && g->game_screen &&
                    (!g->pause_menu || !pause_menu_is_open(g->pause_menu)) &&
                    (!g->save_menu || !save_menu_is_open(g->save_menu))) {
                    game_screen_handle_mouse(g->game_screen, &g->event.button, g->renderer);
                }
                // Handle menu clicks
                else if (g->state == STATE_MENU) {
                    int index = menu_handle_click(g->menu, g->event.button.x, g->event.button.y);
                    if (index >= 0) {
                        printf("menu click index=%d\n", index);
                        switch (index) {
                            case 0: /* Continue - check for saves */
                                if (g->save_system) {
                                    bool has_saves = false;
                                    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
                                        if (has_save_in_slot(g->save_system, i)) {
                                            has_saves = true;
                                            break;
                                        }
                                    }
                                    if (has_saves) {
                                        g->state = STATE_SAVE_MENU;
                                        save_menu_open(g->save_menu, SAVE_MENU_MODE_LOAD);
                                    } else {
                                        printf("No saves found, starting new game\n");
                                        start_new_game(g);
                                    }
                                }
                                break;
                            case 1: start_new_game(g); break;
                            case 2: g->state = STATE_CREDITS; break;
                            case 3: g->isRunning = false; break;
                        }
                    }
                }
                break;

            case SDL_EVENT_KEY_DOWN:
                if (g->state == STATE_OPENING_ANIMATION && g->opening) {
                    if (g->event.key.scancode == SDL_SCANCODE_ESCAPE || 
                        g->event.key.scancode == SDL_SCANCODE_SPACE) {
                        g->opening->phase = OPENING_PHASE_COMPLETE;
                        g->opening->complete = true;
                    }
                    break;
                }
                if (g->state == STATE_BATTLE && g->battle) {
                    battle_handle_key(g->battle, &g->event.key);
                }
                
                // ESC to toggle pause menu during gameplay
                if (g->state == STATE_PLAYING && g->game_screen) {
                    if (g->event.key.scancode == SDL_SCANCODE_ESCAPE) {
                        // Check if in dialogue first
                        if (g->game_screen && !g->game_screen->in_dialogue) {
                            if (g->pause_menu) {
                                if (pause_menu_is_open(g->pause_menu)) {
                                    pause_menu_close(g->pause_menu);
                                } else {
                                    pause_menu_open(g->pause_menu);
                                }
                            }
                        }
                        break;
                    } else if (!g->pause_menu || !pause_menu_is_open(g->pause_menu)) {
                        game_screen_handle_input(g->game_screen, &g->event.key);
                    }
                    break;
                }
                
                if (g->state == STATE_MENU) {
                    int index = menu_handle_key(g->menu, &g->event.key);
                    if (index >= 0) {
                        printf("menu enter index=%d\n", index);
                        switch (index) {
                            case 0: /* Continue */
                                if (g->save_system) {
                                    bool has_saves = false;
                                    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
                                        if (has_save_in_slot(g->save_system, i)) {
                                            has_saves = true;
                                            break;
                                        }
                                    }
                                    if (has_saves) {
                                        g->state = STATE_SAVE_MENU;
                                        save_menu_open(g->save_menu, SAVE_MENU_MODE_LOAD);
                                    } else {
                                        start_new_game(g);
                                    }
                                }
                                break;
                            case 1: start_new_game(g); break;
                            case 2: g->state = STATE_CREDITS; break;
                            case 3: g->isRunning = false; break;
                        }
                    }
                }
                else if (g->state == STATE_CREDITS) {
                    return_to_menu(g);
                }
                break;

            default:
                break;
        }
    }
}

void game_update(struct Game *g) {
    // Update save menu animation
    if (g->save_menu) {
        save_menu_update(g->save_menu, 0.016f);
    }
    
    // Update pause menu animation
    if (g->pause_menu) {
        pause_menu_update(g->pause_menu, 0.016f);
    }
    
    switch (g->state) {
        case STATE_MENU:
            break;
            
        case STATE_SAVE_MENU:
            // Just animating the menu
            if (!save_menu_is_open(g->save_menu)) {
                // Menu was closed, return to appropriate state
                if (!g->game_screen) {
                    g->state = STATE_MENU;
                } else {
                    g->state = STATE_PLAYING;
                }
            }
            break;
            
        case STATE_PLAYING:
            // Don't update game world when paused or in save menu
            if ((g->pause_menu && pause_menu_is_open(g->pause_menu)) ||
                (g->save_menu && save_menu_is_open(g->save_menu))) {
                break;
            }
            
            if (g->game_screen) {
                game_screen_update(g->game_screen, 0.016f);
                
                if (g->game_screen->battle_triggered && !g->game_screen->in_dialogue) {
                    g->game_screen->battle_triggered = false;
                    
                    int w, h;
                    SDL_GetWindowSize(g->window, &w, &h);
                    
                    g->opening = opening_create(g->renderer, w, h);
                    if (g->opening) {
                        g->state = STATE_OPENING_ANIMATION;
                    } else {
                        g->battle = battle_create(g->renderer, w, h);
                        if (g->battle) {
                            g->state = STATE_BATTLE;
                        }
                    }
                }
            }
            break;
            
        case STATE_OPENING_ANIMATION:
            if (g->opening) {
                opening_update(g->opening, 0.016f);
                
                if (opening_is_complete(g->opening)) {
                    opening_destroy(g->opening);
                    g->opening = NULL;
                    
                    int w, h;
                    SDL_GetWindowSize(g->window, &w, &h);
                    
                    g->battle = battle_create(g->renderer, w, h);
                    if (g->battle) {
                        g->state = STATE_BATTLE;
                    } else {
                        g->state = STATE_PLAYING;
                    }
                }
            }
            break;
            
        case STATE_BATTLE:
            if (g->battle) {
                battle_update(g->battle, 0.016f);
                if (!battle_is_active(g->battle)) {
                    battle_destroy(g->battle);
                    g->battle = NULL;
                    g->state = STATE_PLAYING;
                }
            }
            break;
            
        case STATE_CREDITS:
            break;
    }
}

void game_draw(struct Game *g) {
    SDL_SetRenderDrawColor(g->renderer, 0, 0, 0, 255);
    SDL_RenderClear(g->renderer);
    
    switch (g->state) {
        case STATE_MENU:
            if (g->menu) menu_render(g->renderer, g->menu);
            break;
            
        case STATE_SAVE_MENU:
            // Render game screen behind if it exists
            if (g->game_screen) {
                game_screen_render(g->renderer, g->game_screen);
            } else {
                // Render menu behind if no game
                if (g->menu) menu_render(g->renderer, g->menu);
            }
            // Render save menu on top
            if (g->save_menu) save_menu_render(g->renderer, g->save_menu);
            break;
            
        case STATE_PLAYING:
            if (g->game_screen) game_screen_render(g->renderer, g->game_screen);
            if (g->pause_menu && pause_menu_is_open(g->pause_menu)) {
                pause_menu_render(g->renderer, g->pause_menu);
            }
            break;
            
        case STATE_OPENING_ANIMATION:
            if (g->game_screen) game_screen_render(g->renderer, g->game_screen);
            if (g->opening) opening_render(g->renderer, g->opening);
            break;
            
        case STATE_BATTLE:
            if (g->battle) battle_render(g->renderer, g->battle);
            break;
            
        case STATE_CREDITS:
            SDL_SetRenderDrawColor(g->renderer, 10, 10, 30, 255);
            SDL_RenderClear(g->renderer);
            break;
    }
    
    SDL_RenderPresent(g->renderer);
}

void game_run(struct Game *g) 
{
    while (g->isRunning) 
    {
        game_events(g);
        game_update(g);
        game_draw(g);
        SDL_Delay(16);
    }
}

int main(void) 
{
    bool exit_status = EXIT_FAILURE;
    struct Game *game = NULL;

    if(game_new(&game)) 
    {
        game_run(game);
        exit_status = EXIT_SUCCESS;
    }

    game_free(&game);
    return exit_status;
}