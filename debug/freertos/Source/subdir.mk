################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../freertos/Source/croutine.c \
../freertos/Source/event_groups.c \
../freertos/Source/list.c \
../freertos/Source/queue.c \
../freertos/Source/tasks.c \
../freertos/Source/timers.c 

OBJS += \
./freertos/Source/croutine.o \
./freertos/Source/event_groups.o \
./freertos/Source/list.o \
./freertos/Source/queue.o \
./freertos/Source/tasks.o \
./freertos/Source/timers.o 

C_DEPS += \
./freertos/Source/croutine.d \
./freertos/Source/event_groups.d \
./freertos/Source/list.d \
./freertos/Source/queue.d \
./freertos/Source/tasks.d \
./freertos/Source/timers.d 


# Each subdirectory must supply rules for building sources it contributes
freertos/Source/%.o: ../freertos/Source/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -ffreestanding -fno-builtin -Wall  -g -DDEBUG -DLWIP_IGMP=1 -DCPU_MK64FN1M0VMD12 -DUSE_RTOS=1 -DPRINTF_ADVANCED_ENABLE=1 -DFSL_RTOS_FREE_RTOS -DFRDM_K64F -DFREEDOM -I../CMSIS/Include -I"C:\Users\Greg\workspace.kds\E131 No Class\sources" -I"C:\Users\Greg\workspace.kds\E131 No Class" -I../devices/gcc -I../devices -I../drivers -I../freertos/Source/include -I../freertos/Source/portable/GCC/ARM_CM4F -I../freertos/Source -I../lwip/contrib/apps -I../lwip/port/arch -I../lwip/port -I../lwip/src/include -I../sources -I../utilities -std=gnu99 -mapcs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


