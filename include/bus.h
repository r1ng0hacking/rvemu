#ifndef __BUS_H__
#define __BUS_H__

#include "device.h"

struct bus{
	int nr_devices;
	struct device *devices;
};

struct bus* alloc_bus();
void add_device(struct bus *bus,struct device *dev);
struct device* find_device(struct bus *bus,uint64_t addr);

#endif
