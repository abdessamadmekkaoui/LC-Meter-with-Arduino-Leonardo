//Project done and release by Group 3 on Project C mesure directed by Prof BABA


#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include "millis.c"


#define LCD_PORT PORTB
#define D4 PORTB4
#define D5 PORTB5
#define D6 PORTB6
#define D7 PORTB7
#define RS PORTD2
#define EN PORTD3


#define chargePin PORTC7
#define dechargePin PORTD6
#define F_CPU 16000000UL
#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

#define resistorValue 10000.0F

volatile uint32_t startTime;
volatile uint32_t elapsedTime;
volatile float microFarads;
volatile float nanoFarads;

uint16_t analogRead(uint8_t adcChannel) {
	// Ensure adcChannel is within the valid range (0 to 7)
	adcChannel &= 0x07;

	// Configure the ADC Multiplexer to select the specified ADC channel
	ADMUX = (ADMUX & 0xF0) | (adcChannel & 0x0F);

	// Initiate a single ADC conversion by setting the ADSC bit in ADCSRA
	ADCSRA |= (1 << ADSC);

	// Wait for the ADC conversion to complete by checking the ADSC bit
	while (ADCSRA & (1 << ADSC));

	// Return the 10-bit ADC result stored in the ADC register
	return ADC;
}
// Function to initialize the ADC (Analog-to-Digital Converter)
void initADC() {
	ADMUX = (1 << REFS0); // Set voltage reference to AVCC with external capacitor at AREF pin
	ADCSRA = (1 << ADEN)  // Enable ADC
	| (1 << ADPS2) // Set prescaler to 128
	| (1 << ADPS1)
	| (1 << ADPS0);
}

// Function to initialize Timer1 for a specific time interval
void initTimer1() {
	TCCR1B |= (1 << WGM12); // Set Timer1 to CTC (Clear Timer on Compare Match) mode
	OCR1A = (F_CPU / 1000) - 1; // Set the value to achieve a 1ms interval (assuming a 16MHz clock)
	TIMSK1 |= (1 << OCIE1A); // Enable Timer1 compare match interrupt
	TCCR1B |= (1 << CS11) | (1 << CS10); // Set prescaler to 64
	sei(); // Enable global interrupts
}
//customize delay
void custom_delay_ms(uint16_t milliseconds) {
	// Assuming a 16MHz clock, adjust accordingly
	for (uint16_t i = 0; i < milliseconds; i++) {
		for (uint16_t j = 0; j < 1141; j++) {
			asm volatile("nop"); 
		}
	}
}


// Function to send a command to the LCD
void LCD_command(unsigned char cmnd) {
	// Sending upper 4 bits of the command
	LCD_PORT = (LCD_PORT & 0x0F) | (cmnd & 0xF0);

	// RS = 0 for command
	PORTD &= ~(1 << RS);

	// Enable high-to-low pulse
	PORTD |= (1 << EN);
	_delay_us(1);
	PORTD &= ~(1 << EN);

	// Small delay
	_delay_us(200);

	// Sending lower 4 bits of the command
	LCD_PORT = (LCD_PORT & 0x0F) | ((cmnd << 4) & 0xF0);

	// Enable high-to-low pulse
	PORTD |= (1 << EN);
	_delay_us(1);
	PORTD &= ~(1 << EN);

	// Delay after sending the command
	_delay_ms(2);
}
// Function to set the cursor position on the LCD
// Parameters:
//   - row: The row index (0 or 1)
//   - col: The column index (0 to 15)
void LCD_set_cursor(uint8_t row, uint8_t col) {
	// Calculate the address based on row and col
	uint8_t address = (row == 0 ? 0x00 : 0x40) + col;

	// Set the cursor position command
	LCD_command(0x80 | address);
}

// Function to display a string on the LCD
// Parameters:
//   - data: The string to be displayed
// Function to send a string of data to be displayed on the LCD
// Parameters:
//   - data: The string of data to be displayed
void LCD_data_string(const char* data) {
	// Loop through each character in the data string
	for (size_t i = 0; i < strlen(data); ++i) {
		// Sending upper 4 bits
		LCD_PORT = (LCD_PORT & 0x0F) | (data[i] & 0xF0);

		// Set RS = 1 for data
		PORTD |= (1 << RS);

		// Enable high-to-low pulse
		PORTD |= (1 << EN);
		_delay_us(1);
		PORTD &= ~(1 << EN);

		// Small delay
		_delay_us(200);

		// Sending lower 4 bits
		LCD_PORT = (LCD_PORT & 0x0F) | ((data[i] << 4) & 0xF0);

		// Enable high-to-low pulse
		PORTD |= (1 << EN);
		_delay_us(1);
		PORTD &= ~(1 << EN);

		// Delay after sending each character
		_delay_ms(2);
	}
}


// Function to initialize the LCD
void LCD_init() {
	// Set data pins as output
	DDRB |= (1<<D4) | (1<<D5) | (1<<D6) | (1<<D7);
	// Set control pins as output
	DDRD |= (1<<RS) | (1<<EN);

	_delay_ms(15);

	LCD_command(0x02); // Initialize in 4-bit mode
	LCD_command(0x28); // 4-bit mode, 2 lines, 5x8 font
	LCD_command(0x0C); // Display ON, cursor OFF
	LCD_command(0x06); // Auto-increment cursor
	LCD_command(0x01); // Clear display

	_delay_ms(2);
	
		// Add the contrast control command here
LCD_command(0x39); // Function set: 8-bit, 2-line display
 LCD_command(0x70); // Set contrast value to maximum (adjust as needed)
 LCD_command(0x38);
 	_delay_ms(2);

}


int main(void) {
	
 
	CPU_PRESCALE(0);
	// Set chargePin as output
	
	DDRC |= (1 << chargePin);
	
	// Set chargePin LOW
	PORTC &= ~(1 << chargePin);

	initADC();
		
char buffer[20];
	init_millis(F_CPU);

	while(1){
		LCD_init();
		
		    memset(buffer, 0, sizeof(buffer));

		microFarads = 0;
		nanoFarads = 0;
		elapsedTime = 0;
		
		uint16_t adcValue ;
// Set chargePin HIGH
		PORTC |= (1 << chargePin);

		// Record start time
		startTime = millis();
		//usb_tx_uint(startTime);
		
		
		// Wait until analog voltage on analogPin is greater than or equal to 648 /
		//648 is 2/3 of 1024. 1024 being the highest value for the 10 bits ADC
		//648 is 63.2% of 1024… 1024 is the ADC reading for full voltage.
		while (adcValue < 648) {
			adcValue = analogRead(7);
		}
// 		_delay_ms(500	
	elapsedTime = millis() - startTime;	
		microFarads = ((float)elapsedTime / resistorValue)*1000;
						
							LCD_data_string("Capacity");
							LCD_set_cursor(1,0);
		
							
				// Format and display the result
				if (microFarads > 1) {
					
					snprintf(buffer, sizeof(buffer), "%d microFarads", (int)microFarads);
					LCD_data_string(buffer);
					custom_delay_ms(500);
					}
// 					else if(microFarads == 0) {
// 					LCD_data_string("No Data Found");
// 					custom_delay_ms(500);	
// 					}
					 else {
					nanoFarads = microFarads * 1000.0;
					snprintf(buffer, sizeof(buffer), "%d nanoFarads", (int)nanoFarads);
					LCD_data_string(buffer);
					custom_delay_ms(500);				}
					
					
		
		LCD_set_cursor(0,0);
		// Set chargePin  LOW
		PORTC &= ~(1 << chargePin);

		// Configure dischargePin as OUTPUT
		DDRB |= (1 << dechargePin);
		// Set  dischargePin LOW

		PORTB &= ~(1 << dechargePin);


		// Wait until analog voltage on analogPin is greater than 0
		//adc value decrement
		while (adcValue > 0) {
			adcValue = analogRead(7);
		}
		
		// Configure dischargePin as INPUT
		DDRB &= ~(1 << dechargePin);
	}
}
