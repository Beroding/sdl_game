#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

#define SDL_FLAGS SDL_INIT_VIDEO

#define WINDOW_TITLE "Closed Window"
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 700

struct Game 
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture* player_texture;
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

    // ✅ INIT SDL_image
    if (!IMG_Init(0))
    {
        fprintf(stderr, "Error initializing SDL_image: %s\n", SDL_GetError());
        return false;
    }//

    g->window = SDL_CreateWindow(WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if(!g->window)
    {
        fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
        return false;
    }
    printf("Window created\n");

    g->renderer = SDL_CreateRenderer(g->window, "opengl");
    if(!g->renderer) 
    {
        printf("Error creating renderer: %s\n", SDL_GetError());
        return false;
    }
    printf("Renderer created\n");

    //
    const char *path = "./knight_idle_spritesheet.png";

    g->player_texture = IMG_LoadTexture(g->renderer, path);
    if (!g->player_texture)
    {
        fprintf(stderr, "Error loading texture: %s\n", SDL_GetError());
        return false;
    }

    printf("Texture loaded\n");//

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

        if(g->player_texture)
        {
            SDL_DestroyTexture(g->player_texture);
            g->player_texture = NULL;
        }

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

        IMG_Quit();

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
            default:
                break;
        }
    }
}

void game_draw(struct Game *g)
{
    SDL_SetRenderDrawColor(g->renderer, 255, 255, 0, 255);
    SDL_RenderClear(g->renderer);
    
    // SDL_RenderPresent(g->renderer);
    // ✅ Draw texture
    SDL_FRect dst = {100, 100, 128, 128}; // position + size
    SDL_RenderTexture(g->renderer, g->player_texture, NULL, &dst);

    SDL_RenderPresent(g->renderer);//
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
// ###################################################################################################
// #define SDL_MAIN_USE_CALLBACKS
// #include <SDL3/SDL_main.h>
// #include <SDL3/SDL.h>

// SDL_Window* window;
// SDL_Renderer* renderer;
// SDL_Event event;

// void SDL_AppQuit(void *appstate, SDL_AppResult result){
//     SDL_DestroyRenderer(renderer);
//     renderer = NULL;
//     SDL_DestroyWindow(window);
//     window = NULL;
//     SDL_QuitSubSystem(SDL_INIT_VIDEO);
// }

// SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
//     if (event->type == SDL_EVENT_QUIT) {
//         return SDL_APP_SUCCESS;
//     }
//     return SDL_APP_CONTINUE;
// }

// void update() {

// }

// void render() {
//     SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);  // ← Set color FIRST
//     SDL_RenderClear(renderer);                         // ← Then clear with that color
//     SDL_RenderPresent(renderer);
// }

// SDL_AppResult SDL_AppIterate(void *appstate) {
//     update();
//     render();
//     return SDL_APP_CONTINUE;
// }

// SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
//     if (!SDL_Init(SDL_INIT_VIDEO)) {
//         SDL_Log("Error initializing SDL: %s", SDL_GetError());
//         return SDL_APP_FAILURE;
//     }

//     const char path[] = " ./knight_idle_spritesheet.png";
//     player_texture = IMG_LoadTexture(renderer, path);

//     return SDL_APP_CONTINUE;
// }
