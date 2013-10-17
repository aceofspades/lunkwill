#include "log.h"

char *Error_level[]=  \
	{ "[  DEBUG  ]", \
	  "[  INFO   ]", \
	  "[ WARNING ]", \
	  "[  ERROR  ]", \
	  "[  FATAL  ]" };

int Log_level=0;
FILE *Logfile=NULL;

int init_logger(char *LOGFILE, int log_lev)
{
	if((Logfile = fopen(LOGFILE, "a")) == NULL){
		fprintf(stderr, "%s\tCould not open logfile. Error reporting to stderr only\n", Error_level[2]);
		return 1;
	}
	
	Log_level=log_lev;
	sighndlr_add(close_log, Logfile);
	
	return 0;
}

int logprint(char *message, int error_level, int print_stderr)
{	
	if(error_level<Log_level) return 1;

	struct tm *ti;
	time_t time_s;
	char t_buf[128];

	time(&time_s);
	ti = localtime(&time_s);

	strftime(t_buf, 128, "%d. %b %Y %H:%M:%S", ti);

	if(Logfile==NULL)
	{
		fprintf(stderr, "%s\t%s\t%s\n", "[ UNINITIALIZED ]", t_buf, message);		
		return 1;
	}

	if(print_stderr)
	{
		fprintf(stderr, "%s\t%s\t%s\n", Error_level[error_level], t_buf, message);
	}

	fprintf(Logfile, "%s\t%s\t%s\n", Error_level[error_level], t_buf, message);
	
	
	return 0;
}

void *close_log(void *a)
{
	fclose(a);
	return NULL;
}
