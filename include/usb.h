#ifndef INC_USB_H_
#define INC_USB_H_

/* Header includes require for prototypes */
#include <stdint.h>

/* Public function prototypes */

void USB_Rx_Handler();
void USB_Tx_Handler(const uint8_t* buf, uint32_t count);

#endif /* INC_USB_H_ */
