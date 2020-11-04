#ifndef __CACHE_H__
#define __CACHE_H__

#include <stdint.h>

//#define __CACHE_DEBUG_INFO__
//#define __CACHE_DEBUG_LRU__


#define LEVEL_1_SIZE (32*1024)    //32k
#define LEVEL_2_SIZE (512*1024)   //512k

#define CACHE_LINE_SIZE 64

#define LEVEL_1_WAYS 8
#define LEVEL_2_WAYS 16

#define CACHE_LINE_COHERENCY_INVALID_STATE 0
#define CACHE_LINE_COHERENCY_MODIFIED_STATE 1
#define CACHE_LINE_COHERENCY_SHARED_STATE 2

struct cache_entry{
	uint64_t tag;
	uint8_t data[CACHE_LINE_SIZE];

	uint8_t coherency_state;

	//LRU
	struct cache_entry *head;
	struct cache_entry *prev;
	struct cache_entry *next;
};

struct cache {
	char *name;
	int level;

	struct cache *next_level;
	struct cache *next;//cache list
	struct cache_entry *entrys;

	uint64_t entrys_count;
	uint64_t size;

	uint64_t line_size;
	uint64_t ways;

	uint64_t off_offset;
	uint64_t off_bit_number;
	uint64_t idx_offset;
	uint64_t idx_bit_number;
	uint64_t tag_offset;
	uint64_t tag_bit_number;


	struct ram *ram;
	struct cpu *cpu;
};

struct cache_line_info {
	struct cache_entry *set;
	int idx_in_set;
	struct cache *cache;
};

void print_caches(void);

void put_byte_to_cache(struct cache *cache,uint64_t addr,uint8_t x);
void put_word_to_cache(struct cache *cache,uint64_t addr,uint16_t x);
void put_dword_to_cache(struct cache *cache,uint64_t addr,uint32_t x);
void put_qword_to_cache(struct cache *cache,uint64_t addr,uint64_t x);

uint8_t get_byte_from_cache(struct cache *cache,uint64_t addr);
uint16_t get_word_from_cache(struct cache *cache,uint64_t addr);
uint32_t get_dword_from_cache(struct cache *cache,uint64_t addr);
uint64_t get_qword_from_cache(struct cache *cache,uint64_t addr);
void init_cache(struct cache *cache,struct cpu *cpu,char *name,int level,struct ram *ram,struct cache *next);

#endif
