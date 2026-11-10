
/*
 * FILE        : ui_b.h
 * PROJECT     : SENG3030 Assignment 4 - Device B
 * DESCRIPTION : Device B display interface.
 */

#ifndef UI_B_H
#define UI_B_H

#include <stdint.h>

#include "readings.h"


// ================= page navigation =================

void uiBChangePage(int step);

Page uiBGetCurrentPage();


// ================= display =================

void uiBDrawScreen(uint32_t nowMs);


#endif // UI_B_H
