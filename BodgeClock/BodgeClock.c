/*
 * BodgeClock.c
 *
 * Created: 01/03/2015 19:37:26
 *  Author: Robert
 */ 

#define F_CPU 1000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

const char portd_segments[10] = {248, 72, 188, 220, 76, 212, 244, 200, 252, 220};
const char portb_segments[10] = {1, 0, 0, 0, 1, 1, 1, 0, 1, 1};
const char portc_digits[6] = {32, 1, 2, 4, 8, 16};

volatile uint8_t segment_counter;
volatile uint8_t temp;
volatile uint8_t in_counter;

volatile uint8_t out_numbers[6]  = {0, 0, 0, 0, 0, 0};
volatile uint8_t rx_numbers[6];
volatile uint8_t in_sleep;

int main(void)
{
	segment_counter = 0;
	in_counter = 0;
	in_sleep = 0;

	DDRB = 0x03;
	DDRC = 0xFF;
	DDRD = 0xFF;
	
	//timer 0: display refresh
	OCR0A = 20; 
	TIMSK0 = (1<<OCIE0A);
	TCCR0A = (1<<WGM01);
	TCCR0B = (1<<CS01);
	
	//timer 2: RTC
	ASSR = (1<<AS2);
	_delay_ms(500);
	TIMSK2 = 0;
	TCNT2 = 0x00;
	TCCR2A = 0x00;
	TCCR2B = (1<<CS22)|(1<<CS00);
	//while(ASSR & ((1<<TCN2UB)|(1<<TCR2BUB)));
	TIFR2  = (1<<TOV2);
	TIMSK2  = (1<<TOIE2);
	
	
	
	
	//uart
	UCSR0A = (1<<U2X0);
	UCSR0B = (1<<RXEN0) | (1<<TXEN0);
	UCSR0C = (1<<UCSZ00) | (1<<UCSZ01);
	UBRR0H = 0;
	UBRR0L = 12;
	
	PRR = (1<<PRTWI) | (1<<PRTIM1) | (1<<PRSPI) | (1<<PRADC);
	
	sei();
	
    while(1)
	{			
		if(out_numbers[5] == 10)
		{
			out_numbers[5] = 0;
			out_numbers[4]++;
				
			if(out_numbers[4] == 6)
			{
				out_numbers[4] = 0;
				out_numbers[3]++;
					
				if(out_numbers[3] == 10)	
				{
					out_numbers[3] = 0;
					out_numbers[2]++;
						
					if(out_numbers[2] == 6)
					{
						out_numbers[2] = 0;
						out_numbers[1]++;
							
						//increment 10-hours and 09++ and 19++
						if(out_numbers[1] == 10)
						{
							out_numbers[1] = 0;
							out_numbers[0]++;
						}
								
						//reset hours at 24
						if((out_numbers[0]) == 2 && (out_numbers[1] == 4))
						{
							out_numbers[0] = 0;
							out_numbers[1] = 0;
						}	
					}	
				}
			}
		}
	
		
		
		if(UCSR0A & (1<<RXC0))
		{
			temp = UDR0;
			
			while (!(UCSR0A & (1<<UDRE0)));
			UDR0 = temp;
			
			if(in_counter == 6 && temp == 'x')
			{
				cli();
				out_numbers[5] = rx_numbers[5];
				out_numbers[4] = rx_numbers[4];
				out_numbers[3] = rx_numbers[3];
				out_numbers[2] = rx_numbers[2];
				out_numbers[1] = rx_numbers[1];
				out_numbers[0] = rx_numbers[0];
				sei();
			}
			
			if(temp == 'b')
			{
				in_counter = 0;
			}
			else
			{
				if(temp >= '0' && temp <= '9')
				{
					rx_numbers[in_counter] = temp - '0';	
				}
				
				in_counter++;
			}
		}
			
		
		if(!(PINB & 0x04))
		{
			set_sleep_mode(SLEEP_MODE_PWR_SAVE);
			sleep_enable();
			
		
			if(!in_sleep)
			{	
				in_sleep = 1;
				
				//turn off display timer
				PRR |= (1<<PRTIM0);
				TIMSK0 = 0;
				
				//turn off all digits and segments
				PORTC = 0xFF;
				PORTD = 0x00;
				PORTB &= 0xFE;
			}
			
			sleep_cpu();
			sleep_disable();
			
			
		}
		else
		{
			if(in_sleep)
			{
				PRR &= ~(1<<PRTIM0);
				TIMSK0 = (1<<OCIE0A);
			}
			
			in_sleep = 0;
		}
	}

}

ISR(TIMER0_COMPA_vect)
{
	
	
	//move forward one segment
	segment_counter++;
	if(segment_counter == 6)
		segment_counter = 0;
		
	
	//turn off all digits and segments
	PORTC = 0xFF;
	PORTD = 0x00;
	PORTB &= 0xFE;
	
	//output segments for current digit
	PORTD = portd_segments[out_numbers[segment_counter]];
	if(portb_segments[out_numbers[segment_counter]])
		PORTB |= 0x01;
	else
		PORTB &= 0xFE;
		
	//enable current digit
	PORTC = ~portc_digits[segment_counter];
}

ISR(TIMER2_OVF_vect)
{
	PORTB ^= 0x02;
	out_numbers[5]++;
}