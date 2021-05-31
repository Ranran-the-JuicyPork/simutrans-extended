/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef SYSLOG
#include <syslog.h>
#endif

#include "log.h"
#include "../simdebug.h"
#include "../sys/simsys.h"


#ifdef MAKEOBJ
//debuglevel is global variable
#define dr_fopen fopen
#else
#ifdef NETTOOL
#define debuglevel (0)
#define dr_fopen fopen

#else // (not) NETTOOL
#define debuglevel (env_t::verbose_debug)

// for display ...
#include "../gui/messagebox.h"
#include "../display/simgraph.h"
#include "../gui/simwin.h"

#include "../dataobj/environment.h"
#endif // NETTOOL
#endif // MAKEOBJ

/**
 * writes a debug message into the log.
 */
void log_t::debug(const char *who, const char *format, ...)
{
	if(log_debug  &&  debuglevel>=4) {
		va_list argptr;
		va_start(argptr, format);

		if( log ) {                           /* only log when a log */
			fprintf(log ,"Debug: %s:\t",who); /* is already open */
			vfprintf(log, format, argptr);
			fprintf(log,"\n");

			if( force_flush ) {
				fflush(log);
			}
		}
		va_end(argptr);

		va_start(argptr, format);
		if( tee ) {                           /* only log when a log */
			fprintf(tee, "Debug: %s:\t",who); /* is already open */
			vfprintf(tee, format, argptr);
			fprintf(tee,"\n");
		}
		va_end(argptr);

#ifdef SYSLOG
		va_start( argptr, format );
		if (  syslog  ) {
			// Replace with dynamic memory allocation
			char buffer[4096];
			sprintf( buffer, "Debug: %s\t%s", who, format );
			vsyslog( LOG_DEBUG, buffer, argptr );
		}
		va_end( argptr );
#endif
	}
}


/**
 * writes a message into the log.
 */
void log_t::message(const char *who, const char *format, ...)
{
	if(debuglevel>=3) {
		va_list argptr;
		va_start(argptr, format);

		if( log ) {                             /* only log when a log */
			fprintf(log ,"Message: %s:\t",who); /* is already open */
			vfprintf(log, format, argptr);
			fprintf(log,"\n");

			if( force_flush ) {
				fflush(log);
			}
		}
		va_end(argptr);

		va_start(argptr, format);
		if( tee ) {                             /* only log when a log */
			fprintf(tee, "Message: %s:\t",who); /* is already open */
			vfprintf(tee, format, argptr);
			fprintf(tee,"\n");
		}
		va_end(argptr);

#ifdef SYSLOG
		va_start( argptr, format );
		if (  syslog  ) {
			// Replace with dynamic memory allocation
			char buffer[4096];
			sprintf( buffer, "Message: %s\t%s", who, format );
			vsyslog( LOG_INFO, buffer, argptr );
		}
		va_end( argptr );
#endif
	}
}


/**
 * writes a warning into the log.
 */
void log_t::warning(const char *who, const char *format, ...)
{
	if(debuglevel>=2) {
		va_list argptr;
		va_start(argptr, format);

		if( log ) {                             /* only log when a log */
			fprintf(log ,"Warning: %s:\t",who); /* is already open */
			vfprintf(log, format, argptr);
			fprintf(log,"\n");

			if( force_flush ) {
				fflush(log);
			}
		}
		va_end(argptr);

		va_start(argptr, format);
		if( tee ) {                             /* only log when a log */
			fprintf(tee, "Warning: %s:\t",who); /* is already open */
			vfprintf(tee, format, argptr);
			fprintf(tee,"\n");
		}
		va_end(argptr);

#ifdef SYSLOG
		va_start( argptr, format );
		if (  syslog  ) {
			// Replace with dynamic memory allocation
			char buffer[4096];
			sprintf( buffer, "Warning: %s\t%s", who, format );
			vsyslog( LOG_WARNING, buffer, argptr );
		}
		va_end( argptr );
#endif
	}
}


/**
 * writes an error into the log.
 */
void log_t::error(const char *who, const char *format, ...)
{
	if(debuglevel>=1) {
		va_list argptr;
		va_start(argptr, format);

		if( log ) {                           /* only log when a log */
			fprintf(log ,"ERROR: %s:\t",who); /* is already open */
			vfprintf(log, format, argptr);
			fprintf(log,"\n");

			if( force_flush ) {
				fflush(log);
			}

			fprintf(log ,"For help with this error or to file a bug report please see the Simutrans forum:\n");
			fprintf(log ,"https://forum.simutrans.com\n");
		}
		va_end(argptr);

		va_start(argptr, format);
		if( tee ) {                           /* only log when a log */
			fprintf(tee, "ERROR: %s:\t",who); /* is already open */
			vfprintf(tee, format, argptr);
			fprintf(tee,"\n");

			fprintf(tee ,"For help with this error or to file a bug report please see the Simutrans forum:\n");
			fprintf(tee ,"https://forum.simutrans.com\n");
		}
		va_end(argptr);

#ifdef SYSLOG
		va_start( argptr, format );
		if (  syslog  ) {

			// Replace with dynamic memory allocation
			char buffer[4096];
			sprintf( buffer, "ERROR: %s\t%s", who, format );
			vsyslog( LOG_ERR, buffer, argptr );
		}
		va_end( argptr );
#endif
	}
}



/**
 * writes a warning into the log.
 */
void log_t::doubled(const char *what, const char *name )
{
	if(debuglevel>=2) {

		if( log ) {                             /* only log when a log */
			fprintf(log ,"Warning: object %s::%s is overlaid!\n",what,name); /* is already open */
			if( force_flush ) {
				fflush(log);
			}
		}

		if( tee ) {
			fprintf(tee, "Warning: object %s::%s is overlaid!\n",what,name);
		}

#ifdef SYSLOG
		if(  syslog  ) {
			::syslog( LOG_WARNING, "Warning: object %s::%s is overlaid!", what, name );
		}
#endif
	}
	doublettes.append( (std::string)what+"::"+name+"<br/>" );
}



/**
 * writes an error into the log, aborts the program.
 */
void log_t::fatal(const char *who, const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);

	static char formatbuffer[512];
	sprintf( formatbuffer,
		"FATAL ERROR: %s - %s\n"
		"Aborting program execution ...\n"
		"\n"
		"For help with this error or to file a bug report please see the Simutrans forum at\n"
		"https://forum.simutrans.com\n",
		who, format );

	static char buffer[8192];
	int n = vsprintf( buffer, formatbuffer, argptr );

	if(  log  ) {
		fputs( buffer, log );
		if (  force_flush  ) {
			fflush( log );
		}
	}

	if(  tee  &&  log!=tee  ) {
		fputs( buffer, tee );
	}

#ifdef SYSLOG
	if (  syslog  ) {
		::syslog( LOG_ERR, buffer );
	}
#endif

	if (  tee == NULL  &&  log == NULL  &&  tag == NULL  ) {
		fputs( buffer, stderr );
	}

	va_end(argptr);

#if defined MAKEOBJ
	(void)n;
	exit(1);
#elif defined NETTOOL
	// no display available
	(void)n;
	puts( buffer );
#else
	// not MAKEOBJ/NETTOOL
#  ifdef MSG_LEVEL
	int old_level = env_t::verbose_debug;
#  endif
	env_t::verbose_debug = 0; // no more window concerning messages
	if(is_display_init()) {
		// show notification
		destroy_all_win( true );

		strcpy( buffer+n+1, "PRESS ANY KEY\n" );
		fatal_news* sel = new fatal_news(buffer);

		scr_coord xy( display_get_width()/2 - sel->get_windowsize().w/2, display_get_height()/2 - sel->get_windowsize().h/2 );
		event_t ev;

		create_win( xy.x, xy.y, sel, w_info, magic_none );

		while(win_is_top(sel)) {
			// do not move, do not close it!
			dr_sleep(50);
			dr_prepare_flush();
			sel->draw( xy, sel->get_windowsize() );
			dr_flush();
			display_poll_event(&ev);
			// main window resized
			check_pos_win(&ev);
			if(ev.ev_class==EVENT_KEYBOARD) {
				break;
			}
		}
	}
	else {
		// use OS means, if there
		dr_fatal_notify(buffer);
	}

#endif
	abort();
}



#ifdef NETTOOL
void log_t::vmessage(const char *, const char *, const char *, va_list )
{
}

#else

void log_t::vmessage(const char *what, const char *who, const char *format, va_list args )
{
	if(debuglevel>0) {
		va_list args2;
		va_copy(args2, args);
		if( log ) {                               /* only log when a log */
			fprintf(log ,"%s: %s:\t", what, who); /* is already open */
			vfprintf(log, format, args);
			fprintf(log,"\n");

			if( force_flush ) {
				fflush(log);
			}
		}
		if( tee ) {                              /* only log when a log */
			fprintf(tee,"%s: %s:\t", what, who); /* is already open */
			vfprintf(tee, format, args2);
			fprintf(tee,"\n");
		}
		va_end(args2);
	}
}

#endif


// create a logfile for log_debug=true
log_t::log_t( const char *logfilename, bool force_flush, bool log_debug, bool log_console, const char *greeting, const char* syslogtag ) :
	log(NULL),
	tee(NULL),
	force_flush(force_flush), // if true will always flush when an entry is written to the log
	log_debug(log_debug),
	tag(NULL),
	syslog(false)
{
	if(logfilename == NULL) {
		log = NULL;                       /* not a log */
		tee = NULL;
	}
	else if(strcmp(logfilename,"stdio") == 0) {
		log = stdout;
		tee = NULL;
	}
	else if(strcmp(logfilename,"stderr") == 0) {
		log = stderr;
		tee = NULL;
#ifdef SYSLOG
	}
	else if(  strcmp( logfilename, "syslog" ) == 0  ) {
		syslog = true;
		if (  syslogtag  ) {
			tag = syslogtag;
			// Set up syslog
			openlog( tag, LOG_PID, LOG_DAEMON );
		}
		log = NULL;
		tee = NULL;
#endif
	}
	else {
		log = dr_fopen(logfilename,"wb");

		if(log == NULL) {
			fprintf(stderr,"log_t::log_t: can't open file '%s' for writing\n", logfilename);
		}
		tee = stderr;
	}
	if (!log_console) {
	    tee = NULL;
	}

	if(  greeting  ) {
		if( log ) {
			fputs( greeting, log );
//			message("log_t::log_t","Starting logging to %s", logfilename);
		}
		if( tee ) {
			fputs( greeting, tee );
		}
#ifdef SYSLOG
		if (  syslog  ) {
			::syslog( LOG_NOTICE, greeting );
		}
#else
		(void)syslogtag;
#endif
	}
}


void log_t::close()
{
	message("log_t::~log_t","stop logging, closing log file");

	if( log ) {
		fclose(log);
		log = NULL;
	}
}


// close all logs during cleanup
log_t::~log_t()
{
	if( log ) {
		close();
	}
}
