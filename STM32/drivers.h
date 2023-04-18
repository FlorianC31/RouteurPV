#ifndef DRIVERS_H
#define DRIVERS_H

#include "stm32f10x.h"                  // Device header

#define NULL 0;

//############### Clocks ########################

// RCC_APB1ENR
#define TIMER2_CLK	100
#define TIMER3_CLK	101
#define TIMER4_CLK	102
#define TIMER5_CLK	103
#define TIMER6_CLK	104
#define TIMER7_CLK	105
#define SPI2_CLK		114
#define SPI3_CLK		115
#define USART2_CLK	117
#define USART3_CLK	118
#define USART4_CLK	119
#define USART5_CLK	120
#define I2C1_CLK	  121
#define I2C2_CLK	  122

// RCC_APB2ENR
#define GPIOA_CLK		202
#define GPIOB_CLK		203
#define GPIOC_CLK		204
#define GPIOD_CLK		205
#define GPIOE_CLK		206
#define ADC1_CLK		209
#define ADC2_CLK		210
#define TIMER1_CLK	211
#define SPI1_CLK		212
#define USART1_CLK	214

void enableClock(uint8_t periph);


//############### GPIO ########################

// GPIO pin mode
#define INPUT  			0
#define OUTPUT 			1
#define ALT_OUTPUT 	2
#define ANALOG_IN 	3

// GPIO output mode action
#define OFF	0
#define ON	1
#define REV	2

void initPin(GPIO_TypeDef* gpio, uint8_t pin, uint8_t mode);
int isOn(GPIO_TypeDef* gpio, uint8_t pin);
void setOutput(GPIO_TypeDef* gpio, uint8_t pin, uint8_t action);



//############### Timers ########################

void timer2init(void);
void systickDelay(uint32_t ms);


//############### USART ########################

void uartInit(uint8_t uart);
void sendUart2(char data);


//############### ADC ########################

void initAdc(uint8_t nbPins);
void setAdcSqr(uint8_t SQi, uint8_t adcChannel);
void grabAdc(int* data);


#endif
