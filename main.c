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

volatile uint8_t running = 1;  // Shared variable to control the loop
volatile uint8_t last_key = '\0'; // Shared variable to store the last key pressed
pthread_mutex_t key_mutex;  // Mutex for synchronizing access to `last_key`
struct winsize w;

typedef struct {
    uint8_t height;
    uint8_t weight;
    uint8_t size;

    int8_t y;
    int8_t dy;
    uint8_t step;

    uint32_t *screen;
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
    0x00000000, 0x0000b84b, 0x08c70000, 0x00000000,
    0x00000000, 0x083bffff, 0xff7f0300, 0x00000000,
    0x00000000, 0xffe6f4ff, 0xffffff19, 0x00000000,
    0x00000000, 0x44000000, 0xe0ffff13, 0x12000000,
    0x00000000, 0x00000000, 0x00fffeff, 0xffff0000,
    0x00000000, 0x00000000, 0x0080c0c0, 0xc0400000,
};
uint32_t run_1[24] = {
    0x00000000, 0x0000b84b, 0x08130200, 0x00000000,
    0x00000000, 0x083bffff, 0xff7f0300, 0x00000000,
    0x00000000, 0xffe6f4ff, 0xffffff19, 0x00000000,
    0x00000000, 0x44000000, 0xe0ffff13, 0x12000000,
    0x00000000, 0x00000000, 0x00fffeff, 0xffff0000,
    0x00000000, 0x00000000, 0x0080c0c0, 0xc0400000,
};
uint32_t run_2[24] = {
    0x00000000, 0x0000182f, 0x08c70000, 0x00000000,
    0x00000000, 0x083bffff, 0xff7f0300, 0x00000000,
    0x00000000, 0xffe6f4ff, 0xffffff19, 0x00000000,
    0x00000000, 0x44000000, 0xe0ffff13, 0x12000000,
    0x00000000, 0x00000000, 0x00fffeff, 0xffff0000,
    0x00000000, 0x00000000, 0x0080c0c0, 0xc0400000,
};
uint32_t death[24] = {
    0x00000000, 0x0000b84b, 0x08c70000, 0x00000000,
    0x00000000, 0x083bffff, 0xff7f0300, 0x00000000,
    0x00000000, 0xffe6f4ff, 0xffffff19, 0x00000000,
    0x00000000, 0x44000000, 0xe0ffff1b, 0x1b090000,
    0x00000000, 0x00000000, 0x00ffd0f8, 0xffff0000,
    0x00000000, 0x00000000, 0x0080c0c0, 0xc0400000,
};
uint32_t down_1[24] = {
    0x00001812, 0xb84b0018, 0x02000000, 0x00000000,
    0x0008bbff, 0xffffffff, 0x5f3b3f2f, 0x2d090000,
    0x3bf7f6fe, 0xffffffff, 0xf6fefdff, 0xfff70000,
    0x40000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};
uint32_t down_2[24] = {
    0x0000b84b, 0x00130218, 0x02000000, 0x00000000,
    0x00083bff, 0xffffffff, 0x5f3b3f2f, 0x2d090000,
    0x3bf7f6fe, 0xffffffff, 0xf6fefdff, 0xfff70000,
    0x40000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};
uint32_t mask = 0b01010101;

uint32_t spread_bits(const uint32_t x) {
    return (x & 0b00000001) | ((x & 0b00000100) << 6) | ((x & 0b00010000) << 12) | ((x & 0b01000000) << 18);
}

void print_dina(game_t *game) {
    system("clear");
    for (int y = game->height - 1; y >= 0; --y) {
        for (int x = 0; x < game->weight; ++x) {
            union {
                uint8_t str[4];
               uint32_t char4;
            } data;
            data.char4 = game->screen[y * game->weight + x];
            wprintf(L"%lc%lc%lc%lc", 0x2800 | data.str[3], 0x2800 | data.str[2], 0x2800 | data.str[1], 0x2800 | data.str[0]);
        }
        wprintf(L"\n");
    }
}


// void main() {
//     setlocale(LC_CTYPE, "");
//     // setlocale(LC_ALL, "");
//     // printf("\033[?25l"); // hide the cursor
//
//     getchar();
//     while (1) {
//         usleep(100000);
//         print_dina_loh(run_1);
//         usleep(100000);
//         print_dina_loh(run_2);
//     }
//
//
//
//     // printf("\033[?25h"); // show the cursor
// }

// Enter 10
// Space 32
// Up 65
// Down 66
// ESC 27
// R 114

void update_console_events(game_t *game) {
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) return;
    game->height = w.ws_row;
    game->weight = w.ws_col >> 2;
    if (game->weight * game->height > game->size) {
        if (game->screen != NULL) free(game->screen);
        game->screen = malloc(game->weight * game->height * sizeof(uint32_t));
    }
    memset(game->screen, 0, game->weight * game->height * sizeof(uint32_t));
    game->size = game->weight * game->height;


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
    game->step ^= 1;

    const uint32_t *tile = idle;
    if (crouch) tile = game->step ? down_1 : down_2;
    else tile = game->step ? run_1 : run_2;

    for (int y = 0; y < 6; ++y) {
        for (int x = 0; x < 4; ++x) {
            game->screen[(y + game->y) * game->weight + x] = tile[y * 4 + x];
        }
    }
}

// Drawing thread to simulate console drawing
void* drawing_thread(void* arg) {
    game_t game = {0,0,0, 0, 0, 0, NULL};
    while (running) {
        usleep(50000);  // Refresh every 0.5 seconds

        // Get the terminal size
        update_console_events(&game);
        print_dina(&game);
    }
    return NULL;
}

int main() {
    setlocale(LC_CTYPE, "");
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





/* -------------------- Tile
 *            00000000
 *           00 0000000
 *           0000000000
 *           0000000000
 *           0000000000
 *           00000
 *           00000000
 * 0        00000
 * 0       000000
 * 00    0000000000
 * 000  000000000 0
 * 00000000000000
 * 00000000000000
 *  000000000000
 *   00000000000
 *    000000000
 *     0000000
 *      000 00
 *      00   0
 *      0    0
 *      00   00
*/// w - 20 h - 21

/* -------------------- Tile
 *            00000000
 *           00   00000
 *           00 0 00000
 *           00   00000
 *           0000000000
 *           0000000000
 *           00000000
 * 0        00000
 * 0       000000
 * 00    0000000000
 * 000  000000000 0
 * 00000000000000
 * 00000000000000
 *  000000000000
 *   00000000000
 *    000000000
 *     0000000
 *      000 00
 *      00   0
 *      0    0
 *      00   00
*/// w - 20 h - 21

