# STM32 Analog Clock Project

This repository contains the source code and documentation for an
independent three-hand analog clock implemented using the STM32F407
microcontroller.

## Features
- Independent seconds, minutes, and hours hands
- Stepper motors driven via ULN2003
- Fractional accumulator algorithm for accurate timekeeping
- User button for time adjustment
- Modular embedded C firmware

## Hardware Used
- STM32F407 Development Board
- 28BYJ-48 Stepper Motors (3x)
- ULN2003 Motor Drivers
- Push Button
- External 5V Power Supply

## GPIO Mapping
- Seconds: PB0–PB3
- Minutes: PB4–PB7
- Hours: PB8–PB11
- Button: PA0

## Author
Waleed Hassan  
Department of Electrical Engineering  
UET Lahore (New Campus)
