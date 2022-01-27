#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/spi.h"
#include "imu.h"

void loop() {
    /* Returns the PROD_ID register */
    IMU_Read_Register(0x72);

    sleep_ms(100);
}

int main()
{
    IMU_SPI_Init();

    while (true) {
        loop();
    }
}
