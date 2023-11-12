/*
 * Authors: Claudio Tomaiuolo (5630055)
 *          Teodoro Lima (5452613)
 */

#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <xc.h>
#include <libpic30.h>

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

//Setup for TIMERs
#define TIMER1 1
#define TIMER2 2
#define TIMER3 3
#define TIMER4 4
#define TIMER5 5
#define FOSC  7372800
#define MAXn 65535

//Global variables shared within interrupts
int charsRCVD = 0; //counts for received datas from UART2
int buttonS5State = 0; // 0 = unpressed; 1 = pressed

// Circular Array struct
#define SIZE_OF_BUFFER 15 // round((9600(baud rate)/10(signals for a byte))/10(program running at 100Hz)) + 20% more or less
typedef struct { 
    char array[SIZE_OF_BUFFER];
    int front, rear;
} CircularArray;

CircularArray circularArray; // declaration of a Circular Array

void initCircularArray(CircularArray *circularArray) { // Initialization of an empty Circular Array
    circularArray->front = -1;
    circularArray->rear = -1;
}

int isFull(CircularArray *circularArray){
    return (circularArray->front == 0 && circularArray->rear == SIZE_OF_BUFFER - 1) || (circularArray->front == circularArray->rear + 1);
}

int isEmpty(CircularArray *circularArray){
    return circularArray->front == -1;
}

void enqueue(CircularArray *circularArray, int data) { //Add data to the circular array
    if (!isFull(circularArray)) {
        if (circularArray->front == -1)
            circularArray->front = circularArray->rear = 0;
        else if (circularArray->rear == SIZE_OF_BUFFER - 1)
            circularArray->rear = 0;
        else
            circularArray->rear++;

        circularArray->array[circularArray->rear] = data;
    }
}

char dequeue(CircularArray *circularArray) { // Get and remove a data from the circular array
    if (!isEmpty(circularArray)) {
        char data = circularArray->array[circularArray->front];
            
        if (circularArray->front == circularArray->rear)
            circularArray->front = circularArray->rear = -1;
        else if (circularArray->front == SIZE_OF_BUFFER - 1)
            circularArray->front = 0;
        else
            circularArray->front++;

        return data;
    }
}

// TIMER SETUP
void tmr_setup_period(int timer, int ms) { 
    // Fcy = Fosc*10^6 / 4 = 7372800 / 4 = 1843200 (*0.5) (number of clocks in one second)
    // in 0.5 second there would be 921600 clocks steps
    // this is too high to be put in a 16 bit register (max 65535)
    // If we set a prescaler of 1:8 we have 921600/8 = PR1 clock steps, OK!
    double ms1 = (double)ms / 1000;
    double FCY = FOSC/4;
    double result = FCY * ms1;
    int prescaler, tckps;
    if (result <= MAXn) {
        prescaler = 1;
        tckps = 0;
    } else if (result/MAXn > 1 && result/MAXn <= 8) {
        prescaler = 8;
        tckps = 1;
    } else if (result/MAXn > 8 && result/MAXn <= 64) {
        prescaler = 64;
        tckps = 2;
    } else {
        prescaler = 256;
        tckps = 3;
    }
    switch (timer) {
        case 1: 
            TMR1 = 0;
            T1CONbits.TCKPS = tckps;
            PR1 = result / (double) prescaler;
            T1CONbits.TON = 1; // starts the timer!
            break;
        case 2: 
            TMR2 = 0;
            T2CONbits.TCKPS = tckps;
            PR2 = result / (double) prescaler;
            T2CONbits.TON = 1; // starts the timer!
            break;
        case 3: 
            TMR3 = 0;
            T3CONbits.TCKPS = tckps;
            PR3 = result / (double) prescaler;
            T3CONbits.TON = 1; // starts the timer!
            break;
        case 4: 
            TMR4 = 0;
            T4CONbits.TCKPS = tckps;
            PR4 = result / (double) prescaler;
            T4CONbits.TON = 1; // starts the timer!
            break;
        case 5: 
            TMR5 = 0;
            T5CONbits.TCKPS = tckps;
            PR5 = result / (double) prescaler;
            T5CONbits.TON = 1; // starts the timer!
            break;
    }
}

void tmr_wait_period(int timer){
    switch(timer){
        case 1:  
            while(IFS0bits.T1IF==0){}
            IFS0bits.T1IF=0;
            break;
        case 2:
            while(IFS0bits.T2IF==0){}
            IFS0bits.T2IF=0;
            break;
        case 3:
            while(IFS0bits.T3IF==0){}
            IFS0bits.T3IF=0;
            break;
        case 4:
            while(IFS1bits.T4IF==0){}
            IFS1bits.T4IF=0;
            break;
        case 5:
            while(IFS1bits.T5IF==0){}
            IFS1bits.T5IF=0;
            break;
    }  
}

void tmr_wait_ms(int timer, int ms){
    tmr_setup_period(timer, ms);
    tmr_wait_period(timer);
    switch(timer){
        case 1:  
            T1CONbits.TON = 0;
            break;
        case 2:
            T2CONbits.TON = 0;
            break;
        case 3:  
            T3CONbits.TON = 0;
            break;
        case 4:
            T4CONbits.TON = 0;
            break;
        case 5:
            T5CONbits.TON = 0;
            break;
    }
}

void TimerInterruptEnabler(int timer, int interrupt){
    switch(timer){
        case 1: 
            if(interrupt==1){
                IEC0bits.T1IE = 1; //enable Timer 1 interrupt
            }else{
                IEC0bits.T1IE = 0; //disable Timer 1 interrupt
            }
            break;
        case 2:
            if(interrupt==1){
                IEC0bits.T2IE = 1; //enable Timer 2 interrupt
            }else{
                IEC0bits.T2IE = 0; //disable Timer 2 interrupt
            }
            break;
        case 3:
            if(interrupt==1){
                IEC0bits.T3IE = 1; //enable Timer 2 interrupt
            }else{
                IEC0bits.T3IE = 0; //disable Timer 2 interrupt
            }
            break;
    }
}

// SPI SETUP
void SPI_setup(){
    SPI1CONbits.MSTEN = 1; // master mode 
    SPI1CONbits.MODE16 = 0; // 8?bit mode
    SPI1CONbits.PPRE = 3; // 1:1 primary prescaler 
    SPI1CONbits.SPRE = 6; // 5:1 secondary prescaler 
    SPI1STATbits.SPIEN = 1; // enable SPI
    
    tmr_wait_ms(TIMER1, 1000); // wait 1000ms until SPI is ready
}

void chartoSPI(char data){ // send a char to the SPI
    while(SPI1STATbits.SPITBF == 1); // wait until not full 
    SPI1BUF = data; // send the character  to SPI
}

void stringtoSPI(char* str){ // send one by one the chars of a string to the SPI
    for(int i = 0; str[i] != '\0'; i++) {
        chartoSPI(str[i]);
    }
}

void cursorSPI(int row, int column) { //function to manage the SPI cursor
    switch (row) {
        case 0:
            chartoSPI(0x80 + column);
            return;
        case 1:
            chartoSPI(0xC0 + column);
            return;
    }
}

void clearDisplay(int mode){ //clean the DISPLAY
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
            break;
        case 1:
            while(SPI1STATbits.SPITBF == 1); // wait until not full 
            cursorSPI(1,0);
            for (int i=0; i<16; i++){
                chartoSPI(' ');
            }
            break;
        case 2:
            clearDisplay(0);
            clearDisplay(1);
            break;
    }
}


void buttonEnabler(int button, int interrupt){ // buttons Enabler function with Interrupts enabler
    /* button 0:S5 button in reading mode
     * button 1:S6 button in reading mode
     * interrupt 0:interrupt for the respective button disabled
     * interrupt 1:interrupt for the respective button enabled
     */
    if(button==0){
        TRISEbits.TRISE8 = 1; // put E8 (S5 button) in reading mode
        if(interrupt==1){
            IEC0bits.INT0IE = 1; //Enable the interrupt for S5 button
        } else {
            IEC0bits.INT0IE = 0;
        }
    } else if(button==1){
        TRISDbits.TRISD0 = 1; // put R0 (S6 button) in reading mode
        if(interrupt==1){
            IEC1bits.INT1IE = 1; //Enable the interrupt for S6 button
        } else {
            IEC1bits.INT1IE = 0;
        }
    } 
}

void __attribute__ ((__interrupt__ , __auto_psv__)) _T3Interrupt(){ // interrupt for S5 button bouncing
    T3CONbits.TON = 0; // disable timer3
    IFS0bits.T3IF = 0; // reset interrupt flag
    
    if(PORTEbits.RE8 != 0){ // check S5 button status
        buttonS5State = 1; // actions in the main
    } else {
        TMR3 = 0; // reset TIMER3
        T3CONbits.TON = 1; // start TIMER3
    }
}

void __attribute__ (( __interrupt__ , __auto_psv__ )) _INT0Interrupt(){ //S5 button interrupt
    IFS0bits.INT0IF = 0; // reset interrupt flag
    tmr_setup_period(TIMER3, 20); // timer3 for debouncing
}

/*
void __attribute__ (( __interrupt__ , __auto_psv__ )) _INT1Interrupt(){ //S6 button interrupt
    IFS1bits.INT1IF = 0; // reset interrupt flag
}*/


void UARTEnabler(int uart, int interrupt){ //UART enabler function 
    /* case 0: enable UART1 (UART1 pins are shared with SPI pins. It is not possible to use them together. Either use Alternate UART1 pins, or use UART 2)
     * case 1: enable UART2
     * case 2: enable UART1 alternative (not implemented yet)
     */
    switch(uart){
        case 0:
            U1BRG = 11; // (7372800 / 4) / (16 * 9600) ? 1
            U1MODEbits.UARTEN = 1; // enable UART
            U1STAbits.UTXEN = 1; // enable U1TX (must be after UARTEN)
            break;
        case 1:
            U2BRG = 11; // (7372800 / 4) / (16 * 9600) ? 1
            U2MODEbits.UARTEN = 1; // enable UART2
            U2STAbits.UTXEN = 1; // enable U1TX (must be after UARTEN)
            if(interrupt==1){
                IEC1bits.U2RXIE = 1; //interrupt enabled
            }
            break;
        //case 2:
        //    U1MODEbits.ALTIO = 1;
        //    break;
    }
}

void __attribute__((interrupt, no_auto_psv)) _U2RXInterrupt(void) { // UART2 interrupt
    enqueue(&circularArray, U2RXREG);
    charsRCVD++;
    IFS1bits.U2RXIF = 0; // Clear the interrupt flag
}

void sendData(int uart, char data){ //send data to the UART
    switch(uart){
        case 0:
            U1TXREG = data;
            break;
        case 1:
            U2TXREG = data;
            break;
    }
}

void sendString(int uart, char* str){ //send one by one the chars of a string to the UART
    for(int i = 0; str[i] != '\0'; i++) {
        sendData(uart, str[i]);
    }
}

void writeCharsRCVD(char* str, int columnIndex){ // function to write the second row saying how many chars we received
    clearDisplay(1);
    cursorSPI(1,0);
    sprintf(str, "Char Recv: %d", charsRCVD);
    stringtoSPI(str);
    cursorSPI(0,columnIndex);
}


int main(void){
    SPI_setup();
    buttonEnabler(0,1);
    buttonEnabler(1,0);
    UARTEnabler(1,1);
    TimerInterruptEnabler(TIMER3,1);
    
    initCircularArray(&circularArray); // initialization in the circular array
    
    char* str[16]; //used to write the numbers of received datas from UART2 (display columns are 16)
    
    int columnIndex = 0; //counts the column we are writing on the display (max 16)
    char data; // used to store the char taken from the circular array
    
    // Simulate an algorithm that needs 7 ms for its execution, and needs to work at 100 Hz
    tmr_setup_period(TIMER1, 10);
    
    while(1){
        tmr_wait_ms(TIMER2,7);
        
        if(buttonS5State == 1){ // handle S5 button functions
            sprintf(str, "%d", charsRCVD);
            sendString(1,&str);
            buttonS5State = 0;
        }
        if(IFS1bits.INT1IF == 1){ // handle S6 button functions directly with the interrupt flag
            IFS1bits.INT1IF = 0;
            clearDisplay(0);
            columnIndex = 0;
            charsRCVD = 0;
            writeCharsRCVD(&str, columnIndex);
        }

        while(U2STAbits.UTXBF); //Wait until the UART2 buffer is not empty

        if(!isEmpty(&circularArray)){
            data = dequeue(&circularArray);
            if (data=='\r' || data=='\n'){ // handles special characters
                clearDisplay(0);
                columnIndex=0;
            } else {
                if(columnIndex>15){ // if the cursor reaches the end of the display, it starts again from the beginning
                    clearDisplay(0);
                    columnIndex=0;
                }

                cursorSPI(0,columnIndex);
                chartoSPI(data); // send the char to the SPI
                columnIndex++;
            }
            writeCharsRCVD(&str, columnIndex);
        }
        
        tmr_wait_period(TIMER1);
    }
    return 0;
}