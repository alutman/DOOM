
#include "d_main.h"
#include "v_video.h"
#include <SDL.h>
#include <tgmath.h>

#include "i_system.h"

// SDL implementation of the video system
// render is still 320x200 but scaled up to be visible on higher res monitors

SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;
SDL_Texture* gTexture = NULL;
byte* currentPalette;



int scale;

int scaleWidth;
int scaleHeight;

// doom designed aspect ratio is 1.2
#define ASPECT_RATIO 1.2


SDL_Rect* srcrect;
SDL_Rect* destrect;


// Called by D_DoomMain,
// determines the hardware configuration
// and sets up the video mode
void I_InitGraphics (void) {
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        I_Error("SDL_Init fail: %s\n", SDL_GetError());
    }


    // div by 2 gives more control over the screen size
    float realScale = (float)scale/(float)2;

    // set scale minimum
    if (realScale < 1) {
        realScale = 1;
    }

    int displayIndex = 0;
    SDL_DisplayMode current;

    if (SDL_GetCurrentDisplayMode(displayIndex, &current) != 0) {
        I_Error("SDL_GetCurrentDisplayMode Error: %s\n", SDL_GetError());
    }

    // set scale maximum
    if (SCREENWIDTH * realScale > current.w) {
        realScale = current.w / realScale;
    }
    if (SCREENHEIGHT * realScale > current.h) {
        realScale = current.h / realScale;
    }


    scaleWidth = SCREENWIDTH * (realScale);
    scaleHeight = SCREENHEIGHT * (realScale);

    srcrect = malloc(sizeof(SDL_Rect));
    if (srcrect == NULL) {
        I_Error("malloc fail in srcrect");
    }
    destrect = malloc(sizeof(SDL_Rect));
    if (destrect == NULL) {
        I_Error("malloc fail in destrect");
    }

    // source renderer to output display translation rectangles
    *destrect = (SDL_Rect){0, 0, scaleWidth, scaleHeight * ASPECT_RATIO};
    *srcrect =  (SDL_Rect){0, 0, SCREENWIDTH, SCREENHEIGHT};


    gWindow = SDL_CreateWindow("DOOM", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        scaleWidth, scaleHeight * ASPECT_RATIO, SDL_WINDOW_INPUT_GRABBED );
    if( gWindow == NULL )
    {
        I_Error( "SDL_CreateWindow fail: %s\n", SDL_GetError() );
    }

    //Create renderer for window
    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
    if( gRenderer == NULL )
    {
        I_Error("SDL_CreateRenderer fail: %s\n", SDL_GetError() );
    }

    gTexture = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
        SCREENWIDTH, SCREENHEIGHT);

    if( gRenderer == NULL )
    {
        I_Error("SDL_CreateTexture fail: %s\n", SDL_GetError() );
    }

    // lock mouse to screen
    if( SDL_SetRelativeMouseMode(SDL_TRUE) < 0 )
    {
        I_Error("SDL_SetRelativeMouseMode fail: %s\n", SDL_GetError() );
    }

}



void I_ShutdownGraphics(void) {
    SDL_DestroyRenderer( gRenderer );
    SDL_DestroyWindow( gWindow );
    SDL_DestroyTexture(gTexture);
    SDL_Quit();
    gWindow = NULL;
    gRenderer = NULL;
    gTexture = NULL;
}


// Takes a 256 array of 3 8 bit values representing the rgb of the palette index
void I_SetPalette (byte* palette) {
    currentPalette = palette;
}


void I_UpdateNoBlit (void) {

}

// Writes to a SDL texture for copying to the renderer
// * iterates over every texture pixel
// * retrieves the relevant screen[0] palette index
// * converts the palette index to rgb
void I_FinishUpdate (void) {

    Uint32* pixels = malloc(SCREENWIDTH * SCREENHEIGHT * sizeof(Uint32));
    for (int y = 0; y < SCREENHEIGHT; y++) {
        for (int x = 0; x < SCREENWIDTH; x++) {
            const int palIdx = screens[0][y * SCREENWIDTH + x];

            // palette data entries are 3 bytes as r,g,b
            const byte r = currentPalette[3 * palIdx + 0];
            const byte g = currentPalette[3 * palIdx + 1];
            const byte b = currentPalette[3 * palIdx + 2];

            pixels[y * SCREENWIDTH + x] = SDL_MapRGBA(SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888), r, g, b, 255);
        }
    }

    SDL_UpdateTexture(gTexture, NULL, pixels, SCREENWIDTH * sizeof(Uint32));
    free(pixels);

    // Use SDL to scale the image and apply aspect ratio
    // SDL_RenderClear(gRenderer);
    SDL_RenderCopy(gRenderer, gTexture, srcrect, destrect);
    SDL_RenderPresent(gRenderer);
}


void I_ReadScreen (byte* scr) {
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}

void sendKey(SDL_Event event, evtype_t type) {
    int keyid = -1;
    switch(event.key.keysym.sym) {
        case SDLK_RETURN:
            keyid = KEY_ENTER;
        break;
        case SDLK_UP:
            keyid = KEY_UPARROW;
        break;
        case SDLK_DOWN:
            keyid = KEY_DOWNARROW;
        break;
        case SDLK_LEFT:
            keyid = KEY_LEFTARROW;
        break;
        case SDLK_RIGHT:
            keyid = KEY_RIGHTARROW;
        break;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:
            keyid = KEY_RSHIFT;
        break;
        case SDLK_RALT:
            keyid = KEY_RALT;
        break;
        case SDLK_LCTRL:
        case SDLK_RCTRL:
            keyid = KEY_RCTRL;
        break;
        case SDLK_ESCAPE:
            keyid = KEY_ESCAPE;
        break;
        case SDLK_SPACE:
            keyid = ' ';
        break;
        case SDLK_BACKSPACE:
            keyid = KEY_BACKSPACE;
        break;
        case SDLK_MINUS:
            keyid = KEY_MINUS;
        break;
        case SDLK_EQUALS:
            keyid = KEY_EQUALS;
        break;
        case SDLK_PAUSE:
            keyid = KEY_PAUSE;
        break;
        case SDLK_LALT:
            keyid = KEY_LALT;
        break;
        case SDLK_TAB:
            keyid = KEY_TAB;
        break;
        default:
            keyid = event.key.keysym.sym;
    }
    event_t event1;
    event1.type = type;
    event1.data1 = keyid;
    D_PostEvent(&event1);
}

void sendMouse() {
    int lastX = 0;
    int lastY = 0;
    int buttons = SDL_GetRelativeMouseState(&lastX, &lastY);
    event_t event1;
    event1.type = ev_mouse;
    event1.data1 = buttons;
    event1.data2 = lastX;
    // explicitly un-set as the y axis is used to move the player and can't be turned off in game
    event1.data3 = 0;

    D_PostEvent(&event1);
}


void I_StartTic (void) {
    int hadMouse = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT) {
            I_Quit();
        }

        switch (event.type)
        {
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEMOTION:
                // only send one mouse event per tick
                // otherwise the engine is overloaded with mouse movement events
                if (hadMouse) {
                    break;
                }
                sendMouse();
                hadMouse = 1;
                break;
            case SDL_KEYUP:
                sendKey(event, ev_keyup);
            break;
            case SDL_KEYDOWN:
                sendKey(event, ev_keydown);
            break;
        default: ;
        }
    }
}
void I_StartFrame (void)
{}
