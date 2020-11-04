#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include "ram.h"
#include "cpu.h"
#include "cache.h"
#include "bus.h"

struct cpu* alloc_cpu(struct ram *ram,struct bus *bus)
{
	struct cpu *cpu = malloc(sizeof(struct cpu));

	if(cpu == NULL) {
		printf("alloc cpu error:%s",strerror(errno));
		exit(-1);
	}
	memset(cpu,0,sizeof(struct cpu));

	init_cache(&cpu->icache,cpu,"icache",1,NULL,&cpu->cache);
	init_cache(&cpu->dcache,cpu,"dcache",1,NULL,&cpu->cache);
	init_cache(&cpu->cache,cpu,"cache",2,ram,NULL);

	cpu->regfile[2]	= ram->size;//sp

	cpu->bus = bus;
	return cpu;
}


static uint64_t get_register(struct cpu *cpu,int idx)
{
	return idx == 0 ? 0 : cpu->regfile[idx];
}

static void set_register(struct cpu *cpu,int idx,uint64_t data)
{
	cpu->regfile[idx] = data;
}

void dump_registers(struct cpu *cpu)
{
	for(int i = 0;i<8;i++){
		printf("x%d:0x%lx\t\t\t",i*4,get_register(cpu,i*4));
		printf("x%d:0x%lx\t\t\t",i*4+1,get_register(cpu,i*4+1));
		printf("x%d:0x%lx\t\t\t",i*4+2,get_register(cpu,i*4+2));
		printf("x%d:0x%lx\n",i*4+3,get_register(cpu,i*4+3));
	}
}


static void cpu_fetch(struct cpu *cpu)
{
	cpu->inst.instruction = get_dword_from_cache(&cpu->icache, cpu->pc);
	cpu->pc += 4;
}

static void cpu_exec(struct cpu *cpu)
{
	int64_t sign_imm_64;
	uint64_t unsign_imm_64;
	uint8_t rd_idx;
	uint8_t rs1_idx;
	uint8_t rs2_idx;

	uint64_t orig_pc_plus4;
	int64_t mem_sign_data;
	int64_t tmp_int64_t;
	int64_t tmp_int32_t;

	uint64_t csr;
	switch(cpu->inst.r_type.opcode){
	case 0x1B://I type
		unsign_imm_64 = (uint64_t)cpu->inst.i_type.imm11_0;
		sign_imm_64 = (int64_t)cpu->inst.i_type.imm11_0;
		if(sign_imm_64 & 0x800){
			sign_imm_64 |= (~0x7ff);
		}
		rd_idx = cpu->inst.i_type.rd;
		rs1_idx = cpu->inst.i_type.rs1;
		switch(cpu->inst.i_type.funct3){
		case 0://addiw
			tmp_int64_t =  get_register(cpu,rs1_idx) + sign_imm_64;
			tmp_int64_t = (tmp_int64_t<<32)>>32;
			set_register(cpu, rd_idx, tmp_int64_t);
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("addiw\tx%d,x%d,%ld\n",rd_idx,rs1_idx,sign_imm_64);
#endif
			break;
		case 1://slliw
			tmp_int64_t = get_register(cpu,rs1_idx) << (sign_imm_64&0x3F);
			tmp_int64_t = (tmp_int64_t<<32)>>32;
			set_register(cpu, rd_idx, tmp_int64_t);
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("slliw\tx%d,x%d,%ld\n",rd_idx,rs1_idx,sign_imm_64&0x3F);
#endif
			break;
		case 5://srliw,sraiw
			if( !(cpu->inst.i_type.imm11_0>>6) ){//srliw
				tmp_int64_t = get_register(cpu,rs1_idx) >> (sign_imm_64&0x3F);
				tmp_int64_t = (tmp_int64_t<<32)>>32;
				set_register(cpu, rd_idx, tmp_int64_t);
#ifdef __CPU_EXEC_INST_DEBUG__
				printf("srliw\tx%d,x%d,%ld\n",rd_idx,rs1_idx,sign_imm_64&0x3F);
#endif
			}else{//sraiw
				tmp_int32_t = get_register(cpu,rs1_idx);
				tmp_int32_t = tmp_int32_t>> (sign_imm_64&0x3F);
				tmp_int64_t = tmp_int32_t;
				set_register(cpu, rd_idx, tmp_int64_t);
#ifdef __CPU_EXEC_INST_DEBUG__
				printf("sraiw\tx%d,x%d,%ld\n",rd_idx,rs1_idx,sign_imm_64&0x3F);
#endif
			}
			break;
		default:
			printf("%s: unknow instruction(opcode:0x%x pc:0x%lx func3:0x%x)\n",__func__,cpu->inst.r_type.opcode,cpu->pc-4,cpu->inst.r_type.funct3);
			dump_registers(cpu);
			exit(-1);
			break;
		}
		break;
	case 0x13:
		unsign_imm_64 = (uint64_t)cpu->inst.i_type.imm11_0;
		sign_imm_64 = (int64_t)cpu->inst.i_type.imm11_0;
		if(sign_imm_64 & 0x800){
			sign_imm_64 |= (~0x7ff);
		}
		rd_idx = cpu->inst.i_type.rd;
		rs1_idx = cpu->inst.i_type.rs1;
		switch(cpu->inst.i_type.funct3){
		case 0://I type , addi
			set_register(cpu, rd_idx, get_register(cpu,rs1_idx) + sign_imm_64);
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("addi\tx%d,x%d,%ld\n",rd_idx,rs1_idx,sign_imm_64);
#endif
			break;
		case 2://I type , slti
			set_register(cpu, rd_idx, (int64_t)get_register(cpu,rs1_idx) < sign_imm_64);
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("slti\tx%d,x%d,%ld\n",rd_idx,rs1_idx,sign_imm_64);
#endif
			break;
		case 3://I type , sltiu
			set_register(cpu, rd_idx, get_register(cpu,rs1_idx) < unsign_imm_64);
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("sltiu\tx%d,x%d,%lu\n",rd_idx,rs1_idx,unsign_imm_64);
#endif
			break;
		case 4://I type , xori
			set_register(cpu, rd_idx, get_register(cpu,rs1_idx) ^ sign_imm_64);
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("xori\tx%d,x%d,%ld\n",rd_idx,rs1_idx,sign_imm_64);
#endif
			break;
		case 6://I type , ori
			set_register(cpu, rd_idx, get_register(cpu,rs1_idx) | sign_imm_64);
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("ori\tx%d,x%d,%ld\n",rd_idx,rs1_idx,sign_imm_64);
#endif
			break;
		case 7://I type , andi
			set_register(cpu, rd_idx, get_register(cpu,rs1_idx) & sign_imm_64);
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("andi\tx%d,x%d,%ld\n",rd_idx,rs1_idx,sign_imm_64);
#endif
			break;
		case 1://I type , slli
			set_register(cpu, rd_idx, get_register(cpu,rs1_idx) << (sign_imm_64&0x3F));
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("slli\tx%d,x%d,%ld\n",rd_idx,rs1_idx,sign_imm_64&0x3F);
#endif
			break;
		case 5://srli,srai
			if( !(cpu->inst.i_type.imm11_0>>6) ){//srli
				set_register(cpu, rd_idx, get_register(cpu,rs1_idx) >> (sign_imm_64&0x3F));
#ifdef __CPU_EXEC_INST_DEBUG__
				printf("srli\tx%d,x%d,%ld\n",rd_idx,rs1_idx,sign_imm_64&0x3F);
#endif
			}else{//srai
				set_register(cpu, rd_idx, (int64_t)get_register(cpu,rs1_idx) >> (sign_imm_64&0x3F));
#ifdef __CPU_EXEC_INST_DEBUG__
				printf("srai\tx%d,x%d,%ld\n",rd_idx,rs1_idx,sign_imm_64&0x3F);
#endif
			}
			break;
		default:
			printf("%s: unknow instruction(opcode:0x%x pc:0x%lx func3:0x%x)\n",__func__,cpu->inst.i_type.opcode,cpu->pc-4,cpu->inst.i_type.funct3);
			dump_registers(cpu);
			exit(-1);
			break;
		}
		break;
	case 0x33:
		rd_idx = cpu->inst.r_type.rd;
		rs1_idx = cpu->inst.r_type.rs1;
		rs2_idx = cpu->inst.r_type.rs2;
		switch(cpu->inst.r_type.funct3){
		case 0://R type , add,sub
			if(!(cpu->inst.r_type.funct7)){//add
				set_register(cpu,rd_idx,get_register(cpu,rs1_idx) + get_register(cpu,rs2_idx));
#ifdef __CPU_EXEC_INST_DEBUG__
				printf("add\tx%d,x%d,x%d\n",rd_idx,rs1_idx,rs2_idx);
#endif
			} else {
				set_register(cpu,rd_idx,get_register(cpu,rs1_idx) - get_register(cpu,rs2_idx));
#ifdef __CPU_EXEC_INST_DEBUG__
				printf("sub\tx%d,x%d,x%d\n",rd_idx,rs1_idx,rs2_idx);
#endif
			}
			break;
		case 1://R type , sll
			set_register(cpu,rd_idx,(get_register(cpu,rs1_idx) << (get_register(cpu,rs2_idx)&0x3F)) );
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("sll\tx%d,x%d,x%d\n",rd_idx,rs1_idx,rs2_idx);
#endif
			break;
		case 2://R type , slt
			set_register(cpu,rd_idx,(int64_t)get_register(cpu,rs1_idx) < (int64_t)get_register(cpu,rs2_idx) );
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("slt\tx%d,x%d,x%d\n",rd_idx,rs1_idx,rs2_idx);
#endif
			break;
		case 3://R type , sltu
			set_register(cpu,rd_idx,get_register(cpu,rs1_idx) < get_register(cpu,rs2_idx) );
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("sltu\tx%d,x%d,x%d\n",rd_idx,rs1_idx,rs2_idx);
#endif
			break;
		case 4://R type , xor
			set_register(cpu,rd_idx,get_register(cpu,rs1_idx) ^ get_register(cpu,rs2_idx) );
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("xor\tx%d,x%d,x%d\n",rd_idx,rs1_idx,rs2_idx);
#endif
			break;
		case 5://R type , srl,sra
			if(!cpu->inst.r_type.funct7){//srl
				set_register(cpu,rd_idx,(get_register(cpu,rs1_idx) >> (get_register(cpu,rs2_idx)&0x3F)) );
#ifdef __CPU_EXEC_INST_DEBUG__
				printf("srl\tx%d,x%d,x%d\n",rd_idx,rs1_idx,rs2_idx);
#endif
			} else{//sra
				set_register(cpu,rd_idx,((int64_t)get_register(cpu,rs1_idx) >> (get_register(cpu,rs2_idx)&0x3F)) );
#ifdef __CPU_EXEC_INST_DEBUG__
				printf("sra\tx%d,x%d,x%d\n",rd_idx,rs1_idx,rs2_idx);
#endif
			}
			break;
		case 6://R type , or
			set_register(cpu,rd_idx,get_register(cpu,rs1_idx) | get_register(cpu,rs2_idx) );
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("or\tx%d,x%d,x%d\n",rd_idx,rs1_idx,rs2_idx);
#endif
			break;
		case 7://R type , and
			set_register(cpu,rd_idx,get_register(cpu,rs1_idx) & get_register(cpu,rs2_idx) );
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("and\tx%d,x%d,x%d\n",rd_idx,rs1_idx,rs2_idx);
#endif
			break;
		default:
			printf("%s: unknow instruction(opcode:0x%x pc:0x%lx func3:0x%x)\n",__func__,cpu->inst.r_type.opcode,cpu->pc-4,cpu->inst.r_type.funct3);
			dump_registers(cpu);
			exit(-1);
			break;
		}
		break;
	case 0x3B://R type
		rd_idx = cpu->inst.r_type.rd;
		rs1_idx = cpu->inst.r_type.rs1;
		rs2_idx = cpu->inst.r_type.rs2;
		switch(cpu->inst.r_type.funct3){
		case 0://addw,subw
			if(!(cpu->inst.r_type.funct7)){//addw
				tmp_int32_t = get_register(cpu,rs1_idx) + get_register(cpu,rs2_idx);
				tmp_int64_t = tmp_int32_t;
				set_register(cpu,rd_idx,tmp_int64_t);
#ifdef __CPU_EXEC_INST_DEBUG__
				printf("addw\tx%d,x%d,x%d\n",rd_idx,rs1_idx,rs2_idx);
#endif
			} else {//subw
				tmp_int32_t = get_register(cpu,rs1_idx) - get_register(cpu,rs2_idx);
				tmp_int64_t = tmp_int32_t;
				set_register(cpu,rd_idx,tmp_int64_t);
#ifdef __CPU_EXEC_INST_DEBUG__
				printf("subw\tx%d,x%d,x%d\n",rd_idx,rs1_idx,rs2_idx);
#endif
			}
			break;
		case 1://sllw
			tmp_int32_t = (get_register(cpu,rs1_idx) << (get_register(cpu,rs2_idx)&0x3F));
			tmp_int64_t = tmp_int32_t;
			set_register(cpu,rd_idx,tmp_int64_t );
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("sllw\tx%d,x%d,x%d\n",rd_idx,rs1_idx,rs2_idx);
#endif
			break;
		case 5://srlw,sraw
			if(!cpu->inst.r_type.funct7){//srlw
				tmp_int32_t = (get_register(cpu,rs1_idx) >> (get_register(cpu,rs2_idx)&0x3F));
				tmp_int64_t = tmp_int32_t;
				set_register(cpu,rd_idx,tmp_int64_t);
#ifdef __CPU_EXEC_INST_DEBUG__
				printf("srlw\tx%d,x%d,x%d\n",rd_idx,rs1_idx,rs2_idx);
#endif
			} else{//sraw
				tmp_int32_t = get_register(cpu,rs1_idx);
				tmp_int32_t = tmp_int32_t >> (get_register(cpu,rs2_idx)&0x3F);
				tmp_int64_t = tmp_int32_t;
				set_register(cpu,rd_idx,tmp_int64_t);
#ifdef __CPU_EXEC_INST_DEBUG__
				printf("sraw\tx%d,x%d,x%d\n",rd_idx,rs1_idx,rs2_idx);
#endif
			}
			break;
		default:
			printf("%s: unknow instruction(opcode:0x%x pc:0x%lx func3:0x%x)\n",__func__,cpu->inst.r_type.opcode,cpu->pc-4,cpu->inst.r_type.funct3);
			dump_registers(cpu);
			exit(-1);
		}
		break;
	case 0x37://U lui
		rd_idx = cpu->inst.u_type.rd;
		sign_imm_64 = cpu->inst.u_type.imm31_12;
		if(sign_imm_64 & 0x80000){
			sign_imm_64 |= (~0x7FFFF);
		}
		set_register(cpu,rd_idx,sign_imm_64<<12);
#ifdef __CPU_EXEC_INST_DEBUG__
		printf("lui\tx%d,%ld\n",rd_idx,sign_imm_64);
#endif
		break;
	case 0x17://U auipc
		rd_idx = cpu->inst.u_type.rd;
		sign_imm_64 = cpu->inst.u_type.imm31_12;
		if(sign_imm_64 & 0x80000){
			sign_imm_64 |= (~0x7FFFF);
		}
		set_register(cpu,rd_idx,(sign_imm_64<<12) + cpu->pc - 4);
#ifdef __CPU_EXEC_INST_DEBUG__
		printf("auipc\tx%d,%ld\n",rd_idx,sign_imm_64);
#endif
		break;
	case 0x6F://J jal
		rd_idx = cpu->inst.j_type.rd;
		sign_imm_64 = (cpu->inst.j_type.imm10_1<<1) | (cpu->inst.j_type.imm11<<11)
				| 	(cpu->inst.j_type.imm19_12<<12) | (cpu->inst.j_type.imm20<<20);
		if(cpu->inst.j_type.imm20){
			sign_imm_64 |= (~0xFFFFF);
		}

		set_register(cpu,rd_idx,cpu->pc);
		cpu->pc = (cpu->pc-4) + (sign_imm_64);
#ifdef __CPU_EXEC_INST_DEBUG__
		printf("jal\tx%d,%ld\n",rd_idx,sign_imm_64);
#endif
		break;
	case 0x67://I jalr
		rd_idx = cpu->inst.i_type.rd;
		rs1_idx = cpu->inst.i_type.rs1;
		orig_pc_plus4 = cpu->pc;
		sign_imm_64 = (int64_t)cpu->inst.i_type.imm11_0;
		if(sign_imm_64 & 0x800){
			sign_imm_64 |= (~0x7ff);
		}
		cpu->pc = ( (get_register(cpu, rs1_idx) + sign_imm_64) & (~(uint64_t)(1)) );
		set_register(cpu, rd_idx, orig_pc_plus4);
#ifdef __CPU_EXEC_INST_DEBUG__
		printf("jalr\tx%d,%ld(x%d)\n",rd_idx,sign_imm_64,rs1_idx);
#endif
		break;
	case 0x63://B type
		rs1_idx = cpu->inst.b_type.rs1;
		rs2_idx = cpu->inst.b_type.rs2;
		sign_imm_64 = (cpu->inst.b_type.imm4_1<<1) |
					  (cpu->inst.b_type.imm10_5<<5) |
					  (cpu->inst.b_type.imm_11<<11) |
					  (cpu->inst.b_type.imm12<<12);
		if(cpu->inst.b_type.imm12){
			sign_imm_64 |= (~0xFFF);
		}
		switch(cpu->inst.b_type.funct3){
		case 0://beq
			if(get_register(cpu, rs1_idx) == get_register(cpu, rs2_idx)){
				cpu->pc = cpu->pc-4 + sign_imm_64;
			}
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("beq\tx%d,x%d,%ld\n",rs1_idx,rs2_idx,sign_imm_64);
#endif
			break;
		case 1://bne
			if(get_register(cpu, rs1_idx) != get_register(cpu, rs2_idx)){
				cpu->pc = cpu->pc-4 + sign_imm_64;
			}
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("bne\tx%d,x%d,%ld\n",rs1_idx,rs2_idx,sign_imm_64);
#endif
			break;
		case 4://blt
			if((int64_t)get_register(cpu, rs1_idx) < (int64_t)get_register(cpu, rs2_idx)){
				cpu->pc = cpu->pc-4 + sign_imm_64;
			}
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("blt\tx%d,x%d,%ld\n",rs1_idx,rs2_idx,sign_imm_64);
#endif
			break;
		case 5://bge
			if((int64_t)get_register(cpu, rs1_idx) >= (int64_t)get_register(cpu, rs2_idx)){
				cpu->pc = cpu->pc-4 + sign_imm_64;
			}
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("bge\tx%d,x%d,%ld\n",rs1_idx,rs2_idx,sign_imm_64);
#endif
			break;
		case 6://bltu
			if(get_register(cpu, rs1_idx) < get_register(cpu, rs2_idx)){
				cpu->pc = cpu->pc-4 + sign_imm_64;
			}
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("bltu\tx%d,x%d,%ld\n",rs1_idx,rs2_idx,sign_imm_64);
#endif
			break;
		case 7://bgeu
			if(get_register(cpu, rs1_idx) >= get_register(cpu, rs2_idx)){
				cpu->pc = cpu->pc-4 + sign_imm_64;
			}
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("bgeu\tx%d,x%d,%ld\n",rs1_idx,rs2_idx,sign_imm_64);
#endif
			break;
		default:
			printf("%s: unknow instruction(opcode:0x%x pc:0x%lx func3:0x%x)\n",__func__,cpu->inst.b_type.opcode,cpu->pc-4,cpu->inst.b_type.funct3);
			dump_registers(cpu);
			exit(-1);
			break;
		}
		break;
	case 0x3://I load
		rs1_idx = cpu->inst.i_type.rs1;
		rd_idx = cpu->inst.i_type.rd;
		sign_imm_64 = cpu->inst.i_type.imm11_0;
		if(sign_imm_64 & 0x800){
			sign_imm_64 |=(~0x7FF);
		}
		switch(cpu->inst.i_type.funct3){
		case 0://lb
			mem_sign_data = get_byte_from_cache(&cpu->dcache, get_register(cpu, rs1_idx) + sign_imm_64);
			mem_sign_data = (mem_sign_data<<56)>>56;
			set_register(cpu, rd_idx, mem_sign_data);
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("lb\tx%d,%ld(x%d)\n",rd_idx,sign_imm_64,rs1_idx);
#endif
			break;
		case 1://lh
			mem_sign_data = get_word_from_cache(&cpu->dcache, get_register(cpu, rs1_idx) + sign_imm_64);
			mem_sign_data = (mem_sign_data<<48)>>48;
			set_register(cpu, rd_idx, mem_sign_data);
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("lh\tx%d,%ld(x%d)\n",rd_idx,sign_imm_64,rs1_idx);
#endif
			break;
		case 2://lw
			mem_sign_data = get_dword_from_cache(&cpu->dcache, get_register(cpu, rs1_idx) + sign_imm_64);
			mem_sign_data = (mem_sign_data<<32)>>32;
			set_register(cpu, rd_idx, mem_sign_data);
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("lw\tx%d,%ld(x%d)\n",rd_idx,sign_imm_64,rs1_idx);
#endif
			break;
		case 4://lbu
			mem_sign_data = get_byte_from_cache(&cpu->dcache, get_register(cpu, rs1_idx) + sign_imm_64);
			set_register(cpu, rd_idx, mem_sign_data);
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("lbu\tx%d,%ld(x%d)\n",rd_idx,sign_imm_64,rs1_idx);
#endif
			break;
		case 5://lhu
			mem_sign_data = get_word_from_cache(&cpu->dcache, get_register(cpu, rs1_idx) + sign_imm_64);
			set_register(cpu, rd_idx, mem_sign_data);
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("lhu\tx%d,%ld(x%d)\n",rd_idx,sign_imm_64,rs1_idx);
#endif
			break;
		case 6://lwu
			mem_sign_data = get_dword_from_cache(&cpu->dcache, get_register(cpu, rs1_idx) + sign_imm_64);
			set_register(cpu, rd_idx, mem_sign_data);
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("lwu\tx%d,%ld(x%d)\n",rd_idx,sign_imm_64,rs1_idx);
#endif
			break;
		case 3://ld
			mem_sign_data = get_qword_from_cache(&cpu->dcache, get_register(cpu, rs1_idx) + sign_imm_64);
			set_register(cpu, rd_idx, mem_sign_data);
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("ld\tx%d,%ld(x%d)\n",rd_idx,sign_imm_64,rs1_idx);
#endif
			break;
		default:
			printf("%s: unknow instruction(opcode:0x%x pc:0x%lx func3:0x%x)\n",__func__,cpu->inst.i_type.opcode,cpu->pc-4,cpu->inst.i_type.funct3);
			dump_registers(cpu);
			exit(-1);
			break;
		}
		break;
	case 0x23://S store
		rs2_idx = cpu->inst.s_type.rs2;
		rs1_idx = cpu->inst.s_type.rs1;
		sign_imm_64 = (cpu->inst.s_type.imm4_0) | (cpu->inst.s_type.imm11_5<<5);
		if(sign_imm_64 & 0x800){
			sign_imm_64 |=(~0x7FF);
		}
		switch(cpu->inst.s_type.funct3){
		case 0://sb
			put_byte_to_cache(&cpu->dcache, get_register(cpu, rs1_idx) + sign_imm_64, get_register(cpu, rs2_idx));
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("sb\tx%d,%ld(x%d)\n",rs2_idx,sign_imm_64,rs1_idx);
#endif
			break;
		case 1://sh
			put_word_to_cache(&cpu->dcache, get_register(cpu, rs1_idx) + sign_imm_64, get_register(cpu, rs2_idx));
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("sh\tx%d,%ld(x%d)\n",rs2_idx,sign_imm_64,rs1_idx);
#endif
			break;
		case 2://sw
			put_dword_to_cache(&cpu->dcache, get_register(cpu, rs1_idx) + sign_imm_64, get_register(cpu, rs2_idx));
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("sw\tx%d,%ld(x%d)\n",rs2_idx,sign_imm_64,rs1_idx);
#endif
			break;
		case 3://sd
			put_qword_to_cache(&cpu->dcache, get_register(cpu, rs1_idx) + sign_imm_64, get_register(cpu, rs2_idx));
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("sd\tx%d,%ld(x%d)\n",rs2_idx,sign_imm_64,rs1_idx);
#endif
			break;
		default:
			printf("%s: unknow instruction(opcode:0x%x pc:0x%lx func3:0x%x)\n",__func__,cpu->inst.s_type.opcode,cpu->pc-4,cpu->inst.s_type.funct3);
			dump_registers(cpu);
			exit(-1);
			break;
		}
		break;
	case 0xF://fence,fence.i
		switch(cpu->inst.i_type.funct3){
		case 0://fence
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("fence\n");
#endif
			break;
		case 1://fence.i
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("fence.i\n");
#endif
			break;
		default:
			printf("%s: unknow instruction(opcode:0x%x pc:0x%lx func3:0x%x)\n",__func__,cpu->inst.i_type.opcode,cpu->pc-4,cpu->inst.i_type.funct3);
			dump_registers(cpu);
			exit(-1);
			break;
		}
		break;
	case 0x73:
		switch(cpu->inst.i_type.funct3){
		case 0://ecall,ebreak
			break;
		case 1://csrrw
			csr = cpu->inst.i_type.imm11_0;
			unsign_imm_64 = cpu->csrs[csr];
			cpu->csrs[csr] = get_register(cpu, cpu->inst.i_type.rs1);
			set_register(cpu, cpu->inst.i_type.rd, unsign_imm_64);
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("csrrw\tx%d,0x%lx,x%d\n",cpu->inst.i_type.rd,csr,cpu->inst.i_type.rs1);
#endif
			break;
		case 2://csrrs
			csr = cpu->inst.i_type.imm11_0;
			unsign_imm_64 = cpu->csrs[csr];
			cpu->csrs[csr] =unsign_imm_64 | get_register(cpu, cpu->inst.i_type.rs1);
			set_register(cpu, cpu->inst.i_type.rd, unsign_imm_64);
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("csrrs\tx%d,0x%lx,x%d\n",cpu->inst.i_type.rd,csr,cpu->inst.i_type.rs1);
#endif
			break;
		case 3://csrrc
			csr = cpu->inst.i_type.imm11_0;
			unsign_imm_64 = cpu->csrs[csr];
			cpu->csrs[csr] =unsign_imm_64 & ~get_register(cpu, cpu->inst.i_type.rs1);
			set_register(cpu, cpu->inst.i_type.rd, unsign_imm_64);
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("csrrc\tx%d,0x%lx,x%d\n",cpu->inst.i_type.rd,csr,cpu->inst.i_type.rs1);
#endif
			break;
		case 5://csrrwi
			csr = cpu->inst.i_type.imm11_0;
			unsign_imm_64 = cpu->inst.i_type.rs1;
			set_register(cpu, cpu->inst.i_type.rd, cpu->csrs[csr]);
			cpu->csrs[csr] = unsign_imm_64;
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("csrrwi\tx%d,0x%lx,%d\n",cpu->inst.i_type.rd,csr,cpu->inst.i_type.rs1);
#endif
			break;
		case 6://csrrsi
			csr = cpu->inst.i_type.imm11_0;
			unsign_imm_64 = cpu->csrs[csr];
			cpu->csrs[csr] =unsign_imm_64 | cpu->inst.i_type.rs1;
			set_register(cpu, cpu->inst.i_type.rd, unsign_imm_64);
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("csrrsi\tx%d,0x%lx,%d\n",cpu->inst.i_type.rd,csr,cpu->inst.i_type.rs1);
#endif
			break;
		case 7://csrrci
			csr = cpu->inst.i_type.imm11_0;
			unsign_imm_64 = cpu->csrs[csr];
			cpu->csrs[csr] =unsign_imm_64 & ~cpu->inst.i_type.rs1;
			set_register(cpu, cpu->inst.i_type.rd, unsign_imm_64);
#ifdef __CPU_EXEC_INST_DEBUG__
			printf("csrrci\tx%d,0x%lx,%d\n",cpu->inst.i_type.rd,csr,cpu->inst.i_type.rs1);
#endif
			break;
		default:
			printf("%s: unknow instruction(opcode:0x%x pc:0x%lx func3:0x%x)\n",__func__,cpu->inst.i_type.opcode,cpu->pc-4,cpu->inst.i_type.funct3);
			dump_registers(cpu);
			exit(-1);
			break;
		}
		break;
	default:
		printf("%s: unknow instruction(opcode:0x%x pc:0x%lx)\n",__func__,cpu->inst.r_type.opcode,cpu->pc-4);
		dump_registers(cpu);
		exit(-1);
		break;
	}
}

void cpu_run(struct cpu *cpu)
{
	while(1){
		cpu_fetch(cpu);
		cpu_exec(cpu);
		if(cpu->pc == 0){
			printf("All instructions have been executed\n");
//			dump_registers(cpu);
			exit(0);
		}
	}
}


