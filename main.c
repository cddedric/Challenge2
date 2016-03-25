#include "msp432.h"
#include "driverlib.h"
#include "HAL_I2C.h"
#include "HAL_OPT3001.h"
#include <stdio.h>


void delay(void);
void buzzerInit(void);
void initPorts(void);
void Step5ConfigureRtcUsingRegisterAccess(void);
void Step6ConfigureADC14UsingDriverLib(void);


int main(void) 
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	char readable=1;
	char burglaryState=0;
	char alarm=0;
	long i;
	unsigned long int lux;

	int16_t xAxis;

	Step5ConfigureRtcUsingRegisterAccess();
	Step6ConfigureADC14UsingDriverLib();

	MAP_CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);
	initPorts();
	buzzerInit();

    Init_I2C_GPIO();
    I2C_init();
    OPT3001_init();

    __delay_cycles(100000);

	while(1){
		//1 check lux for above level
		//else mode check: burglar on/off
		//on: RED/GREEN LEDS
			//check mic for above level
		//off: LEDS off
//		if() //lux above level
//		else //check other things... fire is far more important


		lux = OPT3001_getLux();
		//if (lux>)

		if (!(P1IN & BIT1)){		//doorbell
			if (!alarm){
				TA0CCR0=16000;
			TA0CCR4 = 800;
			delay();
			TA0CCR4 = 12000;
			delay();
			TA0CCR4 = 0;	//turn off doorbell
			}
		}
		if ((!(P1IN & BIT4))&&readable){		//burglar alarm toggle
			if (burglaryState==0){	//burglar alarm deactive to active
				burglaryState=1;
				P2OUT &= ~BIT4;		//turn off GREEN
				P2OUT |= BIT6;		//turn on RED
				i=0;
				readable=0;
			}
			else{
				burglaryState=0; //
				P2OUT &= ~BIT6;
				P2OUT |= BIT4;
				TA0CCR4=0;
				i=0;
				readable=0;
				alarm=0;
			}
		}
		if(burglaryState==1){
			xAxis= (ADC14->MEM[0] &0x3FFF) - 8192;
			if (xAxis<0)
				xAxis =- xAxis;
			if (xAxis > 0x02FF){
				TA0CCR0 = 22000;
				TA0CCR4 = 16000;
				alarm=1;
			}
		}


		if (!(readable==0)){ //incrementer for debounce
			i++;
			if(i>=240000){
				i=0;
				readable=1;
			}
		}
	}// end of almighty while

}
void delay()      	// 1 second delay for 1.5 MHz clock
{
	long  dcntr;			// delay counter variable

	for (dcntr=0;dcntr<1960000;dcntr++);		//~1 second delay loop
	return;
}

void buzzerInit(){
	TA0CCR0 = 16000;
	TA0CCTL4 = OUTMOD_7;
	TA0CCR4 = 20;		//800 lowest activate tested, 20 is off
	TA0CTL = TASSEL__SMCLK | MC__UP | TACLR;
}

void initPorts(){
	P2SEL0 |= BIT7;		//buzzer
	P2SEL1 &= ~BIT7;	//buzzer function 1
	P2DIR |= BIT7;		//buzzer output

	P2DIR |= BIT4;		//GREEN LED output
	P2OUT |= BIT4;		//turn on green led
	P2DIR |= BIT6;		//RED LED output
	P2OUT &= ~BIT6;

	P1DIR &= ~BIT1;		//button 1 input
	P1REN |= BIT1;		//enable resisto ron button 1
	P1OUT |= BIT1;		//pull up resistor on button 1

	P1DIR &= ~BIT4;		//button 2 input
	P1REN |= BIT4;		//enabl resistor on button 2 input
	P1OUT |= BIT4;		//pull up resistor on button 2
}



void Step5ConfigureRtcUsingRegisterAccess()
{
    RTCCTL0_H = RTCKEY_H;                   // Unlock RTC
    RTCCTL1 |= RTCHOLD;                     // RTC hold
    // Enable prescaler interrupt, interval  = (1/32)s, dividied by 16
    RTCPS1CTL = RT1IP__16 | RT1PSIE;
    RTCCTL1 &= ~(RTCHOLD);                  // Start RTC
    RTCCTL0_H = 0;                          // Lock RTC module

    NVIC->ISER[0] = 1 << ((RTC_C_IRQn ) & 31); // Enable RTC interrupt in NVIC module
}

void Step6ConfigureADC14UsingDriverLib()
{

    // Configuring GPIOs for Analog In
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4,
            GPIO_PIN3, GPIO_TERTIARY_MODULE_FUNCTION);

    // Change resolution setting to 14-bit
    MAP_ADC14_setResolution(ADC_14BIT);

    // Init ADC14
    MAP_ADC14_initModule(ADC_CLOCKSOURCE_SYSOSC, ADC_PREDIVIDER_1, ADC_DIVIDER_1, 0);

    // Configuring ADC Memory ADC_MEM0 with no repeat with VCC reference
    ADC14_configureSingleSampleMode(ADC_MEM0, false);
    MAP_ADC14_configureConversionMemory(ADC_MEM0,
            ADC_VREFPOS_AVCC_VREFNEG_VSS,
            ADC_INPUT_A14, false);

    MAP_ADC14_setSampleHoldTime(ADC_PULSE_WIDTH_16, ADC_PULSE_WIDTH_16);

    // Setting up the sample timer to automatically step through the sequence convert.
    MAP_ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);

    // Enabling the interrupt when a conversion on channel 0
     //  is complete and enabling conversions
    MAP_ADC14_enableInterrupt(ADC_INT0);
    // Enabling Interrupts
    MAP_Interrupt_enableInterrupt(INT_ADC14);
    Interrupt_enableInterrupt(INT_ADC14);
    //SCB->SCR &= ~SCB_SCR_SLEEPONEXIT_Msk;        // Wake up on exit from ISR
    __enable_interrupt();                   // intrinsic to enable NVIC global/master interrupt
    // Alternatively, you can also use the DriverLib API
    // MAP_Interrupt_enableMaster();        //

    MAP_ADC14_enableModule();

}

