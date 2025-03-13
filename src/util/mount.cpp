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

#include <unordered_map>

#include "cpu.hpp"
#include "media.hpp"
#include "mount.hpp"
#include "devices/diskii/diskii.hpp"
#include "devices/prodos_block/prodos_block.hpp"

/**
 * Media Key
 * key = (hex) SSUU
 * S = Slot (key >> 8)
 * U = Unit (key & 0xFF)
 **/

int Mounts::register_drive(drive_type_t drive_type, uint64_t key) {
    mounted_media[key].drive_type = drive_type;
    return 0;
}

int Mounts::mount_media(disk_mount_t disk_mount) {

    fprintf(stdout,"Mounting disk %s in slot %d drive %d\n", disk_mount.filename, disk_mount.slot, disk_mount.drive);
    media_descriptor * media = new media_descriptor();
    media->filename = disk_mount.filename;
    if (identify_media(*media) != 0) {
        fprintf(stderr, "Failed to identify media %s\n", disk_mount.filename);
        return false;
    }
    display_media_descriptor(*media);

    uint64_t key = (disk_mount.slot << 8) | disk_mount.drive;
    mounted_media[key].media = media;
    mounted_media[key].key = key;

    // TODO: this should look up what type of disk device is in the slot
    if (disk_mount.slot == 6) { // TODO: instead of based on slot, should be based on the card type in the slot.
        mount_diskII(cpu, disk_mount.slot, disk_mount.drive, media);
        mounted_media[key].drive_type = DRIVE_TYPE_DISKII;
    } else if (disk_mount.slot == 5) {
        mount_prodos_block(disk_mount.slot, disk_mount.drive, media);
        mounted_media[key].drive_type = DRIVE_TYPE_PRODOS_BLOCK;
    } else {
        fprintf(stderr, "Invalid slot. Expected 5 or 6\n");
    }

    return key;
}

int Mounts::unmount_media(disk_mount_t disk_mount) {
    // TODO
    return false;
}

drive_status_t Mounts::media_status(uint64_t key) {
    auto it = mounted_media.find(key);
    if (it == mounted_media.end()) {
        return {false, nullptr, false, 0};
    }
    if (it->second.drive_type == DRIVE_TYPE_DISKII) {
        return diskii_status(cpu, key);
    } /* else if (it->second.drive_type == DRIVE_TYPE_PRODOS_BLOCK) {
        return prodos_block_status(cpu, key);
    } */
   return {false, nullptr, false, 0};
}

void Mounts::dump() {
    for (auto it = mounted_media.begin(); it != mounted_media.end(); it++) {
        drive_status_t status = media_status(it->first);
        //fprintf(stdout, "Mounted media: %llu typ: %d mnt: %d mot:%d pos: %d\n", it->first, it->second.drive_type, status.is_mounted, status.motor_on, status.position);
    }
}