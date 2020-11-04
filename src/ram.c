#include "ram.h"
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

struct ram *alloc_ram(uint64_t size)
{
	struct ram *ram = malloc(sizeof(struct ram));
	if(ram == NULL) {
		printf("alloc ram error:%s\n",strerror(errno));
		exit(-1);
	}

	ram->data = malloc(size);
	if(ram->data == NULL) {
		printf("alloc ram data error:%s\n",strerror(errno));
		exit(-1);
	}
	ram->size = size;

	return ram;
}

void write_to_ram(struct ram*ram,uint64_t addr,uint64_t size,uint8_t *data)
{
	addr = addr%ram->size;//wrap round

	if(addr + size > ram->size){
		size = ram->size - addr;
	}

#ifdef __RAM_WRITE_DEBUG__
	printf("%s: addr:0x%lx size:%ld\n",__func__,addr,size);
#endif

	memcpy(ram->data + addr,data,size);
}

void read_from_ram(struct ram*ram,uint64_t addr,uint64_t size,uint8_t *data)
{
	addr = addr%ram->size;//wrap round

	if(addr + size > ram->size){
		size = ram->size - addr;
	}

#ifdef __RAM_READ_DEBUG__
	printf("%s: addr:0x%lx size:%ld\n",__func__,addr,size);
#endif

	memcpy(data,ram->data + addr,size);
}

static uint64_t get_file_size(char *filename)
{
	uint64_t filesize = -1;
	struct stat statbuff;
	if(stat(filename, &statbuff) < 0){
		return filesize;
	}else{
		filesize = statbuff.st_size;
	}
	return filesize;

}

void load_data_from_file(struct ram*ram,uint64_t addr,char *filename)
{
	int ret;
	int fd;
	int size;
	addr = addr % ram->size;
	uint64_t filesize = get_file_size(filename);

	if(filesize == -1){
		printf("get %s file size error(%s)\n",filename,strerror(errno));
		exit(-1);
	}

#ifdef __RAM_LOAD_FILE_DEBUG__
	printf("%s filesize:%ld\n",filename,filesize);
#endif

	size = filesize;
	if(addr + filesize > ram->size){
		size = ram->size - addr;
	}

	fd = open(filename,O_RDONLY);
	if(fd == -1){
		printf("open %s file error(%s)\n",filename,strerror(errno));
		exit(-1);
	}

	ret = read(fd,ram->data+addr,size);
	if(ret == -1){
		printf("read %s file error(%s)\n",filename,strerror(errno));
		exit(-1);
	}
}
