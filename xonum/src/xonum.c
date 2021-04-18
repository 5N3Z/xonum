
/*
        link enumerater for the popular 
  http://x0.no/ terminal url shortening service

    can be used in tandem with 'grep' for
     information gathering of all sorts

        functional as of april 2020
               ~ alex snez
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define VERSION "1.0"
#define PORT "80"
#define REQTAIL " HTTP/1.1\r\nHost: x0.no\r\n\r\n"
#define BUFFER 2048

void req(char *index);
void handle(int sig);
char decr(char *s, unsigned int p);
void status(char *reason);
void loc(char *resp);

char *steps = "0123456789abcdefghijklmnopqrstuvwxyz";

int
main(int argc, char *argv[]) { // 1:start uri  2:depth

	if (argc != 3) {
		printf("\033[0;33m <i> usage: %s <starting uri> <depth>\n\033[0m", argv[0]);
		exit(1);
	}
    if (strlen(argv[1]) > 12)
        status("exceeded maximum uri length [12]");

    unsigned int c, depth = atoi(argv[2]);
    char *uri = argv[1];
    signal(SIGINT, handle);

    printf("\n\033[0;35m  [ xonum.c - by\033[0m\033[0;36m alex snez\033[0m\033[0;35m - version %s ]\n\033[0m", VERSION);
    printf("\n\033[0;33m <i> enumerating all url's %d steps below http://x0.no/%s...\033[0m\n", depth, uri);

    // string decrementing algorithm i came up with:
    // decrement outermost value of given string based on position in *steps,
    // while adjusting place values accordingly
    // some values hardcoded for readability
    for (c = 0; c < depth && strcmp(uri, "0") != 0; c++) {
        unsigned int pos = 1;
        
        if (decr(uri, pos) == '0') {
            uri[strlen(uri) - pos] = decr(uri, pos);
            req(uri);
            if (pos + 1 <= strlen(uri)) {
                pos++;
                if (uri[strlen(uri) - pos] != '0') { 
                    uri[strlen(uri) - pos] = decr(uri, pos);
                    uri[strlen(uri) - (pos - 1)] = 'z';
                }
                else {
                    uri[strlen(uri) - (pos - 1)] = 0x00;
                    uri[strlen(uri) - pos] = 'z';
                }
                pos--;
            }
        }
        else
            uri[strlen(uri) - pos] = decr(uri, pos);        
        
        req(uri);
    }
	return 0;
}

char
decr(char *s, unsigned int p) {
    return steps[(strlen(steps)-1) - (strlen(strchr(steps, (int)s[strlen(s)-p])))];
}

void // isolation of 'location' parameter in the http response
loc(char *resp) {
    char *l = "http";
    char url[200]; strncpy(url, strstr(resp, l), 199);
    for (int x = 0; x < strlen(url); x++) {
        if (url[x] == '\n') {
            url[x] = 0x00;
            break;
        }
    }
    if (strcmp(url, "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">") == 0)
        printf("\033[0;31m <!> no shortened url under this index\n\033[0m");
    else
        printf("\033[0;32m <url> \033[0m%s\n", url);
}

void // http request for uri, response stored in heap
req(char *index) {

    int sock;
    struct addrinfo info, *serv, *n;
    char *host = "x0.no";
    char *res = (char *) malloc(BUFFER);
        
    // request header
    char header[66] = "GET /";
    strncat(header, index, strlen(index));
    strcat(header, REQTAIL);

    memset(&info, 0, sizeof(info));
    info.ai_family = AF_UNSPEC;
    info.ai_socktype = SOCK_STREAM;

    if ((getaddrinfo(host, PORT, &info, &serv)) != 0)
        status("host information couldn't be retrieved");
    
    for (n = serv; n != NULL; n = n->ai_next) {
        if ((sock = socket(n->ai_family, n->ai_socktype, n->ai_protocol)) == -1)
            continue;
        if ((connect(sock, n->ai_addr, n->ai_addrlen)) == -1) {
            close(sock);
            continue;
        }
        break;
    }

    if (n == NULL) {
        freeaddrinfo(serv);
        free(res);
        status("connection to host couldn't be made");
    }

    if ((send(sock, header, 44, 0)) == -1)
        status("sending http request");

    if ((recv(sock, res, BUFFER-1, 0)) == -1)
        status("receiving http response");

   	loc(res);

    close(sock);
    freeaddrinfo(serv);
    free(res);
}

void // error handler
status(char *reason) {
    fprintf(stderr, "\033[1;31m <!> problem: %s\n\033[0m", reason);
    exit(-1);
}

void // signal handler
handle(int sig) {
    printf("\n\033[0;31m <!> caught interrupt signal \n\033[0m");
    exit(2);
}
