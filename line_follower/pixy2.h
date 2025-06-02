#ifndef PIXY2_H
#define PIXY2_H

#include <stdint.h>

bool pixy2_init();
int pixy2_get_line_error();  // Returns -100 to +100, or LINE_NOT_FOUND

#endif