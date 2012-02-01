#include <inttypes.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#include "globals.h"
#include "utils.h"

extern volatile uint8_t tt_buffer[NBUFFERS][42];
extern volatile uint8_t buffer_head;
extern volatile uint8_t buffer_tail;

uint8_t column;

void passthrough_setup() {
    buffer_head = 0;
    buffer_tail = 0;
    column = 0;
}

void passthrough_start() {
    column = 0;
}

void passthrough_putchar(uint8_t c) {
    tt_buffer[buffer_head][column] = c;
    column++;
    if(column == 42) {
        uint8_t tmp = (buffer_head+1)%NBUFFERS;
        column = 0;
        while(tmp == buffer_tail) { ; }
        buffer_head = tmp;
    }
}
