/*
 * ㅊㅊ.c
 *
 * Created: 2025-09-21 오후 5:12:37
 * Author : jinny
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//시리얼통신할때 필요하다고함
#define BAUD 9600 //이건 아마도 통신 속도 아닐까 
#define MYUBRR (F_CPU/16/BAUD-1)

//모터 핀.. 어쩌고
#define MOTOR_LEFT_IN1  PB5
#define MOTOR_LEFT_IN2  PB6
#define MOTOR_LEFT_EN   PE3

#define MOTOR_RIGHT_IN1 PB2
#define MOTOR_RIGHT_IN2 PB3
#define MOTOR_RIGHT_EN  PE4

//카메라 픽셀 폭..이랑 높이
#define CAM_W 640
#define CAM_H 480
#define CAM_CENTER (CAM_W / 2)

//이거 값 나중에 정해야댐 (지금은 그냥 넣어놨음)
#define speed_high    200  // 높은 속도
#define speed_low     100  // 낮은 속도
#define speed_stop    0    // 정지

#define approach_w    400  // 물체가 이 폭보다 크면 접근한 것으로 판단 (낮은 속도)
#define stop_w        550  // 물체가 이 폭보다 크면 정지 (너무 가까움)
#define center_range  30  // 중심에서 이 값의 픽셀만큼 벗어나면 방향을 유지하는 최소한의 픽셀

//변수 설정
int x=0, y=0, w=0, h=0;

//함수여러개
void uart_init(void);
void uart_putch(char data);
char uart_getch(void);
void motor_init(void);
void left_motor(uint8_t direction, uint8_t speed);
void right_motor(uint8_t direction, uint8_t speed);
void motor_forward(uint8_t speed);
void motor_backward(uint8_t speed);
void motor_stop(void);
void read_serial(void);

void uart_init(void) //아니 이부분이 도대체 뭔지 모르겠음. 일단 필요하다하니..
{
	// Set Baud rate
	UBRR0H = (unsigned char)(MYUBRR >> 8);
	UBRR0L = (unsigned char)MYUBRR;
	
	// Enable receiver and transmitter
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	
	// Set frame format: 8 data bits, 1 stop bit
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

char uart_getch(void) //이것도 머임
{
	while (!(UCSR0A & (1 << RXC0)));
	return UDR0;
}

void motor_init(void)
{
	DDRB |= (1<< MOTOR_LEFT_IN1) | (1<<MOTOR_LEFT_IN2) | (1<<MOTOR_RIGHT_IN1) | (1<< MOTOR_RIGHT_IN2);
	DDRE |= (1<<MOTOR_LEFT_EN) | (1<<MOTOR_RIGHT_EN);
	
	TCCR3A = (1<<COM3A1) | (1<< COM3B1) | (1<<WGM30);
	TCCR3B = (1<<WGM32) | (1<<CS31); // 분주비 8. 음 이것도 나중에 pwm 전송 주기 설정하고 계산해서 분주비 수정해야 할 듯
	
	// 초기의 모터 상태는 멈춰있는걸로
	PORTB &= ~((1<<MOTOR_LEFT_IN1) | (1<<MOTOR_LEFT_IN2) | (1<< MOTOR_RIGHT_IN1) | (1<< MOTOR_RIGHT_IN2));
	OCR3A = 0; // 왼쪽
	OCR3B = 0; // 오른쪽
}


void left_motor(uint8_t direction, uint8_t speed)
{
	if (direction == 1) // 앞으로
	{
		PORTB |= (1<< MOTOR_LEFT_IN1);
		PORTB &= ~(1<<MOTOR_LEFT_IN2);
	}
	else // 뒤로
	{
		PORTB &= ~(1<<MOTOR_LEFT_IN1);
		PORTB |= (1<< MOTOR_LEFT_IN2);
	}
	OCR3A = speed;
}

void right_motor(uint8_t direction, uint8_t speed)
{
	if (direction == 1) // 앞으로
	{
		PORTB |= (1 << MOTOR_RIGHT_IN1);
		PORTB &= ~(1 << MOTOR_RIGHT_IN2);
	}
	else // 뒤로
	{
		PORTB &= ~(1 << MOTOR_RIGHT_IN1);
		PORTB |= (1 << MOTOR_RIGHT_IN2);
	}
	OCR3B = speed;
}

void motor_forward(uint8_t speed)
{
	left_motor(1, speed);
	right_motor(1, speed);
}

void motor_backward(uint8_t speed)
{
	left_motor(0, speed);
	right_motor(0, speed);
}

void motor_stop(void)
{
	OCR3A = 0;
	OCR3B =0;
	PORTB &= ~((1 << MOTOR_LEFT_IN1) | (1 << MOTOR_LEFT_IN2) |
	(1 << MOTOR_RIGHT_IN1) | (1 << MOTOR_RIGHT_IN2));
}

void read_serial(void) //진지하게 시리얼 통신 부분 아무것도 모르겠음.
{
	static char buffer[32];
	static int index = 0;
	
	char c = uart_getch(); // 한 글자 받기

	// 줄바꿈 문자가 오면 한 줄이 완성된 것
	if (c == '\n' || c == '\r')
	{
		buffer[index] = '\0'; // 문자열 끝 표시
		
		// 쉼표(,)를 기준으로 문자열 자르기
		char *token1 = strtok(buffer, ",");
		char *token2 = strtok(NULL, ",");
		char *token3 = strtok(NULL, ",");
		char *token4 = strtok(NULL, ",");
		
		// 모든 토큰이 유효한지 확인하고 정수로 변환
		if (token1 && token2 && token3 && token4)
		{
			x = atoi(token1);
			y = atoi(token2);
			w = atoi(token3);
			h = atoi(token4);
		}
		
		index = 0; // 다음 메시지를 위해 인덱스 초기화
	}
	else if (index < 31) // 버퍼 오버플로우 방지
	{
		buffer[index] = c;
		index++;
	}
}

int main(void)
{
	uart_init();
	motor_init();
	
	while(1)
	{
		// 시리얼 통신으로 데이터 읽기
		read_serial();
		
		// 물체의 중심 x 좌표 계산(왜 여기서 x+w인거지?)
		int object_center_x = x + w / 2;
		
		if (w > stop_w)
		{
			// 물체가 너무 가까운 경우
			// motor_stop();
			// 음 멈추는 걸로 할 지 후진으로 할 지 모르겠음.
			motor_backward(speed_low);
		}
		else if (w > approach_w) // 물체에 접근 중 (저속)
		{
			// 물체가 적당히 가까운 경우, 방향 제어
			if (object_center_x > CAM_CENTER + center_range) // 중심에서 오른쪽으로 벗어나면
			{
				// 우회전: 오른쪽 모터는 저속 후진, 왼쪽 모터는 저속 전진
				left_motor(1, speed_low);
				right_motor(0, 50); // 약간 후진하며 회전
			}
			else if (object_center_x < CAM_CENTER - center_range) // 중심에서 왼쪽으로 벗어나면
			{
				// 좌회전: 왼쪽 모터는 저속 후진, 오른쪽 모터는 저속 전진
				// 저속 후진이 맞을까 아니면 저속 전진이 맞을까
				left_motor(0, 50); // 약간 후진하며 회전
				right_motor(1, speed_low);
			}
			else // 물체가 중심에 있으면
			{
				// 저속으로 직진하여 천천히 접근
				motor_forward(speed_low);
			}
		}
		else // 물체가 멀리 있으면 빠르게 달리자
		{
			if (object_center_x > CAM_CENTER + center_range) // 중심에서 오른쪽으로 벗어나면
			{
				// 우회전: 왼쪽 모터를 고속 전진, 오른쪽 모터를 저속 전진
				// 이건 이게 맞는듯
				left_motor(1, speed_high);
				right_motor(1, speed_low);
			}
			else if (object_center_x < CAM_CENTER - center_range) // 중심에서 왼쪽으로 벗어나면
			{
				// 좌회전: 왼쪽 모터를 저속 전진, 오른쪽 모터를 고속 전진
				left_motor(1, speed_low);
				right_motor(1, speed_high);
			}
			else // 물체가 중심에 있으면
			{
				// 고속으로 직진하여 빠르게 접근
				motor_forward(speed_high);
			}
		}
		_delay_ms(10); // 딜레이 시간도 나중에 조절하면 될듯
	}
	return 0;
}

