/*
 * motor basic.c
 *
 * Created: 2025-11-02 오후 5:24:39
 * Author : jinny
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>


int main(void) {
	DDRB = 0xff;
	DDRE = 0xff;
	PORTE = 0b00000101;
	
	TCCR1A = (1<<COM1A1)|(0<<COM1A0)|(0<<WGM10)|(1<<WGM11)|(1<<COM1B1)|(0<<COM1B0);
	TCCR1B = (1<<WGM12)|(1<<WGM13)|(1<<CS10);
	
	ICR1 = 799;//새로운top
	//OCR1A = ICR1*0.9;//새로운top기준으로pwm 설정(지금 90%)
	//OCR1B = ICR1*0.9;
	int i=1;//%수치 변경할때 횟수 카운팅해줄 변수
	int j=0;//%수치 변경할떄 곱할 변수
	while(1) {
		if(i<11) {//10->100
			PORTE = 0b00000101;
			j++;
			OCR1A=ICR1*(j*0.1);
			OCR1B=ICR1*(j*0.1);
			_delay_ms(100);//0.1초
			i++;
		}
		if(i>=11&&i<21) {//100->0
			PORTE = 0b00000101;
			j--;
			OCR1A=ICR1*(j*0.1);
			OCR1B=ICR1*(j*0.1);
			_delay_ms(100);
			i++;
		}
		if(i>=21&&i<31) {//0->-100
			PORTE = 0b00001010;
			j--;
			OCR1A=ICR1*(j*(-0.1));
			OCR1B=ICR1*(j*(-0.1));
			_delay_ms(100);
			i++;
		}
		if(i>=31&&i<41) {//-100->0
			PORTE = 0b00001010;
			j++;
			OCR1A=ICR1*(j*(-0.1));
			OCR1B=ICR1*(j*(-0.1));
			_delay_ms(100);
			i++;
		}
		if(i==41) {//초기화
			i=1;
			j=0;
		}
	}
}

