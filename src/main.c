#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "menu_screen.h"

#define SDL_FLAGS SDL_INIT_VIDEO

#define WINDOW_TITLE "something sumting"
#define WINDOW_WIDTH 1500
#define WINDOW_HEIGHT 900

struct Game 
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    Menu *menu;
    SDL_Event event;
    bool isRunning;
};

bool game_init_sdl(struct Game *g);
bool game_new(struct Game **game);
void game_free(struct Game **game);
void game_events(struct Game *g);
void game_draw(struct Game *g);
void game_run(struct Game *g);

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

    g->menu = menu_create(g->renderer, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!g->menu)
    {
        printf("menu_create failed\n");
        return false;
    }
    printf("Menu created\n");

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

    return true;
}

void game_free(struct Game **game) 
{    
    if(*game)
    {
        struct Game *g = *game;

        menu_destroy(g->menu);

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

void game_events(struct Game *g)
{
    while (SDL_PollEvent(&g->event)) 
    {
        switch (g->event.type) 
        {
            case SDL_EVENT_QUIT:
                g->isRunning = false;
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                int index = menu_handle_click(g->menu, g->event.button.x, g->event.button.y);
                if (index >= 0)
                {
                    printf("menu option click index=%d\n", index);
                    if (index == 3) g->isRunning = false; // Quit
                }
            }
                break;
            case SDL_EVENT_KEY_DOWN:
            {
                int index = menu_handle_key(g->menu, &g->event.key);
                if (index >= 0)
                {
                    printf("menu option enter index=%d\n", index);
                    if (index == 0) printf("Continue selected\n");
                    else if (index == 1) printf("New Game selected\n");
                    else if (index == 2) printf("Credit selected\n");
                    else if (index == 3) g->isRunning = false;
                }
            }
                break;
            default:
                break;
        }
    }
}

void game_draw(struct Game *g)
{
    SDL_SetRenderDrawColor(g->renderer, 0, 0, 0, 255);
    SDL_RenderClear(g->renderer);
    
    if(g-> menu){
        menu_render(g->renderer, g->menu);
    }
    
    SDL_RenderPresent(g->renderer);
}

void game_run(struct Game *g) 
{
    while (g->isRunning) 
    {
        game_events(g);

        game_draw(g);

        SDL_Delay(16);
    }
}

int main(void) 
{
    bool exit_status = EXIT_FAILURE;

    struct Game *game = NULL;

    printf("game1: %p\n", game);

    if(game_new(&game)) 
    {
        printf("game2: %p\n", game);

        game_run(game);

        exit_status = EXIT_SUCCESS;
    }

    printf("game3: %p\n", game);

    game_free(&game);

    printf("game4: %p\n", game);

    return exit_status;
}