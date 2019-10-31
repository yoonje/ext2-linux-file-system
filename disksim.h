#ifndef _DISKSIM_H_
#define _DISKSIM_H_

#include "common.h"

int disksim_init(SECTOR, unsigned int, DISK_OPERATIONS *);

void disksim_uninit(DISK_OPERATIONS *);

int disksim_read(DISK_OPERATIONS *this, SECTOR sector, void *data);

int disksim_write(DISK_OPERATIONS *this, SECTOR sector, const void *data);

#endif
