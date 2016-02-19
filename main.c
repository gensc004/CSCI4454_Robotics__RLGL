//*****************************************************************************
//
// MSP432 main.c template - Empty main
//
//****************************************************************************

#include "msp.h"

void setData(void){
	unsigned char data=0xA5;//1010 0101 first 4 for second 1, last 4 is pattern fo seckond
	static unsigned char activeBit=7;
	if(P1IN & BIT6){
		data=0xA5; //high collector pin (blocked)
	}else{
		data=0xC3; //1100 0011
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
			counter=0;
		}
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
	}else{
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
	}
}

void initLines(void){
	selectPortFunction(2,3,0,0);
	selectPortFunction(2,4,0,0);
	selectPortFunction(1,6,0,0);
	P2DIR|=BIT3|BIT4;
	P1DIR&=~BIT6; //input
}

void setClockFrequency(void){
	CSKEY=0x695A;       //unlock
	CSCTL1=0x00000223;  //run at 128, enable stuff for clock
	CSCLKEN=0x0000800F;
	CSKEY=0xA596;       //lock
}

void configureTimer(void){
	TA0CTL=0x0100;   //Picks clock (above), count up mode, sets internal divider, shuts timer off.

	TA0CCTL0=0x2000; //Pick compare (not capture) register, interrupt off
	TA0CCR0=0x80;  //(128)//sets the max time compare register (1,2,3 depend on this peak)
					 //interrups every milisecond

	TA0CCTL1=0x2010; //Pick compare (not capture) register, interrupt on
	TA0CCR1=0x0040;   //sets the max time compare  for this capture, will wait until overflow (will be overwritten)
	TA0CCTL2=0x2010;
	TA0CCR2=0x0040;
	TA0CCTL3=0x2010;
	TA0CCR3=0x0040;

	TA0CTL=0x0116;   //Sets counter to 0, turns counter on, enables timeout (aka overflow) interrups
}


void main(void)
{
	
    WDTCTL = WDTPW | WDTHOLD;           // Stop watchdog timer
    setClockFrequency();
	configureTimer();
	initLines();
	P2OUT&=~(BIT3&BIT4);
	NVIC_EnableIRQ(TA0_N_IRQn);
	
	while(1){}
}
