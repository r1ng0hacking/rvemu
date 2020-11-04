################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/bus.c \
../src/cache.c \
../src/cpu.c \
../src/device.c \
../src/display.c \
../src/main.c \
../src/ram.c 

OBJS += \
./src/bus.o \
./src/cache.o \
./src/cpu.o \
./src/device.o \
./src/display.o \
./src/main.o \
./src/ram.o 

C_DEPS += \
./src/bus.d \
./src/cache.d \
./src/cpu.d \
./src/device.d \
./src/display.d \
./src/main.d \
./src/ram.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/john/eclipse-workspace/rvemu/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


