#include "main.h"
#include <string.h>
#define RX_BUFFER_LENGTH (512)

void m_start_receive(void);
int m_send_Nchar(uint8_t *str, uint32_t n);
int send_string(uint8_t *str);
void low_level_init(void);
void process_buffer(void);
