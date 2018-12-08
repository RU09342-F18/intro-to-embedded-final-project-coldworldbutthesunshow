#include <msp430.h> 

volatile float leftsensor;
volatile float rightsensor;
volatile float topsensor;
volatile float bottomsensor;
volatile float LRdiff, TBdiff;
volatile float leftright, topbottom;

volatile float LRCalibration, TBCalibration;

void ButtonSetup(void);

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
    /*
   //P6DIR &= ~BIT0 | ~BIT1 | ~BIT2 | ~BIT3;                                      // Sets P6.0 to input direction for ADC_12A input from voltage divider
   P6SEL |= BIT0 | BIT1 | BIT2 | BIT3;                                       // Sets P6.0 as the input for ADC12_A sample and conversion
   ADC12CTL2 = ADC12RES_2;                              // AD12_A resolution set to 12-bit
   ADC12CTL1 = ADC12SHP | ADC12CONSEQ1;                 // ADC12_A sample-and-hold pulse-mode select - SAMPCON signal is sourced from the sampling timer
   // ADC12_A Control Register 0 - 1024 cycles in a sampling period - Auto Trigger - Ref Volt off - Conversion overflow enable - Conversion enable - Start Sampling
   ADC12CTL0 = ADC12SHT1_15 | ADC12SHT0_15 | ADC12MSC | ADC12ON | ADC12TOVIE | ADC12ENC | ADC12SC;
   ADC12IE = 0x08;                           // Enables ADC12 interrupt
   //ADC12IFG &= ~ADC12IFG0 | ~ADC12IFG1 | ~ADC12IFG2 | ~ADC12IFG3;                              // Clears ADC12 interrupt flag
   ADC12MCTL0 = ADC12INCH_0;
   ADC12MCTL1 = ADC12INCH_1;
   ADC12MCTL2 = ADC12INCH_2;
   ADC12MCTL3 = ADC12INCH_3 + ADC12EOS;
   */
    P6SEL = 0x0F;                             // Enable A/D channel inputs
      ADC12CTL0 = ADC12ON+ADC12MSC+ADC12SHT0_2; // Turn on ADC12, set sampling time
      ADC12CTL1 = ADC12SHP+ADC12CONSEQ_1;       // Use sampling timer, single sequence
      ADC12MCTL0 = ADC12INCH_0;                 // ref+=AVcc, channel = A0
      ADC12MCTL1 = ADC12INCH_1;                 // ref+=AVcc, channel = A1
      ADC12MCTL2 = ADC12INCH_2;                 // ref+=AVcc, channel = A2
      ADC12MCTL3 = ADC12INCH_3+ADC12EOS;        // ref+=AVcc, channel = A3, end seq.
      ADC12IE = 0x08;                           // Enable ADC12IFG.3
      ADC12CTL0 |= ADC12ENC;                    // Enable conversions
}




int main(void)
{
    UCSCTL4 = SELA_0;                                    // Enables UART ACLK (32.768 kHz signal)
      WDTCTL = WDTPW | WDTHOLD;                            // Stop watchdog timer

      configurePWM();
      //configureUARTLED();
      //configureUART();
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
        // Temperature calculations
        topsensor = ADC12MEM0;                                // Sets the digital voltage value to float ADC for math
        bottomsensor = ADC12MEM1;
        rightsensor = ADC12MEM2;
        leftsensor = ADC12MEM3;

        LRdiff = (leftsensor - rightsensor) - LRCalibration;
        TBdiff = (topsensor - bottomsensor) - TBCalibration;

        topbottom += TBdiff * 0.05;
        leftright += LRdiff * 0.05;

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

    P1IFG &= ~BIT1;
}

