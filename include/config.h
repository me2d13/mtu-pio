#ifndef CONFIG_H
#define CONFIG_H

// create secrets.h file with your WiFi credentials by copying secrets_template.h to secrets.h and filling in your credentials
#include "secrets.h"

#endif // CONFIG_H

// Define I2C pins and frequency
#define SDA_PIN_LCD 1
#define SCL_PIN_LCD 2

#define SDA_PIN_SENSORS 3
#define SCL_PIN_SENSORS 8

#define PIN_UART_RX 4
#define PIN_UART_TX 5

#define PIN_12V_IN 12
#define PIN_OUT_PARK_LED 14
#define PIN_BUTTONS_INTERRUPT 10


#define PIN_MOTOR_ADDR_0 6
#define PIN_MOTOR_ADDR_1 7
#define PIN_MOTOR_ADDR_2 15

#define PIN_REVERSE_1 13
#define PIN_REVERSE_2 14

#define NUMBER_OF_DIGITAL_AXIS 5
#define NUMBER_OF_AXIS 7
#define NUMBER_OF_BUTTONS 16
#define REV_1_BUTTON_INDEX 16
#define REV_2_BUTTON_INDEX 17


// axis indexes in axes array
#define AXIS_INDEX_THROTTLE_1 1
#define AXIS_INDEX_THROTTLE_2 2
#define AXIS_INDEX_SPEED_BRAKE 0
#define AXIS_INDEX_FLAPS 3
#define AXIS_INDEX_REVERSE_1 5
#define AXIS_INDEX_REVERSE_2 6
#define AXIS_INDEX_TRIM 4

// axis indexes assigned to joystick axes
#define X_AXIS 1
#define Y_AXIS 2
#define Z_AXIS 3
#define RX_AXIS 5
#define RY_AXIS 6
#define BRAKE_AXIS 0
#define RZ_AXIS 4

// motor indexes in motors array
#define MOTOR_INDEX_THROTTLE_1 0
#define MOTOR_INDEX_THROTTLE_2 1
#define MOTOR_INDEX_SPEED_BRAKE 2
#define MOTOR_INDEX_TRIM 3
#define MOTOR_INDEX_TRIM_IND_1 4
#define MOTOR_INDEX_TRIM_IND_2 5

// motor adresses for UART switching
#define MOTORS_COUNT 6
#define MOTOR_THR1 4
#define MOTOR_THR2 5
#define MOTOR_SPEED_BRAKE 2
#define MOTOR_TRIM 3
#define MOTOR_TRIM_IND_1 0
#define MOTOR_TRIM_IND_2 1


#define I2C_CHANNEL_LCD 5
#define I2C_CHANNEL_THROTTLE_1 1
#define I2C_CHANNEL_THROTTLE_2 2
#define I2C_CHANNEL_SPEED_BRAKE 0
#define I2C_CHANNEL_FLAPS 3
#define I2C_CHANNEL_TRIM 4

#define ENABLE_LCD 1
#define ENABLE_SENSORS 1
#define ENABLE_HTTP_SERVER 1
#define ENABLE_JOYSTICK 1
#define ENABLE_UDP 1
#define ENABLE_NETWORK 1

#define AXIS_MAX_CALIBRATED_VALUE 10000

// do not move lever by motor if changes were caused by mtu (joystick move)
#define MOTORIZED_UPDATE_IGNORE_INTERVAL 1000 // 1 second

/*
                                                                              
                            ┌─────────────────┐                               
                        ┌───└─────────────────┘───┐                           
                        │3V3                   GND│                           
                        │3v3                GPIO43│  free                     
                        │RST                GPIO44│  free                     
               UART RX  │GPIO4               GPIO1│  SDA LCD                  
               UART TX  │GPIO5               GPIO2│  SCK LCD                  
             MOTOR ADD0 │GPIO6              GPIO42│  THR1 STEP                
             MOTOR ADD1 │GPIO7              GPIO41│  THR1 DIR                 
             MOTOR ADD2 │GPIO15             GPIO40│  THR2 STEP                
              ROTARY A  │GPIO16             GPIO39│  THR2 DIR                 
              ROTARY B  │GPIO17             GPIO38│  SPEED BRAKE STEP         
           ROTARY PRESS │GPIO18             GPIO37│  SPEED BRAKE DIR          
               SCK SENS │GPIO8              GPIO36│  TRIM STEP                
               SDA SENS │GPIO3              GPIO35│  TRIM DIR                 
                      x │GPIO46              GPIO0│  x                        
                        │GPIO9              GPIO45│  x                        
           BUTTONS INT  │GPIO10             GPIO48│  TRIM IND1 STEP           
                 5V IN  │GPIO11             GPIO47│  TRIM IND1 DIR            
                12V IN  │GPIO12             GPIO21│  TRIM IND2 STEP           
             REVERSE 1  │GPIO13             GPIO20│  TRIM IND2 DIR            
             REVERSE 2  │GPIO14             GPIO19│                           
                        │5V0                   GND│                           
                        │GND    USB   UART     GND│                           
                        └───────┌─┐───┌──┐────────┘                           
                                └─┘   └──┘                                    
                                                                              
*/        