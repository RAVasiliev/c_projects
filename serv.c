#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#define LISTEN_QLEN 32
#define INBUFSIZE 1024

int main_num = 0;

char msgfin[] = "Server has been correctly finished\n";
char msgup[] = "Uped\n";
char msgdown[] = "Downed\n";
char msghello[] = "Hello!\n";
char msgporterr[] = "Please, give me port more than 1023\n";
char msglongbuferr[] = "Line too long!\n";
char msgcmderr[] = "Wrong command\n";

struct session
{
	int fd;
	char buf[INBUFSIZE];
	int buf_used;
};

void send_msg(struct session *s, const char *str)
{
	write(s->fd, str, strlen(str));
}

void reverse(char s[])
{
    int i, j;
    char c;
    for (i = 0, j = strlen(s)-1; i<j; i++, j--){
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}
char *intochar(int n)
{
	int i, sign, num = n, l = 0;
	char *s;
	if(num == 0)
	{
		s = malloc(sizeof(char)*(3));
		s[0] = '0';
		s[1] = '\n';
		s[2] = '\0';
		return s;
	}
	while(num != 0){
		num /= 10;
		l++;
	}
	if ((sign = n) < 0)
	{ 
		n = -n;  
		l++;  
	} 
	s = malloc(sizeof(char)*(l+2));
	i = 0;
	do {       
		s[i++] = n % 10 + '0';  
	} while ((n /= 10) > 0);   
	if (sign < 0)
		s[i++] = '-';
	reverse(s);
	s[l] = '\n';
	s[l+1] = '\0';
	return s;
}
int cmpstr(char *str1,char *str2)
{
	int tmp = 1;
	while(*str1++ && *str2++)
	{
		if(*str1 != *str2)
		{
			tmp = 0;
			break;
		}
	}
	return tmp;
}

struct session *make_new_session(int fd, struct sockaddr_in *from)
{
	struct session *s = malloc(sizeof(*s));
	s->fd = fd;
	s->buf_used = 0;
	send_msg(s, msghello);
	return s;
}

static void delete_session(struct session **sarr, int sd)
{
	shutdown(sd,SHUT_RDWR);
	close(sd);
	sarr[sd]->fd = -1;
	free(sarr[sd]);
	sarr[sd] = NULL;
}
void session_status(struct session *s, char *line)
{
	char *num = NULL;
	if(cmpstr(line, "up"))
	{
		main_num++;
		send_msg(s, msgup);
		return;
	}
	if(cmpstr(line, "down"))
	{
		main_num--;
		send_msg(s, msgdown);
		return;
	}
	
	if(cmpstr(line, "show"))
	{
		num = intochar(main_num);
		send_msg(s, num);
		return;
	}	
	send_msg(s, msgcmderr);
}
void check_newline(struct session *s)
{
	int pos = -1;
	int i;
	char *line;
	for(i = 0; i < s->buf_used; i++)
	{
		if(s->buf[i] == '\n')
		{
			pos = i;
			break;
		}
	}
	if(pos == -1)
		return;
	line = malloc(pos + 1);
//	printf("Pos: %d\n", pos);
	memcpy(line, s->buf, pos);
	line[pos] = 0;
	memmove(s->buf, s->buf+pos, pos+1);
	s->buf_used -= (pos+1);
	if(line[pos-1] == '\r')
		line[pos-1] = 0;
	session_status(s, line);
}

int session_read(struct session *s)
{
	int rc, bufp = s->buf_used;
	rc = read(s->fd, s->buf + bufp, INBUFSIZE - bufp);
	if(rc <= 0)
	{
//		perror("read error");
		return 0;
	}
	s -> buf_used += rc;
	check_newline(s);
	if(s -> buf_used >= INBUFSIZE)
	{
		send_msg(s, msglongbuferr);
		perror("bufer overflow");
		return 0;
	}
	return 1;
}

int connecting(int *ls, int argc, char **argv)
{
	int opt, port;
	struct sockaddr_in addr;
	char *endptr;
	port = strtol(argv[1], &endptr, 10); /*from string to long */
	
	if(port < 1024)
	{
		write(0, msgporterr, sizeof(msgporterr));
    		return 1;
	}

	(*ls) = socket(AF_INET, SOCK_STREAM, 0);/*ls - discr of socket*/
	if((*ls) == -1) 
	{
		perror("socket");
		return 1;
	}
	opt = 1;
	setsockopt((*ls), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	/*free port after using*/
	addr.sin_family = AF_INET;	/*connect socket with adress*/
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	if(-1 == bind((*ls), (struct sockaddr*) &addr, sizeof(addr))) 
	{
		perror("bind");
		return 1;
	}
	
	if (-1 == listen((*ls), 5))	/*listening state(waiting for clients)*/
	{
		perror("listen");
		return 1;
	}
	return 0;

}

int server_start(int *ls)
{	
	struct session **main_struct;
	fd_set readfds;
	int  sr,  maxfd, ssr;
	int i;
	int el_count;
	main_struct = malloc(LISTEN_QLEN * sizeof(struct session *));
	write(0, msghello, sizeof(msghello));
	for(;;)		/* ========MAIN LOOP==========*/ 
	{
		FD_ZERO(&readfds);        
		FD_SET((*ls), &readfds);
		maxfd = (*ls);
		for(i = 0; i < LISTEN_QLEN; i++) 
		{
			if(main_struct[i]) 
			{
				FD_SET(i, &readfds);
				if(i > maxfd)
					maxfd = i;
			}
		}
		sr = select(maxfd+1, &readfds, NULL, NULL, NULL);
		if(sr == -1) 
		{
			perror("select");
			return 1;
		}
		if(FD_ISSET((*ls), &readfds))
		{
			int sd, j;
    			struct sockaddr_in addr;
			socklen_t len = sizeof(addr);
			sd = accept((*ls), (struct sockaddr*) &addr, &len);
			if(sd == -1)
			{
				perror("accept");
				continue;
			}
			if(sd >= el_count) 
			{ 
        			int newlen = el_count;
        			while(sd >= newlen)
            				newlen += INBUFSIZE;
        			main_struct = realloc(main_struct, newlen * \
						sizeof(struct session *));
        			for(j = INBUFSIZE; j < newlen; j++)
            				main_struct[j] = NULL;
        			el_count = newlen;
			}
			printf("Client %d started\n", sd);
			main_struct[sd] = make_new_session(sd, &addr);
		}
		for(i = 0; i < el_count; i++) 
		{
			if(main_struct[i] && FD_ISSET(i, &readfds)) 
			{
				ssr = session_read(main_struct[i]);
				if(!ssr)
				{
					delete_session(main_struct, i);
					printf("Client %d finished\n", i);
				}
			}
		}
	}
	return 0;

}

int main(int argc, char **argv)
{
	int ls;
	if(argc != 2)
	{
		printf("Please, write in form: ./serv <port>\n");
		return 1;
	}

	if (connecting((&ls), argc, argv))
		return 1;
	return	server_start(&ls);
}
