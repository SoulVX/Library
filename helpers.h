#ifndef _HELPERS_
#define _HELPERS_

#define BUFLEN 4096
#define LINELEN 1000

// shows the current error
void error(const char *msg);

// adds a line to a string message
void compute_message(char *message, const char *line);

// opens a connection with server host_ip on port portno, returns a socket
int open_connection(char *host_ip, int portno, int ip_type, int socket_type, int flag);

// closes a server connection on socket sockfd
void close_connection(int sockfd);

// send a message to a server
void send_to_server(int sockfd, char *message);

// receives and returns the message from a server
char *receive_from_server(int sockfd);

// extracts and returns a JSON from a server response
char *basic_extract_json_response(char *str);

//Compose a request of specified action
char *compute_request(char *action, char *url, char *cookie, char* auth,
            char *data, char *content_type);

//Returns the field value of a simple JSON (error, token)
const char *get_simple_json(char *json_response, char* field);

//Checks if the request was succesfull or not, and prints the output
int show_response(char *response, char *right_response);

void register_user(int socket, char *username, char *password, char *cookie);

void login_user(int socket, char *username, char *password, char **cookie);

char *get_access(int socket, char *cookie);

void get_books(int socket, char *cookie, char *auth);

void get_book(int socket, char *cookie, char *auth, int id);

void add_book(int socket, char *cookie, char *auth, char *title, char *author, char *genre, 
                                                        int page_count, char *publisher);

void delete_book(int socket, char *cookie, char *auth, int id);

void logout_user(int socket, char **cookie, char **auth);

#endif
