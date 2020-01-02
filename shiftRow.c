#include <avr/io.h>
#include <util/delay.h>

// Pins for interfacing with a TPIC6B596 shift register
// Used to select which rows in a LED array should be lit
#define ROW_SER     PC3
#define ROW_RCLK    PC2
#define ROW_SRCLK   PC1
#define ROW_SRCLR   PC0
#define ROW_DDR     DDRC
#define ROW_PORT    PORTC
#define row_clk_up  (ROW_PORT |= ((1 << ROW_RCLK) | (1 << ROW_SRCLK)))
#define row_clk_dn  (ROW_PORT &= ~((1 << ROW_RCLK) | (1 << ROW_SRCLK)))

int main() {
    uint8_t i;

    // Initialize registers
    ROW_DDR |= (1 << ROW_SER);
    ROW_DDR |= (1 << ROW_RCLK);
    ROW_DDR |= (1 << ROW_SRCLK);
    ROW_DDR |= (1 << ROW_SRCLR);

    // Clear the shift register
    // Clear input is active low
    ROW_PORT &= ~(1 << ROW_SRCLR);
    row_clk_dn;
    _delay_us(10);
    row_clk_up;
    _delay_us(10);
    ROW_PORT |= (1 << ROW_SRCLR);

    while (1) {
        // Shift in 1 for 1 cycle
        ROW_PORT |= (1 << ROW_SER);
        row_clk_dn;
        _delay_ms(500);
        row_clk_up;
        _delay_ms(500);

        // Shift in 0 for 7 cycles
        ROW_PORT &= ~(1 << ROW_SER);
        for (i = 0; i < 7; ++i) {
            row_clk_dn;
            _delay_ms(500);
            row_clk_up;
            _delay_ms(500);
        }
    }
}
