#include <avr/io.h>
#include "timer.h"
#include "io.c"
#include "io.h"
//#include "nokia5110_chars.h"


//header to enable data flow control over pins

#define F_CPU 1000000
#include "nokia5110.h"
#include "nokia5110.c"

//telling controller crystal frequency attached

#include <util/delay.h>

char PORT[8] = {1,2,4,8,16,32,64,128};//pin values of PORTD

static int ALPHA[26][8]={{0,0b01111111,0b11111111,0b11001100,0b11001100,0b11001100,0b11111111,0b01111111},

{0,0b00111100,0b01111110,0b11011011,0b11011011,0b11011011,0b11111111,0b11111111},

{0,0b11000011,0b11000011,0b11000011,0b11000011,0b11100111,0b01111110,0b00111100},

{0,0b01111110,0b10111101,0b11000011,0b11000011,0b11000011,0b11111111,0b11111111},

{0,0b11011011,0b11011011,0b11011011,0b11011011,0b11011011,0b11111111,0b11111111},

{0,0b11011000,0b11011000,0b11011000,0b11011000,0b11011000,0b11111111,0b11111111},

{0b00011111,0b11011111,0b11011000,0b11011011,0b11011011,0b11011011,0b11111111,0b11111111},

{0,0b11111111,0b11111111,0b00011000,0b00011000,0b00011000,0b11111111,0b11111111},

{0b11000011,0b11000011,0b11000011,0b11111111,0b11111111,0b11000011,0b11000011,0b11000011},

{0b11000000,0b11000000,0b11000000,0b11111111,0b11111111,0b11000011,0b11001111,0b11001111},

{0,0b11000011,0b11100111,0b01111110,0b00111100,0b00011000,0b11111111,0b11111111},

{0b00000011,0b00000011,0b00000011,0b00000011,0b00000011,0b00000011,0b11111111,0b11111111},

{0b11111111,0b11111111,0b01100000,0b01110000,0b01110000,0b01100000,0b11111111,0b11111111},

{0b11111111,0b11111111,0b00011100,0b00111000,0b01110000,0b11100000,0b11111111,0b11111111},

{0b01111110,0b11111111,0b11000011,0b11000011,0b11000011,0b11000011,0b11111111,0b01111110},

{0,0b01110000,0b11111000,0b11001100,0b11001100,0b11001100,0b11111111,0b11111111},

{0b01111110,0b11111111,0b11001111,0b11011111,0b11011011,0b11000011,0b11111111,0b01111110},

{0b01111001,0b11111011,0b11011111,0b11011110,0b11011100,0b11011000,0b11111111,0b11111111},

{0b11001110,0b11011111,0b11011011,0b11011011,0b11011011,0b11011011,0b11111011,0b01110011},

{0b11000000,0b11000000,0b11000000,0b11111111,0b11111111,0b11000000,0b11000000,0b11000000},

{0b11111110,0b11111111,0b00000011,0b00000011,0b00000011,0b00000011,0b11111111,0b11111110},

{0b11100000,0b11111100,0b00011110,0b00000011,0b00000011,0b00011110,0b11111100,0b11100000},

{0b11111110,0b11111111,0b00000011,0b11111111,0b11111111,0b00000011,0b11111111,0b11111110},

{0b01000010,0b11100111,0b01111110,0b00111100,0b00111100,0b01111110,0b11100111,0b01000010},

{0b01000000,0b11100000,0b01110000,0b00111111,0b00111111,0b01110000,0b11100000,0b01000000},

{0b11000011,0b11100011,0b11110011,0b11111011,0b11011111,0b11001111,0b11000111,0b11000011}};//characters a,b,c,d,e,f,g,...z binary codecs

char NAME[5];///circuitdigest character values

uint8_t l =0;

//header to enable delay function in program
unsigned char GetBit(unsigned char x, unsigned char k){
	return ((x & (0x01 << k)) != 0);
}
unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b) {
	return (b ?  (x | (0x01 << k))  :  (x & ~(0x01 << k)) );
	//   Set bit to 1           Set bit to 0
}
void ADC_Init()
{
	DDRA = 0x00;		/* Make ADC port as input */
	ADCSRA = 0x87;		/* Enable ADC, fr/128  */
	ADMUX = 0x40;		/* Vref: Avcc, ADC channel: 0 */
}

int ReadADC(char channel)
{
	int ADC_value;
	
	ADMUX = (0x40) | (channel & 0x07);/* set input channel to read */
	ADCSRA |= (1<<ADSC);	/* start conversion */
	while((ADCSRA &(1<<ADIF))== 0);	/* monitor end of conversion interrupt flag */
	
	ADCSRA |= (1<<ADIF);	/* clear interrupt flag */
	ADC_value = (int)ADCL;	/* read lower byte */
	ADC_value = ADC_value + (int)ADCH*256;/* read higher 2 bits, Multiply with weightage */

	return ADC_value;		/* return digital value */
}

static unsigned char result;
static unsigned char lightmsg;
static unsigned char reset;
static unsigned char  lives = 3;
enum J_states {j_start, wait, up, down,press} j_state;
void TickFct_J()
{
	unsigned short y;
	unsigned short x;
	y = ReadADC(1);
	x = ReadADC(2);
	switch(j_state)
	{
		case j_start:
					if ((y*5)/1024 < 2 )
					{
						j_state = down;
					}
					else if ((y*5)/1024 > 2.5)
					{
						
						j_state = up;
					}
					else if((x*5)/1024 < 2.5)
					{
						j_state = press;
					}
					else
					{
						j_state = wait;
					}
					break;
		case up:
					if ((y*5)/1024 < 2 )
					{
						j_state = down;
					}
					else if ((y*5)/1024 > 2.5)
					{
						
						j_state = up;
					}
					else if((x*5)/1024 < 2.5)
					{
						j_state = press;
					}
					else
					{
						j_state = wait;
					}
					break;
		case down:
					if ((y*5)/1024 < 2 )
					{
						j_state = down;
					}
					else if ((y*5)/1024 > 2.5)
					{
						
						j_state = up;
					}
					else if((x*5)/1024 < 2.5)
					{
						j_state = press;
					}
					else
					{
						j_state = wait;
					}
					break;
		case wait:
					if ((y*5)/1024 < 2 )
					{
						j_state = down;
					}
					else if ((y*5)/1024 > 2.5)
					{
						
						j_state = up;
					}
					else if((x*5)/1024 < 2.5)
					{
						j_state = press;
					}
					else
					{
						j_state = wait;
					}
					break;
					
		case press:
					//PORTC = SetBit(PORTC, 6,0);
					if ((y*5)/1024 < 2 )
					{
						j_state = down;
					}
					else if ((y*5)/1024 > 2.5)
					{
						
						j_state = up;
					}
					else
					{
						j_state = wait;
					}
					break;
	}
	switch(j_state) //actions
	{
		case j_start:
					break;
		case up:
					result = 0x02; // up or 0x02
					//shared variable result will represent 'up choice'
					break;
		case down:
					result = 0x01; //down or 0x01
					//shared variable result will represent 'down choice'
					break;
					
		case press:
					//PORTC = SetBit(PORTC, 6,1);
					reset = 0x01;
					break;
		case wait:
					result = 0x00; //centered
					//shared variable result will represent 'no choice'
					break;
					
	}
}

enum L_states{L_start, ouch, nice,} l_state;
void TickFct_L()
{
	switch(l_state) //transitions
	{
		case L_start:
					if(lightmsg == 0x02)
					{
						l_state = ouch;
						break;
					}
					else if (lightmsg == 0x01)
					{
						l_state = nice;
						break;
					}
					else if (lightmsg == 0x00)
					{
						l_state = L_start;
						break;
					}
					break;
		case ouch:
					if(lightmsg == 0x02)
					{
						l_state = ouch;
						break;
					}
					else if (lightmsg == 0x01)
					{
						l_state = nice;
						break;
					}
					else if (lightmsg == 0x00)
					{
						l_state = L_start;
						break;
					}
					break;
		case nice:
					if(lightmsg == 0x02)
					{
						l_state = ouch;
						break;
					}
					else if (lightmsg == 0x01)
					{
						l_state = nice;
						break;
					}
					else if (lightmsg == 0x00)
					{
						l_state = L_start;
						break;
					}
					break;
		
	}
	
	switch(l_state) //actions
	{
		
		case L_start:
					lightmsg = 0x00;
					break;
		case ouch:
					lives--;
					if(lives == 3)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,1);
					}
					else if (lives == 2)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 1)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 0)
					{
						PORTC = SetBit(PORTC,7,0);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
					}
					//char NAME[] ={22, 17, 14, 13, 6}; OUCH
					NAME[0] = 22;
					NAME[1] = 17;
					NAME[2] = 14;
					NAME[3] = 13;
					NAME[4] = 6;
					for (int m=0;m<sizeof NAME;m++)
					{
						l = NAME[m];
						for (int n=0;n<500;n++)//execute 200 times for the eye to catch
						{
							for (int j=0;j<4;j++)
							{
								PORTB = PORT[j];// ROW
								PORTD = ~ALPHA[l][j];//show half of character (COLUMN)
								_delay_us(1000);
							}
							PORTB=0x00;//clear screen after show
							for (int k=0;k<4;k++)
							{
								PORTC = (PORTC & 0xF0) | PORT[k];// ROW
								PORTD = ~ALPHA[l][k+4];//show other half of character(COLUMN)
								_delay_us(1000);
							}
							PORTC= PORTC & 0xF0;//clear screen after show.
						}
					}
					lightmsg = 0x00;
					//lives--;
					break;
		case nice:
					//char NAME[] ={17,8,6,7,19}; //RIGHT 
					NAME[0] = 17;
					NAME[1] = 8;
					NAME[2] = 6;
					NAME[3] = 7;
					NAME[4] = 19;
					for (int m=0;m<sizeof NAME;m++)
					{
						l = NAME[m];
						for (int n=0;n<500;n++)//execute 200 times for the eye to catch
						{
							for (int j=0;j<4;j++)
							{
								PORTB = PORT[j];// ROW
								PORTD = ~ALPHA[l][j];//show half of character (COLUMN)
								_delay_us(1000);
							}
							PORTB=0x00;//clear screen after show
							for (int k=0;k<4;k++)
							{
								PORTC = (PORTC & 0xF0) | PORT[k];// ROW
								PORTD = ~ALPHA[l][k+4];//show other half of character(COLUMN)
								_delay_us(1000);
							}
							PORTC=PORTC & 0xF0;//clear screen after show.
						}
					}
					lightmsg = 0x00;
					break;
	}
	
	
}


 
enum N_states{n_start, one,oneA,oneB,two,twoA,twoB,three,threeA,threeB, four, fourA,fourB, win, intro, intro2,intro3,intro4,intro5, gameover}n_state;
void TickFct_N()
{
	switch(n_state)
	{
		case n_start: //transitions
					n_state = intro;
					break;
		case intro:
					
					if(reset == 0x01)
					{
						reset = 0x00;
						n_state = intro;
						break;
					}
					
					if(result == 0x02) //up first
					{
						n_state = intro2;
						break;
					}
					else if (result == 0x01) // down second
					{
						n_state = intro2;
						break;
					}
					else
					{
						lightmsg = 0x00;
						n_state = intro;
						break;
					}
		case intro2:
		
					if(reset == 0x01)
					{
						reset = 0x00;
						n_state = intro;
						break;
					}
					
					if(result == 0x02) //up first
					{
						
						n_state = intro3;
						break;
					}
					else if (result == 0x01) // down second
					{
						
						n_state = intro3;
						break;
					}
					else
					{
						lightmsg = 0x00;
						n_state = intro2;
						break;
					}
		case intro3:
					if(reset == 0x01)
					{
						reset = 0x00;
						n_state = intro;
						break;
					}
					
					if(result == 0x02) //up first
					{
						
						n_state = intro4;
						break;
					}
					else if (result == 0x01) // down second
					{
						
						n_state = intro4;
						break;
					}
					else
					{
						lightmsg = 0x00;
						n_state = intro3;
						break;
					}
					
		case intro4:
					if(reset == 0x01)
					{
						reset = 0x00;
						n_state = intro;
						break;
					}
					
					if(result == 0x02) //up first
					{
						
						n_state = intro5;
						break;
					}
					else if (result == 0x01) // down second
					{
						
						n_state = intro5;
						break;
					}
					else
					{
						lightmsg = 0x00;
						n_state = intro4;
						break;
					}
		case intro5:
					if(reset == 0x01)
					{
						reset = 0x00;
						n_state = intro;
						break;
					}
					
					if(result == 0x02) //up first
					{
						
						n_state = one;
						break;
					}
					else if (result == 0x01) // down second
					{
						
						n_state = one;
						break;
					}
					else
					{
						lightmsg = 0x00;
						n_state = intro5;
						break;
					}
		case gameover:
					
					if(reset == 0x01)
					{
						reset = 0x00;
						n_state = intro;
						break;
					}
					

		case one:
					if(lives == 0)
					{
						n_state = gameover;
						break;
					}
					
					if(reset == 0x01)
					{
						reset = 0x00;
						n_state = intro;
						break;
					}
					
					if(result == 0x02) //up first
					{
						lightmsg = 0x02; //nice
						n_state = oneB;
						break;
					}
					else if (result == 0x01) // down second
					{
						lightmsg = 0x01; //ouch
						n_state = oneA;
						break;
					}
					else
					{
						lightmsg = 0x00;
						n_state = one;
						break;
					}
		case oneA:
					if(lives == 0)
					{
						n_state = gameover;
						break;
					}
					if(reset == 0x01)
					{
						reset = 0x00;
						n_state = intro;
						break;
					}
					if(result == 0x01 || result == 0x02)
					{
						n_state = two;
						break;
					}
					break;
		case oneB:
					if(lives == 0)
					{
						n_state = gameover;
						break;
					}
					if(reset == 0x01)
					{
						reset = 0x00;
						n_state = intro;
						break;
					}
					if(result == 0x01 || result == 0x02)
					{
						n_state = two;
						break;
					}
					break;
		case two:	
					if(lives == 0)
					{
						n_state = gameover;
						break;
					}
					
					if(reset == 0x01)
					{
						reset = 0x00;
						n_state = intro;
						break;
					}
					
					if(result == 0x02) //up first
					{
						lightmsg = 0x02; //nice
						n_state = twoB;
						break;
					}
					else if (result == 0x01) // down second
					{
						lightmsg = 0x01; //ouch
						n_state = twoA;
						break;
					}
					else
					{
						lightmsg = 0x00;
						n_state = two;
						break;
					}
		case twoA:
					if(lives == 0)
					{
						n_state = gameover;
						break;
					}
					if(reset == 0x01)
					{
						reset = 0x00;
						n_state = intro;
						break;
					}
					if(result == 0x01 || result == 0x02)
					{
						n_state = three;
						break;
					}
					break;
		case twoB:
					if(lives == 0)
					{
						n_state = gameover;
						break;
					}
					if(reset == 0x01)
					{
						reset = 0x00;
						n_state = intro;
						break;
					}
					if(result == 0x01 || result == 0x02)
					{
						n_state = three;
						break;
					}
					break;
		case three:
					
					if(lives == 0)
					{
						n_state = gameover;
						break;
					}
					if(reset == 0x01)
					{
						reset = 0x00;
						n_state = intro;
						break;
					}
					
					if(result == 0x02) //up first
					{
						lightmsg = 0x01; //nice
						n_state = threeB;
						break;
					}
					else if (result == 0x01) // down second
					{
						lightmsg = 0x02; //ouch
						n_state = threeA;
						break;
					}
					else
					{
						lightmsg = 0x00;
						n_state = three;
						break;
					}
		case threeA:
					if(lives == 0)
					{
						n_state = gameover;
						break;
					}
					if(reset == 0x01)
					{
						reset = 0x00;
						n_state = intro;
						break;
					}
					if(result == 0x01 || result == 0x02)
					{
						n_state = four;
						break;
					}
					break;
		case threeB:
					if(lives == 0)
					{
						n_state = gameover;
						break;
					}
					if(reset == 0x01)
					{
						reset = 0x00;
						n_state = intro;
						break;
					}
					if(result == 0x01 || result == 0x02)
					{
						n_state = four;
						break;
					}
					break;
		case four:
					if(lives == 0)
					{
						n_state = gameover;
						break;
					}
					if(reset == 0x01)
					{
						reset = 0x00;
						n_state = intro;
						break;
					}
					
					if(result == 0x02) //up first
					{
						lightmsg = 0x01; //nice
						n_state = fourB;
						break;
					}
					else if (result == 0x01) // down second
					{
						lightmsg = 0x02; //ouch
						n_state = fourA;
						break;
					}
					else
					{
						lightmsg = 0x00;
						n_state = four;
						break;
					}
		case fourA:
					if(lives == 0)
					{
						n_state = gameover;
						break;
					}
					if(reset == 0x01)
					{
						reset = 0x00;
						n_state = intro;
						break;
					}
					if(result == 0x01 || result == 0x02)
					{
						n_state = win;
						break;
					}
					break;
		case fourB:
					if(lives == 0)
					{
						n_state = gameover;
						break;
					}
					if(reset == 0x01)
					{
						reset = 0x00;
						n_state = intro;
						break;
					}
					if(result == 0x01 || result == 0x02)
					{
						n_state = win;
						break;
					}
					break;
		case win:
					if(lives == 0)
					{
						
						n_state = gameover;
					}
					if(reset == 0x01)
					{
						reset = 0x00;
						n_state = intro;
						break;
					}
					
				
	}
	switch(n_state) //actions
	{
		case n_start:
					lives = 3;
					lightmsg = 0x00;
					if(lives == 3)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,1);
					}
					else if (lives == 2)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 1)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 0)
					{
						PORTC = SetBit(PORTC,7,0);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
					}
					break;
		case intro:	
					lives = 3;
					nokia_lcd_clear();
					nokia_lcd_power(1);
					nokia_lcd_write_string("Welcome to this text adventure! Flick up or down to continue the tutorial!", 1);
					nokia_lcd_render();
					if(lives == 3)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,1);
					}
					else if (lives == 2)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 1)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 0)
					{
						PORTC = SetBit(PORTC,7,0);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
					}
					//lightmsg = 0x00;
					break;
					
		case intro2:
					nokia_lcd_clear();
					nokia_lcd_power(1);
					nokia_lcd_write_string("Flick up for the first presented choice and down for the second choice.", 1);
					nokia_lcd_render();
					break;
		case intro3:
					nokia_lcd_clear();
					nokia_lcd_power(1);
					nokia_lcd_write_string("You have three lives (LEDS) to choose correctly and win the game!", 1);
					nokia_lcd_render();
					break;
		case intro4:
					nokia_lcd_clear();
					nokia_lcd_power(1);
					nokia_lcd_write_string("Clicking the joystick will reset the game to the very beginning!", 1);
					nokia_lcd_render();
					break;
		case intro5:
					nokia_lcd_clear();
					nokia_lcd_power(1);
					nokia_lcd_write_string("If you're ready simply flick up or down to begin the game!", 1);
					nokia_lcd_render();
					break;
		case gameover:
					nokia_lcd_clear();
					nokia_lcd_power(1);
					nokia_lcd_write_string("You've died!  Game Over!", 1);
					nokia_lcd_render();
					if(lives == 3)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,1);
					}
					else if (lives == 2)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 1)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 0)
					{
						PORTC = SetBit(PORTC,7,0);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
					}
					//lightmsg = 0x00;
					break;
		case one:
					nokia_lcd_clear();
					nokia_lcd_power(1);
					nokia_lcd_write_string("An angry cyclops appears! Aim for the eye? Or go for the legs?", 1);
					nokia_lcd_render();
					
					if(lives == 3)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,1);
					}
					else if (lives == 2)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 1)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 0)
					{
						PORTC = SetBit(PORTC,7,0);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
					}
					
					//lightmsg = 0x00;
					break;
		case oneA:
					nokia_lcd_clear();
					nokia_lcd_power(1);
					nokia_lcd_write_string("Blinding the cylops, you were able to defeat him!", 1);
					nokia_lcd_render();
					
					if(lives == 3)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,1);
					}
					else if (lives == 2)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 1)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 0)
					{
						PORTC = SetBit(PORTC,7,0);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
					}
					
					//lightmsg = 0x00;
					break;
		case oneB:
					nokia_lcd_clear();
					nokia_lcd_power(1);
					nokia_lcd_write_string("You get kicked in the face before you can even get close!", 1);
					nokia_lcd_render();
					
					if(lives == 3)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,1);
					}
					else if (lives == 2)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 1)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 0)
					{
						PORTC = SetBit(PORTC,7,0);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
					}
					
					//lightmsg = 0x00;
					break;
		case two:
					nokia_lcd_clear();
					nokia_lcd_power(1);
					nokia_lcd_write_string("A suspicious gold treasure chest! Open or ignore it?", 1);
					nokia_lcd_render();
					if(lives == 3)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,1);
					}
					else if (lives == 2)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 1)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 0)
					{
						PORTC = SetBit(PORTC,7,0);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
					}
					//lightmsg = 0x00;
					break;
		case twoA:
					nokia_lcd_clear();
					nokia_lcd_power(1);
					nokia_lcd_write_string("You're rich! It was filled with gleaming gold!", 1);
					nokia_lcd_render();
					if(lives == 3)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,1);
					}
					else if (lives == 2)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 1)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 0)
					{
						PORTC = SetBit(PORTC,7,0);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
					}
					//lightmsg = 0x00;
					break;
		case twoB:
					nokia_lcd_clear();
					nokia_lcd_power(1);
					nokia_lcd_write_string("You get the feeling you've made a huge mistake.", 1);
					nokia_lcd_render();
					if(lives == 3)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,1);
					}
					else if (lives == 2)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 1)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 0)
					{
						PORTC = SetBit(PORTC,7,0);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
					}
					//lightmsg = 0x00;
					break;
		case three:
					nokia_lcd_clear();
					nokia_lcd_power(1);
					nokia_lcd_write_string("A witch! Avoid her? Or ask for magical potions?", 1);
					nokia_lcd_render();
					if(lives == 3)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,1);
					}
					else if (lives == 2)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 1)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 0)
					{
						PORTC = SetBit(PORTC,7,0);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
					}
					//lightmsg = 0x00;
					break;
		case threeA:	
					nokia_lcd_clear();
					nokia_lcd_power(1);
					nokia_lcd_write_string("She doesn't take kindly to being ignore. You're cursed!", 1);
					nokia_lcd_render();
					if(lives == 3)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,1);
					}
					else if (lives == 2)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 1)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 0)
					{
						PORTC = SetBit(PORTC,7,0);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
					}
					//lightmsg = 0x00;
					break;
		case threeB:
					nokia_lcd_clear();
					nokia_lcd_power(1);
					nokia_lcd_write_string("Glowing bottles fill your pack! You're so prepared!", 1);
					nokia_lcd_render();
					if(lives == 3)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,1);
					}
					else if (lives == 2)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 1)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 0)
					{
						PORTC = SetBit(PORTC,7,0);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
					}
					//lightmsg = 0x00;
					break;
		case four:
					nokia_lcd_clear();
					nokia_lcd_power(1);
					nokia_lcd_write_string("A great dragon! Slay it or steal its treasure?", 1);
					nokia_lcd_render();
					if(lives == 3)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,1);
					}
					else if (lives == 2)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 1)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 0)
					{
						PORTC = SetBit(PORTC,7,0);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
					}
					//lightmsg = 0x00;
					break;
		case fourA:
					nokia_lcd_clear();
					nokia_lcd_power(1);
					nokia_lcd_write_string("The dragon laughs at your futile attempts.", 1);
					nokia_lcd_render();
					if(lives == 3)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,1);
					}
					else if (lives == 2)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 1)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 0)
					{
						PORTC = SetBit(PORTC,7,0);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
					}
					//lightmsg = 0x00;
					break;
		case fourB:
					nokia_lcd_clear();
					nokia_lcd_power(1);
					nokia_lcd_write_string("Somehow you actually pulled that off.", 1);
					nokia_lcd_render();
					if(lives == 3)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,1);
					}
					else if (lives == 2)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,1);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 1)
					{
						PORTC = SetBit(PORTC,7,1);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
						
					}
					else if (lives == 0)
					{
						PORTC = SetBit(PORTC,7,0);
						PORTC = SetBit(PORTC,6,0);
						PORTC = SetBit(PORTC,5,0);
					}
					//lightmsg = 0x00;
					break;
		case win:
					nokia_lcd_clear();
					nokia_lcd_power(1);
					nokia_lcd_write_string("You have won!", 1);
					nokia_lcd_render();
					//lightmsg = 0x00;
					break;
	}
}

int main(void) {
	DDRA = 0xF8; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	
	ADC_Init();
	nokia_lcd_init();
	TimerSet(100);
	TimerOn();
	
	unsigned long J_elapsed_time = 100;
	unsigned long L_elapsed_time = 500;
	unsigned long N_elasped_time = 500;
	const unsigned long timerPeriod = 100;
	
	j_state = j_start;
	l_state = L_start;
	n_state = n_start;
	while (1) 
	{
		if(J_elapsed_time >= 100)
		{
			TickFct_J(); //joystick
			J_elapsed_time = 0;
		}
		if (N_elasped_time >= 500)
		{
			TickFct_N(); //nokia lcd
			N_elasped_time = 0;
		}
		if(L_elapsed_time >= 500) // 5 seconds
		{
			TickFct_L(); //LED matrix
			L_elapsed_time = 0;
		}
		while(!TimerFlag){}
		TimerFlag = 0;
		J_elapsed_time = J_elapsed_time + timerPeriod;
		L_elapsed_time = L_elapsed_time + timerPeriod;
		N_elasped_time = N_elasped_time + timerPeriod;
		
	}
	return 1;
}