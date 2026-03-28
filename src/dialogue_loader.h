#ifndef DIALOGUE_LOADER_H
#define DIALOGUE_LOADER_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>

#define MAX_DIALOGUE_LINES 100
#define MAX_TEXT_LEN 1024

typedef struct {
    char speaker[64];
    char text[MAX_TEXT_LEN];
    bool is_player_choice;
    char choice_a[128];
    char choice_b[128];
    int next_line;       // For regular lines
    int next_line_a;     // For choice A
    int next_line_b;     // For choice B
    bool triggers_battle;
} DialogueEntry;

typedef struct {
    char file_path[256];
    DialogueEntry lines[MAX_DIALOGUE_LINES];
    int line_count;
} DialogueScript;

// Functions
DialogueScript* dialogue_load(const char *filename);
void dialogue_destroy(DialogueScript *script);
DialogueEntry* dialogue_get_line(DialogueScript *script, int index);

#endif