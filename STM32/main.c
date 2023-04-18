#include "drivers.h" 

int main(void){
	
	// LED : set PA5 (pin 5 of GPIOA) as output
	initPin(GPIOA, 5, OUTPUT);
	
	// LED : set PC13 (pin 13 of GPIOC) as input
	initPin(GPIOC, 13, INPUT);
	
	timer2init();	
	uartInit(2);
	
	sendUart2('a');
	
	initAdc(2);
		
	while(1){
		//systickDelay(200);
		
		if (TIM2->SR & (1<<0)){	// wait UIF=1
			TIM2->SR &=~ (1<<0);	// clear flag
			if (!isOn(GPIOC, 13)){				// if PC13 is pressed (Pressed: 1, not pressed: 0)
				setOutput(GPIOA, 5, REV);		// reverse PA5
				sendUart2('a');
			}
		}
	}
}

