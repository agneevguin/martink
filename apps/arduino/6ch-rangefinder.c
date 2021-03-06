/*

Copyright (c) 2016 Martin Schröder <mkschreder.uk@gmail.com>
License: GPLv3

Quick and dirty way to read 6 rangefinders on arduino. (note: really dirty!)
*/ 

#include <arch/soc.h>
#include <kernel/mt.h>
#include <kernel/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <adc/adc.h>
#include <gpio/gpio.h>
#include <gpio/atmega_gpio.h>
#include <pwm/pwm.h>
#include <serial/serial.h>
#include <serial/atmega_uart.h>
#include <arch/avr/mega/time.h>
#include <kernel/time.h>

#define SENSOR_COUNT 6

// 15ms is enough for measuring up to 2.7m. Beyond that sensor readings are unreliable. 
#define SENSOR_READING_TIMEOUT 15UL

//#define SENSOR_MAX_READING (200 * 58)
#define SENSOR_READING_INVALID (-1)

static int16_t _readings[SENSOR_COUNT]; 
static int16_t _health[SENSOR_COUNT]; 
static timestamp_t _timestamps[SENSOR_COUNT]; 
static uint8_t _cur_sensor = 0; 
static uint8_t _reading_pending = 0; 
static struct gpio_adapter *gpio = 0; 

static void _pcint_handler(struct gpio_adapter *self, void *data){
	int up = gpio_read_pin(gpio, GPIO_PD2 + _cur_sensor); 
	if(!up){
		// falling edge is when we calc the time 
		int32_t reading = tsc_ticks_to_us(tsc_read() - _timestamps[_cur_sensor]); 	
		if(reading >= 0){ // if distance is less than 0 then it is invalid so we only update if we have what *seems* like a valid distance. 
			_readings[_cur_sensor] = reading;  
		}
		_reading_pending = 0; 
		// disable pcint here as a precaution
		gpio_disable_pcint(gpio, GPIO_PD2 + _cur_sensor); 
	} else {
		// rising edge is start of measure
		_timestamps[_cur_sensor] = tsc_read(); 	
	}
}

int main(void){
	gpio = atmega_gpio_get_adapter(); 	
	for(int c = 0; c < SENSOR_COUNT; c++){
		// configure trigger pin
		gpio_configure_pin(gpio, GPIO_PB0 + c, GP_OUTPUT); 
		gpio_write_pin(gpio, GPIO_PB0 + c, 0); 
		// configure echo pin
		gpio_configure_pin(gpio, GPIO_PD2 + c, GP_INPUT); 
		gpio_register_pcint(gpio, GPIO_PD2 + c, &_pcint_handler, NULL); 
	}
	printk("6THSENSE\n"); 
	
    for( ;; ){
		// invalidate previous reading
		_readings[_cur_sensor] = SENSOR_READING_INVALID; 
	
		// output trigger pulse
		gpio_write_pin(gpio, GPIO_PB0 + _cur_sensor, 0); 
		_delay_us(5); 
		gpio_write_pin(gpio, GPIO_PB0 + _cur_sensor, 1);  
		_delay_us(10); 
		gpio_write_pin(gpio, GPIO_PB0 + _cur_sensor, 0); 

		// configure echo pin as input 
		gpio_configure_pin(gpio, GPIO_PD2 + _cur_sensor, GP_INPUT); 
		gpio_write_pin(gpio, GPIO_PD2 + _cur_sensor, 1); 
		
		// set the reading flag
		_reading_pending = 1; 

		// enable echo interrupt 
		gpio_enable_pcint(gpio, GPIO_PD2 + _cur_sensor); 
		
		// wait until reading done or cancel after a sensor delay
		timestamp_t timeout = timestamp_from_now_us(SENSOR_READING_TIMEOUT * 1000UL); 
		while(_reading_pending && !timestamp_expired(timeout)){
			_delay_ms(1); 
		}

		// disable pin interrupt
		gpio_disable_pcint(gpio, GPIO_PD2 + _cur_sensor); 

		// configure echo pin to a reset state
		gpio_configure_pin(gpio, GPIO_PD2 + _cur_sensor, GP_OUTPUT); 
		gpio_write_pin(gpio, GPIO_PD2 + _cur_sensor, 0); 
	
		// go to next sensor
		_cur_sensor++; 
		if(_cur_sensor == SENSOR_COUNT) _cur_sensor = 0; 

		if(_cur_sensor == 0){
			printk("R %lu ", (unsigned long)(timestamp_ticks_to_us(tsc_read()) / 1000UL)); 
			for(int c = 0; c < SENSOR_COUNT; c++){
				if(c != 0) printk(" "); 
				printk("%d", _readings[c]); 
			}
			printk("\n"); 
		}
	}
}

