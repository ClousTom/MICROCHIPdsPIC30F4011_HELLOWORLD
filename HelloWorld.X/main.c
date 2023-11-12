/*
 * File:   main.c
 * Author: claud
 *
 * Created on 26 September 2023, 12:19
 */


#include <xc.h>
// FOSC
#pragma config FPR = XT // Primary Oscillator Mode (XT)
#pragma config FOS = PRI // Oscillator Source (Primary Oscillator)
#pragma config FCKSMEN = CSW_FSCM_OFF // Clock Switching and Monitor (Sw Disabled, Mon Disabled)
// FWDT
#pragma config FWPSB = WDTPSB_16 // WDT Prescaler B (1:16)
#pragma config FWPSA = WDTPSA_512 // WDT Prescaler A (1:512)
#pragma config WDT = WDT_OFF // Watchdog Timer (Disabled)
// FBORPOR
#pragma config FPWRT = PWRT_64 // POR Timer Value (64ms)
#pragma config BODENV = BORV20 // Brown Out Voltage (Reserved)
#pragma config BOREN = PBOR_ON // PBOR Enable (Enabled)
#pragma config LPOL = PWMxL_ACT_HI // Low?side PWM Output Polarity (Active High)
#pragma config HPOL = PWMxH_ACT_HI // High?side PWM Output Polarity (Active High)
#pragma config PWMPIN = RST_IOPIN // PWM Output Pin Reset (Control with PORT/TRIS regs)
#pragma config MCLRE = MCLR_EN // Master Clear Enable (Enabled)
// FGS
#pragma config GWRP = GWRP_OFF // General Code Segment Write Protect (Disabled)
#pragma config GCP = CODE_PROT_OFF // General Segment Code Protection (Disabled)
// FICD
#pragma config ICS = ICS_PGD


int main(void) {
    //Case:
    //  0. Led ON
    //  1. Led ON when you press and keep the bottom S5 pressed
    //  2. Led ON/OFF pressing one time the bottom S5
    int func = 1;
 
    ADPCFG = 1;
    TRISBbits.TRISB0 = 0;
    
    switch(func){
    case 0: 
        LATBbits.LATB0 = 1;
        break; 
    case 1: 
        TRISEbits.TRISE8 = 1;
        while(1){
            if(PORTEbits.RE8==0){    
                LATBbits.LATB0 = 1;
            }else{
               LATBbits.LATB0 = 0;
            }
        }
        break; 
    case 2: 
        TRISEbits.TRISE8 = 1;
        int bottom_value = PORTEbits.RE8;
        int led_value = 0; 
        while(1){
            if(PORTEbits.RE8!=bottom_value && led_value==0){
                LATBbits.LATB0 = 1;
                led_value = 1;
                bottom_value = PORTEbits.RE8;
            }else if(PORTEbits.RE8!=bottom_value && led_value==1){
                LATBbits.LATB0 = 0;
                led_value = 0;
                bottom_value = PORTEbits.RE8;
            }
        }
        break;
    }
    return 0;
}
