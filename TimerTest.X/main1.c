/*
 * File:   main1.c
 * Author: claud
 *
 * Created on 03 October 2023, 15:23
 */

#include <math.h>
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

#define TIMER1 1
#define TIMER2 2
#define FOSC  7372800
#define MAXn 65535

void tmr_setup_period(int timer, int ms) {
    // Fcy = Fosc*10^6 / 4 = 7372800 / 4 = 1843200 (*0.5) (number of clocks in one second)
    // in 0.5 second there would be 921600 clocks steps
    // this is too high to be put in a 16 bit register (max 65535)
    // If we set a prescaler of 1:8 we have 921600/8 = PR1 clock steps, OK!
    double ms1 = (double)ms / 1000;
    double FCY = FOSC/4;
    double result = FCY * ms1;
    int prescaler = 0;
    switch (timer) {
        case 1: 
            TMR1 = 0;
            if (result <= MAXn) {
                T1CONbits.TCKPS = 0;
                prescaler = 1;
            } else if (result/MAXn > 1 && result/MAXn <= 8) {
                T1CONbits.TCKPS = 1;
                prescaler = 8;
            } else if (result/MAXn > 8 && result/MAXn <= 64) {
                T1CONbits.TCKPS = 2;
                prescaler = 64;
            } else {
                T1CONbits.TCKPS = 3;
                prescaler = 256;
            }
            PR1 = result / (double) prescaler;

            T1CONbits.TON = 1; // starts the timer!
            break;
        case 2: 
            TMR2 = 0;
            if (result <= MAXn) {
                T2CONbits.TCKPS = 0;
                prescaler = 1;
            } else if (result/MAXn > 1 && result/MAXn <= 8) {
                T2CONbits.TCKPS = 1;
                prescaler = 8;
            } else if (result/MAXn > 8 && result/MAXn <= 64) {
                T2CONbits.TCKPS = 2;
                prescaler = 64;
            } else {
                T2CONbits.TCKPS = 3;
                prescaler = 256;
            }
            PR2 = result / (double) prescaler;

            T2CONbits.TON = 1; // starts the timer!
            break;
    }
}

void tmr_wait_period(int timer){
    
    switch(timer){
        case 1:  
            while(IFS0bits.T1IF==0){
            }
            IFS0bits.T1IF=0;
            break;
        case 2:
            while(IFS0bits.T2IF==0){
            }
            IFS0bits.T2IF=0;
            break;
    }
    
}

void tmr_wait_ms(int timer, int ms){
    switch(timer){
        case 1:  
            tmr_setup_period(TIMER1, ms);
            tmr_wait_period(TIMER1);
            break;
        case 2:
            tmr_setup_period(TIMER2, ms);
            tmr_wait_period(TIMER2);
            break;
    }
}

int main(void) {
    // func 0: blink led for 500ms
    // func 1: blink led for different 
    // func 2:  Initially, D3 is off. Whenever S5 is pressed, D3 should increase its pulses, up to a
    //  maximum of 3. Once 3 is reached, a further press will reset the
    //  count to 1. If S5 is kept pressed for at least 3 seconds, D3 should be turned
    //  off.
    int func = 2;
    
    ADPCFG = 1;
    TRISBbits.TRISB0 = 0;
    
    switch(func){
        case 0:
            tmr_setup_period(TIMER1, 500);
            while(1){
                LATBbits.LATB0 = 1;
                tmr_wait_period(TIMER1);
                LATBbits.LATB0 = 0;
                tmr_wait_period(TIMER1);
            }
            break;
        case 1:
            LATBbits.LATB0 = 1;
            tmr_wait_ms(TIMER1, 1000);
            LATBbits.LATB0 = 0;
            tmr_wait_ms(TIMER1, 5000);
            LATBbits.LATB0 = 1;
            tmr_wait_ms(TIMER1, 500);
            LATBbits.LATB0 = 0;
            while(1){
            }
            break;
        case 2:
            TRISEbits.TRISE8 = 1;
            int mod = 0;
            int count = 0;
            int pressed = 0;
            int reset = 0;
            while(1){
                
                /*
                while(PORTEbits.RE8==0){
                    if(pressed==0){
                        mod+=1;
                        count=0;
                        if(mod>=4){
                            mod=1;
                        }
                        pressed = 1;
                    }
                    tmr_wait_ms(TIMER1, 100);
                    reset+=1;
                }
                if (reset>=30){
                    mod = 0;
                }
                if(PORTEbits.RE8==1 && pressed==1){
                    pressed = 0;
                    reset = 0;
                }
                */
                
                /*
                if(PORTEbits.RE8==0 && pressed==0){
                    mod+=1;
                    count=0;
                    if(mod>=4){
                        mod=1;
                    }
                    pressed = 1;
                } else if(PORTEbits.RE8==1 && pressed==1){
                    pressed = 0;
                }
                */
                
                if(PORTEbits.RE8==0 && pressed==0){
                    tmr_setup_period(TIMER2, 30000);
                    mod+=1;
                    count=0;
                    if(mod>=4){
                        mod=1;
                    }
                    pressed = 1;
                } else if(PORTEbits.RE8==1 && pressed==1){
                    T2CONbits.TON = 0;
                    pressed = 0;
                }
                
                if(IFS0bits.T2IF==1){
                    mod=0;
                    IFS0bits.T2IF=0;
                }
                

                switch(mod){
                    case 0:
                        LATBbits.LATB0 = 0;
                        break;
                    case 1:
                        if(count==0){
                            LATBbits.LATB0 = 1;
                            tmr_wait_ms(TIMER1, 100);
                            LATBbits.LATB0 = 0;
                            count+=1;
                        }else{
                            tmr_wait_ms(TIMER1, 100);
                            count+=1;
                            if(count>=10){
                                count=0;
                            }
                        }
                        break;
                    case 2:
                        if(count==0 || count==2){
                            LATBbits.LATB0 = 1;
                            tmr_wait_ms(TIMER1, 100);
                            LATBbits.LATB0 = 0;
                            count+=1;
                        }else{
                            tmr_wait_ms(TIMER1, 100);
                            count+=1;
                            if(count>=10){
                                count=0;
                            }
                        }
                        break;
                    case 3:
                        if(count==0 || count==2 || count==4){
                            LATBbits.LATB0 = 1;
                            tmr_wait_ms(TIMER1, 100);
                            LATBbits.LATB0 = 0;
                            count+=1;
                        }else{
                            tmr_wait_ms(TIMER1, 100);
                            count+=1;
                            if(count>=10){
                                count=0;
                            }
                        }
                        break;
                }   
            }          
    }
    return 0;
}
