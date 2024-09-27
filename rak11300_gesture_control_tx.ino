#include <Arduino.h>
#include "SparkFunLIS3DH.h" //http://librarymanager/All#SparkFun-LIS3DH
#include <Wire.h>
#include <control2_inferencing.h>
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"  // Include the Edge Impulse model header
#include "LoRaWan-Arduino.h" 

#define BUFFER_SIZE 64
#define CONVERT_G_TO_MS2    9.80665f
#define FREQUENCY_HZ        50
#define INTERVAL_MS         (1000 / (FREQUENCY_HZ + 1))

LIS3DH SensorTwo(I2C_MODE, 0x18);

//sensor data buffer
float input_buf[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE]={0};

char sendBuffer[BUFFER_SIZE] = {0};

//LoRa parameters
#define RF_FREQUENCY 868300000  // Hz
#define TX_OUTPUT_POWER 22      // dBm
#define LORA_BANDWIDTH 0        // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR 7 // [SF7..SF12]
#define LORA_CODINGRATE 1       // [1: 4/5, 2: 4/6,  3: 4/7,  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8  // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0   // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define TX_TIMEOUT_VALUE 3000

//function declaration
void sendGesture(const char *gesture);
static int get_signal_data(size_t offset, size_t length, float *out_ptr);

//capture accelerometer data and store it in the buffer
void lis3dh_get(float *x, float *y, float *z) {
    *x = SensorTwo.readFloatAccelX() * CONVERT_G_TO_MS2;
    *y = SensorTwo.readFloatAccelY() * CONVERT_G_TO_MS2;
    *z = SensorTwo.readFloatAccelZ() * CONVERT_G_TO_MS2;
}
/*//capture accelerometer data and store it in the buffer
void lis3dh_get(float* x, float* y, float* z) {
    *x = SensorTwo.readFloatAccelX() * CONVERT_G_TO_MS2;
    *y = SensorTwo.readFloatAccelY() * CONVERT_G_TO_MS2;
    *z = SensorTwo.readFloatAccelZ() * CONVERT_G_TO_MS2;

  Serial.print(*x);
  Serial.print(',');
  Serial.print(*y);
  Serial.print(',');
  Serial.println(*z);
}*/

void setup()
{
	time_t timeout = millis();
	Serial.begin(115200);
	while (!Serial)
	{
		if ((millis() - timeout) < 5000)
    {
      delay(100);
    }
    else
    {
      break;
    }
	}

	if (SensorTwo.begin() != 0)
	{
		Serial.println("Problem starting the sensor at 0x18.");
	}
	else
	{
		Serial.println("Sensor at 0x18 started.");
		// Set low power mode
		uint8_t data_to_write = 0;
		SensorTwo.readRegister(&data_to_write, LIS3DH_CTRL_REG1);
		data_to_write |= 0x08;
		SensorTwo.writeRegister(LIS3DH_CTRL_REG1, data_to_write);
		delay(100);

		data_to_write = 0;
		SensorTwo.readRegister(&data_to_write, 0x1E);
		data_to_write |= 0x90;
		SensorTwo.writeRegister(0x1E, data_to_write);
		delay(100);
	}
	//initialize LoRa
    lora_rak11300_init();
    Serial.println("=====================================");
    Serial.println("Gesture Control Tx");
    Serial.println("=====================================");
    RadioEvents_t RadioEvents;
    RadioEvents.TxDone = NULL;
    RadioEvents.RxDone = NULL;
    RadioEvents.TxTimeout = NULL;
    RadioEvents.RxTimeout = NULL;
    RadioEvents.RxError = NULL;
    RadioEvents.CadDone = NULL;
    Radio.Init(&RadioEvents);
    Radio.SetChannel(RF_FREQUENCY);
    Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                      LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                      LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                      true, 0, 0, LORA_IQ_INVERSION_ON, TX_TIMEOUT_VALUE);

    Serial.println("Setup completed!");
}

void loop()
{
    static size_t buffer_index = 0;
    float x,y,z;
    const char *gesture = "idle";

    //get accelerometer data
    lis3dh_get(&x, &y, &z);

    //store accelerometer data in input buffer
    input_buf[buffer_index++] = x;
    input_buf[buffer_index++] = y;
    input_buf[buffer_index++] = z;

    if (buffer_index >= EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        //reset the buffer index for the next round of data collection
        buffer_index = 0;

    //prepare signal for inference
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    signal.get_data = &get_signal_data;

    //run inference
    ei_impulse_result_t result;
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

     //check for successful inference
        if (res == EI_IMPULSE_OK) {
            if (result.classification[0].value > 0.55) {
                gesture = "forward";
            } else if (result.classification[1].value > 0.80) {
                gesture = "idle";
            } else if (result.classification[2].value > 0.60) {
                gesture = "left";
            } else if (result.classification[3].value > 0.60) {
                gesture = "right";
            }

            Serial.printf("Gesture: %s\n", gesture);
            sendGesture(gesture);
        } else {
            Serial.println("Inference failed");
        }
    }
    delay(INTERVAL_MS);
}

//function to send gesture to LoRa
void sendGesture(const char *gesture) {
    Serial.printf("Sending gesture via LoRa: %s\n", gesture);
    Radio.Send((uint8_t *)gesture, strlen(gesture));
}

//function to retrieve data for inference
static int get_signal_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, input_buf + offset, length * sizeof(float));
    return 0;
}