/*
 * Google T-Rex Console Game - Object Rendering Module
 * ---------------------------------------------------
 * This module provides functions to render objects, specifically the T-Rex 
 * and obstacles, in a console environment. The code uses pixel compression 
 * techniques to display objects in a reduced resolution, suitable for a 
 * text-based interface.
 *
 * Key Components:
 * - buf[][]: A temporary buffer to store pixel data before compression.
 * - res[][]: A buffer to store compressed pixel data for display.
 *
 * Functions:
 * - compress_objects(): This function accepts a pixel buffer along 
 *   with its height and width, compresses each 2x4 block into a single byte, 
 *   and outputs the result as hexadecimal values, which can be used to display 
 *   objects on the console.
 *
 * Usage Notes:
 * - This module requires a terminal that supports Unicode Braille patterns 
 *   for displaying compressed graphics.
 * - Uncomment the wprintf lines within compress_objects() to display 
 *   the object as Braille characters in the console.
 *
 * Author: Sanzhar Zhanalin
 * Date: 02.11.2024
 */

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

// Buffer arrays for drawing objects and storing results.
uint8_t buf[1000][1000];
uint8_t res[1000][1000];

// Function to compress and print the objects
void compress_objects(uint8_t *buffer, int h, int w) {
    // Variables to calculate reduced dimensions (each cell represents a 2x4 grid).
    int r_h = h / 4 + (h % 4 != 0);  // Reduced height
    int r_w = w / 2 + (w % 2 != 0);  // Reduced width
    int _h = r_h * 4;                // Expanded height based on reduction factor
    int _w = r_w * 2;                // Expanded width based on reduction factor

    // Initialize the buffer for the reduced resolution
    for (int i = 0; i < r_h; ++i) {
        for (int j = 0; j < r_w; ++j) {
            buf[i][j] = 0;
        }
    }

    // Initialize the result matrix to zero
    for (int i = 0; i < _h; ++i) {
        for (int j = 0; j < _w; ++j) {
            res[i][j] = 0;
        }
    }

    // Copy data from input buffer to the drawing buffer, flipping vertically.
    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            buf[h - i - 1][j] = buffer[i * w + j];
        }
    }

    // Compress 2x4 pixels into a single byte for the result matrix using bit-shifting.
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

    // Print the compressed result in hexadecimal format.
    for (int i = 0; i < r_h; ++i) {
        for (int j = 0; j < r_w / 4 + (r_w % 4 != 0); ++j) {
            printf("0x%.8x, ", ((uint32_t *) res[i])[j]);
        }
        printf("\n");
        // Uncomment this to display the result as Unicode Braille characters.
        // for (int j = 0; j < r_w; ++j) {
        //     wprintf(L"%lc", 0x2800 | res[i][j]);
        // }
        // wprintf(L"\n");
    }
}