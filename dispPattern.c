#include <avr/io.h>
#include <util/delay.h>

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
}

void clearShiftRegisters() {
    // Clear the shift registers
    // Clear inputs are active low
    COL_PORT &= ~(1 << COL_SRCLR);
    ROW_PORT &= ~(1 << ROW_SRCLR);
    col_clk_dn;
    row_clk_dn;
    _delay_us(1);
    col_clk_up;
    row_clk_up;
    _delay_us(1);
    COL_PORT |= (1 << COL_SRCLR);
    ROW_PORT |= (1 << ROW_SRCLR);
}

int main() {
    uint8_t i, j;
    // Array of patterns for each row
    uint8_t rows[8] = {0xc6,0x6c,0x54,0x44,0x44,0x44,0x44,0xc6};

    initRegisters();
    clearShiftRegisters();

    // Shift in a 1 to avoid a wasted cycle -- see below
    row_clk_dn;
    ROW_PORT |= (1 << ROW_SER);
    _delay_us(1);
    row_clk_up;
    _delay_us(1);

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
