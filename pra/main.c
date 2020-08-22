#include "reg51.h"

#define uchar unsigned char		   
#define uint  unsigned int

/*****���ƶ˿ڶ���*****/
// ����˿ڶ���
sbit motor_1  = P2^6;
sbit motor_2  = P2^7;
sbit motor_EN = P3^0;

// �����λѡ�˿ڶ��壬��ѡ�˿�:P0
sbit smg_1 = P2^0;		      // �����1λѡ
sbit smg_2 = P2^1;		      // �����2λѡ
sbit smg_3 = P2^2;		      // �����3λѡ
sbit smg_4 = P2^3;		      // �����4λѡ
sbit smg_5 = P2^4;		      // �����5λѡ
sbit smg_6 = P2^5;		      // �����6λѡ

uchar code table[10] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f}; // ���������
//                         0��   1��  2��   3��   4��   5��   6��  7��   8��   9

/*****ȫ�ֱ�������*****/
uint  speed_counter  = 0; 	  // ���������������
uint  speed          = 0;     // ʵ���ٶ�
uint  set_speed      = 10;	  // �趨�ٶ�
uint  target_speed   = 10; 	  // Ŀ���ٶ�
uint  PWM            = 0;     // PWM��ռ�ձ� PWM %
int   pwm_reg        = 0;
uchar motor_begin    = 0;     // �����ʼת����־λ 
uchar PID_begin      = 0;     // PID�����־λ
// PID��ز���
float Kp = 2.0, 
      Ki = 0.6, 
      Kd = 0.4;               // P I D ����
int   error = 0;              // �ٶ�ƫ��
int   last_error_1 = 0;       // ��һ���ٶ�ƫ��
int   last_error_2 = 0;       // ����һ���ٶ�ƫ��

/*****��������*****/
void display(uchar i, uchar number); 			// ��ʾ����
void delay();   								// ��ʱ����
void KeyScan();									// ����ɨ�躯��
int  PID(uint tar_speed, uint real_speed); 		// PID���ں���


/**************************************
// ������ ������
// ��  �� ��
// ����ֵ ��
***************************************/
void main()
{
    P0 = 0x00; 
    P1 = 0xf0;
	TMOD = 0x01;		// ��ʱ��0��ʽ1
    TH0 = 0xFF;		    // ��ʱ������ֵ ��ʱ100us
    TL0 = 0x9C;	
	EA  = 1;    		// �����ж�
	ET0 = 1;   			// �򿪶�ʱ��0�ж�
	TR0 = 1;	 	    // �򿪶�ʱ��0
	IT0 = 1; 			// �����ⲿ�ж�0������ʽ, 0Ϊ�͵�ƽ���� 1Ϊ�½��ش���
	EX0 = 1; 			// ���ⲿ�ж�0
    motor_1 = 0;
    motor_2 = 0;
	
    while(1)
    {
        KeyScan(); // ����ɨ��
		
		// ��ʾʵ���ٶ�
		display(1,speed / 100);		delay();
		display(2,speed % 100 / 10);delay();
		display(3,speed % 10);		delay();
		
		// ��ʾ�趨�ٶ�
		display(4,set_speed / 100);	    delay();
		display(5,set_speed % 100 / 10);delay();
		display(6,set_speed % 10);	    delay();
		
		// PWM����
		if(PID_begin == 1)
		{
            PWM = PID(target_speed, speed);
			PID_begin = 0;
		}
	}
}


/**************************************
// ������ PID���ں���
// ��  �� tar_speed  Ŀ���ٶ�
//		  real_speed ʵ���ٶ�
// ����ֵ pwm        PWM��ռ�ձ�
 **************************************/
int PID(uint tar_speed, uint real_speed)
{    
    static int pwm_INC = 0, np = 0, ni = 0, nd = 0;
    
    // ����pwm�������
	error = tar_speed - real_speed;    
    np = Kp * (error - last_error_1);
    ni = Ki * error;
    nd = Kd * (error - 2 * last_error_1 + last_error_2);
    pwm_INC = np + ni + nd;
    
    pwm_reg = pwm_reg + pwm_INC;
 
    last_error_2 = last_error_1;
	last_error_1 = error;
    
    // ����޷�
	if(pwm_reg < 0)
		pwm_reg = 0;
	if(pwm_reg > 100)
		pwm_reg = 100;
    
	pwm_INC = 0;
    return pwm_reg;
    
}


/**************************************
// ������ ��ʱ��0�жϷ�����
// ��  �� ��
// ����ֵ ��
***************************************/
void TR0_isr() interrupt 1
{
	static uint i = 0;
	static uint speed_control = 0;
	
    TH0 = 0xFF; // ��ʱ����װ��ֵ����ʱ100us
    TL0 = 0x9C;
	
	i++;
	speed_control++;
	
	// �������� 500ms
	if(i == 5000)
	{	
		speed = speed_counter;
		speed_counter = 0;
		i = 0;
		if(motor_begin == 1) // PID����һ��
			PID_begin = 1;
	}
	
	// PWM������ 10ms Ƶ�� 100Hz
	if(speed_control <= PWM)
		motor_EN = 1;
	if(speed_control >  PWM)
		motor_EN = 0;
	if(speed_control >= 100)
		speed_control = 0;
}


/**************************************
// ������ �ⲿ�ж�0������
// ��  �� ��
// ����ֵ ��
***************************************/
void INT0_isr() interrupt 0    
{
	speed_counter++;		// ����������������
}


/**************************************
// ������ ��ʱ1ms����
// ��  �� ��
// ����ֵ ��
***************************************/
void delay() 
{
	uchar i, j; 
	for(i = 0; i < 10; i++) 
		for(j = 0; j < 33; j++) ; 
}


/**************************************
// ������ ����ɨ�躯��
// ��  �� ��
// ����ֵ ��
//
////////////��������//////////////
//   1      2      3      4    //
//   5      6      7      8    //
//   9      0    ȷ�ϼ� �����  //
// ��ʼ�� ֹͣ��  Ԥ��   Ԥ��    //
/////////////////////////////////
***************************************/
void KeyScan()
{
	static uchar ten_flag = 0;
	P1 = 0x0f;
	if (P1 != 0x0f)
	{
		switch(P1)
		{
			//��һ�а�������
			case 0x07: 
				P1 = 0xf0;
				switch(P1)
				{
					// ��һ�е�һ������ ����1
					case 0xe0 : 
						if(ten_flag == 0) 
						{
							set_speed = 1;
							ten_flag = 1;
						}
						else
							set_speed = set_speed * 10 + 1 ; 
					break;
					// ��һ�еڶ������� ����2	
					case 0xd0 :
						if(ten_flag == 0) 
						{
							set_speed = 2;
							ten_flag = 1;
						}
						else 
							set_speed = set_speed * 10 + 2; 
					break;	
					// ��һ�е��������� ����3
					case 0xb0 : 						
						if(ten_flag == 0) 
						{
							set_speed = 3;
							ten_flag = 1;
						}
						else 
							set_speed = set_speed * 10 + 3; 
					break;
					// ��һ�е��ĸ����� ����4	
					case 0x70 : 
						if(ten_flag == 0) 
						{
							set_speed = 4;
							ten_flag = 1;
						}
						else 
							set_speed = set_speed * 10 + 4; 
					break;
				} while(P1 != 0xf0); break;
                
			//�ڶ��а�������	
			case 0x0b: 
				P1 = 0xf0;
				switch(P1)
				{
					// �ڶ��е�һ������ ����5
					case 0xe0 : 
						if(ten_flag == 0) 
						{
							set_speed = 5;
							ten_flag = 1;
						}
						else 
							set_speed = set_speed * 10 + 5; 
					break;
					// �ڶ��еڶ������� ����6	
					case 0xd0 :
						if(ten_flag == 0) 
						{
							set_speed = 6;
							ten_flag = 1;
						}
						else 
							set_speed = set_speed * 10 + 6; 
					break;	
					// �ڶ��е��������� ����7	
					case 0xb0 : 						
						if(ten_flag == 0) 
						{
							set_speed = 7;
							ten_flag = 1;
						}
						else 
							set_speed = set_speed * 10 + 7; 
					break;
					// �ڶ��е��ĸ����� ����8	
					case 0x70 : 
						if(ten_flag == 0) 
						{
							set_speed = 8;
							ten_flag = 1;
						}
						else 
							set_speed = set_speed * 10 + 8; 
					break;
				}while(P1 != 0xf0); break;
                
			//�����а�������
			case 0x0d: 
				P1 = 0xf0;
				switch(P1)
				{
					// �����е�һ������ ����9
					case 0xe0 :
						if(ten_flag == 0) 
						{
							set_speed = 9;
							ten_flag = 1;
						}
						else 
							set_speed = set_speed * 10 +9 ; 
					break;
					// �����еڶ������� ����0	
					case 0xd0 : 
						if(ten_flag == 0) 
						{
							set_speed = 0;
							ten_flag = 0;
						}
						else 
							set_speed = set_speed * 10; 
					break;
					// �����е��������� ȷ�ϼ�
					case 0xb0 : target_speed = set_speed;ten_flag=0; break;
					// �����е��ĸ����� �����	
					case 0x70 : set_speed = 0;ten_flag=0;break;
				}while(P1 != 0xf0); break;
                
			//�����а�������
			case 0x0e: 
				P1 = 0xf0;
				switch(P1)
				{
					// �����е�һ������ ��ʼ��
					case 0xe0 : motor_begin = 1; motor_1 = 0; motor_2	= 1; break;
					// �����еڶ������� ֹͣ��
					case 0xd0 : motor_begin = 0; PWM = 0; pwm_reg = 0; motor_1 = 0; motor_2	= 0; break;
					// �����е��������� Ԥ��
					case 0xb0 : break;
					// �����е��ĸ����� Ԥ��
					case 0x70 : break;
				} while(P1 != 0xf0);break;
		}
	}
}


/**************************************
// ������ �������ʾ���� 
// ��  �� i : ѡ���i���������ʾ ��Χ:1-6
// 		  number : ��ʾ����number ��Χ:0-9
// ����ֵ ��
***************************************/
void display(uchar i, uchar number)
{
	switch (i)
	{
		case 1 : smg_1 = 0; smg_2 = 1;smg_3 = 1;smg_4 = 1; smg_5 = 1; smg_6 = 1; break;
		case 2 : smg_1 = 1; smg_2 = 0;smg_3 = 1;smg_4 = 1; smg_5 = 1; smg_6 = 1; break;
		case 3 : smg_1 = 1; smg_2 = 1;smg_3 = 0;smg_4 = 1; smg_5 = 1; smg_6 = 1; break;
		case 4 : smg_1 = 1; smg_2 = 1;smg_3 = 1;smg_4 = 0; smg_5 = 1; smg_6 = 1; break;
		case 5 : smg_1 = 1; smg_2 = 1;smg_3 = 1;smg_4 = 1; smg_5 = 0; smg_6 = 1; break;
		case 6 : smg_1 = 1; smg_2 = 1;smg_3 = 1;smg_4 = 1; smg_5 = 1; smg_6 = 0; break;		
	}
	P0 = table[number];
}





