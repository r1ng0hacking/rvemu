#include "cache.h"
#include "ram.h"
#include "device.h"
#include "cpu.h"
#include "bus.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <malloc.h>
#include <assert.h>

static struct cache *caches = NULL;


static uint64_t get_addr_tag(struct cache *cache,uint64_t addr)
{
	return addr >> cache->tag_offset;
}

static uint64_t get_addr_idx(struct cache *cache,uint64_t addr)
{
	return (addr >> cache->idx_offset) & ((1<<cache->idx_bit_number) - 1);
}


static int find_msb(uint64_t x)//x only have 1 bit set.
{
	int i = 0;

	while((x = x>>1)){
		i++;
	}
	return i;
}

void init_cache(struct cache *cache,struct cpu *cpu,char *name,int level,struct ram *ram,struct cache *next)
{
	uint64_t size;
	uint64_t ways;
	uint64_t line_size;

	switch(level){
	case 1:
		size = LEVEL_1_SIZE;
		ways = LEVEL_1_WAYS;
		line_size = CACHE_LINE_SIZE;

		break;
	case 2:
		size = LEVEL_2_SIZE;
		ways = LEVEL_2_WAYS;
		line_size = CACHE_LINE_SIZE;

		break;
	default:
		printf("cache level(%d) error!(%s)\n",level,__func__);
		exit(-1);
		break;
	}

	if(caches == NULL){
		cache->next = NULL;
		caches = cache;
	} else {
		cache->next = caches;
		caches = cache;
	}

	cache->line_size = line_size;
	cache->entrys_count = size/line_size;
	cache->ways = ways;
	cache->name = name;
	cache->ram = ram;
	cache->level = level;
	cache->size = size;
	cache->cpu = cpu;
	cache->next_level = next;

	cache->off_offset = 0;
	cache->off_bit_number = find_msb(line_size);
	cache->idx_offset = cache->off_bit_number;
	cache->idx_bit_number = find_msb(cache->entrys_count / cache->ways);
	cache->tag_offset = cache->off_bit_number + cache->idx_bit_number;
	cache->tag_bit_number = 64 - (cache->off_bit_number + cache->idx_bit_number);

	cache->entrys = malloc(cache->entrys_count * sizeof(struct cache_entry));
	if(cache->entrys == NULL){
		printf("alloc cache memory error:%s",strerror(errno));
		exit(-1);
	}
	memset(cache->entrys,0,cache->entrys_count * sizeof(struct cache_entry));

#ifdef __CACHE_DEBUG_INFO__
	printf("cache level:%d,cache name:%s,cache size:%ld\n",cache->level,cache->name,cache->size);
	printf("cache line size:%ld cache entrys_count:%ld cache ways:%ld\n",cache->line_size,cache->entrys_count,cache->ways);
	printf("cache off_offset:%ld cache off_bit number:%ld\n",cache->off_offset,cache->off_bit_number);
	printf("cache idx_offset:%ld cache idx_bit_number:%ld\n",cache->idx_offset,cache->idx_bit_number);
	printf("cache tag_offset:%ld cache tag_bit_number:%ld\n\n",cache->tag_offset,cache->tag_bit_number);
#endif
}

static void make_line_accessed(struct cache_line_info line_info)
{
	struct cache_entry *line = line_info.set + line_info.idx_in_set;
	struct cache_entry *set  = line_info.set;
	if(set->head == NULL){
		set->head = line;
		line->next = line;
		line->prev = line;
	} else if(set->head != line) {
		if(line->next != NULL && line->prev !=NULL){
			line->prev->next = line->next;
			line->next->prev = line->prev;
		}
		line->prev = set->head->prev;
		line->next = set->head;
		set->head->prev->next = line;
		set->head->prev = line;


		set->head = line;
	}
}


static int line_valid(struct cache_entry *line)
{
	int ret = 0;

	switch(line->coherency_state){
	case CACHE_LINE_COHERENCY_SHARED_STATE:
		ret = 1;
		break;
	case CACHE_LINE_COHERENCY_MODIFIED_STATE:
		ret = 1;
		break;
	case CACHE_LINE_COHERENCY_INVALID_STATE:
		ret = 0;
		break;
	}

	return ret;
}

static int find_line_in_set(struct cache_entry *set,uint64_t ways,uint64_t tag)
{
	int idx = -1;

	for(int i = 0;i<ways;i++){
		if(line_valid(set+i) && set[i].tag == tag){
			idx = i;
			break;
		}
	}

	return idx;
}

static struct cache_line_info find_in_cache(struct cache *cache,uint64_t addr)
{
	struct cache_line_info ret = {0};
	struct cache_entry *set;
	int hit_idx_in_set;
	uint64_t tag;
	uint64_t idx;

	idx = get_addr_idx(cache,addr);
	tag = get_addr_tag(cache,addr);

	set = cache->entrys+ idx*cache->ways;

	hit_idx_in_set = find_line_in_set(set, cache->ways, tag);

	ret.cache = cache;
	ret.idx_in_set = hit_idx_in_set;
	ret.set = set;
	return ret;
}

static struct cache_line_info find_in_other_cache(struct cache *current_cache,uint64_t addr)
{
	struct cache_line_info line_info,ret;

	struct cache *cache = caches;
	while(cache){
		if(cache != current_cache){
			line_info = find_in_cache(cache, addr);

			if(line_info.idx_in_set != -1) return line_info;
		}
		cache = cache->next;
	}

	ret.idx_in_set = -1;
	return ret;
}

static struct cache_line_info find_lru_line_in_cache(struct cache *cache,uint64_t addr)
{
	struct cache_line_info ret = {0};
	int ways = cache->ways;

	uint64_t idx;
	struct cache_entry *set;

	idx = get_addr_idx(cache,addr);

	set = cache->entrys+ idx*cache->ways;

	ret.cache = cache;
	ret.set = set;

	//find invalid
	for(int i = 0;i<ways;i++){
		if(!line_valid(set+i)){
			ret.idx_in_set = i;
			return ret;
		}
	}

	ret.idx_in_set = set->head->prev - set;
	return ret;
}

static uint64_t get_addr_from_lineinfo(struct cache_line_info line_info)
{
	struct cache *cache = line_info.cache;
	struct cache_entry *line = line_info.set + line_info.idx_in_set;


	return (line->tag << cache->tag_offset) | (((cache->entrys - line)/cache->ways)<<cache->idx_offset);
}

static void writeback_cache_line(struct cache_line_info line_info)
{
	struct cache *cache = line_info.cache;
	struct cache_entry *line = line_info.set + line_info.idx_in_set;
	uint64_t addr = get_addr_from_lineinfo(line_info);
	struct ram *ram;

#ifdef __CACHE_DEBUG_LRU__
	printf("lru writeback: cache name:%s addr:0x%lx\n",cache->name,addr);
#endif

	while(!cache->ram){
		cache = cache->next_level;
	}
	ram = cache->ram;

	write_to_ram(ram, addr, CACHE_LINE_SIZE, line->data);
}

static void *read_line_from_ram(struct cache *cache,uint64_t addr)
{
	void *data;
	struct cache_line_info lru_line_info = find_lru_line_in_cache(cache, addr);
	struct cache_entry *lru_line = lru_line_info.set+lru_line_info.idx_in_set;

	if(lru_line->coherency_state == CACHE_LINE_COHERENCY_MODIFIED_STATE){
		writeback_cache_line(lru_line_info);
	}

#ifdef __CACHE_DEBUG_LRU__
	printf("lru: cache name:%s set:%p level:%d idx_in_set:%d\n",cache->name,lru_line_info.set,cache->level,lru_line_info.idx_in_set);
#endif

	if(cache->ram == NULL){
		data = read_line_from_ram(cache->next_level,addr);
		memcpy(lru_line->data,data,CACHE_LINE_SIZE);
	}else{
		read_from_ram(cache->ram, addr, CACHE_LINE_SIZE, lru_line->data);
	}

	lru_line->tag = get_addr_tag(cache, addr);
	lru_line->coherency_state = CACHE_LINE_COHERENCY_SHARED_STATE;
	make_line_accessed(lru_line_info);
	return lru_line->data;
}

static void *read_line_from_other_cache(struct cache *current,struct cache *other,uint64_t addr)
{
	struct cache_line_info lru_line_info = find_lru_line_in_cache(current, addr);
	struct cache_entry *lru_line = lru_line_info.set+lru_line_info.idx_in_set;
	struct cache_line_info line_info = find_in_cache(other,addr);
	struct cache_entry *line = line_info.set + line_info.idx_in_set;

	if(line->coherency_state == CACHE_LINE_COHERENCY_MODIFIED_STATE){
		line->coherency_state = CACHE_LINE_COHERENCY_SHARED_STATE;
		writeback_cache_line(line_info);
	}

	memcpy(lru_line->data,line->data,CACHE_LINE_SIZE);

	lru_line->tag = get_addr_tag(current, addr);
	lru_line->coherency_state = CACHE_LINE_COHERENCY_SHARED_STATE;
	make_line_accessed(lru_line_info);
	return lru_line->data;
}

static void invalid_other_cache_lines(struct cache*cur,uint64_t addr)
{
	struct cache_line_info line_info;
	struct cache *cache = caches;
	while(cache){
		if(cache != cur){
			line_info = find_in_cache(cache, addr);

			if(line_info.idx_in_set != -1){
				(line_info.set + line_info.idx_in_set)->coherency_state = CACHE_LINE_COHERENCY_INVALID_STATE;
			}
		}
		cache = cache->next;
	}
}

static void read_write_cache_line(struct cache *cache,uint64_t addr,void *data,uint8_t read)
{
	struct cache_line_info other_line_info;
	struct cache_line_info line_info;
	void *line_data;

	line_info = find_in_cache(cache,addr);

	if(read){//read
		if(line_info.idx_in_set == -1){//not found in local cache
			other_line_info = find_in_other_cache(cache, addr);
			if(other_line_info.idx_in_set == -1){//read from memory
				line_data = read_line_from_ram(cache,addr);
				memcpy(data,line_data,CACHE_LINE_SIZE);
			} else { // found in other local cache
				line_data = read_line_from_other_cache(cache,other_line_info.cache,addr);
				memcpy(data,line_data,CACHE_LINE_SIZE);
			}
		}else{//found in local cache
			memcpy(data,(line_info.set + line_info.idx_in_set)->data,CACHE_LINE_SIZE);
			make_line_accessed(line_info);
		}
	}else{//write,must hit
		assert(line_info.idx_in_set != -1);
		invalid_other_cache_lines(line_info.cache,addr);
		memcpy((line_info.set + line_info.idx_in_set)->data,data,CACHE_LINE_SIZE);
		(line_info.set + line_info.idx_in_set)->coherency_state = CACHE_LINE_COHERENCY_MODIFIED_STATE;
	}
}

uint8_t get_byte_from_cache(struct cache *cache,uint64_t addr)
{
	uint8_t data[CACHE_LINE_SIZE] = {0};
	uint64_t base_addr = (addr & (~(uint64_t)(CACHE_LINE_SIZE - 1)));
	uint64_t offset = addr - base_addr;
	read_write_cache_line(cache, base_addr, data, 1);//read

	return data[offset];
}

uint16_t get_word_from_cache(struct cache *cache,uint64_t addr)
{
	return (uint16_t)get_byte_from_cache(cache,addr) | (uint16_t)(get_byte_from_cache(cache,addr+1)<<8);
}

uint32_t get_dword_from_cache(struct cache *cache,uint64_t addr)
{
	return (uint32_t)get_word_from_cache(cache,addr) | (uint32_t)get_word_from_cache(cache,addr+2)<<16;
}

uint64_t get_qword_from_cache(struct cache *cache,uint64_t addr)
{
	return (uint64_t)get_dword_from_cache(cache,addr) | (uint64_t)get_dword_from_cache(cache,addr+4)<<32;
}

void put_byte_to_cache(struct cache *cache,uint64_t addr,uint8_t x)// read before write
{

	struct cpu *cpu = cache->cpu;
	struct device *dev;
	dev = find_device(cpu->bus, addr);

	uint8_t data[CACHE_LINE_SIZE] = {0};
	uint64_t base_addr = (addr & (~(uint64_t)(CACHE_LINE_SIZE - 1)));
	uint64_t offset = addr - base_addr;

	if(dev == NULL || dev->write_byte_func == NULL){ // write memory
		read_write_cache_line(cache, base_addr, data, 1);//read
		data[offset] = x;
		read_write_cache_line(cache, base_addr, data, 0);//write
	} else {
		dev->write_byte_func(dev,addr,x);
	}
}

void put_word_to_cache(struct cache *cache,uint64_t addr,uint16_t x)
{
	put_byte_to_cache(cache,addr,x);
	put_byte_to_cache(cache,addr+1,x>>8);
}

void put_dword_to_cache(struct cache *cache,uint64_t addr,uint32_t x)
{
	put_word_to_cache(cache,addr,x);
	put_word_to_cache(cache,addr+2,x>>16);
}

void put_qword_to_cache(struct cache *cache,uint64_t addr,uint64_t x)
{
	put_dword_to_cache(cache,addr,x);
	put_dword_to_cache(cache,addr+4,x>>32);
}

void put_data_to_cache(struct cache *cache,uint64_t addr,uint64_t pdata,uint64_t len)
{

}

void print_caches(void)
{
	struct cache *cache = NULL;

	cache = caches;
	printf("all caches:\n");
	while(cache){
		printf("\t%s\n",cache->name);
		cache = cache->next;
	}
}
