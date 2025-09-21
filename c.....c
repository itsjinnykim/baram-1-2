#include <avr/io.h>
#include <util/delay.h>

#define MOTOR_LEFT_IN1  PB5  
#define MOTOR_LEFT_IN2  PB6  
#define MOTOR_LEFT_EN   PE3 

#define MOTOR_RIGHT_IN1 PB2 
#define MOTOR_RIGHT_IN2 PB3 
#define MOTOR_RIGHT_EN  PE4 

#define CAM_W 640
#define CAM_H 480
#define CAM_CENTER (CAM_W / 2)

#define speed_high 
#define speed_low
#define speed_stop 

#define approach_w // �ٰ����� �� 
#define stop_w // ���� �̰ź��� ũ�� ���� 
#define center_range // �߽ɿ��� �� ���� �ȼ���ŭ ����� �� �״��. ���� �����ϴ� �ּ����� �ȼ� 

// �� ���� ���� �� �غ��鼭 �����ϱ� 

int x=0, y=0, w=0, h=0;

void motor_init(void);
void left_motor(uint8_t direction, uint8_t speed);
void right_motor(uint8_t direction, uint8_t speed);
void motor_forward(uint8_t speed);
void motor_backward(uint8_t speed);
void motor_stop(void);
void read_serial(void);

void motor_init(void)
{
	DDRB |= (1<< MOTOR_LEFT_IN1) | (1<<MOTOR_LEFT_IN2) | (1<<MOTOR_RIGHT_IN1) | (1<< MOTOR_RIGHT_IN2);
	DDRE |= (1<<MOTOR_LEFT_EN) | (1<<MOTOR_RIGHT_EN);
	
	TCCR3A = (1<<COM3A1) | (1<< COM3B1) | (1<<WGM30);
	TCCR3B = (1<<WGM32) | (1<<CS31);
	
	PORTB &= ~((1<<MOTOR_LEFT_IN1) | (1<<MOTOR_LEFT_IN2) | (1<< MOTOR_RIGHT_IN1) | (1<< MOTOR_RIGHT_IN2));
	OCR3A = 0;
	OCR3B = 0; 
}
	

void left_motor(uint8_t direction, uint8_t speed)
{
	if (direction == 1)
	{
		PORTB |= (1<< MOTOR_LEFT_IN1);
		PORTB &= ~(1<<MOTOR_LEFT_IN2);
	}
	else
	{
		PORTB &= ~(1<<MOTOR_LEFT_IN1);
		PORTB |= (1<< MOTOR_LEFT_IN2);
	}
	OCR3A = speed; 
}

void right_motor(uint8_t direction, uint8_t speed)
{
	if (direction == 1) 
	{ 
		PORTB |= (1 << MOTOR_RIGHT_IN1);
		PORTB &= ~(1 << MOTOR_RIGHT_IN2);
	} 
	else
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
	function read_serial():
	// 1. ���ۿ� �ε��� �غ�
	static buffer[32]         // ������ ���ڿ��� �ӽ� ����
	static index = 0          // ���� �� ��° ���ڱ��� �����ߴ��� ǥ��
	
	// 2. UART���� �� ���� �ޱ�
	char c = uart_receive()   // (��: '1', '2', '0', ',', '2', '4', ...)

	// 3. �ٹٲ� ���ڰ� ���� �� �� �ϼ�
	if c == '\n':
	buffer[index] = '\0'  // ���ڿ� �� ǥ�� (C ���ڿ� �ϼ�)
	
	// 4. ��ǥ(,) �������� ���ڿ� �ڸ���
	token1 = strtok(buffer, ",")   // ù ��° �� �� "120"
	token2 = strtok(NULL, ",")     // �� ��° �� �� "240"
	token3 = strtok(NULL, ",")     // �� ��° �� �� "50"
	token4 = strtok(NULL, ",")     // �� ��° �� �� "60"
	
	// 5. ���ڿ� �� ���� ��ȯ
	x = atoi(token1)   // x = 120
	y = atoi(token2)   // y = 240
	w = atoi(token3)   // w = 50
	h = atoi(token4)   // h = 60
	
	// 6. ���� �޽��� �غ�
	index = 0
	
	// 7. �ٹٲ��� �ƴ� ��� �� ���ۿ� ����
	else:
	buffer[index] = c
	index = index + 1
}

int main(void)
{
	motor_init();
	
	while(1)
	{
		if(w>stop_w)
		{
			���� ���߰ų� ���� ���� �Լ� ȣ��
		}
		else if(w>approach_w)
		{
			if(cx>screen center right){
				��ȸ�� 
			}
			else if(cx<screen center left){
				��ȸ�� 
		    }
			else{
				���� �ӵ��� ���� 
			}
	    }
	   else//��ü�� �ָ� ������ 
	   {
		   if (cx > SCREEN_CENTER_RIGHT): ��ȸ��

		   else if (cx < SCREEN_CENTER_LEFT): ��ȸ��
	   }
   }