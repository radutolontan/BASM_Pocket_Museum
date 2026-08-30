// CONFIGURATION PARAMETERS FOR THE POWER SYSTEM

// [BMS] ChargeController and Feedback
#define VBAT_VTHRESHOLD             3.4f// If the system is predicted to reach VBAT_VTHRESHOLD within VBAT_TIME_TO_VTHRESHOLD_MIN, LOW_POWER will be indicated
#define VBAT_TIME_TO_VTHRESHOLD_MIN 15.0f
#define VBAT_SETTLING_PERIOD_SEC    90.0f// To avoid LiPo transients, within VBAT_SETTLING_PERIOD_SEC from transitioning to BATT_ONLY, no LOW_BATT predictions are made
#define VBAT_CHECK_INTERVAL_SEC     10.0f// [s] - sample Battery voltage every X seconds
#define VBAT_HISTORY_LEN            60   // number of samples in VBat History
#define VBAT_AVG_SAMPLES            4    // Number of averaged ADC readings per sample
#define VBAT_MIN_VOLTAGE            2.85f// Acceptable physical range for battery voltage (Volts) - MAX
#define VBAT_MAX_VOLTAGE            4.30f// Acceptable physical range for battery voltage (Volts) - MIN

// ===================== BMSTask ==================== //
#define BMS_LDO_ATTACH_CMD_PIN      1
#define BMS_ONOFF_PUSHBUTTON_PIN    4
#define BMS_TIMER_STARTUP           2200 // [ms] - HOLD ON/OFF pressed before latching ON
#define BMS_TIMER_SHUTDOWN          2200 // [ms] - HOLD ON/OFF pressed before latching OFF


// ================ ChargeController ================ //
#define BMS_CHG_FDBCK_PIN           6
#define BMS_POWOK_FDBCK_PIN         7

// ================== SoCController ================= //
#define BMS_VBAT_VOLT_PIN           0
#define VBAT_DIVIDER_RTOP           178e3f
#define VBAT_DIVIDER_RBOTTOM        61.9e3f
#define ADC_CORRECTION_SLOPE        0.9796f // ACTUAL_VOLTAGE = ADC_CORRECTION_SLOPE * ADC_MEASURED_VOLTAGE + ADC_CORRECTION_OFFSET
#define ADC_CORRECTION_OFFSET       0.0548f // ACTUAL_VOLTAGE = ADC_CORRECTION_SLOPE * ADC_MEASURED_VOLTAGE + ADC_CORRECTION_OFFSET
#define EMA_ALPHA                   0.004 // Computed for a time_constant of approx. 5 sec [α = 1 - exp(-dt / τ)]
#define VBAT_MIN_VOLTAGE            2.85f // Acceptable physical range for battery voltage (Volts) - MAX
#define VBAT_MAX_VOLTAGE            4.30f // Acceptable physical range for battery voltage (Volts) - MIN
