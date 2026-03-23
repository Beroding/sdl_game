#ifndef MENU_SCREEN_H
#define MENU_SCREEN_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

typedef struct {
    SDL_Texture *texture;
    SDL_FRect rect;
    TTF_Font *font;
    SDL_Texture *item_text[4];
    SDL_FRect option_rects[4];
    int selected;
    int option_count;
} Menu;

Menu *menu_create(SDL_Renderer *renderer, int window_width, int window_height);
void menu_render(SDL_Renderer *renderer, Menu *menu);
int menu_handle_click(Menu *menu, float mouse_x, float mouse_y);
int menu_handle_key(Menu *menu, SDL_KeyboardEvent *key);
void menu_destroy(Menu *menu);

#endif