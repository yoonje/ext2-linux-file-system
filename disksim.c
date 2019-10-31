#include <stdlib.h>
#include <memory.h>
#include "ext2.h"
#include "disk.h"
#include "disksim.h"

typedef struct {
    char *address;
} DISK_MEMORY;

int disksim_init(SECTOR numberOfSectors, unsigned int bytesPerSector, DISK_OPERATIONS *disk) {
    if (disk == NULL)
        return -1;

    disk->pdata = malloc(sizeof(DISK_MEMORY));
    if (disk->pdata == NULL) {
        disksim_uninit(disk);
        return -1;
    }

    ((DISK_MEMORY *) disk->pdata)->address = (char *) malloc(bytesPerSector * numberOfSectors);
    if (disk->pdata == NULL) {
        disksim_uninit(disk);
        return -1;
    }

    memset(((DISK_MEMORY *) disk->pdata)->address, 0, bytesPerSector * (numberOfSectors));

    disk->read_sector = disksim_read;
    disk->write_sector = disksim_write;
    disk->numberOfSectors = numberOfSectors;
    disk->bytesPerSector = bytesPerSector;
    return 0;
}

void disksim_uninit(DISK_OPERATIONS *this) {
    if (this) {
        if (this->pdata)
            free(this->pdata);
    }
}

int disksim_read(DISK_OPERATIONS *this, SECTOR sector, void *data) {
    char *disk = (char *) ((DISK_MEMORY *) this->pdata)->address;
    if (sector < 0 || sector >= NUMBER_OF_SECTORS)
        return -1;
    memcpy(data, &disk[sector * this->bytesPerSector], this->bytesPerSector);

    return 0;
}

int disksim_write(DISK_OPERATIONS *this, SECTOR sector, const void *data) {
    char *disk = (char *) ((DISK_MEMORY *) this->pdata)->address;
    if (sector < 0 || sector >= NUMBER_OF_SECTORS)
        return -1;
    memcpy(&disk[sector * this->bytesPerSector], data, this->bytesPerSector);
    return 0;
}