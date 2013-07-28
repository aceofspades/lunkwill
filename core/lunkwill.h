#ifndef __LUNKWILL_H__
#define __LUNKWILL_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <netinet/in.h>


#include <pthread.h>   
#include <getopt.h>
#include <libconfig.h>

#include "../modules/modules.h"

#define nfree(a) free(a);a=NULL;


#define BUF_SIZE 1024
#define HTTP_404 "HTTP/1.0 404 Not Found\nContent-Type:text/html\n\n"
#define HTTP_404_MESS "<html><body><h1>FILE NOT FOUND</h1></body></html>"



static struct{
	void *(*module[62])(void *);
    config_t config;
	void *sighndlr;
} session;


int start_server();

void init_sighndlr();
void sighndlr_add(void *(*func)(void *), void *param);
void sighndlr_remove(void *(*func)(void *), void *param);


int load_config(char *config_file_name);


#endif