#include "reg51.h"

#define uchar unsigned char		   
#define uint  unsigned int

/*****控制端口定义*****/
// 电机端口定义
sbit motor_1  = P2^6;
sbit motor_2  = P2^7;
sbit motor_EN = P3^0;

// 数码管位选端口定义，段选端口:P0
sbit smg_1 = P2^0;		      // 数码管1位选
sbit smg_2 = P2^1;		      // 数码管2位选
sbit smg_3 = P2^2;		      // 数码管3位选
sbit smg_4 = P2^3;		      // 数码管4位选
sbit smg_5 = P2^4;		      // 数码管5位选
sbit smg_6 = P2^5;		      // 数码管6位选

uchar code table[10] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f}; // 共阴数码管
//                         0、   1、  2、   3、   4、   5、   6、  7、   8、   9

/*****全局变量定义*****/
uint  speed_counter  = 0; 	  // 编码器输出计数器
uint  speed          = 0;     // 实际速度
uint  set_speed      = 10;	  // 设定速度
uint  target_speed   = 10; 	  // 目标速度
uint  PWM            = 0;     // PWM波占空比 PWM %
int   pwm_reg        = 0;
uchar motor_begin    = 0;     // 电机开始转动标志位 
uchar PID_begin      = 0;     // PID计算标志位
// PID相关参数
float Kp = 2.0, 
      Ki = 0.6, 
      Kd = 0.4;               // P I D 参数
int   error = 0;              // 速度偏差
int   last_error_1 = 0;       // 上一次速度偏差
int   last_error_2 = 0;       // 上上一次速度偏差

/*****函数声明*****/
void display(uchar i, uchar number); 			// 显示函数
void delay();   								// 延时函数
void KeyScan();									// 按键扫描函数
int  PID(uint tar_speed, uint real_speed); 		// PID调节函数


/**************************************
// 函数名 主函数
// 参  数 无
// 返回值 无
***************************************/
void main()
{
    P0 = 0x00; 
    P1 = 0xf0;
	TMOD = 0x01;		// 定时器0方式1
    TH0 = 0xFF;		    // 定时器赋初值 定时100us
    TL0 = 0x9C;	
	EA  = 1;    		// 打开总中断
	ET0 = 1;   			// 打开定时器0中断
	TR0 = 1;	 	    // 打开定时器0
	IT0 = 1; 			// 设置外部中断0触发方式, 0为低电平触发 1为下降沿触发
	EX0 = 1; 			// 打开外部中断0
    motor_1 = 0;
    motor_2 = 0;
	
    while(1)
    {
        KeyScan(); // 按键扫描
		
		// 显示实际速度
		display(1,speed / 100);		delay();
		display(2,speed % 100 / 10);delay();
		display(3,speed % 10);		delay();
		
		// 显示设定速度
		display(4,set_speed / 100);	    delay();
		display(5,set_speed % 100 / 10);delay();
		display(6,set_speed % 10);	    delay();
		
		// PWM更新
		if(PID_begin == 1)
		{
            PWM = PID(target_speed, speed);
			PID_begin = 0;
		}
	}
}


/**************************************
// 函数名 PID调节函数
// 参  数 tar_speed  目标速度
//		  real_speed 实际速度
// 返回值 pwm        PWM波占空比
 **************************************/
int PID(uint tar_speed, uint real_speed)
{    
    static int pwm_INC = 0, np = 0, ni = 0, nd = 0;
    
    // 计算pwm输出增量
	error = tar_speed - real_speed;    
    np = Kp * (error - last_error_1);
    ni = Ki * error;
    nd = Kd * (error - 2 * last_error_1 + last_error_2);
    pwm_INC = np + ni + nd;
    
    pwm_reg = pwm_reg + pwm_INC;
 
    last_error_2 = last_error_1;
	last_error_1 = error;
    
    // 输出限幅
	if(pwm_reg < 0)
		pwm_reg = 0;
	if(pwm_reg > 100)
		pwm_reg = 100;
    
	pwm_INC = 0;
    return pwm_reg;
    
}


/**************************************
// 函数名 定时器0中断服务函数
// 参  数 无
// 返回值 无
***************************************/
void TR0_isr() interrupt 1
{
	static uint i = 0;
	static uint speed_control = 0;
	
    TH0 = 0xFF; // 定时器重装载值，定时100us
    TL0 = 0x9C;
	
	i++;
	speed_control++;
	
	// 采样周期 500ms
	if(i == 5000)
	{	
		speed = speed_counter;
		speed_counter = 0;
		i = 0;
		if(motor_begin == 1) // PID计算一次
			PID_begin = 1;
	}
	
	// PWM波周期 10ms 频率 100Hz
	if(speed_control <= PWM)
		motor_EN = 1;
	if(speed_control >  PWM)
		motor_EN = 0;
	if(speed_control >= 100)
		speed_control = 0;
}


/**************************************
// 函数名 外部中断0服务函数
// 参  数 无
// 返回值 无
***************************************/
void INT0_isr() interrupt 0    
{
	speed_counter++;		// 电机编码器输出计数
}


/**************************************
// 函数名 延时1ms函数
// 参  数 无
// 返回值 无
***************************************/
void delay() 
{
	uchar i, j; 
	for(i = 0; i < 10; i++) 
		for(j = 0; j < 33; j++) ; 
}


/**************************************
// 函数名 按键扫描函数
// 参  数 无
// 返回值 无
//
////////////按键布局//////////////
//   1      2      3      4    //
//   5      6      7      8    //
//   9      0    确认键 清零键  //
// 开始键 停止键  预留   预留    //
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
			//第一行按键按下
			case 0x07: 
				P1 = 0xf0;
				switch(P1)
				{
					// 第一行第一个按键 数字1
					case 0xe0 : 
						if(ten_flag == 0) 
						{
							set_speed = 1;
							ten_flag = 1;
						}
						else
							set_speed = set_speed * 10 + 1 ; 
					break;
					// 第一行第二个按键 数字2	
					case 0xd0 :
						if(ten_flag == 0) 
						{
							set_speed = 2;
							ten_flag = 1;
						}
						else 
							set_speed = set_speed * 10 + 2; 
					break;	
					// 第一行第三个按键 数字3
					case 0xb0 : 						
						if(ten_flag == 0) 
						{
							set_speed = 3;
							ten_flag = 1;
						}
						else 
							set_speed = set_speed * 10 + 3; 
					break;
					// 第一行第四个按键 数字4	
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
                
			//第二行按键按下	
			case 0x0b: 
				P1 = 0xf0;
				switch(P1)
				{
					// 第二行第一个按键 数字5
					case 0xe0 : 
						if(ten_flag == 0) 
						{
							set_speed = 5;
							ten_flag = 1;
						}
						else 
							set_speed = set_speed * 10 + 5; 
					break;
					// 第二行第二个按键 数字6	
					case 0xd0 :
						if(ten_flag == 0) 
						{
							set_speed = 6;
							ten_flag = 1;
						}
						else 
							set_speed = set_speed * 10 + 6; 
					break;	
					// 第二行第三个按键 数字7	
					case 0xb0 : 						
						if(ten_flag == 0) 
						{
							set_speed = 7;
							ten_flag = 1;
						}
						else 
							set_speed = set_speed * 10 + 7; 
					break;
					// 第二行第四个按键 数字8	
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
                
			//第三行按键按下
			case 0x0d: 
				P1 = 0xf0;
				switch(P1)
				{
					// 第三行第一个按键 数字9
					case 0xe0 :
						if(ten_flag == 0) 
						{
							set_speed = 9;
							ten_flag = 1;
						}
						else 
							set_speed = set_speed * 10 +9 ; 
					break;
					// 第三行第二个按键 数字0	
					case 0xd0 : 
						if(ten_flag == 0) 
						{
							set_speed = 0;
							ten_flag = 0;
						}
						else 
							set_speed = set_speed * 10; 
					break;
					// 第三行第三个按键 确认键
					case 0xb0 : target_speed = set_speed;ten_flag=0; break;
					// 第三行第四个按键 清零键	
					case 0x70 : set_speed = 0;ten_flag=0;break;
				}while(P1 != 0xf0); break;
                
			//第四行按键按下
			case 0x0e: 
				P1 = 0xf0;
				switch(P1)
				{
					// 第四行第一个按键 开始键
					case 0xe0 : motor_begin = 1; motor_1 = 0; motor_2	= 1; break;
					// 第四行第二个按键 停止键
					case 0xd0 : motor_begin = 0; PWM = 0; pwm_reg = 0; motor_1 = 0; motor_2	= 0; break;
					// 第四行第三个按键 预留
					case 0xb0 : break;
					// 第四行第四个按键 预留
					case 0x70 : break;
				} while(P1 != 0xf0);break;
		}
	}
}


/**************************************
// 函数名 数码管显示函数 
// 参  数 i : 选择第i个数码管显示 范围:1-6
// 		  number : 显示数字number 范围:0-9
// 返回值 无
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





