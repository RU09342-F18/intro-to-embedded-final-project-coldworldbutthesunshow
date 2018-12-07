#include <msp430.h> 

volatile float leftsensor;
volatile float rightsensor;
volatile float topsensor;
volatile float bottomsensor;
volatile float LRdiff, TBdiff;
volatile float leftright, topbottom;


void configurePWM()
{
   // Sets P1.2 as the output pin
   P1DIR |= BIT2;                                       // Sets P1.2 as output driver for pwm for fan speed control
   P1SEL |= BIT2;                                       // Selects the port 1.2 as the timer A output
   P1DIR |= BIT3;
   P1SEL |= BIT3;
   TA0CTL = TASSEL_2 | MC_1 | TACLR;                    // Sets timerA_0 to SMCLK, up-mode, clears the register
   TA0CCR0 = 19999;                                     // Sets CCR0 max pwm
   TA0CCR1 = 1499;                                      // Sets CCR1 to initial value of 90 degree position for base motor
   TA0CCR2 = 1499;                                      // Sets CCR2 to initial value of 90 degree position for hub motor
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
   switch (__even_in_range(ADC12IV,34))
   {

   case 12:
   {
        // Temperature calculations
        topsensor = ADC12MEM0;                                // Sets the digital voltage value to float ADC for math
        bottomsensor = ADC12MEM1;
        rightsensor = ADC12MEM2;
        leftsensor = ADC12MEM3;

        LRdiff = leftsensor - rightsensor;
        TBdiff = topsensor - bottomsensor;
        if (TBdiff > 10)
        {
            topbottom += TBdiff * 2;
        }
       if (LRdiff > 10)
       {
           leftright += LRdiff * 2;
       }
        if (leftright <= 999)
                    {
                        TA0CCR1 = 999;
                    }

                    else if (leftright >= 1999)
                    {
                        TA0CCR1 = 1999;
                    }

                    else if (leftright < 999)
                    {
                        TA0CCR1 = 999;
                    }
                    else
                    {
                        TA0CCR1 = leftright;
                    }
        if (topbottom <= 999)
                         {
                             TA0CCR2 = 999;
                         }

                         else if (topbottom >= 1999)
                         {
                             TA0CCR2 = 1999;
                         }

                         else if (topbottom < 999)
                         {
                             TA0CCR2 = 999;
                         }
                         else
                         {
                             TA0CCR2 = topbottom;
                         }
                break;
   }
   default: break;
   }
}



