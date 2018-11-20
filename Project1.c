#define F_CPU 4000000UL

#define C_EEPROM 1024U

#include <avr/io.h>

#include <avr/interrupt.h>

#include <util/delay.h>
#include <avr/eeprom.h>

unsigned int ADC_read (unsigned int);

void EEPROM_write (unsigned int uiAddress, unsigned char ucData);

unsigned char EEPROM_read (unsigned int uiAddress);

void PORT_init (void);

void TIMER1_init (void);

void EEPROM_Bye (void); 

void USART_init (unsigned int baud);

void USART_transmit (unsigned char data);

unsigned char USART_receive (void);

void USART_FLUSH(void);

int recordTimeout = 0;

int switch2 = 0;

int main (void) {

	PORT_init();//initial values

	USART_init(7);

	//EEPROM Location
	unsigned int LastNote = 0;
	uint8_t writeAddress = 0; //from unsigned int

	uint8_t readAddress = 0; //from unsigned int

	//EEPROM Data storage 

	uint8_t data[8]; //from unsigned char
	
	TIMER1_init();

	while (1) {



		//Record Mode

		if (PINA & (1 << PA0)&& !(PINA & (1 << PA1))){
			
			writeAddress = 0;
						
			

	
		//need to clear EEPROM if we re-record shorter length segment

	while (PINA & (1 << PA0)) 
		{
        
		USART_FLUSH();
        
		for(int i=0; i<6;i++)
			{
			if(writeAddress == 0 && (i==3)){
			data[6] = 0;//TCNT1 >> 8;//TCNT1H
			data[7] = 0;// TCNT1 & 0xFF;//TCNT1L
			TCNT1 = 0;//When note begins
			
			}

		else if((i==3)){
			data[6] = TCNT1 >> 8;//TCNT1H
			data[7] = TCNT1 & 0xFF;//TCNT1L
			TCNT1 = 0;//When note begins
			}
            		
			data[i]= USART_receive();
         //   if(writeAddress == 0)
		//	 TCNT1 = 0;
				if ((i==1)&&(PINA & (1 << PA0))){
				PORTB = data[1];
			}


			}
			

			//when note ends 
	         for(int i=0; i<8;i++)
				{	
				if(!(PINA & (1 << PA0)))
				break;
				EEPROM_write(writeAddress++,data[i]);
				}
				LastNote=writeAddress;
		 	}
		 }




		//PLAYBACK LOOP
else if (PINA & (1 << PA1)){
	 
	 	readAddress = 0;
	
		while (PINA & (1 << PA1)){
			
			USART_FLUSH();
			unsigned int timeCounter = 0;
			unsigned int limit = EEPROM_read(C_EEPROM - 1); //Makes sure we don't exceed memory allocation
			
				while(readAddress < limit  && !switch2){       //limit might be adding extra time between loops 
					if (!(PINA & (1 << PA1))) {
							switch2 = 1;
				}

					else {

						for(uint8_t i = 0;i < 6; i++){

							if(readAddress <= limit - 8){
							timeCounter = EEPROM_read(readAddress + 6) <<8;
							timeCounter |= EEPROM_read(readAddress + 7);
						}
						
						if(PINA & (1<<PA2)){
						double adcValue;
						adcValue = (double) ADC_read(PINA & (1 << PA7));
						
						if(adcValue == 0){
						adcValue = 0.5 * (adcValue / 256);
						timeCounter *= adcValue;

						}

						else{
						adcValue = 2 * (adcValue / 256);
						timeCounter *= adcValue;
							}
						}

						while(TCNT1 < timeCounter ) {}  


							USART_transmit(EEPROM_read(readAddress + i));
							if (i==1 && !(EEPROM_read(readAddress + i)== 0xFF))
                              PORTB = EEPROM_read(readAddress + i);
						}


                    
						TCNT1 = 0;
						
						readAddress += 8;
						
					}			
			}
           
		}


   }
  }

   	
}



unsigned int ADC_read (unsigned int volt) {

	//Sets REFS1 to 1 if not already in ADMUX (sets reference volt)

	ADMUX |= volt;

	/*starts AC conversion of what is specified on the ADMUX ADC same as ADCSRA=1<<6. 
	The clock that the converter runs on is controlled by this 8-bit register. 
	We can change the factor it divided it by by using the ADPS registers inside of ADCSRA. 
	Again this changes the ADSC register which clears to 0 when it finishes the conversation.
    */

	ADCSRA |= 1 << ADSC;


	/* program will stay in a while loop until it finishes the conversation because circuit is running on a clock. */

	while(ADCSRA & (1<<ADSC)) {  }


	return ADC;

}

//from book

void EEPROM_write (unsigned int uiAddress, unsigned char ucData) 
{

	/* Wait for completion of previous write */

	while (EECR & (1 << EEWE)) 
		
		;

	/* Setup address and data registers */

	EEAR = uiAddress;

	EEDR = ucData;



	/* Write logical one to EEMWE */

	EECR |= (1 << EEMWE);



	/* Start EEPROM write by setting EEWE */

	EECR |= (1 << EEWE);

	//PORTB = ucData;

}


//From book
unsigned char EEPROM_read (unsigned int uiAddress) {

	/* Wait for completion of previous write */

	while (EECR & (1 << EEWE)) {  }



	/* Set up address register */

	EEAR = uiAddress;



	/* Start EEPROM read by writing EERE */

	EECR |= (1 << EERE);



	/* Return data from data register */

	return EEDR;

}





ISR (TIMER1_COMPB_vect) {

	PORTB = 0x00;// make sure this interupt is enabled

}


//initilize ports

void PORT_init (void) {

	// PORT A=0 

	DDRA = 0x00;

	// PORT B=1

	DDRB = 0xFF;

	// PORT D=0

	DDRD = 0x00;

	// portb set to off

	PORTB = 0x00;

	//Dividing Frequency 4MHz by a factor of 32 //CHANGE THIS BITCH TO MATCH WITH OUR PRESCALE

	ADCSRA |= (1 << ADPS2) | (1 << ADPS0);

	//turning on REFS0 when we pick our Vref

	ADMUX = (1 << REFS0);
	//selecting the photocell ADC7)

	ADMUX |= (1 << MUX2) | (1 << MUX1) | (1 << MUX0);

	//Enables ADC converstion but does not start it unless ADSC is on

	ADCSRA |= 1<<ADEN | 0<<ADSC;

}

//Timer
void TIMER1_init (void) {

	//scalar 256

	TCCR1B = (1 << CS12);

	//Timer=0
	TCNT1 = 0;

	//Compare interupts

	TIMSK = (1 << OCIE1A) | (1 << OCIE1B);

	//A for 4s, gives timer count of 62500

	OCR1A = (F_CPU * 4) / 256;


	//B for 800ms, timer count of 12500

	OCR1B = (F_CPU * 8) / (10 * 256);

	//Interrupts

	sei();
}


//from book

void USART_init (unsigned int baud) {

	/* Set baud rate */

	UBRRH = (uint8_t) (((uint16_t)baud) >> 8);

	UBRRL = (uint8_t) baud;



	/* Enable reciever and transmitter */

	UCSRC = (1 << URSEL) | (0b11<<UCSZ0);

	UCSRB = (1 << RXEN) | (1 << TXEN);

}

//from book 
void USART_FLUSH(void)
{
unsigned char dummy;
while(UCSRA & (1<<RXC)) dummy = UDR; 
}



//from book

void USART_transmit (unsigned char data) {

	/* Wait for empty transmit buffer */

	while (!(UCSRA & (1 << UDRE))) ;



	/* Put data into buffer, sends the data */

	UDR = data;

}


//from book

unsigned char USART_receive (void) 
{

	/* Wait for data to be received */



	while (!(UCSRA & (1 << RXC)) && (PINA & (1 << PA0))) {
			
	}



	/* Get and return received data from buffer */

	return UDR;

} 

