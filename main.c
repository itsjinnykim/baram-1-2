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
#define CAM_W 640 // 폭
#define CAM_H 480 // 높이 
#define CAM_CENTER (CAM_W / 2) // 카메라 중앙 정의 

//이거 값 나중에 정해야댐 (지금은 그냥 넣어놨음)
#define speed_high    300  // 높은 속도
#define speed_low     00  // 낮은 속도
#define speed_stop    0    // 정지

#define approach_w    500  // 물체가 이 폭보다 크면 접근한 것으로 판단 (낮은 속도)
#define stop_w        600  // 물체가 이 폭보다 크면 정지 (너무 가까움)
#define center_range  20  // 중심에서 이 값의 픽셀만큼 벗어나면 방향을 유지하는 최소한의 픽셀.. 이것도 나중에 정해야겠다 돌려보면서 
 
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

void uart_init(void) //아니 이부분이 도대체 뭔지 모르겠음. 일단 필요하다하니.. 아마 유아트 초기화 아닐까 
{
	// Set Baud rate
	UBRR0H = (unsigned char)(MYUBRR >> 8);
	UBRR0L = (unsigned char)MYUBRR;
	
	// Enable receiver and transmitter
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	
	// Set frame format: 8 data bits, 1 stop bit
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

char uart_getch(void) //이거는 데이터 받는거 
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

void read_serial(void)
{
	static char buffer[32];
	static int index = 0;
	
	char c = uart_getch(); // 한 글자 받기

	// 줄바꿈 문자가 오면 한 줄 완성
	if (c == '\n' || c == '\r')
	{
		buffer[index] = '\0'; // 문자열 끝 표시
		
		// 쉼표(,)를 기준으로 문자열 자르기
		char *token1 = strtok(buffer, ",");
		char *token2 = strtok(NULL, ",");
		char *token3 = strtok(NULL, ",");
		char *token4 = strtok(NULL, ",");
		
		// 모든 토큰이 유효한지 확인하고 정수로 변환하여 전역 변수에 저장
		if (token1 && token2 && token3 && token4)
		{
			x = atoi(token1); // x 좌표
			y = atoi(token2); // y 좌표
			w = atoi(token3); // 폭 (Width)
			h = atoi(token4); // 높이 (Height)
			} else {
			// 파싱 실패 시 변수들을 0으로 초기화하여 미감지 상태로 간주
			x=0; y=0; w=0; h=0;
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
	uart_init(); // 시리얼 통신 초기화
	motor_init(); // 모터 및 PWM 초기화
	
	while(1)
	{
		// 시리얼 통신으로 데이터 읽기
		read_serial();
		
		// w (물체 폭)가 0보다 클 때만 객체 추적 로직 실행 (객체 감지)
		if (w > 0)
		{
			// 물체의 중심 x 좌표 계산 (x + w/2)
			int object_center_x = x + w / 2;
			
			// === 1. 너무 가까우면 정지 조건 (w > stop_w) ===
			if (w > stop_w)
			{
				// 물체가 너무 가까우면 정지
				motor_stop();
			}
			// === 2. 적당히 가까우면 저속 추적 주행 조건 (approach_w < w <= stop_w) ===
			else if (w > approach_w)
			{
				// 저속 추적
				if (object_center_x > CAM_CENTER + center_range) // 중심에서 오른쪽으로 벗어나면
				{
					// 우회전: 왼쪽 모터 전진, 오른쪽 모터 속도 줄여 전진 (선회)
					left_motor(1, speed_low);
					right_motor(1, speed_low / 2); // 오른쪽 모터는 더 느리게
				}
				else if (object_center_x < CAM_CENTER - center_range) // 중심에서 왼쪽으로 벗어나면
				{
					// 좌회전: 오른쪽 모터 전진, 왼쪽 모터 속도 줄여 전진 (선회)
					left_motor(1, speed_low / 2); // 왼쪽 모터는 더 느리게
					right_motor(1, speed_low);
				}
				else // 물체가 중심 범위 안에 있으면
				{
					// 저속으로 직진하여 천천히 접근
					motor_forward(speed_low);
				}
			}
			// === 3. 물체가 멀리 있으면 고속 추적 주행 조건 (w <= approach_w) ===
			else
			{
				// 고속 추적
				if (object_center_x > CAM_CENTER + center_range) // 중심에서 오른쪽으로 벗어나면
				{
					// 우회전: 왼쪽 모터를 고속 전진, 오른쪽 모터를 저속 전진
					left_motor(1, speed_high);
					right_motor(1, speed_low);
					
				}
				else if (object_center_x < CAM_CENTER - center_range) // 중심에서 왼쪽으로 벗어나면
				{
					// 좌회전: 왼쪽 모터를 저속 전진, 오른쪽 모터를 고속 전진
					left_motor(1, speed_low);
					right_motor(1, speed_high);
				}
				else // 물체가 중심 범위 안에 있으면
				{
					// 고속으로 직진하여 빠르게 접근
					motor_forward(speed_high);
				}
			}
		}
		// === 4. 객체 미감지 조건 (w <= 0): 저속으로 선회하며 객체 찾기 (수정됨) ===
		else
		{
			// 객체 정보가 없거나 유효하지 않으면 정지하지 않고 저속으로 우회전하며 선회
			// 우회전: 왼쪽 모터 전진(speed_low), 오른쪽 모터 속도 줄여 전진(speed_low / 2)
			left_motor(1, speed_low);
			right_motor(1, speed_low / 2);
		}
		
		_delay_ms(10); // 제어 주기 조절 (10ms)
	}
	return 0;
}