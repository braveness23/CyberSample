/* RUNCMD.C - run a list of TOS programs from a command file (CI helper for Hatari).
 *
 * Hatari's --auto can start one program but cannot pass it arguments.  RUNCMD is that
 * one program: it reads C:\BUILD.CMD and Pexec()s each line ("program args..."), echoing
 * progress to the console (host sees it via hatari --conout 2) and finally writes
 * C:\DONE.TXT so the host knows the batch finished.  Lines starting with # are comments.
 * Lines of the form  set NAME=value  build the environment passed to every program
 * (Lattice's driver finds its compiler passes via PATH, and uses INCLUDE / LIB).
 */
#include <osbind.h>
#include <string.h>
#include <stdio.h>

static char buf[4096];
static char env[1024];
static short elen = 0;

int
main( void )
{
	short fh;
	long n, rc, bad = 0;
	char *p, *line, *args;
	char cmd[130];
	short len, alen;

	fh = Fopen( "C:\\BUILD.CMD", 0 );
	if( fh < 0 ) {
		Cconws( "RUNCMD: cannot open C:\\BUILD.CMD\r\n" );
		bad = 1;
	}
	else {
		n = Fread( fh, sizeof( buf ) - 1, buf );
		Fclose( fh );
		buf[ n > 0 ? n : 0 ] = 0;

		for( p = buf; *p; ) {
			line = p;
			while( *p && *p != '\n' ) p++;
			if( *p ) *p++ = 0;

			len = strlen( line );
			while( len && ( line[ len - 1 ] == '\r' || line[ len - 1 ] == ' ' ) ) line[ --len ] = 0;
			while( *line == ' ' || *line == '\t' ) line++;
			if( !*line || *line == '#' ) continue;

			if( !strncmp( line, "set ", 4 ) ) {			/* environment variable */
				short l = strlen( line + 4 );
				if( elen + l + 2 < sizeof( env ) ) {
					strcpy( env + elen, line + 4 );
					elen += l + 1;
					env[ elen ] = 0;					/* double NUL terminator */
				}
				continue;
			}

			args = line;
			while( *args && *args != ' ' ) args++;
			if( *args ) *args++ = 0;

			alen = strlen( args );
			if( alen > 126 ) alen = 126;
			cmd[ 0 ] = (char)alen;
			memcpy( cmd + 1, args, alen );
			cmd[ alen + 1 ] = 0;

			printf( "RUNCMD> %s %s\r\n", line, args );
			rc = Pexec( 0, line, cmd, env );
			printf( "RUNCMD< exit %ld\r\n", rc );
			if( rc != 0 ) bad++;
		}
	}

	fh = Fcreate( "C:\\DONE.TXT", 0 );
	if( fh >= 0 ) {
		Fwrite( fh, bad ? 5L : 3L, bad ? "FAIL\n" : "OK\n" );
		Fclose( fh );
	}
	return (int)bad;
}
