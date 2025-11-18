#define F_CPU 16000000UL

// --- C 표준 라이브러리 헤더 ---
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 왼쪽 모터 (ENA/B5, IN1/E0, IN2/E1)
#define MOTOR_LEFT_EN PB5 // OCR1A
#define MOTOR_LEFT_IN1 PE0
#define MOTOR_LEFT_IN2 PE1

// 오른쪽 모터 (ENB/B6, IN3/E2, IN4/E3)
#define MOTOR_RIGHT_EN PB6 // OCR1B
#define MOTOR_RIGHT_IN3 PE2 // IN3 (정방향)
#define MOTOR_RIGHT_IN4 PE3 // IN4 (역방향)

// --- 모터 속도 정의 (0 ~ 799) --- (수정)
#define MAX_SPEED 799   // 최대 추적 속도 (650 -> 700)
#define high_speed 750  // 빠른 속도 (600 -> 650)
#define middle_speed 700 // 적당한 속도 (550 -> 600)
#define low_speed 600   // 적정 거리 유지 속도 (450 -> 500)
#define slow_speed 550  // 후진 또는 미세조정 속도 (300 -> 350)

// --- 전역 변수 (ISR 공유) ---
volatile int x=0, y=0, w=0, h=0;
char uart_buf[32];
volatile int buf_index = 0;

// --- 추적 로직 상수 (카메라 폭 기준: 640) ---
#define cam_w 640
#define cam_h 480
#define cam_center (cam_w/2)

// --- 객체 폭 (w) 기반 거리 제어 구역 (수정) ---
#define BACK_W 480          // w > 480 : 너무 가까움 -> 후진 (380 -> 480)
#define HOVER_STOP_W 400    // 400 < w <= 480 : 정지/미세조정 (300 -> 400)
#define HOVER_W 350         // 350 < w <= 400 : 적당한 속도 (250 -> 350)
#define FOLLOW_W 250        // 250 < w <= 350 : 빠른 속도 (180 -> 250)

// --- 직진성 강화를 위한 새로운 상수 (수정) ---
#define ERROR_DEADBAND 40 // 픽셀 오차 40 이내면 직진 (35 -> 40)

// --- 비례 제어 상수 (수정) ---
#define KP_ROTATION 2.0 // 조향 민감도 약간 낮춤 (2.2 -> 2.0)

// 함수 선언 (동일)
void timer_init(void);
void motor_init(void);
void uart_init(void);
void parse_data(char* str);
void motor_forward(int speed);
void motor_backward(int speed);
void motor_set_diff_speed(int left_speed, int right_speed);
void motor_stop(void);

// ... (timer_init, motor_init, uart_init, parse_data, ISR, motor 제어 함수들은 생략, 내용은 동일) ...

void timer_init(void)
{
	TCCR1A = (1<<COM1A1) | (1<<COM1B1) | (1<<WGM11);
	TCCR1B = (1<<WGM12) | (1<<WGM13) | (1<<CS10);
	ICR1 = 799;
	OCR1A = 0;
	OCR1B = 0;
}

void motor_init()
{
	DDRE |= (1<<MOTOR_LEFT_IN1) | (1<<MOTOR_LEFT_IN2) | (1<<MOTOR_RIGHT_IN3) | (1<<MOTOR_RIGHT_IN4);
	DDRB |= (1<<MOTOR_LEFT_EN) | (1<<MOTOR_RIGHT_EN);
	motor_stop();
}

void uart_init()
{
	UCSR1A = 0x00;
	UCSR1B = (1<<RXEN1) | (1<<TXEN1) | (1<<RXCIE1);
	UCSR1C = (1<<UCSZ11) | (1<<UCSZ10);
	UBRR1H = 0;
	UBRR1L = 103;
}

void parse_data(char* str)
{
	int *target_vars[] = {(int*)&x, (int*)&y, (int*)&w, (int*)&h};
	int val_index = 0;
	char *token_start = str;
	char *comma_pos;

	for (val_index = 0; val_index < 4; val_index++)
	{
		comma_pos = strchr(token_start, ',');

		if (comma_pos != NULL)
		{
			*comma_pos = '\0';
			*target_vars[val_index] = atoi(token_start);
			token_start = comma_pos + 1;
		}
		else
		{
			*target_vars[val_index] = atoi(token_start);
			break;
		}
	}
}

ISR(USART1_RX_vect)
{
	char c = UDR1;
	int current_buf_index = buf_index;

	if(c == '\n')
	{
		cli();
		uart_buf[current_buf_index] = 0;
		parse_data(uart_buf);
		buf_index = 0;
		sei();
	}
	else
	{
		if(current_buf_index < 31)
		{
			uart_buf[current_buf_index] = c;
			buf_index = current_buf_index + 1;
		}
		else {
			buf_index = 0;
		}
	}
}

void motor_forward(int speed)
{
	PORTE = (1<<MOTOR_LEFT_IN1) | (1<<MOTOR_RIGHT_IN3);
	OCR1A = speed;
	OCR1B = speed;
}

void motor_backward(int speed)
{
	PORTE = (1<<MOTOR_LEFT_IN2) | (1<<MOTOR_RIGHT_IN4);
	OCR1A = speed;
	OCR1B = speed;
}

void motor_set_diff_speed(int left_speed, int right_speed)
{
	PORTE = (1<<MOTOR_LEFT_IN1) | (1<<MOTOR_RIGHT_IN3);
	OCR1A = left_speed;
	OCR1B = right_speed;
}

void motor_stop()
{
	PORTE &= ~((1<<MOTOR_LEFT_IN1) | (1<<MOTOR_LEFT_IN2) | (1<<MOTOR_RIGHT_IN3) | (1<<MOTOR_RIGHT_IN4));
	OCR1A = 0;
	OCR1B = 0;
}


// --- 메인 추적 로직 함수 ---
void tracking_logic(int local_x, int local_w) {

	int obj_center = local_x + local_w / 2;
	int base_speed = 0;
	int error = obj_center - cam_center;
	int rotation_speed_adj;

	// 1. 거리 제어 (객체 폭 W 기준) -> base_speed 결정
	if (local_w > BACK_W) // W > 480: 후진 (요청에 따라 W 480 기준 설정)
	{
		motor_backward(slow_speed);
		return;
	}
	else if (local_w > HOVER_STOP_W) // 400 < W <= 480: 미세조정 (정지-후진 사이)
	{
		base_speed = slow_speed / 2; // 초저속 전진
	}
	else if (local_w > HOVER_W) // 350 < W <= 400: 적당한 속도
	{
		base_speed = middle_speed; // 600
	}
	else if (local_w > FOLLOW_W) // 250 < W <= 350: 빠른 속도
	{
		base_speed = high_speed; // 650
	}
	else // W <= 250: 빠른 속도 유지
	{
		base_speed = high_speed; // 650
	}


	// 2. 방향 제어 (객체 중심 X 기준)
	
	if (abs(error) <= ERROR_DEADBAND) // 오차 40 픽셀 이내일 때
	{
		motor_forward(base_speed); // 직진 수행
		return;
	}
	
	// 객체가 중앙 불감대 밖에 있을 경우: 비례 제어 (P-Control)
	rotation_speed_adj = (int)((float)error * KP_ROTATION); // KP 2.0 적용

	int left_speed = base_speed + rotation_speed_adj;
	int right_speed = base_speed - rotation_speed_adj;

	// 속도 제한 (0 ~ MAX_SPEED 범위)
	if (left_speed > MAX_SPEED) left_speed = MAX_SPEED;
	if (left_speed < 0) left_speed = 0;
	if (right_speed > MAX_SPEED) right_speed = MAX_SPEED;
	if (right_speed < 0) right_speed = 0;


	// 3. 모터 구동
	if (left_speed == 0 && right_speed == 0)
	{
		motor_stop();
	}
	else
	{
		motor_set_diff_speed(left_speed, right_speed); // 차동 구동
	}
}


int main(void)
{
	timer_init();
	motor_init();
	uart_init();
	sei(); // 전역 인터럽트 활성화

	int local_x, local_w;

	while(1)
	{
		cli();
		local_x = x;
		local_w = w;
		sei();

		if (local_w == 0)
		{
			motor_stop();
		}
		else
		{
			tracking_logic(local_x, local_w);
		}

		_delay_ms(10);
	}

	return 0;
}