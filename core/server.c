#include "lunkwill.h"
#include "server.h"
#include "tools.h"
#include "worker.h"
#include "fifo.h"

pthread_mutex_t lock_send=PTHREAD_MUTEX_INITIALIZER;
int send_fd=0;

pthread_mutex_t lock_count=PTHREAD_MUTEX_INITIALIZER;
int thread_count=0;

int exit_server;

struct _fifo *jobs=NULL;

int start_worker(int max_num_threads, int fd_ro, int fd_wr)
{
	pthread_t thread;
	send_fd=fd_wr;
		
	while(!exit_server)
	{
		struct pipe_rxtx *buffer=calloc(1,sizeof(struct pipe_rxtx));

		if(read(fd_ro, buffer, sizeof(struct pipe_rxtx))<=0)
		{
			nfree(buffer);
			break;
		}
		
		buffer->data=calloc(1,buffer->size+2);
		if(read(fd_ro, buffer->data, buffer->size)<=0)
		{
			nfree(buffer->data);
			nfree(buffer);
			break;
		}

		dbgprintf("Socket:%d Size:%d\n", buffer->fd, buffer->size);		
		
		pthread_mutex_lock( &lock_count );
			fifo_push(&jobs,buffer);

			if(thread_count<max_num_threads)
			{
				int i;
				thread_count++;
				dbgprintf("New Thread -> Total active:%d\n", thread_count);		
				if((i=pthread_create( &thread, NULL, workerthread, NULL))!=0)
				{
					dbgprintf("Failed to create Thread:%d %s\n", i, strerror(errno));		
					thread_count--;					
				}
				else
				{
					pthread_detach(thread);
				}
			}
		pthread_mutex_unlock( &lock_count );
	}
	
	int i=1;
	do
	{
		usleep(2000);
		pthread_mutex_lock( &lock_count );
			i=thread_count;
		pthread_mutex_unlock( &lock_count );		
	}while(i!=0);
	
	return 0;
}

int start_server(int port, int listen_queue, int timeout, int fd_ro, int fd_wr)
{
	struct sockaddr_in server_addr, client_addr;
	unsigned int server_sock, client_sock, addr_len;
	int fdmax,i;
 
	fd_set master;
	fd_set read_fds;
	
	int optval = 1;
	struct timeval tv;

	tv.tv_sec = timeout;
	tv.tv_usec = 0;


	//Set socket options
	server_sock = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(&(server_addr.sin_zero), '\0', 8);

	setsockopt(server_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
	setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));


	if(bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
		dbgprintf("Error binding to port %d\n", port);
		return 1;
	}


	if((listen(server_sock, listen_queue)) < 0){
		dbgprintf("Error listening on port %d\n",port);
		return 1;
	}
	

	setvbuf(stdin,NULL,_IONBF,0);

	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	FD_SET(server_sock, &master);
	FD_SET(0, &master);
	FD_SET(fd_ro, &master);
	fdmax = (server_sock>fd_wr)?server_sock:fd_wr;
	
	while(!exit_server)
	{
		read_fds = master;
		if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
		{
			dbgprintf("Error select failed with errno: %d\n", errno);
			return -1;
		}
		
		for(i = 0; i <= fdmax; i++)
		{
			if(FD_ISSET(i, &read_fds))
			{ 
				if(i == server_sock)
				{
					addr_len = sizeof(client_addr);
					if((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, (socklen_t*)&addr_len)) != -1)
					{
						FD_SET(client_sock, &master);
						if(client_sock > fdmax) fdmax = client_sock;
						dbgprintf("New connection from %s\n", inet_ntoa(client_addr.sin_addr));
					}
				}
				else if(i==fd_ro)
				{
					struct pipe_rxtx buffer;
					if(read(fd_ro, &buffer, sizeof(struct pipe_rxtx))<=0)
					{
						return 1;
					}

					dbgprintf("Read %d bytes from pipe\n", buffer.size);

					buffer.data=malloc(buffer.size+2);
					if(read(fd_ro, buffer.data, buffer.size)<=0)
					{
						nfree(buffer.data);
						dbgprintf("Pipe %d is broken\n", fd_ro);
						return 1;
					}
					
					dbgprintf("Forward pipe data to socket %d\n", buffer.fd);

					if((send(buffer.fd, buffer.data, buffer.size, 0)) <= 0)
					{
						close(buffer.fd);
					}

					close(buffer.fd);
					nfree(buffer.data);
				}
				else if(i==0)
				{
					char buf[11]={0};
					if(fgets(buf,10,stdin)==NULL)return 1;

					dbgprintf("%s\n",buf);
					if(strbegin(buf, "quit")==0)
					{
						dbgprintf("%s\n",buf);
						return 0;
					}
				}
				else
				{
					struct pipe_rxtx todo;
					todo.data=calloc(1, BUF_SIZE);
					if((todo.size = recv(i, todo.data, BUF_SIZE, 0)) <= 0)
					{
						dbgprintf("Client %d closed connection\n",i);
						close(i);
					}
					else
					{
						todo.fd=i;
						if(write(fd_wr, &todo, sizeof(struct pipe_rxtx))==-1)
						{
							nfree(todo.data);
							dbgprintf("Pipe %d is broken\n", fd_wr);
							return -1;
						}
						if(write(fd_wr, todo.data, todo.size)==-1)
						{
							dbgprintf("Pipe %d is broken\n", fd_wr);
							nfree(todo.data);
							return -1;
						}
					}
					FD_CLR(i, &master);
					nfree(todo.data);
				}
			}
		}
	}
	close(server_sock);
	return 1;
}
