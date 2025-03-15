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

#pragma once

#include "gs2.hpp"
#include "cpu.hpp"
#include "platforms.hpp"

#define SCALE_X 2
#define SCALE_Y 4
#define BASE_WIDTH 560
#define BASE_HEIGHT 192
#define BORDER_WIDTH 10
#define BORDER_HEIGHT 10

// Graphics vs Text, C050 / C051
typedef enum {
    TEXT_MODE = 0,
    GRAPHICS_MODE = 1,
} display_mode_t;

// Full screen vs split screen, C052 / C053
typedef enum {
    FULL_SCREEN = 0,
    SPLIT_SCREEN = 1,
} display_split_mode_t;

// Lo-res vs Hi-res, C056 / C057
typedef enum {
    LORES_MODE = 0,
    HIRES_MODE = 1,
} display_graphics_mode_t;

typedef enum {
    LM_TEXT_MODE    = 0,
    LM_LORES_MODE   = 1,
    LM_HIRES_MODE   = 2
} line_mode_t;

typedef enum {
    DM_COLOR_MODE = 0,
    DM_GREEN_MODE,
    DM_AMBER_MODE,
    DM_NUM_MODES,
} display_color_mode_t;


typedef uint16_t display_page_table_t[24] ;

typedef struct display_page_t {
    uint16_t text_page_start;
    uint16_t text_page_end;
    display_page_table_t text_page_table;
    uint16_t hgr_page_start;
    uint16_t hgr_page_end;
    display_page_table_t hgr_page_table;
} display_page_t;

typedef enum {
    DISPLAY_PAGE_1 = 0,
    DISPLAY_PAGE_2,
    NUM_DISPLAY_PAGES
} display_page_number_t;

typedef enum {
    DISPLAY_WINDOWED_MODE = 0,
    DISPLAY_FULLSCREEN_MODE = 1,
    NUM_FULLSCREEN_MODES
} display_fullscreen_mode_t;

typedef class display_state_t {

public:
    display_state_t();

    SDL_Window *window;
    SDL_Renderer* renderer ;
    SDL_Texture* screenTexture;

    display_fullscreen_mode_t display_fullscreen_mode;
    display_color_mode_t color_mode;
    display_mode_t display_mode;
    display_split_mode_t display_split_mode;
    display_graphics_mode_t display_graphics_mode;
    display_page_number_t display_page_num;
    display_page_t *display_page_table;
    bool flash_state;
    int flash_counter;

    uint32_t dirty_line[24];
    line_mode_t line_mode[24]; // 0 = TEXT, 1 = LO RES GRAPHICS, 2 = HI RES GRAPHICS

} display_state_t;


extern uint32_t lores_color_table[16]; 

void force_display_update(cpu_state *cpu);
void update_display(cpu_state *cpu);

uint64_t init_display_sdl(display_state_t *ds);
void free_display(cpu_state *cpu);

void txt_memory_write(uint16_t , uint8_t );
void update_flash_state(cpu_state *cpu);
void init_mb_device_display(cpu_state *cpu, uint8_t slot);
void render_line(cpu_state *cpu, int y);
void pre_calculate_font(rom_data *rd);
void init_display_font(rom_data *rd);
void set_display_color_mode(cpu_state *cpu, display_color_mode_t mode);
void toggle_display_color_mode(cpu_state *cpu);
void toggle_display_fullscreen(cpu_state *cpu);
void update_line_mode(cpu_state *cpu);
void set_display_mode(cpu_state *cpu, display_mode_t mode);
void set_split_mode(cpu_state *cpu, display_split_mode_t mode);
void set_graphics_mode(cpu_state *cpu, display_graphics_mode_t mode);
void display_capture_mouse(cpu_state *cpu, bool capture);
void display_dump_hires_page(cpu_state *cpu, int page);
void display_dump_text_page(cpu_state *cpu, int page);
