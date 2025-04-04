#ifndef CONFIG_H
#define CONFIG_H

#define WIFI_SSID ""
#define WIFI_PASSWORD ""

#define ENABLE_HTTP_SERVER 1

#endif // CONFIG_H

// Define I2C pins and frequency
#define SDA_PIN_LCD 1
#define SCL_PIN_LCD 2

#define SDA_PIN_SENSORS 3
#define SCL_PIN_SENSORS 8

#define PIN_UART_RX 4
#define PIN_UART_TX 5


#define PIN_MOTOR_ADDR_0 6
#define PIN_MOTOR_ADDR_1 7
#define PIN_MOTOR_ADDR_2 15

#define NUMBER_OF_AXIS 6
#define NUMBER_OF_BUTTONS 32
#define X_AXIS 0
#define Y_AXIS 1
#define Z_AXIS 2
#define RX_AXIS 3
#define RY_AXIS 4
#define RZ_AXIS 5

#define MOTOR_THR_1 0
#define MOTOR_THR_2 1
#define MOTOR_SPEED_BRAKE 2
#define MOTOR_TRIM 3
#define MOTOR_TRIM_IND_1 4
#define MOTOR_TRIM_IND_2 5

#define I2C_CHANNEL_LCD 5

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