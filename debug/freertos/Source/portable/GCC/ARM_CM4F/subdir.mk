################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../freertos/Source/portable/GCC/ARM_CM4F/fsl_tickless_systick.c \
../freertos/Source/portable/GCC/ARM_CM4F/port.c 

OBJS += \
./freertos/Source/portable/GCC/ARM_CM4F/fsl_tickless_systick.o \
./freertos/Source/portable/GCC/ARM_CM4F/port.o 

C_DEPS += \
./freertos/Source/portable/GCC/ARM_CM4F/fsl_tickless_systick.d \
./freertos/Source/portable/GCC/ARM_CM4F/port.d 


# Each subdirectory must supply rules for building sources it contributes
freertos/Source/portable/GCC/ARM_CM4F/%.o: ../freertos/Source/portable/GCC/ARM_CM4F/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -ffreestanding -fno-builtin -Wall  -g -DDEBUG -DLWIP_IGMP=1 -DCPU_MK64FN1M0VMD12 -DUSE_RTOS=1 -DPRINTF_ADVANCED_ENABLE=1 -DFSL_RTOS_FREE_RTOS -DFRDM_K64F -DFREEDOM -I../CMSIS/Include -I"C:\Users\Greg\workspace.kds\E131 No Class\sources" -I"C:\Users\Greg\workspace.kds\E131 No Class" -I../devices/gcc -I../devices -I../drivers -I../freertos/Source/include -I../freertos/Source/portable/GCC/ARM_CM4F -I../freertos/Source -I../lwip/contrib/apps -I../lwip/port/arch -I../lwip/port -I../lwip/src/include -I../sources -I../utilities -std=gnu99 -mapcs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


