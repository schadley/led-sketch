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

int main() {
    uint8_t i;

    // Initialize registers
    COL_DDR |= (1 << COL_SER);
    COL_DDR |= (1 << COL_RCLK);
    COL_DDR |= (1 << COL_SRCLK);
    COL_DDR |= (1 << COL_SRCLR);

    // Clear the shift register
    // Clear input is active low
    COL_PORT &= ~(1 << COL_SRCLR);
    col_clk_dn;
    _delay_us(10);
    col_clk_up;
    _delay_us(10);
    COL_PORT |= (1 << COL_SRCLR);

    while (1) {
        // Shift in 1 for 8 cycles
        COL_PORT |= (1 << COL_SER);
        for (i = 0; i < 8; ++i) {
            col_clk_dn;
            _delay_ms(500);
            col_clk_up;
            _delay_ms(500);
        }

        // Shift in 0 for 8 cycles
        COL_PORT &= ~(1 << COL_SER);
        for (i = 0; i < 8; ++i) {
            col_clk_dn;
            _delay_ms(500);
            col_clk_up;
            _delay_ms(500);
        }
    }
}
