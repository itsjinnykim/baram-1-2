#include <avr/io.h>
#include <util/delay.h>
#include <HardwareSerial.h> // Arduino의 Serial 통신을 위해 필요

// 웹캠 해상도 및 화면 중심 정의
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define SCREEN_CENTER_X (SCREEN_WIDTH / 2)

// 모터 드라이버 핀 정의 (L298N)
#define MOTOR_LEFT_IN1  PB5
#define MOTOR_LEFT_IN2  PB6
#define MOTOR_LEFT_EN   PE3  // PWM 핀 (OCR3A)

#define MOTOR_RIGHT_IN1 PB2
#define MOTOR_RIGHT_IN2 PB3
#define MOTOR_RIGHT_EN  PE4  // PWM 핀 (OCR3B)

// 모터 속도 정의 (0-255)
#define SPEED_HIGH 200
#define SPEED_LOW  100
#define SPEED_STOP 0

// 물체 감지를 위한 임계값 (실제 환경에 맞게 조정 필요)
#define THRESHOLD_APPROACH_WIDTH 250 // 물체의 폭이 이 값보다 크면 속도 조절
#define THRESHOLD_STOP_WIDTH     400 // 물체의 폭이 이 값보다 크면 정지
#define THRESHOLD_CENTER_RANGE   30  // 중심에서 벗어난 허용 오차 (좌우 +- 30픽셀)

// 시리얼 데이터 수신 변수 (파이썬에서 보낸 x,y,w,h)
int x = 0, y = 0, w = 0, h = 0;

// 함수 프로토타입 (메인 함수 전에 선언)
void motor_init(void);
void left_motor(uint8_t direction, uint8_t speed);
void right_motor(uint8_t direction, uint8_t speed);
void motors_forward(uint8_t speed);
void motors_backward(uint8_t speed);
void motors_stop(void);
void serial_read_data(void);

// 메인 함수
int main(void) {
	motor_init();
	Serial.begin(9600); // 파이썬과 통신 속도 일치

	while (1) {
		serial_read_data(); // 시리얼 데이터 수신

		if (w == 0 && h == 0) {
			// 물체가 감지되지 않았을 때 (파이썬에서 '0,0,0,0' 전송)
			motors_stop();
			} else {
			// 물체와의 거리에 따라 속도 조절
			if (w > THRESHOLD_STOP_WIDTH) {
				motors_stop(); // 너무 가까우면 정지
				} else if (w > THRESHOLD_APPROACH_WIDTH) {
				// 적당히 가까우면 저속 주행
				if (x + w / 2 > SCREEN_CENTER_X + THRESHOLD_CENTER_RANGE) {
					// 오른쪽으로 치우침 -> 우회전 (좌측 모터 속도 유지)
					left_motor(1, SPEED_LOW);
					right_motor(1, SPEED_STOP);
					} else if (x + w / 2 < SCREEN_CENTER_X - THRESHOLD_CENTER_RANGE) {
					// 왼쪽으로 치우침 -> 좌회전 (우측 모터 속도 유지)
					left_motor(1, SPEED_STOP);
					right_motor(1, SPEED_LOW);
					} else {
					// 중앙에 위치 -> 저속 직진
					motors_forward(SPEED_LOW);
				}
				} else {
				// 물체가 멀리 있음 -> 고속 주행
				if (x + w / 2 > SCREEN_CENTER_X + THRESHOLD_CENTER_RANGE) {
					// 오른쪽으로 치우침 -> 우회전
					left_motor(1, SPEED_HIGH);
					right_motor(1, SPEED_LOW);
					} else if (x + w / 2 < SCREEN_CENTER_X - THRESHOLD_CENTER_RANGE) {
					// 왼쪽으로 치우침 -> 좌회전
					left_motor(1, SPEED_LOW);
					right_motor(1, SPEED_HIGH);
					} else {
					// 중앙에 위치 -> 고속 직진
					motors_forward(SPEED_HIGH);
				}
			}
		}
	}
	return 0;
}

// --- 기존 모터 제어 함수들 (변경 없음) ---
void motor_init(void) {
	DDRB |= (1 << MOTOR_LEFT_IN1) | (1 << MOTOR_LEFT_IN2) |
	(1 << MOTOR_RIGHT_IN1) | (1 << MOTOR_RIGHT_IN2);
	DDRE |= (1 << MOTOR_LEFT_EN) | (1 << MOTOR_RIGHT_EN);

	TCCR3A = (1 << COM3A1) | (1 << COM3B1) | (1 << WGM30);
	TCCR3B = (1 << WGM32) | (1 << CS31);

	PORTB &= ~((1 << MOTOR_LEFT_IN1) | (1 << MOTOR_LEFT_IN2) |
	(1 << MOTOR_RIGHT_IN1) | (1 << MOTOR_RIGHT_IN2));
	OCR3A = 0;
	OCR3B = 0;
}

void left_motor(uint8_t direction, uint8_t speed) {
	if (direction == 1) { // 전진
		PORTB |= (1 << MOTOR_LEFT_IN1);
		PORTB &= ~(1 << MOTOR_LEFT_IN2);
		} else { // 후진
		PORTB &= ~(1 << MOTOR_LEFT_IN1);
		PORTB |= (1 << MOTOR_LEFT_IN2);
	}
	OCR3A = speed;
}

void right_motor(uint8_t direction, uint8_t speed) {
	if (direction == 1) { // 전진
		PORTB |= (1 << MOTOR_RIGHT_IN1);
		PORTB &= ~(1 << MOTOR_RIGHT_IN2);
		} else { // 후진
		PORTB &= ~(1 << MOTOR_RIGHT_IN1);
		PORTB |= (1 << MOTOR_RIGHT_IN2);
	}
	OCR3B = speed;
}

void motors_forward(uint8_t speed) {
	left_motor(1, speed);
	right_motor(1, speed);
}

void motors_backward(uint8_t speed) {
	left_motor(0, speed);
	right_motor(0, speed);
}

void motors_stop(void) {
	OCR3A = 0;
	OCR3B = 0;
	PORTB &= ~((1 << MOTOR_LEFT_IN1) | (1 << MOTOR_LEFT_IN2) |
	(1 << MOTOR_RIGHT_IN1) | (1 << MOTOR_RIGHT_IN2));
}

// 시리얼 데이터 파싱 함수
void serial_read_data(void) {
	if (Serial.available()) {
		String receivedData = Serial.readStringUntil('\n');

		int comma1 = receivedData.indexOf(',');
		int comma2 = receivedData.indexOf(',', comma1 + 1);
		int comma3 = receivedData.indexOf(',', comma2 + 1);

		if (comma1 != -1 && comma2 != -1 && comma3 != -1) {
			String x_str = receivedData.substring(0, comma1);
			String y_str = receivedData.substring(comma1 + 1, comma2);
			String w_str  = receivedData.substring(comma2 + 1, comma3);
			String h_str  = receivedData.substring(comma3 + 1);

			x = x_str.toInt();
			y = y_str.toInt();
			w  = w_str.toInt();
			h  = h_str.toInt();
		}
	}
}