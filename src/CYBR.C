/* CYBR.C */

#define MAINMODUAL

#include <gemlib.h>
#include <osbind.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include "defs.h"



void
_main( int argc, char *argv[] )
{


	if( ( gl_apid = appl_init() ) != -1 ) {
		base = (KBDVBASE *)Kbdvbase();
		midivect = (void *)base->midivec;
		base->midivec = (void *)midithru;
		midibuf = Iorec(2);
		start_drv = Dgetdrv();
		/* fix: Dgetpath() drive arg is 1-based (0=default) but Dgetdrv() is 0-based; also keep drive letter in path */
		start_path[0] = 'A' + start_drv;
		start_path[1] = ':';
		Dgetpath( start_path + 2, 0 );
		strcpy( pexec_path, start_path );
		strcpy( sd_path, start_path );
		strcpy( del_path, start_path );
		strcpy( raw_path, start_path );
		graf_mouse( ARROW, 0L );
		wind_get( 0, WF_WXYWH, ELTX( &desk ) );
		rsrc_init();
		handle = graf_handle( &wchar, &hchar, &wbox, &hbox );
		v_opnvwk( in, &handle, out );
		if( handle ) {
			disp = DISPMONO;
			setup_samples();
			if( argc > 1 ) open_sd( t, argv[1] );

			while( main_flag ) {
				switch( main_flag ) {
					case M_MODE_1 :
						main_flag = mode_1();
						break;
					case M_PEXEC :
						main_flag = mode_pexec();
						break;
					case M_QUIT :
					default :
						main_flag = 0;
				}
			}
			cleanup_samples();
			v_clsvwk( handle );
		} else form_alert( 1, "[1][Error with open_vwork][ABORT]" );
		Dsetdrv( start_drv );
		Dsetpath( start_path );
		base->midivec = midivect;
		appl_exit();
		exit( 0 );
	}
}

short 
mode_1( void ) 
{
	int event;
	short mousex, mousey, button, shiftkey, keycode, clicked, msg[8], mode = M_MODE_1;

	menu_bar( CYBMENU, TRUE );

	if( ! wind_create_open( &g, disp ) ) return M_QUIT;
	
	do {				// *** THIS IS THE MAIN LOOP *** - while( mode == M_MODE_1 )

			enable_check_menu();

			event = evnt_multi(
			MU_MESAG|MU_KEYBD|MU_BUTTON,
			257, 3, 0,
			0,0,0,0,0,
			0,0,0,0,0,
			msg,
			0,0,
			&mousex, &mousey, &button, &shiftkey,
			&keycode, &clicked );
			
		if( event == MU_MESAG ) {
			switch( msg[0] ) {
				case WM_REDRAW :
					situate_object_in_window( g.hnd, disp );
					set_grid( t );
					set_viewbar( t );
					objc_iselect( disp, MODE_EDIT, left_mode == MODE_EDIT );
					objc_iselect( disp, MODE_LOOP, left_mode == MODE_LOOP );
					objc_iselect( disp, MODE_VIEW, left_mode == MODE_VIEW );
					wind_redraw( (int)msg[3], (GRECT * )&msg[4], my_redraw  );
					break;
				case WM_TOPPED :
					wind_set( msg[3], WF_TOP );
					break;
				case WM_CLOSED :
					if( form_alert( 1, ALERT_QUIT ) == 1 ) 
						mode = M_QUIT;
					break;
				case WM_FULLED :
					{
						GRECT full, curr, prev;
						
						wind_get( msg[3], WF_CXYWH, ELTX( &curr ) );
						wind_get( msg[3], WF_FXYWH, ELTX( &full ) );
						wind_get( msg[3], WF_PXYWH, ELTX( &prev ) );
						if( rc_equal( &full, &curr ) ) wind_set( msg[3], WF_CXYWH, ELTS(prev) );	 
						else  wind_set( msg[3], WF_CXYWH, ELTS(full) ); 
					}
					break;
				case WM_SIZED :
					{
						GRECT inside;

						if( msg[3] == g.hnd ) 
						{
							calc_interior( g.type, (GRECT *)&msg[4], &inside );
							rc_intersect( (GRECT *)&disp[0].ob_x, &inside );
							calc_exterior( g.type, &inside, (GRECT *)&msg[4] );
							wind_set( g.hnd, WF_CURRXYWH, msg[4], msg[5], msg[6], msg[7] ); 
						}
					}
					break;
				case WM_MOVED :
					if( msg[3] == g.hnd ) 
					{
						wind_set( g.hnd, WF_CURRXYWH, msg[4], msg[5], msg[6], msg[7] ); 
						force_complete_redraw();
					}
					break;
				case MN_SELECTED :
					if( msg[3] ) menu_tnormal( CYBMENU, msg[3], FALSE );	// highlight menubar only if msg[3] != 0
					switch( msg[4] ) {
						case MN_ABOUT : form_alert( 1, ALERT_ABOUT );	break;
						case MN_OPEN :
							open_sd( t, NULL );
							break;
						case MN_NEW :
							if( new_sample( t, PROMPT ) )	force_complete_redraw();
							break;
						case MN_SAVE :
							if( sample_flag ) save_sd( t, NULL );
							break;
						case MN_SAVE_RAW :
							if( sample_flag ) save_raw( t, NULL );
							break;
						case MN_DELETE :
							delete_file( );
							break;
						case MN_GET :
							base->midivec = midivect;
							hndl_get( t );
							base->midivec = (void *)midithru;
							break;
						case MN_SEND :
							if( sample_flag ) {
								base->midivec = midivect;
								hndl_send( t );
								base->midivec = (void *)midithru;
							}
							break;
						case MN_SAMP_TO_CLIP :
							hndl_sample_to_clipboard( t );
							break;
						case MN_CLIP_TO_SAMP :
							hndl_clipboard_to_sample( t );
							break;
						case MN_RUN_OTHER : 	
							mode = M_PEXEC;		
							break;
						case MN_QUIT :
							if( form_alert( 1, ALERT_QUIT ) == 1 )
								mode = M_QUIT;
							break;
						case MN_IMPORT_RAW :
							import_raw( t );
							break;
						case MN_UNDO :
							if( undo_flag ) {
								memswp( t, u, sizeof( SD_HEADER ) );
								force_complete_redraw();
							}
							break;
						case MN_CUT :
							if( edit_flag )hndl_cut( t );
							break;
						case MN_COPY :
							if( edit_flag )hndl_copy( t );
							break;
						case MN_PASTE :
							if( paste_flag )hndl_paste( t );
							break;
						case MN_SMPL1:	case MN_SMPL2:	case MN_SMPL3:	case MN_SMPL4:
						case MN_SMPL5:	case MN_SMPL6:	case MN_SMPL7:	case MN_SMPL8:
						case MN_PASTE_BUF :
							smp_no = msg[4] - MN_SMPL1;
							t = &s[ smp_no ];
							force_complete_redraw();
							break;
						case MN_SAMP_RATE :
							hndl_change_rate( t );
							break;
						case MN_DISP_TOGGLES :
							hndl_disp_toggles();
							break;
						case MN_MIDITHRU :
							midithru_flag = midithru_flag ? 0 : 1;
							break;
						case MN_SET_NOTES :
							hndl_set_midi_notes( form_alert( 1, "[1][ Select Note To Edit ][ 1 | 2 | 3 ]" ) - 1 );
							break;
						case MN_LOOP_OFF :
							t->Loop1Type = 0;
							break;
						case MN_LOOP_FWD :
							t->Loop1Type = 1;
							break;
						case MN_LOOP_REV :
							t->Loop1Type = 2;
							break;
						case MN_SELECT_ALL :	if( sample_flag )hndl_select_all( t );		break;
						case MN_VIEW_ALL :		if( sample_flag )hndl_view_all( t ); break;
						case MN_TRUNCATE:		if( edit_flag )truncate( t );			break;
						case MN_NORMALIZE:		if( edit_flag )edit_amplitude( t, EA_NORMALIZE, 0 );	break;
						case MN_INVERT: 		if( edit_flag )edit_amplitude( t, EA_INVERT, 0 );		break;
						case MN_UP_6DB: 		if( edit_flag )edit_amplitude( t, EA_UP_6DB, 0 );		break;
						case MN_DOWN_6DB:		if( edit_flag )edit_amplitude( t, EA_DOWN_6DB, 0 );	break;
						case MN_REVERSE:		if( edit_flag )edit_amplitude( t, EA_REVERSE, 0 ); 	break;
						case MN_ZERO  : 		if( edit_flag )edit_amplitude( t, EA_ZERO, 0 );		break;
						case MN_FADE :			if( edit_flag )hndl_fade_in_out( t );	break;
						case MN_MIRAGE :		if( edit_flag )hndl_mirage( t );break;
						case MN_SMOOTHING :			hndl_smoothing( t ); break;
						case MN_RATE_CONV :			hndl_rate_conv( t ); break;
						case MN_COMPRESS :			hndl_compress( t ); break;
						case MN_GATE :				hndl_gate( t ); break;
						case MN_BANDPASS :			hndl_bandpass( t ); break;
						case MN_HIGHPASS :			hndl_highlowpass( t, 0 ); break;
						case MN_LOWPASS :			hndl_highlowpass( t, 1 ); break;

						default :
							break;
					}
					if( msg[3] ) menu_tnormal( CYBMENU, msg[3], TRUE );			// 'un-highlight' the menubar only if msg[3] != 0
					break;
				default :
					break;
			}
		}
		else if( event == MU_KEYBD ) {
			switch( keycode ) {
				case UNDO	:	force_menu_event( 0,		MN_UNDO );		break;
//				case CNTL_Z	:	force_menu_event( 0,		MN_UNDO );		break;
				case CLR_HOME:	force_menu_event( 0,		MN_VIEW_ALL );	break;
				case CNTL_N :	force_menu_event( MN_FILE,	MN_NEW );		break;
				case CNTL_O :	force_menu_event( MN_FILE,	MN_OPEN );		break;
				case CNTL_S :	force_menu_event( MN_FILE,	MN_SAVE );		break;
				case CNTL_D :	force_menu_event( MN_FILE,	MN_DELETE );	break;
				case CNTL_Q :	force_menu_event( MN_FILE,	MN_QUIT );		break;
				case CNTL_X :	force_menu_event( MN_EDIT,	MN_CUT );		break;
				case CNTL_C :	force_menu_event( MN_EDIT,	MN_COPY );		break;
				case CNTL_V :	force_menu_event( MN_EDIT,	MN_PASTE ); 	break;
				case CNTL_A :	force_menu_event( 0,		MN_SELECT_ALL );break;
				case CNTL_G :	force_menu_event( 0,		MN_GET );		break;
				case K_R :		force_menu_event( 0,		MN_REVERSE );	break;
				case K_N :		force_menu_event( 0,		MN_NORMALIZE ); break;
				case N_PLUS :	force_menu_event( 0,		MN_UP_6DB );	break;
				case N_MINUS :	force_menu_event( 0,		MN_DOWN_6DB );	break;
				case K1 :		force_menu_event( 0,		MN_SMPL1 ); 	break;
				case K2 :		force_menu_event( 0,		MN_SMPL2 ); 	break;
				case K3 :		force_menu_event( 0,		MN_SMPL3 ); 	break;
				case K4 :		force_menu_event( 0,		MN_SMPL4 ); 	break;
				case K5 :		force_menu_event( 0,		MN_SMPL5 ); 	break;
				case K6 :		force_menu_event( 0,		MN_SMPL6 ); 	break;
				case K7 :		force_menu_event( 0,		MN_SMPL7 ); 	break;
				case K8 :		force_menu_event( 0,		MN_SMPL8 ); 	break;
				case F1 :
					left_mode = MODE_EDIT;
					force_redraw( g.hnd, disp, MODE_PRNT );
					break;
				case F2 :
					left_mode = MODE_LOOP;
					force_redraw( g.hnd, disp, MODE_PRNT );
					break;
				case F3 :
					left_mode = MODE_VIEW;
					force_redraw( g.hnd, disp, MODE_PRNT );
					break;
				case OPN_PARENTH:
					start_zero( t );
					break;
				case CLS_PARENTH:
					end_zero( t );
					break;
				
				default :
					break;
			}
		}
		else if( event == MU_BUTTON ) { 
			if( button == M_RIGHT ) {
				if( wind_find( mousex, mousey ) == g.hnd ) {
					short obj;
					
					obj = objc_find( disp, ROOT, MAX_DEPTH, mousex, mousey );
					switch( obj ) {
						case PLAY_1 :
						case PLAY_2 :
						case PLAY_3 :
							hndl_set_midi_notes( obj - PLAY_1 );
							break;
						default :
							break;
					}
				}
			}
			else if( button == M_LEFT ) { 
				if( wind_find( mousex, mousey ) == g.hnd ) {
					short obj;
					
					obj = objc_find( disp, ROOT, MAX_DEPTH, mousex, mousey );
					switch( obj ) {
						case WAVE :
						case CLOSE_UP :
							{ 
								GRECT wave;
								short endw, endh;
								long leftedge = obj == WAVE ? 0 : t->LeftEdge;
								long ppc = obj == WAVE ? max_zoom_ratio(t) : t->HScaleFact;
								long butn_down_location;
								
								objc_xywh( disp, obj, &wave );

								butn_down_location = ( ( mousex - wave.g_x ) * ppc ) + leftedge;

								switch( left_mode ) {
									case MODE_VIEW :
									{
										long RightEdge;

										t->LeftEdge = butn_down_location;
										graf_rubberbox( mousex, mousey, 1, 1, &endw, &endh );
										RightEdge = t->LeftEdge + ( endw * ppc );
										if( RightEdge > t->SampLength ) RightEdge = t->SampLength;
										t->HScaleFact = (( RightEdge - t->LeftEdge ) / disp[CLOSE_UP].ob_width ) + 1;
										force_redraw( g.hnd, disp, CLOSE_UP );
										force_redraw( g.hnd, disp, TIMELINE );
										force_redraw( g.hnd, disp, VIEW_BAR );
									}
									break;

									case MODE_EDIT :
										if( ! shiftkey ) {
											t->EditStart = butn_down_location;
											graf_rubberbox( mousex, mousey, 1, 1, &endw, &endh );
											t->EditEnd = t->EditStart + ( endw * ppc );
											EditStart_constrain( t );
											EditEnd_constrain( t );
											force_complete_redraw();
										}
										else if( shiftkey == K_LSHIFT ) {
											t->EditStart = butn_down_location;
											wait_for_button_up();
											EditStart_constrain( t );
											force_complete_redraw();
										}
										else if( shiftkey == K_RSHIFT ) {
											t->EditEnd = butn_down_location;
											wait_for_button_up();
											EditEnd_constrain( t );
											force_complete_redraw();
										}
										break;
									case MODE_LOOP :
										if( ! shiftkey ) {
											t->Loop1Start = butn_down_location;
											graf_rubberbox( mousex, mousey, 1, 1, &endw, &endh );
											t->Loop1End = t->Loop1Start + ( endw * ppc );
											Loop1Start_constrain( t );
											Loop1End_constrain( t );
											force_complete_redraw();
										}
										else if( shiftkey == K_LSHIFT ) {
											t->Loop1Start = butn_down_location;
											wait_for_button_up();
											Loop1Start_constrain( t );
											force_complete_redraw();
										}
										else if( shiftkey == K_RSHIFT ) {
											t->Loop1End = butn_down_location;
											wait_for_button_up();
											Loop1End_constrain( t );
											force_complete_redraw();
										}
										break;
								}
							}				
							break;
						case VIEW_AREA :
							{
								short lastx, lasty;
								GRECT view_bar, view_area;
							
								objc_xywh( disp, VIEW_BAR, &view_bar );
								objc_xywh( disp, VIEW_AREA, &view_area );
								graf_dragbox(	view_area.g_w, view_area.g_h,
												view_area.g_x, view_area.g_y,
												ELTS(view_bar),
												&lastx, &lasty );
								disp[ VIEW_AREA ]. ob_x = lastx - view_bar.g_x;
								objc_xywh( disp, VIEW_BAR, &view_bar );
								objc_xywh( disp, VIEW_AREA, &view_area );
								t->LeftEdge = ( view_area.g_x - view_bar.g_x ) * max_zoom_ratio( t );
								force_redraw( g.hnd, disp, CLOSE_UP );
								force_redraw( g.hnd, disp, TIMELINE );
								force_redraw( g.hnd, disp, VIEW_BAR );
							}
							break;
						case PLAY_1 :
							hndl_play_note( 0 );
							break;
						case PLAY_2 :
							hndl_play_note( 1 );
							break;
						case PLAY_3 :
							hndl_play_note( 2 );
							break;
						case MODE_EDIT :
						case MODE_LOOP :
						case MODE_VIEW :
							left_mode = obj;
							force_redraw( g.hnd, disp, MODE_PRNT );
							break;							
						default :
							break;
					}
				}
			}
		}
	} while( mode == M_MODE_1 );															//	THIS IS THE END OF THE MAIN LOOP

	wind_close_delete( &g );
	menu_bar( CYBMENU, FALSE );

	return mode;
}

void
enable_check_menu( void )
{
	short z;

	wind_update( BEG_UPDATE );	// Borrow control from AES

	for( z = MN_SMPL1; z < MN_PASTE_BUF + 1; z++ ) {	// 'uncheck' each sample number
		if( CYBMENU[ z ].ob_state & CHECKED )
			menu_icheck( CYBMENU, z, FALSE );
	}
	menu_icheck( CYBMENU, MN_SMPL1 + smp_no, TRUE );	// 'check' the current sample number

	sample_flag = sample_length(t) 	? TRUE : FALSE;
	edit_flag 	= ( sample_flag && edit_length(t) ) ? TRUE : FALSE;	/* a selection only counts when a sample is loaded */
	paste_flag 	= sample_length(p) 	? TRUE : FALSE;
	undo_flag 	= sample_length(u) 	? TRUE : FALSE;

	menu_ienable( CYBMENU, MN_SAVE,			sample_flag );
	menu_ienable( CYBMENU, MN_SEND,			sample_flag );
	menu_ienable( CYBMENU, MN_UNDO,			undo_flag );
	menu_ienable( CYBMENU, MN_CUT,				edit_flag );
	menu_ienable( CYBMENU, MN_COPY,			edit_flag );
	menu_ienable( CYBMENU, MN_CLEAR,			edit_flag );
	menu_ienable( CYBMENU, MN_TRIM,			edit_flag );
	menu_ienable( CYBMENU, MN_PASTE,			paste_flag );
	menu_ienable( CYBMENU, MN_SELECT_ALL,		sample_flag );
	menu_ienable( CYBMENU, MN_TRUNCATE,		edit_flag );
	menu_ienable( CYBMENU, MN_INVERT,			edit_flag );
	menu_ienable( CYBMENU, MN_NORMALIZE,		edit_flag );
	menu_ienable( CYBMENU, MN_UP_6DB,			edit_flag );
	menu_ienable( CYBMENU, MN_DOWN_6DB,		edit_flag );
	menu_ienable( CYBMENU, MN_REVERSE, 		edit_flag );
	menu_ienable( CYBMENU, MN_ZERO,			edit_flag );
	menu_ienable( CYBMENU, MN_MIRAGE,			edit_flag );
	menu_ienable( CYBMENU, MN_FADE,			edit_flag );
	menu_ienable( CYBMENU, MN_SMOOTHING,		edit_flag );
	menu_ienable( CYBMENU, MN_COMPRESS,		edit_flag );
	menu_ienable( CYBMENU, MN_GATE,			edit_flag );
	menu_ienable( CYBMENU, MN_BANDPASS,		edit_flag );
	menu_ienable( CYBMENU, MN_LOWPASS,			edit_flag );
	menu_ienable( CYBMENU, MN_HIGHPASS,		edit_flag );
	menu_ienable( CYBMENU, MN_VIEW_ALL,		sample_flag );
	menu_icheck ( CYBMENU, MN_MIDITHRU,		midithru_flag );
	menu_icheck ( CYBMENU, MN_LOOP_OFF,		t->Loop1Type == 0 );
	menu_icheck ( CYBMENU, MN_LOOP_FWD,		t->Loop1Type == 1 );
	menu_icheck ( CYBMENU, MN_LOOP_REV,		t->Loop1Type == 2 );
	sprintf( CYBMENU[MN_LOOP_START].ob_spec, "  Loop Start: %ld", t->Loop1Start );
	sprintf( CYBMENU[MN_LOOP_END  ].ob_spec, "  Loop End  : %ld", t->Loop1End );
	sprintf( CYBMENU[MN_EDIT_START].ob_spec,  "  Start:% ld", t->EditStart );
	sprintf( CYBMENU[MN_EDIT_END  ].ob_spec,  "  End  :% ld", t->EditEnd );
	sprintf( CYBMENU[MN_SAMP_RATE ].ob_spec,  "  Rate: %ld Hz...", t->SampRate );
	sprintf( CYBMENU[MN_SAMP_LENGTH].ob_spec, "  Length: %ld", t->SampLength );


	wind_update( END_UPDATE );	// Return control to AES
}

#undef MAINMODUAL
