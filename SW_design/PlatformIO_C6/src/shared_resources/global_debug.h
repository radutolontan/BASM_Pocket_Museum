// Comment/uncomment to enable/disable debug prints
// SHOW DEBUG MESSAGES WITH SENSOR VALUES
#define DEBUG_SENSOR     1
// SHOW DEBUG MESSAGES W. TASK RATES
#define DEBUG_TASK_RATES 1


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