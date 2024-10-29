#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <wchar.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <string.h>
#include <ncurses.h>
#include <stdio.h>
#include <sys/time.h>

volatile uint8_t running = 1;  // Shared variable to control the loop
volatile uint8_t last_key = '\0'; // Shared variable to store the last key pressed
pthread_mutex_t key_mutex;  // Mutex for synchronizing access to `last_key`
struct winsize w;
struct timespec time_s, time_c;

typedef enum {
    pterodactyl = 1,
} enemy_type;

typedef struct {
    uint8_t height;
    uint8_t weight;
    uint8_t size;

    int8_t y;
    int8_t dy;
    uint8_t step;

    uint32_t *screen;
    struct {
        enemy_type type;
        int32_t pos_y;
        int32_t pos_x;
    } enemy[2];
} game_t;

// Input thread to capture key presses
void* input_thread(void* _) {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);  // Disable canonical mode and echo
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

// int buf[21][20] = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0,},
//                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,},
//                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
//                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
//                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,},
//                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,},
//                {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,},
//                {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,},
//                {1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,},
//                {1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,},
//                {1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0,},
//                {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,},
//                {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,},
//                {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,},
//                {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,},
//                {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,},
//                {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
//                {0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
//                {0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
//                {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
//                {0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,}};



uint32_t idle[24] = {
    0x00000000, 0x4bb80000, 0x0000c708, 0x00000000,
    0x00000000, 0xffff3b08, 0x00037fff, 0x00000000,
    0x00000000, 0xfff4e6ff, 0x19ffffff, 0x00000000,
    0x00000000, 0x00000044, 0x13ffffe0, 0x00000012,
    0x00000000, 0x00000000, 0xfffeff00, 0x0000ffff,
    0x00000000, 0x00000000, 0xc0c08000, 0x000040c0,
};
uint32_t run_1[24] = {
    0x00000000, 0x2f180000, 0x0000c708, 0x00000000,
    0x00000000, 0xffff3b08, 0x00037fff, 0x00000000,
    0x00000000, 0xfff4e6ff, 0x19ffffff, 0x00000000,
    0x00000000, 0x00000044, 0x13ffffe0, 0x00000012,
    0x00000000, 0x00000000, 0xfffeff00, 0x0000ffff,
    0x00000000, 0x00000000, 0xc0c08000, 0x000040c0,
};
uint32_t run_2[24] = {
    0x00000000, 0x4bb80000, 0x00021308, 0x00000000,
    0x00000000, 0xffff3b08, 0x00037fff, 0x00000000,
    0x00000000, 0xfff4e6ff, 0x19ffffff, 0x00000000,
    0x00000000, 0x00000044, 0x13ffffe0, 0x00000012,
    0x00000000, 0x00000000, 0xfffeff00, 0x0000ffff,
    0x00000000, 0x00000000, 0xc0c08000, 0x000040c0,
};
uint32_t death[24] = {
    0x00000000, 0x4bb80000, 0x0000c708, 0x00000000,
    0x00000000, 0xffff3b08, 0x00037fff, 0x00000000,
    0x00000000, 0xfff4e6ff, 0x19ffffff, 0x00000000,
    0x00000000, 0x00000044, 0x1bffffe0, 0x0000091b,
    0x00000000, 0x00000000, 0xf8d0ff00, 0x0000ffff,
    0x00000000, 0x00000000, 0xc0c08000, 0x000040c0,
};
uint32_t down_1[24] = {
    0x4bb80000, 0x18021300, 0x00000002, 0x00000000,
    0xff3b0800, 0xffffffff, 0x2f3f3b5f, 0x0000092d,
    0xfef6f73b, 0xffffffff, 0xfffdfef6, 0x0000f7ff,
    0x00000040, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};
uint32_t down_2[24] = {
    0x12180000, 0x18004bb8, 0x00000002, 0x00000000,
    0xffbb0800, 0xffffffff, 0x2f3f3b5f, 0x0000092d,
    0xfef6f73b, 0xffffffff, 0xfffdfef6, 0x0000f7ff,
    0x00000040, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};
uint32_t pterodactyl_1[15] = {
    0x18000000, 0x00000001, 0x00000000,
    0xb8000000, 0x09090b7f, 0x00000001,
    0xbb080000, 0xf7ffffff, 0x0002d2f6,
    0xc6fffee0, 0x00c0c0c0, 0x00000000,
    0x00c00000, 0x00000000, 0x00000000,
};
uint32_t pterodactyl_2[15] = {
    0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x09090908, 0x00000001,
    0x3b080000, 0xf7ffffff, 0x0002d2f6,
    0xc6fffee0, 0x00e6ffff, 0x00000000,
    0x38c00000, 0x000040e6, 0x00000000,
};
uint32_t ground[] = {
    0x20000000, 0x00000424, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00200000, 0x40c0c080, 0x00000024, 0x00000000, 0x00000000, 0x00000000, 0x20021212, 0xc0800004, 0x00000040, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x12121000, 0x00000000, 0x000040c0, 0x00242400, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x12121000, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00242400,
    0x02121200, 0x80000000, 0x000000c0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00002400,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x24120800, 0x12242424, 0x00000001, 0x00000000, 0x00000000, 0x00000000,

    0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0,
    0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0,
    0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0,
    0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0, 0x0a14c0c0, 0x84120909, 0xc0c0c0c0, 0x000040c0, 0x00000000, 0xc0c0c080, 0xc0c0c0c0, 0xc0c0c0c0, 0xc0c0c0c0,
};


uint32_t mask = 0b01010101;

uint32_t spread_bits(const uint32_t x) {
    return (x & 0b00000001) | ((x & 0b00000100) << 6) | ((x & 0b00010000) << 12) | ((x & 0b01000000) << 18);
}

void print_dina(const game_t *game) {
    system("clear");
    for (int y = game->height - 1; y >= 0; --y) {
        const uint8_t *screen_raw = (uint8_t *)game->screen + y * game->weight;
        for (int x = 0; x < game->weight; ++x) {
            wprintf(L"%lc", 0x2800 | screen_raw[x]);
        }
        wprintf(L"\n");
    }
}

void update_console_events(game_t *game) {
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) return;
    if (w.ws_col == 0) return;
    game->height = w.ws_row;
    game->weight = w.ws_col;
    if (game->weight * game->height > game->size) {
        if (game->screen != NULL) free(game->screen);
        game->screen = malloc(game->weight * game->height * sizeof(uint32_t));
    }
    memset(game->screen, 0, game->weight * game->height * sizeof(uint32_t));
    game->size = game->weight * game->height;

    clock_gettime(CLOCK_MONOTONIC_RAW, &time_c);
    uint32_t score = (time_c.tv_sec - time_s.tv_sec) * 10000 + (time_c.tv_nsec - time_s.tv_nsec) / 100000;
    uint32_t speed = 3 + score / 300;
    if (speed > 7) speed = 7;

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
        default:;
    }

    game->y = game->y + game->dy > 0? game->y + game->dy-- : 0;
    if (crouch && game->dy != 0) --game->dy;
    if (game->y == 0) game->dy = space ? 5 : 0;
    game->step = (game->step + 1) % 4;


    const uint32_t *tile = idle;
    if (crouch) tile = game->step & 1 ? down_1 : down_2;
    else tile = game->step & 1 ? run_1 : run_2;

    // Dino

    for (int e = 0; e < 1; ++e) {
        if (game->enemy[e].type == 0) {
            // TODO random enemy;
            game->enemy[e].type = pterodactyl;
            game->enemy[e].pos_x = game->weight;
            if (game->enemy[e].type == pterodactyl)
                game->enemy[e].pos_y = (rand() & 1) ? 5 : 0;
            continue;
        }
        uint8_t ok = 1;
        for (int y = 0; y < 5; ++y) {
            const uint8_t *enemy_raw = (uint8_t *) ((game->step ? pterodactyl_1 : pterodactyl_2) + y * 3);
            uint8_t *screen_raw = (uint8_t *) (game->screen) + (y + game->enemy[e].pos_y) * game->weight;
            for (int x = 0; x < 3 * 4; ++x) {
                if (x + game->enemy[e].pos_x < 0) continue;
                ok = 0;
                if (x + game->enemy[e].pos_x >= w.ws_col) continue;
                screen_raw[x + game->enemy[e].pos_x] |= enemy_raw[x];
            }
        }
        game->enemy[e].pos_x -= speed;
        if (ok) game->enemy[e].type = 0;
    }

    for (int y = 0; y < 6; ++y) {
        uint8_t *dino_raw = (uint8_t *) (tile + y * 4);
        uint8_t *screen_raw = (uint8_t *) (game->screen) + (y + game->y) * game->weight;
        for (int x = 0; x < 4 * 4; ++x) {
            if (screen_raw[x] & dino_raw[x]) {
                running = 0;
            }
            screen_raw[x] |= dino_raw[x];
        }
    }
}

// Drawing thread to simulate console drawing
void* drawing_thread(void* arg) {
    game_t game = {0,0,0, 0, 0, 0, NULL, {0, 0, 0}};
    while (running) {
        usleep(75000);
        update_console_events(&game);
        print_dina(&game);
    }
    return NULL;
}


uint8_t pt_2[21 * 20] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,


};

uint8_t buf[1000][1000];
uint8_t res[1000][1000];


void print_dina_make_objects(uint8_t *buffer, int h, int w) {
    // system("clear");

    int r_h = h / 4 + (h % 4 != 0);
    int r_w = w / 2 + (w % 2 != 0);
    int _h = r_h * 4;
    int _w = r_w * 2;
    for (int i = 0; i < r_h; ++i) {
        for (int j = 0; j < r_w; ++j) {
            buf[i][j] = 0;
        }
    }

    for (int i = 0; i < _h; ++i) {
        for (int j = 0; j < _w; ++j) {
            res[i][j] = 0;
        }
    }

    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            buf[h - i - 1][j] = buffer[i * w + j];
        }
    }


    for (int i = 0; i < _h; i += 4) {
        for (int j = 0; j < _w;  j += 2) {
            res[i / 4][j / 2] |= buf[i + 0][j + 0] << 6;
            res[i / 4][j / 2] |= buf[i + 0][j + 1] << 7;
            res[i / 4][j / 2] |= buf[i + 1][j + 0] << 2;
            res[i / 4][j / 2] |= buf[i + 1][j + 1] << 5;
            res[i / 4][j / 2] |= buf[i + 2][j + 0] << 1;
            res[i / 4][j / 2] |= buf[i + 2][j + 1] << 4;
            res[i / 4][j / 2] |= buf[i + 3][j + 0] << 0;
            res[i / 4][j / 2] |= buf[i + 3][j + 1] << 3;
        }
    }

    for (int i = 0; i < r_h; ++i) {
        for (int j = 0; j < r_w / 4 + (r_w % 4 != 0); ++j) {
            printf("0x%.8x, ", ((uint32_t *)res[i])[j]);
        }
        printf("\n");
        // for (int j = 0; j < r_w; ++j) {
        //     wprintf(L"%lc", 0x2800 | res[i][j]);
        // }
        // wprintf(L"\n");
    }
}
int main() {
    clock_gettime(CLOCK_MONOTONIC_RAW, &time_s);
    setlocale(LC_CTYPE, "");

    // print_dina_make_objects(pt_2, 21, 20);
    // printf("\n");

    pthread_t input_tid, draw_tid;
    pthread_mutex_init(&key_mutex, NULL);

    // Create threads
    pthread_create(&input_tid, NULL, input_thread, NULL);
    pthread_create(&draw_tid, NULL, drawing_thread, NULL);

    // Wait for threads to finish
    pthread_join(input_tid, NULL);
    pthread_join(draw_tid, NULL);

    pthread_mutex_destroy(&key_mutex);
    printf("Program exited.\n");
    return 0;
}