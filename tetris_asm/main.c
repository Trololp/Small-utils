#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct
{
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    int xres;
    int yres;
}
Gpu;

typedef struct
{
    uint32_t* pixels;
    int width;
}
Display;

extern void game_init();
extern void game_run();

const int width = 400; //image file width
const int height = 300; //image file height

int key = 0;

uint32_t* pixels = NULL;

// Setups the software gpu.
static Gpu setup(const int xres, const int yres, const bool vsync)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        puts(SDL_GetError());
        exit(1);
    }
    SDL_Window* const window = SDL_CreateWindow(
        "test",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        xres, yres,
        SDL_WINDOW_SHOWN);
    SDL_Renderer* const renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | (vsync ? SDL_RENDERER_PRESENTVSYNC : 0x0));
    // Notice the flip between xres and yres.
    // The texture is 90 degrees flipped on its side for fast cache access.
    SDL_Texture* const texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        yres, xres);
    if(window == NULL || renderer == NULL || texture == NULL)
    {
        puts(SDL_GetError());
        exit(1);
    }
    const Gpu gpu = { window, renderer, texture, xres, yres };
    return gpu;
}

static void present(const Gpu gpu)
{
    const SDL_Rect dst = {
        (gpu.xres - gpu.yres) / 2,
        (gpu.yres - gpu.xres) / 2,
        gpu.yres, gpu.xres,
    };
    SDL_RenderCopyEx(gpu.renderer, gpu.texture, NULL, &dst, -90, NULL, SDL_FLIP_NONE);
    SDL_RenderPresent(gpu.renderer);
}

// Locks the gpu, returning a pointer to video memory.
static Display lock(const Gpu gpu)
{
    void* screen;
    int pitch;
    SDL_LockTexture(gpu.texture, NULL, &screen, &pitch);
    const Display display = { (uint32_t*) screen, pitch / (int) sizeof(uint32_t) };
    return display;
}

// Unlocks the gpu, making the pointer to video memory ready for presentation
static void unlock(const Gpu gpu)
{
    SDL_UnlockTexture(gpu.texture);
}

static bool done()
{
    SDL_Event event;
    SDL_PollEvent(&event);
    return event.type == SDL_QUIT
        || event.key.keysym.sym == SDLK_END
        || event.key.keysym.sym == SDLK_ESCAPE;
}

void set_key_variable() {
    const uint8_t* keys_table = SDL_GetKeyboardState(NULL);
    key = 0;
    if(keys_table[SDL_SCANCODE_W]) key |= 8;
    if(keys_table[SDL_SCANCODE_S]) key |= 4;
    if(keys_table[SDL_SCANCODE_A]) key |= 2;
    if(keys_table[SDL_SCANCODE_D]) key |= 1;

}

void render(Gpu gpu) {
    const int t0 = SDL_GetTicks();
    const Display display = lock(gpu);


    // run tetris

    memset(display.pixels, 0, width * height * sizeof(uint32_t));
    pixels = display.pixels;


    //for(int i = 0; i < 1000; i++) {
     //   pixels[i] = 0xFF00FFFF;
    //}
    game_run();

    unlock(gpu);
    present(gpu);



    const int t1 = SDL_GetTicks();
    const int ms = 16 - (t1 - t0);
    SDL_Delay(ms < 0 ? 0 : ms);
}

int main()
{
    //tetris run
    const Gpu gpu = setup(width, height, true);

    game_init();

    while(!done())
    {
        set_key_variable();
        render(gpu);
    }

    printf("Done!\n");
}
