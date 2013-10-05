/* 
    Copyright 2012 Alistair Buxton <a.j.buxton@gmail.com>

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

#include <avr/io.h>
#include "globals.h"

.macro delay n
.if \n
	nop
	delay (\n-1)
.endif
.endm

.macro serial_send
	nop
	sts	UDR0, r16
	nop
.endm

/* Vertical sync interrupt.
 * Loads with zero then decrements to get 255. */

.global INT0_vect
INT0_vect:
vsync:
	push	r15
	in      r15, _SFR_IO_ADDR(SREG)
	push 	r16
	clr	r16
	dec	r16
	sts	line_counter, r16
	pop 	r16
	out     _SFR_IO_ADDR(SREG), r15
	pop	r15
	reti

/* End of vertical sync interrupt. */




/* Horizontal sync interrupt. */

.global INT1_vect
INT1_vect:
hsync:
	push	r15
	in      r15, _SFR_IO_ADDR(SREG)
	push	r16
	push	r17

	lds	r16, line_counter		
	andi	r16, 0xf0		; If line_counter = 0 to 15
	breq	do_line			; output a line.
	rjmp	skip_line		

do_line:
	delay	45	; We can't start outputting the line too early,
			; but the following code is optimized to start
			; outputting as soon as possible...

	push 	ZL			; Save registers that are
	push	ZH			; used only when outputting.

	ldi	r16,0x18		; Enable TX
	sts	UCSR0B,r16		;

	ldi	r16, 0x55		; Output clock run-in.
	serial_send
	serial_send

	lds	r16, buffer_head	; Check buffer status.
	lds	r17, buffer_tail	; 
	cp	r16, r17		;
	brne	load_buffer_addr

load_fill_buffer_addr:
	ldi	ZL, lo8(fill_buffer)	; Load pointer to desired
	ldi	ZH, hi8(fill_buffer)	; buffer to Z.
	delay	8
	rjmp	got_buffer

load_buffer_addr:
	ldi	ZL, lo8(tt_buffer)	; Load address of buffers
	ldi	ZH, hi8(tt_buffer)	; pointer array to X.
	push 	r0
	ldi	r16, 42
	mul	r16, r17
	add	ZL, r0			; Add to Z.
	adc	ZH, r1			; Carry.
	clr	r17
	mov 	r1, r17
	pop	r0

got_buffer:
	ldi	r16, 0x27			; Output framing code.
	serial_send

	lds	r16, buffer_head	; Check buffer status.
	lds	r17, buffer_tail	; Check buffer status.
	cp	r16, r17	; 1	; r17 is normal or fill buffer?
	brne	normal_out	; 1,2	; It's normal.
	rjmp	fill_out	; 2	; It's a fill buffer.



normal_out:
/* This is the main part of the loop that outputs each byte of the packet. */
	ldi	r17, 42		; 1		; output byte down count.
loop_n:
	nop			; 1
	ld	r16, Z+		; 2		; Load byte from Z and inc.
	serial_send		; 4

	delay	6		; 6		; Loop must take 16 cycles.

	dec	r17		; 1
	brne	loop_n		; 1 or 2
	rjmp	done_buffer	; 2
/* End of loop. */


fill_out:
/* This is the main part of the loop that outputs each byte of the packet. */
	ldi	r17, 42		; 1		; output byte down count.
loop_f:
	lpm	r16, Z+		; 3		; Load byte from Z and inc.
	serial_send		; 4
	
	delay	6		; 6		; Loop must take 16 cycles.

	dec	r17		; 1
	brne	loop_f		; 1 or 2
/* End of loop. */


done_buffer:
	clr	r16			; Disable TX again
	sts	UCSR0B,r16		;

	lds	r16, buffer_head	; Check buffer pointers again.
	lds	r17, buffer_tail	; If head == tail do not increment.
	cp	r16, r17
	breq	no_buffer_inc
	inc	r17
	ldi	r16, NBUFFERS		; If r17 == fill buffer then it
	cp	r16, r17		; has overflowed. buffer_tail
	brne	store_buffer		; should then bet set back to zero.
	clr	r17			; 
store_buffer:
	sts	buffer_tail, r17

no_buffer_inc:
	pop 	ZH			; Restore registers that are
	pop	ZL			; used only when outputting.

skip_line:
	lds 	r16, line_counter	; Prevent the line_counter
	cpi 	r16, 0x40		; from oveflowing.
	breq	skip_inc		; Only increment it if
	inc 	r16			; less than 0x40.
	sts	line_counter, r16

skip_inc:
	pop	r17			; Restore registers.
	pop 	r16
	out     _SFR_IO_ADDR(SREG), r15	; Restore state.
	pop	r15
	reti

/* End of horizontal sync interrupt. */
