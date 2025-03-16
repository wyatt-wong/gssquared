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

#include <unordered_map>

#include "cpu.hpp"
#include "media.hpp"

typedef struct {
    int slot;
    int drive;
    char *filename;
    media_descriptor *media;
} disk_mount_t;

struct drive_status_t {
    bool is_mounted;
    const char *filename;
    bool motor_on;
    int position;
};

enum drive_type_t {
    DRIVE_TYPE_DISKII,
    DRIVE_TYPE_PRODOS_BLOCK,
};

struct drive_media_t {
    uint64_t key;
    drive_type_t drive_type;
    media_descriptor *media;
};

class Mounts {
protected:
    cpu_state *cpu;

    std::unordered_map<uint64_t, drive_media_t> mounted_media;

public:
    Mounts(cpu_state *cpux) : cpu(cpux) {}
    int mount_media(disk_mount_t disk_mount);
    int unmount_media(disk_mount_t disk_mount);
    drive_status_t media_status(uint64_t key);
    int register_drive(drive_type_t drive_type, uint64_t key);
    void dump();
};


