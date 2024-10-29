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
// Function to detect key press without blocking
uint8_t keyboard_hit(void) {
    struct termios oldt, newt;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);  // Disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    const uint32_t oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    const int32_t ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

// Input thread to capture key presses
void* input_thread(void* _) {
    while (running) {
        if (!keyboard_hit()) continue;
        pthread_mutex_lock(&key_mutex);
        last_key = (char) getchar();
        pthread_mutex_unlock(&key_mutex);
    }
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



uint32_t idle[6][4] = {
    0x00000000, 0x07060406, 0x60202030, 0x00000000,
    0x00000000, 0x7f3f1f0f, 0xf8f8f0e0, 0x00000000,
    0x00000000, 0xc3e7ffff, 0xfffdfcfc, 0x00000000,
    0x00000000, 0x00008080, 0x3e3f7cfc, 0x00c00000,
    0x00000000, 0x00000000, 0x373f3f3f, 0xf0f0f0f0,
    0x00000000, 0x00000000, 0x0000001f, 0x000000e0,
};
uint32_t run_1[6][4] = {
    0x00000000, 0x07060406, 0x60380000, 0x00000000,
    0x00000000, 0x7f3f1f0f, 0xf8f8f0e0, 0x00000000,
    0x00000000, 0xc3e7ffff, 0xfffdfcfc, 0x00000000,
    0x00000000, 0x00008080, 0x3e3f7cfc, 0x00c00000,
    0x00000000, 0x00000000, 0x373f3f3f, 0xf0f0f0f0,
    0x00000000, 0x00000000, 0x0000001f, 0x000000e0,
};
uint32_t run_2[6][4] = {
    0x00000000, 0x07060300, 0x60202030, 0x00000000,
    0x00000000, 0x7f3f1f0f, 0xf8f8f0e0, 0x00000000,
    0x00000000, 0xc3e7ffff, 0xfffdfcfc, 0x00000000,
    0x00000000, 0x00008080, 0x3e3f7cfc, 0x00c00000,
    0x00000000, 0x00000000, 0x373f3f3f, 0xf0f0f0f0,
    0x00000000, 0x00000000, 0x0000001f, 0x000000e0,
};
uint32_t death[6][4] = {
    0x00000000, 0x07060406, 0x60202030, 0x00000000,
    0x00000000, 0x7f3f1f0f, 0xf8f8f0e0, 0x00000000,
    0x00000000, 0xc3e7ffff, 0xfffdfcfc, 0x00000000,
    0x00000000, 0x00008080, 0x3f3f7cfc, 0xf0c00000,
    0x00000000, 0x00000000, 0x3135313f, 0xf0f0f0f0,
    0x00000000, 0x00000000, 0x0000001f, 0x000000e0,
};
uint32_t down_1[6][4] = {
    0x04070000, 0x71614060, 0x00800000, 0x00000000,
    0x1f0f0707, 0xffffffff, 0xfffe9f80, 0xf000c000,
    0xe1ff7f3f, 0xffffffff, 0x1ff7ffff, 0xe0f0f0f0,
    0x00000080, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    };
uint32_t down_2[6][4] = {
    0x07060406, 0x21390000, 0x00800000, 0x00000000,
    0x1f0f0703, 0xffffffff, 0xfffe9f80, 0xf000c000,
    0xe1ff7f3f, 0xffffffff, 0x1ff7ffff, 0xe0f0f0f0,
    0x00000080, 0x00000000, 0x00000000, 0x00000000,
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
            data.char4 = 0;
            data.char4 |= spread_bits(game->screen[y * game->weight + x] & mask) << 7;
            data.char4 |= spread_bits((game->screen[y * game->weight + x] >> 1) & mask) << 6;
            data.char4 |= spread_bits((game->screen[y * game->weight + x] >> 8) & mask) << 5;
            data.char4 |= spread_bits((game->screen[y * game->weight + x] >> 9) & mask) << 2;
            data.char4 |= spread_bits((game->screen[y * game->weight + x] >> 16) & mask) << 4;
            data.char4 |= spread_bits((game->screen[y * game->weight + x] >> 17) & mask) << 1;
            data.char4 |= spread_bits((game->screen[y * game->weight + x] >> 24) & mask) << 3;
            data.char4 |= spread_bits((game->screen[y * game->weight + x] >> 25) & mask);

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

    const uint32_t (*tile)[4] = idle;
    if (crouch) tile = game->step ? down_1 : down_2;
    else tile = game->step ? run_1 : run_2;

    for (int y = 0; y < 6; ++y) {
        for (int x = 0; x < 4; ++x) {
            game->screen[(y + game->y) * game->weight + x] = tile[y][x];
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
    // print_dina_loh(down_1);
    // printf("\n");
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

