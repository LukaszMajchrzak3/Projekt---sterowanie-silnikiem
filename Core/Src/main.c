/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "LCD1602.h"
#include "aio.h"
#include<stdio.h>
#include <math.h>
#include <stdint.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
int pot1_mV = 0;
int pot1_mV_filtered = 0;
int previous_value = 0;
int previous_value2 = 0;
uint16_t prev_period = 0;
uint16_t freq = 0;
//float freq = 0;
uint16_t cur_period = 0;
//uint16_t vel = 0;
uint16_t vel = 0;
int count_to_clear_lcd = 0;
float ref_vel = 400; //wartosc referencyjna  - JAK NIE DZIALA ZMIENIC NA FLOAT!!!!!!!!!
int pwm_level = 400;
int brake_dyn = 1;
int en_brake = 0;

int was_reached = 0;

char uart_buf[50];
int uart_buf_len;
char led_buf[50];
int led_buf_len;
HAL_StatusTypeDef uart2_received_status;
uint8_t single_message_received[] = {0};
char single_message_response[30]={0};
uint16_t timer_val = 0;
int change_operation = 0;
int time_acc = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int row=0;
int col=0;

void DelayUS(uint16_t us) {
    __HAL_TIM_SET_COUNTER(&htim8,0);
    while(__HAL_TIM_GET_COUNTER(&htim8) < us);
}

void pwm_control(int potential, int prev_potential)
{
	if((was_reached == 0) & (potential > 2800)) //2500
	{
		was_reached = 1;
	}

	if((was_reached == 1) & (potential < 100) & (prev_potential < 100))
	{
		was_reached = 0;
		if(change_operation == 0)
		{
			timer_val = __HAL_TIM_GET_COUNTER(&htim16);
			change_operation = 1;
		}
		else if(change_operation == 1)
		{
			timer_val = __HAL_TIM_GET_COUNTER(&htim16) - timer_val;
			change_operation = 0;
			cur_period = timer_val;
			freq = 1000000/cur_period;

			if(count_to_clear_lcd < 60)
			{
				count_to_clear_lcd = count_to_clear_lcd + 1;
				if(fabs(ref_vel-vel) > 260.0)
				{
					if(vel <= ref_vel)
					{
						pwm_level = pwm_level + 1;
					}
					else if(vel > ref_vel)
					{
						pwm_level = pwm_level - 1;
					}
				}
				else if((fabs(ref_vel-vel) <= 260.0) & (fabs(ref_vel-vel) > 40.0))
				{
					if((count_to_clear_lcd % 5) == 4)
					{
						if(vel <= ref_vel)
						{
							pwm_level = pwm_level + 1;
						}
						else if(vel > ref_vel)
						{
							pwm_level = pwm_level - 1;
						}
					}
				}
				else if(fabs(ref_vel-vel) <= 40.0)
				{
					if((count_to_clear_lcd % 40) == 36)
					{
						if(vel <= ref_vel)
						{
							pwm_level = pwm_level + 1;
						}
						else if(vel > ref_vel)
						{
							pwm_level = pwm_level - 1;
						}
					}
				}
			}
			else if(count_to_clear_lcd >= 60)
			{
				count_to_clear_lcd = 0;
				lcd_clear();
				lcd_put_cur(0, 0);
				lcd_send_string("Predkosc:");
				vel = freq*60/4;
				uart_buf_len = sprintf(led_buf, "%uRPM",vel);
				lcd_send_string(led_buf);
				lcd_put_cur(1, 0);
				uart_buf_len = sprintf(led_buf, "PWM:%d ",pwm_level/10);
				lcd_send_string(led_buf);
				uart_buf_len = sprintf(led_buf, "Ref.:%f",ref_vel);
				lcd_send_string(led_buf);
			}
		}
	}

	__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 1000.0-pwm_level);
}


void UART_StartReception_IT(void) {
	char *charTable = (char *)malloc(10 * sizeof(char));
	if (HAL_UART_Receive_DMA(&huart2, (uint8_t*)single_message_received, strlen("STARTMOTOR;")) == HAL_OK) {
    	sscanf((char*)single_message_received,"%c%c%c%c%c%c%c%c%c%c;",&charTable[0],&charTable[1],&charTable[2],&charTable[3],&charTable[4],&charTable[5],&charTable[6],&charTable[7],&charTable[8],&charTable[9]);
    	if((charTable[0] == 'S') & (charTable[1] == 'T') & (charTable[2] == 'A') & (charTable[3] == 'R') & (charTable[4] == 'T') & (charTable[5] == 'M') & (charTable[6] == 'O') & (charTable[7] == 'T') & (charTable[8] == 'O') & (charTable[9] == 'R'))
    	{
    		brake_dyn = 0;
    		en_brake = 0;
    		sprintf(single_message_response, "Silnik zalaczony!\r\n");
    		HAL_UART_Transmit(&huart2, (uint8_t*)single_message_response, strlen(single_message_response), 50000);
    	}
    	if((charTable[0] == 'B') & (charTable[1] == 'R') & (charTable[2] == 'A') & (charTable[3] == 'K') & (charTable[4] == 'E') & (charTable[5] == '_') & (charTable[6] == 'D') & (charTable[7] == 'N') & (charTable[8] == 'M') & (charTable[9] == 'C'))
    	{
    		brake_dyn = 1;
    		en_brake = 0;
    		sprintf(single_message_response, "Hamowanie dynamiczne!\r\n");
        	HAL_UART_Transmit(&huart2, (uint8_t*)single_message_response, strlen(single_message_response), 50000);

    	}
    	if((charTable[0] == 'C') & (charTable[1] == 'O') & (charTable[2] == 'N') & (charTable[3] == 'T') & (charTable[4] == 'R') & (charTable[5] == '_') & (charTable[6] == 'S') & (charTable[7] == 'T') & (charTable[8] == 'O') & (charTable[9] == 'P'))
    	{
    		en_brake = 1;
    		brake_dyn = 0;
    		sprintf(single_message_response, "Aktywne hamowanie!\r\n");
        	HAL_UART_Transmit(&huart2, (uint8_t*)single_message_response, strlen(single_message_response), 50000);
    	}
    	if((charTable[0] == 'V') & (charTable[1] == 'E') & (charTable[2] == 'L') & (charTable[3] == ':') & (charTable[8] == ';') & (charTable[9] == ';'))
    	{
    		int num1int = charTable[4] - 48;
    		int num2int = charTable[5] - 48;
    		int num3int = charTable[6] - 48;
    		int num4int = charTable[7] - 48;
    		int ref_vel_local = 0;
    		ref_vel_local = num1int*1000 + num2int*100 + num3int*10 + num4int*1;
    		ref_vel = ref_vel_local;
    		sprintf(single_message_response, "Nowa predkosc: %f\r\n",ref_vel);
        	HAL_UART_Transmit(&huart2, (uint8_t*)single_message_response, strlen(single_message_response), 50000);
    	}
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_ADC1_Init();
  MX_TIM8_Init();
  MX_TIM16_Init();
  MX_TIM2_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_Base_Start(&htim3);
  HAL_TIM_Base_Start(&htim16);
  HAL_TIM_Base_Start(&htim8);

  lcd_init ();

  //pwm_level = 1000.0;

  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

  if(brake_dyn == 0)
  {
	  //HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
	  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 1000.0-pwm_level); //wypelnienie
  }
  else if(brake_dyn == 1)
  {
	  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 1000.0);
	  HAL_GPIO_WritePin(BRAKE_DYN_GPIO_Port, BRAKE_DYN_Pin, GPIO_PIN_SET);
  }

  UART_StartReception_IT();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  HAL_ADC_Start(&hadc1);

	  if(HAL_ADC_PollForConversion(&hadc1, ADC1_TIMEOUT) == HAL_OK)
	  {
		  pot1_mV = ADC_REG2VOLTAGE(HAL_ADC_GetValue(&hadc1));
	  }

		  if(brake_dyn == 1)
		  {
			  HAL_GPIO_WritePin(BRAKE_DYN_GPIO_Port, BRAKE_DYN_Pin, GPIO_PIN_SET);
			  HAL_GPIO_WritePin(BRAKE_EN_GPIO_Port, BRAKE_EN_Pin, GPIO_PIN_RESET);
			  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 1000.0-0.0);
			  if(pot1_mV < 5)
			  {
				  lcd_clear();
				  lcd_put_cur(0, 0);
				  lcd_send_string("HAMOWANIE");
				  lcd_put_cur(1, 0);
				  lcd_send_string("DYNAMICZNE!");
				  timer_val = 0;
			  }
			  HAL_Delay(2);
		  }
		  else if(en_brake == 1)
		  {
			  HAL_GPIO_WritePin(BRAKE_DYN_GPIO_Port, BRAKE_DYN_Pin, GPIO_PIN_RESET);
			  HAL_GPIO_WritePin(BRAKE_EN_GPIO_Port, BRAKE_EN_Pin, GPIO_PIN_SET);
			  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 1000.0-0.0);
			  if(pot1_mV < 5)
			  {
				  lcd_clear();
				  lcd_put_cur(0, 0);
				  lcd_send_string("HAMOWANIE");
				  lcd_put_cur(1, 0);
				  lcd_send_string("AKTYWNE!");
				  timer_val = 0;
			  }
			  HAL_Delay(2);
		  }
		  else if(brake_dyn == 0)
		  {
			  HAL_GPIO_WritePin(BRAKE_DYN_GPIO_Port, BRAKE_DYN_Pin, GPIO_PIN_RESET);
			  HAL_GPIO_WritePin(BRAKE_EN_GPIO_Port, BRAKE_EN_Pin, GPIO_PIN_RESET);
			  pwm_control(pot1_mV, previous_value);
			  previous_value = pot1_mV;
		  }

//	  char *charTable = (char *)malloc(10 * sizeof(char));
//	  uart2_received_status = HAL_UART_Receive(&huart2, (uint8_t*)single_message_received, strlen("STARTMOTOR;"), 50000);
/*	  if(uart2_received_status == HAL_OK)
	  {
		  sscanf((char*)single_message_received,"%c%c%c%c%c%c%c%c%c%c;",&charTable[0],&charTable[1],&charTable[2],&charTable[3],&charTable[4],&charTable[5],&charTable[6],&charTable[7],&charTable[8],&charTable[9]);
		  if((charTable[0] == 'S') & (charTable[1] == 'T') & (charTable[2] == 'A') & (charTable[3] == 'R') & (charTable[4] == 'T') & (charTable[5] == 'M') & (charTable[6] == 'O') & (charTable[7] == 'T') & (charTable[8] == 'O') & (charTable[9] == 'R'))
		  {
			  brake_dyn = 0;
		  }
	  }
*/
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_TIM1
                              |RCC_PERIPHCLK_TIM16|RCC_PERIPHCLK_TIM8
                              |RCC_PERIPHCLK_ADC12|RCC_PERIPHCLK_TIM2
                              |RCC_PERIPHCLK_TIM34;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.Adc12ClockSelection = RCC_ADC12PLLCLK_DIV1;
  PeriphClkInit.Tim1ClockSelection = RCC_TIM1CLK_HCLK;
  PeriphClkInit.Tim16ClockSelection = RCC_TIM16CLK_HCLK;
  PeriphClkInit.Tim8ClockSelection = RCC_TIM8CLK_HCLK;
  PeriphClkInit.Tim2ClockSelection = RCC_TIM2CLK_HCLK;
  PeriphClkInit.Tim34ClockSelection = RCC_TIM34CLK_HCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART2)
	{
		UART_StartReception_IT();
	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
