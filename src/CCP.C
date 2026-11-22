/* CCP.C ( cut, copy, paste ) */

#include <gemlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "defs.h"

void
hndl_cut( SD_HEADER *samp )
{
	short *segaddr[3];
	long seglen[3];
	/* The paste buffer is also selectable as the current sample.  Cutting from it can't also
	   fill it, so there the cut just deletes the selection (Undo brings it back). */
	short to_clip = ( samp != p );

	if( edit_length(samp) == sample_length(samp) ) {
		form_alert( 1, ALERT_CUT_ALL );
		return;
	}

	new_sample( u, NPROMPT );
	if( to_clip ) new_sample( p, NPROMPT );
	*u = *samp;
	samp->SampLength = sample_length(u) - edit_length(u);
	if( to_clip ) p->SampLength = edit_length(u);
	if( ( samp->FirstSamp = (short *)malloc( SIZ(samp->SampLength) ) ) &&
		( !to_clip || ( p->FirstSamp = (short *)malloc( SIZ(edit_length(u) ) ) ) ) )
	{
		segaddr[0] = off_addr( u, 0L );
		seglen[0] = u->EditStart;
		segaddr[1] = off_addr( u, u->EditStart );
		seglen[1] = edit_length( u );
		segaddr[2] = off_addr( u, u->EditEnd );
		seglen[2] = u->SampLength - u->EditEnd;
		if( seglen[0] ) 
			memcpy( off_addr( samp, 0L ), segaddr[0], SIZ(seglen[0]) );							
		if( seglen[2] )
			memcpy( off_addr( samp, seglen[0] ), segaddr[2], SIZ(seglen[2]) );
		if( to_clip && seglen[1] ) 
			memcpy( off_addr( p, 0L ), segaddr[1], SIZ(seglen[1]) );
		if( to_clip ) {
			reset_filename(p);
			reset_view_params(p);
			reset_edit_params(p);
			reset_loop1_params(p);
			reset_loop2_params(p);
		}
		reset_view_params(samp);
		reset_edit_params(samp);
		reset_loop1_params(samp);
		reset_loop2_params(samp);
	}				
	else {
		form_alert( 1, ALERT_MEMORY );
		if( samp->FirstSamp ) free( samp->FirstSamp );		/* first allocation succeeded, second failed */
		*samp = *u;
		*u = default_sd_header;
		if( to_clip ) new_sample( p, NPROMPT );
	}

	force_complete_redraw();
}

void
hndl_copy( SD_HEADER *samp )
{
	SD_HEADER src;
	short *buf;
	long len;

	if( !edit_flag ) {
		form_alert( 1, ALERT_NOEDIT );
		return;
	}

	/* Take the selection out of the source *before* touching the paste buffer: when the
	   source is the paste buffer itself, new_sample( p ) would free the very data being copied. */
	len = edit_length( samp );
	if( !( buf = (short *)malloc( SIZ(len) ) ) ) {
		form_alert( 1, ALERT_MEMORY );						/* the old paste buffer is left alone */
		return;
	}
	memcpy( buf, off_addr( samp, samp->EditStart ), SIZ(len) );
	src = *samp;

	new_sample( p, NPROMPT );
	*p = src;
	p->FirstSamp = buf;
	p->SampLength = len;
	reset_filename(p);
	reset_view_params(p);
	reset_edit_params(p);
	reset_loop1_params(p);
	reset_loop2_params(p);
}

void
hndl_paste( SD_HEADER *samp )
{
	static short paste_mode = PASTE_INSERT;
	static short times = 1;
	long location = samp->EditStart;

	deselect_tree( PASTE );

	sprintf( TED_PTEXT( PASTE, PASTE_BUF_LENGTH ), "%ld", sample_length(p) );
	sprintf( TED_PTEXT( PASTE, PASTE_LOCATION ), "%ld", location );
	sprintf( TED_PTEXT( PASTE, PASTE_TIMES ), "%d", times );

	objc_iselect( PASTE, paste_mode, TRUE );

	if( hndl_dial( PASTE, PASTE_TIMES ) == PASTE_CANCEL ) return;

	paste_mode = find_selected_radio( PASTE, PASTE_OPT_PRNT );
	times = atoi( TED_PTEXT( PASTE, PASTE_TIMES ) );
	location = atol( TED_PTEXT( PASTE, PASTE_LOCATION ) );	// the dialog's Location field is honoured now
	if( location < 0 ) location = 0;
	if( location > samp->SampLength ) location = samp->SampLength;
	if( times < 1 ) times = 1;

	switch( paste_mode ) {
		case PASTE_INSERT :
						
			new_sample( u, NPROMPT );
			*u = *samp;
			samp->SampLength = sample_length(u) + ( sample_length(p) * times );
			if( samp->FirstSamp = (short *)malloc( SIZ( samp->SampLength ) ) ) {

				short *segaddr[3];
				long seglen[3];
				short x;

				segaddr[0] = off_addr( u, 0L );
				seglen[0]  = location;
				segaddr[1] = off_addr( u, location );
				seglen[1]  = u->SampLength - location;
							segaddr[2] = off_addr( p, 0L );
							seglen[2]  = p->SampLength;
							
				if( seglen[0] ) 
					memcpy( off_addr( samp, 0L ), segaddr[0], SIZ(seglen[0]) );							
				if( seglen[2] )
					for( x = 0; x < times; x++ )
						memcpy( off_addr( samp, seglen[0] + ( x * seglen[2] ) ), segaddr[2], SIZ(seglen[2]) );
				if( seglen[1] ) 
					memcpy( off_addr( samp, seglen[0] + ( times * seglen[2] ) ), segaddr[1], SIZ(seglen[1]) );
			}
			else {
				form_alert( 1, ALERT_MEMORY );
				*samp = *u;
				*u = default_sd_header;
			}
			break;
		case PASTE_REPLACE :
			new_sample( u, NPROMPT );
			*u = *samp;
			if( samp->FirstSamp = (short *)malloc( SIZ( samp->SampLength ) ) ) {
				memcpy( off_addr( samp, 0L ), off_addr( u, 0L ),
					SIZ( sample_length( u ) ) );
				memcpy( off_addr( samp, location ), off_addr( p, 0L ), 
					min( SIZ( sample_length(p) ), SIZ(( samp->SampLength - location )) ) );
			}
			else {
				form_alert( 1, ALERT_MEMORY );
				*samp = *u;
				*u = default_sd_header;
			}
			break;
		default :
			break;
	}	
	force_complete_redraw();
}


void
truncate( SD_HEADER *samp )
{
	size_t num = SIZ( edit_length( samp ) );
	short *temp_S_data;
	

	if( edit_length(samp) == sample_length(samp) ) {
		form_alert( 1, ALERT_TRUNC_ALL );
		return;
	}

	if( !undo_save( samp ) ) return;
	if( temp_S_data = (short *)malloc( num ) ) {

		graf_mouse( HOURGLASS, 0L );

		memcpy( temp_S_data, edit_address( samp ), num );
		free( samp->FirstSamp );

		samp->FirstSamp = temp_S_data;
		samp->SampLength = samp->EditEnd = edit_length( samp );
		reset_edit_params( samp );
		reset_view_params( samp );
		reset_loop1_params( samp );
		reset_loop2_params( samp );

		graf_mouse( ARROW, 0L );
		force_complete_redraw();
	}
	else 
		form_alert( 1, ALERT_MEMORY );
}

