
#include <msp430.h>				
#include <msp430g2253.h>

#define STP 0x00;
#define FWD 0x01;
#define BWD 0x02;
#define RTN 0x04;
#define LTN 0x08;
#define PROX_D 400;

volatile unsigned int drive = STP;
volatile unsigned int prox_v = 0;

int main(void) {
	WDTCTL = WDTPW | WDTHOLD;		     // Stop watchdog timer

    TA0CCTL0 = CCIE;                     // CCR0 interrupt enabled
    TA0CTL = TASSEL_2 | MC_1 | ID_3;     // SMCLK/8, upmode
    TA0CCR0 = 10000;                     // 12.5 Hz

    P1OUT &= 0x00;                       // Shut 'er down
    P1DIR &= 0x00;

    P2SEL &= ~BIT0;
    P2SEL2 &= ~BIT0;
    
    P1DIR |= BIT0 + BIT6;                // P1.0 and P1.6 pin output the rest are input
    P2DIR |= BIT0 + BIT1 + BIT2 + BIT3 + BIT4 + BIT5;
    P2OUT &= ~(BIT0 + BIT1 + BIT2 + BIT3 + BIT4 + BIT5);
    //P1OUT |= BIT0;
    P1REN |= BIT3;                       // Enable internal pull-up
    P1OUT |= BIT3;                       // Select pullup

    P1IE |= BIT3;                        // P1.3 interrupt enabled
    P1IES |= BIT3;                       // P1.3 Hi/lo edge
    P1IFG &= ~BIT3;                      // P1.3 IFG cleared

    P1SEL |= BIT7;

    ADC10CTL1 = INCH_7 | ADC10DIV_3;          //SETUP ADC
    ADC10CTL0 = SREF_0 + ADC10SHT_3 + ADC10ON + ADC10IE;
    ADC10AE0 |= BIT7;


    _BIS_SR(CPUOFF + GIE);               // Enter LPM0 with all interrupts

    while(1) {
        switch(drive) {
            case: STP
                P2OUT &= ~(BIT0 + BIT1 + BIT2 + BIT3 + BIT4 + BIT5);
            break;
            
            case: FWD
            break;

            case: BWD
            break

            case: RTN
            break;

            case: LTN
            break;

            default:
                drive = STP;
        }
    }
}

void __attribute__((interrupt(TIMER0_A0_VECTOR))) TimerA_ISR(void)
{
    ADC10CTL0 |= ENC + ADC10SC;
}

void __attribute__((interrupt(ADC10_VECTOR))) ADC10_ISR(void) 
{
    prox_v = ADC10MEM;
    if(prox_v > PROX_D) {
        P1OUT |= BIT6;
        P2OUT &= ~BIT1;
        P2OUT |= BIT2;
        P2OUT |= BIT0;
    }
    else {
        P1OUT &= ~BIT6;
        P2OUT &= ~BIT0;
        P2OUT &= ~BIT2;
    }
}

void __attribute__((interrupt(PORT1_VECTOR))) Port1_ISR(void)
{
    P1OUT ^= BIT6;              // Toggle P1.0
    P1IFG &= ~BIT3;             // P1.3 interrupt flag cleared
}
