.module DELAY

.globl _dly_100us

.area CODE

;---------------------------------------------------------------------------;
; Delay 100 microseconds
;
; void dly_100us (void);

; 5 clocks per loop
; 16 clocks per us (FCPU = 16MHz)
; 1600/5 loops needed

_dly_100us:
	ldw	x, #0x0140
delayLoop$: 
	decw	x 			; 1
	tnzw	x 			; 2
	jrne	delayLoop$	; 2
	ret

