#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <stdarg.h>
#include "../firmware/scenes/games/celeste_classic.h"

// Define stubs for assets included in celeste_classic.c
#include "../firmware/scenes/games/tilemap.h"

#define NUM_ROOMS 8
#define ACTION_REPEAT 4          // 4-frame action repeat (~0.13s each)
#define MAX_STEPS 225            // 225 * 4 = 900 ticks (30 seconds)
#define POPULATION_SIZE 200
#define NUM_ACTIONS 12
#define ELITE_PCT 10             // keep top 10%
#define PARENT_PCT 35            // breed from top 35%
#define IMMIGRANT_PCT 15         // replace bottom 15% with fresh randoms every 25 gens
#define IMMIGRANT_INTERVAL 25    // inject immigrants every N generations
#define MUTATION_RATE 0.10f
#define MAX_GENERATIONS 2000

typedef struct {
    unsigned char actions[MAX_STEPS];
    float fitness;
    int ticks_taken;
    bool cleared;
} Individual;

static Individual population[POPULATION_SIZE];
static Individual next_population[POPULATION_SIZE];
static unsigned char best_sequences[NUM_ROOMS][MAX_STEPS];
static int best_sequence_lengths[NUM_ROOMS];
static bool room_solved_flags[NUM_ROOMS];

// Current virtual button inputs for the callback
static bool sim_buttons[6] = {false};

// PICO-8 Callback stub for training
int train_p8_callback(CELESTE_P8_CALLBACK_TYPE calltype, ...) {
    va_list args;
    va_start(args, calltype);
    int ret = 0;
    switch (calltype) {
        case CELESTE_P8_BTN: {
            int b = va_arg(args, int);
            if (b >= 0 && b < 6) ret = sim_buttons[b];
            break;
        }
        case CELESTE_P8_MGET: {
            int tx = va_arg(args, int);
            int ty = va_arg(args, int);
            if (tx >= 0 && tx < 128 && ty >= 0 && ty < 64)
                ret = tilemap_data[tx + ty * 128];
            break;
        }
        case CELESTE_P8_FGET: {
            int tile = va_arg(args, int);
            int flag = va_arg(args, int);
            ret = !!(tile_flags[tile] & (1 << flag));
            break;
        }
        default: break;
    }
    va_end(args);
    return ret;
}

void apply_action(int action) {
    memset(sim_buttons, 0, sizeof(sim_buttons));
    // btn: 0=L 1=R 2=Up 3=Down 4=Jump 5=Dash
    // 0=noop 1=L 2=R 3=Jump 4=Dash 5=L+J 6=R+J 7=L+D 8=R+D 9=Up+D 10=Up+L+D 11=Up+R+D
    switch (action) {
        case 1:  sim_buttons[0] = true; break;
        case 2:  sim_buttons[1] = true; break;
        case 3:  sim_buttons[4] = true; break;
        case 4:  sim_buttons[5] = true; break;
        case 5:  sim_buttons[0] = true; sim_buttons[4] = true; break;
        case 6:  sim_buttons[1] = true; sim_buttons[4] = true; break;
        case 7:  sim_buttons[0] = true; sim_buttons[5] = true; break;
        case 8:  sim_buttons[1] = true; sim_buttons[5] = true; break;
        case 9:  sim_buttons[2] = true; sim_buttons[5] = true; break;  // Up+Dash
        case 10: sim_buttons[0] = true; sim_buttons[2] = true; sim_buttons[5] = true; break; // Up+L+D
        case 11: sim_buttons[1] = true; sim_buttons[2] = true; sim_buttons[5] = true; break; // Up+R+D
        // case 0: noop
    }
}

int compare_desc(const void *a, const void *b) {
    float fa = ((Individual *)a)->fitness;
    float fb = ((Individual *)b)->fitness;
    return (fa < fb) - (fa > fb); // descending
}

void randomize(Individual *ind) {
    for (int s = 0; s < MAX_STEPS; s++)
        ind->actions[s] = rand() % NUM_ACTIONS;
    ind->fitness = -99999.0f;
    ind->cleared = false;
    ind->ticks_taken = MAX_STEPS * ACTION_REPEAT;
}

float evaluate(Individual *ind, void *start_state, int r) {
    Celeste_P8_load_state(start_state);
    float min_y = 9999.0f;
    float npx = 8.0f, npy = 96.0f;
    bool cleared = false;
    int ticks = 0;

    for (int s = 0; s < MAX_STEPS && !cleared && npx >= 0; s++) {
        apply_action(ind->actions[s]);
        for (int f = 0; f < ACTION_REPEAT; f++) {
            Celeste_P8_update();
            ticks++;
            float nvx, nvy;
            int ndjump, nroom_x, nroom_y;
            bool non_ground, nwall_left, nwall_right;
            get_celeste_player_state(&npx, &npy, &nvx, &nvy, &ndjump,
                                    &non_ground, &nwall_left, &nwall_right,
                                    &nroom_x, &nroom_y);
            // Check room transition BEFORE checking alive
            if (nroom_x != r) { cleared = true; break; }
            if (npx < 0) break; // died
            if (npy < min_y) min_y = npy;
        }
    }

    ind->cleared = cleared;
    ind->ticks_taken = ticks;
    if (cleared) return 100000.0f - (float)ticks;
    // No death penalty: reward height reached regardless of how episode ended
    return (min_y < 9999.0f) ? -min_y : -9999.0f;
}

int main() {
    srand((unsigned)time(NULL));
    Celeste_P8_set_call_func(train_p8_callback);

    size_t state_size = Celeste_P8_get_state_size();
    void *start_state = malloc(state_size);

    printf("Starting Evolutionary Policy Search on %d rooms...\n", NUM_ROOMS);
    fflush(stdout);

    for (int r = 0; r < NUM_ROOMS; r++) {
        printf("--- Training Room (%d, 0) ---\n", r); fflush(stdout);

        // Initialize room and wait for player to fully spawn
        Celeste_P8_init();
        Celeste_P8__DEBUG();
        Celeste_P8_debug_load_room(r, 0);

        float px = -1, py, vx, vy;
        int djump, room_x, room_y;
        bool on_ground, wall_left, wall_right;
        for (int t = 0; t < 200 && px < 0; t++) {
            Celeste_P8_update();
            get_celeste_player_state(&px, &py, &vx, &vy, &djump, &on_ground,
                                    &wall_left, &wall_right, &room_x, &room_y);
        }
        Celeste_P8_save_state(start_state);

        // Random initial population
        for (int i = 0; i < POPULATION_SIZE; i++) randomize(&population[i]);

        bool room_solved = false;
        int stale_since = 0;
        float best_ever = -99999.0f;

        for (int gen = 0; gen < MAX_GENERATIONS && !room_solved; gen++) {
            // Evaluate
            for (int i = 0; i < POPULATION_SIZE; i++)
                population[i].fitness = evaluate(&population[i], start_state, r);

            qsort(population, POPULATION_SIZE, sizeof(Individual), compare_desc);

            if (population[0].fitness > best_ever) {
                best_ever = population[0].fitness;
                stale_since = gen;
            }

            if (gen % 25 == 0) {
                printf("  Gen %d: fitness=%.1f cleared=%s stale=%d\n",
                       gen, population[0].fitness,
                       population[0].cleared ? "YES" : "NO",
                       gen - stale_since);
                fflush(stdout);
            }

            if (population[0].cleared) {
                room_solved = true;
                room_solved_flags[r] = true;
                printf("Room (%d,0) SOLVED gen=%d ticks=%d\n",
                       r, gen, population[0].ticks_taken);
                fflush(stdout);
                memcpy(best_sequences[r], population[0].actions, MAX_STEPS);
                best_sequence_lengths[r] = (population[0].ticks_taken + ACTION_REPEAT - 1) / ACTION_REPEAT;
                break;
            }

            // Breed
            int elite   = POPULATION_SIZE * ELITE_PCT / 100;
            int parents = POPULATION_SIZE * PARENT_PCT / 100;
            memcpy(next_population, population, elite * sizeof(Individual));

            for (int i = elite; i < POPULATION_SIZE; i++) {
                int p1 = rand() % parents;
                int p2 = rand() % parents;
                // Uniform crossover
                for (int s = 0; s < MAX_STEPS; s++)
                    next_population[i].actions[s] = (rand() & 1)
                        ? population[p1].actions[s]
                        : population[p2].actions[s];
                // Mutation
                for (int s = 0; s < MAX_STEPS; s++)
                    if ((float)rand() / RAND_MAX < MUTATION_RATE)
                        next_population[i].actions[s] = rand() % NUM_ACTIONS;
            }

            // Immigrant injection every IMMIGRANT_INTERVAL gens to escape local optima
            if (gen > 0 && gen % IMMIGRANT_INTERVAL == 0) {
                int immigrants = POPULATION_SIZE * IMMIGRANT_PCT / 100;
                int start_idx  = POPULATION_SIZE - immigrants;
                for (int i = start_idx; i < POPULATION_SIZE; i++)
                    randomize(&next_population[i]);
            }

            memcpy(population, next_population, POPULATION_SIZE * sizeof(Individual));
        }

        if (!room_solved) {
            printf("Room (%d,0) NOT solved — saving best partial sequence\n", r);
            // Re-evaluate to get best (already sorted from last gen)
            memcpy(best_sequences[r], population[0].actions, MAX_STEPS);
            best_sequence_lengths[r] = MAX_STEPS;
        }
    }

    printf("Training complete! Writing firmware/scenes/games/celeste_ai_weights.h...\n");

    FILE *f = fopen("firmware/scenes/games/celeste_ai_weights.h", "w");
    if (!f) { perror("open"); free(start_state); return 1; }

    fprintf(f, "// Generated Celeste AI action sequences\n");
    fprintf(f, "#ifndef CELESTE_AI_WEIGHTS_H\n#define CELESTE_AI_WEIGHTS_H\n\n");
    fprintf(f, "#define CELESTE_AI_NUM_ROOMS %d\n", NUM_ROOMS);
    fprintf(f, "#define CELESTE_AI_MAX_STEPS %d\n", MAX_STEPS);
    fprintf(f, "#define CELESTE_AI_REPEAT    %d\n\n", ACTION_REPEAT);

    fprintf(f, "static const int celeste_ai_seq_lens[CELESTE_AI_NUM_ROOMS] = {\n    ");
    for (int r = 0; r < NUM_ROOMS; r++)
        fprintf(f, "%d%s", best_sequence_lengths[r], r < NUM_ROOMS-1 ? ", " : "");
    fprintf(f, "\n};\n\n");

    fprintf(f, "static const bool celeste_ai_room_solved[CELESTE_AI_NUM_ROOMS] = {\n    ");
    for (int r = 0; r < NUM_ROOMS; r++)
        fprintf(f, "%s%s", room_solved_flags[r] ? "true" : "false", r < NUM_ROOMS-1 ? ", " : "");
    fprintf(f, "\n};\n\n");

    fprintf(f, "static const unsigned char celeste_ai_sequences[CELESTE_AI_NUM_ROOMS][CELESTE_AI_MAX_STEPS] = {\n");
    for (int r = 0; r < NUM_ROOMS; r++) {
        fprintf(f, "    {");
        for (int s = 0; s < MAX_STEPS; s++)
            fprintf(f, "%u%s", best_sequences[r][s], s < MAX_STEPS-1 ? "," : "");
        fprintf(f, "}%s\n", r < NUM_ROOMS-1 ? "," : "");
    }
    fprintf(f, "};\n\n#endif\n");
    fclose(f);

    free(start_state);
    printf("Done.\n");
    return 0;
}
