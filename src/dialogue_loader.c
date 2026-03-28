#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dialogue_loader.h"

static char* trim_whitespace(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

static void parse_line(char *line, DialogueEntry *entry) {
    // Clear entry
    memset(entry, 0, sizeof(DialogueEntry));
    entry->next_line = -1; // -1 means end
    
    line = trim_whitespace(line);
    
    // Skip empty lines and comments
    if (strlen(line) == 0 || line[0] == '#' || line[0] == '/') return;
    
    // Check for special commands
    if (strncmp(line, "[BATTLE]", 8) == 0) {
        entry->triggers_battle = true;
        strcpy(entry->speaker, "SYSTEM");
        strcpy(entry->text, "BATTLE_START");
        return;
    }
    
    // Check for player choice
    if (strncmp(line, "[CHOICE]", 8) == 0) {
        entry->is_player_choice = true;
        // Format: [CHOICE] Speaker | Question | Option A | Option B | Next A | Next B
        char *rest = line + 8;
        char *token = strtok(rest, "|");
        
        if (token) {
            strcpy(entry->speaker, trim_whitespace(token));
            token = strtok(NULL, "|");
        }
        if (token) {
            strcpy(entry->text, trim_whitespace(token));
            token = strtok(NULL, "|");
        }
        if (token) {
            strcpy(entry->choice_a, trim_whitespace(token));
            token = strtok(NULL, "|");
        }
        if (token) {
            strcpy(entry->choice_b, trim_whitespace(token));
            token = strtok(NULL, "|");
        }
        if (token) {
            entry->next_line_a = atoi(trim_whitespace(token));
            token = strtok(NULL, "|");
        }
        if (token) {
            entry->next_line_b = atoi(trim_whitespace(token));
        }
        return;
    }
    
    // Check for GOTO
    if (strncmp(line, "[GOTO]", 6) == 0) {
        entry->next_line = atoi(line + 6);
        strcpy(entry->speaker, "SYSTEM");
        strcpy(entry->text, "GOTO");
        return;
    }
    
    // Regular dialogue line
    // Format: Speaker: Text | NextLine (optional)
    char *colon = strchr(line, ':');
    if (colon) {
        *colon = '\0';
        strcpy(entry->speaker, trim_whitespace(line));
        
        char *rest = colon + 1;
        char *pipe = strchr(rest, '|');
        
        if (pipe) {
            *pipe = '\0';
            strcpy(entry->text, trim_whitespace(rest));
            entry->next_line = atoi(trim_whitespace(pipe + 1));
        } else {
            strcpy(entry->text, trim_whitespace(rest));
            entry->next_line = -1; // Auto-increment
        }
    } else {
        // No speaker specified, use previous or default
        strcpy(entry->text, line);
    }
}

DialogueScript* dialogue_load(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Failed to open dialogue file: %s\n", filename);
        return NULL;
    }
    
    DialogueScript *script = calloc(1, sizeof(DialogueScript));
    if (!script) {
        fclose(file);
        return NULL;
    }
    
    strcpy(script->file_path, filename);
    
    char line[MAX_TEXT_LEN];
    int line_num = 0;
    
    while (fgets(line, sizeof(line), file) && script->line_count < MAX_DIALOGUE_LINES) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        parse_line(line, &script->lines[script->line_count]);
        
        // Only count non-empty lines
        if (strlen(script->lines[script->line_count].text) > 0 ||
            script->lines[script->line_count].triggers_battle) {
            
            // Auto-set next line if not specified
            if (script->lines[script->line_count].next_line == -1 && 
                !script->lines[script->line_count].is_player_choice) {
                script->lines[script->line_count].next_line = script->line_count + 1;
            }
            
            script->line_count++;
        }
    }
    
    fclose(file);
    
    printf("Loaded dialogue: %d lines from %s\n", script->line_count, filename);
    return script;
}

void dialogue_destroy(DialogueScript *script) {
    free(script);
}

DialogueEntry* dialogue_get_line(DialogueScript *script, int index) {
    if (!script || index < 0 || index >= script->line_count) return NULL;
    return &script->lines[index];
}