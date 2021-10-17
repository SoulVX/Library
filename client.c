#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "helpers.h"
#include "parson.h"

int main(int argc, char *argv[])
{
    char *cookie = NULL;
    char *jwt = NULL;
    char input[100];
    int sockfd;

    while(1) {
        scanf("%s", input);
        sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);
        if(strcmp(input, "register") == 0) {
            char username[100], password[100];
            printf("username=");
            scanf("%s", username);
            printf("password=");
            scanf("%s", password);
            if(strlen(username) == 0) {
                printf("Username can't be empty!\n");
            } else {
            register_user(sockfd, username, password, cookie);
            }
        } else if(strcmp(input, "login") == 0) {
            if(cookie == NULL) {
                char username[100], password[100];
                printf("username=");
                scanf("%s", username);
                printf("password=");
                scanf("%s", password);
                if(strlen(username) == 0) {
                    printf("Username can't be empty!\n");
                } else {
                    login_user(sockfd, username, password, &cookie);
                }
            } else {
                printf("You are already logged in!\n");
            }
        } else if(strcmp(input, "enter_library") == 0) {
            jwt = get_access(sockfd, cookie);
        } else if(strcmp(input, "get_books") == 0) {
            get_books(sockfd, cookie, jwt);
        } else if(strcmp(input, "get_book") == 0) {
            char id[100];
            printf("id=");
            scanf("%s", id);
            if(atoi(id) == 0 && id[0] != '0') {
                printf("ID is not a number!\n");
            } else {
                get_book(sockfd, cookie, jwt, atoi(id));
            }
        } else if(strcmp(input, "add_book") == 0) {
            char title[100], author[100], genre[100], publisher[100], page_count[10];
            printf("title=");
            scanf("%s", title);
            printf("author=");
            scanf("%s", author);
            printf("genre=");
            scanf("%s", genre);
            printf("publisher=");
            scanf("%s", publisher);
            printf("page_count=");
            scanf("%s", page_count);
            if(atoi(page_count) == 0 && page_count[0] != '0') {
                printf("page_count is not a number!\n");
            } else if(strlen(title) == 0) {
                printf("title field can't be empty\n");
            }
            else {
                add_book(sockfd, cookie, jwt, title, author, genre, atoi(page_count), publisher);
            }
        } else if(strcmp(input, "delete_book") == 0) {
            char id[100];
            printf("id=");
            scanf("%s", id);
            if(atoi(id) == 0 && id[0] != '0') {
                printf("ID is not a number!\n");
            } else {
                delete_book(sockfd, cookie, jwt, atoi(id));
            }
        } else if(strcmp(input, "logout") == 0) {
            logout_user(sockfd, &cookie, &jwt);
        } else if(strcmp(input, "exit") == 0) {
            break;
        } else {
            printf("Unknown command!\n");
        }
        close_connection(sockfd);
    }
    return 0;
}
