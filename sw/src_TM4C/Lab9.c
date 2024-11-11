// Lab9.c
// Runs on TM4C123 (not LF120)
// 
// Daniel and Jonathan Valvano
// 
// Mark McDermott
// Add: MQTT.c ESP8266.c
// July 1, 2024
// Bump switches
//   TM4C MSPM0
//   PA5  PA27    Left, Bump 0,
//   PF0  PB15    Center Left, Bump 1,
//   PB3  PA28    Center Right, Bump 2
//   PC4  PA31    Right, Bump 3

// Motor
//   TM4C MSPM0
//   PF2  PB4  Motor_PWML, ML+, IN3, PWM M1PWM6
//   PF3  PB1  Motor_PWMR, MR+, IN1, PWM M1PWM7
//   PA3  PB0  Motor_DIR_L,ML-, IN4, GPIO 0 means forward, 1 means backward
//   PA2  PB16 Motor_DIR_R,MR-, IN2, GPIO 0 means forward, 1 means backward

// tachometer
//   TM4C MSPM0
//   PB7  PB8  ELA  T0CCP1
//   PB6  PB7  ERB  GPIO input 
//   PA4  PB6  ELB  GPIO input 
//   PB2  PB12 ERA  T3CCP0

// SSD1306 I2C OLED
//   TM4C MSPM0
//   PA6  PB2  SCL I2C clock
//   PA7  PB3  SDA I2C data

// IR analog distance sensors
//   TM4C MSPM0
//   PE4  PA26 Right  Ain9
//   PE5  PB24 Center Ain8 
//   PE1  PA24 Left   Ain2

// TF Luna TOF distance sensor
//   TM4C MSPM0
//   PB1  PA8 TxD microcontroller sensor RxD pin 2
//   PB0  PA9 RxD microcontroller sensor TxD pin 3

// LaunchPad pins
//   PF1 red LED 
//   PF2 blue LED 
//   PF3 green LED 
//   PF4 S2 negative logic switch
//   PF0 S1 negative logic switch (shared with Bump 1)

/* Hardware connections
  Vcc should be connected to RSLK 3.3V (not debugger)
 /------------------------------\
 |              chip      1   8 |
 | Ant                    2   7 |
 | enna       processor   3   6 |
 |                        4   5 |
 \------------------------------/ 
 Set #define  for UART2 (PD) and Reset PB5 
 
 ESP8266    TM4C123 
  1 URxD    PD7   UART out of TM4C123, 115200 baud
  2 GPIO0         +3.3V for normal operation (ground to flash)
  3 GPIO2         Status
  4 GND     Gnd   GND (70mA)
  5 UTxD    PD6   UART out of ESP8266, 115200 baud
  6 Ch_PD         chip select, 10k resistor to 3.3V
  7 Reset   PB5   TM4C123 can issue output low to cause hardware reset
  8 Vcc           regulated 3.3V supply with at least 70mA
 */
 
 
#include <stdio.h>
#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"


#include "../inc/ST7735.h"
#include "../inc/PLL.h"
#include "../inc/Timer2A.h"
#include "../inc/Timer5A.h"
#include "../inc/UART.h"
#include "../inc/UART2.h"
#include "../inc/UART5.h"
#include "../inc/esp8266.h"
#include "../inc/MQTT.h"




//----- Prototypes of functions in startup.s  ----------------------
//
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // Go into low power mode


// -----------------------------------------------------------------
// -------------------- MAIN LOOP ----------------------------------
//
int main(void){       
  PLL_Init(Bus80MHz);         // Bus clock at 80 MHz
  DisableInterrupts();        // Disable interrupts until finished with inits
  Output_Init();              // Initialize ST7735 LCD
	UART_Init();                // Enable Debug Serial Port
  UART2_Init(115200);         // Enable Debug Serial Port
  UART5_Init();               // Enable ESP8266 Serial Port
  Reset_8266();               // Reset the WiFi module
  SetupWiFi();                // Setup communications to MQTT Broker via 8266 WiFi
  
  Timer2A_Init(&MQTT_to_TM4C, 400000, 3);         // Get data every 5ms 
  Timer5A_Init(&TM4C_to_MQTT, 80000000, 3);       // Send data back to MQTT Web App every second 
  
  EnableInterrupts();

  while(1){   
    WaitForInterrupt();       // Wait to run the clock until the next interrupt
  }
}



