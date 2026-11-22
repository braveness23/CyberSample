/* Host-side test of Cut/Copy (src/CCP.C) together with the sample-slot helpers from src/MISC.C.
   The functions under test are extracted verbatim by run.sh; everything the Atari build gets from
   GEM/DEFS.H is replaced by a minimal stand-in below.  Run under ASan so use-after-free, double
   free and leaks fail the test. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	short *FirstSamp;
	long SampLength, EditStart, EditEnd, HScaleFact, LeftEdge;
	long Loop1Start, Loop1End, Loop2Start, Loop2End;
	char Loop1Type, Loop2Type;
	char FileName[ 64 ];
} SD_HEADER;

#define TRUE 1
#define FALSE 0
#define NPROMPT 0
#define PROMPT 1
#define NUM_SMPL 9
#define PASTE_BUF 8
#define SIZ(a) (a*sizeof(short))
static char ALERT_CUT_ALL[] = "cut all", ALERT_NOEDIT[] = "no edit", ALERT_MEMORY[] = "memory";
static int alerts;
static short form_alert( short d, char *s ) { (void)d; (void)s; alerts++; return 1; }
static void force_complete_redraw( void ) {}

static SD_HEADER default_sd_header = { NULL, 0, 0, 0, 1, 0, -1, -1, -1, -1, 1, 1, "" };
static SD_HEADER s[ NUM_SMPL ], us[ NUM_SMPL ], *p = &s[ PASTE_BUF ], *u;
static short edit_flag = TRUE;

/* allocation failure injection: fail the Nth malloc from now (0 = never) */
static int fail_at, malloc_calls;
static void *test_malloc( size_t n ) { if( fail_at && ++malloc_calls == fail_at ) return NULL; return malloc( n ); }
#define malloc test_malloc

long sample_length( SD_HEADER *samp );
long edit_length( SD_HEADER *samp );
short *off_addr( SD_HEADER *samp, long offset );
short new_sample( SD_HEADER *samp, short warn );
void init_sample_parameters( SD_HEADER *samp );
void reset_filename( SD_HEADER *samp ), reset_view_params( SD_HEADER *samp ), reset_edit_params( SD_HEADER *samp );
void reset_loop1_params( SD_HEADER *samp ), reset_loop2_params( SD_HEADER *samp );
void hndl_cut( SD_HEADER *samp );
void hndl_copy( SD_HEADER *samp );

#include "ccp_extract.h"

static int fails;
#define CK(c) do{ if(!(c)){ printf("FAIL line %d: %s\n",__LINE__,#c); fails++; } }while(0)

static void load( SD_HEADER *h, int n, long es, long ee )
{
	int i;
	new_sample( h, NPROMPT );
	h->FirstSamp = malloc( n * sizeof(short) );
	for( i = 0; i < n; i++ ) h->FirstSamp[ i ] = i + 1;
	h->SampLength = n; h->EditStart = es; h->EditEnd = ee;
}
static int same( SD_HEADER *h, const short *want, int n )
{
	return h->SampLength == n && ( n == 0 || !memcmp( h->FirstSamp, want, n * sizeof(short) ) );
}
static void undo_swap( SD_HEADER *t ) { SD_HEADER tmp = *t; *t = *u; *u = tmp; }	/* what MN_UNDO does */
static void clear_all( void ) { int i; for( i = 0; i < NUM_SMPL; i++ ) { new_sample( &s[ i ], NPROMPT ); new_sample( &us[ i ], NPROMPT ); } }

int main( void )
{
	const short orig[ 10 ] = { 1,2,3,4,5,6,7,8,9,10 };
	const short sel[ 3 ] = { 3,4,5 }, rest[ 7 ] = { 1,2,6,7,8,9,10 };
	int i;
	setvbuf( stdout, NULL, _IONBF, 0 );		/* LeakSanitizer exits without flushing stdio */
	for( i = 0; i < NUM_SMPL; i++ ) { s[ i ] = default_sd_header; us[ i ] = default_sd_header; }

	/* Copy from an ordinary slot */
	u = &us[ 3 ]; load( &s[ 3 ], 10, 2, 5 );
	hndl_copy( &s[ 3 ] );
	CK( same( p, sel, 3 ) ); CK( same( &s[ 3 ], orig, 10 ) );

	/* Copy while the paste buffer IS the current sample: selection becomes the buffer, no use-after-free */
	u = &us[ 8 ]; load( p, 10, 2, 5 );
	hndl_copy( p );
	CK( same( p, sel, 3 ) ); CK( p->FirstSamp != NULL );

	/* Cut from an ordinary slot: remainder stays, piece goes to the paste buffer, original kept for Undo */
	clear_all(); u = &us[ 3 ]; load( &s[ 3 ], 10, 2, 5 );
	hndl_cut( &s[ 3 ] );
	CK( same( &s[ 3 ], rest, 7 ) ); CK( same( p, sel, 3 ) ); CK( same( u, orig, 10 ) );
	undo_swap( &s[ 3 ] ); CK( same( &s[ 3 ], orig, 10 ) );

	/* Cut while the paste buffer is the current sample: just deletes the selection, Undo restores it */
	clear_all(); u = &us[ 8 ]; load( p, 10, 2, 5 );
	hndl_cut( p );
	CK( same( p, rest, 7 ) ); CK( same( u, orig, 10 ) ); CK( p->FirstSamp != u->FirstSamp );
	undo_swap( p ); CK( same( p, orig, 10 ) );

	/* Cut when the SECOND allocation fails: sample restored intact, nothing leaked */
	clear_all(); u = &us[ 3 ]; load( &s[ 3 ], 10, 2, 5 ); load( p, 4, 0, 1 );
	fail_at = 2; malloc_calls = 0; alerts = 0;
	hndl_cut( &s[ 3 ] ); fail_at = 0;
	CK( alerts == 1 ); CK( same( &s[ 3 ], orig, 10 ) );
	/* Copy when the allocation fails: the old paste buffer survives */
	clear_all(); u = &us[ 3 ]; load( &s[ 3 ], 10, 2, 5 ); load( p, 4, 0, 1 );
	fail_at = 1; malloc_calls = 0;
	hndl_copy( &s[ 3 ] ); fail_at = 0;
	{ const short oldp[ 4 ] = { 1,2,3,4 }; CK( same( p, oldp, 4 ) ); }

	clear_all();
	printf( fails ? "%d FAILED\n" : "all passed\n", fails );
	return fails != 0;
}
