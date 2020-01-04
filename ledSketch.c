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

// Pins for reading buttons
#define BUTTON_L    PD0
#define BUTTON_U    PD1
#define BUTTON_D    PD2
#define BUTTON_R    PD3
#define BUTTON_PIN  PIND
#define BUTTON_PORT PORTD

#define SET_BIT(NUM, BIT)   (NUM |= (1 << BIT))
#define CLEAR_BIT(NUM, BIT) (NUM &= ~(1 << BIT))

// Array of patterns for each row
// Must be global to interact with interrupts
uint8_t rows[8] = {0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
// Keep track of current row and column
// Start in top left corner
uint8_t currentRow = 0;
uint8_t currentCol = 7;

// Previous states of buttons
uint8_t prevL = (1 << BUTTON_L);
uint8_t prevU = (1 << BUTTON_U);
uint8_t prevD = (1 << BUTTON_D);
uint8_t prevR = (1 << BUTTON_R);

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

    // Enable pull-up resistors
    SET_BIT(BUTTON_PORT, BUTTON_L);
    SET_BIT(BUTTON_PORT, BUTTON_U);
    SET_BIT(BUTTON_PORT, BUTTON_D);
    SET_BIT(BUTTON_PORT, BUTTON_R);
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

void initPinChangeInterrupts() {
    SET_BIT(PCICR, PCIE2);      // Interrupt for D pins
    SET_BIT(PCMSK2, PCINT16);   // PD0
    SET_BIT(PCMSK2, PCINT17);   // PD1
    SET_BIT(PCMSK2, PCINT18);   // PD2
    SET_BIT(PCMSK2, PCINT19);   // PD3
    sei();                      // Global interrupt enable
}

// Display a pattern stored in rows[] a specified number of times
// Each loop takes approx 320 ns.
void displayPattern() {
    uint8_t i, j;

    for (i = 0; i < 8; ++i) {
        // Set clock low for the output registers
        CLEAR_BIT(ROW_PORT, ROW_RCLK);
        CLEAR_BIT(COL_PORT, COL_RCLK);

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
        SET_BIT(COL_PORT, COL_RCLK);
        SET_BIT(ROW_PORT, ROW_RCLK);
        _delay_us(20);
    }
}

// Check whether a button has been pressed
// Use debounce to avoid double presses
uint8_t checkPress(volatile uint8_t *reg, uint8_t bit, uint8_t *prev) {
    uint8_t current;

    // Check whether the button has changed position
    if ((*reg & (1 << bit)) != *prev) {
        // Check the value of the button after a
        // short delay to debounce the input
        // Clear the row register before the delay
        // to avoid one row being displayed too long
        clearRowRegister();
        _delay_ms(5);
        current = *reg & (1 << bit);

        // If the change in position is confirmed,
        // update the previous position and
        // return whether the button has been pressed
        if (current != *prev) {
            *prev = current;
            return (*prev == 0);
        }
    }
    return 0;
}

ISR(PCINT2_vect) {
    if (checkPress(&BUTTON_PIN, BUTTON_L, &prevL)) {
        // The MSB is the leftmost bit in each row
        // Thus, a movement left should increase the column
        if (currentCol < 7) {
            ++currentCol;
        }
    }
    else if (checkPress(&BUTTON_PIN, BUTTON_U, &prevU)) {
        // The first row in the array is the topmost row
        // Thus, a  movement up should decrease the row
        if (currentRow > 0) {
            --currentRow;
        }
    }
    else if (checkPress(&BUTTON_PIN, BUTTON_D, &prevD)) {
        if (currentRow < 7) {
            ++currentRow;
        }
    }
    else if (checkPress(&BUTTON_PIN, BUTTON_R, &prevR)) {
        if (currentCol > 0) {
            --currentCol;
        }
    }
    else {
        return;
    }
    // Update the LED pattern
    SET_BIT(rows[currentRow], currentCol);
}

int main() {
    initRegisters();
    clearColRegister();
    clearRowRegister();
    initPinChangeInterrupts();

    while (1) {
        displayPattern();
    }
}
