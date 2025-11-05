#define F_CPU 16000000UL

#define speed 500

// 모터 드라이버 핀 설정
// 왼쪽 모터 (ENA/B5, IN1/E0, IN2/E1)
#define MOTOR_LEFT_EN   PB5  // OCR1A
#define MOTOR_LEFT_IN1  PE0
#define MOTOR_LEFT_IN2  PE1

// 오른쪽 모터 (ENB/B6, IN3/E2, IN4/E3)
#define MOTOR_RIGHT_EN  PB6  // OCR1B
#define MOTOR_RIGHT_IN3 PE2  // IN3 (정방향)
#define MOTOR_RIGHT_IN4 PE3  // IN4 (역방향)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// 모터 정지
void motor_stop() {
	// 방향 핀 모두 0
	PORTE &= ~((1<<MOTOR_LEFT_IN1) | (1<<MOTOR_LEFT_IN2) | (1<<MOTOR_RIGHT_IN3) | (1<<MOTOR_RIGHT_IN4));
	// PWM 모두 0
	OCR1A = 0;
	OCR1B = 0;
}

// 전진 (양쪽 모터 정방향)
void motor_forward(unsigned short speed) {
	// 왼쪽: IN1(E0)=1, IN2(E1)=0 | 오른쪽: IN3(E2)=1, IN4(E3)=0
	PORTE = (1<<MOTOR_LEFT_IN1) | (1<<MOTOR_RIGHT_IN3); // 0x05
	OCR1A = speed; // ENA/B5
	OCR1B = speed; // ENB/B6
}

// 후진 (양쪽 모터 역방향)
void motor_backward(unsigned short speed) {
	// 왼쪽: IN1(E0)=0, IN2(E1)=1 | 오른쪽: IN3(E2)=0, IN4(E3)=1
	PORTE = (1<<MOTOR_LEFT_IN2) | (1<<MOTOR_RIGHT_IN4); // 0x0A
	OCR1A = speed;
	OCR1B = speed;
}

// 좌회전 (왼쪽 정지, 오른쪽 전진)
void motor_left_turn(unsigned short speed) {
	// 왼쪽: 정지 | 오른쪽: IN3(E2)=1
	PORTE = (1<<MOTOR_RIGHT_IN3); // 0x04
	OCR1A = 0;      // 왼쪽 PWM (정지)
	OCR1B = speed;  // 오른쪽 PWM (전진)
}

// 우회전 (오른쪽 정지, 왼쪽 전진)
void motor_right_turn(unsigned short speed) {
	// 왼쪽: IN1(E0)=1 | 오른쪽: 정지
	PORTE = (1<<MOTOR_LEFT_IN1); // 0x01
	OCR1A = speed;  // 왼쪽 PWM (전진)
	OCR1B = 0;      // 오른쪽 PWM (정지)
}

// 타이머 초기화 (Fast PWM Mode 14, TOP=ICR1=799, 분주비 1)
void TIMER_init(void) {
	TCCR1A = (1<<COM1A1)|(1<<COM1B1)|(0<<WGM10)|(1<<WGM11);
	TCCR1B = (1<<WGM12)|(1<<WGM13)|(1<<CS10);
	ICR1 = 799;
	OCR1A = 0;
	OCR1B = 0;
}


int main(void)
{
	// 핀 설정
	DDRE = (1<<MOTOR_LEFT_IN1) | (1<<MOTOR_LEFT_IN2) | (1<<MOTOR_RIGHT_IN3) | (1<<MOTOR_RIGHT_IN4); // PE0~PE3 출력
	DDRB = (1<<MOTOR_LEFT_EN) | (1<<MOTOR_RIGHT_EN); // PB5(OC1A), PB6(OC1B) PWM 출력

	TIMER_init(); // 타이머/PWM 초기화

	while (1) {
		// 1. 전진 (2초)
		motor_forward(speed);
		_delay_ms(2000);

		// 2. 좌회전 (1초)
		motor_left_turn(speed);
		_delay_ms(1000);

		// 3. 우회전 (1초)
		motor_right_turn(speed);
		_delay_ms(1000);

		// 4. 후진 (2초)
		motor_backward(speed);
		_delay_ms(2000);

		// 5. 정지 (1초)
		motor_stop();
		_delay_ms(1000);
	}
}