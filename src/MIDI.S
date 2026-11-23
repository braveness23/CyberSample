MIDI_CSR	EQU	$fffc04
MIDI_DATA	EQU	$fffc06
SIZE		EQU	4
	CSECT	name
	XDEF _midithru
	XDEF _midifilter
	XDEF _miditerm
	XREF _midithru_flag
	XREF _midibuf
	XREF _midivect
	XREF _base
	XREF _oldterm

* MIDI thru. Installed as KBDVBASE midivec, so it runs inside the MIDI interrupt with the
* received byte in d0. It writes the byte straight to the MIDI ACIA: calling the BIOS
* (Bconout) from interrupt context is not reentrant and crashed the system under steady
* input, e.g. a sequencer's clock while a dialog was open (issue #24).
_midithru
	tst.w	_midithru_flag
	beq.s	thru_done
	move.l	d1,-(sp)
	move.w	#2000,d1		bounded wait for the transmitter (a byte takes 320 us)
thru_wait
	btst	#1,MIDI_CSR		transmit data register empty?
	bne.s	thru_send
	dbra	d1,thru_wait
	bra.s	thru_restore		transmitter stuck: drop the byte, never hang the interrupt
thru_send
	move.b	d0,MIDI_DATA
thru_restore
	move.l	(sp)+,d1
thru_done
	rts

* Get/Send filter. MIDI real-time bytes (F8-FF: clock, active sensing...) may arrive anywhere,
* even inside a sysex dump; the SDS code reads packets at fixed offsets in the BIOS MIDI
* buffer, so they are dropped here (issue #33). Every other byte goes on to the original
* system handler (the address left on the stack is "returned" to).
_midifilter
	cmp.b	#$f8,d0
	bcc.s	filt_drop		unsigned d0 >= F8: real-time byte, drop it
	move.l	_midivect,-(sp)
filt_drop
	rts

* etv_term handler: put the system MIDI vector back if the program ends any other way than
* through its normal exit path (issue #24), then chain to the previous handler.
_miditerm
	move.l	a0,-(sp)
	move.l	_base,a0
	move.l	_midivect,(a0)		KBDVBASE.midivec is the first field
	move.l	(sp)+,a0
	move.l	_oldterm,-(sp)
	rts

	END
