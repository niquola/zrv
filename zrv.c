#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/in.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

char static notfound[] ="HTTP/1.1 404 OK\r\nContent-Length: 0\r\n\r\n";

int static  yes = 1;
int static no = 0;

/* typedef struct */
/* { */
/* 	int sock; */
/* 	struct sockaddr address; */
/* 	int addr_len; */
/* } connection_t; */

void * process(void * ptr)
{

  char inbuf[5000];
  char response[1000];
  int br = 0;
  int num = 0;
  char* token;
  char* rest;


  struct stat stat_buf;
  int src;
  char filename[1000];
  off_t offset;

	if (!ptr) pthread_exit(0);
	int conn = (int) ptr;

  printf("Connection %d; thread %ld", conn, pthread_self());

  while (1) {
    printf("\nloop %d\n", num);
    num++;
    br = recv(conn, &inbuf, 10000, 0);
    if(br <= 0) {
      printf("Finish\n");
      break;
    } else if (br > 0) {
      inbuf[br] = '\0';                      // NEW
      printf("\n\n%s", &inbuf[0]);
    }

    rest = &inbuf[0];
    token = strtok_r(rest, " ", &rest);
    printf("meth %s\n", token);
    token = strtok_r(rest, " /", &rest);
    printf("path %s\n", token);

    src = open(token, O_RDONLY);

    if(-1 == src)
      {
        printf("\n open() failed with error [%s]\n",strerror(errno));
        write(conn, &notfound[0], strlen(notfound));

      } else {

      fstat(src, &stat_buf);

      sprintf(&response[0], "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n" , stat_buf.st_size);

      setsockopt(conn, IPPROTO_TCP, TCP_CORK, &yes, sizeof(int));

      printf("Response [%ld]:\n%s\n", strlen(response), &response[0]);

      write(conn, &response[0], strlen(response));

      offset = 0;
      for (size_t size_to_send = stat_buf.st_size; size_to_send > 0; )
        {
          printf("Try to send %ld with off %ld\n",size_to_send, offset);
          ssize_t sent = sendfile(conn, src, &offset, size_to_send);
          if (sent < 0) {
                fprintf(stderr, "Warning: sendfile(3EXT) returned %ld " "(errno %d)\n", sent, errno);
                close(conn);
                close(src);
                return (void *) 0;
          } else if (sent == 0) {
            printf("Was sent 0 :(\n");
            break;
          } else {
            printf("Was sent %ld\n", sent);
            offset += sent;
            size_to_send -= sent;
          }

       }

      setsockopt(conn, IPPROTO_TCP, TCP_CORK, &no, sizeof(int));

      /* close socket and clean up */
      /* close(conn); */
      close(src);
      printf("Served\n");
    }
  }

  printf("Close conn\n");
  pthread_exit(0);
}


int main()
{
  pthread_t thread;

  int flags;
  int e;
  int sock = socket(AF_INET, SOCK_STREAM, 0);

  if (sock < 0) {
    err(1, "can't open socket");
  }

  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

  int port = 8080;
  struct sockaddr_in svr_addr =
    {
     .sin_family = AF_INET,
     .sin_addr.s_addr = INADDR_ANY,
     .sin_port = htons(port)
    };

  if (bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) == -1) {
    close(sock);
    err(1, "Can't bind");
  }

  e = listen(sock, SOMAXCONN);
  if (e < 0) {
    err(1,"Could not listen: %s\n", strerror(errno));
  }

  printf("Server started at 8080\n\n");

	int conn;
	struct sockaddr conn_address;
	int conn_addr_len;

  while (1) {
    conn = accept(sock, &conn_address, &conn_addr_len);

    if(conn <= 0) {
      perror("CANT ACCEPT");
    } else {
      pthread_create(&thread, 0, process, (void *)conn);
      pthread_detach(thread);
    }
  }
}
