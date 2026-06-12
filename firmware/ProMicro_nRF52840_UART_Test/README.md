# ProMicro nRF52840 UART Test

Arduino sketch for verifying UART receive on a ProMicro nRF52840 and forwarding all received bytes to USB serial.

## Pin Mapping

| Signal | nRF52840 Pin | Connect To |
|--------|--------------|------------|
| UART TX | P0.06 | External device RX |
| UART RX | P0.08 | External device TX |
| GND | GND | External device GND |

## Serial Settings

- USB Serial: 115200 baud
- UART Serial1: 115200 baud

Startup message:

```text
NRF52840 UART Ready
```

All bytes received on UART RX P0.08 are written directly to USB serial.

## Arduino IDE board note

When compiling as Adafruit Feather nRF52840 Express, the sketch uses Arduino D12 for physical P0.08 RX and Arduino D11 for physical P0.06 TX.
The Serial Monitor prints a heartbeat once per second so USB CDC can be verified even when no UART data is received.
