// Obtained from GitHub, Arduino IDE's built-in example was not working
#include <Arduino.h>
#include "LoRaWan-Arduino.h" //http://librarymanager/All#SX126x
#include <SPI.h>

#include <stdio.h>

#include "mbed.h"
#include "rtos.h"

using namespace std::chrono_literals;
using namespace std::chrono;

bool doOTAA = true;   // OTAA is used by default.
#define SCHED_MAX_EVENT_DATA_SIZE APP_TIMER_SCHED_EVENT_DATA_SIZE // Maximum size of scheduler events. 
#define SCHED_QUEUE_SIZE 60										                    // Maximum number of events in the scheduler queue. 
#define LORAWAN_DATERATE DR_5								                    // LoRaMac datarates definition, from DR_0 to DR_5 sf
#define LORAWAN_TX_POWER TX_POWER_5							                  // LoRaMac tx power definition, from TX_POWER_0 to TX_POWER_15
#define JOINREQ_NBTRIALS 3										                    // Number of trials for the join request. 
DeviceClass_t g_CurrentClass = CLASS_A;					                  // Class definition
LoRaMacRegion_t g_CurrentRegion = LORAMAC_REGION_AS923;           // Region:EU868
lmh_confirm g_CurrentConfirm = LMH_UNCONFIRMED_MSG;				        // Confirm/unconfirm packet definition
uint8_t gAppPort = LORAWAN_APP_PORT;							                // Data port

/**@brief Structure containing LoRaWan parameters, needed for lmh_init()
*/
static lmh_param_t g_lora_param_init = {LORAWAN_ADR_ON, LORAWAN_DATERATE, LORAWAN_PUBLIC_NETWORK, JOINREQ_NBTRIALS, LORAWAN_TX_POWER, LORAWAN_DUTYCYCLE_OFF};

// Foward declaration
static void lorawan_has_joined_handler(void);
static void lorawan_join_failed_handler(void);
static void lorawan_rx_handler(lmh_app_data_t *app_data);
static void lorawan_confirm_class_handler(DeviceClass_t Class);
static void send_lora_frame(void);
void lorawan_unconf_finished(void);
void lorawan_conf_finished(bool result);

/**@brief Configure data rate
 *
 * @param data_rate data rate
 * @param enable_adr  enable adaptative data rate
 */
void lmh_datarate_set(uint8_t data_rate, bool enable_adr);

/**@brief Structure containing LoRaWan callback functions, needed for lmh_init()
*/
static lmh_callback_t g_lora_callbacks = {BoardGetBatteryLevel, BoardGetUniqueId, BoardGetRandomSeed,
                                          lorawan_rx_handler, lorawan_has_joined_handler,
                                          lorawan_confirm_class_handler, lorawan_join_failed_handler,
                                          lorawan_unconf_finished, lorawan_conf_finished
                                         };
//OTAA keys !!!! KEYS ARE MSB !!!!
uint8_t nodeDeviceEUI[8] = {0xAC, 0x1F, 0x09, 0xFF, 0xFE, 0x06, 0xB7, 0xE1};
uint8_t nodeAppEUI[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t nodeAppKey[16] = {0x65, 0x8F, 0x63, 0xA4, 0xF1, 0x09, 0xFE, 0x8C, 0xF3, 0x84, 0x89, 0xFE, 0x94, 0xD4, 0x58, 0x4B};

// ABP keys
uint32_t nodeDevAddr = 0x260116F8;
uint8_t nodeNwsKey[16] = {0x7E, 0xAC, 0xE2, 0x55, 0xB8, 0xA5, 0xE2, 0x69, 0x91, 0x51, 0x96, 0x06, 0x47, 0x56, 0x9D, 0x23};
uint8_t nodeAppsKey[16] = {0xFB, 0xAC, 0xB6, 0x47, 0xF3, 0x58, 0x45, 0xC7, 0x50, 0x7D, 0xBF, 0x16, 0x8B, 0xA8, 0xC1, 0x7C};

// Private defination
#define LORAWAN_APP_DATA_BUFF_SIZE 64                                         // Buffer size of the data to be transmitted. 
#define LORAWAN_APP_INTERVAL 7050                                             // Defines for user timer, the application data transmission interval. 20000 = 20s, value in [ms]. 
static uint8_t m_lora_app_data_buffer[LORAWAN_APP_DATA_BUFF_SIZE];            // Lora user application data buffer.
static lmh_app_data_t m_lora_app_data = {m_lora_app_data_buffer, 0, 0, 0, 0}; // Lora user application data structure.

mbed::Ticker appTimer;
void tx_lora_periodic_handler(void);

static uint32_t count = 0;
static uint32_t count_fail = 0;

bool send_now = false;

//Set Spreading Factor
void set_spreading_factor(uint8_t data_rate) {
    uint8_t spreading_factor;
    //Map the spreading factor to the corresponding data rate
    switch (data_rate) {
        case DR_0:
            spreading_factor=12;
            break;
        case DR_1:
            spreading_factor=11;
            break;
        case DR_2:
            spreading_factor=10;
            break;
        case DR_3:
            spreading_factor=9;
            break;
        case DR_4:
            spreading_factor=8;
            break;
        case DR_5:
            spreading_factor=7;
            break;
        default:
            Serial.println("Invalid data rate");
            return;
    }
    //Set the data rate with ADR disabled
    lmh_datarate_set(data_rate, false);
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);


  // Initialize Serial for debug output
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
  // Initialize LoRa chip.
  lora_rak11300_init();
  
  Serial.println("=====================================");
  Serial.println("Welcome to RAK11300 LoRaWan!!!");
  if (doOTAA)
  {
    Serial.println("Type: OTAA");
  }
  else
  {
    Serial.println("Type: ABP");
  }

  switch (g_CurrentRegion)
  {
    case LORAMAC_REGION_AS923:
      Serial.println("Region: AS923");
      break;
    case LORAMAC_REGION_AU915:
      Serial.println("Region: AU915");
      break;
    case LORAMAC_REGION_CN470:
      Serial.println("Region: CN470");
      break;
    case LORAMAC_REGION_CN779:
      Serial.println("Region: CN779");
      break;
    case LORAMAC_REGION_EU433:
      Serial.println("Region: EU433");
      break;
    case LORAMAC_REGION_IN865:
      Serial.println("Region: IN865");
      break;
    case LORAMAC_REGION_EU868:
      Serial.println("Region: EU868");
      break;
    case LORAMAC_REGION_KR920:
      Serial.println("Region: KR920");
      break;
    case LORAMAC_REGION_US915:
      Serial.println("Region: US915");
      break;
    case LORAMAC_REGION_RU864:
      Serial.println("Region: RU864");
      break;
    case LORAMAC_REGION_AS923_2:
      Serial.println("Region: AS923-2");
      break;
    case LORAMAC_REGION_AS923_3:
      Serial.println("Region: AS923-3");
      break;
    case LORAMAC_REGION_AS923_4:
      Serial.println("Region: AS923-4");
      break;
  }
  Serial.println("=====================================");

  // Setup the EUIs and Keys
  if (doOTAA)
  {
    lmh_setDevEui(nodeDeviceEUI);
    lmh_setAppEui(nodeAppEUI);
    lmh_setAppKey(nodeAppKey);
  }
  else
  {
    lmh_setNwkSKey(nodeNwsKey);
    lmh_setAppSKey(nodeAppsKey);
    lmh_setDevAddr(nodeDevAddr);
  }

  // Initialize LoRaWan
  uint32_t err_code = lmh_init(&g_lora_callbacks, g_lora_param_init, doOTAA, g_CurrentClass, g_CurrentRegion);
  if (err_code != 0)
  {
    //Serial.printf("lmh_init failed - %d\n", err_code);
    return;
  }

  // Start Join procedure
  lmh_join();

  set_spreading_factor(LORAWAN_DATERATE);
}

void loop()
{
  // Every LORAWAN_APP_INTERVAL milliseconds send_now will be set
  // true by the application timer and collects and sends the data
  if (send_now)
  {
    Serial.println("Sending frame now...");
    send_now = false;
    set_spreading_factor(LORAWAN_DATERATE);
    send_lora_frame();
  }
}


/**@brief LoRa function for handling HasJoined event.
*/
void lorawan_has_joined_handler(void)
{
  if (doOTAA == true)
  {
    Serial.println("OTAA Mode, Network Joined!");
  }
  else
  {
    Serial.println("ABP Mode");
  }

  lmh_error_status ret = lmh_class_request(g_CurrentClass);
  if (ret == LMH_SUCCESS)
  {
    // delay(100);
    // Start the application timer. Time has to be in microseconds
    appTimer.attach(tx_lora_periodic_handler, (std::chrono::microseconds)(LORAWAN_APP_INTERVAL * 1000));
  }
}
/**@brief LoRa function for handling OTAA join failed
*/
static void lorawan_join_failed_handler(void)
{
  Serial.println("OTAA join failed!");
  Serial.println("Check your EUI's and Keys's!");
  Serial.println("Check if a Gateway is in range!");
}
/**@brief Function for handling LoRaWan received data from Gateway

   @param[in] app_data  Pointer to rx data
*/
void lorawan_rx_handler(lmh_app_data_t *app_data)
{
  //Serial.printf("LoRa Packet received on port %d, size:%d, rssi:%d, snr:%d, data:%s\n",
                //app_data->port, app_data->buffsize, app_data->rssi, app_data->snr, app_data->buffer);
}

void lorawan_confirm_class_handler(DeviceClass_t Class)
{
  //Serial.printf("switch to class %c done\n", "ABC"[Class]);
  // Informs the server that switch has occurred ASAP
  m_lora_app_data.buffsize = 0;
  m_lora_app_data.port = gAppPort;
  lmh_send(&m_lora_app_data, g_CurrentConfirm);
}

void lorawan_unconf_finished(void)
{
  Serial.println("TX finished");
}

void lorawan_conf_finished(bool result)
{
  //Serial.printf("Confirmed TX %s\n", result ? "success" : "failed");
}

void send_lora_frame(void)
{
  if (lmh_join_status_get() != LMH_SET)
  {
    //Not joined, try again later
    return;
  }

  uint32_t i = 0;
  memset(m_lora_app_data.buffer, 0, LORAWAN_APP_DATA_BUFF_SIZE);
  m_lora_app_data.port = gAppPort;
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'e';
  m_lora_app_data.buffer[i++] = 'l';
  m_lora_app_data.buffer[i++] = 'l';
  m_lora_app_data.buffer[i++] = 'o';
  m_lora_app_data.buffer[i++] = '!';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = '!'; //10
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = '!'; //20
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = '!'; //30
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = '!'; //40
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = '!'; //50
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = '!'; //60
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = '!'; //70
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = '!'; //80
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = '!'; //90
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = '!'; //100
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = '!'; //110
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = '!'; //120
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = '!'; //130
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = '!'; //140
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = '!'; //150
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = '!'; //160
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = '!'; //170
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = '!'; //180
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = '!'; //190
  m_lora_app_data.buffer[i++] = 'H';
  m_lora_app_data.buffer[i++] = 'H';

  m_lora_app_data.buffsize = i;

  lmh_error_status error = lmh_send(&m_lora_app_data, g_CurrentConfirm);
  if (error == LMH_SUCCESS)
  {
    count++;
    //Serial.printf("lmh_send ok count %d\n", count);
  }
  else
  {
    count_fail++;
    //Serial.printf("lmh_send fail count %d\n", count_fail);
  }
}

/**@brief Function for handling user timerout event.
*/
void tx_lora_periodic_handler(void)
{
  appTimer.attach(tx_lora_periodic_handler, (std::chrono::microseconds)(LORAWAN_APP_INTERVAL * 1000));
  // This is a timer interrupt, do not do lengthy things here. Signal the loop() instead
  send_now = true;
}