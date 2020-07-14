
/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/

#include "stdio.h"
#include "stdint.h"
#include "string.h"
#include "stdlib.h"
#include "stm32f30x.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "stm32f3xx_nucleo.h"
#include "queue.h"
#include "semphr.h"

/*Macros*/
#define TRUE 1
#define FALSE 0

/*Task functions prototypes*/
void toggleGPIOBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
static void prvSetupHardware(void);
static void prvSetupUART(void);
static void prvSetupGPIO(void);
void printmsg(char* msg);
void rtos_delay(uint32_t delay_in_ms);
void vEmployeeWork(unsigned int workID);

/*Tasks prototypes*/
void vTask1_handler(void* params);
void vTask2_periodic(void* params);

/*Global variable section*/
char usr_msg[40];
/*Task handles*/
TaskHandle_t xHandlerTask = NULL;
TaskHandle_t xPeriodicTask = NULL;
/*Queue handle*/
QueueHandle_t xWorkQueue=NULL;
/*Semaphore handle*/
SemaphoreHandle_t xCountingSemaphore=NULL;

int main(void)
{
	DWT->CTRL |= (1<<0);/*Enable CYCCNT in DWT_CTRL*/

	/*1. Resets the RCC clock configuration to the default reset state.*/
	/*HSI ON, PLL OFF, HSE OFF, System clock = 8 MHz, CPU clock = 8 MHZ*/
	RCC_DeInit();
	/*2. Update SystemCoreClock variable according to Clock Register Values.*/
	SystemCoreClockUpdate();
	prvSetupHardware();

	sprintf(usr_msg,"\r\nCounting Semaphore Demo\r\n");
	printmsg(usr_msg);

	/*Start Recording*/
	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();

	/*Let's create a queue*/
	xWorkQueue=xQueueCreate(1,sizeof(unsigned int));

	/*Let's create a semaphore*/
	xCountingSemaphore=xSemaphoreCreateCounting(10,0);

	if((xWorkQueue!=NULL)&&(xCountingSemaphore!=NULL))
	{
		/*3. Let's create 2 tasks, task-1 and task-2*/
		xTaskCreate(vTask1_handler,"TASK1_HANDLER",200,NULL,1,&xHandlerTask);
		xTaskCreate(vTask2_periodic,"TASK2_PERIODIC",200,NULL,3,&xPeriodicTask);
		/*4. Start the scheduler*/
		vTaskStartScheduler();
	}
	else
	{
		sprintf(usr_msg,"Queue or semaphore creation failed\r\n");
		printmsg(usr_msg);
	}

	for(;;);
}

void vTask1_handler(void* params)
{
	while(1)
	{
		xSemaphoreTake(xCountingSemaphore,portMAX_DELAY);
		sprintf(usr_msg,"Handler task processing event\r\n");
		printmsg(usr_msg);
	}
}

void vTask2_periodic(void* params)
{
	while(1)
	{
		vTaskDelay(pdMS_TO_TICKS(500));
		sprintf(usr_msg,"Periodic task - Pending the interrupt\r\n");
		printmsg(usr_msg);
		NVIC_SetPendingIRQ(EXTI15_10_IRQn);
		sprintf(usr_msg,"Periodic task - Resuming\r\n");
		printmsg(usr_msg);
	}
}

void vEmployeeWork(unsigned int workID)
{
	sprintf(usr_msg,"Working on the task ID: %d\r\n",workID);
	printmsg(usr_msg);
	vTaskDelay(workID);
}

static void prvSetupUART(void)
{
	GPIO_InitTypeDef gpio_uart_pins;
	USART_InitTypeDef uart2_init;
	/*1. Enable the UART2 and GPIOA peripheral clock*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	/*PA2 is UART2_TX, PA3 is UART2_RX*/
	/*2. Alternate function configuration of MCU pins to behave as UART2 TX and RX*/
	gpio_uart_pins.GPIO_Pin=GPIO_Pin_2|GPIO_Pin_3;
	gpio_uart_pins.GPIO_Mode=GPIO_Mode_AF;
	gpio_uart_pins.GPIO_PuPd=GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &gpio_uart_pins);
	/*3. AF mode settings for the pins*/
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_7);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_7);
	/*4. UART parameter initialization*/
	uart2_init.USART_BaudRate=19200;
	uart2_init.USART_HardwareFlowControl=USART_HardwareFlowControl_None;
	uart2_init.USART_Mode=USART_Mode_Tx|USART_Mode_Rx;
	uart2_init.USART_Parity=USART_Parity_No;
	uart2_init.USART_StopBits=USART_StopBits_1;
	uart2_init.USART_WordLength=USART_WordLength_8b;
	USART_Init(USART2, &uart2_init);
	/*5. Enable the UART peripheral*/
	USART_Cmd(USART2, ENABLE);
}

static void prvSetupHardware(void)
{
	/*Setup UART2*/
	prvSetupUART();
	/*Setup LED and button*/
	prvSetupGPIO();
}

void printmsg(char* msg)
{
	for(uint32_t i=0;i<strlen(msg);i++)
	{
		while(USART_GetFlagStatus(USART2, USART_FLAG_TXE)!=SET);
		USART_SendData(USART2, msg[i]);
	}
}

static void prvSetupGPIO(void)
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA,ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG,ENABLE);

	GPIO_InitTypeDef gpio_led_pin, gpio_button_pin;
	gpio_led_pin.GPIO_Pin=GPIO_Pin_5;
	gpio_led_pin.GPIO_Mode=GPIO_Mode_OUT;
	gpio_led_pin.GPIO_OType=GPIO_OType_PP;
	gpio_led_pin.GPIO_PuPd=GPIO_PuPd_NOPULL;
	gpio_led_pin.GPIO_Speed=GPIO_Speed_Level_2;
	GPIO_Init(GPIOA,&gpio_led_pin);

	gpio_button_pin.GPIO_Pin=GPIO_Pin_13;
	gpio_button_pin.GPIO_Mode=GPIO_Mode_IN;
	gpio_button_pin.GPIO_OType=GPIO_OType_PP;
	gpio_button_pin.GPIO_PuPd=GPIO_PuPd_NOPULL;
	gpio_button_pin.GPIO_Speed=GPIO_Speed_Level_2;
	GPIO_Init(GPIOC,&gpio_button_pin);

	/*Interrupt configuration for the button*/
	/*1. System configuration for exti line*/
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC,EXTI_PinSource13);
	/*2. EXTI line configuration 13, falling edge, interrupt mode*/
	EXTI_InitTypeDef exti_init;
	exti_init.EXTI_Line=EXTI_Line13;
	exti_init.EXTI_Mode=EXTI_Mode_Interrupt;
	exti_init.EXTI_Trigger=EXTI_Trigger_Falling;
	exti_init.EXTI_LineCmd=ENABLE;
	EXTI_Init(&exti_init);
	/*3. NVIC settings (IRQ settings for the selected EXTI line)*/
	NVIC_SetPriority(EXTI15_10_IRQn,5);
	NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void rtos_delay(uint32_t delay_in_ms)
{
	uint32_t current_tick_count = xTaskGetTickCount();
	uint32_t delay_in_ticks=(delay_in_ms*configTICK_RATE_HZ)/1000;
	while(xTaskGetTickCount()<(current_tick_count+delay_in_ticks));
}

void toggleGPIOBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	if(GPIO_ReadOutputDataBit(GPIOx, GPIO_Pin)==1)
	{
		GPIO_ResetBits(GPIOx, GPIO_Pin);
	}
	else
	{
		GPIO_SetBits(GPIOx, GPIO_Pin);
	}
}

void vApplicationMallocFailedHook(void)
{
	sprintf(usr_msg,"Memory allocation failed\r\n");
	printmsg(usr_msg);
}

void EXTI15_10_IRQHandler(void)
{
	traceISR_ENTER();
	portBASE_TYPE xHigherPriorityTaskWoken=pdFALSE;
	EXTI_ClearITPendingBit(EXTI_Line13);
	sprintf(usr_msg,"==> Button handler\r\n");
	printmsg(usr_msg);
	xSemaphoreGiveFromISR(xCountingSemaphore,&xHigherPriorityTaskWoken);
	xSemaphoreGiveFromISR(xCountingSemaphore,&xHigherPriorityTaskWoken);
	xSemaphoreGiveFromISR(xCountingSemaphore,&xHigherPriorityTaskWoken);
	xSemaphoreGiveFromISR(xCountingSemaphore,&xHigherPriorityTaskWoken);
	xSemaphoreGiveFromISR(xCountingSemaphore,&xHigherPriorityTaskWoken);

	//NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
	traceISR_EXIT();
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}
