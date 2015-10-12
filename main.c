#include <avr/io.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#include "/Users/student/Desktop/io.c"
#include "/Users/student/Desktop/timer.h"
#include "/Users/student/Desktop/seven_seg.h"

static unsigned int motion = 0;
#define inc 39
static unsigned int increment = inc;
static unsigned int in_alarm = 0;
static unsigned int out_of_way = 0;
static unsigned int light = 0;
unsigned char going = 1;
#define F_CPU 8000000UL
#define N_BER 66000
#define DIS_PORT PORTA //A3-4
#define DIS_PIN PINA
#define DIS_DDR DDRA


unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b)
{
	return (b ? x | (0x01 << k) : x & ~(0x01 << k));
}
unsigned char GetBit(unsigned char x, unsigned char k)
{
	return (( x & (0x01 << k)) != 0);
}

unsigned sensor = 0x00;
unsigned trig = 0x00;//C0, output
unsigned echo = 0x10;//A3, input
volatile long avg = 0;
volatile long user_set_time = 0;
volatile uint32_t running = 0;
volatile unsigned up = 0;
volatile uint32_t timercounter = 0;
volatile uint16_t start_time;
volatile uint16_t end_time;
volatile unsigned char set;
uint16_t duration;
int turn = 0;

uint16_t pulse()
{
	//Use trig to get the signal duration
	uint32_t i = 0;//uint32 initially
	going = 1;
	
	//while(i < N_BER)// && going)//wait for high
	for(i = 0; i < N_BER; i++)
	{
		if(GetBit(PINA, 2))//(PINA & (1 << echo)))
			break;
		else
			continue;
		i++;
	}
	
	if(i == N_BER)
	{
		return 0xFFFF;//Ends Here Somehow (change to 0xFFFF when fixed)
	}
	//Start Timer
	//TimerOn();
	callTimer();
	
	//wait for falling edge
	i = 0;
	//going = 1;
	//while(i < N_BER)// && going)//wait for low
	for(i = 0; i < N_BER; i++)
	{
		if(GetBit(PINA, 2))//(PINA & (1 << echo)))//(!GetBit(PINA, 4))//
		{
			if(TCNT1 > N_BER)
				break;
			else
				continue;
		}
		else
			break;
		i++;
	}
	
	if(i == N_BER)
	{
		return 0xFFFF;
	}
	//falling edge found
	
	//Stop Timer
	uint32_t amount = TCNT1;
	TimerOff();
	if(amount > N_BER)
		return 0xFFFE;
	else
		return (amount >> 1);

	
}
void callTimer()
{
	TimerOn();
	TCCR1A = 0x00;
}
void setup()
{
	//PORTA &= ~(1 << trig);
	PORTC |= 1<<0X00;//set trig high
	_delay_us(10);
	PORTC &= ~(1<<0x00);//set trig low
	_delay_ms(5);

}
void alarm()
{
	if(light % 2 == 0)
	{
		PORTC = SetBit(PORTC, 7, 1);
		PORTC = SetBit(PORTC, 1, 0);
		//light++;
	}
	else if(light % 2 == 1)
	{
		PORTC = SetBit(PORTC, 7, 0);
		PORTC = SetBit(PORTC, 1, 1);
		//light++;
	}
	in_alarm++;
}
int main() {
	DDRA = 0x03; PORTA = 0xFC; //input(2-7) output(0-1)
	DDRB = 0xFF; PORTB = 0x00; //output
	DDRC = 0xFF; PORTC = 0x00; //output
	DDRD = 0xFF; PORTD = 0x00; //output
	TimerSet(1000);
	sei();
	LCD_init();
	uint16_t dist;
	int convert;
	int digit1;
	int digit2;
	int digit3;
	int digit4;
	int convert1 = 0;
	light = 0;
	while(1) {
		/*
		if(convert > 50)
		{
			//motion = 0;
			PORTC = SetBit(PORTC, 7, 0);
			PORTC = SetBit(PORTC, 1, 0);
		}
		*/
		
		if(out_of_way >= 10)//seven seconds of distance >= 10
		{
			in_alarm = 0;
			motion = 0;
			increment = inc;
			light = 0;
			out_of_way = 0;
			PORTC = SetBit(PORTC, 7, 0);
			PORTC = SetBit(PORTC, 1, 0);
		}
		
		if(GetBit(PINA, 5))
		{
			motion = 1;
			PORTA = SetBit(PORTA, 2, 1);
		}
		if(motion)
		{
			if(increment > 0)
				increment--;
			else
				increment = 0;
		}
		Write7Seg(increment/10);
		if(increment == 0 && convert < 10 && convert1 == 1)
		{
			out_of_way = 0;
			alarm();
			light++;
			//call alarm for X seconds
			//have alarm set increment back to inc and motion back to 0
		}
		if(convert >= 10 && increment == 0)
		{
			out_of_way++;
		}
		
		
		//DISTANCE
		setup();
		dist = pulse();
		if(dist == 0xFFFF)//Error
		{
			convert1 = 0;
			LCD_DisplayString(1, "Error");
			//Wait();
		}
		else if(dist == 0xFFFE)//No Obstacle
		{
			convert1 = 0;
			LCD_DisplayString(1, "No Obstacle");
			//Wait();
		}
		else if(dist == 0xFFFD)
		{
			convert1 = 0;
			LCD_DisplayString(1, "Error1");
			//Wait();
		}
		else
		{
			convert = (dist/58.0);
			convert1 = 1;
			if(convert >= 1000)
			{
				digit4 = convert % 10;
				digit3 = (convert / 10) % 10;
				digit2 = (convert / 100) % 10;
				digit1 = (convert / 1000);
			}
			else if(convert >= 100)
			{
				digit4 = convert % 10;
				digit3 = (convert / 10) % 10;
				digit2 = (convert / 100);
				digit1 = 0;
			}
			else if(convert >= 10)
			{
				digit4 = convert % 10;
				digit3 = (convert / 10);
				digit2 = 0;
				digit1 = 0;			
			}
			else
			{
				digit4 = convert;
				digit3 = 0;
				digit2 = 0;
				digit1 = 0;
			}
			LCD_Cursor(1);
			LCD_WriteData('0' + digit1);
			LCD_Cursor(2);
			LCD_WriteData('0' + digit2);
			LCD_Cursor(3);
			LCD_WriteData('0' + digit3);
			LCD_Cursor(4);
			LCD_WriteData('0' + digit4);
			LCD_Cursor(5);
			LCD_WriteData('i');
			LCD_Cursor(6);
			LCD_WriteData('n');
			LCD_Cursor(0);
		}
		_delay_ms(500);
		
	}
}
