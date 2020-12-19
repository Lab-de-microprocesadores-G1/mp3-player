################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include startup/subdir.mk
-include source/subdir.mk
-include drivers/MCAL/gpio/subdir.mk
-include drivers/MCAL/equaliser/fsl_uart/subdir.mk
-include drivers/MCAL/equaliser/fsl_common/subdir.mk
-include drivers/MCAL/equaliser/fsl_clock/subdir.mk
-include drivers/MCAL/equaliser/subdir.mk
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
all: dsp_testbench.axf

# Tool invocations
dsp_testbench.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -L"C:\Users\facun\mcuxpresso\01\SDKPackages\SDK_2.8.2_FRDM-K64F\CMSIS\DSP\Lib\GCC" -Xlinker -Map="dsp_testbench.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -T "dsp_testbench_Debug.ld" -o "dsp_testbench.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) dsp_testbench.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "dsp_testbench.axf"; # arm-none-eabi-objcopy -v -O binary "dsp_testbench.axf" "dsp_testbench.bin" ; # checksum -p MK64FN1M0xxx12 -d "dsp_testbench.bin";
	-@echo ' '

.PHONY: all clean dependents post-build

-include ../makefile.targets
