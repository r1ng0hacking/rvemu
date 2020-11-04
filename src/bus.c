#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "bus.h"

struct bus* alloc_bus()
{
	struct bus *bus = malloc(sizeof(struct bus));
	if(bus == NULL){
		printf("alloc bus error(%s)\n",strerror(errno));
		exit(-1);
	}

	memset(bus,0,sizeof(struct bus));

	return bus;
}

void add_device(struct bus *bus,struct device *dev)
{
	assert(bus != NULL);
	assert(dev != NULL && dev->next == NULL);

	if(bus->devices == NULL){
		bus->devices = dev;
		dev->next = NULL;
	}else{
		dev->next = bus->devices;
		bus->devices = dev;
	}
}

struct device* find_device(struct bus *bus,uint64_t addr)
{
	struct device *dev = bus->devices;
	struct device *ret = NULL;

	while(dev){
		if(dev->start_addr<= addr && addr <dev->end_addr){
			ret = dev;
			break;
		}
		dev = dev->next;
	}

	return ret;
}
