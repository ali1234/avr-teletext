#include <inttypes.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "globals.h"
#include "console.h"
#include "utils.h"

#define WITH_LM1881

#define nop() __asm__ __volatile__("nop\nnop\nnop\nnop");

void io_setup(void)
{

// +---+-----+-----+-----+-----+-----+-----+-----+-----+
// |   |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |
// +---+-----+-----+-----+-----+-----+-----+-----+-----+
// | B | OUT | OUT | OUT | OUT | IN  | IN  |  Crystal  |
// +---+-----+-----+-----+-----+-----+-----+-----+-----+
// | C | OUT | OUT | OUT | OUT | SDA | SCL | RST |  -  |
// +---+-----+-----+-----+-----+-----+-----+-----+-----+
// | D | RXD | TXD |INT0 |INT1 | T0  | T1  | OUT | OUT |
// +---+-----+-----+-----+-----+-----+-----+-----+-----+

    // D[7] = shifter enable
    DDRD = 0x12; PORTD = 0;

    // SPI setup
    // baud Fosc/2
    UBRR0H = 0;
    UBRR0L = 0;
    // USART0 MSPIM mode, LSB
    UCSR0C = 0xc4;
    // enable Tx
    UCSR0B = 0x18;

    // TWI setup
    // no need to set baud rate for slave mode
    // enable pull-ups
    DDRC = 0x00; PORTC = 0x30;
    // match addresses 0x40 - 0x7f
    TWAR = 0x40<<1;
    TWAMR = 0x1f<<1;
    // enable TWI, no interrupt request
    TWCR = _BV(TWEN) | _BV(TWEA);

    // external INT0 and INT1 enable
    EICRA = 0x0b;
    EIMSK = 0x03;

    sei();

}

volatile uint8_t line_counter;
volatile uint8_t tt_buffer[NBUFFERS][42];
volatile uint8_t buffer_head = 0;
volatile uint8_t buffer_tail = 0;

/* Buffer that we send when we have nothing else to do. */
const uint8_t fill_buffer[42] PROGMEM = {
    0x49, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15,
    0x15, 0x15, 
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
    0x20, 0x31, 0xb0, 0xb0, 0x20, 0x54, 0x75, 0xe5, 
    0x20, 0x31, 0x37, 0x20, 0x4a, 0x61, 0x6e, 0x83, 
    0x32, 0x31, 0xba, 0xb5, 0xb0, 0x2f, 0xb3, 0xb6
};

/* hmm */

#define BLACK "\x80"
#define RED "\x01"
#define GREEN "\x02"
#define YELLOW "\x83"
#define BLUE "\x04"
#define MAGENTA "\x85"
#define CYAN "\x86"
#define WHITE "\x07"

#define FLASHON "\x08"
#define FLASHOFF "\x09"
#define ENDBOX "\x0A"
#define STARTBOX "\x0B"
#define NORMAL "\x0C"
#define DOUBLEH "\x0D"
#define DOUBLEW "\x0E"
#define DOUBLEWH "\x0F"

/*
 0123456789012345678901234567890123456789
*/
const char long_text[] PROGMEM = 
"       "DOUBLEWH"H e l l o   w o r l d !\n"
"\n"
"\n"
"           "RED"* "GREEN"* "YELLOW"* "BLUE"* "MAGENTA"* "CYAN"*\n\n"
"  The"CYAN"AVR"WHITE"is a modified Harvard\n"
"architecture 8-bit RISC single chip\n"
"microcontroller which was developed by\n"
"Atmel in"GREEN"1996."WHITE"The AVR was one of the\n"
"first microcontroller families to use\n"
"on-chip flash memory for program\n"
"storage, as opposed to one-time\n"
"programmable ROM, EPROM, or EEPROM used\n"
"by other microcontrollers at the time.\n"
"\n"
"           "RED"* "GREEN"* "YELLOW"* "BLUE"* "MAGENTA"* "CYAN"*\n\n"
" "YELLOW"Teletext"WHITE"(or \"broadcast teletext\") is\n"
"a television information retrieval\n"
"service developed in the United Kingdom\n"
"in the early"GREEN"1970s."WHITE"It offers a range\n"
"of text-based information, typically\n"
"including national, international and\n"
"sporting news, weather and television\n"
"schedules. Subtitle information is also\n"
"transmitted in the teletext signal,\n"
"typically on page 888, 777 or 333."
;

extern uint8_t first_row;

void run_demo(void) {

    for(;;)
    {
        int text_pos;
        char c;
        text_pos = 0;
        //console_clear();
        while((c = pgm_read_byte(&(long_text[text_pos]))) != 0) {
            console_putchar(c);
            delay_ms(10);
            text_pos++;
        }
        //delay_s(1);
        console_putchar('\n');
        console_putchar('\n');
    }

}

int main(void)
{
    uint8_t dest_register = 0xff;
    uint8_t control = 0;
    uint8_t i = 0;

    io_setup();
    passthrough_setup();
    control=1;
    //run_demo();

    for(;;)
    {
        
        if(TWCR&0x80) {
            switch(TWSR&0xf8) {
                case 0x60:
                case 0x68:
                    dest_register = (TWDR>>1)&(TWAMR>>1);
                    if(control&0x01) passthrough_start();
                    break;
                case 0x80:
                    switch(dest_register) {
                        case 0:
                            if(control&0x01) passthrough_putchar(TWDR);
                            else console_putchar(TWDR&0x7f);
                            break;
                        case 1:
                            control = TWDR;
                            if(control&0x01) passthrough_setup();
                            else console_setup();
                            break;
                        default:
                            
                            break;
                    }
                    break;
                default:
                    dest_register = 0xff;
                    //error - bail out
                    break;
            }
            // clear interrupt
            TWCR |= 0x80;
        }
        //delay_ms(1);
    }
    
    // Never reached.
    return(0);
}
