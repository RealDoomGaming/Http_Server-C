// #include <bits/pthreadtypes.h>
#include <malloc.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <threads.h>
#include <unistd.h>

#include "../Thpool/thpool.h"

#define PORT 8080
#define BACKLOG 10
#define BUFFER 1024

// a struct for our server
typedef struct server {
  int listen_fd;
} server_t;

// function for just sending plain text when the client has the method GET
int sendPlainText(int conn_fd) {
  // making a new error variable to store errors
  int err;
  printf("Writing to fd: %d\n", conn_fd);

  // now we need to send something with write
  // in the content needs to be the protocoll, the status code
  // the content type and then after that the actual text or whatever you
  // want to send
  char *content = "HTTP/1.1 200 OK\r\n"
                  "Content-Type: text/html\n"
                  "\r\n"
                  "Jukulumbrien\r\n";

  // size of our content
  size_t contentSize = strlen(content);

  // then we write it to the client with the client fd and the content and the
  // size of the content
  err = write(conn_fd, content, contentSize);

  // if error then we do the usual stuff
  if (err == -1) {
    perror("write");
    printf("failed to write to client\n");
    return err;
  }

  // if no error then we just return 0
  return 0;
}

char *getMethod(int conn_fd) {
  // reading client data
  // buffer is for future use in the read function and we use malloc to give it
  // the storage so stuff can be read into it over read
  char *buffer = malloc(BUFFER);

  // we also need a new error variable to store the error
  int err;

  // with read we get the data from the client with conn_fd since thats our
  // clients socket and then we just give buffer in which we write the stuff we
  // read and the buffer
  err = read(conn_fd, buffer, BUFFER);

  // if err then we just do our usual error handeling
  if (err <= 0) {
    perror("read");
    printf("failed reading from the client\n");
    return NULL;
  }

  // if no error then we print it
  printf("The client data was read and is: %s\n\n", buffer);

  // read in buffer until we find a space to get the method
  // first we get the buffer for the biggest method (Delete) + for the 0 pointer
  size_t bufferMethodsize = 8;
  // then we malloc the method
  char *method = malloc(bufferMethodsize);
  // we also need to give it a null determinator at the last idx of the method
  memset(method, 0, bufferMethodsize);

  // then we do the entire logic for finding the method
  // first we make a bool for if we have found the method
  bool foundFirstSpace = false;
  // then we have an idx for which char to get from the buffers
  int idx = 0;
  printf("%lu", strlen(buffer));
  // then we enter our while where we check if the bool is false and then check
  // if our idx is smaller then the length of the buffer
  while (!foundFirstSpace && idx < (int)strlen(buffer)) {
    // then we copy the character from the buffer into our method
    // and after that check if our current char is a space
    if (buffer[idx] == ' ') {
      foundFirstSpace = true;
      // we need to break here because else we would continue and save the space
      // in our method which we dont want
      break;
    }
    method[idx] = buffer[idx];
    printf("character at idx: %d %c\n", idx, buffer[idx]);
    // lastly we make our idx + 1 and then we are done with everything
    idx++;
  }

  return method;
}

// in server_listen we will listen for a connection
int server_listen(server_t *server) {
  // listenes for a connection
  // AF_INET says that we want IPv4 we could also use
  // something else if we wanted IPv4
  //
  // SOCK_STREAM just provides a sequenced, reliable, two-way, connection-based
  // byte stream meaning it is very good and u should use it
  int err = (server->listen_fd = socket(AF_INET, SOCK_STREAM, 0));

  if (err == -1) {
    perror("socket");
    printf("Failed to create socket endpoint\n");
    return err;
  }

  // now we need to bind the socket to an adress

  // sockaddr_in allows you to specify a 32-bit adress (IPv4 adress)
  // and a 16-bit number (the actuall port)
  struct sockaddr_in server_addr;

  // this just defines the adress family again, now its IPv4
  // but you could of course again use IPv6
  server_addr.sin_family = AF_INET;

  // sin addr is just the ip adress using 0.0.0.0
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  // and sin port is the port in network and our port is defined at the top
  // (8080)
  server_addr.sin_port = htons(PORT);

  // now we call bind to attach the socket to a specific address it is still
  // closed tho because its CLOSED we give the bind funtion the file descriptor
  // from the server struct then second we cast our server_addr struct to the
  // sockaddr struct and lastly we give it the size of the struct we casted
  // before
  err = bind(server->listen_fd, (struct sockaddr *)&server_addr,
             sizeof(server_addr));

  // if the binding fails we just give error and return the error
  if (err == -1) {
    perror("bind");
    printf("Failed to bind the socket to adress\n");
    return err;
  }

  // now we will have to use listen to mark the socket as
  // SO_ACCEPTION so that it can accept requests (socket also goes from the
  // CLOSED state to the LISTEN state) First we have again the fd from the
  // server Secondly we have the BACKLOG which is the queue size for how many
  // requests can line up we define it at the top
  err = listen(server->listen_fd, BACKLOG);

  // if we have an error we do the same like always
  if (err == -1) {
    perror("listen");
    printf("Failed to put socket into passive mode to listen\n");
    return err;
  }

  return err;
}

// server accept function is only to accept the actuall connection the other
// stuff like sending the http things and getting the method happens later in a
// seperate function
int server_accept(server_t *server) {
  // err variable and the connection file descripton
  int err = 0;
  int conn_fd;
  // also make the client_len for the accept later
  socklen_t client_len;
  // then make a struct the same with the server but just for the client
  struct sockaddr_in client_addr;

  // actually get the size of the client_addr struct we made
  client_len = sizeof(client_addr);

  // then we try to call accept with the listen_fd, and then we also give accept
  // our client_addr but cast it to the sockaddr struct lastly we give the
  // accept function the client length
  err = (conn_fd = accept(server->listen_fd, (struct sockaddr *)&client_addr,
                          &client_len));

  // if we had an error then we return and just print and give error back
  if (err == -1) {
    perror("accept");
    printf("failed accepting connection\n");
    return err;
  }

  // else we print Client Connected bc we connected succesfully
  printf("Client connected!\n");

  return conn_fd;
}

// this connection now is to handle all the stuff we send to the client and also
// where we get the method
void *handle_client(void *arg) {
  int conn_fd = *(int *)arg;
  // after getting our cnnectioon file descriptor we can free the argument
  free(arg);

  // for our errors we make a new variable
  int err;

  // firstly we have to get the method from our client
  char *method = getMethod(conn_fd);

  if (method == NULL) {
    perror("Getting Connection");
    printf("There was an error handeling the client stuff because we couldnt "
           "get the method\n");
    // before we return we also have to close the connection properly
    close(conn_fd);
    return NULL;
  }

  printf("Found method: %s\n", method);

  // then we handle the specific method we get (currently only GET)
  if (strcmp(method, "GET") == 0) {
    // and also send the plain text to the client
    err = sendPlainText(conn_fd);
    printf("sendPlainText returned %d\n", err);

    // here we dont need to return since only the client wont receive the thing
    // we send so its fine
    if (err < 0) {
      perror("Getting Method");
      printf("There was an error getting the clients method\n");
    }
  }

  // after that we can free our method
  free(method);

  // then after all that we have to shutdown the conneciton
  shutdown(conn_fd, SHUT_RDWR);
  // then we wait 2 seconds so the client can actually shutdown
  sleep(2);

  // then lastly we close the conneciton
  err = close(conn_fd);
  if (err < 0) {
    perror("Closing Connection");
    printf("There was an error closing the connection with the client\n");
    // we also dont need to return here since the programm is almost done
  }

  printf("Client Connection Closed\n");
  return NULL;
}

int main() {
  // main method
  int err = 0;           // error variable
  server_t server = {0}; // init the server struct

  // for better performance we also make a thread pool
  threadPool *thpool = threadPoolInit(8);

  // listen for a connection
  err = server_listen(&server);
  // if err then return and stop
  if (err != 0) {
    printf("Failed to listen on adress 0.0.0.0:%d\n", PORT);
    return err;
  }

  // if no error then enter and infinite loop
  for (;;) {
    // first we have to accept the client connection and get the client file
    // descriptor
    int client_fd = server_accept(&server);
    if (client_fd < 0) {
      perror("Accepting");
      printf("There was an error accepting the connection\n");
      // despite the error we can just continue to get other connections no need
      // to shutdown the entire server
      continue;
    }

    // then allocate the memory for the client_fd
    int *conn_fd_ptr = (int *)malloc(sizeof(int));
    *conn_fd_ptr = client_fd;

    // after that we do all the job stuff with our threads
    threadJob *job = threadJobInit(handle_client, conn_fd_ptr);
    addJob(thpool, job);
  }

  return 0;
}
