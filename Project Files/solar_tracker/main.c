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
    
    P1DIR |= BIT0;                            // Button P1.0 input
    P1OUT &= ~BIT0;                           // Button initialized low
// Enables RX and TX buffer
    P4SEL |= BIT5;                           
    P4SEL |= BIT4;
    
    UCA1CTL1 |= UCSWRST;                      // Software reset enable
    UCA1CTL1 |= UCSSEL_1;                     // USCI clock source select - ACLK
    UCA1BR0 = 3;                              // Baud rate clock divider1 for 9600 BR
    UCA1BR1 = 0;                              // Baud rate clock divider2 for 9600 BR
    UCA1MCTL |= UCBRS_3 | UCBRF_0;            // First and second stage modulation for higher accuracy baud rate
    UCA1CTL1 &= ~UCSWRST;                     // Sofware reset disable
    UCA1IE |= UCTXIE;                         // Enables Transfer buffer interrupt
}

void configurePWM()
{
   // Sets P1.2 as the output pin
   P1DIR |= BIT2;                             // Sets P1.2 as output driver for tilt Servo PWM control
   P1SEL |= BIT2;                             // Selects the port 1.2 as the TA0CCR1 interupt output
   P1DIR |= BIT3;                             // Sets P1.3 as output driver for rotational Servo PWM control
   P1SEL |= BIT3;                             // Selects the port 1.3 as the TA0CCR2 interupt output
   TA0CTL = TASSEL_1 | MC_1 | TACLR;          // Sets timerA_0 to SMCLK, up-mode, clears the register
   TA0CCR0 = 655;                             // Sets CCR0 max pwm
   TA0CCR1 = 50;                              // Sets CCR1 to initial value of 90 degree position for base motor
   TA0CCR2 = 50;                              // Sets CCR2 to initial value of 90 degree position for hub motor
   TA0CCTL1 = OUTMOD_7;                       // Output mode 7 reset/set
   TA0CCTL2 = OUTMOD_7;                       // Output mode 7 reset/set
}

void configureADC()
{
    P6SEL = 0x1F;                             // Enable 5 channel inputs, 6.0 - 6.4
    ADC12CTL0 = ADC12ON+ADC12MSC+ADC12SHT0_2; // Turn on ADC12, set sampling time
    ADC12CTL1 = ADC12SHP+ADC12CONSEQ_1;       // Use sampling timer, single sequence, reference voltage = 1.5v
    ADC12MCTL0 = ADC12INCH_0;                 // ref+=AVcc, channel = P6.0
    ADC12MCTL1 = ADC12INCH_1;                 // ref+=AVcc, channel = P6.1
    ADC12MCTL2 = ADC12INCH_2;                 // ref+=AVcc, channel = P6.2
    ADC12MCTL3 = ADC12INCH_3;                 // ref+=AVcc, channel = P6.3
    ADC12MCTL4 = ADC12INCH_4+ADC12EOS;        // ref+=AVcc, channel = P6.4, end of sequence
    ADC12IE = 0x10;                           // Enable ADC12IFG.4
    ADC12CTL0 |= ADC12ENC;                    // Enable conversions
}




int main(void)
{
    UCSCTL4 = SELA_0;                                    // Enables UART ACLK (32.768 kHz signal)
    WDTCTL = WDTPW | WDTHOLD;                            // Stop watchdog timer


    configureUART();                            // Initializes the required UART setup
    configurePWM();                             // Initializes the PWM peripheral set up
    configureADC();                             // Initializes the ADC configuration
    ButtonSetup();                              // For debouncing
while(1)
    {
            ADC12CTL0 |= ADC12SC;               // ADC sampling on
            __bis_SR_register(GIE);             // Enables Global Interrupt - ADC/UART interrupt support
            __no_operation();                   // For Debugger
    }
}

// ADC interupt protocol - implemented with TI resource code - thanks to Bhargavi Nisarga
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=ADC12_VECTOR
__interrupt void ADC12ISR (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC12_VECTOR))) ADC12ISR (void)
#else
#error Compiler not supported!
#endif
{

        topsensor = ADC12MEM0;                  // North photoresistor voltage ADC P6.0                 
        bottomsensor = ADC12MEM1;               // South photoresistor voltage ADC P6.1
        rightsensor = ADC12MEM2;                // East photoresistor voltage ADC P6.2
        leftsensor = ADC12MEM3;                 // West photoresistor voltage ADC P6.3
        solarvoltage = ADC12MEM4;               // System status - Photovoltaic cell output voltage
        volt = solarvoltage * 1.5;              // Math for reference voltage
        UCA1TXBUF = (int) volt;                 // Sends voltage of cell to UART transfer buffer
        P1OUT ^= BIT0;                          // Blinks on-board LED for debugging
    // Error calculations necessary for P-control algorithm - biased by a calibration coefficient
        LRdiff = (leftsensor - rightsensor) - LRCalibration;
        TBdiff = (bottomsensor  - topsensor) - TBCalibration;   
        topbottom += (TBdiff * 0.0025);
        leftright += (LRdiff * 0.0025);
    // Sets boundary limits for mechanical safety of the servo
    // Min Boundary [33] = 1ms time-on, 5% duty cycle, position 0 degree
    // Max Boundary [66] = 2ms time-on, 10% duty cycle, position 180 degree
        if (leftright <= 33) leftright = 33;
        if (leftright >= 66) leftright = 66;
        if (topbottom <= 33) topbottom = 33;
        if (topbottom >= 66) topbottom = 66;
  
        TA0CCR1 = leftright;                    // Sends new position to tilt servo
        TA0CCR2 = topbottom;                    // Sends new position to rotational servo

}
// Interrupt when button is pressed and released
// Calibration interrupt protocol
#pragma vector = PORT1_VECTOR               
__interrupt void Port_1(void)
{
    LRCalibration = leftsensor - rightsensor;   // East and West calibration 
    TBCalibration = topsensor - bottomsensor;   // North and South calibration
    ADC12CTL0 |= ADC12SC;                       // ADC sampling on
    P1IFG &= ~BIT1;                             // Clears button interrupt flag
}
// UART transfer buffer protocol
// Mitigates ISR trap
#pragma vector = USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
    ADC12CTL0 |= ADC12SC;                       // ADC sampling on
}
