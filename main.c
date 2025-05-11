 /*
 * MAIN Generated Driver File
 * 
 * @file main.c
 * 
 * @defgroup main MAIN
 * 
 * @brief This is the generated driver implementation file for the MAIN driver.
 *
 * @version MAIN Driver Version 1.0.2
 *
 * @version Package Version: 3.1.2
*/

/*
© [2025] Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip 
    software and any derivatives exclusively with Microchip products. 
    You are responsible for complying with 3rd party license terms  
    applicable to your use of 3rd party software (including open source  
    software) that may accompany Microchip software. SOFTWARE IS ?AS IS.? 
    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS 
    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,  
    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY 
    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF 
    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE 
    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S 
    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT 
    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR 
    THIS SOFTWARE.
*/
#include "mcc_generated_files/system/system.h"
#define MAX_DELAY_COUNT (10)

volatile uint16_t delay_counter = 0;    //variable to store the count of the timer overflow

void Timer0_UserOverflowCallback(void); //Function prototype for user timer0 interrupt function

//void User_LED_YON_InterruptHandler(void);
//void User_START_IN_InterruptHandler(void);


typedef struct{                         //struct for input button readings with timer interrupt
    bool state;
    bool state_ex;
    uint8_t state_cnt;
}pin_type;

volatile pin_type start_in_pin;         //variables for start stop button input pin
volatile pin_type led_yon_pin;          //variables for direction button input pin

void pin_check(volatile pin_type*,bool);//input pin reading, called in timer interrupt

volatile bool chip_yon_pin_state = 0;   //(connected to motor driver chip DIR) output pin flag
/*
    Main application
*/

int main(void)
{
    SYSTEM_Initialize();
    // If using interrupts in PIC18 High/Low Priority Mode you need to enable the Global High and Low Interrupts 
    // If using interrupts in PIC Mid-Range Compatibility Mode you need to enable the Global and Peripheral Interrupts 
    // Use the following macros to: 

    // Enable the Global Interrupts 
    INTERRUPT_GlobalInterruptEnable(); 

    // Disable the Global Interrupts 
    //INTERRUPT_GlobalInterruptDisable(); 

    // Enable the Peripheral Interrupts 
    INTERRUPT_PeripheralInterruptEnable(); 

    // Disable the Peripheral Interrupts 
    //INTERRUPT_PeripheralInterruptDisable(); 

    
    start_in_pin.state = 0;
    start_in_pin.state_cnt = 0;
    start_in_pin.state_ex = 0;

    led_yon_pin.state = 0;
    led_yon_pin.state_cnt = 0;
    led_yon_pin.state_ex = 0;





    TMR0_OverflowCallbackRegister(Timer0_UserOverflowCallback);
    //LED_YON_SetInterruptHandler(User_LED_YON_InterruptHandler);
    //START_IN_SetInterruptHandler(User_START_IN_InterruptHandler);

    PWM2_Start();
    PWM2_DutyCycleSet(0);
    PWM2_LoadBufferSet();
    PWM2CONbits.OE = 1;


   //__delay_ms(500);
   volatile uint16_t duty_temp = 0;     //initial value is important
   volatile uint32_t hiz_adc = 0;
   volatile uint32_t hiz_adc_ex = 0;
   volatile uint32_t hiz_hedef = 0;
   
   volatile uint32_t rampa_adc = 0;

   volatile uint32_t step_delay = 0;
   volatile uint32_t step_delay_temp=0;

    while (1) {
        if (delay_counter >= MAX_DELAY_COUNT) //every (10ms * MAX_DELAY_COUNT)
        {
            delay_counter = 0; //resetting the delay counter

            hiz_adc = ADC1_ChannelSelectAndConvert(ADC_CHANNEL_ANA0);
            rampa_adc = ADC1_ChannelSelectAndConvert(ADC_CHANNEL_ANA1);
            if (rampa_adc < 10) rampa_adc = 10;

            if ((hiz_adc > hiz_adc_ex) & ((hiz_adc - hiz_adc_ex) > 100) |
                    !(led_yon_pin.state ^ chip_yon_pin_state)) {
                    if (hiz_adc < 10) {
                        step_delay_temp = (rampa_adc);
                    } else {
                        step_delay_temp = (rampa_adc * 1024) / (hiz_adc);
                    }
                    step_delay = step_delay_temp;
            }
            hiz_adc_ex = hiz_adc;
            if (hiz_adc > 1000) hiz_adc = 1000;
        }


        if (!(led_yon_pin.state ^ chip_yon_pin_state) &
             (start_in_pin.state == 1)) {
            hiz_hedef = hiz_adc;
        } else {
            hiz_hedef = 0;
        }

        if (duty_temp > hiz_hedef) {
            duty_temp--;
        }
        if (duty_temp < hiz_hedef) {
            duty_temp++;
        }

        PWM2_DutyCycleSet(duty_temp);
        PWM2_LoadBufferSet();
        for (uint32_t i = 0; i <= step_delay; i++) {
            __delay_us(3);
        }

        if (duty_temp == 0) {
            if (led_yon_pin.state) {
                CHIP_YON_SetHigh();
                chip_yon_pin_state = led_yon_pin.state;
            } else {
                CHIP_YON_SetLow();
                chip_yon_pin_state = led_yon_pin.state;
            }
        }
    }
}




void Timer0_UserOverflowCallback(void) //timer overflow interrupt every 10ms
{
    delay_counter++;

    pin_check(&start_in_pin, START_IN_GetValue());
    pin_check(&led_yon_pin, LED_YON_GetValue());
}


#define PIN_DEBOUNCE_TIME 5
void pin_check(volatile pin_type *pinx, bool read_pin) {
    if (read_pin == pinx->state_ex) {
        pinx->state_cnt++;
        if (pinx->state_cnt >= PIN_DEBOUNCE_TIME) {
            pinx->state = read_pin;
            pinx->state_cnt = PIN_DEBOUNCE_TIME; //to make it not to overflow
        }
    } else {
        pinx->state_ex = read_pin;
        pinx->state_cnt = 0;
    }
}
//void pin_check(volatile pin_type *pinx, bool read_pin) {
//    pinx->state_new = read_pin;
//    if (pinx->state_new == pinx->state_ex) {
//        pinx->state_cnt++;
//        if (pinx->state_cnt >= PIN_DEBOUNCE_TIME) {
//            pinx->state = pinx->state_new;
//            pinx->state_cnt = PIN_DEBOUNCE_TIME; //to make it not to overflow
//        }
//    } else {
//        pinx->state_ex = pinx->state_new;
//        pinx->state_cnt = 0;
//    }
//} 


/*void User_LED_YON_InterruptHandler(void){
}
void User_START_IN_InterruptHandler(void){
}*/

/*
        hiz_adc     0 ~ 1000    =   0 ~ %100 PWMDuty
        rampa_adc   0 ~ 1000    =   0 ~ 10 seconds total ramp time
     */   
