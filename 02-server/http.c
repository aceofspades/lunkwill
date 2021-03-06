#include "http.h"

int send_file(char **buffer, char *file_path)
{
	FILE *file;
	int file_size;

	/** \brief Content-Type for supported MIME types */
	const static char *content_types[] =
	{
		"application/octet-stream",
		"text/plain",
		"text/html",
		"text/javascript",
		"text/css",
		"image/png",
		"image/x-icon",
		"image/svg+xml"
	};

	if((file = fopen(file_path, "r")) == NULL)
	{
		log_write("Could not open requested file", LOG_ERR);
		return -1;
	}

	// This possibly fixes the TOCTOU problem
	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	if(file_size <= 0)
	{
		log_write("Could not read requested file", LOG_ERR);
		fclose(file);

		return -1;
	}

	*buffer=(char *)calloc(BUF_SIZE+file_size,1);

	sprintf(*buffer, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %u\r\nConnection: close\r\nPragma: no-cache\r\nCache-Control: no-store\r\n\r\n", content_types[get_mime(file_path)], file_size);
	int header_len=strlen(*buffer);

	int read_ret = fread((*buffer+strlen(*buffer)), 1, file_size, file);

	if(read_ret != file_size)
	{
		fclose(file);
		return -1;
	}

	file_size+=header_len;

	fclose(file);

	return file_size;
}

int get_mime(char *file_path)
{
	char *ptr = strrchr(file_path, '.');
	if(ptr == NULL)
		return 0;
	ptr++;

	if(strcasecmp(ptr, "txt") == 0) return 1;
	if(strcasecmp(ptr, "html") == 0 || strcasecmp(ptr, "htm") == 0)	return 2;
	if(strcasecmp(ptr, "js") == 0) return 3;
	if(strcasecmp(ptr, "css") == 0)	return 4;
	if(strcasecmp(ptr, "png") == 0)	return 5;
	if(strcasecmp(ptr, "ico") == 0)	return 6;
	if(strcasecmp(ptr, "svg") == 0)	return 7;

	return 0;
}

int send_string(char **buffer, char *string)
{
	*buffer=malloc(strlen(string)+BUF_SIZE);
	sprintf(*buffer, "HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %u\r\nConnection: close\r\nPragma: no-cache\r\nCache-Control: no-store\r\n\r\n%s", (unsigned int)strlen(string), string);
	return strlen(*buffer);
}

int send_raw(char **buffer, char *string)
{
	*buffer=malloc(strlen(string)+BUF_SIZE);
	sprintf(*buffer, "%s", string);
	return strlen(*buffer);
}
