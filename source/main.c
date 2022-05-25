/*	Author: mchoi041
 *	Lab Section: 023
 *	Assignment: Custom Project
 *	Description: A Trip Down Memory Lane
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include "ai.h"

enum Draw_States{Start1,Wait1,Build,Memorize,Move,Win,Loss,Release} Draw_state;

volatile unsigned char TimerFlag = 0;

unsigned long _avr_timer_M = 1;
unsigned long _avr_timer_cntcurr = 0;

static unsigned char Score = 0;
static unsigned char tmpScore = 0;
unsigned char tempB;
static int reset = 0;
static int msg = 0;
static int msg2 = 1;
static int levelNum = 4;

void TimerOn() {
	TCCR1B = 0x0B;
	OCR1A = 125;
	TIMSK1 = 0x02;
	TCNT1 = 0;
	_avr_timer_cntcurr = _avr_timer_M;
	SREG |= 0x80;
}

void TimerOff() {
	TCCR1B = 0x00;
}

void TimerISR() {
	TimerFlag = 1;
}

ISR(TIMER1_COMPA_vect) {
	_avr_timer_cntcurr--;
	if(_avr_timer_cntcurr == 0) {
		TimerISR();
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

void transmit_data(unsigned char data) {
	for(int i = 0; i < 8; i++) {
		PORTD = 0x08;
		PORTD = PORTD | ((data >> i) & 0x01);
		PORTD = PORTD | 0x04;
	}
	PORTD = PORTD | 0x02;
	PORTD = 0x00;
}

void ADC_init() {
	DDRA = 0x00;		
	ADCSRA = 0x87;		
	ADMUX = 0x40;		
}

int ReadADC(char channel) {
	int ADC_value;
	
	ADMUX = (0x40) | (channel & 0x07);
	ADCSRA |= (1<<ADSC);	
	while((ADCSRA &(1<<ADIF))== 0);	
	
	ADCSRA |= (1<<ADIF);	
	ADC_value = (int)ADCL;	
	ADC_value = ADC_value + (int)ADCH*256;

	return ADC_value;		
}

int main(void) {
	DDRA = 0x0F; PORTA = 0xF0;
	DDRD = 0xFF; PORTD = 0x0F;
	DDRC = 0xFF; PORTC = 0x00;
	DDRB = 0xFF; PORTB = 0xFF;
	
	const unsigned long timerPeriod = 100;
	unsigned long Draw_elapsedTime = 100;
	
	TimerSet(timerPeriod);
	TimerOn(); 
	ADC_init();
	LCD_init();

	while (1) {
		reset = (~PINA) & 0x10; //Button input
		if(Draw_elapsedTime >= 100) {
			Draw_Tick();
			Draw_elapsedTime = 0;
		}
		while(!TimerFlag);
		TimerFlag = 0;
		Draw_elapsedTime += timerPeriod;
	}
	return 1;
}

void Draw_Tick() {
	char PORT[8] = {128,64,32,16,8,4,2,1};//PORTB pin values
	char PORTs[8] = {0xFE,0xFD,0xFB,0xF7,0xEF,0xDF,0xBF,0x7F};//PORTD shift register values
	static int a = 0; //y coord
	static int b = 0; //x coord
	static int c = 0; //sm counter
	//static unsigned char d = 0x00;
	static int Order[8][8]={
	{0,0b10011001,0b00111001,0b10010011,0b00100101,0b00110011,0b01100110,0},
	{0,0b00011111,0b00110000,0b00100111,0b00111101,0b00000001,0b00000001,0},
	{0,0b00111011,0b00101010,0b00101110,0b00100000,0b00111000,0b00001000,0},
	{0,0b01111011,0b01001010,0b01011010,0b01010010,0b01011110,0b01000000,0},
	{0,0b00000001,0b01110001,0b01011001,0b01001101,0b01110111,0b00010000,0},
	{0,0b00011111,0b01110000,0b01001110,0b01011010,0b01110110,0b00000100,0},
	{0,0b01100110,0b11100100,0b11100100,0b10010100,0b11001100,0b10011001,0},
	{0,0b11001100,0b01110010,0b11001001,0b01001010,0b10011001,0b11001100,0}}; //levels

	static int Level[8][1] = {0b00011111,0b00110000,0b00100111,0b00111101,0b00000001,0b00000001,0}; //displayed level
	
	uint8_t l =0;
	
	uint16_t xval, yval;
	xval = ReadADC(0);
	yval = ReadADC(1);
	
	switch(Draw_state) {
		case Start1:
		Draw_state = Wait1;
		break;
		case Wait1:
		Draw_state = Build;
		break;
		case Build:
		tmpScore = 0;
		if(reset == 0x10) {
			msg = 0;
			Draw_state = Memorize;
		}	
		else {
			if(msg != 1) {
				LCD_DisplayString(1,"Press button to begin");
				msg = 1;
			}
			Draw_state = Wait1;
		}
		break;
		case Memorize:
		if(reset == 0x10) {
			Draw_state = Release;
		}
		if(c < 1) {
			c = 0;
			Draw_state = Memorize;
		}
		else {
			Draw_state = Move;
		}
		break;
		case Move:
		if((msg2!=1)) {
			LCD_DisplayString(1,"Score: ");
			if(Score > 19) {
				tmpScore = Score - 20;
				LCD_Cursor(8);
				LCD_WriteData('2');
				LCD_Cursor(9);
				LCD_WriteData(tmpScore + '0');
			}
			else if(Score > 9) {
				tmpScore = Score - 10;
				LCD_Cursor(8);
				LCD_WriteData('1');
				LCD_Cursor(9);
				LCD_WriteData(tmpScore + '0');
			}
			else {
				LCD_Cursor(8);
				LCD_WriteData(Score + '0');
			}
			msg2 = 1;
		}
		if(reset == 0x10) {
			msg = 0;
			Draw_state = Release;
		}
		else if(((a!= 0) | (b!=0)) && ((a!=7) )) {
			if(((((Level[levelNum][a]) >> b) & 0x01) == 0x00)||(b==7)) {
			msg = 0;
			Draw_state = Loss;
			}
		}
		else if((a==7)) {
			msg = 0;
			Draw_state = Win;
		}
		else {
			if(msg !=1) {
				LCD_DisplayString(1,"Score: ");
				LCD_Cursor(8);
				LCD_WriteData('0');
				msg = 1;
			}
		Draw_state = Move;
		}
		break;
		case Win:
		if(reset == 0x10) {
			msg = 0;
			Draw_state = Release;
		}
		else {
			if(msg != 1) {
				LCD_DisplayString(1, "Good Memory!    Score:");
				if(Score > 19) {
					tmpScore = Score - 20;
					LCD_Cursor(24);
					LCD_WriteData('2');
					LCD_Cursor(25);
					LCD_WriteData(tmpScore + '0');
				}
				else if(Score > 9) {
					tmpScore = Score - 10;
					LCD_Cursor(24);
					LCD_WriteData('1');
					LCD_Cursor(25);
					LCD_WriteData(tmpScore + '0');
				}
				else {
					LCD_Cursor(24);
					LCD_WriteData(Score + '0');
				}
				msg = 1;
			}
			Draw_state = Win;
		}
		break;
		case Loss:
		if(reset == 0x10) {
			msg = 0;
			Draw_state = Release;
		}
		else {
			if(msg != 1) {
				LCD_DisplayString(1, "Game Over!      Score:");
				if(Score > 19) {
					tmpScore = Score - 20;
					LCD_Cursor(24);
					LCD_WriteData('2');
					LCD_Cursor(25);
					LCD_WriteData(tmpScore + '0');
				}
				else if(Score > 9) {
					tmpScore = Score - 10;
					LCD_Cursor(24);
					LCD_WriteData('1');
					LCD_Cursor(25);
					LCD_WriteData(tmpScore + '0');
				}
				else {
					LCD_Cursor(24);
					LCD_WriteData(Score + '0');
				}
				msg = 1;
			}
			Draw_state = Loss;
		}
		break;
		case Release:
		if(reset == 0x10) {
			Draw_state = Release;
		}
		else if(reset != 0x10) {
			Draw_state = Wait1;
		}
		default:
		Draw_state = Start1;
		break;
	}
	switch(Draw_state) {
		case Start1: break;
		case Wait1:
		levelNum = (rand()%5)+1; //randomize levels %# of levels
		for(int q = 0; q < 8; q++) {
		Level[levelNum][q] = Order[levelNum][q]; //Copy Order to levelNum
		}
		PORTB = 0x00;
		break;
		case Build:
		PORTB = 0x00;
		break;
		case Memorize:
		LCD_Cursor(1);
		LCD_DisplayString(1,"Memorize the    lane!");
		//LCD_Cursor(24); //Display level #
		//LCD_WriteData(levelNum + '0');
		a = 0;
		b = 0;
		Score=0;
		for (int w = 0; w < 8; w++){
			l = PORT[w];
			for (int s = 0; s < 1000; s++){
				for (int z = 0; z < 7; z++){
					PORTB = PORT[z];
					transmit_data(~Level[levelNum][z+1]);
					PORTB = 0xFF;
				}
			}
		}
		c+=1;
		break;
		case Move:
		if((yval >= 0x0250) && (a != 7)) {
			a += 1;
			Score+=1;
			Level[levelNum][a-1] = (Level[levelNum][a-1]) & (0xFF7F >> (7-b));
			msg2 = 0;
		}
		else if(((yval < 0x0100)) && (a != 0)) {
			a = a-1;
			Score+=1;
			Level[levelNum][a+1] = (Level[levelNum][a+1]) & (0xFF7F >> (7-b));
			msg2 = 0;
		}
		else if((xval >= 0x0250) && (b != 0)) {
			b = b-1;
			Score+=1;
			Level[levelNum][a] = (Level[levelNum][a]) & (0xFF7F >> (7-b-1));
			msg2  = 0;
		}
		else if(((xval < 0x0100)) && (b != 7)) {
			b += 1;
			Score+=1;
			Level[levelNum][a] = (Level[levelNum][a]) & (0xFF7F >> (7-b+1));
			msg2 = 0;
		}
		PORTB = PORT[a];
		transmit_data(PORTs[b]); 
		break;
		case Win:
		PORTB = 0xFF;
		transmit_data(0x00);
		break;
		case Loss:
		PORTB = 0x00;
		break;
		case Release:
		break;
		default: break;
	}
}
