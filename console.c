/*
    Copyright 2010 Alistair Buxton <a.j.buxton@gmail.com>

    This file is part of avr-teletext.

    avr-teletext is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    avr-teletext is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with avr-teletext.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <inttypes.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#include "globals.h"
#include "utils.h"

extern volatile uint8_t tt_buffer[NBUFFERS][42];
extern volatile uint8_t buffer_head;
extern volatile uint8_t buffer_tail;

uint8_t first_row = 0;

#define NROWS (NBUFFERS-1)
#define row_buffer(r) tt_buffer[(r)+1]

void move_rows() {

    uint8_t i;
    for(i=0;i<NROWS;i++) {
        fill_mrag(1, 3+((i+NROWS-first_row)%NROWS), row_buffer(i));
    }

    return;
}

void test() {

    uint8_t i,j;

    for(i=0;i<NROWS;i++) {
        row_buffer(i)[2] = parity(hex(i>>4));
        row_buffer(i)[3] = parity(hex(i&0xf));
        row_buffer(i)[4] = ' ';
        for(j=5;j<42;j++)
            row_buffer(i)[j] = parity('A'-5+j);
    }

}


uint8_t column;
uint8_t row;

void console_clear_buffer(uint8_t buffer) {
    uint8_t j;
    for(j=2;j<42;j++)
        row_buffer(row)[j] = ' '; // NOTE: parity(' ') == ' '

}

void console_clear() {

    uint8_t i,j;

    for(i=1;i<NBUFFERS;i++)
        for(j=2;j<42;j++)
            tt_buffer[i][j] = ' '; // NOTE: parity(' ') == ' '

    first_row = 0;
    column = 0;
    row = 0;
    move_rows();
    //test();

}

void console_setup() {

    /* In console mode the buffer is used like a text console.
       Scrolling is implemented by changing the line numbers
       in each packet. The whole buffer is sent repeatedly. */

    tt_buffer[0][0] = 0x2;  tt_buffer[0][1] = 0x15;
    tt_buffer[0][2] = 0x15; tt_buffer[0][3] = 0x15;
    tt_buffer[0][4] = 0x15; tt_buffer[0][5] = 0x15;
    tt_buffer[0][6] = 0x15; tt_buffer[0][7] = 0x15;
    tt_buffer[0][8] = 0x15; tt_buffer[0][9] = 0x15;
    tt_buffer[0][10] = parity('A');
    tt_buffer[0][11] = parity('V');
    tt_buffer[0][12] = parity('R');
    tt_buffer[0][13] = parity(' ');
    tt_buffer[0][14] = parity('T');
    tt_buffer[0][15] = parity('e');
    tt_buffer[0][16] = parity('x');
    tt_buffer[0][17] = parity('t');
    tt_buffer[0][18] = 0x20; tt_buffer[0][19] = 0x31;
    tt_buffer[0][20] = 0xb0; tt_buffer[0][21] = 0xb0;
    tt_buffer[0][22] = 0x20; tt_buffer[0][23] = 0x54;
    tt_buffer[0][24] = 0x75; tt_buffer[0][25] = 0xe5;
    tt_buffer[0][26] = 0x20; tt_buffer[0][27] = 0x31;
    tt_buffer[0][28] = 0x37; tt_buffer[0][29] = 0x20;
    tt_buffer[0][30] = 0x4a; tt_buffer[0][31] = 0x61;
    tt_buffer[0][32] = 0x6e; tt_buffer[0][33] = 0x83;
    tt_buffer[0][34] = 0x32; tt_buffer[0][35] = 0x31;
    tt_buffer[0][36] = 0xba; tt_buffer[0][37] = 0xb5;
    tt_buffer[0][38] = 0xb0; tt_buffer[0][39] = 0x2f;
    tt_buffer[0][40] = 0xb3; tt_buffer[0][41] = 0xb6;

    console_clear();
    buffer_head = NBUFFERS; // Will never be reached.
}

void console_putchar(char c) {
    switch(c) {
        case '\n':
            column = 0;
            row++;
            break;
        default:
            row_buffer(row)[2+column] = parity(c);
            column++;
            break;
    }
    if(column >= 40) {
        column = 0;
        row++;
    }

    if((row > 0) && ((row%NROWS) == first_row)) {
        row %= NROWS;
        first_row++;
        first_row %= NROWS;
        console_clear_buffer(row);
        move_rows();
    }
}

void console_puthex(char c) {
    console_putchar(hex((c>>4)&0xf));
    console_putchar(hex(c&0xf));
}
