#include <msp430.h> 

volatile float leftsensor;
volatile float rightsensor;
volatile float topsensor;
volatile float bottomsensor;
volatile double LRdiff, TBdiff;
volatile double leftright, topbottom;

volatile float LRCalibration, TBCalibration, volt, solarvoltage;

void ButtonSetup(void);
void configureUART()
{
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;

    P4SEL |= BIT5;                                       // Enables RX and TX buffer
    P4SEL |= BIT4;
    UCA1CTL1 |= UCSWRST;                                 // Software reset enable
    UCA1CTL1 |= UCSSEL_1;                                // USCI clock source select - ACLK
    UCA1BR0 = 3;                                         // Baud rate clock divider1 for 9600 BR
    UCA1BR1 = 0;                                         // Baud rate clock divider2 for 9600 BR
    UCA1MCTL |= UCBRS_3 | UCBRF_0;                       // First and second stage modulation for higher accuracy baud rate
    UCA1CTL1 &= ~UCSWRST;
    UCA1IE |= UCTXIE;                                    // Enables Transfer buffer interrupt
}

void configurePWM()
{
   // Sets P1.2 as the output pin
   P1DIR |= BIT2;                                       // Sets P1.2 as output driver for pwm for fan speed control
   P1SEL |= BIT2;                                       // Selects the port 1.2 as the timer A output
   P1DIR |= BIT3;
   P1SEL |= BIT3;
   TA0CTL = TASSEL_1 | MC_1 | TACLR;                    // Sets timerA_0 to SMCLK, up-mode, clears the register
   TA0CCR0 = 655;                                     // Sets CCR0 max pwm
   TA0CCR1 = 50;                                      // Sets CCR1 to initial value of 90 degree position for base motor
   TA0CCR2 = 50;                                      // Sets CCR2 to initial value of 90 degree position for hub motor
   TA0CCTL1 = OUTMOD_7;                                 // Output mode 7 reset/set
   TA0CCTL2 = OUTMOD_7;                                 // Output mode 7 reset/set
}

void configureADC()
{
    P6SEL = 0x1F;                             // Enable A/D channel inputs
    ADC12CTL0 = ADC12ON+ADC12MSC+ADC12SHT0_2; // Turn on ADC12, set sampling time
    ADC12CTL1 = ADC12SHP+ADC12CONSEQ_1;       // Use sampling timer, single sequence
    ADC12MCTL0 = ADC12INCH_0;                 // ref+=AVcc, channel = A0
    ADC12MCTL1 = ADC12INCH_1;                 // ref+=AVcc, channel = A1
    ADC12MCTL2 = ADC12INCH_2;                 // ref+=AVcc, channel = A2
    ADC12MCTL3 = ADC12INCH_3;                 // ref+=AVcc, channel = A3, end seq.
    ADC12MCTL4 = ADC12INCH_4+ADC12EOS;
    ADC12IE = 0x10;                           // Enable ADC12IFG.4
    ADC12CTL0 |= ADC12ENC;                    // Enable conversions
}




int main(void)
{
    UCSCTL4 = SELA_0;                                    // Enables UART ACLK (32.768 kHz signal)
    WDTCTL = WDTPW | WDTHOLD;                            // Stop watchdog timer


    configureUART();
    configurePWM();
    configureADC();
    ButtonSetup();
while(1)
    {
            ADC12CTL0 |= ADC12SC;
            __bis_SR_register(GIE);              // Enables Global Interrupt - ADC/UART interrupt support
            __no_operation();
    }
}


#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=ADC12_VECTOR
__interrupt void ADC12ISR (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC12_VECTOR))) ADC12ISR (void)
#else
#error Compiler not supported!
#endif
{

        topsensor = ADC12MEM0;                                // Sets the digital voltage value to float ADC for math
        bottomsensor = ADC12MEM1;
        rightsensor = ADC12MEM2;
        leftsensor = ADC12MEM3;
        solarvoltage = ADC12MEM4;
        volt = solarvoltage;

        UCA1TXBUF = (int) volt;
        P1OUT ^= BIT0;
        LRdiff = (leftsensor - rightsensor) - LRCalibration;
        TBdiff = (bottomsensor  - topsensor) - TBCalibration;

        topbottom += (TBdiff * 0.005);
        leftright += (LRdiff * 0.005);

        if (leftright <= 33) leftright = 33;
        if (leftright >= 66) leftright = 66;

        if (topbottom <= 33) topbottom = 33;
        if (topbottom >= 66) topbottom = 66;

        TA0CCR1 = leftright;
        TA0CCR2 = topbottom;

}

#pragma vector = PORT1_VECTOR               // Interrupt when button is pressed and released
__interrupt void Port_1(void)
{
    LRCalibration = leftsensor - rightsensor;
    TBCalibration = topsensor - bottomsensor;
    ADC12CTL0 |= ADC12SC;
    P1IFG &= ~BIT1;
}

#pragma vector = USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
    ADC12CTL0 |= ADC12SC;
}
