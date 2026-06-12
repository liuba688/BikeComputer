#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <stdlib.h>

/*
 * ProMicro nRF52840 UART raw bridge test
 *
 * Target physical pins:
 *   UART TX: P0.06  -> connect to external device RX
 *   UART RX: P0.08  -> connect to external device TX
 *
 * Important when compiling as "Adafruit Feather nRF52840 Express":
 *   Arduino D11 maps to P0.06
 *   Arduino D12 maps to P0.08
 */

static const uint32_t USB_BAUD = 115200U;
static const uint32_t DEFAULT_UART_BAUD = 115200U;
static const uint8_t UART_RX_PIN = 12U;  /* Arduino D12 -> P0.08 */
static const uint8_t UART_TX_PIN = 11U;  /* Arduino D11 -> P0.06 */
static const uint32_t HEARTBEAT_PERIOD_MS = 1000U;
static const size_t CMD_BUFFER_SIZE = 16U;

static bool usb_banner_printed = false;
static bool uart_started = false;
static uint32_t uart_baud = DEFAULT_UART_BAUD;
static uint32_t last_heartbeat_ms = 0U;
static uint32_t uart_rx_count = 0U;
static char cmd_buffer[CMD_BUFFER_SIZE];
static size_t cmd_length = 0U;

static void ConfigureUart(uint32_t baud)
{
  if (uart_started)
  {
    Serial1.end();
  }

  uart_baud = baud;
  pinMode(UART_RX_PIN, INPUT_PULLUP);
  Serial1.setPins(UART_RX_PIN, UART_TX_PIN);
  Serial1.begin(uart_baud);
  uart_started = true;
}

static void PrintUsbBanner(void)
{
  Serial.println("NRF52840 UART Ready");
  Serial.println("UART TX = P0.06 / Arduino D11");
  Serial.println("UART RX = P0.08 / Arduino D12");
  Serial.println("Type 9600 or 115200 then Enter to change UART baud.");
  Serial.print("UART baud = ");
  Serial.println(uart_baud);
}

static void ProcessCommand(const char *cmd)
{
  uint32_t new_baud = (uint32_t)strtoul(cmd, NULL, 10);

  if ((new_baud == 9600U) || (new_baud == 38400U) ||
      (new_baud == 57600U) || (new_baud == 115200U))
  {
    ConfigureUart(new_baud);
    uart_rx_count = 0U;
    Serial.print("UART baud changed to ");
    Serial.println(uart_baud);
  }
  else if ((cmd[0] == 'h') || (cmd[0] == 'H') || (cmd[0] == '?'))
  {
    PrintUsbBanner();
  }
  else if (cmd[0] != '\0')
  {
    Serial.println("Unknown command. Use 9600, 38400, 57600, 115200, or h.");
  }
}

static void PollUsbCommands(void)
{
  while (Serial.available() > 0)
  {
    char ch = (char)Serial.read();

    if ((ch == '\r') || (ch == '\n'))
    {
      cmd_buffer[cmd_length] = '\0';
      ProcessCommand(cmd_buffer);
      cmd_length = 0U;
    }
    else if (cmd_length < (CMD_BUFFER_SIZE - 1U))
    {
      cmd_buffer[cmd_length++] = ch;
    }
  }
}

void setup()
{
  Serial.begin(USB_BAUD);
  ConfigureUart(DEFAULT_UART_BAUD);
}

void loop()
{
  if (Serial && !usb_banner_printed)
  {
    PrintUsbBanner();
    usb_banner_printed = true;
    last_heartbeat_ms = millis();
  }

  if (Serial)
  {
    PollUsbCommands();
  }

  while (Serial1.available() > 0)
  {
    int ch = Serial1.read();
    uart_rx_count++;

    if (Serial)
    {
      Serial.write((uint8_t)ch);
    }
  }

  if (Serial && ((millis() - last_heartbeat_ms) >= HEARTBEAT_PERIOD_MS))
  {
    last_heartbeat_ms = millis();
    Serial.print("NRF HEARTBEAT baud=");
    Serial.print(uart_baud);
    Serial.print(" uartBytes=");
    Serial.println(uart_rx_count);
  }
}
