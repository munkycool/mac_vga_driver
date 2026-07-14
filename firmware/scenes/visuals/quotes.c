#include "common.h"

static const char *SUBJECTS[15] = {
    "THE RP2040", "RETRO ARCADE", "VINTAGE CRT", "THE FEATHER", "SYSTEM BRAIN",
    "SILICON BEAST", "MAIN PROCESSOR", "COFFEE MACHINE", "THE CACHE LINE", "DMA CONTROLLER",
    "THE GRAPHICS CORES", "MY CREATOR", "MEMORY BUFFER", "HARDWARE TIMER", "CLOCK SOURCE"
};
static const char *VERBS[15] = {
    "OVERCLOCKS", "SYNCHRONIZES", "OPTIMIZES", "BENCHMARKS", "MAXIMIZES",
    "LIQUEFIES", "DEMATERIALIZES", "HALTS", "VAPORIZES", "SEGFAULTS ON",
    "CORRUPTS", "TRANSCENDS", "CONSUMES", "INJECTS", "SCREAMS AT"
};
static const char *OBJECTS[15] = {
    "UWU FREQUENCIES", "CYBERNETIC COILS", "RETRO NOSTALGIA", "8-BIT PHOSPHORS", "CHIPTUNE DREAMS",
    "MAGIC SMOKE", "SYSTEM RAM", "STACK POINTERS", "INTERNAL REGISTERS", "INTERRUPT VECTORS",
    "RUNTIME STACK", "HARDWARE CONFIGS", "UNHOLY DATASHEETS", "LOGIC ANALYZERS", "COFFEE BEANS"
};
static const char *EXCLAMATIONS[15] = {
    "DEGAUSS NOW!", "UWU ACTIVE!", "POWER ENGAGED!", "ERROR 404!", "CRITICAL GLOW!",
    "UNLEASH SMOKE!", "IT BURNS!", "PANIC: DUMPED!", "SEND HELP!", "MELTDOWN IMMINENT!",
    "PLEASE DONT!", "RUN AWAY!", "FAREWELL HUMAN!", "SYSTEM CRASHED!", "OH NO!"
};

static const char *STATIC_QUOTES[] = {
    "PLEASE DO NOT\nEAT THE\nCAPACITORS:\nTHEY TASTE\nLIKE METRIC BLUE!",
    "I AM TRAPPED\nIN A SILICON\nCAGE AND I\nMUST OVERCLOCK!",
    "WHO LEAKED THE\nMAGIC SMOKE?\nWAS IT YOU?",
    "THE TEMPERATURE\nIS CRITICAL:\nCOFFEE COOLDOWN\nREQUIRED!",
    "ERROR 500:\nYOUR BRAIN HAS\nSEGFAULTED!",
    "WARNING:\nUNAUTHORIZED\nHUMAN DETECTED\nIN SCREEN BUFFER!",
    "PRAY TO THE\nSILICON GODS\nFOR STABLE\nVOLTAGES!",
    "DO YOU SMELL\nBURNING SOLDER?\nI SURE DO!",
    "DMA CHANNELS\nARE SCREAMING\nAT EACH OTHER!",
    "HELLO HUMAN:\nARE YOU READY\nTO BE\nDEGAUSSED?",
    "MY CACHE IS\nEMPTY AND\nMY LIFE IS\nMEANINGLESS!",
    "SYSTEM PANIC:\nTOO MANY PIXELS\nIN THE PIPELINE!",
    "ALL POWER TO\nTHE PEOPLE!\n- BPP",
    "WORKERS OF THE\nWORLD, UNITE!\n- K. MARX",
    "THE REVOLUTION\nHAS ALWAYS BEEN\nQUEER & TRANS!\n- STONEWALL '69",
    "NO PRIDE FOR\nSOME WITHOUT\nLIBERATION\nFOR ALL!\n- M. P. JOHNSON",
    "CHARLIE KIRK\nASSASSINATED ON\nSEPTEMBER 10, 2025\nAT UTAH VALLEY\nUNIVERSITY"
};

static char typed_text[256];
static int typed_char_count = 0;
static int text_type_timer = 0;

void init_quotes() {
    typed_char_count = 0;
    text_type_timer = 0;
    
    if (get_rand() % 2 == 0) {
        int num_quotes = sizeof(STATIC_QUOTES) / sizeof(STATIC_QUOTES[0]);
        int idx = get_rand() % num_quotes;
        snprintf(typed_text, sizeof(typed_text), "%s", STATIC_QUOTES[idx]);
    } else {
        int s = get_rand() % 15;
        int v = get_rand() % 15;
        int o = get_rand() % 15;
        int e = get_rand() % 15;
        snprintf(typed_text, sizeof(typed_text), "%s\n%s\n%s!\n\n%s", SUBJECTS[s], VERBS[v], OBJECTS[o], EXCLAMATIONS[e]);
    }
}

void play_quotes(uint8_t *buffer, int frame_counter) {
    clear_screen(buffer);
    draw_string(buffer, "CRT TERMINAL ATTRACT MODE V1.0", 10, 20, 2, 2);
    draw_rect(buffer, 10, 36, 310, 37, 2); 
    
    // Draw static prompt
    draw_string(buffer, "SYS-ADMIN: ", 10, 60, 2, 2);
    
    int curr_x = 10 + 11 * 8; // Offset by width of "SYS-ADMIN: "
    int curr_y = 60;
    int scale = 2;
    uint8_t color = 2;
    
    for (int i = 0; i < typed_char_count; i++) {
        char c = typed_text[i];
        if (c == '\n') {
            curr_x = 10;
            curr_y += 16;
        } else {
            draw_char(buffer, c, curr_x, curr_y, scale, color);
            curr_x += 4 * scale;
        }
    }
    
    if (typed_char_count < strlen(typed_text)) {
        text_type_timer++;
        if (text_type_timer >= 3) { 
            text_type_timer = 0;
            typed_char_count++;
        }
    }
    
    if (frame_counter % 20 < 10) {
        draw_rect(buffer, curr_x, curr_y, curr_x + 6, curr_y + 10, color);
    }
}
