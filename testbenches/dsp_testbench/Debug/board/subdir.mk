################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../board/board.c \
../board/clock_config.c \
../board/pin_mux.c 

OBJS += \
./board/board.o \
./board/clock_config.o \
./board/pin_mux.o 

C_DEPS += \
./board/board.d \
./board/clock_config.d \
./board/pin_mux.d 


# Each subdirectory must supply rules for building sources it contributes
board/%.o: ../board/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -DCPU_MK64FN1M0VLL12 -DARM_MATH_CM7PLUS -DCPU_MK64FN1M0VLL12_cm4 -DPRINTF_FLOAT_ENABLE=0 -DSCANF_FLOAT_ENABLE=0 -DPRINTF_ADVANCED_ENABLE=0 -DSCANF_ADVANCED_ENABLE=0 -DFRDM_K64F -DFREEDOM -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=0 -DCR_INTEGER_PRINTF -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__REDLIB__ -I"C:\Users\facun\OneDrive\Desktop\ITBA\7C_Laboratorio_de_Microprocesadores\tpf-mp3-player\testbenches\dsp_testbench\board" -I"C:\Users\facun\OneDrive\Desktop\ITBA\7C_Laboratorio_de_Microprocesadores\tpf-mp3-player\testbenches\dsp_testbench\source" -I"C:\Users\facun\OneDrive\Desktop\ITBA\7C_Laboratorio_de_Microprocesadores\tpf-mp3-player\testbenches\dsp_testbench" -I"C:\Users\facun\OneDrive\Desktop\ITBA\7C_Laboratorio_de_Microprocesadores\tpf-mp3-player\testbenches\dsp_testbench\drivers" -I"C:\Users\facun\OneDrive\Desktop\ITBA\7C_Laboratorio_de_Microprocesadores\tpf-mp3-player\testbenches\dsp_testbench\device" -I"C:\Users\facun\OneDrive\Desktop\ITBA\7C_Laboratorio_de_Microprocesadores\tpf-mp3-player\testbenches\dsp_testbench\CMSIS" -I"C:\Users\facun\OneDrive\Desktop\ITBA\7C_Laboratorio_de_Microprocesadores\tpf-mp3-player\testbenches\dsp_testbench\utilities" -I"C:\Users\facun\OneDrive\Desktop\ITBA\7C_Laboratorio_de_Microprocesadores\tpf-mp3-player\testbenches\dsp_testbench\component\serial_manager" -I"C:\Users\facun\OneDrive\Desktop\ITBA\7C_Laboratorio_de_Microprocesadores\tpf-mp3-player\testbenches\dsp_testbench\component\lists" -I"C:\Users\facun\OneDrive\Desktop\ITBA\7C_Laboratorio_de_Microprocesadores\tpf-mp3-player\testbenches\dsp_testbench\component\uart" -O0 -fno-common -g3 -Wall -c  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


