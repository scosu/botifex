################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../botifex.c \
../botifex_cmds.c \
../botifex_know.c \
../libirc.c \
../trie.c 

OBJS += \
./botifex.o \
./botifex_cmds.o \
./botifex_know.o \
./libirc.o \
./trie.o 

C_DEPS += \
./botifex.d \
./botifex_cmds.d \
./botifex_know.d \
./libirc.d \
./trie.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/gnet-2.0 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


