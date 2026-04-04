// ============================================================================
// menu_screen.c - MAIN MENU (Title Screen)
// ============================================================================
// This file handles the title screen with "Continue", "New Game", etc.
// It's the first thing players see when starting the game.

#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL_events.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "../include/menu_screen.h"
#include "../include/music_system.h"

// Create the main menu
Menu *menu_create(SDL_Renderer *renderer, int window_width, int window_height) {
    // Allocate memory for menu
    Menu *menu = malloc(sizeof(Menu));
    if (!menu) {
        fprintf(stderr, "Failed to allocate menu structure\n");
        return NULL;
    }

    // Load background image
    SDL_Surface *surface = IMG_Load("game_assets/menu_screen_bg.jpg");
    if (!surface) {
        fprintf(stderr, "Failed to load menu_screen_bg.jpg: %s\n", SDL_GetError());
        free(menu);
        return NULL;
    }

    // Convert surface to texture (GPU-friendly format)
    menu->texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    if (!menu->texture) {
        fprintf(stderr, "Failed to create texture: %s\n", SDL_GetError());
        free(menu);
        return NULL;
    }

    // Menu starts with first option selected
    menu->selected = 0;
    menu->option_count = 4;  // 4 buttons: Continue, New Game, Credit, Quit

    // Position menu options (centered horizontally)
    float option_w = 360;
    float option_h = 64;
    float option_x = (window_width - option_w) / 2.0f;
    float start_y = window_height * 1.0f;
    float spacing = 85;

    for (int i = 0; i < menu->option_count; i++) {
        menu->option_rects[i].x = option_x;
        menu->option_rects[i].y = start_y + i * spacing;
        menu->option_rects[i].w = option_w;
        menu->option_rects[i].h = option_h;
    }

    // Set up background to fill entire screen
    menu->rect.x = 0;
    menu->rect.y = 0;
    menu->rect.w = window_width;
    menu->rect.h = window_height;

    // Load fonts
    menu->font = TTF_OpenFont("game_assets/Roboto_Medium.ttf", 48);
    printf("Loading font\n");
    if (!menu->font) { 
        fprintf(stderr, "Font load error: %s\n", SDL_GetError());
        free(menu);
        return NULL;
    }

    // Load fancy title font
    menu->game_title_font = TTF_OpenFont("game_assets/MedievalSharp-Regular.ttf", 96);
    if (!menu->game_title_font) { 
        fprintf(stderr, "Title font load error: %s\n", SDL_GetError());
        TTF_CloseFont(menu->font);
        free(menu);
        return NULL;
    }

    // Create text textures for each menu option
    const char *labels[4] = {"CONTINUE", "NEW GAME", "CREDIT", "QUIT"};
    for (int i = 0; i < 4; i++) {
        SDL_Color c = {255,255,255,255};  // White text
        SDL_Surface *surf = TTF_RenderText_Blended(menu->font, labels[i], 0, c);
        menu->item_text[i] = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_DestroySurface(surf);

        // Center text and position vertically
        float w = 0, h = 0;
        SDL_GetTextureSize(menu->item_text[i], &w, &h);
        menu->option_rects[i].w = w;
        menu->option_rects[i].h = h;
        menu->option_rects[i].x = (window_width - w) / 2.0f;
        menu->option_rects[i].y = 500 + i * 80;  // Start at y=500, 80px apart
    }

    // Create game title texture
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *title_surf = TTF_RenderText_Blended(menu->game_title_font, "AfterLife", 0, white);
    menu->game_title_text = SDL_CreateTextureFromSurface(renderer, title_surf);
    SDL_DestroySurface(title_surf);

    return menu;
}

// Draw the menu on screen
void menu_render(SDL_Renderer *renderer, Menu *menu) {
    // Draw background image
    SDL_RenderTexture(renderer, menu->texture, NULL, &menu->rect);

    // Draw each menu option
    for (int i = 0; i < menu->option_count; i++) {
        // Highlight selected option with yellow box
        if (i == menu->selected){
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 120);  // Yellow transparent
            SDL_RenderFillRect(renderer, &menu->option_rects[i]);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 220);  // White border
            SDL_RenderRect(renderer, &menu->option_rects[i]);
        }
        // Draw the text
        SDL_RenderTexture(renderer, menu->item_text[i], NULL, &menu->option_rects[i]);
    }

    // Draw game title at top
    float w, h;
    SDL_GetTextureSize(menu->game_title_text, &w, &h);
    SDL_FRect title_rect = {
        .x = (1500 - w) / 2,  // Center horizontally
        .y = 80,               // Near top
        .w = w,
        .h = h
    };
    SDL_RenderTexture(renderer, menu->game_title_text, NULL, &title_rect);
}

// Check if mouse clicked on a menu option
int menu_handle_click(Menu *menu, float mouse_x, float mouse_y) {
    for (int i = 0; i < menu->option_count; i++) {
        SDL_FRect *r = &menu->option_rects[i];
        // Check if mouse is inside this button
        if (mouse_x >= r->x && mouse_x < r->x + r->w &&
            mouse_y >= r->y && mouse_y < r->y + r->h) {
            menu->selected = i;  // Select this option
            return i;             // Return which option was clicked
        }
    }
    return -1;  // Nothing clicked
}

// Handle keyboard navigation
int menu_handle_key(Menu *menu, SDL_KeyboardEvent *key) {
    if (key->repeat) return -1;  // Ignore key repeat (holding down)

    switch (key->scancode) {
        case SDL_SCANCODE_DOWN:
            // Move selection down (wrap around to top)
            menu->selected = (menu->selected + 1) % menu->option_count;
            return -1;
        case SDL_SCANCODE_UP:
            // Move selection up (wrap around to bottom)
            menu->selected = (menu->selected - 1 + menu->option_count) % menu->option_count;
            return -1;
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_KP_ENTER:
            // Enter key = click selected option
            return menu->selected;
        default:
            return -1;
    }
}

// Cleanup - free all menu memory
void menu_destroy(Menu *menu) {
    if (menu) {
        // Destroy all text textures
        for (int i = 0; i < menu->option_count; i++)
        {
            if (menu->item_text[i]) SDL_DestroyTexture(menu->item_text[i]);
        }

        // Close fonts
        if (menu->font) TTF_CloseFont(menu->font);
        if (menu->game_title_font) TTF_CloseFont(menu->game_title_font);
        
        // Destroy background
        if (menu->texture) SDL_DestroyTexture(menu->texture);
        
        free(menu);
    }
}