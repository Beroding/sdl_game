#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL_events.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "menu_screen.h"

Menu *menu_create(SDL_Renderer *renderer, int window_width, int window_height) {
    Menu *menu = malloc(sizeof(Menu));
    if (!menu) {
        fprintf(stderr, "Failed to allocate menu structure\n");
        return NULL;
    }

    SDL_Surface *surface = IMG_Load("game_assets/menu_screen_bg.jpg");
    if (!surface) {
        fprintf(stderr, "Failed to load menu_screen_bg.jpg: %s\n", SDL_GetError());
        free(menu);
        return NULL;
    }

    menu->texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    if (!menu->texture) {
        fprintf(stderr, "Failed to create texture: %s\n", SDL_GetError());
        free(menu);
        return NULL;
    }

    menu->selected = 0;
    menu->option_count = 4;

    float option_w = 360;
    float option_h = 64;
    float option_x = (window_width - option_w) / 2.0f;
    float start_y = window_height * 0.47f;
    float spacing = 84;

    for (int i = 0; i < menu->option_count; i++) {
        menu->option_rects[i].x = option_x;
        menu->option_rects[i].y = start_y + i * spacing;
        menu->option_rects[i].w = option_w;
        menu->option_rects[i].h = option_h;
    }

    menu->rect.x = 0;
    menu->rect.y = 0;
    menu->rect.w = window_width;
    menu->rect.h = window_height;

    menu->font = TTF_OpenFont("game_assets/Roboto_Medium.ttf", 48);
    printf("Loading font\n");
    if (!menu->font) { 
        fprintf(stderr, "Font load error: %s\n", SDL_GetError());
        free(menu);
        return NULL;
    }

    const char *labels[4] = {"CONTINUE", "NEW GAME", "CREDIT", "QUIT"};
    for (int i = 0; i < 4; i++) {
        SDL_Color c = {255,255,255,255};
        SDL_Surface *surf = TTF_RenderText_Blended(menu->font, labels[i], 0, c);
        menu->item_text[i] = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_DestroySurface(surf);

        float w = 0, h = 0;
        SDL_GetTextureSize(menu->item_text[i], &w, &h);
        menu->option_rects[i].w = w;
        menu->option_rects[i].h = h;
        menu->option_rects[i].x = (window_width - w) / 2.0f;
        menu->option_rects[i].y = 300 + i * 80;
    }

    return menu;
}

void menu_render(SDL_Renderer *renderer, Menu *menu) {
    SDL_RenderTexture(renderer, menu->texture, NULL, &menu->rect);

    for (int i = 0; i < menu->option_count; i++) {
        if (i == menu->selected){
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 120);
            SDL_RenderFillRect(renderer, &menu->option_rects[i]);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 220);
            SDL_RenderRect(renderer, &menu->option_rects[i]);
        }
        SDL_RenderTexture(renderer, menu->item_text[i], NULL, &menu->option_rects[i]);
    }
}

int menu_handle_click(Menu *menu, float mouse_x, float mouse_y) {
    for (int i = 0; i < menu->option_count; i++) {
        SDL_FRect *r = &menu->option_rects[i];
        if (mouse_x >= r->x && mouse_x < r->x + r->w &&
            mouse_y >= r->y && mouse_y < r->y + r->h) {
            menu->selected = i;
            return i;
        }
    }
    return -1;
}

int menu_handle_key(Menu *menu, SDL_KeyboardEvent *key) {
    if (key->repeat) return -1;

    switch (key->scancode) {
        case SDL_SCANCODE_DOWN:
            menu->selected = (menu->selected + 1) % menu->option_count;
            return -1;
        case SDL_SCANCODE_UP:
            menu->selected = (menu->selected - 1 + menu->option_count) % menu->option_count;
            return -1;
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_KP_ENTER:
            return menu->selected;
        default:
            return -1;
    }
}

void menu_destroy(Menu *menu) {
    if (menu) {
        for (int i = 0; i < menu->option_count; i++)
        {
            if (menu->item_text[i]) SDL_DestroyTexture(menu->item_text[i]);
        }

        if (menu->font)
        {
            TTF_CloseFont(menu->font);
        }    
        if (menu->texture) 
        {
            SDL_DestroyTexture(menu->texture);
        }
        free(menu);
    }
}