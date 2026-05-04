## About

This repository contains embedded system projects developed on the STM32F103C8T6 (Blue Pill) microcontroller, focusing on real-time control, peripheral integration, and system-level design.

A key implementation is an RTOS-based vending machine system built using FreeRTOS. The system is structured using multiple concurrent tasks for keypad input, coin detection, motor control, ultrasonic sensing, display handling, and buzzer feedback.

The application implements a complete state machine including idle operation, coin insertion, item and quantity selection, balance verification, dispensing, and error handling. It supports multi-quantity purchases, dynamic stock and price management, and a password-protected restock mode.

Hardware interfaces include:

* I2C-based 16x2 LCD for user interaction
* UART for debugging and system monitoring
* GPIO-based keypad for input
* IR sensor with interrupt for coin detection
* Ultrasonic sensor for drop verification
* PWM-controlled motors using a driver (L293D)
* Buzzer for feedback signals

The system emphasizes reliable operation through event-driven design, task synchronization using queues and signals, and validation mechanisms such as sensor-based confirmation before transaction completion.

Overall, the work demonstrates practical experience in embedded C, STM32 HAL, FreeRTOS task management, and integration of multiple hardware modules into a cohesive real-time application.
