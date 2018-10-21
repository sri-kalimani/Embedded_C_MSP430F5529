#include <msp430.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "peripherals.h"


void runTimerA2(int value);
void stoptimerA2(int reset);
void swDelay(char numLoops);
void swDelay(char numLoops);
void DACInit(void);
void DACSetValue(unsigned int dac_code);
void configPin();
float readPotVoltage();


int state;
char currButton = 0;
unsigned long timer_cnt;
int leap_cnt;
unsigned int input[2];
unsigned long last_cnt;
int ST_resolution = 3.3/4095;
int potValue;



int main (){


    WDTCTL = WDTPW + WDTHOLD;      // Stop WDT
    _BIS_SR(GIE);

    configPin();
    configDisplay();
    configKeypad();
    configButtons();
    DACInit();

    Graphics_clearDisplay(&g_sContext);

    state = 0;

    //runTimerA2();

    int i = 0;

    while (1){

        currButton = buttonState();

        switch (state){

        case 0:

            Graphics_drawStringCentered(&g_sContext, "Function", AUTO_STRING_LENGTH, 48, 20, OPAQUE_TEXT);
            Graphics_drawStringCentered(&g_sContext, "Generator", AUTO_STRING_LENGTH, 48, 30, OPAQUE_TEXT);
            Graphics_drawStringCentered(&g_sContext, "Press:", AUTO_STRING_LENGTH, 48, 40, OPAQUE_TEXT);
            Graphics_drawStringCentered(&g_sContext, "1.    DC wave", AUTO_STRING_LENGTH, 48, 50, OPAQUE_TEXT);
            Graphics_drawStringCentered(&g_sContext, "2.    Sq wave", AUTO_STRING_LENGTH, 48, 60, OPAQUE_TEXT);
            Graphics_drawStringCentered(&g_sContext, "3.    St wave", AUTO_STRING_LENGTH, 48, 70, OPAQUE_TEXT);
            Graphics_drawStringCentered(&g_sContext, "4.    /\\ wave", AUTO_STRING_LENGTH, 48, 80, OPAQUE_TEXT);

            Graphics_flushBuffer(&g_sContext);

            if (currButton == 0x01){
                state = 1;
                Graphics_clearDisplay(&g_sContext);
                Graphics_drawStringCentered(&g_sContext, "DC wave", AUTO_STRING_LENGTH, 48, 30, OPAQUE_TEXT);
                Graphics_flushBuffer(&g_sContext);
                stoptimerA2(1);
                runTimerA2(6552);
            }

            if (currButton == 0x02){
                state = 2;
                Graphics_clearDisplay(&g_sContext);
                Graphics_drawStringCentered(&g_sContext, "Square wave", AUTO_STRING_LENGTH, 48, 30, OPAQUE_TEXT);
                Graphics_flushBuffer(&g_sContext);
                stoptimerA2(1);
                runTimerA2(6552);
            }

            if (currButton == 0x04){
                state = 3;
                Graphics_clearDisplay(&g_sContext);
                Graphics_drawStringCentered(&g_sContext, "Sawtooth wave", AUTO_STRING_LENGTH, 48, 30, OPAQUE_TEXT);
                Graphics_flushBuffer(&g_sContext);
                stoptimerA2(1);
                runTimerA2(278);
            }

            if (currButton == 0x08){
                state = 4;
                Graphics_clearDisplay(&g_sContext);
                Graphics_drawStringCentered(&g_sContext, "Triangle wave", AUTO_STRING_LENGTH, 48, 30, OPAQUE_TEXT);
                   Graphics_flushBuffer(&g_sContext);
                   stoptimerA2(1);
                   runTimerA2(208);


            }

            break;

        case 1:


            potValue = (int)readPotVoltage();
            DACSetValue(potValue);

            break;

        case 2:

            input[0] = 0;
            input[1] = 4095;
            last_cnt = timer_cnt;

           while(1) {

               while (timer_cnt == last_cnt){
                   __no_operation();
               }


               DACSetValue(input[timer_cnt & 1]);
               last_cnt = timer_cnt;
           }
            break;

        case 3:

               DACSetValue(timer_cnt*(ST_resolution));

            break;

        case 4:

            DACSetValue(timer_cnt*(ST_resolution));


            break;

        } //switch


    } // while

} //main

float readPotVoltage(){
    unsigned int potReading = 0;
    float potVolts;

    ADC12CTL0 = ADC12SHT0_9 | ADC12ON; // Turn on ADC
    ADC12CTL1 = ADC12SHP; // Sample and Hold mode
    ADC12MCTL0 = ADC12SREF_0 | ADC12INCH_0; //ref = 3.3V, ch = A0
    P6SEL |= BIT0; // Port 6 pin 0 to function mode in ADC
    ADC12CTL0 &= ~ADC12SC; // clear start bit
    // enable and start single conversion
    ADC12CTL0 |= ADC12SC | ADC12ENC;
    while (ADC12CTL1 & ADC12BUSY)
    __no_operation();
    potReading = ADC12MEM0 & 0x0FFF; //keep low 12 bits
    potVolts = potReading * 3.3/ 4096; // convert to volts
    return potVolts;
}

void runTimerA2(int value){
    TA2CTL = TASSEL_2 | MC_1 | ID_0;
    TA2CCR0 = value; //6552
    TA2CCTL0 = CCIE;
}

void stoptimerA2(int reset){
    TA2CTL = MC_0; //stop timer
    TA2CCTL0 &= ~CCIE;
    if (reset)
        timer_cnt = 0;
}

#pragma vector=TIMER2_A0_VECTOR
__interrupt void TimerA2_ISR (void){
        timer_cnt++;

        if (state == 3){
            if (timer_cnt == 50){
                timer_cnt = 0;
            }
        }

        if (state == 4){
            if (timer_cnt == 25){
                timer_cnt -= 2;
            }
        }
}

void swDelay(char numLoops)
{
    // This function is a software delay. It performs
    // useless loops to waste a bit of time
    //
    // Input: numLoops = number of delay loops to execute
    // Output: none
    //
    // smj, ECE2049, 25 Aug 2013

    volatile unsigned int i,j;  // volatile to prevent removal in optimization
                                // by compiler. Functionally this is useless code

    for (j=0; j<numLoops; j++)
    {
        i = 50000 ;                 // SW Delay
        while (i > 0)               // could also have used while (i)
           i--;
    }
}

/** **********************************************
 * Initialize the DAC and its associated SPI bus,
 * using parameters defined in peripherals.h
 ************************************************/
void DACInit(void)
{
    // Configure LDAC and CS for digital IO outputs
    DAC_PORT_LDAC_SEL &= ~DAC_PIN_LDAC;
    DAC_PORT_LDAC_DIR |=  DAC_PIN_LDAC;
    DAC_PORT_LDAC_OUT |= DAC_PIN_LDAC; // Deassert LDAC

    DAC_PORT_CS_SEL   &= ~DAC_PIN_CS;
    DAC_PORT_CS_DIR   |=  DAC_PIN_CS;
    DAC_PORT_CS_OUT   |=  DAC_PIN_CS;  // Deassert CS
}



void DACSetValue(unsigned int dac_code)
{
    // Start the SPI transmission by asserting CS (active low)
    // This assumes DACInit() already called
    DAC_PORT_CS_OUT &= ~DAC_PIN_CS;

    // Write in DAC configuration bits. From DAC data sheet
    // 3h=0011 to highest nibble.
    // 0=DACA, 0=buffered, 1=Gain=1, 1=Out Enbl
    dac_code |= 0x3000;     // Add control bits to DAC word

    uint8_t lo_byte = (unsigned char)(dac_code & 0x00FF);
    uint8_t hi_byte = (unsigned char)((dac_code & 0xFF00) >> 8);

    // First, send the high byte
    DAC_SPI_REG_TXBUF = hi_byte;

    // Wait for the SPI peripheral to finish transmitting
    while(!(DAC_SPI_REG_IFG & UCTXIFG)) {
        _no_operation();
    }

    // Then send the low byte
    DAC_SPI_REG_TXBUF = lo_byte;

    // Wait for the SPI peripheral to finish transmitting
    while(!(DAC_SPI_REG_IFG & UCTXIFG)) {
        _no_operation();
    }

    // We are done transmitting, so de-assert CS (set = 1)
    DAC_PORT_CS_OUT |=  DAC_PIN_CS;

    // This DAC is designed such that the code we send does not
    // take effect on the output until we toggle the LDAC pin.
    // This is because the DAC has multiple outputs. This design
    // enables a user to send voltage codes to each output and
    // have them all take effect at the same time.
    DAC_PORT_LDAC_OUT &= ~DAC_PIN_LDAC;  // Assert LDAC
     __delay_cycles(10);                 // small delay
    DAC_PORT_LDAC_OUT |=  DAC_PIN_LDAC;  // De-assert LDAC
}

void configPin(){
    P6SEL &= ~BIT1;
    P6DIR &= ~BIT1;

    P6REN &= BIT1;
    P6OUT |= BIT1;
}

