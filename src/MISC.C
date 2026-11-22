/* MISC.C */

#include <gemlib.h>
#include <osbind.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "defs.h"

void setup_samples()
{
	short x; 

	for( x = FIRST_SMPL; x < NUM_SMPL; x++ ) init_sample_parameters( &s[ x ] );
	t = &s[ smp_no ];
	p = &s[ PASTE_BUF ];
	u = &s[ UNDO_BUF ];
}

void cleanup_samples()
{
	short x;

	for( x = FIRST_SMPL; x < NUM_SMPL; x++ ) new_sample( &s[ x ], NPROMPT );
}

short
new_sample( SD_HEADER *samp, short warn )
{
	short rspnc = 1;

	if( ( samp->SampLength > 0 ) || samp->FirstSamp )
		if( warn )
			rspnc = form_alert( 1, "[1][This will clear the current|sample from memory][OK|Cancel]" );
	if( rspnc == 1 ) {
		if( samp->FirstSamp ) free( samp->FirstSamp );
		init_sample_parameters( samp );
		return TRUE;
	}
	return FALSE;
}

void
init_sample_parameters( SD_HEADER *samp )
{
	*(samp) = default_sd_header;
}

long
samp_per_msec( SD_HEADER *s )
{

	return (long)( s->SampRate / 1000 );
}

long
samp_per_sec( SD_HEADER *s )
{

	return (long) ( s->SampRate );
}

long
msec_to_samples( SD_HEADER *s, long msec )
{

	return (long) ( ( s->SampRate * msec ) / 1000 );
}
/*
long
samples_to_msec( SD_HEADER *s, long samples )
{
	long msec;

	return msec;
}
*/

short
percent_to_sample_value( /*SD_HEADER *s,*/ short percent )
{

	return percent * SHRT_MAX / 100;

}

/*
long
sample_value_to_percent( SD_HEADER *s, long samp_value )
{
	long percent;

	return percent;
}
*/
long
sample_length( register SD_HEADER *samp )
{
	return( samp->SampLength > 0 ? samp->SampLength : 0L );
}

long
edit_length( register SD_HEADER *samp )
{
	long length;
	
	return( ( length = samp->EditEnd - samp->EditStart ) ? length : 0L );
}

short *
off_addr( register SD_HEADER *samp, register long offset )
{
	return( &samp->FirstSamp[ offset ] );
}

short *
edit_address( register SD_HEADER *samp )
{
	return ( &samp->FirstSamp[ samp->EditStart ] );
}

short *
post_edit_address( register SD_HEADER *samp )
{
	return( &samp->FirstSamp[ samp->EditEnd ] );
}

short 
max_zoom_ratio( register SD_HEADER *samp )
{
	return( (short) ( samp->SampLength / disp[WAVE].ob_width ) + 1 );
}

void
reset_edit_params( SD_HEADER *samp )
{
	samp->EditStart = default_sd_header.EditStart;
	samp->EditEnd   = default_sd_header.EditEnd;
}

void
reset_view_params( SD_HEADER *samp )
{
	samp->HScaleFact = default_sd_header.HScaleFact;
	samp->LeftEdge   = default_sd_header.LeftEdge;
}

void
reset_loop1_params( SD_HEADER *samp )
{
	samp->Loop1Start = default_sd_header.Loop1Start;
	samp->Loop1End   = default_sd_header.Loop1End;
	samp->Loop1Type  = default_sd_header.Loop1Type;
}

void
reset_loop2_params( SD_HEADER *samp )
{
	samp->Loop2Start = default_sd_header.Loop2Start;
	samp->Loop2End   = default_sd_header.Loop2End;
	samp->Loop2Type  = default_sd_header.Loop2Type;
}

void
reset_filename( SD_HEADER *samp )
{
	strcpy( samp->FileName, default_sd_header.FileName );
}

void
hndl_select_all( SD_HEADER *samp )
{
	samp->EditStart = 0;
	samp->EditEnd = samp->SampLength;
	force_complete_redraw();
}

void
hndl_view_all( SD_HEADER *samp )
{
	samp->LeftEdge = 0;
	samp->HScaleFact = max_zoom_ratio( samp );
	force_redraw( g.hnd, disp, VIEW_BAR );
	force_redraw( g.hnd, disp, CLOSE_UP );
	force_redraw( g.hnd, disp, TIMELINE );
}

short
mode_pexec( void )
{
	char prev_path[200], command[126] =" ";
	short prev_drv;
	short button;
	
	prev_drv = Dgetdrv();						// Store current drive
	prev_path[0] = 'A' + prev_drv;				// Dgetpath's drive arg is 1-based (0 = default)
	prev_path[1] = ':';
	Dgetpath( prev_path + 2, 0 );			// Store current path

	if( pexec_path[ strlen( pexec_path ) - 1 ] != '\\' ) strcat( pexec_path, "\\" );
	strcat( pexec_path, "*.PRG" );				// Tack suffix on end of pexec path

	fsel_exinput( pexec_path, pexec_file, &button, "Select a Program to Launch" );

	*(strrchr( pexec_path, '\\' ) + 1 ) = 0;	// Remove suffix from pexec path
	Dsetdrv( pexec_path[ 0 ] - 'A' );			// Set drive to that of child process
	Dsetpath( pexec_path ); 					// Set path to that of child process

	command[0] = strlen( command + 1 ); 		// prepare the command line

	Pexec( 0, pexec_file, command, 0L );		// Do it - add error checking

	Dsetdrv( prev_drv );						// Restore drive to prev. one
	Dsetpath( prev_path + 2 );						// Restore path to prev. one

	return M_MODE_1;							// Tell prg that we want to go back to M_MODE_1
}

void
hndl_change_rate( SD_HEADER *samp )
{
	long rate;
	
	sprintf( TED_PTEXT( CHANGERATE, RT_RATE ), "%ld", samp->SampRate );

	if( hndl_dial( CHANGERATE, RT_RATE ) == RT_CANCEL ) return;

	rate = atol( TED_PTEXT( CHANGERATE, RT_RATE ) );

	if( rate >= 8000 && rate <= 50000 ) {
		samp->SampRate = rate;
		samp->SampPeriod = 1000000000 / rate;
		force_redraw( g.hnd, disp, TIMELINE );
	}
	else form_alert( 1, "[0][Selected rate is out of|allowable range][ Cancel ]" );
}

void
hndl_set_midi_notes( short note )
{

	deselect_tree( SET_NOTE );

	sprintf( SET_NOTE[SET_NOTE_TITLE].ob_spec, "Set Note Number %d", note + 1 );
	sprintf( TED_PTEXT(SET_NOTE,SET_NOTE_CHAN), "%d", note_channel[ note ] + 1 );

	objc_iselect( SET_NOTE, SET_NOTE_C + ( note_number[ note ] % 12 ), TRUE );
	objc_iselect( SET_NOTE, SET_NOTE_0 + ( note_number[ note ] / 12 ), TRUE );

	sprintf( TED_PTEXT(SET_NOTE,SET_NOTE_VEL ), "%d", note_velocity[ note ] );

	if( hndl_dial( SET_NOTE, SET_NOTE_CHAN ) == SET_NOTE_CANCEL ) return;

	note_number[ note ] = ( find_selected_radio( SET_NOTE, SET_OCTAVE_PRNT ) - SET_NOTE_0 ) * 12;
	note_number[ note ] += find_selected_radio( SET_NOTE, SET_NOTE_PRNT ) - SET_NOTE_C;
	note_channel[ note ] = atoi( TED_PTEXT(SET_NOTE,SET_NOTE_CHAN) ) - 1;
	note_velocity[ note ] = atoi( TED_PTEXT(SET_NOTE,SET_NOTE_VEL) );
}

void
situate_object_in_window( short hnd, OBJECT *o )
{
	short dummy;

	wind_get( hnd, WF_WORKXYWH, &o->ob_x, &o->ob_y, &dummy, &dummy );
}

void
calc_exterior( short w_type, GRECT *gint, GRECT *gext )
{
	wind_calc( WC_BORDER, w_type, PTRS(gint), PTRX( &gext ) );
}

void
calc_interior( short w_type, GRECT *gext, GRECT *gint )
{
	wind_calc( WC_WORK, w_type, PTRS(gext), PTRX( &gint ) );
}

void 
set_clip( GRECT *r )
{
	short points[ 4 ];

	grect_conv( r, points );
	vs_clip( handle, TRUE, points );
}

void 
unset_clip()
{
	short points[ 4 ];

	vs_clip( handle, FALSE, points );
}

void
grect_conv( GRECT *r, short *array )
{
	*( array++ ) = r->g_x;
	*( array++ ) = r->g_y;
	*( array++ ) = r->g_x + r->g_w - 1;
	*( array	 ) = r->g_y + r->g_h - 1;
}

void
hndl_play_note( short note )
{
	char msg[3];
	
	msg[0] = 0x90 | (char)( note_channel[ note ] & 0x0f );
	msg[1] = (char) note_number[ note ] & 0x7f;
	msg[2] = (char) note_velocity[ note ] & 0x7f;
	
	Midiws( 2, msg );
	wait_for_button_up();
	
	msg[2] = 0x00;
	Midiws( 2, msg );
}


void
force_redraw( short hnd, OBJECT *tree, short index )
{
	GRECT w_in, rect;
	short buf[8];
	
	buf[0] = WM_REDRAW;
	buf[3] = hnd;
	wind_get( hnd, WF_CURRXYWH, ELTX( &w_in ) );
	objc_xywh( tree, index, &rect );
	rc_intersect( &w_in, &rect );
	rc_copy( &rect, (GRECT *)&buf[4] );	
	appl_write( gl_apid, 16, buf );
}

void
force_menu_event( short title, short item )
{
	short msg[ 8 ];

	msg[0] =	MN_SELECTED;
	msg[3] = title;
	msg[4] = item;
	appl_write( gl_apid, 16, msg );
}

short
wind_create_open( WINDOW *w, OBJECT *tree )
{
	short rtn = TRUE;
	GRECT full;
	
	calc_exterior( w->type, (GRECT *)&tree[0].ob_x, &full );
	if( ( w->hnd = wind_create( w->type, ELTS(full) ) ) >= 0 ) {
		if( w->type & NAME ) wind_title( w->hnd, w->title );
		if( w->type & INFO ) wind_info(  w->hnd, w->info  );
		wind_open( w->hnd, ELTS(full) );
		w->tree = tree;
	}
	else rtn = FALSE;
	return rtn;
}

short
wind_close_delete( WINDOW *w )
{
	wind_close( w->hnd );
	wind_delete( w->hnd );
	w->hnd = -1;
	w->tree = NULL;	
	return TRUE;
}

void
hndl_disp_toggles( void )
{
	deselect_tree( DISP_TOGGLES );

	objc_iselect( DISP_TOGGLES, TOG_QUICKDRAW, quickdraw );	
	objc_iselect( DISP_TOGGLES, TOG_LOOP_MARKERS, loop_markers );
	objc_iselect( DISP_TOGGLES, TOG_TIMELINE, timeline );

	objc_iselect( DISP_TOGGLES, timeline_format, TRUE );

	if( hndl_dial( DISP_TOGGLES, 0 ) == TOG_CANCEL ) return;

	quickdraw 		= is_selected( DISP_TOGGLES, TOG_QUICKDRAW );
	loop_markers 	= is_selected( DISP_TOGGLES, TOG_LOOP_MARKERS );
	timeline 		= is_selected( DISP_TOGGLES, TOG_TIMELINE );

	timeline_format = find_selected_radio( DISP_TOGGLES, DISP_FORMAT_PRNT );

	force_complete_redraw();
}

void
force_complete_redraw( void )
{
	force_redraw( g.hnd, disp, ROOT );
}

void
wait_for_button_up( void )
{
	short dummy, button;

	do { 
		graf_mkstate( &dummy, &dummy, &button, &dummy ); 
	} while( button );
}

void
EditEnd_constrain( SD_HEADER *samp )
{
	if( samp->SampLength < 1 ) { reset_edit_params( samp ); return; }	/* no sample: nothing to select */
	if( samp->EditEnd <= 0 ) samp->EditEnd = 1;
	if( samp->EditEnd <= samp->EditStart ) samp->EditStart = samp->EditEnd - 1;
	if( samp->EditEnd >  samp->SampLength ) samp->EditEnd = samp->SampLength;
}

void
EditStart_constrain( SD_HEADER *samp )
{
	if( samp->SampLength < 1 ) { reset_edit_params( samp ); return; }	/* no sample: nothing to select */
	if( samp->EditStart >= samp->SampLength ) samp->EditStart = samp->SampLength - 1;
	if( samp->EditStart >= samp->EditEnd ) samp->EditEnd = samp->EditStart + 1;
}

void
Loop1Start_constrain( SD_HEADER *samp )
{
	if( samp->SampLength < 1 ) { reset_loop1_params( samp ); return; }	/* no sample: no loop */
	if( samp->Loop1Start >= samp->SampLength ) samp->Loop1Start = samp->SampLength - 1;
	if( samp->Loop1Start >= samp->Loop1End ) samp->Loop1End = samp->Loop1Start + 1;
}

void
Loop1End_constrain( SD_HEADER *samp )
{
	if( samp->SampLength < 1 ) { reset_loop1_params( samp ); return; }	/* no sample: no loop */
	if( samp->Loop1End <= 0 ) samp->Loop1End = 1;
	if( samp->Loop1End <= samp->Loop1Start ) samp->Loop1Start = samp->Loop1End - 1;
	if( samp->Loop1End >  samp->SampLength ) samp->Loop1End = samp->SampLength;
}

void
set_viewbar( SD_HEADER *samp )
{
	disp[ VIEW_AREA ].ob_x = samp->LeftEdge / max_zoom_ratio( t );
	disp[ VIEW_AREA ].ob_width = ( samp->HScaleFact * disp[VIEW_BAR].ob_width ) / max_zoom_ratio( t );
}

short
hndl_dial( OBJECT *tree, short first_edit )
{
	GRECT r;
	short exit_butn;
	
	form_center( tree, ELTX(&r));
	objc_draw( tree, ROOT, MAX_DEPTH, ELTS(r) );
	exit_butn = form_do( tree, first_edit );
	form_dial( FMD_FINISH, 0, 0, 0, 0, ELTS(r) );
	tree[ exit_butn ].ob_state &= ~SELECTED;
	return exit_butn;
}

void
start_zero( SD_HEADER *samp )
{
	long temp_start = samp->Loop1Start + 1;

	if( !samp->FirstSamp || samp->SampLength < 3 ) return;	/* nothing to search */
	if( temp_start < 1 ) temp_start = 1;

	/* search forward for a negative-to-positive crossing, staying inside the buffer */
	while( temp_start < samp->SampLength - 1 &&
		!( samp->FirstSamp[ temp_start - 1 ] < 0 && samp->FirstSamp[ temp_start + 1 ] > 0 ) )
		temp_start += 1;

	if( temp_start < samp->SampLength - 1 && temp_start < samp->Loop1End )
		samp->Loop1Start = temp_start;

	force_complete_redraw();
}

void
end_zero( SD_HEADER *samp )
{
	long temp_end = samp->Loop1End - 1;

	if( !samp->FirstSamp || samp->SampLength < 3 ) return;	/* nothing to search */
	if( temp_end > samp->SampLength - 2 ) temp_end = samp->SampLength - 2;

	/* search backward for a negative-to-positive crossing, staying inside the buffer */
	while( temp_end >= 1 &&
		!( samp->FirstSamp[ temp_end - 1 ] < 0 && samp->FirstSamp[ temp_end + 1 ] > 0 ) )
		temp_end -= 1;

	if( temp_end >= 1 && temp_end > samp->Loop1Start )
		samp->Loop1End = temp_end;

	force_complete_redraw();
}

short
is_selected( register OBJECT *tree, register short index )
{
	return tree[ index ].ob_state & SELECTED;
}

void
objc_iselect( register OBJECT *tree, register short index, register short truth )
{
	if( tree[ index ].ob_flags & SELECTABLE ) {
		if( truth ) tree[ index ].ob_state |= SELECTED;
		else tree[ index ].ob_state &= ~SELECTED;
	}
}

void
objc_deselect( OBJECT *tree, short index )
{
	tree[ index ].ob_state &= ~SELECTED;
}

short
find_selected_radio( register OBJECT *tree, register short parent )
{
	register short first_child = tree[ parent ].ob_head;
	register short last_child = tree[ parent ].ob_tail;
	register short this_child;
	
	for( this_child = first_child; this_child != last_child; this_child = tree[ this_child ].ob_next ) {
		if( tree[ this_child ].ob_flags & RBUTTON && tree[ this_child ].ob_state & SELECTED )
			return this_child;
	}
	return last_child;
}

void
deselect_tree( register OBJECT *tree ) 
{
	register short index = 0;
	
	do {
		if( tree[ index ].ob_flags & SELECTABLE )
			tree[ index ].ob_state &= ~SELECTED;		
	} while ( ! ( tree[ index++ ].ob_flags & LASTOB ) );
}