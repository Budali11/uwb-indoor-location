#include "uart.h"
#include "stm32f103xb.h"
#include "stm32f1xx.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_dma.h"
#include "stm32f1xx_hal_uart.h"
#include "uwb.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

uint8_t rx_buffer[RX_BUFFER_LENGTH] = {};
extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern DMA_HandleTypeDef hdma_usart1_rx;
void m_start_receive(void) {
  // __HAL_DMA_DISABLE(&hdma_usart1_rx);
  // SET_BIT(USART1->CR3, USART_CR3_DMAR);
  // DMA1_Channel5->CPAR = USART1->DR;
  // DMA1_Channel5->CMAR = (volatile uint32_t)rx_buffer;
  // DMA1_Channel5->CNDTR = RX_BUFFER_LENGTH;
  // SET_BIT(DMA1_Channel5->CCR, DMA_CCR_MINC);
  // __HAL_DMA_ENABLE(&hdma_usart1_rx);

  __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
  HAL_UART_Receive_DMA(&huart1, rx_buffer, RX_BUFFER_LENGTH);
}
int m_send_Nchar(uint8_t *str, uint32_t n) {
  __HAL_DMA_DISABLE(&hdma_usart1_tx);
  SET_BIT(DMA1_Channel4->CCR, DMA_MINC_ENABLE);
  SET_BIT(USART1->CR3, USART_CR3_DMAT);
  SET_BIT(DMA1_Channel4->CCR, DMA_MEMORY_TO_PERIPH);
  if (HAL_DMA_Start_IT(&hdma_usart1_tx, (uint32_t)str, (uint32_t)&(USART1->DR),
                       n) != HAL_OK) {
    return HAL_ERROR;
  }
  return HAL_OK;
}
int send_string(uint8_t *str) {
  uint32_t i = 0;
  uint8_t *pstr = (uint8_t *)str;
  for (; pstr[i] != '\0'; i++)
    ;
  if (HAL_UART_Transmit(&huart1, pstr, i, 100) != HAL_OK)
    return HAL_ERROR;
  return HAL_OK;
}
void low_level_init(void) { memset(rx_buffer, 0, RX_BUFFER_LENGTH); }

extern uint32_t cur_rx_ptr;
extern uint32_t old_rx_ptr;
extern uint8_t buffer_update;
void process_buffer(void) {
  while (buffer_update == 0)
    ;
  buffer_update = 0;
  if (cur_rx_ptr == old_rx_ptr)
    return;
  char *recv = &rx_buffer[old_rx_ptr];
  char *rx_buffer_rep = 0;
  uint16_t recv_num = 0;
  if (cur_rx_ptr > old_rx_ptr) {
    recv_num = cur_rx_ptr - old_rx_ptr;
    rx_buffer_rep = malloc(recv_num);
    memcpy(rx_buffer_rep, recv, recv_num);
  } else if (cur_rx_ptr < old_rx_ptr) {
    recv_num = RX_BUFFER_LENGTH - old_rx_ptr + cur_rx_ptr;
    rx_buffer_rep = malloc(recv_num);
    memcpy(rx_buffer_rep, recv, RX_BUFFER_LENGTH - old_rx_ptr);
    if (cur_rx_ptr != 0) {
      memcpy(rx_buffer_rep + RX_BUFFER_LENGTH - old_rx_ptr, rx_buffer,
             cur_rx_ptr - 1);
    }
  }
  uwb_parse(rx_buffer_rep, recv_num);
  while (rx_buffer_rep == NULL)
    ;
  free(rx_buffer_rep);
}
