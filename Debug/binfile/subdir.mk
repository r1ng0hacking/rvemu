################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../binfile/bin.c 

OBJS += \
./binfile/bin.o 

C_DEPS += \
./binfile/bin.d 


# Each subdirectory must supply rules for building sources it contributes
binfile/%.o: ../binfile/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/john/eclipse-workspace/rvemu/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


