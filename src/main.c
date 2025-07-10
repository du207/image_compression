#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>

#include "awi.h"
#include "bitrw.h"
#include "bmp.h"
#include "color.h"
#include "utils.h"

int convert_bmp_to_awi(char* filename);
int revert_awi_to_bmp(char* filename);
int show_awi_file(char* filename);


int main (int argc, char** argv) {
    // expected command:
    // image_compression convert (.bmp image file)
    // image_compression revert (.awi image file)
    // image_compression show (.awi image file)
    // ONLY SUPPORT 24BITS BMP FILE

    int err_code = 0;
    char* task = argv[1];
    char* filename = argv[2];
    if (task == NULL) task = "";

    if (strcmp(task, "convert") == 0) {
        if (filename == NULL) {
            printf(".bmp image file name required!\n");
            return 1;
        }

        err_code = convert_bmp_to_awi(filename);
        if (err_code < 0) goto error_handle;
    } else if (strcmp(task, "revert") == 0) {
        if (filename == NULL) {
            printf(".awi image file name required!\n");
            return 1;
        }

        err_code = revert_awi_to_bmp(filename);
        if (err_code < 0) goto error_handle;
    } else if (strcmp(task, "show") == 0) {
        if (filename == NULL) {
            printf(".awi image file name required!\n");
            return 1;
        }

        err_code = show_awi_file(filename);
        if (err_code < 0) goto error_handle;
    } else {
        // show help command
        printf("*********************************\n");
        printf("*** .awi project command list ***\n");
        printf("*********************************\n\n");
        printf("image_compression convert (.bmp image file)\n");
        printf("image_compression revert (.awi image file)\n");
        printf("image_compression show (.awi image file)\n");
        printf("\n* Only supports for 24bit bmp file\n");
    }

    return 0;


error_handle:
    switch (err_code) {
    case -1:
        printf("File reading/writing error!\n");
        return 1;
    case -2:
        printf("Invalid file format!\n");
        return 2;
    case -3:
        printf("SDL Error!\n");
        return 3;
    default:
        printf("Unknown error!\n");
        return 1;
    }
}


int convert_bmp_to_awi(char* filename) {
    if (!is_end_with(filename, ".bmp")) return -2;

    // Read bmp file
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) return -1;

    int width, height;
    RGBImage* rgb_image = read_bmp_file(fp, &width, &height);
    if (rgb_image == NULL) {
        fclose(fp);
        return -2;
    }

    fclose(fp);

    AWIContent* content = rgb_img_to_awi_content(rgb_image);

   // Write .awi file
    char* awi_filename = replace_str_end(filename, ".awi");
    if (awi_filename == NULL) {
        destroy_awi_content(content);
        return -2;
    }

    FILE* fp_write_awi = fopen(awi_filename, "wb");
    if (fp_write_awi == NULL) {
        destroy_awi_content(content);
        return -1;
    }

    BitWriter* bw = create_bit_writer(fp_write_awi);

    if (write_awi_file(bw, content, width, height) < 0) {
        destroy_awi_content(content);
        destroy_bit_writer(bw);
        fclose(fp_write_awi);
        return -1;
    }

    destroy_awi_content(content);
    destroy_bit_writer(bw);
    fclose(fp_write_awi);

    printf("%s: convert completed!\n", awi_filename);
    free(awi_filename);

    return 0;
}

int revert_awi_to_bmp(char* filename) {
    if (!is_end_with(filename, ".awi")) return -2;

    // Read awi file
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) return -1;

    BitReader* br = create_bit_reader(fp);

    int width, height;

    AWIContent* content = read_awi_file(br, &width, &height);
    destroy_bit_reader(br);
    fclose(fp);

    if (content == NULL) return -2;

    RGBImage* rgb_img = awi_content_to_rgb_img(content, width, height);
    destroy_awi_content(content);

    // Write .bmp file
    char* bmp_filename = replace_str_end(filename, ".bmp");
    if (bmp_filename == NULL) {
        destroy_rgb_image(rgb_img);
        return -2;
    }

    FILE* bmp_fp = fopen(bmp_filename, "wb");
    if (write_bmp_file(bmp_fp, rgb_img, width, height) < 0) {
        destroy_rgb_image(rgb_img);
        fclose(bmp_fp);
        return -1;
    }

    destroy_rgb_image(rgb_img);
    fclose(bmp_fp);

    printf("%s: revert completed!\n", bmp_filename);
    free(bmp_filename);

    return 0;
}



int show_awi_file(char* filename) {
    if (!is_end_with(filename, ".awi")) return -2;

    // Read awi file
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) return -1;

    BitReader* br = create_bit_reader(fp);

    int width, height;

    AWIContent* content = read_awi_file(br, &width, &height);
    destroy_bit_reader(br);
    fclose(fp);

    if (content == NULL) return -1;

    RGBImage* rgb_img = awi_content_to_rgb_img(content, width, height);

    destroy_awi_content(content);

    // SDL Image Viewer
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return -3;
    }

    SDL_Window* window = SDL_CreateWindow("AWI Image Viewer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, SDL_WINDOW_SHOWN);

    if (!window) {
        fprintf(stderr, "CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return -3;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer) {
        fprintf(stderr, "CreateRenderer Error: %s\n", SDL_GetError());
        destroy_rgb_image(rgb_img);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -3;
    }

    SDL_Texture* texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width, height);

    if (!texture) {
        fprintf(stderr, "CreateTexture Error: %s\n", SDL_GetError());
        destroy_rgb_image(rgb_img);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -3;
    }

    uint8_t* pixels = (uint8_t*) malloc(width * height * 3);

    uint8_t** r_p = rgb_img->r->p;
    uint8_t** g_p = rgb_img->g->p;
    uint8_t** b_p = rgb_img->b->p;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int offset = (y * width + x) * 3;
            pixels[offset] = r_p[y][x];
            pixels[offset + 1] = g_p[y][x];
            pixels[offset + 2] = b_p[y][x];
        }
    }

    destroy_rgb_image(rgb_img);

    int running = 1;
    SDL_Event e;

    SDL_UpdateTexture(texture, NULL, pixels, width * 3);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                running = 0;
        }

        SDL_Delay(16); // 62.5fps
    }

    free(pixels);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
