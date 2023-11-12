/*
 * File:   mainInterrupts.c
 * Author: claud
 *
 * Created on 17 October 2023, 15:45
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
            IFS0bits.T1IF = 0;
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

void __attribute__ (( __interrupt__ , __auto_psv__ )) _T1Interrupt(){
    IFS0bits.T1IF = 0; // reset interrupt flag
    
    LATBbits.LATB1 ^= 1;
}

void __attribute__ (( __interrupt__ , __auto_psv__ )) _INT0Interrupt(){
    IFS0bits.INT0IF = 0; // reset interrupt flag
    
    LATBbits.LATB1 ^= 1;
}

int main(void) {
    // func 0:Blink D3 led at 1 Hz frequency (500ms time on, 500ms off)
    // without using interrupts; then make D4 blink at 2 Hz frequency
    // using interrupts.
    // func 1:Blink D3 led at 1 Hz frequency (500ms time on, 500ms off)
    // without using interrupts; every time the button S5 is pressed,
    // toggle the led D4 using interrupts.
    int func = 1;
    
    ADPCFG = 1;
    TRISBbits.TRISB0 = 0;
    TRISBbits.TRISB1 = 0;
    
    switch(func){
        case 0:
            IEC0bits.T1IE = 1; // enable TIMER1 interrupt!
            tmr_setup_period(TIMER1, 250);
            tmr_setup_period(TIMER2, 500);
            while(1){
                tmr_wait_period(TIMER2);
                LATBbits.LATB0 ^= 1;
            }
            break;
        case 1:
            TRISEbits.TRISE8 = 1; // put E8 (S5 button) in reading mode
            IEC0bits.INT0IE = 1; // enable S5 button interrupt!
            tmr_setup_period(TIMER2, 500);
            while(1){
                tmr_wait_period(TIMER2);
                LATBbits.LATB0 ^= 1;
            }
            break;
    }
    
            
    return 0;
}
