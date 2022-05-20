# pico16470

This code interacts with an ADIS IMU like the 16470 or 16505 to sample measurements over SPI at the maximum rate of 1Mhz and communicates the data over USB.
It runs on any RP2040 processor.
This project offloads the sampling work from the main computer, samples over SPI much faster than a USB converter, and provides an interface for real-time interaction with the IMU from non-realtime userspace of an ordinary computer.
It is based on the [iSensor-SPI-Buffer](https://github.com/ajn96/iSensor-SPI-Buffer) repository.
