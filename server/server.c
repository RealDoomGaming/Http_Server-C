#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT 8080
#define BACKLOG 10

// a struct for our server
typedef struct server {
  int listen_fd;
} server_t;

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

  return 0;
}

int server_accept(server_t *server) {}

int main() {
  // main method
  int err = 0;           // error variable
  server_t server = {0}; // init the server struct

  // listen for a connection
  err = server_listen(&server);
  // if err then return and stop
  if (err != 0) {
    printf("Failed to listen on adress 0.0.0.0:%d\n", PORT);
    return err;
  }

  // if no error then enter and infinite loop
  for (;;) {
    // where we try to accept incoming connections
    err = server_accept(&server);
    // else if there is an error while accepting we return out of the inifite
    // loop and the programm closes
    if (err) {
      printf("Failed accepting connection\n");
      return err;
    }
  }

  return 0;
}
