obj-$(CONFIG_APP_print_hello) += apps/print-hello.o
obj-$(CONFIG_APP_imx23_ssd1306) += apps/imx23-ssd1306.o
obj-$(CONFIG_APP_test) += apps/test.o
obj-$(CONFIG_APP_RTOS_TEST) += apps/rtos-test.o
obj-$(CONFIG_ARDUINO_PWM_BLINK_THREAD) += apps/arduino/pwm_blink_and_input.o
obj-$(CONFIG_ARDUINO_EXTERNAL_RTC_EEPROM) += apps/arduino/i2c_external_rtc.o
obj-$(CONFIG_ARDUINO_APP_RANGEFINDER) += apps/arduino/6ch-rangefinder.o
obj-$(CONFIG_ARDUINO_SSD1306) += apps/arduino/i2c_ssd1306.o
obj-$(CONFIG_ARDUINO_GPIO_WAKEUP) += apps/arduino/gpio_wakeup.o
obj-$(CONFIG_APP_ARDUINO_MINOPOSD) += apps/arduino/minoposd.o
obj-$(CONFIG_APP_STM32_BLINKY) += apps/stm32/blinky.o
LDFLAGS-$(CONFIG_APP_test) += -lpthread

include $(srctree)/apps/arduino/Makefile
