#include <msp430.h>

#define LED0 BIT0
#define LED1 BIT7
#define LED_OUT P1OUT
#define LED_OUT4 P4OUT
#define LED_DIR P1DIR
#define PULL_UP P1REN
#define LED_SEL P1SEL
#define LED_SEL2 P1SEL2
#define BUTTON BIT1
#define BTN_INT_EN P1IE
#define BTN_INT_EDGE P1IES
#define CLRFLG P1IFG

void ButtonSetup(void)
{
    LED_OUT |= BUTTON;              //Set pin 1.3 to a 1
    PULL_UP |= BUTTON;              //Enable a pull up resistor on 1.3
    BTN_INT_EN |= BUTTON;           // P1.1 interrupt enabled
    BTN_INT_EDGE |= BUTTON;         // P1.1 Hi/lo edge
    CLRFLG &= ~BUTTON;              // P1.1 IFG cleared
}
