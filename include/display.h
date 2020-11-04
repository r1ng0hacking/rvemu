#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#define DISPLAY_START_PHY_ADDR 0xFFFFFFFFFFFF1000
#define DISPLAY_END_PHY_ADDR   0xFFFFFFFFFFFFFFFF

#define DISPLAY_CHAR_PHY_ADDR 0xFFFFFFFFFFFF1000

#include "device.h"

struct display{
	struct device dev;
};

struct device * alloc_display();

#endif
