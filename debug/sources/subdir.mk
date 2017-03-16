################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../sources/E131.c \
../sources/board.c \
../sources/clock_config.c \
../sources/fsl_phy.c \
../sources/main.c \
../sources/pin_mux.c 

OBJS += \
./sources/E131.o \
./sources/board.o \
./sources/clock_config.o \
./sources/fsl_phy.o \
./sources/main.o \
./sources/pin_mux.o 

C_DEPS += \
./sources/E131.d \
./sources/board.d \
./sources/clock_config.d \
./sources/fsl_phy.d \
./sources/main.d \
./sources/pin_mux.d 


# Each subdirectory must supply rules for building sources it contributes
sources/%.o: ../sources/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -ffreestanding -fno-builtin -Wall  -g -DDEBUG -DLWIP_IGMP=1 -DCPU_MK64FN1M0VMD12 -DUSE_RTOS=1 -DPRINTF_ADVANCED_ENABLE=1 -DFSL_RTOS_FREE_RTOS -DFRDM_K64F -DFREEDOM -I../CMSIS/Include -I"C:\Users\Greg\workspace.kds\E131 No Class\sources" -I"C:\Users\Greg\workspace.kds\E131 No Class" -I../devices/gcc -I../devices -I../drivers -I../freertos/Source/include -I../freertos/Source/portable/GCC/ARM_CM4F -I../freertos/Source -I../lwip/contrib/apps -I../lwip/port/arch -I../lwip/port -I../lwip/src/include -I../sources -I../utilities -std=gnu99 -mapcs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


