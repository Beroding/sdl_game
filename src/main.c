#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "menu_screen.h"
#include "game_screen.h"

#define SDL_FLAGS SDL_INIT_VIDEO

#define WINDOW_TITLE "something sumting"
#define WINDOW_WIDTH 1500
#define WINDOW_HEIGHT 900

typedef enum {
    STATE_MENU,
    STATE_PLAYING,
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
};

// Function declarations
bool game_init_sdl(struct Game *g);
bool game_new(struct Game **game);
void game_free(struct Game **game);
void game_events(struct Game *g);
void game_update(struct Game *g);
void game_draw(struct Game *g);
void game_run(struct Game *g);

// State transition helpers
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

    g->window = SDL_CreateWindow(WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if(!g->window)
    {
        fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
        return false;
    }
    printf("Window created\n");
    SDL_SetWindowResizable(g->window, true);

    g->renderer = SDL_CreateRenderer(g->window, NULL);
    if(!g->renderer) 
    {
        printf("Error creating renderer: %s\n", SDL_GetError());
        return false;
    }
    printf("Renderer created\n");

    // Create menu (always needed)
    g->menu = menu_create(g->renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!g->menu)
    {
        printf("menu_create failed\n");
        return false;
    }
    printf("Menu created\n");

    // Game screen created on demand
    g->game_screen = NULL;

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
    
    // Clean up old game screen if exists
    game_screen_destroy(g->game_screen);
    g->game_screen = NULL;
    
    // Create fresh game screen
    g->game_screen = game_screen_create(g->renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!g->game_screen) {
        fprintf(stderr, "Failed to create game screen\n");
        return;
    }
    
    g->state = STATE_PLAYING;
}

void return_to_menu(struct Game *g)
{
    printf("Returning to menu...\n");
    
    // Optional: Keep game screen for "Continue" or destroy it
    // game_screen_destroy(g->game_screen);
    // g->game_screen = NULL;
    
    g->state = STATE_MENU;
}

void game_events(struct Game *g)
{
    while (SDL_PollEvent(&g->event)) 
    {
        switch (g->event.type) 
        {
            case SDL_EVENT_KEY_UP:
                if (g->state == STATE_PLAYING && g->game_screen) {
                    game_screen_handle_input(g->game_screen, &g->event.key);
                }
                break;

            case SDL_EVENT_QUIT:
                g->isRunning = false;
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (g->state == STATE_MENU) {
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
                else if (g->state == STATE_PLAYING) {
                    // ESC returns to menu
                    if (g->event.key.scancode == SDL_SCANCODE_ESCAPE) {
                        return_to_menu(g);
                    } else {
                        game_screen_handle_input(g->game_screen, &g->event.key);
                    }
                }
                else if (g->state == STATE_CREDITS) {
                    // Any key returns to menu from credits
                    return_to_menu(g);
                }
                break;

            default:
                break;
        }
    }
}

void game_update(struct Game *g)
{
    switch (g->state) {
        case STATE_MENU:
            // Menu animations, etc.
            break;
            
        case STATE_PLAYING:
            if (g->game_screen) {
                game_screen_update(g->game_screen, 0.016f); // ~60fps delta
            }
            break;
            
        case STATE_CREDITS:
            // Credits animation
            break;
    }
}

void game_draw(struct Game *g)
{
    SDL_SetRenderDrawColor(g->renderer, 0, 0, 0, 255);
    SDL_RenderClear(g->renderer);
    
    switch (g->state) {
        case STATE_MENU:
            if (g->menu) {
                menu_render(g->renderer, g->menu);
            }
            break;
            
        case STATE_PLAYING:
            if (g->game_screen) {
                game_screen_render(g->renderer, g->game_screen);
            }
            break;
            
        case STATE_CREDITS:
            // Simple credits screen
            SDL_SetRenderDrawColor(g->renderer, 10, 10, 30, 255);
            SDL_RenderClear(g->renderer);
            // TODO: Render credits text
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