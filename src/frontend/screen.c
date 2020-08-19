#include "v4l2_driver.h"
#include <SDL2/SDL.h>
#include <linux/videodev2.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include "socket_t.h"

pthread_t thread_stream;
SDL_Window *sdlScreen;
SDL_Renderer *sdlRenderer;
SDL_Texture *sdlTexture;
SDL_Rect sdlRect;

int thread_exit_sig = 0;

#define PROTOCOL_ITSFRAME 2

#define SAVE_EVERY_FRAME 0

int IMAGE_WIDTH;
int IMAGE_HEIGHT;

static void frame_handler(void *pframe, int length) {
  SDL_UpdateTexture(sdlTexture, &sdlRect, pframe, IMAGE_WIDTH * 2);
  //  SDL_UpdateYUVTexture
  SDL_RenderClear(sdlRenderer);
  SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
  SDL_RenderPresent(sdlRenderer);

#if SAVE_EVERY_FRAME
  static yuv_index = 0;
  char yuvifle[100];
  sprintf(yuvifle, "yuv-%d.yuv", yuv_index);
  FILE *fp = fopen(yuvifle, "wb");
  fwrite(pframe, length, 1, fp);
  fclose(fp);
  yuv_index++;
#endif
}

void* frame_receiver(void *_socket) {
    while(!thread_exit_sig) {
        uint8_t protocol = 0;
        uint8_t _;
        socket_send(_socket, &_, 1);
        socket_recv(_socket, &protocol, sizeof(uint8_t));
        switch(protocol) {
            case PROTOCOL_ITSFRAME: {
                unsigned int size;
                uint8_t *buffer;
                if(socket_recv(_socket, (uint8_t*)&size, sizeof(unsigned int)) < 0) {
                    perror("recv");
                    exit(EXIT_FAILURE);
                }
                printf("Receive a frame of size %u\n", size);
                buffer = calloc(size, sizeof(uint8_t));
                if(socket_recv(_socket, buffer, size) < 0) {
                    perror("recv");
                    exit(EXIT_FAILURE);
                }
                frame_handler(buffer, size);
                free(buffer);
                break;
            }
            default:
                printf("%s\n", "Unknown protocol.");
        }
    }
    
    pthread_exit(NULL);
}

void sdl2_begin(int img_w, int img_h) {
    IMAGE_WIDTH = img_w;
    IMAGE_HEIGHT = img_h;

    socket_p _socket = socket_new(PROTO_TCP);
    if(!socket_connect(_socket, "127.0.0.1", 22122)){
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // SDL2 begins
    memset(&sdlRect, 0, sizeof(sdlRect));
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    sdlScreen = SDL_CreateWindow("Simple YUV Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, IMAGE_WIDTH, IMAGE_HEIGHT, SDL_WINDOW_SHOWN);

    if (!sdlScreen) {
        fprintf(stderr, "SDL: could not create window - exiting:%s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    sdlRenderer = SDL_CreateRenderer(sdlScreen, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (sdlRenderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer Error\n");
        exit(EXIT_FAILURE);
    }
    sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_YUY2, SDL_TEXTUREACCESS_STREAMING, IMAGE_WIDTH, IMAGE_HEIGHT);
    sdlRect.w = IMAGE_WIDTH;
    sdlRect.h = IMAGE_HEIGHT;

    // create a thread that will update frame int the buffer
    pthread_create(&thread_stream, NULL, frame_receiver, (void*)_socket);

    int quit = 0;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { // click close icon then quit
            quit = 1;
            }
            if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_ESCAPE) // press ESC the quit
                quit = 1;
            }
        }
    usleep(25);
    }

    thread_exit_sig = 1;
}

int main(int argc, char **argv) {
    sdl2_begin(640, 480);
    SDL_Quit();
    return 0;
}