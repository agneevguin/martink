/*
	This file is part of martink project.

	martink firmware project is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	martink firmware is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with martink firmware.  If not, see <http://www.gnu.org/licenses/>.

	Author: Martin K. Schröder
	Email: info@fortmax.se
	Github: https://github.com/mkschreder
*/

/**
 \mainpage LibK firmware development library

 <b>Introduction</b>

 LibK is a driver support library providing portable device driver
 support for embedded applications.

 It is distributed under the GNU General Public License - see the
 accompanying LICENSE.txt file for more details.

 It is designed to even be able to compile on a desktop. The future
 may very well involve using libk to use the same device drivers for
 both arduino and a linux based board that uses linux to access i2c
 ports and other ports. LibK can basically be compiled as a native
 library. All that is necessary to run it on any board is to
 implement the arch interface for that board. For a native version
 this layer would use linux system calls to access i2c or SPI ports
 instead of accessing hardware registers directly - but for the
 drivers it does not matter.

 <b>Download source code</b>

 You can get libk source code from the project github page:

 https://github.com/mkschreder

 
*/
#include "kernel.h"

#include <setjmp.h>

LIST_HEAD(_running); 
LIST_HEAD(_idle); 

void libk_create_thread(struct libk_thread *self, char (*func)(struct pt *)){
	PT_INIT(&self->thread); 
	self->proc = func; 
	list_add_tail(&self->list, &_running); 
}

void libk_schedule(void){
	struct list_head *ptr, *n; 
	list_for_each_safe(ptr, n, &_running){
		struct libk_thread *thr = container_of(ptr, struct libk_thread, list); 
		thr->proc(&thr->thread); 
	}
}

LIBK_THREAD(_main_thread){
	static timestamp_t time; 
	
	PT_BEGIN(pt); 
	while(1){
		PT_WAIT_UNTIL(pt, timestamp_expired(time)); 
		printf("Main thread!\n"); 
		time = timestamp_from_now_us(1000000); 
	}
	PT_END(pt); 
}

extern int __cxa_guard_acquire(__guard *g);
extern void __cxa_guard_release (__guard *g); 
extern void __cxa_guard_abort (__guard *g); 
extern void __cxa_pure_virtual(void); 

int __cxa_guard_acquire(__guard *g) {return !*(char *)(g);}
void __cxa_guard_release (__guard *g) {*(char *)g = 1;}
void __cxa_guard_abort (__guard *g) {*(char*)g = 0;}
void __cxa_pure_virtual(void) {}
