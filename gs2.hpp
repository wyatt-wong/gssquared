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

#define USE_SDL2 1

#include <unistd.h>
#include <stdint.h>

/* Address types */
typedef uint8_t zpaddr_t;
typedef uint16_t absaddr_t;

/* Data bus types */
typedef uint8_t byte_t;
typedef uint16_t word_t;
typedef uint8_t opcode_t;

typedef struct gs2_app_t {
    const char *base_path;
    bool console_mode = false;
} gs2_app_t;

extern gs2_app_t gs2_app_values;
