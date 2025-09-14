/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_dma.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_uart.h"
#include "uart.h"
#include "uwb.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#if !(defined(ALERT_MODE))
#define ALERT_MODE (0) // 0: led, 1: beep
#endif

#if !(defined(UWB_DIST_THRESHOLD))
#define UWB_DIST_THRESHOLD (300)
#endif

#if !(defined(UWB_ENABLE_HIGH_PERFORMANCE))
#define UWB_ENABLE_HIGH_PERFORMANCE (1)
#endif

#if !(defined(UWB_SCAN_INTERVAL))
#define UWB_SCAN_INTERVAL 1000
#endif
#define UWB_SCAN(interval)                                                     \
  do {                                                                         \
    char param[16] = {0};                                                      \
    sprintf(param, "AT+SCAN=%d,1\r\n", interval);                              \
    uwb_cmd(param);                                                            \
  } while (0)

#if !(defined(UWB_SCAN_CHANNEL))
#define UWB_SCAN_CHANNEL 5
#endif
#define UWB_SET_CHANNEL(channel)                                               \
  do {                                                                         \
    char param[16] = {0};                                                      \
    sprintf(param, "AT+RANGECH=%d\r\n", channel);                              \
    uwb_cmd(param);                                                            \
  } while (0)
/* USER CODE END PD */

/* Private macro
   -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables
   ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_tx;
DMA_HandleTypeDef hdma_usart1_rx;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
extern uint8_t *rx_buffer;
extern enum uwb_state_t_ uwb_state;
extern uwb_beacon uwb_beacon_info_global[4];
extern uint8_t buffer_update;

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  low_level_init();
  m_start_receive();
  srand(time(NULL));

#ifdef LED_TEST
  while (1) {
    HAL_Delay(100);
    HAL_GPIO_WritePin(TEST_LED_GPIO_Port, TEST_LED_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(TEST_LED_GPIO_Port, TEST_LED_Pin, GPIO_PIN_SET);
  }
#endif

  // test if uwb module is available
  uwb_cmd("AT\r\n");

  // uwb_cmd("AT+DEVID?\r\n");

  // configure channel(5 or 9)
  UWB_SET_CHANNEL(UWB_SCAN_CHANNEL);
  // uwb_cmd((uint8_t *)"AT+RANGECH=5\r\n");

// enable/disable high performance mode
#if (UWB_ENABLE_HIGH_PERFORMANCE == 1)
  uwb_cmd("AT+PERFORMANCE=1\r\n");
#else
  uwb_cmd("AT+PERFORMANCE=0\r\n");
#endif
  // save parameters
  uwb_cmd((uint8_t *)"AT+SAVE\r\n");

  int detected = 0; // how many gateway were detected
  int undetected_times = 0;
  uint32_t delay_time = 0;
  while (1) {
    // wait for module ok
    while (uwb_state != UWB_OK) {
      while (buffer_update == 0) {
        HAL_Delay(100);
        uwb_wakeup();
        send_string((uint8_t *)"AT\r\n");
      }
      process_buffer();
    }
    // uwb_cmd((uint8_t *)"AT+SCAN=500,1\r\n");
    UWB_SCAN(UWB_SCAN_INTERVAL);

#if (ALERT_MODE == 1)
    for (int i = 0; i < 4; i++) {
      if (uwb_beacon_info_global[i].id != 0) {
        if (uwb_beacon_info_global[i].dist < UWB_DIST_THRESHOLD) {
          HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_SET);
        } else {
          HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
        }
      }
    }
#else
    for (int i = 0; i < 4; i++) {
      if (uwb_beacon_info_global[i].id != 0) {
        detected += 1;
        if (uwb_beacon_info_global[i].dist < UWB_DIST_THRESHOLD) {
          HAL_GPIO_WritePin(TEST_LED_GPIO_Port, TEST_LED_Pin, GPIO_PIN_SET);
        } else {
          HAL_GPIO_WritePin(TEST_LED_GPIO_Port, TEST_LED_Pin, GPIO_PIN_RESET);
        }
      }
    }
    if (detected == 0) { // gateway is too far to detect any
      undetected_times++;
      if (undetected_times > 3) {
        // we should give the uwb module and beacon a few channces
        HAL_GPIO_WritePin(TEST_LED_GPIO_Port, TEST_LED_Pin, GPIO_PIN_RESET);
        undetected_times = 0;
      }
      // else {
      //   // and we should wait a few while here, let other tag communicate
      //   uint32_t delay_time = rand() % (undetected_times * 300);
      //   HAL_Delay(delay_time);
      // }
    } else {
      undetected_times = 0;
    }
    detected = 0;
#endif
    // process_buffer();

    // delay_time = rand() % 200 + 600 - undetected_times * 200;
    delay_time = rand() % (650 - undetected_times * 200) +
                 (350 - undetected_times * 100);
    // delay_time = rand() % (850 - undetected_times * 200);
    HAL_Delay(delay_time);
    // HAL_Delay(100);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void) {

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */
}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void) {

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, BEEP_Pin | WAKE_UP_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(UWB_RSTN_GPIO_Port, UWB_RSTN_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(TEST_LED_GPIO_Port, TEST_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : BEEP_Pin */
  GPIO_InitStruct.Pin = BEEP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BEEP_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TEST_LED_Pin */
  GPIO_InitStruct.Pin = TEST_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TEST_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : WAKE_UP_Pin UWB_RSTN_Pin */
  GPIO_InitStruct.Pin = WAKE_UP_Pin | UWB_RSTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
#ifdef __GNUC__
#include <sys/time.h>

int _gettimeofday(struct timeval *tv, void *tzvp) {

  uint64_t t = HAL_GetTick(); // get uptime in microseconds

  tv->tv_sec = t / 1000; // convert to seconds

  tv->tv_usec = t % 1000; // get remaining microseconds

  return 0; // return non-zero for error

} // end _gettimeofday()
#endif
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
#endif /* USE_FULL_ASSERT */
