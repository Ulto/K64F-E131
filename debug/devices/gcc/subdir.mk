################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
../devices/gcc/startup_MK64F12.S 

OBJS += \
./devices/gcc/startup_MK64F12.o 

S_UPPER_DEPS += \
./devices/gcc/startup_MK64F12.d 


# Each subdirectory must supply rules for building sources it contributes
devices/gcc/%.o: ../devices/gcc/%.S
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM GNU Assembler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -ffreestanding -fno-builtin -Wall  -g -x assembler-with-cpp -DDEBUG -D__STARTUP_CLEAR_BSS -I../.. -mapcs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


