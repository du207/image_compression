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
#include "pixelrw.h"
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
    FILE* bmp_fp = fopen(filename, "rb");
    if (bmp_fp == NULL) return -1;

    char* awi_filename = replace_str_end(filename, ".awi");
    FILE* awi_fp = fopen(awi_filename, "wb");
    if (awi_fp == NULL) {
        fclose(bmp_fp);
        return -1;
    }

    PixelInput bmp_pi;
    BMPInputContext bmp_ctx = {
        .fp = bmp_fp,
    };
    init_bmp_pixel_input(&bmp_pi, &bmp_ctx);

    BitsOutput awi_bo;
    AWIOutputContext awi_ctx = {
        .fp = awi_fp,
    };
    init_awi_bits_output(&awi_bo, &awi_ctx);

    if (!compress_to_awi(&bmp_pi, &awi_bo)) return -1;

    printf("%s: convert completed!\n", awi_filename);

    fclose(bmp_fp);
    fclose(awi_fp);
    free(awi_filename);

    return 0;
}

int revert_awi_to_bmp(char* filename) {
    if (!is_end_with(filename, ".awi")) return -2;

    FILE* awi_fp = fopen(filename, "rb");
    if (awi_fp == NULL) return -1;

    char* bmp_filename = replace_str_end(filename, ".bmp");
    FILE* bmp_fp = fopen(bmp_filename, "wb");
    if (bmp_fp == NULL) return -1;

    BitsInput awi_bi;
    AWIInputContext awi_ctx = {
        .fp = awi_fp,
    };
    init_awi_bits_input(&awi_bi, &awi_ctx);

    PixelOutput bmp_po;
    BMPOutputContext bmp_ctx = {
        .fp = bmp_fp
    };
    init_bmp_pixel_output(&bmp_po, &bmp_ctx);

    if (!decompress_awi(&awi_bi, &bmp_po)) return -1;

    printf("%s: revert completed!\n", bmp_filename);

    fclose(awi_fp);
    fclose(bmp_fp);
    free(bmp_filename);

    return 0;
}



int show_awi_file(char* filename) {
    if (!is_end_with(filename, ".awi")) return -2;

    // Read awi file
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) return -1;

    BitsInput awi_bi;
    AWIInputContext awi_ctx = {
        .fp = fp,
    };
    init_awi_bits_input(&awi_bi, &awi_ctx);

    PixelOutput pixel_po;
    TextureOutputContext pixel_ctx;
    uint8_t* pixels;
    init_rgb_pixel_output(&pixel_po, &pixel_ctx, &pixels);

    if (!decompress_awi(&awi_bi, &pixel_po)) return -1;

    int width = pixel_po.width;
    int height = pixel_po.height;

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
        free_safe(pixels);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -3;
    }

    SDL_Texture* texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width, height);

    if (!texture) {
        fprintf(stderr, "CreateTexture Error: %s\n", SDL_GetError());
        free_safe(pixels);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -3;
    }

    int running = 1;
    SDL_Event e;

    SDL_UpdateTexture(texture, NULL, pixels, width * 3);
    free_safe(pixels);

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

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
