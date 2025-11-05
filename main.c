fast pwm mode 14

#define F_CPU 16000000UL

// 왼쪽 모터
#define MOTOR_LEFT_IN1  PB0
#define MOTOR_LEFT_IN2  PB1
#define MOTOR_LEFT_EN   PE3  // PWM 출력 (OC1A)

// 오른쪽 모터
#define MOTOR_RIGHT_IN1 PB2
#define MOTOR_RIGHT_IN2 PB3
#define MOTOR_RIGHT_EN  PE4  // PWM 출력 (OC1B)

// PWM 설정 (Mode 14: ICR1을 TOP으로)
#define PWM_TOP_VALUE 799    // 약 20kHz 주파수
#define HIGH_SPEED_RATIO 0.8 // 80% Duty Cycle
#define LOW_SPEED_RATIO  0.5 // 50% Duty Cycle

// OCR 값으로 변환하여 사용 (기존 main 로직과의 호환성을 위해 이름 유지)
#define high_speed (unsigned short)(PWM_TOP_VALUE * HIGH_SPEED_RATIO)
#define low_speed  (unsigned short)(PWM_TOP_VALUE * LOW_SPEED_RATIO)

#define BAUD 9600
#define MYUBRR (F_CPU/16/BAUD-1)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h> // atoi() 사용을 위해

// 전역 변수 (UART 수신 데이터)
int x=0, y=0, w=0, h=0;
char uart_buf[32];
int buf_index = 0;

// 추적 로직 상수
#define cam_w 640
#define cam_h 480
#define cam_center (cam_w/2)

#define error_range 50
#define stop_w 500
#define near_w 400


// 함수 선언
void timer_init(void);
void motor_init(void);
void motor_forward(int speed);
void motor_backward(int speed);
void motor_left(int speed);
void motor_right(int speed);
void motor_stop(void);
void uart_init(void);
void parse_data(char* str);


void parse_data(char* str)
{
	int *values[] = {&x, &y, &w, &h};
	int val_index = 0;
	char *token_start = str;

	for (int i = 0; str[i] != '\0'; i++)
	{
		if (str[i] == ',' || str[i] == '\n')
		{
			str[i] = '\0';
			
			if (val_index < 4) {
				*values[val_index] = atoi(token_start);
			}

			token_start = &str[i+1];
			val_index++;
		}
	}
}

// Timer1 Fast PWM Mode 14 (ICR1 TOP) 설정
void timer_init(void)
{
	// Mode 14: Fast PWM, TOP=ICR1, 비반전 출력 (COM1A1, COM1B1)
	TCCR1A = (1<<COM1A1) | (1<<COM1B1) | (1<<WGM11);
	TCCR1B = (1<<WGM12) | (1<<WGM13) | (1<<CS10); // 분주비 1
	
	ICR1 = PWM_TOP_VALUE; // TOP 값 설정
	OCR1A = 0;
	OCR1B = 0;
}

void motor_init()
{
	// IN 핀 출력: PB0, PB1, PB2, PB3 (DDRB 설정)
	DDRB |= (1<<MOTOR_LEFT_IN1) | (1<<MOTOR_LEFT_IN2) | (1<<MOTOR_RIGHT_IN1) | (1<<MOTOR_RIGHT_IN2);
	
	// EN 핀 PWM 출력: PE3(OC1A), PE4(OC1B) (DDRE 설정) <-- 수정된 부분
	DDRE |= (1<<MOTOR_LEFT_EN) | (1<<MOTOR_RIGHT_EN);

	motor_stop();
}

void uart_init()
{
	UBRR0H = (unsigned char)(MYUBRR>>8);
	UBRR0L = (unsigned char)MYUBRR;
	UCSR0B = (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0); // RX interrupt enable
	UCSR0C = (1<<UCSZ01) | (1<<UCSZ00); // 8N1
}

ISR(USART_RX_vect)
{
	char c = UDR0;

	if(c == '\n')
	{
		uart_buf[buf_index] = 0; // 문자열 종료
		parse_data(uart_buf);
		buf_index = 0;
	}
	else
	{
		uart_buf[buf_index++] = c;
		if(buf_index >= 31) buf_index = 31;
	}
}

// 모터 제어 함수 (Mode 14와 호환되도록 speed는 OCR 값으로 사용)
void motor_forward(int speed)
{
	PORTB |=  (1<<MOTOR_LEFT_IN1) | (1<<MOTOR_RIGHT_IN1);
	PORTB &= ~((1<<MOTOR_LEFT_IN2) | (1<<MOTOR_RIGHT_IN2));
	OCR1A = speed;
	OCR1B = speed;
}

void motor_backward(int speed)
{
	PORTB |=  (1<<MOTOR_LEFT_IN2) | (1<<MOTOR_RIGHT_IN2);
	PORTB &= ~((1<<MOTOR_LEFT_IN1) | (1<<MOTOR_RIGHT_IN1));
	OCR1A = speed;
	OCR1B = speed;
}

void motor_left(int speed)
{
	// 왼쪽 모터 정지
	PORTB &= ~((1<<MOTOR_LEFT_IN1) | (1<<MOTOR_LEFT_IN2));
	OCR1A = 0;

	// 오른쪽 모터 전진
	PORTB |= (1<<MOTOR_RIGHT_IN1);
	PORTB &= ~(1<<MOTOR_RIGHT_IN2);
	OCR1B = speed;
}

void motor_right(int speed)
{
	// 오른쪽 모터 정지
	PORTB &= ~((1<<MOTOR_RIGHT_IN1) | (1<<MOTOR_RIGHT_IN2));
	OCR1B = 0;

	// 왼쪽 모터 전진
	PORTB |= (1<<MOTOR_LEFT_IN1);
	PORTB &= ~(1<<MOTOR_LEFT_IN2);
	OCR1A = speed;
}

void motor_stop()
{
	PORTB &= ~((1<<MOTOR_LEFT_IN1) | (1<<MOTOR_LEFT_IN2) | (1<<MOTOR_RIGHT_IN1) | (1<<MOTOR_RIGHT_IN2));
	OCR1A = 0;
	OCR1B = 0;
}

int main(void)
{
	timer_init();
	motor_init();
	uart_init();
	sei(); // 전역 인터럽트 활성화

	while(1)
	{
		// 파싱된 x, w 값을 사용하여 객체 중심 위치 계산
		int obj_center = x + w/2;

		// 1. 객체가 너무 가까울 때 (w > stop_w)
		if(w > stop_w) {
			motor_backward(low_speed);
		}
		// 2. 객체가 가까울 때 (near_w < w <= stop_w)
		else if(w > near_w)
		{
			if(obj_center > cam_center + error_range) // 오른쪽으로 치우쳐져 있을 때
			motor_right(low_speed);
			else if(obj_center < cam_center - error_range) // 왼쪽으로 치우쳐져 있을 때
			motor_left(low_speed);
			else // 중앙일 때
			motor_forward(low_speed);
		}
		// 3. 객체가 멀리 있을 때 (w <= near_w)
		else
		{
			if(obj_center > cam_center + error_range)
			motor_right(high_speed);
			else if(obj_center < cam_center - error_range)
			motor_left(high_speed);
			else
			motor_forward(high_speed);
		}

		_delay_ms(10);
	}

	return 0;
}