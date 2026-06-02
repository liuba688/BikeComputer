# BikeComputer

## Project Overview

Embedded bike computer based on STM32F411CEU6.

## Hardware

MCU:
- STM32F411CEU6

Display:
- ILI9341 TFT LCD
- Resolution: 240x320
- Interface: SPI1

Communication:
- USART1 for debugging
- I2C1 reserved for sensors

## Development Environment

- STM32CubeMX
- STM32CubeIDE
- ST-Link V2

## Display Module

Model:
- 2.8" TFT SPI LCD Module

LCD Controller:
- ILI9341

Resolution:
- 240x320

Interface:
- SPI

Touch Controller:
- XPT2046

Storage:
- MicroSD Card Slot

### LCD Pin Mapping

| LCD Signal | STM32 Pin |
|------------|-----------|
| CS         | PB0 |
| DC         | PB1 |
| RESET      | PB2 |
| SCK        | PA5 |
| MOSI       | PA7 |
| MISO       | PA6 |

### Optional LCD Pins

| LCD Signal | Function |
|------------|-----------|
| LED        | Backlight |
| VCC        | 3.3V |
| GND        | GND |

### Touch Pins (Reserved)

| Touch Signal | Function |
|--------------|----------|
| T_CLK | XPT2046 SPI Clock |
| T_CS  | Touch Chip Select |
| T_DIN | Touch MOSI |
| T_DO  | Touch MISO |
| T_IRQ | Touch Interrupt |

### SD Card Pins (Reserved)

| Signal | Function |
|---------|----------|
| SD_CS   | SD Card CS |
| SD_MOSI | Shared SPI MOSI |
| SD_MISO | Shared SPI MISO |
| SD_SCK  | Shared SPI Clock |

## Repository Structure

firmware/
- STM32 firmware source code

hardware/
- Schematics and PCB files

docs/
- Project documentation

## Development Rules

- Do not modify STM32CubeMX generated code outside USER CODE sections.
- Keep BikeComputer.ioc synchronized with firmware changes.
- Prefer HAL drivers unless otherwise specified.
- Verify compilation in STM32CubeIDE before committing.

## Current Status

- CubeMX configuration completed
- ST-Link debugging working
- PC13 LED blink test passed