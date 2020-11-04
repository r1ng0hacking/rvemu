#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <stdint.h>

struct device;

typedef uint8_t (*device_read_byte_func)(struct device *dev,uint64_t addr);
typedef void (*device_write_byte_func)(struct device *dev,uint64_t addr,uint8_t data);

struct device{
	struct device *next;
	uint64_t start_addr,end_addr;

	device_read_byte_func  read_byte_func;
	device_write_byte_func write_byte_func;
};

#endif
