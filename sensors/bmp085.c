/*
bmp085 lib 0x01

copyright (c) Davide Gironi, 2012

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <arch/soc.h>
//#include <avr/io.h>
//#include <util/delay.h>

#include "bmp085.h"

//variables
static int bmp085_regac1, bmp085_regac2, bmp085_regac3, bmp085_regb1, bmp085_regb2, bmp085_regmb, bmp085_regmc, bmp085_regmd;
static unsigned int bmp085_regac4, bmp085_regac5, bmp085_regac6;
static long bmp085_rawtemperature, bmp085_rawpressure;

//#include <i2c.h>

/*
 * i2c write
 */
void bmp085_writemem(struct bmp085 *self, uint8_t reg, uint8_t value) {
	//uart0_printf("BMP: writing reg %d: %d .. ", reg, value); 
	
	uint8_t buf[8] = {BMP085_ADDR, reg, value};
	p_begin(self->port);
	p_write(self->port, buf, 3);
	p_sync(self->port);
	p_end(self->port);
	/*
	twi0_start_transaction(TWI_OP_LIST(
		TWI_OP(BMP085_ADDR | I2C_WRITE, buf, 2)
	)); 
	twi0_wait(); */
	/*i2c_start_wait(BMP085_ADDR | I2C_WRITE);
	i2c_write(reg);
	i2c_write(value);
	i2c_stop();*/
}

/*
 * i2c read
 */
static inline void bmp085_readmem(struct bmp085 *self, uint8_t reg, uint8_t *buff, uint8_t bytes) {
	//bytes &= 0x07; 
	//uart0_printf("BMP: reading reg %d:  %d\n", reg, bytes); 
	uint8_t buf[8] = {BMP085_ADDR, reg, 0, 0, 0, 0, 0, 0};
	p_begin(self->port);
	p_write(self->port, buf, 2);
	p_sync(self->port);
	_delay_us(10); 
	p_read(self->port, buf, bytes + 1);
	p_sync(self->port);
	p_end(self->port);
	
	memcpy(buff, &buf[1], bytes); 
	/*twi0_start_transaction(TWI_OP_LIST(
		TWI_OP(BMP085_ADDR | I2C_WRITE, buf, 1),
		TWI_OP(BMP085_ADDR | I2C_READ, buff, bytes)
	)); 
	twi0_wait(); */
	/*uint8_t i =0;
	i2c_start_wait(BMP085_ADDR | I2C_WRITE);
	i2c_write(reg);
	i2c_rep_start(BMP085_ADDR | I2C_READ);
	for(i=0; i<bytes; i++) {
		if(i==bytes-1)
			buff[i] = i2c_readNak();
		else
			buff[i] = i2c_readAck();
	}
	i2c_stop();*/
}


#if BMP085_FILTERPRESSURE == 1
#define BMP085_AVARAGECOEF 21
static long k[BMP085_AVARAGECOEF];
long bmp085_avaragefilter(long input) {
	uint8_t i=0;
	long sum=0;
	for (i=0; i<BMP085_AVARAGECOEF; i++) {
		k[i] = k[i+1];
	}
	k[BMP085_AVARAGECOEF-1] = input;
	for (i=0; i<BMP085_AVARAGECOEF; i++) {
		sum += k[i];
	}
	return (sum /BMP085_AVARAGECOEF) ;
}
#endif

/*
 * read calibration registers
 */
void bmp085_getcalibration(struct bmp085 *self) {
	uint8_t buff[4];
	memset(buff, 0, sizeof(buff));

	bmp085_readmem(self, BMP085_REGAC1, buff, 2);
	bmp085_regac1 = ((int)buff[0] <<8 | ((int)buff[1]));
	bmp085_readmem(self, BMP085_REGAC2, buff, 2);
	bmp085_regac2 = ((int)buff[0] <<8 | ((int)buff[1]));
	bmp085_readmem(self, BMP085_REGAC3, buff, 2);
	bmp085_regac3 = ((int)buff[0] <<8 | ((int)buff[1]));
	bmp085_readmem(self, BMP085_REGAC4, buff, 2);
	bmp085_regac4 = ((unsigned int)buff[0] <<8 | ((unsigned int)buff[1]));
	bmp085_readmem(self, BMP085_REGAC5, buff, 2);
	bmp085_regac5 = ((unsigned int)buff[0] <<8 | ((unsigned int)buff[1]));
	bmp085_readmem(self, BMP085_REGAC6, buff, 2);
	bmp085_regac6 = ((unsigned int)buff[0] <<8 | ((unsigned int)buff[1]));
	bmp085_readmem(self, BMP085_REGB1, buff, 2);
	bmp085_regb1 = ((int)buff[0] <<8 | ((int)buff[1]));
	bmp085_readmem(self, BMP085_REGB2, buff, 2);
	bmp085_regb2 = ((int)buff[0] <<8 | ((int)buff[1]));
	bmp085_readmem(self, BMP085_REGMB, buff, 2);
	bmp085_regmb = ((int)buff[0] <<8 | ((int)buff[1]));
	bmp085_readmem(self, BMP085_REGMC, buff, 2);
	bmp085_regmc = ((int)buff[0] <<8 | ((int)buff[1]));
	bmp085_readmem(self, BMP085_REGMD, buff, 2);
	bmp085_regmd = ((int)buff[0] <<8 | ((int)buff[1]));
}

/*
 * get raw temperature as read by registers, and do some calculation to convert it
 */
void bmp085_getrawtemperature(struct bmp085 *self) {
	uint8_t buff[2];
	memset(buff, 0, sizeof(buff));
	long ut,x1,x2;

	//read raw temperature
	bmp085_writemem(self, BMP085_REGCONTROL, BMP085_REGREADTEMPERATURE);
	time_delay(5000L); // min. 4.5ms read Temp delay
	bmp085_readmem(self, BMP085_REGCONTROLOUTPUT, buff, 2);
	ut = ((long)buff[0] << 8 | ((long)buff[1])); //uncompensated temperature value

	//calculate raw temperature
	x1 = ((long)ut - bmp085_regac6) * bmp085_regac5 >> 15;
	x2 = ((long)bmp085_regmc << 11) / (x1 + bmp085_regmd);
	bmp085_rawtemperature = x1 + x2;
}

/*
 * get raw pressure as read by registers, and do some calculation to convert it
 */
void bmp085_getrawpressure(struct bmp085 *self) {
	uint8_t buff[3];
	memset(buff, 0, sizeof(buff));
	long up,x1,x2,x3,b3,b6,p;
	unsigned long b4,b7;

	#if BMP085_AUTOUPDATETEMP == 1
	bmp085_getrawtemperature(self);
	#endif

	//read raw pressure
	bmp085_writemem(self, BMP085_REGCONTROL, BMP085_REGREADPRESSURE+(BMP085_MODE << 6));
	time_delay((2 + (3<<BMP085_MODE)) * 1000L);
	bmp085_readmem(self, BMP085_REGCONTROLOUTPUT, buff, 3);
	up = ((((long)buff[0] <<16) | ((long)buff[1] <<8) | ((long)buff[2])) >> (8-BMP085_MODE)); // uncompensated pressure value

	//calculate raw pressure
	b6 = bmp085_rawtemperature - 4000;
	x1 = (bmp085_regb2* (b6 * b6) >> 12) >> 11;
	x2 = (bmp085_regac2 * b6) >> 11;
	x3 = x1 + x2;
	b3 = (((((long)bmp085_regac1) * 4 + x3) << BMP085_MODE) + 2) >> 2;
	x1 = (bmp085_regac3 * b6) >> 13;
	x2 = (bmp085_regb1 * ((b6 * b6) >> 12)) >> 16;
	x3 = ((x1 + x2) + 2) >> 2;
	b4 = (bmp085_regac4 * (uint32_t)(x3 + 32768)) >> 15;
	b7 = ((uint32_t)up - b3) * (50000 >> BMP085_MODE);
	p = b7 < 0x80000000 ? (b7 << 1) / b4 : (b7 / b4) << 1;
	x1 = (p >> 8) * (p >> 8);
	x1 = (x1 * 3038) >> 16;
	x2 = (-7357 * p) >> 16;
	bmp085_rawpressure = p + ((x1 + x2 + 3791) >> 4);

	#if BMP085_FILTERPRESSURE == 1
	bmp085_rawpressure = bmp085_avaragefilter(bmp085_rawpressure);
	#endif
}

/*
 * get celsius temperature
 */
int16_t bmp085_gettemperature(struct bmp085 *self) {
	bmp085_getrawtemperature(self);
	/*long x1, x2; 
	x1 = ((bmp085_rawtemperature - bmp085_regac6) * bmp085_regac5) >> 15; 
	x2 = (bmp085_regmc << 11) / (x1 + bmp085_regmd); 
	return (((x1 + x2 + 8) >> 4) - 32.0f) / 1.8f; 
	*/
	long temperature = ((bmp085_rawtemperature + 8)>>4);
	temperature = temperature;
	return temperature;
}

/*
 * get pressure
 */
long bmp085_getpressure(struct bmp085 *self) {
	bmp085_getrawpressure(self);
	return (bmp085_rawpressure + BMP085_UNITPAOFFSET);
}

/*
 * get altitude
 */
int16_t bmp085_getaltitude(struct bmp085 *self) {
	bmp085_getrawpressure(self);
	return ((1 - pow(bmp085_rawpressure/(double)101325, 0.1903 )) / 0.0000225577) + BMP085_UNITMOFFSET; 
}

/*
 * init bmp085
 */
void bmp085_init(struct bmp085 *self, struct packet_interface *port) {
	self->port = port;
	
	bmp085_getcalibration(self); //get calibration data
	bmp085_getrawtemperature(self); //update raw temperature, at least the first time

	#if BMP085_FILTERPRESSURE == 1
	//initialize the avarage filter
	uint8_t i=0;
	for (i=0; i<BMP085_AVARAGECOEF; i++) {
		bmp085_getrawpressure(self);
	}
	#endif
}
