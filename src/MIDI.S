MIDI_CSR	EQU	0xfffc04
MIDI_DATA	EQU	0xfffc06
SIZE		EQU	4
	CSECT	name
	XDEF _midithru 
	XREF _midithru_flag
	XREF _midibuf

_midithru
	tst.w	_midithru_flag
	beq.w	return
	move.w	D0,-(sp)
	move.w	#3,-(sp)
	move.w	#3,-(sp)
	trap	#13
	addq.l	#6,sp
return
	rts	

	END

