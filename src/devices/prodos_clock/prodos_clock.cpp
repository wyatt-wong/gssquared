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

#include <time.h>
#include <stdint.h>
#include <stdio.h>

#include "debug.hpp"

#include "cpu.hpp"

#include "bus.hpp"
#include "memory.hpp"

#include "prodos_clock.hpp"

#include "util/ResourceFile.hpp"


/**
 *
 * from prodos8 manual:
  The ProDOS clock driver expects the clock card to send an ASCII string to the GETLN input buffer ($200).
  This string must have the following format (including the commas):
  mo,da,dt,hr,mn
  
  where:
  mo is the month (01 = January...12 = December)
  da is the day of the week (00 = Sunday...06 = Saturday)
  dt is the date (01 through 31)
  hr is the hour (00 through 23)
  mn is the minute (00 through 59)
  
  For example:
  
  07,04,14,22,46
  
 */

/**
.org $C100
.byte $08
.byte $60
.byte $28
.byte $60
.byte $58
.byte $60
.byte $70
.byte $60

rd: jmp foo
wr: rts
  rts
  rts
  
foo:
  php
  pha
  lda #$AE
  STA $c090
  pla
  plp
  rts

 */

void prodos_clock_getln_handler(cpu_state *cpu, char *buf) {
    time_t now = time(nullptr);
    struct tm *tm = localtime(&now);

    snprintf(buf, 255, "%02d,%02d,%02d,%02d,%02d\r", tm->tm_mon + 1, tm->tm_wday, tm->tm_mday, tm->tm_hour, tm->tm_min);
    for (int i = 0; buf[i] != '\0'; i++) {
        raw_memory_write(cpu, 0x200 + i, buf[i] | 0x80);
    }
}

void prodos_clock_write_register(cpu_state *cpu, uint16_t address, uint8_t value) {
    prodos_clock_state * prodosclock_d = (prodos_clock_state *)get_module_state(cpu, MODULE_PRODOS_CLOCK);
    fprintf(stderr, "prodos_clock_write_register: %04x %02x\n", address, value);
    if (value == PRODOS_CLOCK_GETLN_TRIGGER) {
        prodos_clock_getln_handler(cpu, prodosclock_d->buf);
    }
}

void init_slot_prodosclock(cpu_state *cpu, uint8_t slot) {
    fprintf(stderr, "ProDOS_Clock init at SLOT %d\n", slot);

    prodos_clock_state * prodosclock_d = new prodos_clock_state;

    set_module_state(cpu, MODULE_PRODOS_CLOCK, prodosclock_d);

    // load the firmware into the slot memory
    uint8_t slx = 0x80 + (slot * 0x10) + PRODOS_CLOCK_PV_TRIGGER;
    uint8_t sly = 0xC0 + slot;
    uint8_t pdfirm[256] = {
        0x08, 0x60, 0x28, 0x60, 0x58, 0x60, 0x70, 0x60,
        0x4C, 0x0E, sly,  0x60, 0x60, 0x60, 0x08, 0x48,
        0xA9, 0xAE, 0x8D, slx,  0xC0, 0x68, 0x28, 0x60
    };
    for (int i = 0; i < 24; i++) {
        raw_memory_write(cpu, 0xC000 + (slot * 0x0100) + i, pdfirm[i]);
    }
    for (int i = 24; i < 256; i++) {
        raw_memory_write(cpu, 0xC000 + (slot * 0x0100) + i, 0x60);
    }

    register_C0xx_memory_write_handler(0xC000 + slx, prodos_clock_write_register);

}
