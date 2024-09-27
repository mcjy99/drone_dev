#include <Arduino.h>
#include "LoRaWan-Arduino.h" // Include LoRa library
#include <SPI.h>
#include <stdio.h>

//function declaration
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void OnRxTimeout(void);
void OnRxError(void);

//define LoRa parameters
#define RF_FREQUENCY 868300000  // Hz
#define TX_OUTPUT_POWER 22     // dBm
#define LORA_BANDWIDTH 0       // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR 7 // [SF7..SF12]
#define LORA_CODINGRATE 1       // [1: 4/5, 2: 4/6,  3: 4/7,  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8  // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0   // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define RX_TIMEOUT_VALUE 3000

static RadioEvents_t RadioEvents;
static uint8_t RcvBuffer[64];

void setup()
{
  //initialize serial
  Serial.begin(115200);
  while (!Serial) {
    delay(100);
  }

  //initialize LoRa chip.
  lora_rak11300_init();
  Serial.println("=====================================");
  Serial.println("Gesture Control Rx");
  Serial.println("=====================================");

  //initialize the Radio callbacks
  RadioEvents.TxDone = NULL;
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.TxTimeout = NULL;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;
  RadioEvents.CadDone = NULL;

  //initialize the radio
  Radio.Init(&RadioEvents);

  //set radio channel
  Radio.SetChannel(RF_FREQUENCY);

  //set radio rx configuration
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

  //start LoRa
  Serial.println("Starting Radio.Rx");
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void loop()
{
  // Empty loop - all work is handled in the RX callback
}

/**@brief Function to be executed on Radio Rx Done event
*/
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
    /*Serial.println("OnRxDone");*/

    // ensure received size does not exceed buffer size
    if (size < sizeof(RcvBuffer)) {
        memcpy(RcvBuffer, payload, size);
        RcvBuffer[size] = '\0';  //null-terminate the buffer for string operations

        //print the received payload
        /*Serial.print("Received payload: ");*/
        Serial.println((char *)RcvBuffer);  //cast buffer to char* to print as string
    } else {
        Serial.println("Received data size exceeds buffer size");
    }

    //restart listening for new messages
    Radio.Rx(RX_TIMEOUT_VALUE);
}

/**@brief Function to be executed on Radio Rx Timeout event
*/
void OnRxTimeout(void)
{
  Serial.println("OnRxTimeout");
  Radio.Rx(RX_TIMEOUT_VALUE);
}

/**@brief Function to be executed on Radio Rx Error event
*/
void OnRxError(void)
{
  Serial.println("OnRxError");
  Radio.Rx(RX_TIMEOUT_VALUE);
}
