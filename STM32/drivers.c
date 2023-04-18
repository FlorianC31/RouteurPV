#include "drivers.h"

//############### Clocks ########################

void enableClock(uint8_t periph)
{
	int apb = periph / 100;
	int bit = periph % (apb * 100);
	
	if (apb == 1) {
		RCC->APB1ENR |= (1<<bit);
	}
	else if  (apb == 2) {
		RCC->APB2ENR |= (1<<bit);
	}	
}


//############### GPIO ########################

void initPin(GPIO_TypeDef* gpio, uint8_t pin, uint8_t mode)
{
	__IO uint32_t* reg;
	int prefix = 0;
	
	if (gpio == GPIOA) {
		enableClock(GPIOA_CLK);
	}
	else if (gpio == GPIOB) {
		enableClock(GPIOB_CLK);
	}
	else if (gpio == GPIOC) {
		enableClock(GPIOC_CLK);
	}
	else if (gpio == GPIOD) {
		enableClock(GPIOD_CLK);
	}	
	
	if (pin <= 7) {		// pin 0-7
		prefix = pin * 4;
		reg = &(gpio->CRL);
	}
	else {		 				// pin 8-15
		prefix = (pin - 8) * 4;
		reg = &(gpio->CRH);
	}	
	
	if (mode == INPUT) {
		// Set as floating input (CNF:01)
		// Set as input mode (MODE:00)
		//0100
		*reg |= (1<<(prefix+2));    //bit 2 = 1
	  *reg &=~ ((1<<(prefix+0)) | (1<<(prefix+1)) | (1<<(prefix+3)));  //bit 0 and bit 1 and bit 3 = 0
	}
	else if (mode == OUTPUT) {
		// Set as output push pull (CNF:00)
		// Set output refresh speed as 10 MHz (MODE:01)
		//0001
		*reg |= (1<<(prefix+0));    //bit 0 = 1
	  *reg &=~ ((1<<(prefix+1)) | (1<<(prefix+2)) | (1<<(prefix+3)));  //bit 1 and bit 2 and bit 3 = 0
	}
	else if (mode == ALT_OUTPUT) {
		// Set as output push pull (CNF:10)
		// Set output refresh speed as 10 MHz (MODE:01)
		//4001
		*reg |= ((1<<(prefix+0)) | (1<<(prefix+3)));    //bit 0 and bit 3 = 1
	  *reg &=~ ((1<<(prefix+1)) | (1<<(prefix+2)));  //bit 1 and bit 2 = 0
	}
	else if (mode == ANALOG_IN) {
		// Set as analog input (CNF:00)
		// Set as input mode (MODE:00)
	  *reg &=~ ((1<<(prefix+0)) | (1<<(prefix+1)) | (1<<(prefix+2)) | (1<<(prefix+3)));  //bit 0 and bit 1 and bit 3 = 0
	}
	
}

int isOn(GPIO_TypeDef* gpio, uint8_t pin)
{
	return gpio->IDR & (1<<pin);
}

void setOutput(GPIO_TypeDef* gpio, uint8_t pin, uint8_t action)
{
	switch(action) {
		case OFF:
			gpio->ODR &=~ (1<<pin);
		break;
		default:
		case ON:
			gpio->ODR |= (1<<pin);
		break;
		case REV:
			gpio->ODR ^= (1<<pin);
		break;			
	}
}


//############### Timers ########################

void timer2init(void)
{
	enableClock(TIMER2_CLK);
	TIM2->PSC = 7200 - 1;			//	(72MHz -> 10 kHz
	TIM2->ARR = 10000 - 1; 		//  (10000 / 10 khZ -> 1s)
	TIM2->CNT = 0;						// 	reset the systick counter flag 
	TIM2->CR1 |= (1<<0);			//  timer start
}

void systickDelay(uint32_t ms){
	int i = 0;
	
	// configure Systick to generate 50ms delay
	SysTick->LOAD = 72000 - 1;	// Durée du delay (1ms * 72MHz)
	SysTick->VAL = 0;	// reset the systick counter flag 
	SysTick->CTRL |= ((1<<2) | (1<<0));		// enable systick and select processor clock
	
	for (i=0; i<ms; i++){
		while((SysTick->CTRL & (1<<16)) == 0){
		}
	}	
}


//############### USART ########################

void uartInit(uint8_t uart)
{
	switch (uart) {
		default:
		case 2:
			// USART2
		  enableClock(USART2_CLK);
		  enableClock(GPIOA_CLK);
			initPin(GPIOA, 2, ALT_OUTPUT);		// USART2_TX on PA2
			USART2->CR1 |= (1<<13);			//  enable USART2
			USART2->CR1 &=~ (1<<12);		//  8 bits DATA size (M=0)
			USART2->CR2 &=~ (1<<13) | (1<<12);	//  1 stop bit (STROP = 00)
			USART2->BRR = 0x0EA6;				// usartDiv = 36000000 / 9600 = 3750 = 0x0EA6 (36 MHz / 9600 bauds)
			USART2->CR1 |= (1<<3);			// transmitter enable (TE=1)
			break;
	}		
}

void sendUart2(char data)
{
	USART2-> DR = data;					// writing the date in the DR register
	while(!(USART2->SR & (1<<6))) {}	// waiting for the end of transmission
}


//############### ADC ########################

void initAdc(uint8_t nbPins){	
	int i=0;
	struct AnalogPin{
		uint8_t channel;
		GPIO_TypeDef* gpio;
		uint8_t gpioClk;
		uint8_t pin;
	} analogPins[6] = {
    {0,  GPIOA, GPIOA_CLK, 0},
    {1,  GPIOA, GPIOA_CLK, 1},
    {2,  GPIOA, GPIOA_CLK, 2},
    {8,  GPIOB, GPIOB_CLK, 0},
    {11, GPIOC, GPIOC_CLK, 1},
    {10, GPIOC, GPIOC_CLK, 0}
	};
	
	enableClock(ADC1_CLK);
	
	// Configure ADCs prescaler to 72 MHz / 6 = 12 MHz (< 14 MHz). see §7.3.2
	RCC->CFGR |= (1<<15);
	RCC->CFGR &=~ (1<<14);
	
	// Enable converter (see §11.3.1 and §11.12.3)
	ADC1->CR2 |= (1<<0);
	
	
	// 1 conversion (see §11.3.3 and §11.12.9)
	ADC1->SQR1 &=~ ((1<<23) | (1<<22) | (1<<21) | (1<<20));
	
	
	for (i=0; i<nbPins; i++) {
		// Connect the analog pins of the nucleo board to the ADC sequential registers
		setAdcSqr(i+1, analogPins[i].channel);
		
		// enable the clock of the corresponding GPIO
		enableClock(analogPins[i].gpioClk);
		
		// Set the pin in analogic input
		initPin(analogPins[i].gpio, analogPins[i].pin, ANALOG_IN);
	}	
	
}

void setAdcSqr(uint8_t SQi, uint8_t adcChannel) {
	int gap = 0;
	__IO uint32_t* reg = NULL;
	
	if (adcChannel >16) {
		return;
	}
	if (SQi >= 1 && SQi <= 6) {
		reg = &(ADC1->SQR3); // (see § 11.12.11)
		gap = (SQi - 1) * 5;
	}
	if (SQi <= 12) {
		reg = &(ADC1->SQR2); // (see § 11.12.10)
		gap = (SQi - 7) * 5;
	}
	if (SQi <= 16) {
		reg = &(ADC1->SQR1); // (see § 11.12.9)
		gap = (SQi - 13) * 5;
	}
	else{
		return;
	}
	
	*reg &=~ (31<<gap); 		// Set all the 5 bits of the SQi to 0
	*reg |= (adcChannel<<gap);	// Set the value of the channel in the SQi
}

void grabAdc(int* data)
{
	ADC1->CR2 |= (1<<0); // Put 1 on bit ADON of ADC_CR2 register to start a new conversion (see §11.3.4)
	
	while(!(ADC1->SR & (1<<1))) {} // Wait for the end of conversion (EOC = 1) (see §11.12.1)
		
	*data = ADC1->DR;	
	
}
