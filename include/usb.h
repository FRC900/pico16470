#ifndef INC_USB_H_
#define INC_USB_H_

/* Header includes require for prototypes */
#include <stdint.h>

/* Public function prototypes */

void USB_Rx_Handler();
bool USB_Wait_For_Tx_Done(uint32_t TimeoutMs);
void USB_Tx_Handler(const uint8_t* buf, uint32_t count);
//void USB_Watermark_Autoset();
//void USB_Reset();

#endif /* INC_USB_H_ */
