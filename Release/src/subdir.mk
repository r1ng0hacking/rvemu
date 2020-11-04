################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/cache.c \
../src/cpu.c \
../src/main.c \
../src/ram.c 

OBJS += \
./src/cache.o \
./src/cpu.o \
./src/main.o \
./src/ram.o 

C_DEPS += \
./src/cache.d \
./src/cpu.d \
./src/main.d \
./src/ram.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/home/john/eclipse-workspace/rvemu/include -I"/home/john/eclipse-workspace/rvemu/include" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


