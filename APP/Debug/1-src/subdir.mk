################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../1-src/ad7606.c \
../1-src/can_config.c \
../1-src/can_data_deal.c \
../1-src/crc_table.c \
../1-src/data_test.c \
../1-src/dictionary.c \
../1-src/ftp_client.c \
../1-src/hdd_save.c \
../1-src/iniparser.c \
../1-src/jxds_pw.c \
../1-src/led.c \
../1-src/lh_math.c \
../1-src/multi_timer.c \
../1-src/pthread_policy.c \
../1-src/ptu_app.c \
../1-src/pw_diagnos.c \
../1-src/ringbuffer.c \
../1-src/sec_app.c \
../1-src/self_queue.c \
../1-src/self_test.c \
../1-src/speed.c \
../1-src/sw_diagnos.c \
../1-src/udp_client.c \
../1-src/udp_server.c \
../1-src/update.c \
../1-src/user_data.c \
../1-src/watchdog.c \
../1-src/wtd_app.c 

OBJS += \
./1-src/ad7606.o \
./1-src/can_config.o \
./1-src/can_data_deal.o \
./1-src/crc_table.o \
./1-src/data_test.o \
./1-src/dictionary.o \
./1-src/ftp_client.o \
./1-src/hdd_save.o \
./1-src/iniparser.o \
./1-src/jxds_pw.o \
./1-src/led.o \
./1-src/lh_math.o \
./1-src/multi_timer.o \
./1-src/pthread_policy.o \
./1-src/ptu_app.o \
./1-src/pw_diagnos.o \
./1-src/ringbuffer.o \
./1-src/sec_app.o \
./1-src/self_queue.o \
./1-src/self_test.o \
./1-src/speed.o \
./1-src/sw_diagnos.o \
./1-src/udp_client.o \
./1-src/udp_server.o \
./1-src/update.o \
./1-src/user_data.o \
./1-src/watchdog.o \
./1-src/wtd_app.o 

C_DEPS += \
./1-src/ad7606.d \
./1-src/can_config.d \
./1-src/can_data_deal.d \
./1-src/crc_table.d \
./1-src/data_test.d \
./1-src/dictionary.d \
./1-src/ftp_client.d \
./1-src/hdd_save.d \
./1-src/iniparser.d \
./1-src/jxds_pw.d \
./1-src/led.d \
./1-src/lh_math.d \
./1-src/multi_timer.d \
./1-src/pthread_policy.d \
./1-src/ptu_app.d \
./1-src/pw_diagnos.d \
./1-src/ringbuffer.d \
./1-src/sec_app.d \
./1-src/self_queue.d \
./1-src/self_test.d \
./1-src/speed.d \
./1-src/sw_diagnos.d \
./1-src/udp_client.d \
./1-src/udp_server.d \
./1-src/update.d \
./1-src/user_data.d \
./1-src/watchdog.d \
./1-src/wtd_app.d 


# Each subdirectory must supply rules for building sources it contributes
1-src/%.o: ../1-src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gnueabihf-gcc -I"/mnt/hgfs/code/LH-TFDS-WZS1/code/PSW/APP/2-inc" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


