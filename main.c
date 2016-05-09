//*****************************************************************************
// Red Light Green Light
// Sean Stockholm, Henry Megarry, Thomas Harren
//****************************************************************************

#include "msp.h"
#include <time.h>
#include <stdlib.h>


unsigned short sonarTime=0;
unsigned char enablePing=1;
unsigned char lost=0;
unsigned char winner=0;
unsigned char redLight=0;
unsigned int r = 0;

// Changes the light by xoring P1.7
void redLightGreenLight(void){
	P1OUT^=BIT7;
	redLight = ~redLight;
}

// setData is called every ten milliseconds.
// If the game has been lost then data is set to 0xFF and the lights will be on
// If we have not lost the game and P1IN is high (BIT6 is 1) then we win and chaange the lights to blink in th pattern 1100 0011
void setData(void){
	unsigned char data=0xA5;//1010 0101 first 4 for second 1, last 4 is pattern fo seckond
	static unsigned char activeBit=7;
	if(lost){
		data=0xFF;
	}else{
		if(P1IN & BIT6){
			data=0xA5; //high collector pin (blocked)
		}else{
			winner=1; //you win!!!!!!
			data=0xC3; //1100 0011
		}
	}
	if(data & (1<<activeBit)){
		//if bit 7 of data is high, return true.
		//light data line (pull signal from pin low)

		////*1*010 0101 -> get 1
		P2OUT&=~BIT4;
	}else{
		P2OUT|=BIT4;
	}
	if(activeBit == 0){
		activeBit=7;
	}else{
		activeBit--;
	}
}

// Controls when setData is run and when the shift register will push a data byte from P2.4 onto the shift register 
void TimerA0Interrupt(void) {
	static unsigned short counter=0;
	unsigned short intv=TA0IV; //IV=interrupt vector
	if(intv==0x0E){// OE is overflow interrupt
		if(counter < 40){
			if(counter % 10 == 0){
				setData();
			}
			if(counter %10 == 4){
				P2OUT&=~BIT3;
			}
			if(counter %10 == 6){
				P2OUT|=BIT3;
			}
		}
		if(++counter == 1000){
			enablePing=1; //at 1 sec, reanable ping
			counter=0;
		}
	}
}

// This controls the sonar.
void TimerA1Interrupt(void) {
	unsigned short intv=TA1IV; //IV=interrupt vector
	if(enablePing){
		if(intv==0x04){// set ping on
			P4OUT&=~BIT2; //interface transistor set to low => invert with pull-up give hi
		}else if (intv==0x06){ //turn ping back off
			P4OUT|=BIT2; //interface transistor set to hi
		}else if (intv==0x08){ //Start timer
			P4IFG&=~BIT4; //clear any pending P4 interpt
			P4IE|=BIT4; //actually turning interrupt on
			TA1CCTL1=0xB910; //pulls signal high, now we wait for ping to return
		}else if (intv==0x02){
			if(!winner){
				//see if target moved
				if(redLight){
					if(TA1CCR1 < (sonarTime - 50)){
						lost = 1;
					}
				}
				//pull a new rand to see if light color changes
				r = rand() % 10;
				if(r < 3){
					redLightGreenLight();
				}
				//write down the target's location for reference:
				sonarTime=TA1CCR1; //read val of caputre register
				P4IE&=~BIT4; //turning interrupt off
				enablePing=0;
			}
		}
	}

}

//This interrupt triggers the capture event by ending the timer
void PortFourInterrupt(void) {
	unsigned short iflag=P4IV; //IV=interrupt vector
	if(iflag == 0x0A){//P4.4 trigger (when sonar returns)
		TA1CCTL1=0xA910; //ground out to trigger capture event
	}
}


void selectPortFunction(int port, int line, int sel10, int sel1){
	//(p,l,0,0) will set port to Digital I/O
	if(port==1){
		if(P1SEL0 & BIT(line)!=sel10){
			if(P1SEL1 & BIT(line)!=sel1){
				P1SELC|=BIT(line);
			}else{
				P1SEL0^=BIT(line);
			}
		}else{
			if(P1SEL1 & BIT(line)!=sel1)
				P1SEL1^=BIT(line);
		}
	}else if (port==2){
		if(P2SEL0 & BIT(line)!=sel10){
			if(P2SEL1 & BIT(line)!=sel1){
				P2SELC|=BIT(line);
			}else{
				P2SEL0^=BIT(line);
			}
		}else{
			if(P2SEL1 & BIT(line)!=sel1)
				P2SEL1^=BIT(line);
		}
	}else if (port==4){
		if(P4SEL0 & BIT(line)!=sel10){
			if(P4SEL1 & BIT(line)!=sel1){
				P4SELC|=BIT(line);
			}else{
				P4SEL0^=BIT(line);
			}
		}else{
			if(P4SEL1 & BIT(line)!=sel1)
				P4SEL1^=BIT(line);
		}
	}
}

void initLines(void){
	selectPortFunction(1,6,0,0);
	selectPortFunction(1,7,0,0);
	selectPortFunction(2,3,0,0);
	selectPortFunction(2,4,0,0);
	selectPortFunction(4,2,0,0);
	selectPortFunction(4,4,0,0);
	P1DIR&=~BIT6; //input (to interrupt)
	P1DIR|=BIT7; //OUT
	P2DIR|=BIT3|BIT4;  //output (to timer(L8 or L9) and data on transistor(near L1)
	P4DIR|=BIT2;  //OUT
	P4DIR&=~BIT4; //IN
	P4IES|=BIT4; //Trigger P4 interupt on falling edge
}

void setClockFrequency(void){
	CS->KEY=0x695A;       //unlock
	CS->CTL1=0x00000223;  //run at 128, enable stuff for clock
	CS->CLKEN=0x0000800F;
	CS->KEY=0xA596;       //lock
}

void configureTimer(void){
	//timer 0
	TA0CTL=0x0100;   //Picks clock (above), count up mode, sets internal divider, shuts timer off.

	TA0CCTL0=0x2000; //Pick compare (not capture) register, interrupt off
	TA0CCR0=0x80;    //1ms timer
					 //interrups every milisecond

	TA0CCTL1=0x2010; //Pick compare (not capture) register, interrupt on
	TA0CCR1=0x0040;   //sets the max time compare  for this capture, will wait until overflow (will be overwritten)

	TA0CTL=0x0116;   //Sets counter to 0, turns counter on, enables timeout (aka overflow) interrups

	//timer 1
	TA1CTL=0x0100;   //Picks clock (above), count up mode, sets internal divider, shuts timer off.

	TA1CCTL0=0x2000; //Pick compare (not capture) register, interrupt off
	TA1CCR0=0x0A00;  //20ms timer (x20 of timer 0)
					 //interrups every milisecond

	TA1CCTL1=0xA910; //Pick **capture** register, capture signals high-low signal init at ground (will set hi in P4interrupt), interrupt on
	TA1CCR1=0x0040;   //sets the max time compare  for this capture, will wait until overflow (will be overwritten)

	TA1CCTL2=0x2010; //Pick compare (not capture) register, interrupt on
	TA1CCR2=0x0001;   //1 clock tick after reset, pulse ping
	TA1CCTL3=0x2010; //Pick compare (not capture) register, interrupt on
	TA1CCR3=0x0002;   //1 clock tick after CCR2, turn ping off
	TA1CCTL4=0x2010; //Pick compare (not capture) register, interrupt on
	TA1CCR4=0x0064;   //Enable echo detect


	TA1CTL=0x0116;   //Sets counter to 0, turns counter on, enables timeout (aka overflow) interrups
}


void main(void)
{
	
    WDTCTL = WDTPW | WDTHOLD;           // Stop watchdog timer
    setClockFrequency();
	configureTimer();
	initLines();
	P2OUT&=~(BIT3&BIT4);
	NVIC_EnableIRQ(TA0_N_IRQn);
	NVIC_EnableIRQ(TA1_N_IRQn);
	NVIC_EnableIRQ(PORT4_IRQn);
	//init random
	srand(time(NULL));
	//init light to green light on
	P1OUT|=BIT7;
	
	while(1){}
}
