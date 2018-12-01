/*******************************************************************************
 *
 *      AUTHOR: Joshua Brazier
 *
 *        DATE: 10/14/2018
 *
 *      OBJECT: timage
 *
 *    FILENAME: main.c
 *
 * DESCRIPTION: Timage (from terminal image) is a terminal based image viewer.
 *
 ******************************************************************************/
#include <math.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

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
} image_t;

// Prototypes
void term_resize_handler(int sig);
void unpack_image(image_t img, unsigned char * img_pkg, int size);
image_t downsmaple_image(int x_size, int y_size, image_t in_img);

int main(int argc, char* argv[])
{
    int x;
    int y;
    int n;
    int height_in = 50;
    unsigned char * data;
    char filename[256];
    image_t img;
    image_t img_resized;

    if (argc == 2)
    {
        strncpy(filename, argv[1], 256);
        filename[256] = '\0';
    }
    else if (argc > 2)
    {
        strncpy(filename, argv[1], 256);
        filename[256] = '\0';
        height_in = atoi(argv[2]);
    }
    else
    {
        // TODO: Make this print the help info instead
        strcpy(filename, "./test_images/Lenna_Test_Image.png");
    }

    // Ncurses setup
    initscr();
    signal(SIGWINCH, term_resize_handler);
    endwin();

    data = stbi_load(filename, &x, &y, &n, 0);

    img.color = (color_t **) malloc(sizeof(color_t *) * y);
    for (int i = 0; i < y; ++i)
    {
        img.color[i] = (color_t *) malloc(sizeof(color_t) * x);
    }
    img.size_x = x;
    img.size_y = y;
    img.channels = n;

    unpack_image(img, data, (x*y*n));
    img_resized = downsmaple_image(2 * (LINES-1), (LINES-1), img);

    for (int i = 0; i < (LINES-1); ++i)
    {
        for (int j = 0; j < 2 * (LINES-1); ++j)
        {
            printf("\033[38;2;%u;%u;%um%c", img_resized.color[i][j].r, img_resized.color[i][j].g, img_resized.color[i][j].b, (char) 35);
        }

        printf("\n");
    }

    //printf("Width = %d, Height = %d\n", COLS, LINES);
    stbi_image_free(data);
    return 0;
}

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

void unpack_image(image_t img, unsigned char * img_pkg, int size)
{
    int k = 0;

    for (int i = 0; i < img.size_y; ++i)
    {
        for (int j = 0; j < img.size_x; ++j)
        {
            img.color[i][j].r = (uint8_t) img_pkg[k];
            img.color[i][j].g = (uint8_t) img_pkg[k+1];
            img.color[i][j].b = (uint8_t) img_pkg[k+2];

            k += 3;
        }
    }
}

image_t downsmaple_image(int x_size, int y_size, image_t in_img)
{
    double pixel_w = (double) in_img.size_x / (double) x_size;
    double pixel_h = (double) in_img.size_y / (double) y_size;
    image_t ret;

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
