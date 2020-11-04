#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <stdint.h>
#include "cpu.h"
#include "ram.h"
#include "cache.h"
#include "bus.h"
#include "device.h"
#include "display.h"

struct ram *ram;
struct cpu *cpu;
struct bus *bus;
struct device *dp;

int main(int argc,char *argv[])
{
	if(argc != 2){
		printf("Usage:%s file_name",argv[0]);
		exit(-1);
	}

	bus = alloc_bus();
	dp = alloc_display();
	add_device(bus, dp);

	ram = alloc_ram(50*1024*1024);//50M
	load_data_from_file(ram,0,argv[1]);
	cpu = alloc_cpu(ram,bus);

	cpu_run(cpu);


	return 0;
}
