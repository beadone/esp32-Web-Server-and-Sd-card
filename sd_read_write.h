#ifndef __SD_READ_WRITE_H
#define __SD_READ_WRITE_H

#include "Arduino.h"
#include "FS.h"


void listDir(fs::FS &fs, const char * dirname, uint8_t levels);


#endif
