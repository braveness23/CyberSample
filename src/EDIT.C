/* EDIT.C */


#include <gemlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "defs.h"
#include <math.h>

/* saturate to the 16-bit sample range instead of wrapping around */
static short
clip16( double v )
{
	if( v > 32767.0 ) return 32767;
	if( v < -32768.0 ) return -32768;
	return (short)v;
}

void
edit_amplitude( register SD_HEADER *samp, register short arg1, register short arg2 )
{
	register short *S_data = edit_address( samp );
	register long num = edit_length( samp );
	register float ratio;
	register short *S_end = post_edit_address( samp ) - 1;	/* last selected sample, not one past it */
	
	if( !edit_flag ) {
		form_alert( 1, ALERT_NOEDIT );
		return;
	}

	graf_mouse( HOURGLASS, 0L );

	switch( arg1 ) {
		case EA_DOWN_6DB :
			while( num-- ) *S_data++ >>= 1;	
			break;
		case EA_UP_6DB :
			while( num-- ) { *S_data = clip16( (double)*S_data * 2.0 ); S_data++; }	
			break;
		case EA_ZERO :
			while( num-- ) *S_data++ = 0;
			break;
		case EA_INVERT :
			while( num-- ) { *S_data = ( *S_data == -32768 ) ? 32767 : -*S_data; S_data++; }
			break;
		case EA_NORMALIZE :
			while( num-- ) arg2 = max( abs( *S_data++ ), arg2 );

			if( arg2 > 0 && arg2 < SHRT_MAX ) {	/* nothing to scale if the selection is silent */
				ratio = SHRT_MAX / (float)arg2;
				S_data = &samp->FirstSamp[ samp->EditStart ];
				num = samp->EditEnd - samp->EditStart;

				while( num-- ) { *S_data = (short)( *S_data * ratio ); S_data++; }

			}
			break;
		case EA_REVERSE :
			num /= 2;
			while( num-- ) {
				arg2 = *S_data;
				*S_data = *S_end;
				*S_end = arg2;
				S_data++;
				S_end--;
			}
			break;		
		default :
			break;
	}
	
	graf_mouse( ARROW, 0L );
	force_complete_redraw();
}

void
hndl_fade_in_out( register SD_HEADER *samp )
{
	static short func = FADE_OUT;
	static type = FADE_LINEAR;
	register short *S_data = edit_address( samp );
	register long num = edit_length( samp );
	register float ratio;
	register short *S_end = post_edit_address( samp ) - 1;	/* last selected sample, not one past it */
	register length = num;
		
	if( !edit_flag ) {
		form_alert( 1, ALERT_NOEDIT );
		return;
	}
	deselect_tree( FADE );
	
	objc_iselect( FADE, func, TRUE );
	objc_iselect( FADE, type, TRUE );

	if( hndl_dial( FADE, 0 ) == FADE_CANCEL ) return;

	func = find_selected_radio( FADE, FADE_FUNC_PRNT );
	type = find_selected_radio( FADE, FADE_TYPE_PRNT );

	graf_mouse( HOURGLASS, 0L );

	if( func == FADE_OUT && type == FADE_LINEAR ) {
		while( num-- ) {
			ratio = num / (float)length; 
			*S_data = (short)( *S_data * ratio );
			S_data++;
		}
	}
	else if( func == FADE_OUT && type == FADE_SQUARE ) {
		while( num-- ) {
			ratio = num / (float)length; 
			ratio *= ratio;
			*S_data = (short)( *S_data * ratio );
			S_data++;
		}
	}
	else if( func == FADE_IN && type == FADE_LINEAR ) {
		while( num-- ) {
			ratio = num / (float)length;
			*S_end = (short)( *S_end * ratio );
			S_end--;
		}
	}
	else if( func == FADE_IN && type == FADE_SQUARE ) {
		while( num-- ) {
			ratio = num / (float)length;
			ratio *= ratio;
			*S_end = (short)( *S_end * ratio );
			S_end--;
		}
	}
	
	graf_mouse( ARROW, 0L );
	force_complete_redraw();
}


void
hndl_mirage( SD_HEADER *s )
{
	static short normalize = 1;
	register short *S_data = edit_address( s );
	register long num = edit_length( s );
	static short resolution = 8;

	if( !edit_flag ) {
		form_alert( 1, ALERT_NOEDIT );
		return;
	}
try_again:
	deselect_tree( MIRAGE );
	
	objc_iselect( MIRAGE, MIRAGE_NORM, normalize );
	sprintf( TED_PTEXT(MIRAGE,MIRAGE_REZ), "%d", resolution );
	
	if( hndl_dial( MIRAGE, MIRAGE_REZ ) == MIRAGE_CANCEL ) return;
	
	normalize = is_selected( MIRAGE, MIRAGE_NORM );
	resolution = atol( TED_PTEXT(MIRAGE,MIRAGE_REZ) );
	
	if( resolution < 1 ) { 
		form_alert( 1, "[1][Minimum value is 1][OK]" );
		resolution = 1;
		goto try_again;
	}
	else if( resolution > 16 ) { 
		form_alert( 1, "[1][Maximum value is 16][OK]" );
		resolution = 16;
		goto try_again;
	}
	graf_mouse( HOURGLASS, 0L );
	while( num-- ) {
		*S_data >>= 16 - resolution;
		*S_data <<= 16 - resolution;
		S_data++;
	}
	graf_mouse( ARROW, 0L );
	force_complete_redraw();
	if( normalize ) force_menu_event( MN_DSP, MN_NORMALIZE );
}

void
hndl_smoothing( SD_HEADER *s )
{
	short type = 2; /* Temporary!!!!!!!!*/
	long n;
	long length = edit_length( s );
	short *od = edit_address( s );
	short *nd = NULL;
	
	if( !edit_flag ) {
		form_alert( 1, ALERT_NOEDIT );
		return;
	}

	type = form_alert( 2, "[2][ Smooth Amount ][Light|Med|Heavy]" );

	if( nd = (short *)malloc( length * sizeof(short) ) ) {
		graf_mouse( HOURGLASS, 0L );
		memcpy( nd, od, length * sizeof(short) );
		switch( type ) {
			case 1 :
				for( n = 1; n < length - 1; n++ )
					nd[n] = ( od[n+1] + od[n] + od[n-1] ) / 3;
				break;
			case 2 :
				for( n = 2; n < length - 2; n++ )
					nd[n] = ( od[n+2] + od[n+1] + od[n] + od[n-1] + od[n-2] ) / 5;
				break;
			case 3 :
				for( n = 3; n < length - 3; n++ )
					nd[n] = ( od[n+3] + od[n+2] + od[n+1] + od[n] + od[n-1] + od[n-2] + od[n-3] ) / 7;
				break;
		}
		memcpy( od, nd, length * sizeof(short) );
		free( nd );
		graf_mouse( ARROW, 0L );
		force_complete_redraw();
	}
	else form_alert( 1, ALERT_MEMORY );
}	

void
hndl_compress( SD_HEADER *s )
{
	static short threshold = 1024;
	static short ratio = 2;
	static short normalize = 1;
	static long release = 500;
	register short *S_data = edit_address( s );
	register long num = edit_length( s );
	long count = 0;
	short in = 0;
		
	deselect_tree( COMP );
	if( !edit_flag ) {
		form_alert( 1, ALERT_NOEDIT );
		return;
	}
try_again:
	objc_iselect( COMP, COMP_NORM, normalize );
	sprintf( TED_PTEXT(COMP,COMP_THRESH), "%d", threshold );
	sprintf( TED_PTEXT(COMP,COMP_RATIO), "%d", ratio );
	sprintf( TED_PTEXT(COMP,COMP_RELEASE), "%ld", release );

	if( hndl_dial( COMP, COMP_THRESH ) == COMP_CANCEL ) return;

	normalize = is_selected( COMP, COMP_NORM );
	threshold 	= atol( TED_PTEXT(COMP,COMP_THRESH) );
	ratio 		= atol( TED_PTEXT(COMP,COMP_RATIO) );
	release		= atol( TED_PTEXT(COMP,COMP_RELEASE) );

	if( ratio < 1 ) {											/* ratio is a divisor */
		form_alert( 1, "[1][Minimum ratio is 1][OK]" );
		ratio = 1;
		goto try_again;
	}

	graf_mouse( HOURGLASS, 0L );
	while( num-- ) {
		if( abs( *S_data ) > threshold ) {
			in = TRUE;
			count = 0;
		}
		if( in ) {
			if( count < release )
				*S_data /= ratio;
			else
				in = FALSE;
			count++;
		}
		S_data++;
	}
	graf_mouse( ARROW, 0L );
	force_complete_redraw();
	if( normalize ) force_menu_event( MN_DSP, MN_NORMALIZE );
}

void
hndl_gate( SD_HEADER *s )
{
	static short thresh_percent = 10;
	static short attack = 1;
	static short decay = 100;
	short thresh_value;
	long attack_count = 0;
	long attack_limit;
	long decay_count = 0;
	long decay_limit;
	float ratio = 0;
	register short *S_data = edit_address( s );
	register long num = edit_length( s );
		
	if( !edit_flag ) {
		form_alert( 1, ALERT_NOEDIT );
		return;
	}

try_again:
	sprintf( TED_PTEXT(GATE,GATE_THRESH), "%d", thresh_percent );
	sprintf( TED_PTEXT(GATE,GATE_ATTACK), "%d", attack );
	sprintf( TED_PTEXT(GATE,GATE_DECAY), "%d", decay );

	if( hndl_dial( GATE, GATE_THRESH ) == GATE_CANCEL ) return;

	thresh_percent 	= atol( TED_PTEXT(GATE,GATE_THRESH) );
	attack  		= atol( TED_PTEXT(GATE,GATE_ATTACK) );
	decay 			= atol( TED_PTEXT(GATE,GATE_DECAY) );

	if( attack < 1 || decay < 1 ) {								/* attack is a divisor */
		form_alert( 1, "[1][Attack and decay must|be at least 1 ms][OK]" );
		if( attack < 1 ) attack = 1;
		if( decay < 1 ) decay = 1;
		goto try_again;
	}

	thresh_value 	= percent_to_sample_value( thresh_percent );
	attack_limit 	= attack * samp_per_msec( s );
	decay_limit		= decay * samp_per_msec( s );
	if( attack_limit < 1 ) attack_limit = 1;					/* rates under 1 kHz round to 0 samples/ms */
	if( decay_limit < 1 ) decay_limit = 1;

	graf_mouse( HOURGLASS, 0L );

	while( num-- ) {
	
		if( abs( *S_data ) < thresh_value || 
			attack_count < attack_limit   ||
			decay_count  > decay_limit ) {
			ratio = (float) attack_count / attack_limit;
			*S_data = (short) *S_data * ratio;
			attack_count++;
			decay_count++;
		}
		if( attack_count >= attack_limit && abs( *S_data ) < thresh_value )
			attack_count = 0;
		S_data++;
	}

	graf_mouse( ARROW, 0L );
	force_complete_redraw();
}

void
hndl_bandpass( SD_HEADER *s )
{
	double a, b, c;
	double out1 = 0, out2 = 0;
	static short normalize = 1;
	static short noise = 1;
	static short c_freq = 1000;
	static short b_width = 1000;
	short *S_data = edit_address( s );
	long num = edit_length( s );
	long done;		
	short l;
	short *od = S_data, *nd, *nd_p;
	double d;
	
	deselect_tree( BP );

	if( !edit_flag ) {
		form_alert( 1, ALERT_NOEDIT );
		return;
	}

	objc_iselect( BP, BP_NOISE, noise );
	objc_iselect( BP, BP_NORM, normalize );

try_again:

	sprintf( TED_PTEXT(BP,BP_C_FREQ), "%d", c_freq );
	sprintf( TED_PTEXT(BP,BP_B_WIDTH), "%d", b_width );

	if( hndl_dial( BP, BP_C_FREQ ) == BP_CANCEL ) return;

	noise 		= is_selected( BP, BP_NOISE );
	normalize 	= is_selected( BP, BP_NORM );
	c_freq 	= atol( TED_PTEXT(BP,BP_C_FREQ) );
	b_width = atol( TED_PTEXT(BP,BP_B_WIDTH) );

	c = exp( -2 * PI * (float)b_width / s->SampRate );
	b = -4 * c / ( 1 + c ) * cos( 2 * PI * (float)c_freq / s->SampRate );
	if( ! noise )
		a = sqrt( ( ( 1 + c ) * ( 1 + c ) - b * b ) * ( 1 - c ) / ( 1 + c ) );
	else
		a = sqrt( 1 - b * b / ( 4 * c ) ) * ( 1 - c );

	if( nd = nd_p = (short *)malloc( num * sizeof(short) ) ) {
		graf_mouse( HOURGLASS, 0L );

		for( done = 0; done < num; done++ ) {
			l = (long)*S_data++;
			d = ( a * l - b * out1 ) - c * out2;
			out2 = out1;
			out1 = d;
			*nd_p++ = clip16( d );
		}			
		memcpy( od, nd, num * sizeof(short) );
		free( nd );
		graf_mouse( ARROW, 0L );
		force_complete_redraw();
		if( normalize ) force_menu_event( MN_DSP, MN_NORMALIZE );
	}
	else form_alert( 1, ALERT_MEMORY );
}

void
hndl_highlowpass( SD_HEADER *s, short type )
{
	double a, b;
	double in1, out1;
	static short c_freq = 1000;
	static short normalize = 1;
	short *S_data = edit_address( s );
	long num = edit_length( s );
	long done;		
	short l;
	short *od = S_data, *nd, *nd_p;
	double d;
	
	deselect_tree( HLP );

	if( !edit_flag ) {
		form_alert( 1, ALERT_NOEDIT );
		return;
	}
try_again:

	objc_iselect( HLP, HLP_NORM, normalize );
	sprintf( HLP[HLP_STRING].ob_spec, "%s", type ? "Low Pass Filter" : "High Pass Filter" );
	sprintf( TED_PTEXT(HLP,HLP_C_FREQ), "%d", c_freq );

	if( hndl_dial( HLP, HLP_C_FREQ ) == HLP_CANCEL ) return;

	normalize 	= is_selected( HLP, HLP_NORM );
	c_freq 	= atol( TED_PTEXT(HLP,HLP_C_FREQ) );

	a = ( PI * 2.0 * c_freq ) / s->SampRate;
	b = exp( -a / s->SampRate );
	in1 = 0.0;
	out1 = 0.0;

	if( nd = nd_p = (short *)malloc( num * sizeof(short) ) ) {
		graf_mouse( HOURGLASS, 0L );

		if( !type ) {
			d = out1;
			for( done = 0; done < num; done++ ) {
				l = *S_data++;
				d = ( b * ( ( d - in1 ) + (double)l ) ) / 65536.0;
				d *= 0.8;
				if( d > 32767 ) d = 32727;
				if( d < -32767 )d = -32676;
				in1 = l;
				*nd_p++ = clip16( d * 65536 );
			}			
			out1 = d;
		}
		else {
			for( done = 0; done < num; done++ ) {
				l = *S_data++;
				d = a * ( 1 / 65536 ) + b * ( in1 / 65536 );
				d *= 0.8;
				if( d > 32767 ) d = 32727;
				if( d < -32767 )d = -32676;
				in1 = l;
				*nd_p++ = clip16( d * 65536 );
			}			
		}		
		memcpy( od, nd, num * sizeof(short) );
		free( nd );
		graf_mouse( ARROW, 0L );
		force_complete_redraw();
		if( normalize ) force_menu_event( MN_DSP, MN_NORMALIZE );
	}
	else form_alert( 1, ALERT_MEMORY );
}

void
hndl_rate_conv( SD_HEADER *s )
{
	long oldrate = s->SampRate;
	static long newrate = 44100;
	double factor = 1.0, increment, curr_flt, rmndr, flr;
	long oldlength = sample_length( s ), newlength, pos, fl, nx;
	short *olddata = s->FirstSamp, *newdata = NULL;

	if( !olddata || oldlength < 2 || oldrate < 1 ) {
		form_alert( 1, "[1][No sample to convert][OK]" );
		return;
	}
		
try_again:
	sprintf( TED_PTEXT(RATE_CONV,RATE_CONV_OLD), "%ld", oldrate );
	sprintf( TED_PTEXT(RATE_CONV,RATE_CONV_NEW), "%ld", newrate );

	if( hndl_dial( RATE_CONV, RATE_CONV_NEW ) == RATE_CONV_CANCEL ) return;

	newrate = atol( TED_PTEXT(RATE_CONV,RATE_CONV_NEW) );

	if( newrate < 8000 ) {
		form_alert( 1, ALERT_LOW_RATE );
		newrate = 8000;
		goto try_again;
	}
	else if( newrate > 50000 ) {
		form_alert( 1, ALERT_HIGH_RATE );
		newrate = 50000;
		goto try_again;
	}

	if( newrate != oldrate ) {
		factor = newrate / (double)oldrate;
		increment = 1 / factor;
		newlength = (long)( oldlength * factor );

		if( newlength < 2 ) {
			form_alert( 1, "[1][Result would be too short][OK]" );
			return;
		}

		if( newdata = (short *)malloc( SIZ(newlength) ) ) {
			graf_mouse( HOURGLASS, 0L );

			newdata[ 0 ] = olddata[ 0 ];
			for( pos = 1; pos < newlength; pos++ ) {
				curr_flt = pos * increment;
				flr = (long)floor( curr_flt );
				rmndr = curr_flt - (double)flr;

				fl = (long)flr;
				nx = ( fl + 1 < oldlength ) ? fl + 1 : fl;			/* don't read past the last sample */
				newdata[pos] = olddata[fl] + ( rmndr * ( olddata[nx] - olddata[fl] ));

				//curr_a = *( olddata + (long)flr );
				//curr_b = *( olddata + (long)flr + 1 );

				//*N = curr_a + (short)( ( curr_b - curr_a ) * rmndr );
			}
			free( olddata );
			s->FirstSamp = newdata;
			s->SampLength = newlength;
			s->SampRate = newrate;
			s->SampPeriod = 1000000000/s->SampRate;		
			s->Loop1Start 	= (long)( s->Loop1Start * factor );
			s->Loop1End 	= (long)( s->Loop1End * factor );
			s->EditStart 	= (long)( s->EditStart * factor );
			s->EditEnd 		= (long)( s->EditEnd * factor );
			reset_view_params( s );								/* old LeftEdge/zoom no longer fit */

			graf_mouse( ARROW, 0L );
			force_complete_redraw();
		}

		else form_alert( 1, ALERT_MEMORY );
	}
}
