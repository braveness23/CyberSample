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

	if( edit_length(samp) == sample_length(samp) ) {
		form_alert( 1, ALERT_CUT_ALL );
		return;
	}

	new_sample( u, NPROMPT );
	new_sample( p, NPROMPT );
	*u = *samp;
	samp->SampLength = sample_length(u) - edit_length(u);
	p->SampLength = edit_length(u);
	if( ( samp->FirstSamp = (short *)malloc( SIZ(samp->SampLength) ) ) &&
		( p->FirstSamp = (short *)malloc( SIZ(edit_length(u) ) ) ) ) 
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
		if( seglen[1] ) 
			memcpy( off_addr( p, 0L ), segaddr[1], SIZ(seglen[1]) );
		reset_filename(p);
		reset_view_params(p);
		reset_edit_params(p);
		reset_loop1_params(p);
		reset_loop2_params(p);
		reset_view_params(samp);
		reset_edit_params(samp);
		reset_loop1_params(samp);
		reset_loop2_params(samp);
	}				
	else {
		form_alert( 1, ALERT_MEMORY );
		*samp = *u;
		*u = default_sd_header;
		new_sample( p, NPROMPT );
	}

	force_complete_redraw();
}

void
hndl_copy( SD_HEADER *samp )
{
	short *segaddr[1];
	long seglen[1];

	if( !edit_flag ) {
		form_alert( 1, ALERT_NOEDIT );
		return;
	}
	new_sample( p, NPROMPT );
	*p = *samp;

	segaddr[0] = off_addr( samp, samp->EditStart );
	seglen[0] = edit_length(samp);

	if( p->FirstSamp = (short *)malloc( SIZ(seglen[0]) ) ) {
		memcpy( p->FirstSamp, segaddr[0], SIZ(seglen[0])  );
		p->SampLength = seglen[0];
		reset_filename(p);
		reset_view_params(p);
		reset_edit_params(p);
		reset_loop1_params(p);
		reset_loop2_params(p);
	}
	else {
		new_sample( p, NPROMPT );
		form_alert( 1, ALERT_MEMORY );
	}
}

void
hndl_paste( SD_HEADER *samp )
{
	static short paste_mode = PASTE_INSERT;
	static short times = 1;
	long location = samp->EditStart;

	deselect_tree( PASTE );

	sprintf( TED_PTEXT( PASTE, PASTE_BUF_LENGTH ), "%d", sample_length(p) );
	sprintf( TED_PTEXT( PASTE, PASTE_LOCATION ), "%d", location );
	sprintf( TED_PTEXT( PASTE, PASTE_TIMES ), "%d", times );

	objc_iselect( PASTE, paste_mode, TRUE );

	if( hndl_dial( PASTE, PASTE_TIMES ) == PASTE_CANCEL ) return;

	paste_mode = find_selected_radio( PASTE, PASTE_OPT_PRNT );
	times = atoi( TED_PTEXT( PASTE, PASTE_TIMES ) );

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
				seglen[0]  = u->EditStart;
				segaddr[1] = off_addr( u, u->EditStart );
				seglen[1]  = u->SampLength - u->EditStart;
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
					min( SIZ( sample_length(p) ), SIZ(( s->SampLength - s->EditStart )) ) );
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

