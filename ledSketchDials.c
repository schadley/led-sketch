#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// Pins for interfacing with a SN74HC595 shift register
// Used to select which columns in a LED matrix should be lit
#define COL_SER     PC3
#define COL_RCLK    PC2
#define COL_SRCLK   PC1
#define COL_SRCLR   PC0
#define COL_DDR     DDRC
#define COL_PORT    PORTC

// Pins for interfacing with a TPIC6B596 shift register
// Used to select which rows in a LED matrix should be lit
#define ROW_SER     PB3
#define ROW_RCLK    PB2
#define ROW_SRCLK   PB1
#define ROW_SRCLR   PB0
#define ROW_DDR     DDRB
#define ROW_PORT    PORTB

#define SET_BIT(NUM, BIT)   (NUM |= (1 << BIT))
#define CLEAR_BIT(NUM, BIT) (NUM &= ~(1 << BIT))
#define TOGGLE_BIT(NUM, BIT)    (NUM ^= (1 << BIT))

// Array of patterns for each row
// Must be global to interact with interrupts
uint8_t rows[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

// Current position of the dials
// Must be global to interact with interrupts
uint8_t currentRow, currentCol;

void initRegisters() {
    // Initialize registers
    SET_BIT(COL_DDR, COL_SER);
    SET_BIT(COL_DDR, COL_RCLK);
    SET_BIT(COL_DDR, COL_SRCLK);
    SET_BIT(COL_DDR, COL_SRCLR);

    SET_BIT(ROW_DDR, ROW_SER);
    SET_BIT(ROW_DDR, ROW_RCLK);
    SET_BIT(ROW_DDR, ROW_SRCLK);
    SET_BIT(ROW_DDR, ROW_SRCLR);
}

void initADC4and5() {
    SET_BIT(ADMUX, REFS0);  // Use AVCC as reference
    // ADC clock division factor of 8
    SET_BIT(ADCSRA, ADPS1);
    SET_BIT(ADCSRA, ADPS0);
    SET_BIT(ADCSRA, ADEN);  // Enable ADC
    SET_BIT(ADMUX, MUX2);   // Set mux to ADC4
}

void initTimerInterrupt() {
    // Use 16-bit timer1 in normal mode, no need to set mode flags
    SET_BIT(TCCR1B, CS01); // Enable timer with prescaling by 8
    SET_BIT(TIMSK1, TOIE1); // Enable interrupts for timer1
    sei();
}

void clearColRegister() {
    // Clear the column shift register
    // Clear inputs are active low
    CLEAR_BIT(COL_PORT, COL_SRCLR);
    CLEAR_BIT(COL_PORT, COL_SRCLK);
    CLEAR_BIT(COL_PORT, COL_RCLK);
    _delay_us(1);
    SET_BIT(COL_PORT, COL_SRCLK);
    _delay_us(1);
    SET_BIT(COL_PORT, COL_RCLK);
    _delay_us(1);
    SET_BIT(COL_PORT, COL_SRCLR);
}

void clearRowRegister() {
    // Clear the row shift register
    // Clear inputs are active low
    CLEAR_BIT(ROW_PORT, ROW_SRCLR);
    CLEAR_BIT(ROW_PORT, ROW_SRCLK);
    CLEAR_BIT(ROW_PORT, ROW_RCLK);
    _delay_us(1);
    SET_BIT(ROW_PORT, ROW_SRCLK);
    _delay_us(1);
    SET_BIT(ROW_PORT, ROW_RCLK);
    _delay_us(1);
    SET_BIT(ROW_PORT, ROW_SRCLR);
}

// Display a pattern stored in rows[] a specified number of times
// Each loop takes approx 320 ns.
void displayPattern() {
    uint8_t i, j;

    for (i = 0; i < 8; ++i) {
        // Set clock low for the row output register
        CLEAR_BIT(ROW_PORT, ROW_RCLK);

        // Update the internal shift register for the row
        // Shift in 1 on the first cycle and a 0 every other cycle
        // SRCLK and RCLK are separated so each value
        // appears on the same cycle it is shifted in
        if (i == 0) {
            SET_BIT(ROW_PORT, ROW_SER);
        }
        else {
            CLEAR_BIT(ROW_PORT, ROW_SER);
        }
        CLEAR_BIT(ROW_PORT, ROW_SRCLK);
        _delay_us(1);
        SET_BIT(ROW_PORT, ROW_SRCLK);
        _delay_us(1);

        // Clear the column register before editing its value
        // to avoid the wrong LEDs being lit
        clearColRegister();

        // Set clock low for the column output register
        CLEAR_BIT(COL_PORT, COL_RCLK);

        // Update the internal shift register for the columns
        // Output will not change until a pulse on RCLK
        for (j = 0; j < 8; ++j) {
            // Set serial output high if
            // the bit in the row pattern is a 1
            if (bit_is_set(rows[i], j)) {
                SET_BIT(COL_PORT, COL_SER);
            }
            else {
                CLEAR_BIT(COL_PORT, COL_SER);
            }

            CLEAR_BIT(COL_PORT, COL_SRCLK);
            _delay_us(1);
            SET_BIT(COL_PORT, COL_SRCLK);
            _delay_us(1);
        }

        // Set clock high for the output registers
        SET_BIT(ROW_PORT, ROW_RCLK);
        // Only update the output register for the column
        // after the new row has been updated
        // to avoid the wrong LEDs being lit
        SET_BIT(COL_PORT, COL_RCLK);
        _delay_us(20);
    }
}

uint8_t readADC4() {
    CLEAR_BIT(ADMUX, MUX0); // Set mux to ADC4 (up/down)
    SET_BIT(ADCSRA, ADSC); // Start ADC conversion
    loop_until_bit_is_clear(ADCSRA, ADSC);
    // ADC value is 10 bits, use top 3 bits
    return (ADC >> 7);
}

uint8_t readADC5() {
    SET_BIT(ADMUX, MUX0); // Set mux to ADC5 (left/right)
    SET_BIT(ADCSRA, ADSC); // Start ADC conversion
    loop_until_bit_is_clear(ADCSRA, ADSC);
    // ADC value is 10 bits, use top 3 bits
    return (ADC >> 7);
}

ISR(TIMER1_OVF_vect) {
    TOGGLE_BIT(rows[currentRow], currentCol);
}

int main() {

    initRegisters();
    initADC4and5();
    initTimerInterrupt();
    clearColRegister();
    clearRowRegister();

    while (1) {
        // Don't display anything while ADC is running
        clearColRegister();

        currentRow = readADC4();
        currentCol = readADC5();

        displayPattern();
    }
}
