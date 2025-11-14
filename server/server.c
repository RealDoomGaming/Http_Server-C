#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT 8080

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
