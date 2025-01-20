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

#include <cstdio>
#include <unistd.h>
#include <cstdlib>

#include "gs2.hpp"
#include "memory.hpp"
#include "debug.hpp"
#include "bus.hpp"
#include "display.hpp"
#include "platforms.hpp"


// Apple II+ Character Set (7x8 pixels)
// Characters 0x20 through 0x7F
#define CHAR_GLYPHS_COUNT 256
#define CHAR_GLYPHS_SIZE 8

// The Character ROM contains 256 entries (total of 2048 bytes).
// Each character is 8 bytes, each byte is 1 row of pixels.

uint32_t APPLE2_FONT_32[CHAR_GLYPHS_COUNT * CHAR_GLYPHS_SIZE * 8 ]; // 8 pixels per row.

/** pre-render the text mode font into a 32-bit bitmap - directly ready to shove into texture*/
/**
 * input: APPLE2_FONT 
 * output: APPLE2_FONT_32
*/

void pre_calculate_font (rom_data *rd) {
    // Draw the character bitmap into a memory array. (32 bit pixels, 7 sequential pixels per row, 8 rows per character)
    // each char will be 56 pixels or 224 bytes

    uint32_t fg_color = 0xFFFFFFFF;
    uint32_t bg_color = 0x00000000;

    uint32_t pos_index = 0;

    for (int row = 0; row < 256 * 8; row++) {
        uint8_t rowBits = (*rd->char_rom_data)[row];
        for (int col = 0; col < 7; col++) {
            bool pixel = rowBits & (1 << (6 - col));
            uint32_t color = pixel ? fg_color : bg_color;
            APPLE2_FONT_32[pos_index] = color;
            pos_index++;
            if (pos_index > CHAR_GLYPHS_COUNT * CHAR_GLYPHS_SIZE * 8 ) {
                fprintf(stderr, "pos_index out of bounds: %d\n", pos_index);
                exit(1);
            }
        }
    }
}

uint32_t text_color_table[DM_NUM_MODES] = {
    0xFFFFFFFF, // color, keep it as-is
    0xFFBF00FF, // amber.
    0x009933FF, // green.
};

/**
 * render single character - in context of a locked texture of a whole display line.
 */
void render_text(cpu_state *cpu, int x, int y, void *pixels, int pitch) {

    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
    display_color_mode_t color_mode = ds->color_mode;
    uint32_t color_value = text_color_table[color_mode];
    uint16_t *TEXT_PAGE_TABLE = ds->display_page_table->text_page_table;

    // Bounds checking
    if (x < 0 || x >= 40 || y < 0 || y >= 24) {
        return;
    }

    uint8_t character = raw_memory_read(cpu, TEXT_PAGE_TABLE[y] + x);

    // Calculate font offset (8 bytes per character, starting at 0x20)
    const uint32_t* charPixels = &APPLE2_FONT_32[character * 56];

    bool inverse = false;
    // Check if top two bits are 0 (0x00-0x3F range)
    if ((character & 0xC0) == 0) {
        inverse = true;
    } else if (((character & 0xC0) == 0x40)) {
        inverse = ds->flash_state;
    }
    
    // for inverse, xor the pixels with 0xFFFFFFFF to invert them.
    uint32_t xor_mask = 0x00000000;
    if (inverse) {
        xor_mask = 0xFFFFFFFF;
    }

    int pitchoff = pitch / 4;
    int charoff = x * 7;
    // Draw the character bitmap into the texture
    uint32_t* texturePixels = (uint32_t*)pixels;
    for (int row = 0; row < 8; row++) {
        uint32_t base = row * pitchoff;
        texturePixels[base + charoff ] = (charPixels[0] ^ xor_mask) & color_value;
        texturePixels[base + charoff + 1] = (charPixels[1] ^ xor_mask) & color_value;
        texturePixels[base + charoff + 2] = (charPixels[2] ^ xor_mask) & color_value;
        texturePixels[base + charoff + 3] = (charPixels[3] ^ xor_mask) & color_value;
        texturePixels[base + charoff + 4] = (charPixels[4] ^ xor_mask) & color_value;
        texturePixels[base + charoff + 5] = (charPixels[5] ^ xor_mask) & color_value;
        texturePixels[base + charoff + 6] = (charPixels[6] ^ xor_mask) & color_value;
        charPixels += 7;
    }
}

void update_flash_state(cpu_state *cpu) {
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
    display_page_t *display_page = ds->display_page_table;
    uint16_t *TEXT_PAGE_TABLE = display_page->text_page_table;

    // 2 times per second (every 30 frames), the state of flashing characters (those matching 0b01xxxxxx) must be reversed.
    static int flash_counter = 0;
    
    if (++flash_counter < 30) {
        return;
    }
    flash_counter = 0;
    ds->flash_state = !ds->flash_state;

    //int flashcount = 0;
    // this is actually very wrong.
    //TODO: invert the loops. And, we can bail on the inner loop after we find the first flash character.

    for (int y = 0; y < 24; y++) {
        for (int x = 0; x < 40; x++) {
            uint16_t addr = TEXT_PAGE_TABLE[y] + x;
            uint8_t character = raw_memory_read(cpu, addr);
            if ((character & 0b11000000) == 0x40) {
                // mark line as dirty
                ds->dirty_line[y] = 1;
                break; // stop after we find any flash char.
                //render_text(x, y, character, flash_state);
                //flashcount++;
            }
        }
    }
    //if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Flash chars updated: %d\n", flashcount);
}


// write a character to the display memory based on the text page memory address.
// converts address to an X,Y coordinate then calls render_text to do it.
void txt_memory_write(cpu_state *cpu, uint16_t address, uint8_t value) {
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
    uint16_t TEXT_PAGE_START = ds->display_page_table->text_page_start;
    uint16_t TEXT_PAGE_END = ds->display_page_table->text_page_end;

    // Strict bounds checking for text page 1
    if (address < TEXT_PAGE_START || address > TEXT_PAGE_END) {
        return;
    }

    // Convert text memory address to screen coordinates
    uint16_t addr_rel = address - TEXT_PAGE_START;
    
    // Each superrow is 128 bytes apart
    uint8_t superrow = addr_rel >> 7;      // Divide by 128 to get 0 or 1
    uint8_t superoffset = addr_rel & 0x7F; // Get offset within the 128-byte block
    
    uint8_t subrow = superoffset / 40;     // Each row is 40 characters
    uint8_t charoffset = superoffset % 40;
    
    // Calculate final screen position
    uint8_t y_loc = (subrow * 8) + superrow; // Each superrow contains 8 rows
    uint8_t x_loc = charoffset;

    if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Address: $%04X -> dirty line y:%d (value: $%02X)\n", 
           address, y_loc, value); // Debug output

    // Extra bounds verification
    if (x_loc >= 40 || y_loc >= 24) {
        if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Invalid coordinates calculated: x=%d, y=%d from addr=$%04X\n", 
               x_loc, y_loc, address);
        return;
    }

    if (ds->display_mode == GRAPHICS_MODE && ds->display_split_mode == SPLIT_SCREEN) {
        // update lines 21 - 24
        if (y_loc >= 20 && y_loc < 24) {
            ds->dirty_line[y_loc] = 1;
        }
    }

    // update any line.
    ds->dirty_line[y_loc] = 1;
}



uint8_t txt_bus_read(cpu_state *cpu, uint16_t address) {
    return 0;
}


