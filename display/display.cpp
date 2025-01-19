/*
 *   Copyright (c) 2025 Jawaid Bazyar

 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.

 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.

 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <SDL3/SDL.h>

#include "cpu.hpp"
#include "gs2.hpp"
#include "memory.hpp"
#include "debug.hpp"

#include "bus.hpp"
#include "display.hpp"
#include "text_40x24.hpp"
#include "lores_40x48.hpp"
#include "hgr_280x192.hpp"
#include "platforms.hpp"


display_page_t display_pages[NUM_DISPLAY_PAGES] = {
    {
        0x0400,
        0x07FF,
        {   // text page 1 line addresses
            0x0400,
            0x0480,
            0x0500,
            0x0580,
            0x0600,
            0x0680,
            0x0700,
            0x0780,

            0x0428,
            0x04A8,
            0x0528,
            0x05A8,
            0x0628,
            0x06A8,
            0x0728,
            0x07A8,

            0x0450,
            0x04D0,
            0x0550,
            0x05D0,
            0x0650,
            0x06D0,
            0x0750,
            0x07D0,
        },
        0x2000,
        0x3FFF,
        { // HGR page 1 line addresses
            0x2000,
            0x2080,
            0x2100,
            0x2180,
            0x2200,
            0x2280,
            0x2300,
            0x2380,

            0x2028,
            0x20A8,
            0x2128,
            0x21A8,
            0x2228,
            0x22A8,
            0x2328,
            0x23A8,

            0x2050,
            0x20D0,
            0x2150,
            0x21D0,
            0x2250,
            0x22D0,
            0x2350,
            0x23D0,
        },
    },
    {
        0x0800,
        0x0BFF,
        {       // text page 2 line addresses
            0x0800,
            0x0880,
            0x0900,
            0x0980,
            0x0A00,
            0x0A80,
            0x0B00,
            0x0B80,

            0x0828,
            0x08A8,
            0x0928,
            0x09A8,
            0x0A28,
            0x0AA8,
            0x0B28,
            0x0BA8,

            0x0850,
            0x08D0,
            0x0950,
            0x09D0,
            0x0A50,
            0x0AD0,
            0x0B50,
            0x0BD0,
        },
        0x4000,
        0x5FFF,
        {       // HGR page 2 line addresses
            0x4000,
            0x4080,
            0x4100,
            0x4180,
            0x4200,
            0x4280,
            0x4300,
            0x4380,

            0x4028,
            0x40A8,
            0x4128,
            0x41A8,
            0x4228,
            0x42A8,
            0x4328,
            0x43A8,

            0x4050,
            0x40D0,
            0x4150,
            0x41D0,
            0x4250,
            0x42D0,
            0x4350,
            0x43D0,
        },
    },
};

// TODO: These should be set from an array of parameters.
void set_display_page(cpu_state *cpu, display_page_number_t page) {
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
    ds->display_page_table = &display_pages[page];

/*     TEXT_PAGE_START = display_pages[page].text_page_start;
    TEXT_PAGE_END = display_pages[page].text_page_end;
    TEXT_PAGE_TABLE = display_pages[page].text_page_table;
    HGR_PAGE_START = display_pages[page].hgr_page_start;
    HGR_PAGE_END = display_pages[page].hgr_page_end;
    HGR_PAGE_TABLE = display_pages[page].hgr_page_table; */
}

void set_display_page1(cpu_state *cpu) {
    set_display_page(cpu, DISPLAY_PAGE_1);
}

void set_display_page2(cpu_state *cpu) {
    set_display_page(cpu, DISPLAY_PAGE_2);
}

uint64_t init_display_sdl(display_state_t *ds) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
        return 1;
    }

    const int SCALE_X = 4;
    const int SCALE_Y = 4;
    const int BASE_WIDTH = 280;
    const int BASE_HEIGHT = 192;
    
    ds->window = SDL_CreateWindow(
        "GSSquared - Apple ][ Emulator", 
        /* SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED,  */
        BASE_WIDTH * SCALE_X, 
        BASE_HEIGHT * SCALE_Y, 
        0 /* SDL_WINDOW_SHOWN */
    );

    if (!ds->window) {
        fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
        return 1;
    }

    // Create renderer with nearest-neighbor scaling (sharp pixels)
    ds->renderer = SDL_CreateRenderer(ds->window, NULL /* -1, 
        SDL_RENDERER_ACCELERATED */ );
    
    if (!ds->renderer) {
        fprintf(stderr, "Error creating renderer: %s\n", SDL_GetError());
        return 1;
    }

    // Set scaling quality to nearest neighbor for sharp pixels
    /* SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); */
    SDL_SetRenderScale(ds->renderer, SCALE_X, SCALE_Y);

    // Create the screen texture
    ds->screenTexture = SDL_CreateTexture(ds->renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        280, 192);

    if (!ds->screenTexture) {
        fprintf(stderr, "Error creating screen texture: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetTextureBlendMode(ds->screenTexture, SDL_BLENDMODE_NONE); /* GRRRRRRR. This was defaulting to SDL_BLENDMODE_BLEND. */
    // LINEAR gets us appropriately blurred pixels.
    // NEAREST gets us sharp pixels.
    // TODO: provide a UI toggle for this.
    SDL_SetTextureScaleMode(ds->screenTexture, SDL_SCALEMODE_LINEAR);

    // Clear the texture to black
    SDL_SetRenderDrawColor(ds->renderer, 0, 0, 0, 255);
    SDL_RenderClear(ds->renderer);
    SDL_RenderPresent(ds->renderer);

    SDL_RaiseWindow(ds->window);

    return 0;
}

void init_display_font(rom_data *rd) {
    pre_calculate_font(rd);
}

/**
 * This is effectively a "redraw the entire screen each frame" method now.
 * With an optimization only update dirty lines.
 */
void update_display(cpu_state *cpu) {
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
    int updated = 0;
    for (int line = 0; line < 24; line++) {
        if (ds->dirty_line[line]) {
            render_line(cpu, line);
            ds->dirty_line[line] = 0;
            updated = 1;
        }
    }

    if (updated) {
        SDL_RenderTexture(ds->renderer, ds->screenTexture, NULL, NULL);
        SDL_RenderPresent(ds->renderer);
    }
}

void force_display_update(cpu_state *cpu) {
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
    for (int y = 0; y < 24; y++) {
        ds->dirty_line[y] = 1;
    }
}

void free_display(cpu_state *cpu) {
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
    if (ds->screenTexture) SDL_DestroyTexture(ds->screenTexture);
    if (ds->renderer) SDL_DestroyRenderer(ds->renderer);
    if (ds->window) SDL_DestroyWindow(ds->window);
    SDL_Quit();
}


void update_line_mode(cpu_state *cpu) {
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);

    line_mode_t top_mode;
    line_mode_t bottom_mode;

    if (ds->display_mode == TEXT_MODE) {
        top_mode = LM_TEXT_MODE;
    } else {
        if (ds->display_graphics_mode == LORES_MODE) {
            top_mode = LM_LORES_MODE;
        } else {
            top_mode = LM_HIRES_MODE;
        }
    }

    if (ds->display_split_mode == SPLIT_SCREEN) {
        bottom_mode = LM_TEXT_MODE;
    } else {
        bottom_mode = top_mode;
    }

    for (int y = 0; y < 20; y++) {
        ds->line_mode[y] = top_mode;
    }
    for (int y = 20; y < 24; y++) {
        ds->line_mode[y] = bottom_mode;
    }
}

void set_display_mode(cpu_state *cpu, display_mode_t mode) {
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);

    ds->display_mode = mode;
    update_line_mode(cpu);
}

void set_split_mode(cpu_state *cpu, display_split_mode_t mode) {
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);

    ds->display_split_mode = mode;
    update_line_mode(cpu);
}

void set_graphics_mode(cpu_state *cpu, display_graphics_mode_t mode) {
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);

    ds->display_graphics_mode = mode;
    update_line_mode(cpu);
}

// TODO: this code can likely lock the whole screen, so we do one lock
// (relatively expensive) instead of one lock per line.

void render_line(cpu_state *cpu, int y) {
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);

    SDL_Rect updateRect = {
        0,          // X position (left of window))
        y * 8,      // Y position (8 pixels per character)
        280,        // Width of line
        8           // Height of line
    };

    // the texture is our canvas. When we 'lock' it, we get a pointer to the pixels, and the pitch which is pixels per row
    // of the area. Since all our chars are the same we can just use the same pitch for all our chars.

    void* pixels;
    int pitch;
    if (!SDL_LockTexture(ds->screenTexture, &updateRect, &pixels, &pitch)) {
        fprintf(stderr, "Failed to lock texture: %s\n", SDL_GetError());
        return;
    }

    for (int x = 0; x < 40; x++) {
        line_mode_t mode = ds->line_mode[y];
        if (mode == LM_TEXT_MODE) {
            render_text(cpu, x, y, pixels, pitch);
        } else if (mode == LM_LORES_MODE) {
            render_lores(cpu, x, y, pixels, pitch);
        } else if (mode == LM_HIRES_MODE) {
            render_hgr(cpu, x, y, pixels, pitch);
        }
    }

    SDL_UnlockTexture(ds->screenTexture);
}


uint8_t txt_bus_read_C050(cpu_state *cpu, uint16_t address) {
    // set graphics mode
    if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Set Graphics Mode\n");
    //display_mode = GRAPHICS_MODE;
    set_display_mode(cpu, GRAPHICS_MODE);
    force_display_update(cpu);
    return 0;
}

void txt_bus_write_C050(cpu_state *cpu, uint16_t address, uint8_t value) {
    txt_bus_read_C050(cpu, address);
}


uint8_t txt_bus_read_C051(cpu_state *cpu, uint16_t address) {
// set text mode
    if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Set Text Mode\n");
    //display_mode = TEXT_MODE;
    set_display_mode(cpu, TEXT_MODE);
    force_display_update(cpu);
    return 0;
}

void txt_bus_write_C051(cpu_state *cpu, uint16_t address, uint8_t value) {
    txt_bus_read_C051(cpu, address);
}


uint8_t txt_bus_read_C052(cpu_state *cpu, uint16_t address) {
    // set full screen
    if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Set Full Screen\n");
    //display_split_mode = FULL_SCREEN;
    set_split_mode(cpu, FULL_SCREEN);
    force_display_update(cpu);
    return 0;
}

void txt_bus_write_C052(cpu_state *cpu, uint16_t address, uint8_t value) {
    txt_bus_read_C052(cpu, address);
}


uint8_t txt_bus_read_C053(cpu_state *cpu, uint16_t address) {
    // set split screen
    if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Set Split Screen\n");
    //display_split_mode = SPLIT_SCREEN;
    set_split_mode(cpu, SPLIT_SCREEN);
    force_display_update(cpu);
    return 0;
}
void txt_bus_write_C053(cpu_state *cpu, uint16_t address, uint8_t value) {
    txt_bus_read_C053(cpu, address);
}


uint8_t txt_bus_read_C054(cpu_state *cpu, uint16_t address) {
    // switch to screen 1
    if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Switching to screen 1\n");
    set_display_page1(cpu);
    force_display_update(cpu);
    return 0;
}
void txt_bus_write_C054(cpu_state *cpu, uint16_t address, uint8_t value) {
    txt_bus_read_C054(cpu, address);
}


uint8_t txt_bus_read_C055(cpu_state *cpu, uint16_t address) {
    // switch to screen 2
    if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Switching to screen 2\n");
    set_display_page2(cpu);
    force_display_update(cpu);
    return 0;
}

void txt_bus_write_C055(cpu_state *cpu, uint16_t address, uint8_t value) {
    txt_bus_read_C055(cpu, address);
}


uint8_t txt_bus_read_C056(cpu_state *cpu, uint16_t address) {
    // set lo-res (graphics) mode
    if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Set Lo-Res Mode\n");
    //display_graphics_mode = LORES_MODE;
    set_graphics_mode(cpu, LORES_MODE);
    force_display_update(cpu);
    return 0;
}

void txt_bus_write_C056(cpu_state *cpu, uint16_t address, uint8_t value) {
    txt_bus_read_C056(cpu, address);
}

uint8_t txt_bus_read_C057(cpu_state *cpu, uint16_t address) {
    // set hi-res (graphics) mode
    if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Set Hi-Res Mode\n");
    //display_graphics_mode = HIRES_MODE;
    set_graphics_mode(cpu, HIRES_MODE);
    force_display_update(cpu);
    return 0;
}

void txt_bus_write_C057(cpu_state *cpu, uint16_t address, uint8_t value) {
    txt_bus_read_C057(cpu, address);
}


void display_capture_mouse(cpu_state *cpu, bool capture) {
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
    SDL_SetWindowRelativeMouseMode(ds->window, capture);
}

/**
 * display_state_t Class Implementation
 */
display_state_t::display_state_t() {
    color_mode = DM_COLOR_MODE;
    for (int i = 0; i < 24; i++) {
        dirty_line[i] = 0;
    }
    display_mode = TEXT_MODE;
    display_split_mode = FULL_SCREEN;
    display_graphics_mode = LORES_MODE;
    window = nullptr;
    renderer = nullptr;
    screenTexture = nullptr;
    display_page_num = DISPLAY_PAGE_1;
    display_page_table = &display_pages[display_page_num];
    flash_state = false;
}

void init_mb_device_display(cpu_state *cpu) {
    // alloc and init display state
    display_state_t *ds = new display_state_t;

    // set in CPU so we can reference later
    set_module_state(cpu, MODULE_DISPLAY, ds);
    
    register_C0xx_memory_read_handler(0xC050, txt_bus_read_C050);
    register_C0xx_memory_read_handler(0xC051, txt_bus_read_C051);
    register_C0xx_memory_read_handler(0xC052, txt_bus_read_C052);
    register_C0xx_memory_read_handler(0xC053, txt_bus_read_C053);
    register_C0xx_memory_read_handler(0xC054, txt_bus_read_C054);
    register_C0xx_memory_read_handler(0xC055, txt_bus_read_C055);
    register_C0xx_memory_read_handler(0xC056, txt_bus_read_C056);
    register_C0xx_memory_read_handler(0xC057, txt_bus_read_C057);

    register_C0xx_memory_write_handler(0xC050, txt_bus_write_C050);
    register_C0xx_memory_write_handler(0xC051, txt_bus_write_C051);
    register_C0xx_memory_write_handler(0xC052, txt_bus_write_C052);
    register_C0xx_memory_write_handler(0xC053, txt_bus_write_C053);
    register_C0xx_memory_write_handler(0xC054, txt_bus_write_C054);
    register_C0xx_memory_write_handler(0xC055, txt_bus_write_C055);
    register_C0xx_memory_write_handler(0xC056, txt_bus_write_C056);
    register_C0xx_memory_write_handler(0xC057, txt_bus_write_C057);

    init_display_sdl(ds);
}

void set_display_color_mode(cpu_state *cpu, display_color_mode_t mode) {
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
    ds->color_mode = mode;
}

void toggle_display_color_mode(cpu_state *cpu) {
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
    ds->color_mode = (display_color_mode_t)((ds->color_mode + 1) % DM_NUM_MODES);
}
