/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Smart Solar Tracker - FINAL ERROR-FREE VERSION
  * @board          : NUCLEO-F401RE
  * @date           : 2025-12-30
  * @version        : 5.6 - ALL ERRORS FIXED
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include "ssd1306.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* =================== DEMO MODE CONFIGURATION =================== */
#define DEMO_MODE 0
#define DEMO_DELAY_MS 3000
#define CALIBRATION_MODE 0      // Set to 1 to test servo movement

/* =================== SMOOTH MOVEMENT CONFIGURATION =================== */
#define SMOOTH_STEP_SIZE 2.0f
#define SMOOTH_STEP_DELAY 20
#define MIN_MOVE_THRESHOLD 1.0f

/* =================== CALIBRATION SMOOTH SETTINGS =================== */
#define CALIBRATION_STEP_SIZE 0.5f      // Smooth within 10°
#define CALIBRATION_STEP_DELAY 40       // Fast enough within step
#define CALIBRATION_MIN_THRESHOLD 0.3f  // Precision threshold

/* =================== NIGHT MODE CONFIGURATION =================== */
#define SUNRISE_HOUR 6
#define SUNSET_HOUR 18
#define SUNRISE_AZIMUTH_POS 30.0f
#define SUNRISE_ELEVATION_POS 10.0f

/* =================== Servo position mappings =================== */
#define AZIMUTH_MIN_ANGLE 0.0f
#define AZIMUTH_MAX_ANGLE 180.0f
#define ELEVATION_MIN_ANGLE 0.0f
#define ELEVATION_MAX_ANGLE 90.0f

/* =================== Handles =================== */
I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
ADC_HandleTypeDef hadc1;
float current_zero_offset = 0.0f;

/* =================== ADC Configuration =================== */
#define VOLTAGE_CHANNEL ADC_CHANNEL_4
#define CURRENT_CHANNEL ADC_CHANNEL_5

/* =================== ADC Constants =================== */
#define VREF 3.3f
#define ADC_RESOLUTION 4095.0f
#define VOLTAGE_DIVIDER_RATIO 5.0f
#define NUM_VOLTAGE_SAMPLES 100

/* =================== Current Sensor Constants =================== */
#define CURRENT_SENSITIVITY 0.185f
#define MAX_CURRENT 5.0f
#define CURRENT_ZERO_VOLTAGE 2.5f
#define NUM_CURRENT_SAMPLES 50

/* =================== Servo PWM Constants =================== */
#define SERVO_MIN_PULSE  500
#define SERVO_MID_PULSE  1500
#define SERVO_MAX_PULSE  2500

/* =================== Solar Position Constants =================== */
#define PI 3.141592653589793
#define RAD (PI / 180.0)
#define DEG (180.0 / PI)
#define DEFAULT_TIMEZONE 5.5f

/* =================== Structures =================== */
typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} DateTime;

typedef struct {
    float azimuth;
    float elevation;
    float zenith;
} SolarPosition;

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t date;
    uint8_t month;
    uint8_t year;
} RTC_Time;

/* =================== Function Prototypes =================== */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
void Error_Handler(void);
void Servo_SetAngle(uint8_t servo_num, float angle);
void Servo_SmoothMove(uint8_t servo_num, float target_angle, float current_angle);
void calculateSolarPosition(DateTime *dt, float lat, float lon, float timezone, SolarPosition *pos);

/* =================== DS3231 RTC Driver =================== */
#define DS3231_ADDRESS (0x68 << 1)

static uint8_t bcd_to_dec(uint8_t b) {
    return ((b >> 4) * 10) + (b & 0x0F);
}

HAL_StatusTypeDef DS3231_GetTime(RTC_Time *t) {
    uint8_t buf[7];
    if (HAL_I2C_Mem_Read(&hi2c1, DS3231_ADDRESS, 0x00, 1, buf, 7, HAL_MAX_DELAY) != HAL_OK)
        return HAL_ERROR;

    t->seconds = bcd_to_dec(buf[0] & 0x7F);
    t->minutes = bcd_to_dec(buf[1] & 0x7F);
    t->hours   = bcd_to_dec(buf[2] & 0x3F);
    t->day     = bcd_to_dec(buf[3] & 0x07);
    t->date    = bcd_to_dec(buf[4] & 0x3F);
    t->month   = bcd_to_dec(buf[5] & 0x1F);
    t->year    = bcd_to_dec(buf[6]);

    return HAL_OK;
}

HAL_StatusTypeDef DS3231_SetTime(RTC_Time *t) {
    uint8_t buf[7];

    buf[0] = ((t->seconds / 10) << 4) | (t->seconds % 10);
    buf[1] = ((t->minutes / 10) << 4) | (t->minutes % 10);
    buf[2] = ((t->hours   / 10) << 4) | (t->hours   % 10);
    buf[3] = ((t->day     / 10) << 4) | (t->day     % 10);
    buf[4] = ((t->date    / 10) << 4) | (t->date    % 10);
    buf[5] = ((t->month   / 10) << 4) | (t->month   % 10);
    buf[6] = ((t->year    / 10) << 4) | (t->year    % 10);

    buf[0] &= ~0x80;

    return HAL_I2C_Mem_Write(&hi2c1, DS3231_ADDRESS, 0x00, 1, buf, 7, HAL_MAX_DELAY);
}

static void ParseCompileTime(RTC_Time *rtc_time) {
    char compile_date[12] = __DATE__;
    char compile_time[9] = __TIME__;

    static const char* months[] = {"Jan","Feb","Mar","Apr","May","Jun",
                                   "Jul","Aug","Sep","Oct","Nov","Dec"};
    for (int i = 1; i <= 12; i++) {
        if (strncmp(compile_date, months[i-1], 3) == 0) {
            rtc_time->month = i;
            break;
        }
    }

    rtc_time->date = (compile_date[4]-'0')*10 + (compile_date[5]-'0');
    rtc_time->year = (compile_date[9]-'0')*10 + (compile_date[10]-'0');
    rtc_time->hours   = (compile_time[0]-'0')*10 + (compile_time[1]-'0');
    rtc_time->minutes = (compile_time[3]-'0')*10 + (compile_time[4]-'0');
    rtc_time->seconds = (compile_time[6]-'0')*10 + (compile_time[7]-'0');
    rtc_time->day = ((rtc_time->date + rtc_time->month + rtc_time->year) % 7) + 1;
}

/* =================== UART Debug Helper =================== */
static void DBG(const char* s) {
    if (s == NULL) return;
    HAL_UART_Transmit(&huart2, (uint8_t*)s, strlen(s), 100);
}

/* =================== OLED Display Helper =================== */
static void OLED_Update(const char* l0, const char* l1, const char* l2, const char* l3) {
    SSD1306_Clear();
    if (l0) SSD1306_DrawString(0, 0,  (char*)l0);
    if (l1) SSD1306_DrawString(0, 16, (char*)l1);
    if (l2) SSD1306_DrawString(0, 32, (char*)l2);
    if (l3) SSD1306_DrawString(0, 48, (char*)l3);
    SSD1306_UpdateScreen();
}

/* =================== Servo Control =================== */
void Servo_Init(void) {
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 16 - 1;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 20000 - 1;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) {
        Error_Handler();
    }

    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = SERVO_MID_PULSE;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK) {
        Error_Handler();
    }

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);

    HAL_Delay(100);
    Servo_SetAngle(1, SUNRISE_AZIMUTH_POS);
    Servo_SetAngle(2, SUNRISE_ELEVATION_POS);
    HAL_Delay(500);
}

void Servo_SetAngle(uint8_t servo_num, float angle) {
    if (angle < 0.0f) angle = 0.0f;
    if (angle > 180.0f) angle = 180.0f;

    uint32_t pulse = (uint32_t)(SERVO_MIN_PULSE +
                                (angle / 180.0f) * (SERVO_MAX_PULSE - SERVO_MIN_PULSE));

    if (servo_num == 1) {
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pulse);
    } else if (servo_num == 2) {
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, pulse);
    }
}

void Servo_SmoothMove(uint8_t servo_num, float target_angle, float current_angle) {
    if (target_angle < 0.0f) target_angle = 0.0f;
    if (target_angle > 180.0f) target_angle = 180.0f;
    if (current_angle < 0.0f) current_angle = 0.0f;
    if (current_angle > 180.0f) current_angle = 180.0f;

    float diff = target_angle - current_angle;

    // Use calibration threshold in calibration mode
    float threshold = CALIBRATION_MODE ? CALIBRATION_MIN_THRESHOLD : MIN_MOVE_THRESHOLD;
    if (fabsf(diff) < threshold) {
        Servo_SetAngle(servo_num, target_angle);
        return;
    }

    // Calibration mode: Extra slow and smooth
    float step_size = CALIBRATION_MODE ? CALIBRATION_STEP_SIZE : SMOOTH_STEP_SIZE;
    uint32_t step_delay = CALIBRATION_MODE ? CALIBRATION_STEP_DELAY : SMOOTH_STEP_DELAY;

    float step = (diff > 0) ? step_size : -step_size;
    float angle = current_angle;

    while (fabsf(target_angle - angle) > step_size) {
        angle += step;
        if (angle < 0.0f) angle = 0.0f;
        if (angle > 180.0f) angle = 180.0f;
        Servo_SetAngle(servo_num, angle);
        HAL_Delay(step_delay);
    }

    Servo_SetAngle(servo_num, target_angle);
}

/* =================== ADC Functions =================== */
void ADC_Init(void) {
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        Error_Handler();
    }
}

float ADC_ReadVoltage(void) {
    uint32_t adc_sum = 0;

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = VOLTAGE_CHANNEL;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;

    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        return 0.0f;
    }

    for (int i = 0; i < NUM_VOLTAGE_SAMPLES; i++) {
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
            adc_sum += HAL_ADC_GetValue(&hadc1);
        }
        HAL_ADC_Stop(&hadc1);
    }

    float adc_avg = (float)adc_sum / (float)NUM_VOLTAGE_SAMPLES;
    float adc_voltage = (adc_avg * VREF) / ADC_RESOLUTION;
    float measured_voltage = adc_voltage * VOLTAGE_DIVIDER_RATIO;

    if (measured_voltage < 0.0f) measured_voltage = 0.0f;

    return measured_voltage;
}

float ADC_ReadCurrent(void) {
    uint32_t adc_sum = 0;

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = CURRENT_CHANNEL;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;

    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        return 0.0f;
    }

    for (int i = 0; i < NUM_CURRENT_SAMPLES; i++) {
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
            adc_sum += HAL_ADC_GetValue(&hadc1);
        }
        HAL_ADC_Stop(&hadc1);
    }

    float adc_avg = (float)adc_sum / (float)NUM_CURRENT_SAMPLES;
    float adc_voltage = (adc_avg * VREF) / ADC_RESOLUTION;
    float current = (adc_voltage - CURRENT_ZERO_VOLTAGE) / CURRENT_SENSITIVITY;

    current = current - current_zero_offset;

    if (current > MAX_CURRENT) current = MAX_CURRENT;
    if (current < -MAX_CURRENT) current = -MAX_CURRENT;
    if (current < 0.0f) current = 0.0f;

    return current;
}

/* =================== GPS NMEA Parser =================== */
#define NMEA_MAX 128
static volatile uint8_t rx_ch;
static char nmea_line[NMEA_MAX];
static volatile uint8_t nmea_ready = 0;
static volatile int nidx = 0;

static int split_csv(char* s, char* f[], int maxf) {
    int n = 0;
    f[n++] = s;
    for (char* p = s; *p && n < maxf; p++) {
        if (*p == ',') { *p = 0; if (n < maxf) f[n++] = p + 1; }
        if (*p == '*') { *p = 0; break; }
    }
    return n;
}

static double nmea_to_deg(const char* ddmm, char hemi) {
    if (ddmm == NULL || *ddmm == 0 || strlen(ddmm) < 4) return 0.0;
    double v = atof(ddmm);
    double deg = (int)(v / 100.0);
    double min = v - (deg * 100.0);
    double decimal = deg + (min / 60.0);
    if (hemi == 'S' || hemi == 'W') decimal = -decimal;
    return decimal;
}

static int parse_rmc(char* s, double* plat, double* plon, char* pNs, char* pEw) {
    char* f[16] = {0};
    int n = split_csv(s, f, 16);
    if (n < 7) return 0;
    if (f[2][0] != 'A') return 0;
    if (strlen(f[3]) < 4 || strlen(f[5]) < 5) return 0;
    char ns = f[4][0], ew = f[6][0];
    if (ns != 'N' && ns != 'S') return 0;
    if (ew != 'E' && ew != 'W') return 0;
    *plat = nmea_to_deg(f[3], ns);
    *plon = nmea_to_deg(f[5], ew);
    *pNs = ns; *pEw = ew;
    return 1;
}

static int parse_gga(char* s, double* plat, double* plon, char* pNs, char* pEw, int* fix) {
    char* f[16] = {0};
    int n = split_csv(s, f, 16);
    if (n < 7) return 0;
    int q = atoi(f[6]);
    if (q == 0) return 0;
    if (strlen(f[2]) < 4 || strlen(f[4]) < 5) return 0;
    char ns = f[3][0], ew = f[5][0];
    if (ns != 'N' && ns != 'S') return 0;
    if (ew != 'E' && ew != 'W') return 0;
    *plat = nmea_to_deg(f[2], ns);
    *plon = nmea_to_deg(f[4], ew);
    *pNs = ns; *pEw = ew; *fix = q;
    return 1;
}

/* =================== Solar Position Algorithm =================== */
double getJulianDay(DateTime *dt, float timezone) {
    int Y = dt->year;
    int M = dt->month;
    double D = dt->day + (dt->hour + dt->minute/60.0 + dt->second/3600.0 - timezone) / 24.0;

    if (M <= 2) {
        Y = Y - 1;
        M = M + 12;
    }

    int A = Y / 100;
    int B = 2 - A + A / 4;
    double JD = floor(365.25 * (Y + 4716)) + floor(30.6001 * (M + 1)) + D + B - 1524.5;

    return JD;
}

void calculateSolarPosition(DateTime *dt, float lat, float lon, float timezone, SolarPosition *pos) {
    double JD = getJulianDay(dt, timezone);
    double JC = (JD - 2451545.0) / 36525.0;

    double L = fmod(280.46646 + 36000.76983 * JC + 0.0003032 * JC * JC, 360.0);
    double M = fmod(357.52911 + 35999.05029 * JC - 0.0001537 * JC * JC, 360.0);

    double C = (1.914602 - 0.004817 * JC - 0.000014 * JC * JC) * sin(M * RAD)
             + (0.019993 - 0.000101 * JC) * sin(2 * M * RAD)
             + 0.000289 * sin(3 * M * RAD);

    double theta = L + C;
    double eps = 23.439291 - 0.0130042 * JC - 0.00000016 * JC * JC + 0.000000504 * JC * JC * JC;

    double lambda = theta - 0.00569 - 0.00478 * sin((125.04 - 1934.136 * JC) * RAD);
    double tanRA = cos(eps * RAD) * sin(lambda * RAD) / cos(lambda * RAD);
    double RA = atan(tanRA) * DEG;

    double Lquadrant = floor(lambda / 90.0) * 90.0;
    double RAquadrant = floor(RA / 90.0) * 90.0;
    RA = RA + (Lquadrant - RAquadrant);

    double delta = asin(sin(eps * RAD) * sin(lambda * RAD)) * DEG;

    double GMST = 280.46061837 + 360.98564736629 * (JD - 2451545.0) + 0.000387933 * JC * JC - JC * JC * JC / 38710000.0;
    GMST = fmod(GMST, 360.0);
    if (GMST < 0) GMST += 360.0;

    double LST = GMST + lon;
    double HA = LST - RA;
    if (HA < 0) HA += 360.0;
    if (HA > 360.0) HA -= 360.0;

    double HA_rad = HA * RAD;
    double lat_rad = lat * RAD;
    double delta_rad = delta * RAD;

    double sinAlt = sin(lat_rad) * sin(delta_rad) + cos(lat_rad) * cos(delta_rad) * cos(HA_rad);
    double alt = asin(sinAlt) * DEG;

    if (alt > -0.83) {
        double correction = 0.0;
        if (alt > 85.0) {
            correction = 0.0;
        } else if (alt > 5.0) {
            correction = 58.1 / tan(alt * RAD) - 0.07 / pow(tan(alt * RAD), 3) + 0.000086 / pow(tan(alt * RAD), 5);
            correction = correction / 3600.0;
        } else if (alt > -0.575) {
            correction = 1735.0 + alt * (-518.2 + alt * (103.4 + alt * (-12.79 + alt * 0.711)));
            correction = correction / 3600.0;
        }
        alt += correction;
    }

    pos->elevation = alt;
    pos->zenith = 90.0 - alt;

    double cosAz = (sin(delta_rad) - sin(alt * RAD) * sin(lat_rad)) / (cos(alt * RAD) * cos(lat_rad));
    if (cosAz > 1.0) cosAz = 1.0;
    if (cosAz < -1.0) cosAz = -1.0;

    double Az = acos(cosAz) * DEG;

    if (HA > 180.0) {
        pos->azimuth = Az;
    } else {
        pos->azimuth = 360.0 - Az;
    }
}

/* =================== UART ISR =================== */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        char c = (char)rx_ch;
        if (c == '\r') { /* ignore */ }
        else if (c == '\n') {
            if (nidx > 0) { nmea_line[nidx] = 0; nmea_ready = 1; }
            nidx = 0;
        } else {
            if (nidx < (NMEA_MAX - 1)) nmea_line[nidx++] = c;
        }
        HAL_UART_Receive_IT(&huart1, (uint8_t*)&rx_ch, 1);
    }
}

/* =================== MAIN PROGRAM =================== */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();

    DBG("\r\n\r\nSOLAR TRACKER v5.6 - ERROR FREE\r\n\r\n");

    DBG("Initializing Servos...\r\n");
    Servo_Init();
    DBG("Servos ready\r\n");

    DBG("Initializing ADC...\r\n");
    ADC_Init();
    DBG("ADC ready\r\n");

    DBG("Calibrating current sensor...\r\n");
    HAL_Delay(3000);
    float sum = 0;
    for (int i = 0; i < 50; i++) {
        uint32_t adc_sum = 0;
        ADC_ChannelConfTypeDef sConfig = {0};
        sConfig.Channel = CURRENT_CHANNEL;
        sConfig.Rank = 1;
        sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
        HAL_ADC_ConfigChannel(&hadc1, &sConfig);

        for (int j = 0; j < NUM_CURRENT_SAMPLES; j++) {
            HAL_ADC_Start(&hadc1);
            HAL_ADC_PollForConversion(&hadc1, 100);
            adc_sum += HAL_ADC_GetValue(&hadc1);
            HAL_ADC_Stop(&hadc1);
        }

        float adc_avg = (float)adc_sum / (float)NUM_CURRENT_SAMPLES;
        float v = (adc_avg * VREF) / ADC_RESOLUTION;
        float temp_current = (v - CURRENT_ZERO_VOLTAGE) / CURRENT_SENSITIVITY;
        sum += temp_current;
        HAL_Delay(20);
    }
    current_zero_offset = sum / 50.0f;
    DBG("Current calibration complete\r\n");

    /* OLED Initialization - Simple approach like working code */
    DBG("Initializing OLED...\r\n");
    SSD1306_Init(&hi2c1);
    HAL_Delay(100);

    SSD1306_Clear();
    SSD1306_DrawString(0, 0, "Solar Tracker");
    SSD1306_DrawString(0, 16, "v5.6 Ready");
    SSD1306_UpdateScreen();

    DBG("OLED ready\r\n");

    /* =================== CALIBRATION MODE =================== */
    /* =================== CALIBRATION MODE - 10° STEPS + 2s PAUSE =================== */
    if (CALIBRATION_MODE) {
        DBG("\r\n");
        DBG("╔════════════════════════════════════════════════╗\r\n");
        DBG("║   10° STEPS + 2s PAUSE - SMOOTH MOVEMENT       ║\r\n");
        DBG("║  Perfect for mechanical testing!              ║\r\n");
        DBG("╚════════════════════════════════════════════════╝\r\n\r\n");

        SSD1306_Clear();
        SSD1306_DrawString(0, 0, "10° STEP CAL");
        SSD1306_DrawString(0, 16, "2s PAUSE");
        SSD1306_UpdateScreen();

        float current_az = SUNRISE_AZIMUTH_POS;
        float current_el = SUNRISE_ELEVATION_POS;

        /* Test Sequence 1: Azimuth Servo (10° steps 0→180°) */
        DBG("TEST 1: AZIMUTH 0°→180° (10° steps)\r\n");
        SSD1306_Clear();
        SSD1306_DrawString(0, 0, "AZIMUTH 10° STEPS");
        SSD1306_DrawString(0, 16, "0->180");
        SSD1306_UpdateScreen();

        for (int angle = 0; angle <= 180; angle += 10) {
            DBG("   AZ -> ");
            char buf[32];
            snprintf(buf, sizeof(buf), "%d°\r\n", angle);
            DBG(buf);

            Servo_SmoothMove(1, (float)angle, current_az);
            current_az = (float)angle;

            // Update display
            SSD1306_Clear();
            SSD1306_DrawString(0, 0, "AZIMUTH STEP");
            snprintf(buf, sizeof(buf), "AZ:%d°", angle);
            SSD1306_DrawString(0, 16, buf);
            SSD1306_DrawString(0, 32, "PAUSE 2s");
            SSD1306_UpdateScreen();

            HAL_Delay(2000);  // 2 seconds pause
        }

        /* Test Sequence 2: Elevation Servo (10° steps 0→90°) */
        DBG("\r\nTEST 2: ELEVATION 0°→90° (10° steps)\r\n");
        SSD1306_Clear();
        SSD1306_DrawString(0, 0, "ELEVATION 10°");
        SSD1306_DrawString(0, 16, "0->90");
        SSD1306_UpdateScreen();

        for (int angle = 0; angle <= 90; angle += 10) {
            DBG("   EL -> ");
            char buf[32];
            snprintf(buf, sizeof(buf), "%d°\r\n", angle);
            DBG(buf);

            Servo_SmoothMove(2, (float)angle, current_el);
            current_el = (float)angle;

            // Update display
            SSD1306_Clear();
            SSD1306_DrawString(0, 0, "ELEVATION STEP");
            snprintf(buf, sizeof(buf), "EL:%d°", angle);
            SSD1306_DrawString(0, 16, buf);
            SSD1306_DrawString(0, 32, "PAUSE 2s");
            SSD1306_UpdateScreen();

            HAL_Delay(2000);  // 2 seconds pause
        }

        /* Test Sequence 3: Sun Positions (10° smooth transitions) */
        DBG("\r\nTEST 3: SUN TRACKING POSITIONS\r\n");
        struct {
            float azimuth;
            float elevation;
            const char* name;
        } sun_pos[] = {
            {30.0f, 10.0f, "SUNRISE"},
            {90.0f, 45.0f, "MORNING"},
            {90.0f, 70.0f, "NOON"},
            {120.0f, 45.0f, "AFTERNOON"},
            {150.0f, 10.0f, "SUNSET"}
        };

        for (int i = 0; i < 5; i++) {
            char buf[64];
            snprintf(buf, sizeof(buf), "   %s: AZ%.0f EL%.0f\r\n",
                     sun_pos[i].name, sun_pos[i].azimuth, sun_pos[i].elevation);
            DBG(buf);

            SSD1306_Clear();
            SSD1306_DrawString(0, 0, "SUN POSITION");
            SSD1306_DrawString(0, 16, (char*)sun_pos[i].name);
            snprintf(buf, sizeof(buf), "AZ%.0f EL%.0f",
                    sun_pos[i].azimuth, sun_pos[i].elevation);
            SSD1306_DrawString(0, 32, buf);
            SSD1306_UpdateScreen();

            Servo_SmoothMove(1, sun_pos[i].azimuth, current_az);
            Servo_SmoothMove(2, sun_pos[i].elevation, current_el);
            current_az = sun_pos[i].azimuth;
            current_el = sun_pos[i].elevation;

            HAL_Delay(3000);  // 3 seconds for sun positions
        }

        /* Return to Home */
        DBG("\r\nRETURNING TO SUNRISE...\r\n");
        SSD1306_Clear();
        SSD1306_DrawString(0, 0, "GOING HOME");
        SSD1306_DrawString(0, 16, "Sunrise Pos");
        SSD1306_UpdateScreen();

        Servo_SmoothMove(1, SUNRISE_AZIMUTH_POS, current_az);
        Servo_SmoothMove(2, SUNRISE_ELEVATION_POS, current_el);

        DBG("\r\n");
        DBG("╔════════════════════════════════════════════════╗\r\n");
        DBG("║      10° STEP CALIBRATION COMPLETE!            ║\r\n");
        DBG("║                                                ║\r\n");
        DBG("║  Checklist:                                    ║\r\n");
        DBG("║  ✓ 10° smooth steps OK?                       ║\r\n");
        DBG("║  ✓ 2s pauses sufficient?                      ║\r\n");
        DBG("║  ✓ No mechanical binding?                     ║\r\n");
        DBG("║  ✓ Wires safe?                                ║\r\n");
        DBG("║                                                ║\r\n");
        DBG("║  Set CALIBRATION_MODE=0 for tracking          ║\r\n");
        DBG("╚════════════════════════════════════════════════╝\r\n\r\n");

        SSD1306_Clear();
        SSD1306_DrawString(0, 0, "CALIBRATION OK");
        SSD1306_DrawString(0, 16, "10° Steps Done");
        SSD1306_DrawString(0, 32, "Set CAL=0");
        SSD1306_UpdateScreen();

        while(1) {
            HAL_Delay(1000);
        }
    }


    DBG("RTC set\r\n");

    /* =================== RTC Setup =================== */
    if (!CALIBRATION_MODE) {
        DBG("Setting RTC...\r\n");
        RTC_Time compile_time;
        ParseCompileTime(&compile_time);
        if (DS3231_SetTime(&compile_time) == HAL_OK) {
            HAL_Delay(50);
            RTC_Time verify_time;
            DS3231_GetTime(&verify_time);
            char rtc_buf[100];
            snprintf(rtc_buf, sizeof(rtc_buf), "RTC: %02d/%02d/20%02d %02d:%02d:%02d\r\n",
                     verify_time.date, verify_time.month, verify_time.year,
                     verify_time.hours, verify_time.minutes, verify_time.seconds);
            DBG(rtc_buf);
        }
    }

    if (!DEMO_MODE) {
        HAL_UART_Receive_IT(&huart1, (uint8_t*)&rx_ch, 1);
    }

    if (!CALIBRATION_MODE) {
        DBG("═══════════════════════════════════════════════\r\n");
        if (DEMO_MODE) {
            DBG("     DEMO MODE: 24-HOUR CYCLE (6 AM START)    \r\n");
        } else {
            DBG("     REAL MODE: GPS + RTC TRACKING ACTIVE     \r\n");
        }
        DBG("═══════════════════════════════════════════════\r\n\r\n");
    }

    HAL_Delay(2000);

    double lat = 11.0, lon = 77.0;
    char ns = 'N', ew = 'E';
    uint8_t gps_valid = DEMO_MODE ? 1 : 0;
    SolarPosition sun_pos;
    float servo1_angle = SUNRISE_AZIMUTH_POS;
    float servo2_angle = SUNRISE_ELEVATION_POS;
    float servo1_prev = SUNRISE_AZIMUTH_POS;
    float servo2_prev = SUNRISE_ELEVATION_POS;
    uint8_t demo_hour = SUNRISE_HOUR;
    uint8_t night_mode_active = 0;
    uint8_t reset_done = 0;
    static uint32_t wait_counter = 0;

    while (1) {
        if (DEMO_MODE) {
            DateTime dt = {2025, 10, 13, demo_hour, 0, 0};
            calculateSolarPosition(&dt, lat, lon, DEFAULT_TIMEZONE, &sun_pos);

            if (demo_hour < SUNRISE_HOUR || demo_hour >= SUNSET_HOUR || sun_pos.elevation < 0) {
                if (!night_mode_active) {
                    night_mode_active = 1;
                    DBG("NIGHT MODE\r\n");
                }
                Servo_SmoothMove(1, SUNRISE_AZIMUTH_POS, servo1_prev);
                Servo_SmoothMove(2, SUNRISE_ELEVATION_POS, servo2_prev);
                servo1_prev = SUNRISE_AZIMUTH_POS;
                servo2_prev = SUNRISE_ELEVATION_POS;

                char buf_l0[21], buf_l1[21];
                snprintf(buf_l0, sizeof(buf_l0), "NIGHT %02d:00", demo_hour);
                snprintf(buf_l1, sizeof(buf_l1), "Sunrise Ready");
                OLED_Update(buf_l0, buf_l1, NULL, NULL);
            } else {
                if (night_mode_active) {
                    night_mode_active = 0;
                    DBG("TRACKING\r\n");
                }

                if (sun_pos.azimuth >= 90.0f && sun_pos.azimuth <= 270.0f) {
                    servo1_angle = (sun_pos.azimuth - 90.0f);
                } else {
                    servo1_angle = SUNRISE_AZIMUTH_POS;
                }
                servo1_angle = fmaxf(AZIMUTH_MIN_ANGLE, fminf(AZIMUTH_MAX_ANGLE, servo1_angle));
                servo2_angle = fmaxf(ELEVATION_MIN_ANGLE, fminf(ELEVATION_MAX_ANGLE, sun_pos.elevation));

                Servo_SmoothMove(1, servo1_angle, servo1_prev);
                Servo_SmoothMove(2, servo2_angle, servo2_prev);
                servo1_prev = servo1_angle;
                servo2_prev = servo2_angle;

                float voltage = ADC_ReadVoltage();
                float current = ADC_ReadCurrent();

                char buf_l0[21], buf_l1[21], buf_l2[21], buf_l3[21];
                snprintf(buf_l0, sizeof(buf_l0), "V:%.1fV I:%.2fA", voltage, current);
                snprintf(buf_l1, sizeof(buf_l1), "P:%.2fW %02d:00", voltage * current, demo_hour);
                snprintf(buf_l2, sizeof(buf_l2), "AZ%02.0f EL%02.0f", servo1_angle, servo2_angle);
                snprintf(buf_l3, sizeof(buf_l3), "L%.1fN%.1fE", lat, lon);
                OLED_Update(buf_l0, buf_l1, buf_l2, buf_l3);
            }

            demo_hour++;
            if (demo_hour >= 24) demo_hour = SUNRISE_HOUR;
            HAL_Delay(DEMO_DELAY_MS);

        } else {
            if (nmea_ready) {
                nmea_ready = 0;
                int valid = 0;
                char tmp[NMEA_MAX];

                if (strncmp(nmea_line, "$GPRMC", 6) == 0 || strncmp(nmea_line, "$GNRMC", 6) == 0) {
                    strncpy(tmp, nmea_line, NMEA_MAX - 1);
                    tmp[NMEA_MAX - 1] = '\0';
                    if (parse_rmc(tmp, &lat, &lon, &ns, &ew)) valid = 1;
                }
                else if (strncmp(nmea_line, "$GPGGA", 6) == 0 || strncmp(nmea_line, "$GNGGA", 6) == 0) {
                    strncpy(tmp, nmea_line, NMEA_MAX - 1);
                    tmp[NMEA_MAX - 1] = '\0';
                    int fix = 0;
                    if (parse_gga(tmp, &lat, &lon, &ns, &ew, &fix) && fix > 0) valid = 1;
                }

                if (valid) {
                    gps_valid = 1;
                    DBG("GPS OK\r\n");
                }
            }

            RTC_Time rtc;
            if (DS3231_GetTime(&rtc) == HAL_OK && gps_valid) {
                DateTime dt;
                dt.year = 2000 + rtc.year;
                dt.month = rtc.month;
                dt.day = rtc.date;
                dt.hour = rtc.hours;
                dt.minute = rtc.minutes;
                dt.second = rtc.seconds;

                float latitude = (ns == 'N') ? lat : -lat;
                float longitude = (ew == 'E') ? lon : -lon;
                calculateSolarPosition(&dt, latitude, longitude, DEFAULT_TIMEZONE, &sun_pos);

                if (dt.hour < SUNRISE_HOUR || dt.hour >= SUNSET_HOUR || sun_pos.elevation < 0) {
                    if (!night_mode_active) {
                        night_mode_active = 1;
                        reset_done = 0;
                        DBG("NIGHT MODE\r\n");
                    }
                    if (!reset_done) {
                        Servo_SmoothMove(1, SUNRISE_AZIMUTH_POS, servo1_prev);
                        Servo_SmoothMove(2, SUNRISE_ELEVATION_POS, servo2_prev);
                        servo1_prev = SUNRISE_AZIMUTH_POS;
                        servo2_prev = SUNRISE_ELEVATION_POS;
                        reset_done = 1;
                    }

                    char buf_l0[21];
                    snprintf(buf_l0, sizeof(buf_l0), "NIGHT %02d:%02d", dt.hour, dt.minute);
                    OLED_Update(buf_l0, "Sunrise Ready", NULL, NULL);

                } else {
                    if (night_mode_active) {
                        night_mode_active = 0;
                        DBG("TRACKING\r\n");
                    }

                    if (sun_pos.azimuth >= 90.0f && sun_pos.azimuth <= 270.0f) {
                        servo1_angle = (sun_pos.azimuth - 90.0f);
                    } else {
                        servo1_angle = SUNRISE_AZIMUTH_POS;
                    }
                    servo1_angle = fmaxf(0.0f, fminf(180.0f, servo1_angle));
                    servo2_angle = fmaxf(0.0f, fminf(90.0f, sun_pos.elevation));

                    Servo_SmoothMove(1, servo1_angle, servo1_prev);
                    Servo_SmoothMove(2, servo2_angle, servo2_prev);
                    servo1_prev = servo1_angle;
                    servo2_prev = servo2_angle;

                    float voltage = ADC_ReadVoltage();
                    float current = ADC_ReadCurrent();

                    char buf_l0[21], buf_l1[21], buf_l2[21], buf_l3[21];
                    snprintf(buf_l0, sizeof(buf_l0), "V:%.1fV I:%.2fA", voltage, current);
                    snprintf(buf_l1, sizeof(buf_l1), "P:%.2fW %02d:%02d", voltage * current, dt.hour, dt.minute);
                    snprintf(buf_l2, sizeof(buf_l2), "AZ%02.0f EL%02.0f", servo1_angle, servo2_angle);
                    snprintf(buf_l3, sizeof(buf_l3), "L%.1fN%.1fE", fabsf(latitude), fabsf(longitude));
                    OLED_Update(buf_l0, buf_l1, buf_l2, buf_l3);
                }
            } else {
                if (++wait_counter % 10 == 0) {
                    if (!gps_valid) {
                        OLED_Update("Waiting GPS...", NULL, NULL, NULL);
                    } else {
                        OLED_Update("Waiting RTC...", NULL, NULL, NULL);
                    }
                }
            }
            HAL_Delay(1000);
        }
    }
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_OFF;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) Error_Handler();
}

static void MX_GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
}

static void MX_I2C1_Init(void) {
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) Error_Handler();
}

static void MX_USART1_UART_Init(void) {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 9600;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) Error_Handler();
}

static void MX_USART2_UART_Init(void) {
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) Error_Handler();
}

void Error_Handler(void) {
    __disable_irq();
    while(1) {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(500);
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif

