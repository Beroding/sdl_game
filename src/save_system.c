#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "save_system.h"

#ifdef _WIN32
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
#else
    #include <unistd.h>
#endif

static unsigned int calculate_checksum(SaveSlotData *data) {
    // Simple checksum - sum of all bytes excluding checksum field
    unsigned int sum = 0;
    unsigned char *ptr = (unsigned char*)data;
    size_t len = sizeof(SaveSlotData) - sizeof(unsigned int);
    
    for (size_t i = 0; i < len; i++) {
        sum += ptr[i];
    }
    return sum;
}

static bool verify_checksum(SaveSlotData *data) {
    return data->checksum == calculate_checksum(data);
}

SaveSystem* save_system_create(void) {
    SaveSystem *sys = calloc(1, sizeof(SaveSystem));
    if (!sys) return NULL;
    
    sys->selected_slot = -1;
    
    // Try to load existing saves
    save_system_read_from_disk(sys);
    
    printf("Save system initialized\n");
    return sys;
}

void save_system_destroy(SaveSystem *sys) {
    if (sys) {
        free(sys);
    }
}

bool save_game(SaveSystem *sys, int slot, const char *player_name,
               int level, int score, float player_x, float player_y,
               const char *dialogue_file, int dialogue_line,
               int max_hp, int current_hp, int max_chakra, int current_chakra,
               bool met_naberius, bool defeated_naberius, bool tutorial_done,
               float play_time) {
    if (!sys || slot < 0 || slot >= MAX_SAVE_SLOTS) return false;
    
    SaveSlotData *data = &sys->slots[slot];
    
    // Clear and fill data
    memset(data, 0, sizeof(SaveSlotData));
    
    data->version = SAVE_FILE_VERSION;
    data->timestamp = time(NULL);
    strncpy(data->player_name, player_name ? player_name : "Vermin Soul", 31);
    data->player_name[31] = '\0';
    
    data->current_level = level;
    data->player_score = score;
    data->player_x = player_x;
    data->player_y = player_y;
    
    if (dialogue_file) {
        strncpy(data->current_dialogue_file, dialogue_file, 255);
        data->current_dialogue_file[255] = '\0';
    }
    data->current_dialogue_line = dialogue_line;
    
    data->player_max_hp = max_hp;
    data->player_current_hp = current_hp;
    data->player_max_chakra = max_chakra;
    data->player_current_chakra = current_chakra;
    
    data->has_met_naberius = met_naberius;
    data->defeated_naberius = defeated_naberius;
    data->tutorial_completed = tutorial_done;
    data->play_time = play_time;
    
    // Calculate checksum
    data->checksum = calculate_checksum(data);
    
    sys->slot_occupied[slot] = true;
    
    // Write to disk immediately
    return save_system_write_to_disk(sys);
}

bool load_game(SaveSystem *sys, int slot, SaveSlotData *out_data) {
    if (!sys || !out_data || slot < 0 || slot >= MAX_SAVE_SLOTS) return false;
    if (!sys->slot_occupied[slot]) return false;
    
    // Verify checksum
    if (!verify_checksum(&sys->slots[slot])) {
        printf("Save slot %d corrupted!\n", slot);
        sys->slot_occupied[slot] = false;
        return false;
    }
    
    memcpy(out_data, &sys->slots[slot], sizeof(SaveSlotData));
    return true;
}

bool delete_save(SaveSystem *sys, int slot) {
    if (!sys || slot < 0 || slot >= MAX_SAVE_SLOTS) return false;
    
    // Clear the slot
    memset(&sys->slots[slot], 0, sizeof(SaveSlotData));
    sys->slot_occupied[slot] = false;
    
    printf("Deleted save slot %d\n", slot);
    
    // Write updated data to disk
    return save_system_write_to_disk(sys);
}

bool has_save_in_slot(SaveSystem *sys, int slot) {
    if (!sys || slot < 0 || slot >= MAX_SAVE_SLOTS) return false;
    return sys->slot_occupied[slot];
}

bool save_system_write_to_disk(SaveSystem *sys) {
    if (!sys) return false;
    
    // Create saves directory if it doesn't exist
    struct stat st = {0};
    if (stat("saves", &st) == -1) {
        mkdir("saves", 0755);
    }
    
    FILE *file = fopen(SAVE_FILENAME, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open save file for writing\n");
        return false;
    }
    
    // Write header
    int version = SAVE_FILE_VERSION;
    fwrite(&version, sizeof(int), 1, file);
    
    // Write slot occupancy flags
    fwrite(sys->slot_occupied, sizeof(bool), MAX_SAVE_SLOTS, file);
    
    // Write each slot
    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
        fwrite(&sys->slots[i], sizeof(SaveSlotData), 1, file);
    }
    
    fclose(file);
    printf("Save data written to disk\n");
    return true;
}

bool save_system_read_from_disk(SaveSystem *sys) {
    if (!sys) return false;
    
    FILE *file = fopen(SAVE_FILENAME, "rb");
    if (!file) {
        printf("No existing save file found\n");
        return false; // No save file yet - that's ok
    }
    
    // Read header
    int version;
    if (fread(&version, sizeof(int), 1, file) != 1) {
        fclose(file);
        return false;
    }
    
    if (version != SAVE_FILE_VERSION) {
        printf("Save file version mismatch (expected %d, got %d)\n", 
               SAVE_FILE_VERSION, version);
        fclose(file);
        return false;
    }
    
    // Read slot occupancy
    if (fread(sys->slot_occupied, sizeof(bool), MAX_SAVE_SLOTS, file) != MAX_SAVE_SLOTS) {
        fclose(file);
        return false;
    }
    
    // Read each slot
    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
        if (fread(&sys->slots[i], sizeof(SaveSlotData), 1, file) != 1) {
            fclose(file);
            return false;
        }
        
        // Verify checksum for occupied slots
        if (sys->slot_occupied[i]) {
            if (!verify_checksum(&sys->slots[i])) {
                printf("Save slot %d corrupted, marking as empty\n", i);
                sys->slot_occupied[i] = false;
            }
        }
    }
    
    fclose(file);
    printf("Save data loaded from disk\n");
    return true;
}

void save_system_get_slot_info(SaveSystem *sys, int slot,
                                char *date_str, size_t date_len,
                                char *preview_str, size_t preview_len) {
    if (!sys || slot < 0 || slot >= MAX_SAVE_SLOTS) {
        if (date_str) strncpy(date_str, "ERROR", date_len);
        if (preview_str) strncpy(preview_str, "ERROR", preview_len);
        return;
    }
    
    if (!sys->slot_occupied[slot]) {
        if (date_str) strncpy(date_str, "EMPTY", date_len);
        if (preview_str) strncpy(preview_str, "No save data", preview_len);
        return;
    }
    
    SaveSlotData *data = &sys->slots[slot];
    
    // Format date
    if (date_str && date_len > 0) {
        struct tm *timeinfo = localtime(&data->timestamp);
        strftime(date_str, date_len, "%Y-%m-%d %H:%M", timeinfo);
    }
    
    // Format preview
    if (preview_str && preview_len > 0) {
        int hours = (int)(data->play_time / 3600);
        int minutes = (int)((data->play_time - hours * 3600) / 60);
        
        snprintf(preview_str, preview_len, 
                 "Lvl %d | %s | %02d:%02d",
                 data->current_level,
                 data->defeated_naberius ? "Cleared" : "In Progress",
                 hours, minutes);
    }
}

const char* save_system_get_empty_slot_text(void) {
    return "EMPTY SLOT";
}

const char* save_system_get_occupied_slot_text(int slot, SaveSlotData *data) {
    static char buffer[64];
    int hours = (int)(data->play_time / 3600);
    int minutes = (int)((data->play_time - hours * 3600) / 60);
    
    snprintf(buffer, sizeof(buffer), "SLOT %d - Lvl %d - %02d:%02d",
             slot + 1, data->current_level, hours, minutes);
    return buffer;
}