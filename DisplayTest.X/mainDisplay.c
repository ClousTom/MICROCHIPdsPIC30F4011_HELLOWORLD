/*
 * File:   mainDisplay.c
 * Author: claud
 *
 * Created on 19 October 2023, 17:57
 */

#include <stdio.h>
#include <unistd.h>
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

int x = 0;
char str[100];   

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
            T1CONbits.TON = 0;
            break;
        case 2:
            tmr_setup_period(TIMER2, ms);
            tmr_wait_period(TIMER2);
            T2CONbits.TON = 0;
            break;
    }
}

void __attribute__ (( __interrupt__ , __auto_psv__ )) _INT0Interrupt(){
    IFS0bits.INT0IF = 0; // reset interrupt flag
    x = 0;
}

void SPI_setup(){
    SPI1CONbits.MSTEN = 1; // master mode 
    SPI1CONbits.MODE16 = 0; // 8?bit mode
    SPI1CONbits.PPRE = 3; // 1:1 primary prescaler 
    SPI1CONbits.SPRE = 6; // 5:1 secondary prescaler 
    SPI1STATbits.SPIEN = 1; // enable SPI
}

void chartoSPI(char data){
    while(SPI1STATbits.SPITBF == 1); // wait until not full 
    SPI1BUF = data; // send the ?x? character 
}


void stringtoSPI(char* str){
    for(int i = 0; str[i] != '\0'; i++) {
        chartoSPI(str[i]);
    }
}

void cursorSPI(int row, int column) {
    switch (row) {
        case 0:
            chartoSPI(0x80 + column);
            return;
        case 1:
            chartoSPI(0xC0 + column);
            return;
    }
}

void clearDisplay(int mode){
    /*mode 0: clear first row
     * mode 1: clear second row
     * mode 2: clear everything
     */
    switch(mode){
        case 0:
            while(SPI1STATbits.SPITBF == 1); // wait until not full 
            cursorSPI(0,0);
            for (int i=0; i<16; i++){
                chartoSPI(' ');
            }
            cursorSPI(0,0);
            break;
        case 1:
            while(SPI1STATbits.SPITBF == 1); // wait until not full 
            cursorSPI(1,0);
            for (int i=0; i<16; i++){
                chartoSPI(' ');
            }
            cursorSPI(1,0);
            break;
        case 2:
            clearDisplay(0);
            clearDisplay(1);
            cursorSPI(0,0);
            break;
    }
}

int main(void) {
    // func 0: display x
    // func 1: display HEllO WORLD
    // func 2: count seconds on display
    // func 3: count seconds on display and reset it if press S5
    int func = 3;
    
    SPI_setup();
    
    tmr_wait_ms(TIMER1, 1000);
    
    switch(func){
        case 0:
            while(SPI1STATbits.SPITBF == 1); // wait until not full 
            SPI1BUF = 'c'; // send the ?x? character 
            while(1){
            }
            break;
        case 1: 
            stringtoSPI("Hello World!");
            while(1){
            }
            break;
        case 2:      
            while(1){
                sprintf(str, "count = %d", x++);
                stringtoSPI(str);
                tmr_wait_ms(TIMER1, 1000);
                clearDisplay(0);
            }
            break;
        case 3:
            TRISEbits.TRISE8 = 1; // put E8 (S5 button) in reading mode
            IEC0bits.INT0IE = 1; // enable S5 button interrupt!                 
            while(1){
                sprintf(str, "count = %d", x++);
                stringtoSPI(str);
                tmr_wait_ms(TIMER1, 1000);
                clearDisplay(0);
            }
            break;
    }    
    return 0;
}
