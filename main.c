#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <wchar.h>
#include <termios.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include "game_objects.h"
#include <math.h>


volatile uint8_t running = 1; // Shared variable to control the loop
volatile uint8_t last_key = '\0'; // Shared variable to store the last key pressed
pthread_mutex_t key_mutex; // Mutex for synchronizing access to `last_key`
struct winsize w;

typedef enum {
    pterodactyl_type = 1,
    cactus_type_1 = 2,
    cactus_type_2 = 3,
    cactus_type_3 = 4,
    cactus_type_4 = 5,
} enemy_type;

typedef enum {
    state_start = 0,
    state_running = 1,
    state_death = 2,
} game_states;

struct game_t {
    int32_t height;
    int32_t weight;
    uint64_t size;

    game_states state;
    uint64_t time_start;

    uint8_t space;
    uint8_t crouch;

    uint64_t score;
    double_t speed;

    double_t x;
    double_t y;
    int32_t dy;

    uint64_t stamp;

    uint64_t dn_new;
    uint8_t dn_mask;
    uint64_t day_night;
    uint8_t moon_phase;

    uint8_t *screen;
    uint32_t *ground;
    uint64_t ground_size;

    struct enemy_st {
        enemy_type e_type;
        double_t e_x;
        int32_t e_y;
    } enemies[ENEMY_COUNT];

    struct cloud_st {
        uint8_t c_type;
        double_t c_x;
        int32_t c_y;
        double_t c_speed;
    } clouds[CLOUD_COUNT];

} game;


// Input thread to capture key presses
void *input_thread(void *_) {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    while (running) {
        const int32_t ch = getchar();
        if (ch == EOF) continue;

        pthread_mutex_lock(&key_mutex);
        last_key = ch;
        pthread_mutex_unlock(&key_mutex);
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return NULL;
}

uint64_t get_time() {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void print_dina() {
    system("clear");
    for (int y = game.height - 1; y >= 0; --y) {
        const uint8_t *screen_raw = game.screen + y * game.weight;
        for (int x = 0; x < w.ws_col; ++x) {
            wprintf(L"%lc", 0x2800 | (game.dn_mask ^ screen_raw[x]));
        }
        wprintf(L"\n");
    }
}
void keyboard_handler() {
    pthread_mutex_lock(&key_mutex);
    const uint8_t c = last_key; // Get the last key pressed
    last_key = '\0';
    pthread_mutex_unlock(&key_mutex);

    uint8_t space = 0;
    uint8_t crouch = 0;

    switch (c) {
        case 10:
        case 32:
        case 65:
            space = 1;
        break;
        case 66:
            crouch = 1;
        break;
        case 114:
            game.state = state_start;
        default: ;
    }
    if (game.state != state_running && (space || crouch)) {
        game.state = state_running;
        game.time_start = get_time();

        const int32_t center_x = game.weight - 1;
        const int32_t center_y = game.height - 1;
        game.day_night = (uint64_t) ((double) get_time() - sqrt(center_x * center_x + center_y * center_y) * 4);
        game.dn_new = 0;
        game.dn_mask = 0xFF;
        for (int e = 0; e < ENEMY_COUNT; ++e)
            game.enemies[e].e_type = 0;
    }
    game.space = space;
    game.crouch = crouch;
}
void player_movement() {
    const uint64_t score = (get_time() - game.time_start) / 50;
    double_t speed = 3.0 + (double_t) score / 300.0;
    if (speed > 7) speed = 7;
    if (game.state == state_start) speed = 3;
    if (game.state == state_death) {
        game.score = 0;
        game.speed = 0;
        return;
    }

    // Jump Calculations
    if (game.stamp != 0) {
        double_t dt = (double_t) (score - game.stamp) / 2.0;
        dt = (-dt + 8) * dt;
        game.y = dt > 0 ? dt : 0;
        if (game.crouch) game.stamp -= 2;
    }
    if (game.y == 0) game.stamp = game.space ? score - 2 : 0;
    game.score = score;
    game.speed = speed;


    if ((get_time() - game.day_night) / 4 > DAY_LIGHT_TIME) {
        game.day_night = get_time();
        game.dn_new = 1;
    }
}
void draw_ground() {
    const uint64_t size = game.weight / 2 + (game.weight % 2 != 0) + 1;
    if (size > game.ground_size) {
        game.ground_size = size;
        game.ground = malloc(GROUND_H * size * sizeof(uint32_t));
        memset(game.ground, 0, GROUND_H * size * sizeof(uint32_t));
        for (int y = 0; y < GROUND_H; ++y) {
            uint32_t *_ground_raw = &game.ground[y * size];
            for (int x = 0; x < size; ++x) {
                _ground_raw[x] = ground_1[y][(rand() & 0x07) == 0x07 ? rand() & 1 : 2];  // NOLINT(*-msc50-cpp)
            }
        }
    }

    for (int y = 0; y < GROUND_H; ++y) {
        uint32_t *_ground_raw = &game.ground[y * size];
        const uint8_t *ground_raw = (uint8_t *)_ground_raw;
        uint8_t *screen_raw = game.screen + y * game.weight;

        for (int x = 0; x < size * 4 && x < game.weight; ++x) {
            screen_raw[x] |= ground_raw[(int32_t)(x + game.x) % (size * 4)];
        }
        _ground_raw[(int32_t)((double_t) size - 1 + game.x / 4) % size] = ground_1[y][(rand() & 0x07) == 0x07 ? rand() & 1 : 2];  // NOLINT(*-msc50-cpp)
    }
    game.x = (int32_t) ((int32_t)(game.x + game.speed) % (size * 4));
}
void draw_player() {
    const uint32_t *tile_dino = NULL;
    const uint32_t *tile_back = NULL;
    if (game.state != state_death) {
        if (game.crouch) tile_dino = game.score & 2 ? down_1 : down_2;
        else tile_dino = game.score & 2 ? run_1 : run_2;
        if (game.crouch) tile_back = game.score & 2 ? down_1_r : down_2_r;
        else tile_back = game.score & 2 ? run_1_r : run_2_r;
    } else tile_dino = death_1;

    if (tile_back != NULL) {
        for (int y = 0; y < DINO_H && y + 1 + game.y < game.height; ++y) {
            const uint32_t *back_raw = &tile_back[y * DINO_W];
            uint32_t *screen_raw = (uint32_t *) (game.screen + (uint32_t)(y + game.y + 1) * game.weight);
            for (int x = 0; x < DINO_W; ++x) {
                screen_raw[x] &= back_raw[x];
            }
        }
    }

    for (int y = 0; y < DINO_H && y + 1 + game.y < game.height; ++y) {
        const uint32_t *dino_raw = &tile_dino[y * DINO_W];
        uint32_t *screen_raw = (uint32_t *) (game.screen + (uint32_t)(y + game.y + 1) * game.weight);
        for (int x = 0; x < DINO_W; ++x) {
            screen_raw[x] |= dino_raw[x];
        }
    }

}
uint32_t *get_enemy(const enemy_type type, const uint64_t step) {
    switch (type) {
        case pterodactyl_type:
            return step? pterodactyl_1 : pterodactyl_2;
        case cactus_type_1:
            return cactus_1;
        case cactus_type_2:
            return cactus_2;
        case cactus_type_3:
            return cactus_3;
        case cactus_type_4:
            return cactus_4;
        default: return NULL;
    }
}
void draw_enemy() {
    double_t mx = 0;
    for (int e = 0; e < ENEMY_COUNT; ++e)
        if (mx < game.enemies[e].e_x) mx = game.enemies[e].e_x;

    for (int e = 0; e < ENEMY_COUNT; ++e) {
        struct enemy_st *enemy = &game.enemies[e];
        if (enemy->e_type == 0) {
            const int32_t min_distance = game.speed * (ENEMY_MIN_DISTANCE + rand() % 10); // NOLINT(*-narrowing-conversions, *-msc50-cpp)
            enemy->e_type = rand() % ENEMY_TYPES + 1;  // NOLINT(*-msc50-cpp)
            enemy->e_x = game.weight;
            if (enemy->e_x < min_distance + mx) enemy->e_x = min_distance + mx;
            if (mx < enemy->e_x) mx = enemy->e_x;
            enemy->e_y = rand() & 1 && enemy->e_type == pterodactyl_type ? 4 : 0; // NOLINT(*-msc50-cpp)
        }

        uint32_t *enemy_tile = get_enemy(enemy->e_type, game.score & 2);
        if (enemy_tile == NULL) return;

        uint8_t ok = 1;
        for (int y = 0; y < ENEMY_H && y + 1 + enemy->e_y < game.height; ++y) {
            const uint8_t *enemy_raw = (uint8_t *) (enemy_tile + y * ENEMY_W);
            uint8_t *screen_raw = game.screen + (y + enemy->e_y + 1) * game.weight + (int32_t) enemy->e_x;
            for (int x = 0; x < ENEMY_W * 4; ++x) {
                if (x + enemy->e_x < 0) continue;
                ok = 0;
                if (x + enemy->e_x >= w.ws_col) continue;
                screen_raw[x] |= enemy_raw[x];
            }
        }
        enemy->e_x -= game.speed;
        if (ok) enemy->e_type = 0;
    }
}
void cheak_death() {
    if (game.state == state_start) return;
    const uint32_t *tile_dino = NULL;
    if (game.crouch) tile_dino = game.score & 2 ? down_1 : down_2;
    else tile_dino = game.score & 2 ? run_1 : run_2;

    for (int y = 0; y < DINO_H && y + 1 + game.y < game.height; ++y) {
        const uint32_t *dino_raw = &tile_dino[y * DINO_W];
        const uint32_t *screen_raw = (uint32_t *) (game.screen + (uint32_t)(y + game.y + 1) * game.weight);
        for (int x = 0; x < DINO_W; ++x) {
            if (screen_raw[x] & dino_raw[x]) {
                game.state = state_death;
                return;
            }
        }
    }
}
void draw_sky() {
    const uint32_t day_night = (get_time() - game.day_night) / 4;

    const int32_t center_x = game.weight - 1;
    const int32_t center_y = game.height - 1;

    if (sqrt(center_x * center_x + center_y * center_y) < day_night) {
        if (game.dn_new) game.dn_mask ^= 0xFF;
        if (game.dn_new && !game.dn_mask) game.moon_phase = (game.moon_phase + 1) % 7;
        game.dn_new = 0;
        return;
    }

    for (int32_t y = center_y; y >= 0; --y) {
        const int32_t dy = (center_y - y) * (center_y - y);
        uint8_t *screen_raw = game.screen + y * game.weight;
        for (int32_t x = center_x; x >= 0; --x) {
            if (sqrt((center_x - x) * (center_x - x) + dy) > day_night) break;
            screen_raw[x] ^= 0xFF;
        }
    }
}
void draw_clouds() {
    double_t mx = 0;
    for (int e = 0; e < CLOUD_COUNT; ++e)
        if (mx < game.clouds[e].c_x) mx = game.clouds[e].c_x;

    for (int e = 0; e < CLOUD_COUNT; ++e) {
        struct cloud_st *cloud = &game.clouds[e];
        if (cloud->c_type == 0) {
            const int32_t min_distance = game.speed * (CLOUD_MIN_DISTANCE + rand() % 10); // NOLINT(*-narrowing-conversions, *-msc50-cpp)
            cloud->c_type = 1;
            cloud->c_x = (float_t) game.weight;
            if (cloud->c_x < (float_t) min_distance + mx) cloud->c_x = (float_t) min_distance + mx;
            if (mx < cloud->c_x) mx = cloud->c_x;
            cloud->c_y = 10 + rand() % (game.height - CLOUD_H - 10); // NOLINT(*-msc50-cpp)
            cloud->c_speed = 0.1 + rand() % 15 / 10.0; // NOLINT(*-msc50-cpp)
        }

        uint8_t ok = 1;
        for (int y = 0; y < CLOUD_H && y + 1 + cloud->c_y < game.height; ++y) {
            const uint8_t *cloud_raw = (uint8_t *) (cloud_1 + y * CLOUD_W);
            const uint8_t *back_raw = (uint8_t *) (cloud_1r + y * CLOUD_W);
            uint8_t *screen_raw = game.screen + (y + cloud->c_y + 1) * game.weight + (int32_t)cloud->c_x;
            for (int32_t x = 0; x < CLOUD_W * 4; x += 1) {
                if (x + (int32_t)cloud->c_x < 0) continue;
                ok = 0;
                if (x + (int32_t) cloud->c_x >= w.ws_col) continue;
                screen_raw[x] &= back_raw[x];
                screen_raw[x] |= cloud_raw[x];
            }
        }
        cloud->c_x -= cloud->c_speed;
        if (ok) cloud->c_type = 0;
    }
}
uint32_t *get_digit(const int type) {
    switch (type) {
        case 0:
            return digit_0;
        case 1:
            return digit_1;
        case 2:
            return digit_2;
        case 3:
            return digit_3;
        case 4:
            return digit_4;
        case 5:
            return digit_5;
        case 6:
            return digit_6;
        case 7:
            return digit_7;
        case 8:
            return digit_8;
        case 9:
            return digit_9;
        default: return NULL;
    }
}
void draw_score() {
    if (game.state == state_start) return;
    int32_t score = game.score;
    uint32_t X = game.weight - 4;
    while(score) {
        const uint32_t *tile = get_digit(score % 10);
        for (int y = 0; y < DIGIT_H; ++y) {
            uint16_t *screen_raw = (uint16_t *) (game.screen + (game.height - DIGIT_H + y) * game.weight);
            screen_raw[X / 2] |= tile[y * DIGIT_W];
        }
        X -= DIGIT_W * 2;
        score /= 10;
    }
}
uint32_t *get_phase(const int type) {
    switch (type) {
        case 0:
            return phase_1;
        case 1:
            return phase_2;
        case 2:
            return phase_3;
        case 3:
            return phase_4;
        case 4:
            return phase_5;
        case 5:
            return phase_6;
        case 6:
            return phase_7;
        default: return phase_4;
    }
}
void draw_sun_moon() {
    const uint32_t dx = game.weight * (double_t)(get_time() - game.day_night) / DAY_LIGHT_TIME / 4;

    uint32_t *phase = phase_4;
    if (!game.dn_mask) phase = get_phase(game.moon_phase);

    for (uint32_t y = 0; y < PHASE_H; ++y) {
        const uint8_t *phase_raw = (uint8_t *) (phase + y * PHASE_W);
        uint8_t *screen_raw = game.screen + (game.height - PHASE_H + y - 1) * game.weight + (game.weight - dx);
        for (uint32_t x = 0; x < PHASE_W * 4; ++x) {
            if (x + (game.weight - dx) < 0) continue;
            if (x + (game.weight - dx) >= w.ws_col) continue;
            screen_raw[x] |= phase_raw[x];
        }
    }
}

void update_console_events() {
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) return;
    if (w.ws_col == 0) return;
    game.height = w.ws_row > 25 ? 25 : w.ws_row;
    game.weight = (w.ws_col / 4 + (w.ws_col % 4 != 0)) * 4;

    if (game.weight * game.height > game.size) {
        if (game.screen != NULL) free(game.screen);
        game.size = game.weight * game.height;
        game.screen = malloc(game.size);

    }
    memset(game.screen, 0, game.size);

    keyboard_handler();
    player_movement();
    draw_enemy();
    cheak_death();
    draw_ground();
    draw_sun_moon();
    draw_clouds();
    draw_player();
    draw_score();
    draw_sky();
}

// Drawing thread to simulate console drawing
void drawing_thread() {
    while (running) {
        usleep(32000);
        update_console_events();
        print_dina();
    }
}

int main() {
    setlocale(LC_CTYPE, "");
    wprintf(L"\e[?25l");
    pthread_t input_tid;
    pthread_mutex_init(&key_mutex, NULL);

    // Create threads
    pthread_create(&input_tid, NULL, input_thread, NULL);
    drawing_thread(NULL);

    // Wait for threads to finish
    pthread_join(input_tid, NULL);

    pthread_mutex_destroy(&key_mutex);
    printf("Program exited.\n");
    return 0;
}
