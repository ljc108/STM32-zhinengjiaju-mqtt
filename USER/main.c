#include "led.h"
#include "key.h"
#include "beep.h"
#include "delay.h"
#include "sys.h"
#include "timer.h"
#include "usart.h"
#include "dht11.h"
#include "bh1750.h"
#include "oled.h"
#include "exti.h"
#include "stdio.h"
#include "esp8266.h"
#include "onenet.h"

u8 alarmFlag = 0;//是否报警的标志
u8 alarm_is_free = 10;//报警器是否被手动操作，如果被手动操作及设为0


u8 humidityH;
u8 humidityL;
u8 temperatureH;
u8 temperatureL;
extern char oledBuf[20];
float Light = 0;

char PUB_BUF[256];//上传数据的buf
const char *devSubTopic[] = {"/mysmarthome/sub"};
const char devPubTopic[] = "/mysmarthome/pub";
/************************************************
 ALIENTEK精英STM32开发板实验1
 跑马灯实验
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com 
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/
 int main(void)
 {	
	unsigned short timeCount = 0;	//发送间隔变量
	unsigned char *dataPtr = NULL;
	delay_init();	    //延时函数初始化	 
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置NVIC中断分组2:2位抢占优先级，2位响应优先级	 
	LED_Init();		  	//初始化与LED连接的硬件接口
	KEY_Init();         	//初始化与按键连接的硬件接口
	EXTIX_Init();         	//初始化外部中断输入 
	BEEP_Init(); 
	DHT11_Init();
	BH1750_Init();
	
	 
	OLED_Init();  
	OLED_ColorTurn(0);//0正常显示，1 反色显示
  OLED_DisplayTurn(0);//0正常显示 1 屏幕翻转显示
	OLED_Clear();
	TIM3_Int_Init(4999,7199);
	TIM2_Int_Init(2499,7199);
	UsartPrintf(USART_DEBUG, " Hardware init OK\r\n");
	Usart1_Init(115200);//debug串口
	Usart2_Init(115200);//stm32-8266通讯串口 
	 
	ESP8266_Init();					//初始化ESP8266
	while(OneNet_DevLink())			//接入OneNET
		delay_ms(500);
	
	BEEP = 1;				//鸣叫提示接入成功
	delay_ms(250);
	BEEP = 0;
	
	OneNet_Subscribe(devSubTopic, 1);
	
	while(1)
	{
		
		if(timeCount % 40 == 0)//1000ms / 25 = 40 约一秒执行一次
		{
			//温湿度传感器获取数据
			DHT11_Read_Data(&humidityH,&humidityL,&temperatureH,&temperatureL);
			UsartPrintf(USART_DEBUG,"湿度：%d.%d温度：%d.%d" ,humidityH,humidityL,temperatureH,temperatureL);
			//光照度传感器获取数据
			if(!i2c_CheckDevice(BH1750_Addr))
			{
				Light = LIght_Intensity();
				UsartPrintf(USART_DEBUG,"当前光照强度为：%.1f lx\r\n", Light); 
			}
			if(alarm_is_free == 10)//报警器控制权是否空闲 alarm_is_free > 0 初始值为10
			{
				if((humidityH < 80)&&(temperatureH < 30)&&(Light < 10000))alarmFlag = 0;
				else alarmFlag = 1;
			} 
			if(alarm_is_free < 10)alarm_is_free++;
			//UsartPrintf(USART_DEBUG,"alarm_is_free = %d\r\n",alarm_is_free);
			//UsartPrintf(USART_DEBUG,"alarmFlag = %d\r\n",alarmFlag);
		}	
		if(++timeCount >= 200)	// 	5000ms /25 = 200ms	//发送间隔5s
	 {
		 UsartPrintf(USART_DEBUG, "OneNet_Publish\r\n");
		 sprintf(PUB_BUF,"{\"Hum\":%d.%d,\"Temp\":%d.%d,\"Light\":%.1f}",
		  humidityH,humidityL,temperatureH,temperatureL,Light);
		 OneNet_Publish(devPubTopic, PUB_BUF);
			timeCount = 0;
			ESP8266_Clear();
		}
		
			dataPtr = ESP8266_GetIPD(3);
			if(dataPtr != NULL)
				OneNet_RevPro(dataPtr);
			delay_ms(10);
			
	}
}


 /**
 *****************下面注视的代码是通过调用库函数来实现IO控制的方法*****************************************
int main(void)
{ 
 
	delay_init();		  //初始化延时函数
	LED_Init();		        //初始化LED端口
	while(1)
	{
			GPIO_ResetBits(GPIOB,GPIO_Pin_5);  //LED0对应引脚GPIOB.5拉低，亮  等同LED0=0;
			GPIO_SetBits(GPIOE,GPIO_Pin_5);   //LED1对应引脚GPIOE.5拉高，灭 等同LED1=1;
			delay_ms(300);  		   //延时300ms
			GPIO_SetBits(GPIOB,GPIO_Pin_5);	   //LED0对应引脚GPIOB.5拉高，灭  等同LED0=1;
			GPIO_ResetBits(GPIOE,GPIO_Pin_5); //LED1对应引脚GPIOE.5拉低，亮 等同LED1=0;
			delay_ms(300);                     //延时300ms
	}
} 
 
 ****************************************************************************************************
 ***/
 

	
/**
*******************下面注释掉的代码是通过 直接操作寄存器 方式实现IO口控制**************************************
int main(void)
{ 
 
	delay_init();		  //初始化延时函数
	LED_Init();		        //初始化LED端口
	while(1)
	{
     GPIOB->BRR=GPIO_Pin_5;//LED0亮
	   GPIOE->BSRR=GPIO_Pin_5;//LED1灭
		 delay_ms(300);
     GPIOB->BSRR=GPIO_Pin_5;//LED0灭
	   GPIOE->BRR=GPIO_Pin_5;//LED1亮
		 delay_ms(300);

	 }
 }
**************************************************************************************************
**/

