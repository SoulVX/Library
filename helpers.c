#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "buffer.h"
#include "parson.h"

#define HEADER_TERMINATOR "\r\n\r\n"
#define HEADER_TERMINATOR_SIZE (sizeof(HEADER_TERMINATOR) - 1)
#define CONTENT_LENGTH "Content-Length: "
#define CONTENT_LENGTH_SIZE (sizeof(CONTENT_LENGTH) - 1)

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int open_connection(char *host_ip, int portno, int ip_type, int socket_type, int flag)
{
    struct sockaddr_in serv_addr;
    int sockfd = socket(ip_type, socket_type, flag);
    if (sockfd < 0)
        error("ERROR opening socket");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = ip_type;
    serv_addr.sin_port = htons(portno);
    inet_aton(host_ip, &serv_addr.sin_addr);

    /* connect the socket */
    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    return sockfd;
}

void close_connection(int sockfd)
{
    close(sockfd);
}

void send_to_server(int sockfd, char *message)
{
    int bytes, sent = 0;
    int total = strlen(message);

    do
    {
        bytes = write(sockfd, message + sent, total - sent);
        if (bytes < 0) {
            error("ERROR writing message to socket");
        }

        if (bytes == 0) {
            break;
        }

        sent += bytes;
    } while (sent < total);
}

char *receive_from_server(int sockfd)
{
    char response[BUFLEN];
    buffer buffer = buffer_init();
    int header_end = 0;
    int content_length = 0;

    do {
        int bytes = read(sockfd, response, BUFLEN);

        if (bytes < 0){
            error("ERROR reading response from socket");
        }

        if (bytes == 0) {
            break;
        }

        buffer_add(&buffer, response, (size_t) bytes);
        
        header_end = buffer_find(&buffer, HEADER_TERMINATOR, HEADER_TERMINATOR_SIZE);

        if (header_end >= 0) {
            header_end += HEADER_TERMINATOR_SIZE;
            
            int content_length_start = buffer_find_insensitive(&buffer, CONTENT_LENGTH, CONTENT_LENGTH_SIZE);
            
            if (content_length_start < 0) {
                continue;           
            }

            content_length_start += CONTENT_LENGTH_SIZE;
            content_length = strtol(buffer.data + content_length_start, NULL, 10);
            break;
        }
    } while (1);
    size_t total = content_length + (size_t) header_end;
    
    while (buffer.size < total) {
        int bytes = read(sockfd, response, BUFLEN);

        if (bytes < 0) {
            error("ERROR reading response from socket");
        }

        if (bytes == 0) {
            break;
        }

        buffer_add(&buffer, response, (size_t) bytes);
    }
    buffer_add(&buffer, "", 1);
    return buffer.data;
}

char *basic_extract_json_response(char *str)
{
    return strstr(str, "{\"");
}

char *compute_request(char *action, char *url, char *cookie, char* auth,
            char *data, char *content_type) {
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    sprintf(line, "%s %s HTTP/1.1", action, url);
    compute_message(message, line);

    sprintf(line, "Host: 34.118.48.238");
    compute_message(message, line);

    if(auth != NULL) {
        sprintf(line, "Authorization: Bearer %s", auth);
        compute_message(message, line);
    }

    if(data != NULL) {
        sprintf(line, "Content-Type: %s", content_type);
        compute_message(message, line);
        sprintf(line, "Content-Length: %ld", strlen(data));
        compute_message(message, line);
    }

    if(cookie != NULL ) {
        sprintf(line, "Cookie: %s;", cookie);
        compute_message(message, line);
    }

    compute_message(message, "");

    if(data != NULL) {
        compute_message(message, data);
    }

    return message;
}

void compute_message(char *message, const char *line)
{
    strcat(message, line);
    strcat(message, "\r\n");
}

const char *get_simple_json(char *json_response, char* field) {
    JSON_Value *root_value = json_parse_string(json_response);
    JSON_Object *obj = json_value_get_object(root_value);
    return json_object_get_string(obj, field);
}

int show_response(char *response, char *right_response) {
    if(strlen(response) == 0)
        return 0;
    char *response_dup = strdup(response);
    char *tok = strtok(response_dup, " ");
    tok = strtok(NULL, " ");
    if(atoi(tok) == 200 || atoi(tok) == 201) {
        printf("%s\n", right_response);
        return 1;
    }
    else if(basic_extract_json_response(response) != NULL) {
        printf("%s\n", get_simple_json(basic_extract_json_response(response), "error"));
        return 0;
    }
    else {
        printf("Unknown error!\n");
        return 0;
    }
}

void register_user(int socket, char *username, char *password, char *cookie) {
    char *data;
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "username", username);
    json_object_set_string(root_object, "password", password);
    data = json_serialize_to_string_pretty(root_value);

    char *message = compute_request("POST", "/api/v1/tema/auth/register", cookie, NULL, data, "application/json");
    send_to_server(socket, message);
    char *response = receive_from_server(socket);
    show_response(response, "Registered successfully!");
}

void login_user(int socket, char *username, char *password, char **cookie) {
    char *data;
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "username", username);
    json_object_set_string(root_object, "password", password);
    data = json_serialize_to_string_pretty(root_value);

    char *message = compute_request("POST", "/api/v1/tema/auth/login", NULL, NULL, data, "application/json");
    send_to_server(socket, message);
    char *response = receive_from_server(socket);
    if(show_response(response, "You are now logged in!")) {
        char *cookie_line = strstr(response, "Set-Cookie: ");
        cookie_line = cookie_line + 12;
        char *tok = strtok(cookie_line, "\r");
        *cookie = calloc(strlen(tok), sizeof(char));
        memcpy(*cookie, cookie_line, strlen(tok));
    }
}

char *get_access(int socket, char *cookie) {
    char *message = compute_request("GET", "/api/v1/tema/library/access", cookie, NULL, NULL, NULL);
    send_to_server(socket, message);
    char *response = receive_from_server(socket);
    if(show_response(response, "Access authorised!"))
        return (char*)get_simple_json(basic_extract_json_response(response), "token");
    else
        return NULL;
}

void get_books(int socket, char *cookie, char *auth) {
    char *message = compute_request("GET", "/api/v1/tema/library/books", cookie, auth, NULL, NULL);
    send_to_server(socket, message);
    char *response = receive_from_server(socket);
    if(show_response(response, "Books in the library:")) {
        JSON_Value *root_value = json_parse_string(strstr(response, "["));
        JSON_Array *books = json_value_get_array(root_value);
        JSON_Object *book;
        for(int i = 0; i < json_array_get_count(books); i++) {
            book = json_array_get_object(books, i);
            printf("%s (ID : %.0f)\n", json_object_get_string(book, "title"), json_object_get_number(book, "id"));
        }
    }
}

void get_book(int socket, char *cookie, char *auth, int id) {
    char path[50];
    sprintf(path, "/api/v1/tema/library/books/%d", id);
    char *message = compute_request("GET", path, cookie, auth, NULL, NULL);
    send_to_server(socket, message);
    char *response = receive_from_server(socket);
    JSON_Value *root_value = json_parse_string(basic_extract_json_response(response));
    JSON_Object *obj = json_value_get_object(root_value);
    printf("title=%s\nauthor=%s\ngenre=%s\npage_count=%d\npublisher=%s\n", json_object_get_string(obj, "title"),
            json_object_get_string(obj, "author"), json_object_get_string(obj, "genre"),
            (int) json_object_get_number(obj, "title"), json_object_get_string(obj, "publisher"));
}

void add_book(int socket, char *cookie, char *auth, char *title, char *author, char *genre, 
                                                        int page_count, char *publisher) {
    char *data;
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "title", title);
    json_object_set_string(root_object, "author", author);
    json_object_set_string(root_object, "genre", genre);
    json_object_set_number(root_object, "page_count", page_count);
    json_object_set_string(root_object, "publisher", publisher);
    data = json_serialize_to_string_pretty(root_value);

    char *message = compute_request("POST", "/api/v1/tema/library/books", cookie, auth, data, "application/json");
    send_to_server(socket, message);
    char *response = receive_from_server(socket);
    show_response(response, "Book added to the library!");
}

void delete_book(int socket, char *cookie, char *auth, int id) {
    char path[50];
    sprintf(path, "/api/v1/tema/library/books/%d", id);
    char *message = compute_request("DELETE", path, cookie, auth, NULL, NULL);
    send_to_server(socket, message);
    char *response = receive_from_server(socket);
    show_response(response, "Book deleted from the library!");
}

void logout_user(int socket, char **cookie, char **auth) {
    char *message = compute_request("GET", "/api/v1/tema/auth/logout", *cookie, *auth, NULL, NULL);
    send_to_server(socket, message);
    char *response = receive_from_server(socket);
    if(show_response(response, "You logged out!")) {
        *cookie = NULL;
        *auth = NULL;
    }
}