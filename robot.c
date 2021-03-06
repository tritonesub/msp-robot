
#include <msp430.h>				
#include <msp430g2253.h>

#define STP 0x0
#define FWD 0x01
#define BWD 0x02
#define RTN 0x04
#define LTN 0x08
#define PROX_D 400

volatile unsigned int drive = STP;
volatile unsigned int prev_drive = STP;
volatile unsigned int prox_v = 0;
volatile unsigned int drive_counter = 0;

void switch_drive(int);

int main(void) {
	WDTCTL = WDTPW | WDTHOLD;		     // Stop watchdog timer

    TA0CCTL0 = CCIE;                     // CCR0 interrupt enabled
    TA0CTL = TASSEL_2 | MC_1 | ID_3;     // SMCLK/8, upmode
    TA0CCR0 = 10000;                     // 12.5 Hz

    P1OUT &= 0x00;                       // Shut 'er down
    P1DIR &= 0x00;
    P2OUT &= 0x00;
    P2DIR &= 0x00;

    P2SEL &= ~BIT0;
    P2SEL2 &= ~BIT0;
    
    P1DIR |= BIT0 + BIT6;                // P1.0 and P1.6 pin output the rest are input
    P2DIR |= BIT0 + BIT1 + BIT2 + BIT3 + BIT4;
    P2OUT &= ~(BIT0 + BIT1 + BIT2 + BIT3 + BIT4);
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

    drive = FWD;                         //start driving

    _bis_SR_register(CPUOFF + GIE);               // Enter LPM0 with all interrupts

    while(1) {
        switch(drive) {
            case STP:
                P2OUT &= ~BIT0;
                switch_drive(BWD);
                P1OUT ^= BIT0;
            break;
            
            case FWD:
                P2OUT &= ~BIT1;
                P2OUT &= ~BIT4;
                P2OUT |= BIT2;
                P2OUT |= BIT3;

                P2OUT |= BIT0;          //Enable
            break;

            case BWD:
                if(drive_counter == 0) {
                    P2OUT &= ~BIT2;
                    P2OUT &= ~BIT3;
                    P2OUT |= BIT1;
                    P2OUT |= BIT4; 
                    P2OUT |= BIT0;
                }
                drive_counter++;
                if(drive_counter > 12) {
                    switch_drive((prox_v % 2) ? RTN : LTN);
                }
            break;

            case RTN:
                if(drive_counter == 0) {
                    //pins to drive right
                    P2OUT &= ~BIT0;
                    P2OUT &= ~BIT4;
                    P2OUT &= ~BIT2;
                    P2OUT |= BIT1;
                    P2OUT |= BIT3;
                    P2OUT |= BIT0;
                }
                drive_counter++;
                if(drive_counter > 5) {
                    switch_drive(FWD);
                }
            break;

            case LTN:
                if(drive_counter == 0) {
                    //pins to drive left
                    P2OUT &= ~BIT0;
                    P2OUT &= ~BIT3;
                    P2OUT &= ~BIT1;
                    P2OUT |= BIT2;
                    P2OUT |= BIT4;
                    P2OUT |= BIT0;
                }
                drive_counter++;
                if(drive_counter > 5) {
                    switch_drive(FWD);
                }
            break;

            default:
                drive = STP;
        }
    
        _bis_SR_register(CPUOFF + GIE);
    }
}

void __attribute__((interrupt(TIMER0_A0_VECTOR))) TimerA_ISR(void)
{
    ADC10CTL0 |= ENC + ADC10SC;
}

void __attribute__((interrupt(ADC10_VECTOR))) ADC10_ISR(void) 
{
    //if we're driving forward and are not within range of an obstacle, keep driving forward. 
    //if we approach and obstacle or are backing up or turning exit low CPU for main loop to process.
    prox_v = ADC10MEM;
    if(prox_v > PROX_D) {
        P1OUT ^= BIT6;
        if(drive == FWD) {
            switch_drive(STP);
            _bic_SR_register_on_exit(CPUOFF);
        }
    }
//    else if(drive != FWD) {
        _bic_SR_register_on_exit(CPUOFF);
//    }
}

void __attribute__((interrupt(PORT1_VECTOR))) Port1_ISR(void)
{
    P1OUT ^= BIT6;              // Toggle P1.0
    P1IFG &= ~BIT3;             // P1.3 interrupt flag cleared
}

void switch_drive(int newDrive) {
    prev_drive = drive;
    drive = newDrive;
    drive_counter = 0;
}
