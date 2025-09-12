#include "uwb.h"
#include "main.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_uart.h"
#include "uart.h"
#include "util.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define IS_RX_IT (1)
uint8_t rx_cnt = 0;
uwb_state_t uwb_state = UWB_OK;
uwb_beacon uwb_beacon_info_global[4] = {};
extern UART_HandleTypeDef huart1;
int uwb_send(char *msg) {
  uwb_wakeup();
  return HAL_UART_Transmit(&huart1, msg, strlen(msg), 100);
}

int uwb_receive(char *msg_container) {
  char rx_char = 0;
  uint8_t cnt = 0;
  for (;;) {
    HAL_UART_Receive(&huart1, &rx_char, 1, 100);
    if (rx_char == '\0') {
      break;
    }
    *msg_container++ = rx_char;
    cnt++;
    rx_char = 0;
  }
  *msg_container = '\0';
  return cnt;
}

ParsedResult *find_results(uint8_t *buffer, uint32_t num) {
  ParsedResult *ret = 0, **tmp = 0;
  tmp = &ret;
  for (uint32_t i = 0; i < num; i++) {
    if (buffer[i] == 'O') { // OK\r\n
      *tmp = malloc(sizeof(ParsedResult));
      (*tmp)->next = NULL;
      (*tmp)->type = KEYWORD_OK;
      (*tmp)->value = 0;
      tmp = &((*tmp)->next);
      i += 4 - 1;
      break;
    } else if (buffer[i] == 'B') {
      *tmp = malloc(sizeof(ParsedResult));
      (*tmp)->next = NULL;
      (*tmp)->type = KEYWORD_BUSY;
      (*tmp)->value = 0;
      tmp = &((*tmp)->next);
      i += 6 - 1;
      break;
    } else if (buffer[i] == '+') {
      i += 7;
      if (buffer[i] == 'N') { // +BeaconNum
        i += 4;
        *tmp = malloc(sizeof(ParsedResult));
        (*tmp)->next = NULL;
        (*tmp)->type = KEYWORD_BEACON_NUM;
        (*tmp)->value = (uint32_t *)atoi(&buffer[i]);
        uint32_t beacon_num = (uint32_t)(*tmp)->value;
        tmp = &((*tmp)->next);
        if (beacon_num >= 1) {
          i++;
          uwb_beacon *beacons = 0;
          uint8_t *data_index = 0;
          uint8_t len_user_define = 0;
          for (uint32_t j = 0; j < beacon_num; j++) {
            *tmp = malloc(sizeof(ParsedResult));
            beacons = malloc(sizeof(uwb_beacon));
            (*tmp)->next = NULL;
            (*tmp)->type = KEYWORD_BEACON_INFO;
            (*tmp)->value = beacons;
            tmp = &((*tmp)->next);
            data_index = find_pattern(buffer + i, "+BeaconInfo:id->",
                                      RX_BUFFER_LENGTH - i + 1,
                                      strlen("+BeaconInfo:id->"));
            beacons->id = atoi(data_index);
            data_index =
                find_pattern(data_index, "distance->", RX_BUFFER_LENGTH - i + 1,
                             strlen("distance->"));
            beacons->dist = atoi(data_index);
            data_index =
                find_pattern(data_index, "life_time->",
                             RX_BUFFER_LENGTH - i + 1, strlen("life_time->"));
            beacons->life_time = atoi(data_index);
            data_index =
                find_pattern(data_index, "battery->", RX_BUFFER_LENGTH - i + 1,
                             strlen("battery->"));
            beacons->battery = atoi(data_index);
            data_index =
                find_pattern(data_index, "user_define->",
                             RX_BUFFER_LENGTH - i + 1, strlen("user_define->"));
            beacons->user_define = atoi(data_index);
            for (len_user_define = 0; data_index[len_user_define] != '\n';
                 len_user_define++)
              ;
            i = data_index - buffer + len_user_define;
          }
        }
      }
    }
  }
  return ret;
}

void uwb_parse(uint8_t *buffer, uint32_t buffer_len) {
#ifdef IS_RX_IT
  ParsedResult *results = find_results(buffer, buffer_len);
  ParsedResult *tobe_free = results;
  uint32_t beacon_num = 0;
  while (results != NULL) {
    if (results->type == KEYWORD_OK) {
      uwb_state = UWB_OK;
      tobe_free = results;
      results = results->next;
      while (tobe_free == NULL)
        ;
      free(tobe_free);
    } else if (results->type == KEYWORD_BUSY) {
      uwb_state = UWB_BUSY;
      tobe_free = results;
      results = results->next;
      while (tobe_free == NULL)
        ;
      free(tobe_free);
    } else if (results->type == KEYWORD_BEACON_NUM) {
      beacon_num = (uint32_t)results->value;
      if (beacon_num == 0) {
        memset(uwb_beacon_info_global, 0, sizeof(uwb_beacon) * 4);
      }
      tobe_free = results;
      results = results->next;
      while (tobe_free == NULL)
        ;
      free(tobe_free);
    } else if (results->type == KEYWORD_BEACON_INFO) {
      void *uwb_beacon_tobe_free = results->value;
      for (uint32_t i = 0; i < beacon_num; i++) {
        uwb_beacon *tmp = &((uwb_beacon *)(results->value))[i];
        uwb_beacon_info_global[i].id = tmp->id;
        uwb_beacon_info_global[i].dist = tmp->dist;
        uwb_beacon_info_global[i].life_time = tmp->life_time;
        uwb_beacon_info_global[i].battery = tmp->battery;
        uwb_beacon_info_global[i].user_define = tmp->user_define;
        tobe_free = results;
        results = results->next;
        while (tobe_free == NULL)
          ;
        free(tobe_free);
      }
      while (tobe_free == NULL)
        ;
      free(uwb_beacon_tobe_free);
    }
  }

  return;
#else
  if (uwb_wait() != 0) {
    *num = 0;
    return NULL;
  }
  char *data_index = find_pattern(rx_buffer, "+BeaconNum:", RX_BUFFER_LENGTH,
                                  strlen("+BeaconNum:"));
  uint8_t beacon_num = atoi(data_index);
  uwb_beacon *ret = malloc(sizeof(uwb_beacon) * beacon_num);
  memset(ret, 0, sizeof(uwb_beacon) * beacon_num);

  for (int i = 0; i < beacon_num; i++) {
    data_index = find_pattern(data_index, "+BeaconInfo:id->",
                              RX_BUFFER_LENGTH - (data_index - rx_buffer),
                              strlen("+BeaconInfo:id->"));
    uint8_t beacon_info_id = atoi(data_index);

    data_index = find_pattern(data_index, "distance->",
                              RX_BUFFER_LENGTH - (data_index - rx_buffer),
                              strlen("distance->"));
    uint8_t beacon_info_distance = atoi(data_index);

    data_index = find_pattern(data_index, "life_time->",
                              RX_BUFFER_LENGTH - (data_index - rx_buffer),
                              strlen("life_time->"));
    uint8_t beacon_info_life_time = atoi(data_index);

    data_index = find_pattern(data_index, "battery->",
                              RX_BUFFER_LENGTH - (data_index - rx_buffer),
                              strlen("battery->"));
    uint8_t beacon_info_battery = atoi(data_index);

    data_index = find_pattern(data_index, "user_define->",
                              RX_BUFFER_LENGTH - (data_index - rx_buffer),
                              strlen("user_define->"));
    uint8_t beacon_info_user_define = atoi(data_index);

    ret[i].id = beacon_info_id;
    ret[i].dist = beacon_info_distance;
    ret[i].life_time = beacon_info_life_time;
    ret[i].battery = beacon_info_battery;
    ret[i].user_define = beacon_info_user_define;
  }

  *num = beacon_num;
  memset(rx_buffer, 0, RX_BUFFER_LENGTH);
  return ret;
#endif /* ifdef IS_RX_IT */
}

void uwb_wakeup(void) {
  HAL_GPIO_WritePin(WAKE_UP_GPIO_Port, WAKE_UP_Pin, GPIO_PIN_SET);
  HAL_Delay(15);
  HAL_GPIO_WritePin(WAKE_UP_GPIO_Port, WAKE_UP_Pin, GPIO_PIN_RESET);
}

void uwb_cmd(const char *cfg) {
  while (uwb_state != UWB_OK) {
    process_buffer();
  }
  uwb_state = UWB_BUSY;
  uwb_wakeup();
  send_string(cfg);
  process_buffer();
}
