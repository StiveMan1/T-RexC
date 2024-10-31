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


volatile uint8_t running = 1; // Shared variable to control the loop
volatile uint8_t last_key = '\0'; // Shared variable to store the last key pressed
pthread_mutex_t key_mutex; // Mutex for synchronizing access to `last_key`
struct winsize w;
uint64_t time_s;

typedef enum {
    pterodactyl_type = 1,
    cactus_type_1 = 2,
    cactus_type_2 = 3,
    cactus_type_3 = 4,
    cactus_type_4 = 5,
} enemy_type;

struct game_t {
    int32_t height;
    int32_t weight;
    uint64_t size;

    uint8_t space;
    uint8_t crouch;

    uint64_t score;
    uint32_t speed;

    int32_t x;
    int32_t y;
    int32_t dy;

    uint64_t stamp;

    uint8_t *screen;
    uint32_t *ground;
    uint64_t ground_size;


    enemy_type e_type;
    int32_t e_x;
    int32_t e_y;
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
        for (int x = 0; x < game.weight; ++x) {
            wprintf(L"%lc", 0x2800 | screen_raw[x]);
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
        default: ;
    }
    game.space = space;
    game.crouch = crouch;
}
void player_movement() {
    const uint64_t score = (get_time() - time_s) / 50;
    uint32_t speed = 3 + score / 300;
    if (speed > 7) speed = 7;

    // Jump Calculations
    if (game.stamp != 0) {
        int32_t dt = (int32_t) (score - game.stamp) / 2;
        dt = (-dt + 8) * dt;
        game.y = dt > 0 ? dt : 0;
        if (game.crouch) game.stamp -= 2;
    }
    if (game.y == 0) game.stamp = game.space ? score - 2 : 0;
    game.score = score;
    game.speed = speed;
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
                _ground_raw[x] = ground[y][(rand() & 0x07) == 0x07 ? rand() & 1 : 2];
            }
        }
    }

    for (int y = 0; y < GROUND_H; ++y) {
        uint32_t *_ground_raw = &game.ground[y * size];
        const uint8_t *ground_raw = (uint8_t *)_ground_raw;
        uint8_t *screen_raw = game.screen + y * game.weight;

        for (int x = 0; x < size * 4 && x < game.weight; ++x) {
            screen_raw[x] |= ground_raw[(x + game.x) % (size * 4)];
        }
        _ground_raw[(size - 1 + game.x / 4) % size] = ground[y][(rand() & 0x07) == 0x07 ? rand() & 1 : 2];
    }
    game.x = (int32_t) ((game.x + game.speed) % (size * 4));
}
void draw_player() {
    const uint32_t *tile_dino = NULL;
    if (game.crouch) tile_dino = game.score & 2 ? down_1 : down_2;
    else tile_dino = game.score & 2 ? run_1 : run_2;
    const uint32_t *tile_back = NULL;
    if (game.crouch) tile_back = game.score & 2 ? down_1_r : down_2_r;
    else tile_back = game.score & 2 ? run_1_r : run_2_r;

    for (int y = 0; y < DINO_H; ++y) {
        const uint32_t *dino_raw = &tile_dino[y * DINO_W];
        const uint32_t *back_raw = &tile_back[y * DINO_W];
        uint32_t *screen_raw = (uint32_t *) (game.screen + (y + game.y + 1) * game.weight);
        for (int x = 0; x < DINO_W; ++x) {
            screen_raw[x] &= back_raw[x];
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
    if (game.e_type == 0) {
        // TODO random enemy;
        game.e_type = rand() % ENEMY_TYPES + 1;
        game.e_x = game.weight;
        game.e_y = (rand() & 1) && game.e_type == pterodactyl_type ? 5 : 0;
    }

    uint32_t *enemy_tile = get_enemy(game.e_type, game.score & 2);
    if (enemy_tile == NULL) return;

    uint8_t ok = 1;
    for (int y = 0; y < ENEMY_H; ++y) {
        const uint8_t *enemy_raw = (uint8_t *) (enemy_tile + y * ENEMY_W);
        uint8_t *screen_raw = game.screen + (y + game.e_y + 1) * game.weight;
        for (int x = 0; x < ENEMY_W * 4; ++x) {
            if (x + game.e_x < 0) continue;
            ok = 0;
            if (x + game.e_x >= w.ws_col) continue;
            screen_raw[x + game.e_x] |= enemy_raw[x];
        }
    }
    game.e_x -= (int32_t) game.speed;
    if (ok) game.e_type = 0;
}

void update_console_events() {
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) return;
    if (w.ws_col == 0) return;
    game.height = w.ws_row;
    game.weight = w.ws_col;

    if (game.weight * game.height > game.size) {
        if (game.screen != NULL) free(game.screen);
        game.size = game.weight * game.height;
        game.screen = malloc(game.size);

    }
    memset(game.screen, 0, game.size);

    keyboard_handler(game);
    player_movement(game);
    draw_ground(game);
    draw_player(game);
    draw_enemy(game);
}

// Drawing thread to simulate console drawing
void *drawing_thread(void *arg) {
    while (running) {
        usleep(32000);
        update_console_events(&game);
        print_dina(&game);
    }
    return NULL;
}

int main() {
    time_s = get_time();
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
