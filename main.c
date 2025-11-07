#define F_CPU 16000000UL

// 왼쪽 모터 (ENA/B5, IN1/E0, IN2/E1)
#define MOTOR_LEFT_EN   PB5  // OCR1A
#define MOTOR_LEFT_IN1  PE0
#define MOTOR_LEFT_IN2  PE1

// 오른쪽 모터 (ENB/B6, IN3/E2, IN4/E3)
#define MOTOR_RIGHT_EN  PB6  // OCR1B
#define MOTOR_RIGHT_IN3 PE2  // IN3 (정방향)
#define MOTOR_RIGHT_IN4 PE3  // IN4 (역방향)

#define high_speed 500
#define low_speed  300 // 799

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h> // atoi() 사용을 위해

// [수정 1] ISR과 공유하는 전역 변수에 volatile 키워드 추가
volatile int x=0, y=0, w=0, h=0;
char uart_buf[32];
volatile int buf_index = 0; // ISR에서 수정되므로 volatile 추가

// 추적 로직 상수
#define cam_w 640
#define cam_h 480
#define cam_center (cam_w/2)

#define error_range 50
#define stop_w 500 // 640
#define near_w 400

// 함수 선언
void timer_init(void);
void motor_init(void);
void uart_init(void);
void parse_data(char* str);
void motor_forward(int speed);
void motor_backward(int speed);
void motor_left_turn(int speed);
void motor_right_turn(int speed);
void motor_stop(void);

// Timer1 Fast PWM Mode 14 (ICR1 TOP) 설정
void timer_init(void)
{
	// Mode 14: Fast PWM, TOP=ICR1, 비반전 출력 (COM1A1, COM1B1)
	TCCR1A = (1<<COM1A1) | (1<<COM1B1) | (1<<WGM11);
	TCCR1B = (1<<WGM12) | (1<<WGM13) | (1<<CS10); // 분주비 1
	
	ICR1 = 799; // TOP 값 설정
	OCR1A = 0;
	OCR1B = 0;
}

void motor_init()
{
	// IN 핀 출력: PE0, PE1, PE2, PE3 (DDRE 설정)
	DDRE |= (1<<MOTOR_LEFT_IN1) | (1<<MOTOR_LEFT_IN2) | (1<<MOTOR_RIGHT_IN3) | (1<<MOTOR_RIGHT_IN4);
	
	// EN 핀 PWM 출력: PB5(OC1A), PB6(OC1B) (DDRB 설정)
	DDRB |= (1<<MOTOR_LEFT_EN) | (1<<MOTOR_RIGHT_EN);

	motor_stop();
}

// [수정] UART 핀 (PD2=RXD1, PD3=TXD1)
void uart_init()
{
	// USART1을 사용하도록 모든 레지스터 변경 (UCSR0->UCSR1 등)
	UCSR1A = 0x00;
	UCSR1B = (1<<RXEN1) | (1<<TXEN1) | (1<<RXCIE1); //enable
	UCSR1C = (1<<UCSZ11) | (1<<UCSZ10); // 8bit
	UBRR1H = 0;
	UBRR1L = 103; //9600
}


void parse_data(char* str)
{
	// 'volatile' 경고 해결
	volatile int *values[] = {&x, &y, &w, &h};
	int val_index = 0;
	char *token_start = str;

	// 0,0,0,0 수신 대비
	if (str[0] == '\0') {
		x = 0; y = 0; w = 0; h = 0;
		return;
	}

	for (int i = 0; ; i++)
	{
		if (str[i] == ',' || str[i] == '\0' || str[i] == '\n')
		{
			char temp_char = str[i];
			str[i] = '\0';
			
			if (val_index < 4) {
				*values[val_index] = atoi(token_start);
			}

			token_start = &str[i+1];
			val_index++;

			if (temp_char == '\0' || temp_char == '\n' || val_index >= 4) {
				break;
			}
		}
	}
}


// [수정] ISR 벡터 이름을 USART1로 변경, UDR1 사용
ISR(USART1_RX_vect)
{
	char c = UDR1; // UDR0 -> UDR1
	int current_buf_index = buf_index; // volatile 변수는 지역 변수로 복사해서 사용

	if(c == '\n')
	{
		uart_buf[current_buf_index] = 0; // 문자열 종료
		parse_data(uart_buf);
		buf_index = 0; // 인덱스 리셋
	}
	else
	{
		if(current_buf_index < 31) // 버퍼 오버플로우 방지
		{
			uart_buf[current_buf_index] = c;
			buf_index = current_buf_index + 1; // volatile 변수 갱신
		}
		else {
			buf_index = 0;  // 버퍼 리셋
		}
	}
}

// 모터 제어 함수 (PORTE 사용)
void motor_forward(int speed)
{
	// 왼쪽: IN1(E0)=1, IN2(E1)=0 | 오른쪽: IN3(E2)=1, IN4(E3)=0
	PORTE = (1<<MOTOR_LEFT_IN1) | (1<<MOTOR_RIGHT_IN3);
	OCR1A = speed;
	OCR1B = speed;
}

void motor_backward(int speed)
{
	// 왼쪽: IN1(E0)=0, IN2(E1)=1 | 오른쪽: IN3(E2)=0, IN4(E3)=1
	PORTE = (1<<MOTOR_LEFT_IN2) | (1<<MOTOR_RIGHT_IN4);
	OCR1A = speed;
	OCR1B = speed;
}

// 좌회전 (왼쪽 정지, 오른쪽 전진)
void motor_left_turn(int speed)
{
	// 왼쪽: 정지 | 오른쪽: IN3(E2)=1
	PORTE = (1<<MOTOR_RIGHT_IN3);
	OCR1A = 0;     // 왼쪽 PWM (정지)
	OCR1B = speed; // 오른쪽 PWM (전진)
}

// 우회전 (오른쪽 정지, 왼쪽 전진)
void motor_right_turn(int speed)
{
	// 왼쪽: IN1(E0)=1 | 오른쪽: 정지
	PORTE = (1<<MOTOR_LEFT_IN1);
	OCR1A = speed; // 왼쪽 PWM (전진)
	OCR1B = 0;     // 오른쪽 PWM (정지)
}

void motor_stop()
{
	// 모든 IN 핀(PE0~PE3) 0
	PORTE &= ~((1<<MOTOR_LEFT_IN1) | (1<<MOTOR_LEFT_IN2) | (1<<MOTOR_RIGHT_IN3) | (1<<MOTOR_RIGHT_IN4));
	// 모든 PWM 0
	OCR1A = 0;
	OCR1B = 0;
}

int main(void)
{
	timer_init();
	motor_init();
	uart_init();
	sei(); // 전역 인터럽트 활성화

	int local_x, local_w;
	int obj_center;

	while(1)
	{
		// 16비트 변수 안전하게 읽기 (Atomic Access)
		cli(); // 인터럽트 끄기
		local_x = x;
		local_w = w;
		sei(); // 인터럽트 켜기

		// 객체 미감지 (w=0) 시 정지
		if (local_w == 0)
		{
			motor_stop();
		}
		else
		{
			// 파싱된 x, w 값을 사용하여 객체 중심 위치 계산
			obj_center = local_x + local_w / 2;

			// 1. 객체가 너무 가까울 때 (w > stop_w)
			if(local_w > stop_w) {
				motor_backward(low_speed);
			}
			// 2. 객체가 가까울 때 (near_w < w <= stop_w)
			else if(local_w > near_w)
			{
				if(obj_center > cam_center + error_range) // 오른쪽으로 치우쳐져 있을 때
				motor_right_turn(low_speed);
				
				else if(obj_center < cam_center - error_range) // 왼쪽으로 치우쳐져 있을 때
				motor_left_turn(low_speed);
				
				else // 중앙일 때
				motor_forward(low_speed);
			}
			// 3. 객체가 멀리 있을 때 (w <= near_w)
			else
			{
				if(obj_center > cam_center + error_range)
				motor_right_turn(high_speed);
				
				else if(obj_center < cam_center - error_range)
				motor_left_turn(high_speed);
				
				else
				motor_forward(high_speed);
			}
		}

		_delay_ms(10);
	}

	return 0;
}