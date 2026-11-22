#include <gemlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <osbind.h>
#include <dos.h>
#include "defs.h"

/*********************************************************************
* Entry point for midi DownLoading
*********************************************************************/

short test_checksum = TRUE;
short freeze_mouse = TRUE;

void hndl_get( SD_HEADER *samp )
{
	STND_HEADER *h = (STND_HEADER *)(midibuf->ibuf + 1 );
	long count = 0;

	deselect_tree( GET );

	sprintf( TED_PTEXT(GET,SAMPLE_NO), "%ld", sample );
	sprintf( TED_PTEXT(GET,DEVICE_NO), "%ld", device );

	objc_iselect( GET, CHECKSUM, test_checksum );
	objc_iselect( GET, FREEZE_MOUSE, freeze_mouse );

	if( hndl_dial( GET, SAMPLE_NO ) == GET_CANCEL ) return;

	sample = atoi( TED_PTEXT(GET,SAMPLE_NO) );
	if( sample > 127 || sample < 0 ) {													// Check for offset errors
		form_alert( 1, "[0][Invalid Sample No.][ Cancel ]" );
		return;
	}
	device = atoi( TED_PTEXT(GET,DEVICE_NO) );
	if( device > 15 || device < 0 ) {													// Check for offset errors
		form_alert( 1, "[0][Invalid Device No.][ Cancel ]" );
		return;
	}

	test_checksum =  is_selected( GET, CHECKSUM );
	freeze_mouse  =  is_selected( GET, FREEZE_MOUSE );

	if( ! new_sample( samp, PROMPT ) ) return;			// dialog confirmed: now it's safe to clear the old sample

	FLUSH_MIDI_BUFFER;
	req_data( device, sample );
	Wait( device,0 );
	while( midibuf->ibuftl < sizeof(STND_HEADER) ) if( count++ > 300000 ) return;
	if(( h->sysex == 0xf0 ) && ( h->id_no == 0x7e ) && ( h->sub_id == 0x01 ) && ( h->eox == 0xf7 )) {
		if( ! extract_stnd_parameters( h, samp ) ) {
			form_alert( 1, "[1][Sample Header Not Valid...][Abort]" );
			new_sample( samp, NPROMPT );
		}
		else if( samp->FirstSamp = (short *) malloc( samp->SampLength * sizeof(short) ) ) {
 
			graf_mouse( HOURGLASS, 0L );
			if( freeze_mouse ) mouse_control( TRUE );
			switch( h->s_format ) {
				case 8: case 9: case 10: case 11: case 12: case 13: case 14:
					receive_stnd_data( samp, 60 );
					break;
				case 15: case 16: case 17: case 18: case 19: case 20: case 21:
					receive_stnd_data( samp, 40 );
					break;
				default :
					form_alert( 1, ALERT_NSUPPORT );
					new_sample( samp, NPROMPT );
					break;
			}
			if( freeze_mouse ) mouse_control( FALSE );
			graf_mouse( ARROW, 0L );
		}
		else form_alert( 1, ALERT_MEMORY );
	}
	else form_alert( 1, "[1][Sample Header Not Valid...][Abort]" );
	Cancel( device, 0 );
}

short
receive_stnd_data( SD_HEADER *samp, register short spp )
{
	register STND_PACKET *p = (STND_PACKET *)( midibuf->ibuf + 1 );
	register short	*S_data = samp->FirstSamp;
	register char x;
	register char check;
	register char *cp;
	register long packets = ( samp->SampLength + spp - 1 ) / spp; 
	register long packet_no;
	long count;
	short temp;
	GRECT r;

	form_center( STATUS, ELTX( &r ) );
	STATUS[STAT_MARKER].ob_width = 0;
 	objc_draw( STATUS, ROOT, MAX_DEPTH, ELTS( r ) );

	FLUSH_MIDI_BUFFER;
	ACK( device, 0 );

	for( packet_no = 0; packet_no < packets; ) {

		while( Bconstat(2) ) 
			if( (char)Bconin(2) == 0x1b )return FALSE;

		count = 0;
		while( midibuf->ibuftl < sizeof(STND_PACKET) ) if( count++ > 30000 ) return FALSE;

		if( test_checksum ) {
			cp = (char *)p;
			for( x = 1, check = 0; x < 125; x++ ) check ^= cp[x];
		}

		if( check == p->checksum || ! test_checksum ) {
			switch( spp ) {
				case 60 :
					cp = p->data;
					for( x = 0; x < spp; x++, cp += 2 )
						*S_data++ = ( cp[0] << 9 ) + ( cp[1] << 2 ) - 0x8000;
					break;
				case 40 :
					cp = p->data;
					for( x = 0; x < spp; x++, cp += 3 )
						*S_data++ = ( cp[0] << 9 ) + ( cp[1] << 2 ) + ( cp[2] >> 5 ) - 0x8000;
					break;
			}

			temp = STATUS[STAT_MARKER].ob_width;
			STATUS[STAT_MARKER].ob_width = ( STATUS[STAT_BAR].ob_width * ( packet_no + 1 ) ) / packets;
			if( temp != STATUS[STAT_MARKER].ob_width )
				objc_draw( STATUS, STAT_MARKER, MAX_DEPTH, ELTS( r ) );

			FLUSH_MIDI_BUFFER;
			ACK( device, (char)packet_no & 0x7f );
			packet_no++;
		}
		else {

			FLUSH_MIDI_BUFFER;
			NAK( device, (char)packet_no & 0x7f );
		}
	}

	force_redraw( g.hnd, STATUS, ROOT );

	return TRUE;
}

/******************************************************************
* Convert the STND_HEADER data into SD_HEADER parameters
*******************************************************************/

short extract_stnd_parameters( STND_HEADER *temp, SD_HEADER *samp )
{
	samp->SampPeriod	= 	( (long)temp->s_rate_h  	<< 14 	) + 
				  			( (long)temp->s_rate_m  	<<  7	) +
				  			( (long)temp->s_rate_l 				);
	if( samp->SampPeriod < 1 ) return FALSE;		// would divide by zero
	samp->SampRate		=	1000000000 / samp->SampPeriod;
	samp->SampLength 	=	( (long)temp->s_length_h 	<< 14	) +
				  			( (long)temp->s_length_m 	<<  7	) +
				  			( (long)temp->s_length_l 			);
	samp->Loop1Start	= 	( (long)temp->l_start_h 	<< 14	) +
				  			( (long)temp->l_start_m 	<<  7	) + 
				  			( (long)temp->l_start_l 			);
	samp->Loop1End		= 	( (long)temp->l_end_h 		<< 14 	) +
				  			( (long)temp->l_end_m 		<<  7 	) +
				  			( (long)temp->l_end_l 				);

	switch( temp->l_mode ) {
		case 0x00 :					// Forward Loop
			samp->Loop1Type = 1;
			break;
		case 0x01 :					// Backward/Forward Loop
			samp->Loop1Type = 2;
			break;
		case 0x7f :					// Loop Off or Other
		default :
			samp->Loop1Type = 0;
			break;
	}

	return samp->SampLength > 0;						// an empty dump is not usable either
}

void req_data( char d, char s )
{
	req_string[2] = d;
	req_string[4] = s;
	Midiws( 6, req_string );
}

void Wait( char d, char p ) 
{
	msg_string[2] = d;
	msg_string[3] = 0x7c;
	msg_string[4] = p;
	Midiws( 5, msg_string );
}

void ACK( char d, char p ) 
{
	msg_string[2] = d;
	msg_string[3] = 0x7f;
	msg_string[4] = p;
	Midiws( 5, msg_string );
}

void Cancel( char d, char p ) 
{
	msg_string[2] = d;
	msg_string[3] = 0x7d;
	msg_string[4] = p;
	Midiws( 5, msg_string );
}

void NAK( char d, char p ) 
{
	msg_string[2] = d;
	msg_string[3] = 0x7e;
	msg_string[4] = p;
	Midiws( 5, msg_string );
}

short mouse_control( short flag )
{
	KBDVBASE *base;
	static void *mousevec;

	if( flag ) { 
		base = (KBDVBASE *)Kbdvbase();
		mousevec = (void *)base->mousevec;
		Initmous( 0, NULL, (void *)NULL );
	}
	else {
		Initmous( 1, &param, mousevec );
	}
	return TRUE;
}

void
hndl_send( SD_HEADER *samp )
{
	static short trunc_to_loop = TRUE;
	static short format = SEND_12;
	STND_HEADER h;
	short bits;

	deselect_tree( SEND );

	sprintf( TED_PTEXT(SEND,SEND_SAMPLE_NO), "%ld", sample );
	sprintf( TED_PTEXT(SEND,SEND_DEVICE_NO), "%ld", device );

	objc_iselect( SEND, TRUNC_TO_LOOP, trunc_to_loop );
	objc_iselect( SEND, format, TRUE );
	
	if( hndl_dial( SEND, SEND_SAMPLE_NO ) == SEND_CANCEL ) return;

	trunc_to_loop = is_selected( SEND, TRUNC_TO_LOOP );
	format = find_selected_radio( SEND, SEND_FORMAT_PRNT );
	
	sample = atoi( TED_PTEXT(SEND,SEND_SAMPLE_NO) );
	if( sample > 127 || sample < 0 ) {													// Check for offset errors
		form_alert( 1, "[0][Invalid Sample No.][ Cancel ]" );
		return;
	}
	device = atoi( TED_PTEXT(SEND,SEND_DEVICE_NO) );
	if( device > 15 || device < 0 ) {													// Check for offset errors
		form_alert( 1, "[0][Invalid Device No.][ Cancel ]" );
		return;
	}

	bits = ( format == SEND_16 ) ? 16 : 12;
	pack_header( &h, samp, bits );
	FLUSH_MIDI_BUFFER;
	Midiws( 20, (char *)&h );
	transmit_stnd_data( samp, trunc_to_loop, bits );
}

short 
transmit_stnd_data( SD_HEADER *samp, short truncate, short bits )
{
	STND_PACKET p;
	long packets;
	long num;
	long packet_no;
	short byte, word, x;
	short spp = ( bits == 16 ) ? 40 : 60;		// words per packet: 3 bytes each for 16 bit, 2 for 12 bit
	char *cp = (char *)&p;
	unsigned short temp;
	long count;
	short handshake_flag = TRUE;
	short temp2;
	GRECT r;
	

	if( truncate && samp->Loop1End <= 0 ) truncate = FALSE;	// no loop set: send the whole sample
	num = truncate ? samp->Loop1End : samp->SampLength;
	packets = ( num + spp - 1 ) / spp;

	STATUS[STAT_MARKER].ob_width = 0;
	form_center( STATUS, ELTX( &r ) );
	objc_draw( STATUS, ROOT, MAX_DEPTH, ELTS( r ) );

	p.sysex = 0xf0;
	p.id_no = 0x7e;
	p.channel = device;
	p.sub_id = 0x02;
	p.eox = 0xf7;

	count = 0;
	while( midibuf->ibuftl < 6 ) 
		if( count++ > 300000 ) {
			handshake_flag = FALSE;
			break;
		}

	for( packet_no = 0; packet_no < packets; packet_no++ ) {


		while( Bconstat(2) ) 
			if( (char)Bconin(2) == 0x1b ) return FALSE;

		if( handshake_flag ) {
			count = 0;
			while( midibuf->ibuftl < 6 ) 
				if( count++ > 300000 ) {
					handshake_flag = FALSE;
					break;
				}
		}
		else for( count = 0; count < 1000; count++ );

		p.packet_no = (char)( packet_no & 0x7f );
		for( byte = 0, word = 0; word < spp; word++ ) {
			temp = num-- > 0 ? *( samp->FirstSamp + ( packet_no * spp ) + word ) + 0x8000 : 0L;
			p.data[byte++] = 	(char)( temp >> 9 ) & 0x7f; 
			p.data[byte++] =	(char)( temp >> 2 ) & 0x7f; 
			if( bits == 16 )								// 16 bit: the last 2 bits, left-justified, in a third byte
				p.data[byte++] = (char)( temp << 5 ) & 0x7f;
		}
		for( x = 1, p.checksum = 0; x < 125; x++ ) p.checksum ^= cp[x];
		Midiws( 126, (char *)&p );
		FLUSH_MIDI_BUFFER;

		temp2 = STATUS[STAT_MARKER].ob_width;
		STATUS[STAT_MARKER].ob_width = ( STATUS[STAT_BAR].ob_width * ( packet_no + 1 ) ) / packets;
		if( temp2 != STATUS[STAT_MARKER].ob_width )
			objc_draw( STATUS, STAT_MARKER, MAX_DEPTH, ELTS( r ) );
		}
	return TRUE;
}

void
pack_header( STND_HEADER *h, SD_HEADER *samp, short bits )
{
	memcpy( h, &default_stnd_header, sizeof( STND_HEADER ) );
	h->channel = device;
	h->s_no_l = sample;
	h->s_format = bits;
	if( samp->SampRate < 1 ) samp->SampRate = 44100;		// avoid dividing by zero below
	samp->SampPeriod = 1000000000 / samp->SampRate;
	h->s_rate_l = 		(char)( ( samp->SampPeriod 		 )	& 0x7f );
	h->s_rate_m = 		(char)( ( samp->SampPeriod >> 7  ) 	& 0x7f );
	h->s_rate_h = 		(char)( ( samp->SampPeriod >> 14 ) 	& 0x7f );
	h->s_length_l =		(char)( ( samp->SampLength 		 )	& 0x7f );
	h->s_length_m =		(char)( ( samp->SampLength >> 7  ) 	& 0x7f );
	h->s_length_h =		(char)( ( samp->SampLength >> 14 ) 	& 0x7f );

	if( ( samp->Loop1Start >= 0 ) && ( samp->Loop1End >= 0 ) ) {
		h->l_start_l = 	(char)(   samp->Loop1Start 			& 0x7f );
		h->l_start_m = 	(char)( ( samp->Loop1Start >> 7  ) 	& 0x7f );
		h->l_start_h = 	(char)( ( samp->Loop1Start >> 14 ) 	& 0x7f );
		h->l_end_l = 	(char)(   samp->Loop1End 			& 0x7f );
		h->l_end_m = 	(char)( ( samp->Loop1End   >> 7  ) 	& 0x7f );
		h->l_end_h = 	(char)( ( samp->Loop1End   >> 14 ) 	& 0x7f );
	}
	else {
		h->l_start_l = 	0x01;
		h->l_start_m = 	0x00;
		h->l_start_h = 	0x00;
		h->l_end_l = 	(char)(   samp->SampLength 			& 0x7f );
		h->l_end_m = 	(char)( ( samp->SampLength >> 7  ) 	& 0x7f );
		h->l_end_h = 	(char)( ( samp->SampLength >> 14 ) 	& 0x7f );
	}
	
	switch( samp->Loop1Type ) {
		case 1 :					// Forward Loop
			h->l_mode = 0x00;
			break;
		case 2 :					// Backward/Forward Loop
			h->l_mode = 0x01;
			break;
		case 0 :					// Loop Off or Other
		default :
			h->l_mode = 0x7f;
			break;
	}
}
	

