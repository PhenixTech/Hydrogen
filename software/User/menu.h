#ifndef MENU_H
#define MENU_H

#include <stdint.h>
#include <stdbool.h>

void showMenu(void);
void VLflagWarning(void);
void LowBattery(uint8_t level);
extern bool force_refresh;



#endif
