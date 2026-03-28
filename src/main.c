#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "menu_screen.h"
#include "game_screen.h"
#include "battle_system.h"
#include "opening_animation.h"
#include "dialogue_loader.h"
#include "pause_menu.h"  // NEW

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
    STATE_CREDITS
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
    PauseMenu *pause_menu;  // NEW
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

    g->game_screen = NULL;
    g->battle = NULL;
    g->pause_menu = NULL;  // Will be created when needed

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

    if(!game_init_sdl(g))
    {
        free(g);
        *game = NULL;
        return false;
    }
    printf("game_init_sdl SUCCESS\n");

    g->isRunning = true;
    g->state = STATE_MENU;

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
        pause_menu_destroy(g->pause_menu);  // NEW

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
    
    g->state = STATE_PLAYING;
}

void return_to_menu(struct Game *g)
{
    printf("Returning to menu...\n");
    g->state = STATE_MENU;
    // Don't destroy pause menu, just close it
    if (g->pause_menu) {
        pause_menu_close(g->pause_menu);
    }
}

void game_events(struct Game *g)
{
    while (SDL_PollEvent(&g->event)) 
    {
        // Handle pause menu first if open
        if (g->pause_menu && pause_menu_is_open(g->pause_menu)) {
            switch (g->event.type) {
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (pause_menu_handle_mouse(g->pause_menu, &g->event.button)) {
                        // Check if exit was requested
                        if (g->pause_menu->exit_to_menu) {
                            g->pause_menu->exit_to_menu = false;
                            return_to_menu(g);
                        }
                        continue; // Event handled
                    }
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (pause_menu_handle_key(g->pause_menu, &g->event.key)) {
                        // Check if exit was requested
                        if (g->pause_menu->exit_to_menu) {
                            g->pause_menu->exit_to_menu = false;
                            return_to_menu(g);
                        }
                        continue; // Event handled
                    }
                    break;
                case SDL_EVENT_KEY_UP:
                    // Don't pass key events to game when paused
                    continue;
            }
        }
        
        switch (g->event.type) 
        {
            case SDL_EVENT_KEY_UP:
                if (g->state == STATE_PLAYING && g->game_screen && 
                    (!g->pause_menu || !pause_menu_is_open(g->pause_menu))) {
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
                    (!g->pause_menu || !pause_menu_is_open(g->pause_menu))) {
                    game_screen_handle_mouse(g->game_screen, &g->event.button, g->renderer);
                }
                // Handle menu clicks
                else if (g->state == STATE_MENU) {
                    int index = menu_handle_click(g->menu, g->event.button.x, g->event.button.y);
                    if (index >= 0) {
                        printf("menu click index=%d\n", index);
                        switch (index) {
                            case 0: /* Continue */ break;
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
                        // Skip to end
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
                        break; // Handled
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
                            case 0: /* Continue */ break;
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
    // Update pause menu animation
    if (g->pause_menu) {
        pause_menu_update(g->pause_menu, 0.016f);
    }
    
    switch (g->state) {
        case STATE_MENU:
            break;
            
        case STATE_PLAYING:
            // Don't update game world when paused
            if (g->pause_menu && pause_menu_is_open(g->pause_menu)) {
                break;
            }
            
            if (g->game_screen) {
                game_screen_update(g->game_screen, 0.016f);
                
                if (g->game_screen->battle_triggered && !g->game_screen->in_dialogue) {
                    g->game_screen->battle_triggered = false;
                    
                    // Transition to opening animation instead of directly to battle
                    int w, h;
                    SDL_GetWindowSize(g->window, &w, &h);
                    
                    g->opening = opening_create(g->renderer, w, h);
                    if (g->opening) {
                        g->state = STATE_OPENING_ANIMATION;
                    } else {
                        // Fallback: go directly to battle if animation fails
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
                    
                    // Now start the battle
                    int w, h;
                    SDL_GetWindowSize(g->window, &w, &h);
                    
                    g->battle = battle_create(g->renderer, w, h);
                    if (g->battle) {
                        g->state = STATE_BATTLE;
                    } else {
                        // Fallback to game if battle fails
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
            
        case STATE_PLAYING:
            if (g->game_screen) game_screen_render(g->renderer, g->game_screen);
            // Render pause menu on top if open
            if (g->pause_menu && pause_menu_is_open(g->pause_menu)) {
                pause_menu_render(g->renderer, g->pause_menu);
            }
            break;
            
        case STATE_OPENING_ANIMATION:
            // Render the game screen behind (frozen)
            if (g->game_screen) game_screen_render(g->renderer, g->game_screen);
            // Render animation on top
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