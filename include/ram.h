#ifndef __RAM_H__
#define __RAM_H__

//#define __RAM_WRITE_DEBUG__
//#define __RAM_READ_DEBUG__
//#define __RAM_LOAD_FILE_DEBUG__

#include <stdint.h>

struct ram {
	uint8_t *data;
	uint64_t size;
};

void load_data_from_file(struct ram*ram,uint64_t addr,char *filename);
void write_to_ram(struct ram*ram,uint64_t addr,uint64_t size,uint8_t *data);
void read_from_ram(struct ram*ram,uint64_t addr,uint64_t size,uint8_t *data);
struct ram *alloc_ram(uint64_t size);
#endif
