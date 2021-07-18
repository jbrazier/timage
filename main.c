/*******************************************************************************
 *
 *         AUTHOR: Joshua Brazier
 *
 *           DATE: 10/14/2018
 *
 *         OBJECT: timage
 *
 *       FILENAME: main.c
 *
 *    DESCRIPTION: Timage (from terminal image) is a terminal based image viewer.
 * 
 *      COPYRIGHT: Copyright 2018 - 2021 Joshua Brazier
 * 
 * LICENSE NOTICE: This file is part of Timage.
 * 
 *                 Timage is free software: you can redistribute it and/or modify
 *                 it under the terms of the GNU General Public License as published by
 *                 the Free Software Foundation, either version 3 of the License, or
 *                 (at your option) any later version.
 *                 
 *                 Timage is distributed in the hope that it will be useful,
 *                 but WITHOUT ANY WARRANTY; without even the implied warranty of
 *                 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *                 GNU General Public License for more details.
 *                 
 *                 You should have received a copy of the GNU General Public License
 *                 along with Timage.  If not, see <https://www.gnu.org/licenses/>.
 *                 
 *                 See the file COPYING for more details.
 *
 ******************************************************************************/
#include <math.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define STB_IMAGE_IMPLEMENTATION // _____ Must be in this order
#include "stb_image/stb_image.h" // __|

// Globals
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_t;

typedef struct {
    color_t ** color;
    int size_x;
    int size_y;
    int channels;
    double ratio;
} image_t;

const char version_string[] = "1.0.1";

// Prototypes
image_t downsmaple_image(int x_size, int y_size, image_t in_img);
bool file_is_valid(const char * path);
void print_copyright(void);
void print_help(void);
void term_resize_handler(int sig);
bool unpack_image(image_t img, unsigned char * img_pkg, int size);

int main(int argc, char* argv[])
{
    int x;                  // Width of the raw image
    int y;                  // Height of the raw image
    int n;                  // Number of color channels in the raw image
    int height_in = 0;      // Custom height specified
    int TERM_LINES;         // Set to terminal height or user specified height
    unsigned char * data;   // Raw image data
    char filename[256];     // Image location
    image_t img;            // Better image container
    image_t img_resized;    // Resized image for printing to terminal
    double term_ratio;      // Aspect ratio of the terminal
    
    // Handle command line arguments
    // TODO: Refactor this
    if (argc == 1)
    {
        print_help();
        printf("\n");
        print_copyright();
        return 0;
    }
    else if (argc == 2)
    {
        if (strcmp(argv[1], "-h") == 0 ||
                strcmp(argv[1], "--help") == 0 )
        {
            print_help();
            printf("\n");
            print_copyright();
            return 0;
        }
        else if (strcmp(argv[1], "-v") == 0 ||
                 strcmp(argv[1], "--version") == 0 )
        {
            printf("Timage v%s\n", version_string);
            print_copyright();
            return 0;
        }
        else
        {
            strncpy(filename, argv[1], 256);
            filename[255] = '\0';
            
            if (!file_is_valid(filename))
            {
                printf("Could not access input file.\n");
                printf("Try \"%s --help\" for usage.\n", argv[0]);
            }
        }
    }
    else if (argc == 3)
    {
        if (strcmp(argv[1], "-h") == 0 ||
                strcmp(argv[1], "--help") == 0 )
        {
            print_help();
            printf("\n");
            print_copyright();
            return 0;
        }
        else if (strcmp(argv[1], "-v") == 0 ||
                strcmp(argv[1], "--version") == 0 )
        {
            printf("Timage v%s\n", version_string);
            print_copyright();
            return 0;
        }
        else
        {
            strncpy(filename, argv[1], 256);
            filename[255] = '\0';
            
            if (!file_is_valid(filename))
            {
                printf("Could not access input file.\n");
                printf("Try \"%s --help\" for usage.\n", argv[0]);
            }
            
            height_in = atoi(argv[2]);
        }
    }
    else if (argc >= 4)
    {
        if (strcmp(argv[1], "-h") == 0 ||
                strcmp(argv[1], "--help") == 0 )
        {
            print_help();
            printf("\n");
            print_copyright();
            return 0;
        }
        else if (strcmp(argv[1], "-v") == 0 ||
                strcmp(argv[1], "--version") == 0 )
        {
            printf("Timage v%s\n", version_string);
            print_copyright();
            return 0;
        }
        else
        {
            printf("\"%s\" is an invalid option.\n\n", argv[1]);
            strncpy(filename, argv[2], 256);
            filename[255] = '\0';
            
            if (!file_is_valid(filename))
            {
                printf("Could not access input file.\n");
                printf("Try \"%s --help\" for usage.\n", argv[0]);
            }
            
            height_in = atoi(argv[3]);
        }
    }

    // Ncurses setup
    initscr();
    signal(SIGWINCH, term_resize_handler);
    endwin();
    
    TERM_LINES = (height_in > 0) ? ceil((height_in + 2.0) / 2.0) : LINES + 1;

    // Get the raw image data
    data = stbi_load(filename, &x, &y, &n, 0);

    // Allocate space for the image in a better container
    img.color = (color_t **) malloc(sizeof(color_t *) * y);
    for (int i = 0; i < y; ++i)
    {
        img.color[i] = (color_t *) malloc(sizeof(color_t) * x);
    }
    img.size_x = x;
    img.size_y = y;
    img.channels = n;
    img.ratio = (double) x / (double) y; // >1 landscape, <1 portrait, =1 square
    term_ratio = (double) COLS / (double) ((TERM_LINES - 1) * 2);

    // Unpack and downsample the image so it fits the correct number of "pixels"
    if (!unpack_image(img, data, (x*y*n)))
    {
        fprintf(stderr, "[ERROR] Failed to unpack image data!\n");
        return 1;
    }

    if (term_ratio >= img.ratio)
    {
        int height = (TERM_LINES-1);
        int width = (int) floor((double) (2 * height) * img.ratio);
        img_resized = downsmaple_image(width, height, img);
    }
    else
    {
        int width = COLS;
        int height = (int) floor((double) COLS / (2.0 * img.ratio));
        img_resized = downsmaple_image(width, height, img);
    }

    // Print the resized image to the terminal
    for (int i = 0; i < img_resized.size_y; ++i)
    {
        for (int j = 0; j < img_resized.size_x; ++j)
        {
            printf("\033[38;2;%u;%u;%um\u2588", img_resized.color[i][j].r, img_resized.color[i][j].g, img_resized.color[i][j].b);
        }

        printf("\n");
    }
    printf("\033[0m");
    
    // Cleanup and return
    stbi_image_free(data);
    return 0;
}

// Returns an image_t that has been downsampled to the size
// x_size, y_size based on in_img.
image_t downsmaple_image(int x_size, int y_size, image_t in_img)
{
    double pixel_w = (double) in_img.size_x / (double) x_size;
    double pixel_h = (double) in_img.size_y / (double) y_size;
    image_t ret;
    
    ret.ratio = (double) x_size / (double) y_size;
    ret.size_x = x_size;
    ret.size_y = y_size;
    ret.channels = in_img.channels;
    ret.color = (color_t **) malloc(sizeof(color_t *) * y_size);
    for (int i = 0; i < y_size; ++i)
    {
        ret.color[i] = (color_t *) malloc(sizeof(color_t) * x_size);
    }
    
    for (int y = 0; y < y_size; ++y)
    {
        for (int x = 0; x < x_size; ++x)
        {
            // TODO: Make this work with images other than 3 channel images
            int red = 0;
            int green = 0;
            int blue = 0;
            int num = 0;
            int x_start = (int) round(pixel_w *  (double) x    );
            int y_start = (int) round(pixel_h *  (double) y    );
            int x_next  = (int) round(pixel_w * ((double) x+1) );
            int y_next  = (int) round(pixel_h * ((double) y+1) );
            //int pixel_w_int = x_end - x_start;
            //int pixel_h_int = y_end - y_start;
            
            for (int y_sp = y_start; y_sp < y_next; ++y_sp)
            {
                for (int x_sp = x_start; x_sp < x_next; ++x_sp)
                {
                    red   += in_img.color[y_sp][x_sp].r;
                    green += in_img.color[y_sp][x_sp].g;
                    blue  += in_img.color[y_sp][x_sp].b;
                    ++num;
                }
            }
            
            ret.color[y][x].r = (uint8_t) round((double) red   / (double) num);
            ret.color[y][x].g = (uint8_t) round((double) green / (double) num);
            ret.color[y][x].b = (uint8_t) round((double) blue  / (double) num);
        }
    }
    
    return ret;
}

// Returns true if path points to a file that exists and is valid.
bool file_is_valid(const char * path)
{
    struct stat st;
    
    return (stat(path, &st) >= 0);
}

// Prints the copyright message to the terminal.
void print_copyright(void)
{
    printf("Timage  Copyright (C) 2018 - 2021  Joshua Brazier\n");
    printf("This program comes with ABSOLUTELY NO WARRANTY.\n");
    printf("This is free software, and you are welcome to redistribute it\n");
    printf("under certain conditions; see the file COPYING for details.\n");
}

// Prints the help message to the terminal.
void print_help(void)
{
    printf("Usage: timage [OPTION] [FILE] [HEIGHT]\n");
    printf("Displays an image file in the terminal output.\n");
    printf("\nOPTIONS:\n");
    printf("  -h, --help\t\tDisplay this help and exit\n");
    printf("  -v, --version\t\tOutput version information and exit.\n");
    printf("\nThe HEIGHT argument can be specified to manually set the height\n");
    printf("of the output image (the width is determined by the images aspect\n");
    printf("ratio.\n");
}

// Signal handler for a terminal resize event.
void term_resize_handler(int sig)
{
    int x, y;
    char tmp[128];
    signal(SIGWINCH, SIG_IGN);

    endwin();
    initscr();
    refresh();
    clear();

    sprintf(tmp, "%dx%d", COLS, LINES);

    x = COLS / 2 - strlen(tmp) / 2;
    y = LINES / 2 - 1;

    mvaddstr(y, x, tmp);
    refresh();

    signal(SIGWINCH, term_resize_handler);
}

// Unpacks an image returned by the image processing library into
// a more convenient image_t container. img_pkg is the input, size
// is the size of the input in number of elements, and img is the
// returned image_t container. Returns true if unpack was successful,
// false otherwise.
bool unpack_image(image_t img, unsigned char * img_pkg, int size)
{
    int k = 0;

    for (int i = 0; i < img.size_y; ++i)
    {
        for (int j = 0; j < img.size_x; ++j)
        {
            if (k+2 >= size)
            {
                return false;
            }

            img.color[i][j].r = (uint8_t) img_pkg[k];
            img.color[i][j].g = (uint8_t) img_pkg[k+1];
            img.color[i][j].b = (uint8_t) img_pkg[k+2];

            k += 3;
        }
    }

    return true;
}


