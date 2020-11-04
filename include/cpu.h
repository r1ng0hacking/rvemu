#ifndef __CPU_H__
#define __CPU_H__

#include <stdint.h>
#include "cache.h"

#define __CPU_EXEC_INST_DEBUG__

struct cpu {
	uint64_t regfile[32];
	uint64_t pc;
	uint64_t csrs[4096];

	union inst {
		struct  {
			uint32_t opcode:7;
			uint32_t rd:5;
			uint32_t funct3:3;
			uint32_t rs1:5;
			uint32_t rs2:5;
			uint32_t funct7:7;
		}r_type;

		struct  {
			uint32_t opcode:7;
			uint32_t rd:5;
			uint32_t funct3:3;
			uint32_t rs1:5;
			uint32_t imm11_0:12;
		}i_type;

		struct  {
			uint32_t opcode:7;
			uint32_t imm4_0:5;
			uint32_t funct3:3;
			uint32_t rs1:5;
			uint32_t rs2:5;
			uint32_t imm11_5:7;
		}s_type;

		struct  {
			uint32_t opcode:7;
			uint32_t rd:5;
			uint32_t imm31_12:20;
		}u_type;

		struct  {
			uint32_t opcode:7;
			uint32_t imm_11:1;
			uint32_t imm4_1:4;
			uint32_t funct3:3;
			uint32_t rs1:5;
			uint32_t rs2:5;
			uint32_t imm10_5:6;
			uint32_t imm12:1;
		}b_type;

		struct  {
			uint32_t opcode:7;
			uint32_t rd:5;
			uint32_t imm19_12:8;
			uint32_t imm11:1;
			uint32_t imm10_1:10;
			uint32_t imm20:1;
		}j_type;

		uint32_t instruction;
	} inst;

	struct cache icache;// 1-level instruction cache
	struct cache dcache;// 1-level data cache
	struct cache cache;//2-level cache

	struct bus *bus;
};

void dump_registers(struct cpu *cpu);
void cpu_run(struct cpu *cpu);
struct cpu* alloc_cpu(struct ram *ram,struct bus *bus);
#endif
