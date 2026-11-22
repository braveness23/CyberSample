/* DRAW.C */

#include <gemlib.h>
#include <osbind.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "defs.h"


int my_redraw( int h, GRECT *rect )
{
	if( h == g.hnd ) {
		objc_draw( disp, ROOT, MAX_DEPTH, PTRS(rect));
		draw_wave( t, disp, WAVE, rect, max_zoom_ratio( t ), 0 );
		draw_wave( t, disp, CLOSE_UP, rect, t->HScaleFact, t->LeftEdge );
	}
	return 1;
}

void
set_grid( SD_HEADER *samp )
{
	short tick;
	long sample;
	
	for( tick = 0; tick < 6; tick++ ) {
		sample = samp->LeftEdge + ( disp[ TICK_0 + tick ].ob_x * samp->HScaleFact );
		switch( timeline_format ) {
			case TOG_SAMP :
				sprintf( TED_PTEXT( disp, TICK_0 + tick ), "%ld", sample );
				break;
			case TOG_MSEC :
				sprintf( TED_PTEXT( disp, TICK_0 + tick ), "%.1f ms", ( (float)sample / samp->SampRate ) * 1000);
				break;
			case TOG_SEC :
				sprintf( TED_PTEXT( disp, TICK_0 + tick ), "%.2f Sec",( (float) sample ) / samp->SampRate );
				break;
		}
	}
	if( timeline )
		disp[TIMELINE].ob_flags &= ~HIDETREE;
	else
		disp[TIMELINE].ob_flags |= HIDETREE;
}

void
draw_wave( SD_HEADER *samp, OBJECT *tree, short index, GRECT *r, long ppc, long leftedge )
{

	register short temp_min, temp_max;
	register long offset;
	register short word, incr = quickdraw ? ( ppc >> 5 ) + 1 : 1;
	register short *S_data = samp->FirstSamp;
	short v_cent;
	GRECT phys_coord, clip = *r, edit_area;
	short xy[4];
	short start_pt, end_pt, current_pt;
	short view_bits = 0;

	while( USHRT_MAX >> view_bits > tree[index].ob_height ) view_bits++;

	objc_xywh( tree, index, &phys_coord );							//Find phys coord of OBJECT
	if( rc_intersect( &phys_coord, &clip ) ) {						//Find intersection of OBJECT and clipping rect, if any
		v_cent = ( tree[index].ob_height >> 1 ) + phys_coord.g_y; 	//Calc phys y coord of OBJECTs center

		edit_area = phys_coord;
		edit_area.g_x += ( samp->EditStart - leftedge ) / ppc;
		edit_area.g_w = edit_length( samp ) / ppc;
		
		set_clip( &clip );
		vswr_mode( handle, MD_REPLACE );
		grect_conv( &edit_area, xy );
		v_bar( handle, xy );
		 
		vswr_mode( handle, MD_XOR );

		start_pt = max( 0, clip.g_x - phys_coord.g_x );				//Calc the first point to be drawn
		end_pt = min( phys_coord.g_w, clip.g_x + clip.g_w );		//Calc the last point to be draw
		xy[0] = xy[2] = phys_coord.g_x + start_pt;					//Set the phys x coord of our line so we can ++ incr them in loop

		for( current_pt = start_pt; current_pt < end_pt; current_pt++ ) { 
			offset = leftedge + ( current_pt * ppc );
			if( offset > samp->SampLength ) break;
			temp_max = 0;
			temp_min = 0;
			for( word = 0; word < ppc; offset += incr, word += incr ) {
				if( offset < samp->SampLength ) {
					temp_max = max( temp_max, *( S_data + offset ));
					temp_min = min( temp_min, *( S_data + offset ));
				}
			}
			xy[ 1 ] = v_cent - ( temp_min >> view_bits );
			xy[ 3 ] = v_cent - ( temp_max >> view_bits );
			v_pline( handle, 2, xy );
			xy[0]++;
			xy[2]++;
		}

		if( loop_markers ) {
			vswr_mode( handle, MD_REPLACE );
			objc_xywh( tree, index, (GRECT *)&ICONS->ob_x );
			ICONS[ICN_LOOP1START].ob_y = 0;
			ICONS[ICN_LOOP1START].ob_x = ( samp->Loop1Start - leftedge ) / ppc;
			ICONS[ICN_LOOP1END].ob_y = ICONS[ROOT].ob_height - ICONS[ICN_LOOP1END].ob_height;
			ICONS[ICN_LOOP1END].ob_x = ( samp->Loop1End - leftedge ) / ppc;
			objc_draw( ICONS, ICN_LOOP1START, 0, ELTS(clip) ); 		
			objc_draw( ICONS, ICN_LOOP1END, 0, ELTS(clip) ); 		
		}
		unset_clip();
	}
}

