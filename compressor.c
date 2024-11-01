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

uint8_t pt_2[8 * 232] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
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
        for (int j = 0; j < _w; j += 2) {
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
            printf("0x%.8x, ", ((uint32_t *) res[i])[j]);
        }
        printf("\n");
        // for (int j = 0; j < r_w; ++j) {
        //     wprintf(L"%lc", 0x2800 | res[i][j]);
        // }
        // wprintf(L"\n");
    }
}