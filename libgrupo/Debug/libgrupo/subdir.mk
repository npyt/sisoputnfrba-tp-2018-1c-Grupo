################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../libgrupo/sockets.c 

OBJS += \
./libgrupo/sockets.o 

C_DEPS += \
./libgrupo/sockets.d 


# Each subdirectory must supply rules for building sources it contributes
libgrupo/%.o: ../libgrupo/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


