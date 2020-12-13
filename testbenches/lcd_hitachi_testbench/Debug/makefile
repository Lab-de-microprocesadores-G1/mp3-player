################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include startup/subdir.mk
-include source/subdir.mk
-include lib/queue/subdir.mk
-include drivers/MCAL/systick/subdir.mk
-include drivers/MCAL/spi/subdir.mk
-include drivers/MCAL/gpio/subdir.mk
-include drivers/HAL/timer/subdir.mk
-include drivers/HAL/button/subdir.mk
-include drivers/HAL/HD44780_LCD/subdir.mk
-include drivers/HAL/HD44780/subdir.mk
-include board/subdir.mk
-include CMSIS/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: lcd_hitachi_testbench.axf

# Tool invocations
lcd_hitachi_testbench.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -Xlinker -Map="lcd_hitachi_testbench.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -T lcd_hitachi_testbench_Debug.ld -o "lcd_hitachi_testbench.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) lcd_hitachi_testbench.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "lcd_hitachi_testbench.axf"; # arm-none-eabi-objcopy -v -O binary "lcd_hitachi_testbench.axf" "lcd_hitachi_testbench.bin" ; # checksum -p MK64FN1M0xxx12 -d "lcd_hitachi_testbench.bin";
	-@echo ' '

.PHONY: all clean dependents post-build

-include ../makefile.targets
