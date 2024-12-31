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
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "LCD1602.h"
#include "aio.h"
#include<stdio.h>

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
int zmienna=0;
uint16_t prev_period = 0;
uint16_t freq = 0;
uint16_t cur_period = 0;
int count_to_clear_lcd = 0;

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

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

	char uart_buf[50];
	int uart_buf_len;
	uint16_t timer_val = 0;
	int change_operation = 0;
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
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_ADC1_Init();
  MX_TIM8_Init();
  MX_TIM16_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_Base_Start(&htim3);
  HAL_TIM_Base_Start(&htim16);
  HAL_TIM_Base_Start(&htim8);

  lcd_init ();
/*  lcd_put_cur(0, 0);
  lcd_send_string("HELLO ");
  lcd_send_string("WORLD ");
  lcd_send_string("FROM");

  lcd_put_cur(1, 0);
  lcd_send_string("CONTROLLERS TECH");*/


  //HAL_Delay(3000);
  //lcd_clear();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  /*for (int i=0;i<128;i++)
	  {
		  lcd_put_cur(row, col);

		  lcd_send_data(i+48);

		  col++;

		  if (col > 15) {row++; col = 0;}
		  if (row > 1) row=0;


		  HAL_Delay(250);
	  }*/
//----------------------------------------------------

	  HAL_ADC_Start(&hadc1);

	  if(HAL_ADC_PollForConversion(&hadc1, ADC1_TIMEOUT) == HAL_OK)
	  {
		  pot1_mV = ADC_REG2VOLTAGE(HAL_ADC_GetValue(&hadc1));
	  }

	  if((pot1_mV-previous_value) > 1000)
	  {
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

			  if(count_to_clear_lcd < 80)
			  {
				  count_to_clear_lcd = count_to_clear_lcd + 1;
			  }
			  else if(count_to_clear_lcd >= 80)
			  {
				  count_to_clear_lcd = 0;
	  			  lcd_clear();
				  lcd_put_cur(0, 0);
				  lcd_send_string("Frequency:");
				  uart_buf_len = sprintf(uart_buf, "%u Hz",freq);
				  lcd_send_string(uart_buf);
			  }
		  }
	  }
	  previous_value = pot1_mV;
	  freq=freq;
	  //uart_buf_len = sprintf(uart_buf, "%u Hz\r\n",cur_period);
	  //HAL_UART_Transmit(&huart2, (uint8_t*)uart_buf, uart_buf_len, 1000);

	  DelayUS(7100/2); //142000


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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_TIM16
                              |RCC_PERIPHCLK_TIM8|RCC_PERIPHCLK_ADC12
                              |RCC_PERIPHCLK_TIM34;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.Adc12ClockSelection = RCC_ADC12PLLCLK_DIV1;
  PeriphClkInit.Tim16ClockSelection = RCC_TIM16CLK_HCLK;
  PeriphClkInit.Tim8ClockSelection = RCC_TIM8CLK_HCLK;
  PeriphClkInit.Tim34ClockSelection = RCC_TIM34CLK_HCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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
