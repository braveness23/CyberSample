/* DEFS.H */


#define BELL Bconout( 2, 7 )

#define FALSE 0
#define TRUE 1

#define M_NEITHER		0
#define M_LEFT			1
#define M_RIGHT			2
#define M_BOTH			M_LEFT|M_RIGHT

#define SINGLE_CLICK	1
#define DOUBLE_CLICK	2

#define EA_DOWN_6DB		0x1000
#define EA_UP_6DB		0x1001
#define EA_ZERO 		0x1002
#define EA_MIRAGE		0x1003
#define EA_INVERT		0x1004
#define EA_NORMALIZE	0x1005
#define EA_REVERSE		0x1006
#define EA_FADE_IN		0x1008
#define	EA_FADE_OUT		0x1009

#define TIME_SAMP 0
#define TIME_MSEC 1
#define TIME_SEC  2


#define M_DOWN			1
#define M_UP			0


#define SIZ(a) (a*sizeof(short))

#define ARROWUP			0x4800
#define ARROWDOWN		0x5000
#define ARROWLEFT		0x4d00
#define ARROWRIGHT		0x4b00
#define UNDO			0x6100
#define CLR_HOME		0x4700
#define CNLT_Z			0x0000
#define CNTL_N			0x310e
#define CNTL_O			0x180f
#define CNTL_S			0x1f13
#define CNTL_Q			0x1011
#define CNTL_X			0x2d18
#define CNTL_V			0x2f16
#define CNTL_C			0x2e03
#define CNTL_A			0x1e01
#define CNTL_D			0x2004
#define CNTL_G			0x2207
#define K_R				0x1372
#define K_N				0x316e
#define N_PLUS			0x4e2b
#define N_MINUS			0x4a2d
#define F1				0x3b00
#define F2				0x3c00
#define F3				0x3d00
#define F4				0x3e00
#define F5				0x3f00
#define F6				0x4000
#define F7				0x4100
#define F8				0x4200
#define F9				0x4300
#define F10				0x4400
#define F11				0x5400
#define F12				0x5500
#define F13				0x5600
#define F14				0x5700
#define F15				0x5800
#define F16				0x5900
#define F17				0x5a00
#define F18				0x5b00
#define F19				0x5c00
#define F20				0x5d00
#define K1				0x0231
#define K2				0x0332
#define K3				0x0433
#define K4				0x0534
#define K5				0x0635
#define K6				0x0736
#define K7				0x0837
#define	K8				0x0938
#define OPN_PARENTH		0x6328
#define CLS_PARENTH		0x6429


#define M_QUIT 		0
#define M_MODE_1	1
#define M_PEXEC		2

#define NPROMPT 0
#define PROMPT 1

typedef struct {
	short hnd;
	short type;
	char title[ 65 ];
	char info[ 40 ];
	OBJECT *tree;
	GRECT curr, prev, full;
} WINDOW;

#define FIRST_SMPL 0
#define LAST_SMPL 7
#define PASTE_BUF 8
#define UNDO_BUF 9
#define NUM_SMPL 10


typedef struct{
	char			Free;
	char			aa;			// unused
	long			Position;
	char			Name[32];
	char			bb[2];		// unused
}SD_MARKER;							


typedef struct{						

//			name					   byte offset  	default value

	short	header_size;			// 0				1136
	short	Version; 				// 2				11
	short	Preview; 				// 4				0
	char	aa[40]; 				// unused			/0
	long	VCenter;				// 46				0
	long	LeftEdge;				// 50				0
	char	bb[2];					// unused			/0
	short	VScaleFact;				// 56				-256
	long	HScaleFact;				// 58				1
	long	VTickUnits;				// 62				327
	char	VTickType;				// 66				04
	char	cc;						// unused			/0
	char	VScaleStr[32];			// 68				"%Scale"
	char	dd[2];					// unused			/0
	long	HTickUnits;				// 102				1
	char	HTickType;				// 106				0
	char	ee;						// unused			/0
	char	HScaleStr[32];			// 108				""
	char	ff[2];					// unused			/0
	short	VScrUpdate; 			// 142				0
	char	gg[8];					// unused			/0
	short	*FirstSamp;				// 152				NULL
	char	hh[12];					// unused			/0
	short	CtlWidthTop;			// 168				0
	short	CtlWidthLeft;			// 170				0
	short	CtlWidthBottom;			// 172				15
	short	CtlWidthRight;			// 174				15
	char	ii[8];					// unused			/0
	long	SampLength;				// 184				0
	char	jj[66];					// unused			/0
	char	FileName[64];			// 254				""
	char	kk[10];					// unused			/0
	char	NoBackup;				// 328				0
	char	ModeType;				// 329				0
	long	EditStart;				// 330				0
	long	EditEnd;				// 334				0
	short	EditExtendSide;			// 338				0
	long	CursorPos;				// 340				0
	char	ll[4];					// unused			/0
	SD_MARKER mark[ 10 ];			// 348				1, /0, 0, "", /0
									// 388
									// 428
									// 468
									// 508
									// 548
									// 588
									// 628
									// 668
									// 708
	long	MarkerOffset;			// 748				0
	long	Loop1Start;				// 752				-1
	long	Loop1End;				// 756				-1
	char	ZeroLineOn;				// 760				0
	char	CursorOn;				// 761				0
	char	ScalesOn;				// 762				0
	char	mm;						// unused			\0
	char	Comment[255];			// 764				""
	long	SampRate;				// 1020				44100
	long	SampPeriod;				// 1024				22
	short	SampSize;				// 1028				16
	char	CodeType[32];			// 1030				"Linear"
	char	nn[262];				// unused			\0
	long	Loop2Start;				// 1324				-1
	long	Loop2End;				// 1328				-1
	char	Loop1Type;				// 1332				1
	char	Loop2Type;				// 1333				1
	char	oo[2];					// unused			\0
}SD_HEADER;

typedef struct {
			char 	sysex,
					id_no,
					channel,
					sub_id,
					s_no_l,
					s_no_h,
					s_format,
					s_rate_l,
					s_rate_m,
					s_rate_h,
					s_length_l,
					s_length_m,
					s_length_h,
					l_start_l,
					l_start_m,
					l_start_h,
					l_end_l,
					l_end_m,
					l_end_h,
					l_mode,
					eox;
}STND_HEADER;

typedef struct {
			char 	sysex,
					id_no,
					channel,
					sub_id,
					packet_no,
					data[120],
					checksum,
					eox;
}STND_PACKET;

typedef struct {
	char topmode;
	char buttons;
	char xparam;
	char yparam;
}MOUSE_PARAM;

#define midi_in() bios(2,3)
#define midi_out(a) bios( 3, 3, a )
#define midi_stat() bios( 1, 3 )
#define FLUSH_MIDI_BUFFER midibuf->ibufhd = 0; midibuf->ibuftl = 0;

#ifdef CYBERSAMPLE

#include <dos.h>
#include <osbind.h>
#include "cybrwerk.h"
#include "cybr.i"
#include "draw.i"
#include "ccp.i"
#include "edit.i"
#include "file.i"
#include "get.i"
#include "misc.i"
#include "cybrwerk.i"

void midithru( void );

#ifdef MAINMODUAL

short in[11] = { 0,1,1,1,1,1,1,1,1,1,2 }, out[57];
short gl_apid, handle, dummy;
short wchar, hchar, wbox, hbox;
short start_drv;
char start_path[128];
char pexec_path[200];
char pexec_file[13] = ".PRG";
short main_flag = M_MODE_1;
short quickdraw = 1;
short timeline = 1;
short timeline_format = TOG_SAMP;
short loop_markers = 1;
short edit_flag = 0, sample_flag = 0, paste_flag = 0, undo_flag = 0;
short smp_no = 0;
SD_HEADER s[NUM_SMPL], *t, *p, *u;
OBJECT *disp;
WINDOW g = { -1, MOVER|NAME|CLOSER|FULLER|SIZER, "CyberSample v.160", "", NULL };
GRECT desk;
short left_mode = MODE_VIEW;
SD_HEADER default_sd_header = { 1336, 11, 0, "", 0, 0, "", -256, 1, 327, 04, 0, "%Scale",
								"", 1, 0, 0, "", "", 0, "", 0, "", 0, 0, 15, 15, "", 0, "", "",
								"", 0, 0, 0, 0, 0, 0, "", 
								1, 0, 0, "", "", 1, 0, 0, "", "", 1, 0, 0, "", "", 1, 0, 0, "", "",
								1, 0, 0, "", "", 1, 0, 0, "", "", 1, 0, 0, "", "", 1, 0, 0, "", "",
								1, 0, 0, "", "", 1, 0, 0, "", "",
								0, -1, -1, 0, 0, 0, 0, "", 44100, 22, 16, "Linear", "", -1, -1, 1, 1, "" };
char sd_path[ 128 ];
char sd_suffix[ 5 ]="SD";
char sd_file[ 16 ];
char raw_path[ 128 ];
char raw_suffix[ 5 ]="*";
char raw_file[ 16 ];
char del_path[ 128 ];
char del_suffix[ 5 ]="*";
char del_file[ 16 ];
char req_string[8] = { 0xf0, 0x7e, 0x00, 0x03, 0x00, 0x00, 0xf7, 0x00 };
char msg_string[7] = { 0xf0, 0x7e, 0x00, 0x00, 0x00, 0xf7, 0x00 };

IOREC *midibuf;
char device = 0, sample = 0;
STND_HEADER default_stnd_header = { 0xf0, 0x7e, 0x00, 0x01, 0x00, 0x00, 0x0c, 0x60, 0x54, 0x03,
									0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xf7 };
MOUSE param = { 0, 0, 1, 1 };
char scrap_path[200];
char scrap_file_name[13] = "SCRAP.SD";
KBDVBASE *base;
void *midivect;
short midithru_flag = 1;
short note_channel[3] = { 0, 0, 0 };
short note_number[3] =  { 60, 60, 60 };
short note_velocity[3] = { 127, 127, 127 };


#else

extern short in[11], out[57];
extern short gl_apid, handle, dummy;
extern short wchar, hchar, wbox, hbox;
extern short start_drv;
extern char start_path[128];
extern char pexec_path[200];
extern char pexec_file[13];
extern short main_flag;
extern short quickdraw;
extern short timeline;
extern short timeline_format;
extern short loop_markers;
extern short edit_flag, sample_flag, paste_flag, undo_flag;
extern short smp_no;
extern SD_HEADER s[NUM_SMPL], *t, *p, *u;
extern OBJECT *disp;
extern WINDOW g;
extern GRECT desk;
extern short left_mode;
extern SD_HEADER default_sd_header;
extern char sd_path[ 128 ];
extern char sd_suffix[ 5 ];
extern char sd_file[ 16 ];
extern char raw_path[ 128 ];
extern char raw_suffix[ 5 ];
extern char raw_file[ 16 ];
extern char del_path[ 128 ];
extern char del_suffix[ 5 ];
extern char del_file[ 16 ];
extern char req_string[8];
extern char msg_string[7];
extern IOREC *midibuf;
extern char device, sample;
extern STND_HEADER default_stnd_header;
extern MOUSE param;
extern char scrap_path[200];
extern char scrap_file_name[13];
extern KBDVBASE *base;
extern void *midivect;
extern short midithru_flag;
extern short note_channel[3];
extern short note_number[3];
extern short note_velocity[3];
#endif	/* end of #ifdef MAINMODUAL - #else ... 	*/

#endif	/* end of #ifdef CYBERSAMPLE 				*/


/* --- Lattice C 5.60 compatibility shims (added 2026-11-19; original in DEFS.H.orig-1993) --- */
/* old-style GRECT element-pointer macro, absent from 5.60 AES.H */
#ifndef ELTX
#define ELTX(r) &(r)->g_x, &(r)->g_y, &(r)->g_w, &(r)->g_h
#endif
#ifndef PTRX
#define PTRX(r) &(*(r))->g_x, &(*(r))->g_y, &(*(r))->g_w, &(*(r))->g_h
#endif
/* old-style accessor: text pointer of a TEDINFO object in a tree */
#ifndef TED_PTEXT
#define TED_PTEXT(tree,obj) (((TEDINFO *)((tree)[obj].ob_spec))->te_ptext)
#endif
/* MENU renamed CYBMENU in CYBR.C/CYBRWERK.C/CYBRWERK.H (clashes with 5.60 AES.H typedef) */
