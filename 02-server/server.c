#include "server.h"
#include "worker.h"

int Exit_Server;

int start_worker(int max_num_threads, int fd_ro, int fd_wr)
{
	struct serverwork _sw=
	{
		.lock_send=PTHREAD_MUTEX_INITIALIZER,
		.send_fd=fd_wr,
		.lock_count=PTHREAD_MUTEX_INITIALIZER,
		.thread_count=0,
		.jobs=NULL
	};

	struct serverwork *sw=&_sw;
	pthread_t thread;
		
	while(!Exit_Server)
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
		
		pthread_mutex_lock( &sw->lock_count );
			fifo_push(&sw->jobs,buffer);

			if(sw->thread_count<max_num_threads)
			{
				int i;
				sw->thread_count++;
				if((i=pthread_create( &thread, NULL, workerthread, sw))!=0)
				{
					sw->thread_count--;					
				}
				else
				{
					pthread_detach(thread);
				}
			}
		pthread_mutex_unlock( &sw->lock_count );
	}
	
	int i=1;
	do
	{
		usleep(2000);
		pthread_mutex_lock( &sw->lock_count );
			i=sw->thread_count;
		pthread_mutex_unlock( &sw->lock_count );		
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
		return 1;
	}


	if((listen(server_sock, listen_queue)) < 0){
		return 1;
	}
	

	setvbuf(stdin,NULL,_IONBF,0);

	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	FD_SET(server_sock, &master);
	FD_SET(0, &master);
	FD_SET(fd_ro, &master);
	fdmax = (server_sock>fd_wr)?server_sock:fd_wr;
	
	while(!Exit_Server)
	{
		read_fds = master;
		if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
		{
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
					}
				}
				else if(i==fd_ro)
				{
					struct pipe_rxtx buffer;
					if(read(fd_ro, &buffer, sizeof(struct pipe_rxtx))<=0)
					{
						return 1;
					}

					buffer.data=malloc(buffer.size+2);
					if(read(fd_ro, buffer.data, buffer.size)<=0)
					{
						nfree(buffer.data);
						return 1;
					}

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
					if(strbegin(buf, "quit")==0)
					{
						return 0;
					}
				}
				else
				{
					struct pipe_rxtx todo;
					todo.data=calloc(1, BUF_SIZE);
					if((todo.size = recv(i, todo.data, BUF_SIZE, 0)) <= 0)
					{
						close(i);
					}
					else
					{
						todo.fd=i;
						if(write(fd_wr, &todo, sizeof(struct pipe_rxtx))==-1)
						{
							nfree(todo.data);
							return -1;
						}
						if(write(fd_wr, todo.data, todo.size)==-1)
						{
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