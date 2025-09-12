#include "main.h"
#include <stdint.h>

typedef struct uwb_beacon_ {
  uint32_t id;
  uint16_t dist;
  uint16_t life_time;
  uint8_t battery;
  uint8_t user_define;
  // struct uwb_beacon_ *next;
} uwb_beacon;

typedef enum uwb_state_t_ {
  UWB_OK = 0,
  UWB_BUSY,
  UWB_ERROR,
} uwb_state_t;

typedef enum {
  KEYWORD_UNKNOWN = 0,
  KEYWORD_OK,          // "OK"
  KEYWORD_BUSY,        // "BUSY"
  KEYWORD_BEACON_NUM,  // "BeaconNum:"
  KEYWORD_BEACON_INFO, // "BeaconInfo:"
  KEYWORD_ID,          // "id->"
  KEYWORD_DISTANCE,    // "distance->"
  KEYWORD_LIFE_TIME,   // "life_time->"
  KEYWORD_BATTERY,     // "battery->"
  KEYWORD_USER_DEFINE  // "user_define->"
} KeywordType;

typedef struct ParsedResult_ {
  KeywordType type;
  void *value;
  struct ParsedResult_ *next;
} ParsedResult;

int uwb_send(char *msg);
void uwb_parse(uint8_t *buffer, uint32_t buffer_len);
void uwb_wakeup(void);
void uwb_cmd(const char *cfg);
