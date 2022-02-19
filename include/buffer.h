#ifndef INC_BUFFER_H_
#define INC_BUFFER_H_

/* Header includes require for prototypes */
#include <stdint.h>

/* Public function prototypes */
void Buffer_Reset();
uint8_t* Buffer_Take_Element();
uint8_t* Buffer_Add_Element();
uint32_t Buffer_Can_Add_Element();

/** Buffer memory allocation */
#define BUF_SIZE        0xA000

/** Smallest buffer data entry size. Real size is +10 bytes */
#define BUF_MIN_ENTRY   2

/** Largest buffer data entry size. Real size is +10 bytes */
#define BUF_MAX_ENTRY   64

/* Public variables exported from module */
extern uint32_t g_bufLastRegIndex;
extern uint32_t g_bufCount;
extern uint32_t g_bufNumWords32;

#endif /* INC_BUFFER_H_ */
