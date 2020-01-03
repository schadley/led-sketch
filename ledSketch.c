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
#define col_clk_up  (COL_PORT |= ((1 << COL_RCLK) | (1 << COL_SRCLK)))
#define col_clk_dn  (COL_PORT &= ~((1 << COL_RCLK) | (1 << COL_SRCLK)))

// Pins for interfacing with a TPIC6B596 shift register
// Used to select which rows in a LED matrix should be lit
#define ROW_SER     PB3
#define ROW_RCLK    PB2
#define ROW_SRCLK   PB1
#define ROW_SRCLR   PB0
#define ROW_DDR     DDRB
#define ROW_PORT    PORTB
#define row_clk_up  (ROW_PORT |= ((1 << ROW_RCLK) | (1 << ROW_SRCLK)))
#define row_clk_dn  (ROW_PORT &= ~((1 << ROW_RCLK) | (1 << ROW_SRCLK)))

// Pins for reading buttons
#define BUTTON_L    PD0
#define BUTTON_U    PD1
#define BUTTON_D    PD2
#define BUTTON_R    PD3
#define BUTTON_PIN  PIND
#define BUTTON_PORT PORTD

// Array of patterns for each row
// Must be global to interact with interrupts
uint8_t rows[8] = {0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
// Keep track of current row and column
// Start in top left corner
uint8_t currentRow = 0;
uint8_t currentCol = 7;

void initRegisters() {
    // Initialize registers
    COL_DDR |= (1 << COL_SER);
    COL_DDR |= (1 << COL_RCLK);
    COL_DDR |= (1 << COL_SRCLK);
    COL_DDR |= (1 << COL_SRCLR);

    ROW_DDR |= (1 << ROW_SER);
    ROW_DDR |= (1 << ROW_RCLK);
    ROW_DDR |= (1 << ROW_SRCLK);
    ROW_DDR |= (1 << ROW_SRCLR);

    // Enable pull-up resistors
    BUTTON_PORT |= (1 << BUTTON_L);
    BUTTON_PORT |= (1 << BUTTON_U);
    BUTTON_PORT |= (1 << BUTTON_D);
    BUTTON_PORT |= (1 << BUTTON_R);
}

void clearShiftRegisters() {
    // Clear the shift registers
    // Clear inputs are active low
    COL_PORT &= ~(1 << COL_SRCLR);
    ROW_PORT &= ~(1 << ROW_SRCLR);
    col_clk_dn;
    row_clk_dn;
    _delay_us(10);
    col_clk_up;
    row_clk_up;
    _delay_us(10);
    COL_PORT |= (1 << COL_SRCLR);
    ROW_PORT |= (1 << ROW_SRCLR);
}

void initPinChangeInterrupts() {
    PCICR |= (1 << PCIE2);      // Interrupt for D pins
    PCMSK2 |= (1 << PCINT16);   // PD0
    PCMSK2 |= (1 << PCINT17);   // PD1
    PCMSK2 |= (1 << PCINT18);   // PD2
    PCMSK2 |= (1 << PCINT19);   // PD3
    sei();                      // Global interrupt enable
}

ISR(PCINT2_vect) {
    if (bit_is_clear(BUTTON_PIN, BUTTON_L)) {
        // The MSB is the leftmost bit in each row
        // Thus, a movement left should increase the column
        if (currentCol < 7) {
            ++currentCol;
        }
    }
    else if (bit_is_clear(BUTTON_PIN, BUTTON_U)) {
        // The first row in the array is the topmost row
        // Thus, a  movement up should decrease the row
        if (currentRow > 0) {
            --currentRow;
        }
    }
    else if (bit_is_clear(BUTTON_PIN, BUTTON_D)) {
        if (currentRow < 7) {
            ++currentRow;
        }
    }
    else if (bit_is_clear(BUTTON_PIN, BUTTON_R)) {
        if (currentCol > 0) {
            --currentCol;
        }
    }
    else {
        return;
    }
    // Update the LED pattern
    rows[currentRow] |= (1 << currentCol);
}

int main() {
    uint8_t i, j;

    initRegisters();
    clearShiftRegisters();
    initPinChangeInterrupts();

    // Shift in a 1 to avoid a wasted cycle -- see below
    row_clk_dn;
    ROW_PORT |= (1 << ROW_SER);
    _delay_us(20);
    row_clk_up;
    _delay_us(20);

    while (1) {
        for (i = 0; i < 8; ++i) {
            // Set clock low for the row shift reg
            // and for the output register of the column shift reg
            row_clk_dn;
            COL_PORT &= ~(1 << COL_RCLK);

            // Update the register for the row
            // The output reg is always 1 cycle behind the internal shift reg
            // Shift in 1 on the last cycle so a 1 appears on the first cycle
            if (i == 7) {
                ROW_PORT |= (1 << ROW_SER);
            }
            else {
                ROW_PORT &= ~(1 << ROW_SER);
            }

            // Update the internal shift register for the columns
            // Output will not change until a pulse on RCLK
            for (j = 0; j < 8; ++j) {
                // Set serial output high if
                // the bit in the row pattern is a 1
                if (rows[i] & (1 << j)) {
                    COL_PORT |= (1 << COL_SER);
                }
                else {
                    COL_PORT &= ~(1 << COL_SER);
                }

                COL_PORT &= ~(1 << COL_SRCLK);
                _delay_us(1);
                COL_PORT |= (1 << COL_SRCLK);
                _delay_us(1);
            }

            row_clk_up;
            COL_PORT |= (1 << COL_RCLK);
            _delay_us(20);
        }
    }
}
