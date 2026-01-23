// Comment/uncomment to enable/disable debug prints
// SHOW DEBUG MESSAGES WITH SENSOR VALUES
#define DEBUG_SENSOR     0
// SHOW DEBUG MESSAGES W. TASK RATES
#define DEBUG_TASK_RATES 0
// SHOW DEBUG MESSAGES FOR NETWORK TASK
#define DEBUG_NETWORK    0
// SHOW NETWORK PACKET CONTENTS (very verbose)
#define DEBUG_NETWORK_PACKETS 0


#if DEBUG_SENSOR
  #define SENSOR_PRINT(x)      Serial.print(x)
  #define SENSOR_PRINTLN(x)    Serial.println(x)
#else
  #define SENSOR_PRINT(x)
  #define SENSOR_PRINTLN(x)
#endif

#if DEBUG_TASK_RATES
  #define RATES_PRINT(x)      Serial.print(x)
  #define RATES_PRINTLN(x)    Serial.println(x)
  #define RATES_PRINTF(...)   Serial.printf(__VA_ARGS__)
#else
  #define RATES_PRINT(x)
  #define RATES_PRINTLN(x)
  #define RATES_PRINTF(...)
#endif

#if DEBUG_NETWORK
  #define NETWORK_PRINT(x)      Serial.print(x)
  #define NETWORK_PRINTLN(x)    Serial.println(x)
  #define NETWORK_PRINTF(...)   Serial.printf(__VA_ARGS__)
#else
  #define NETWORK_PRINT(x)
  #define NETWORK_PRINTLN(x)
  #define NETWORK_PRINTF(...)
#endif