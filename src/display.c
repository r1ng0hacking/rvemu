#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "display.h"

static void display_write_byte(struct device *dev,uint64_t addr,uint8_t data)
{
	if(addr == DISPLAY_CHAR_PHY_ADDR){
		putchar(data);
		fflush(stdout);
	} else{
		printf("%s: write error addr\n",__func__);
		exit(-1);
	}
}

struct device * alloc_display()
{
	struct display *dp = malloc(sizeof(struct display));

	dp->dev.start_addr = DISPLAY_START_PHY_ADDR;
	dp->dev.end_addr   = DISPLAY_END_PHY_ADDR;
	dp->dev.read_byte_func  = NULL;
	dp->dev.write_byte_func = display_write_byte;

	return (struct device *)dp;
}
