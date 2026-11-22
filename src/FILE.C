/* FILE.C */


#include <stdio.h>
#include <string.h>
#include <gemlib.h>
#include <stdlib.h>
#include <osbind.h>
#include <dos.h>
#include "defs.h"

short
open_sd( SD_HEADER *samp, char *file )
{
	short fhandle;
	
	if( ! new_sample( samp, PROMPT ) ) return FALSE;

	if( !file ) {
		if( !hndl_fsel( sd_path, sd_file, sd_suffix, "Select .SD file to load..." ) ) return FALSE;
		file = sd_file;
	}
	
	graf_mouse( HOURGLASS, 0L );

	if( ( fhandle = Fopen( file, 0 ) ) >= 0 ) {
		if( Fread( fhandle, sizeof(SD_HEADER), samp ) == sizeof(SD_HEADER)) {
			if( samp->header_size == 1336 ) {
				if( samp->SampLength ) {
					if(samp->FirstSamp=(short *)malloc( samp->SampLength * sizeof(char) )){
						if( Fread( fhandle, samp->SampLength, samp->FirstSamp)==samp->SampLength){
							samp->SampLength /= sizeof(short);
							reset_view_params( samp );
							reset_filename( samp );
						}
						else {
							form_alert( 1, "[0][Error reading data][ OK ]" );
							new_sample( samp, NPROMPT );
						}
					}
					else {
						form_alert( 1, ALERT_MEMORY );
						new_sample( samp, NPROMPT );
					}
				}
			}
			else form_alert( 1, "[1][Invalid File Header][Abort]" );
		}
		else form_alert( 1, "[0][Error reading header][ OK ]" );

		if( Fclose( fhandle ) != 0 ) 
			form_alert( 1, "[0][Error closing file after read][ OK ]" );  
	}
	else form_alert( 1, "[0][Error opening file for reading][ OK ]" );	

	force_complete_redraw();
	graf_mouse( ARROW, 0L );
	return TRUE;
}

short
save_sd( SD_HEADER *samp, char *file )
{
	SD_HEADER temp = *samp;
	short fhandle;
	short truth = FALSE;

	if( !file ) {
		if( ! hndl_fsel( sd_path, sd_file, sd_suffix, "Save .SD file..." ) ) return FALSE;
		file = sd_file;
	}

	graf_mouse( HOURGLASS, 0L );
	if( ( fhandle = Fcreate( file, 0 ) ) >=0 ) {
		temp.SampLength *= sizeof( short );
		reset_view_params( &temp );
		temp.FirstSamp = NULL;
		reset_filename( &temp );
		if( Fwrite( fhandle, sizeof( SD_HEADER ), &temp ) == sizeof( SD_HEADER ) ) {
			if( temp.SampLength ) {
				if(Fwrite(fhandle,temp.SampLength,samp->FirstSamp)==temp.SampLength){
					truth = TRUE;
				}else form_alert( 1, "[1][Error writing data][OK]" );
			}else form_alert( 1, "[1][Sample of zero length][Thats OK]" );
		}else form_alert( 1, "[1][Error writing header][ OK ]" );
		if( Fclose( fhandle ) != 0 ) form_alert( 1, "[1][Error closing file after save][ OK ]" ); 				
	}else form_alert( 1, "[1][Couldnt open file for writing][ OK ]" );
	graf_mouse( ARROW, 0L );
	return truth;
}


short
delete_file( void )
{
	short fhandle;
	short truth = FALSE;

	if( ! hndl_fsel( del_path, del_file, del_suffix, "Select file to Delete...CAREFULLY!!!" ) ) return FALSE;

	graf_mouse( HOURGLASS, 0L );
	if( form_alert( 1, "[1][Delete File?][OK|Cancel]" ) == 1 ) {
		if( ( fhandle = Fopen( del_file, 0 ) ) >= 0 ) {
			Fclose( fhandle );
			Fdelete( del_file );
			truth = TRUE;
		}	else form_alert( 1, "[0][File not found][ OK ]" );
	}
	graf_mouse( ARROW, 0L );
	return truth;
}
		
//******** SDS offset = 360, period at 20 ******

short
import_raw( SD_HEADER *samp )
{
	static long offset = 0;
	static short type = SIGNED;
	static short rate = RATE_441;
	static short bits = BIT16;
	long user_rate = 44100, period;
	short fhandle;
	struct FILEINFO info;
	long num = 0;
	short *S_data = NULL;
	char buffer[512];
	short read, next = 0;
	char *bit12_buff = NULL;
	long bit12_buff_size = 0;

	if( !new_sample( samp, PROMPT ) ) return FALSE;

	if( !hndl_fsel( raw_path, raw_file, raw_suffix, "Select ANY file to Load..." ) ) return FALSE;

	Fsetdta(&info);
	Fsfirst(raw_file,0);

	deselect_tree( RAW );
	
	objc_iselect( RAW, type, TRUE );
	objc_iselect( RAW, rate, TRUE );
	objc_iselect( RAW, bits, TRUE );

	sprintf( TED_PTEXT(RAW,OFFSET), "%ld", offset );
	sprintf( RAW[FILE_LENGTH].ob_spec, "%ld", info.size );

	if( hndl_dial( RAW, OFFSET ) == RAW_CANCEL ) return FALSE;

	type = find_selected_radio( RAW, RAW_TYPE_PRNT );
	rate = find_selected_radio( RAW, RAW_RATE_PRNT );
	bits = find_selected_radio( RAW, RAW_BITS_PRNT );

	offset = atol( TED_PTEXT(RAW,OFFSET) );
	if( offset >= info.size || offset < 0 ) {													// Check for offset errors
		form_alert( 1, "[0][Invalid Offset Value][ Cancel ]" );
		return FALSE;
	}

	if( rate == RATE_USER ) {													// USER RATE SELECTED
		user_rate = atol( TED_PTEXT(RAW,USER_RATE) );
		if( user_rate < 8000 || user_rate > 50000 ) {														// Check for errors in USER RATE
			form_alert( 1, "[0][Selected rate is out of|allowable range][ Cancel ]" );
			return FALSE;
		}
	}
	period = 1000000000 / user_rate;

	graf_mouse( HOURGLASS, 0L );

	if( ( fhandle = Fopen( raw_file, 0 ) ) >= 0 ) {
		Fseek( offset , fhandle, SEEK_SET );												// locate our offset

		if( bits == BIT16 ) {												// load 16 bit data
			if( samp->SampLength = ( info.size - offset ) / 2 ) {
				if(samp->FirstSamp=(short *)calloc( samp->SampLength , sizeof(short) )){
					Fread( fhandle, SIZ(samp->SampLength), samp->FirstSamp);
				}
				else { 
					form_alert( 1, ALERT_MEMORY );
					init_sample_parameters( samp );
				}
			}
		}
		else if( bits == BIT12 ) {
			if( bit12_buff_size = info.size - offset ) {
				if( bit12_buff =(char *)calloc( bit12_buff_size, sizeof(char) )){
					Fread( fhandle, bit12_buff_size, bit12_buff );

					/***********/

					free( bit12_buff );
				}
				else form_alert( 1, ALERT_MEMORY );
			}
		}
		else if( bits == BIT8 ) {											// load 8 bit data
			if( samp->SampLength = info.size - offset ) {
				if(samp->FirstSamp=(short *)calloc( samp->SampLength , sizeof(short) )){
					S_data = samp->FirstSamp;
					while( read = Fread( fhandle, 512, buffer ) ) {
						while( next < read )
							*S_data++ = buffer[ next++ ] << 8;
						next = 0;
					}
				}
				else { 
					form_alert( 1, ALERT_MEMORY );
					init_sample_parameters( samp );
				}
			}
		}

		if( type == UNSIGNED ) {											// if UNSIGNED, convert it
			num = samp->SampLength;
			S_data = samp->FirstSamp;
			while( num-- )*S_data++ -= 0x8000;
		}
		else if( type == SHIFTED ) {
			num = samp->SampLength;
			S_data = samp->FirstSamp;
			while( num-- )*S_data++ <<= 4;
		}
		samp->SampRate = user_rate;
		samp->SampPeriod = period;

		if( Fclose( fhandle ) != 0 ) form_alert( 1, "[0][Error in closing file after read][ OK ]" );  
	}
	else form_alert( 1, "[0][Error opening file for reading][ OK ]" );

	force_complete_redraw();
	graf_mouse( ARROW, 0L );
	return TRUE;
}

short
hndl_fsel( char *path, char *file, char *suffix, char *msg )
{
	short button;

	strcpy( file, "" );
	strcat( path, "\\*" );
	strmfe( path, path, suffix );
	fsel_exinput( path, file, &button, msg );	/* Do file selector		*/
	*(strrchr( path, '\\' ) ) = 0;				/* Cut file off of path 	*/
	Dsetdrv( path[ 0 ] - 'A' );					/* Set drive and path		*/
	Dsetpath( path ); 							/* as selected 			*/

	return button;
}

void hndl_clipboard_to_sample( SD_HEADER *samp )
{
	char *cp;

	if( scrp_read( scrap_path ) && strlen( scrap_path ) ) {
		if( cp = strrchr( scrap_path, '\\' ) )
			*(cp + 1) = 0;
		else
			strcat( scrap_path, "\\" );
		strcat( scrap_path, "SCRAP.SD" );
		open_sd(  samp, scrap_path );
	}
	else 
		form_alert( 1,"[1][No Clip File Available][OK]" );
}

void
hndl_sample_to_clipboard( SD_HEADER *samp )
{
	char *cp;

	if( scrp_read( scrap_path ) && strlen( scrap_path ) ) {
		if( cp = strrchr( scrap_path, '\\' ) )
			*(cp + 1) = 0;
		else
			strcat( scrap_path, "\\" );
	}
	else {
		if( Drvmap() & 0x04 )
			strcpy( scrap_path, "C:\\CLIPBRD" );
		else
			strcpy( scrap_path, "A:\\CLIPBRD" );
		Dcreate( scrap_path );
		strcat( scrap_path, "\\" );
		scrp_write( scrap_path );
	}
	scrp_clear( scrap_path );
	strcat( scrap_path, "SCRAP.SD" );
	save_sd( samp, scrap_path );
}

void scrp_clear( char *scrppath )
{
	short err;
	char scrpfile[200];
	
	strcpy( scrpfile, scrppath );
	strcat( scrpfile, "SCRAP.*" );
	
	err = Fsfirst( scrpfile, 2 );
	while( ! err ) {
		Fdelete( scrpfile );
		err = Fsfirst( scrpfile, 2 );
	}
}

void
save_raw( SD_HEADER *samp, char *file )
{
	short fhandle;

	if( !file ) {
		if( ! hndl_fsel( sd_path, sd_file, "RAW", "Save Raw Wave Data..." ) ) return;
		file = sd_file;
	}

	graf_mouse( HOURGLASS, 0L );
	if( ( fhandle = Fcreate( file, 0 ) ) >=0 ) {
		Fwrite(fhandle,samp->SampLength * sizeof(short),samp->FirstSamp);
		if( Fclose( fhandle ) != 0 ) form_alert( 1, "[1][Error closing file after save][ OK ]" ); 				
	}else form_alert( 1, "[1][Couldnt open file for writing][ OK ]" );
	graf_mouse( ARROW, 0L );
	return;
}