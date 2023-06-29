#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#define PORT            "25"
#define DOMAIN          "krisplis.owlery.org"

#define BACKLOG_MAX     (10)
#define BUF_SIZE        4096
#define STREQU(a,b)     (strcmp(a, b) == 0)
#define USERNAME_MAXLEN 100
#define PASSWD_MAXLEN   100
#define LINE_LIMIT      500

// Linked list of ints container
struct int_ll {
        int d;
        struct int_ll *next;
};

// Overall server state
struct {
        struct int_ll *sockfds;
        int sockfd_max;
        char *domain;
        pthread_t thread; // Latest spawned thread
} state;

// Function prototypes
void init_socket(void);
void *handle_smtp (void *thread_arg);
void *get_in_addr(struct sockaddr *sa);

/*
 __  __    ___     ___    _  _   
|  \/  |  /   \   |_ _|  | \| |  
| |\/| |  | - |    | |   | .` |  
|_|__|_|  |_|_|   |___|  |_|\_|  
_|"""""|_|"""""|_|"""""|_|"""""| 
"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-' 
*/
int main (int argc, char *argv[]) {
        int rc, i, j;
        char strbuf[INET6_ADDRSTRLEN];

        // Init syslog with program prefix
        char *syslog_buf = (char*) malloc(1024);
        sprintf(syslog_buf, "%s", argv[0]);
        openlog(syslog_buf, LOG_PERROR | LOG_PID, LOG_USER);

        // This would be more useful as an argument
        state.domain = DOMAIN;

        // Open sockets to listen on for client connections
        init_socket();

        // Loop forever listening for connections and spawning
        // threads to handle each exchange via handle_smtp()
        while (1) {
                fd_set sockets;
                FD_ZERO(&sockets);
                struct int_ll *p;

                for (p = state.sockfds; p != NULL; p = p->next) {
                        FD_SET(p->d, &sockets);
                }

                // Wait forever for a connection on any of the bound sockets
                select (state.sockfd_max+1, &sockets, NULL, NULL, NULL);

                // Iterate through the sockets looking for one with a new connection
                for (p = state.sockfds; p != NULL; p = p->next) {
                        if (FD_ISSET(p->d, &sockets)) {
                                struct sockaddr_storage client_addr;
                                socklen_t sin_size = sizeof(client_addr);
                                int new_sock = accept (p->d, \
                                                (struct sockaddr*) &client_addr, &sin_size);
                                if (new_sock == -1) {
                                        syslog(LOG_ERR, "Accepting client connection failed");
                                        continue;
                                }

                                // Convert client IP to human-readable
                                void *client_ip = get_in_addr(\
                                                (struct sockaddr *)&client_addr);
                                inet_ntop(client_addr.ss_family, \
                                                client_ip, strbuf, sizeof(strbuf));
                                syslog(LOG_DEBUG, "Connection from %s", strbuf);

                                // Pack the socket file descriptor into dynamic mem
                                // to be passed to thread; it will free this when done.
                                int * thread_arg = (int*) malloc(sizeof(int));
                                *thread_arg = new_sock;

                                // Spawn new thread to handle SMTP exchange
                                pthread_create(&(state.thread), NULL, \
                                                handle_smtp, thread_arg);

                        }
                }
        } // end forever loop

        return 0;
}



/*
   ___     ___     ___    _  __    ___    _____  
  / __|   / _ \   / __|  | |/ /   | __|  |_   _| 
  \__ \  | (_) | | (__   | ' <    | _|     | |   
  |___/   \___/   \___|  |_|\_\   |___|   _|_|_  
_|"""""|_|"""""|_|"""""|_|"""""|_|"""""|_|"""""| 
"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-' 

Typically this would just be one IPv4 and one IPv6 socket
*/
void init_socket(void) {
        int rc, i, j, yes = 1;
        int sockfd;
        struct addrinfo hints, *hostinfo, *p;

        // Set up the hints indicating all of localhost's sockets
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        state.sockfds = NULL;
        state.sockfd_max = 0;

        rc = getaddrinfo(NULL, PORT, &hints, &hostinfo);
        if (rc != 0) {
                syslog(LOG_ERR, "Failed to get host addr info");
                exit(EXIT_FAILURE);
        }

        for (p=hostinfo; p != NULL; p = p->ai_next) {
                void *addr;
                char ipstr[INET6_ADDRSTRLEN];
                if (p->ai_family == AF_INET) {
                        addr = &((struct sockaddr_in*)p->ai_addr)->sin_addr;
                } else {
                        addr = &((struct sockaddr_in6*)p->ai_addr)->sin6_addr;
                }
                inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));

                sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
                if (sockfd == -1) {
                        syslog(LOG_NOTICE, "Failed to create IPv%d socket", \
                                        (p->ai_family == AF_INET) ? 4 : 6 );
                        continue;
                }

                setsockopt(sockfd, SOL_SOCKET, \
                                SO_REUSEADDR, &yes, sizeof(int));

                rc = bind(sockfd, p->ai_addr, p->ai_addrlen);
                if (rc == -1) {
                        close (sockfd);
                        syslog(LOG_NOTICE, "Failed to bind to IPv%d socket", \
                                        (p->ai_family == AF_INET) ? 4 : 6 );
                        continue;
                }

                rc = listen(sockfd, BACKLOG_MAX);
                if (rc == -1) {
                        syslog(LOG_NOTICE, "Failed to listen to IPv%d socket", \
                                        (p->ai_family == AF_INET) ? 4 : 6 );
                        exit(EXIT_FAILURE);
                }

                // Update highest fd value for select()
                (sockfd > state.sockfd_max) ? (state.sockfd_max = sockfd) : 1;

                // Add new socket to linked list of sockets to listen to
                struct int_ll *new_sockfd = malloc(sizeof(struct int_ll));
                new_sockfd->d = sockfd;
                new_sockfd->next = state.sockfds;
                state.sockfds = new_sockfd;
        }

        if (state.sockfds == NULL) {
                syslog(LOG_ERR, "Completely failed to bind to any sockets");
                exit(EXIT_FAILURE);
        }

        freeaddrinfo(hostinfo);

        return;
}

/*
   ___   __  __   _____     ___  
  / __| |  \/  | |_   _|   | _ \ 
  \__ \ | |\/| |   | |     |  _/ 
  |___/ |_|__|_|  _|_|_   _|_|_  
_|"""""|_|"""""|_|"""""|_| """ | 
"`-0-0-'"`-0-0-'"`-0-0-'"`-0-0-' 


This is typically spawned as a new thread for each exchange
to handle the actual SMTP conversation with each client.
*/
void *handle_smtp (void *thread_arg) {
        syslog(LOG_DEBUG, "Starting thread for socket #%d", *(int*)thread_arg);

        int rc, i, j;
        char buffer[BUF_SIZE], bufferout[BUF_SIZE];
        int buffer_offset = 0;
        buffer[BUF_SIZE-1] = '\0';

        // Login part
        int is_logged = 0;
        char mail_sender[USERNAME_MAXLEN], mail_recipient[USERNAME_MAXLEN];

        // This part is preferably some sort of database
        char mail_user[11] = "uzytkownik\0";
        char simple_passwd[6] = "haslo\0";

        // Mail contents
        char contents[(BUF_SIZE + 1)*LINE_LIMIT];
        int line_ctr = 0;


        // Unpack dynamic mem argument from main()
        int sockfd = *(int*)thread_arg;
        free(thread_arg);

        // Flag for being inside of DATA verb and login process
        // 0 - verb mode
        // 1 - DATA mode
        // 2 - mail mode
        // 3 - password mode
        int message_code = 0;

        sprintf(bufferout, "220 %s SMTP Server\r\n", state.domain);
        char *art = "\t\tI solemnly swear that I am up to no good\n\n\
                                                  ____\n\
                     Pulsatrix perspicillata   ,''    ''.\n\
                                              / `-.  .-' \\\n\
                                             /( (O))((O) )\n\
                                            /'-..-'/\\`-..|\n\
                                          ,'\\   `-.\\/.--'|\n\
                                        ,' ( \\           |\n\
                                      ,'( (   `._        |\n\
                                     /( (  ( ( | `-._ _,-;\n\
                                    /( (  ( ( (|     '  ;\n\
                                   / ((  (    /        /\n\
                                  //         /        /\n\
                                 //  / /  ,'        /\n\
                                // /    ,'         /\n\
                              //  / ,'          ;\n\
                             //_,-'          ;\n\
                            // /,,,,..-))-))\\    /|\n\
                              /; ; ;\\ `.  \\  \\  / |\n\
                             /; ; ; ;\\  \\.  . \\/./\n\
                            (; ; ;_,_,\\  .: \\   /\n\
                             `-'-'     | : . |:|\n\
                ascii.co.uk            |. | : .|\n";

        printf("%s", art);
        send(sockfd, art, strlen(art), 0);
        printf("%s", bufferout);
        send(sockfd, bufferout, strlen(bufferout), 0);

        while (1) {     
                fd_set sockset;
                struct timeval tv;
                        

                FD_ZERO(&sockset);
                FD_SET(sockfd, &sockset);
                tv.tv_sec = 120; // Some SMTP servers pause for ~15s per message
                tv.tv_usec = 0;

                // Wait tv timeout for the server to send anything.
                select(sockfd+1, &sockset, NULL, NULL, &tv);

                if (!FD_ISSET(sockfd, &sockset)) {
                        syslog(LOG_DEBUG, "%d: Socket timed out", sockfd);
                        break;
                }

                int buffer_left = BUF_SIZE - buffer_offset - 1;
                if (buffer_left == 0) {
                        syslog(LOG_DEBUG, "%d: Command line too long", sockfd);
                        sprintf(bufferout, "500 Too long\r\n");
                        printf("S%d: %s", sockfd, bufferout);
                        send(sockfd, bufferout, strlen(bufferout), 0);
                        buffer_offset = 0;
                        continue;
                }

                rc = recv(sockfd, buffer + buffer_offset, buffer_left, 0);
                if (rc == 0) {
                        syslog(LOG_DEBUG, "%d: Remote host closed socket", sockfd);
                        break;
                }
                if (rc == -1) {
                        syslog(LOG_DEBUG, "%d: Error on socket", sockfd);
                        break;
                }

                buffer_offset += rc;

                char *eol;

                // Only process one line of the received buffer at a time
                // If multiple lines were received in a single recv(), goto
                // back to here for each line
                //
processline:
                eol = strstr(buffer, "\r\n");
                if (eol == NULL) {
                        syslog(LOG_DEBUG, "%d: EOL is NULL", sockfd);
                }

                // Null terminate each line to be processed individually
                eol[0] = '\0';

                if (message_code == 0) { // Handle system verbs
                        printf("C%d: %s\n", sockfd, buffer);

                        // Replace all lower case letters so verbs are all caps. Except blank space and @
                        for (i=0; i<105; i++) {
                                if (islower(buffer[i]) && buffer[i] != 32 && buffer[i] != 64) {
                                        buffer[i] += 'A' - 'a';
                                }
                        }

                        char s_verb[5], login_verb[11], m_addr[USERNAME_MAXLEN + 1];

                        // Initial verb
                        for (i=0; i<4; i++){
                                s_verb[i] = buffer[i];
                        }

                        // AUTH LOGIN verb if present
                        for (i=0; i<10; i++){
                                login_verb[i] = buffer[i];
                        }

                        // Email addresses
                        for (i=0; i<USERNAME_MAXLEN; i++){
                                m_addr[i] = buffer[i+5];
                        }

                        s_verb[4] = '\0';
                        login_verb[10] = '\0';
                        m_addr[USERNAME_MAXLEN] = '\0';

                        // Respond to each verb accordingly.
                        //
                        if (STREQU(s_verb, "HELO") || STREQU(s_verb, "EHLO")) { // Initial greeting
                                sprintf(bufferout, "250 Ok\r\n");
                                printf("S%d: %s", sockfd, bufferout);
                                send(sockfd, bufferout, strlen(bufferout), 0);
                        } else if (STREQU(s_verb, "NOOP")) { // Do nothing.
                                sprintf(bufferout, "250 Ok noop\r\n");
                                printf("S%d: %s", sockfd, bufferout);
                                send(sockfd, bufferout, strlen(bufferout), 0);
                        } else if (STREQU(s_verb, "QUIT")) { // Close the connection
                                sprintf(bufferout, "221 Ok\r\n");
                                printf("S%d: %s", sockfd, bufferout);
                                send(sockfd, bufferout, strlen(bufferout), 0);
                                break;
                        } else if (STREQU(login_verb, "AUTH LOGIN")) { // Log in to your mail account
                                sprintf(bufferout, "334 Username:\r\n");
                                printf("S%d: %s", sockfd, bufferout);
                                send(sockfd, bufferout, strlen(bufferout), 0);
                                message_code = 2;
                        }


                        if (is_logged == 1) {
                                if (STREQU(s_verb, "MAIL")) { // New mail from...
                                        sprintf(bufferout, "250 Ok sender\r\n");
                                        printf("S%d: %s", sockfd, bufferout);
                                        send(sockfd, bufferout, strlen(bufferout), 0);

                                        // Email addresses
                                        for (i=0; i<USERNAME_MAXLEN; i++){
                                                mail_sender[i] = buffer[i+5];
                                        }
                                } else if (STREQU(s_verb, "RCPT")) { // Mail addressed to...
                                        sprintf(bufferout, "250 Ok recipient\r\n");
                                        printf("S%d: %s", sockfd, bufferout);
                                        send(sockfd, bufferout, strlen(bufferout), 0);

                                        // Email addresses
                                        for (i=0; i<USERNAME_MAXLEN; i++){
                                                mail_recipient[i] = buffer[i+5];
                                        }
                                } else if (STREQU(s_verb, "DATA")) { // Message contents
                                        if (STREQU(mail_sender, "") || STREQU(mail_recipient, "")) {
                                                sprintf(bufferout, "503 Bad sequence of commands. Sender and recipient addresses are needed first.\r\n");
                                                printf("S%d: %s", sockfd, bufferout);
                                                send(sockfd, bufferout, strlen(bufferout), 0);
                                        } else {
                                                sprintf(bufferout, "354 Continue. When you are ready with the contents, send '.' (dot) sign.\r\n");
                                                printf("S%d: %s", sockfd, bufferout);
                                                send(sockfd, bufferout, strlen(bufferout), 0);
                                                message_code = 1;
                                        }
                                } else if (STREQU(s_verb, "RSET")) { // Reset the connection
                                        sprintf(bufferout, "250 Ok reset\r\n");
                                        printf("S%d: %s", sockfd, bufferout);
                                        send(sockfd, bufferout, strlen(bufferout), 0);
                                        is_logged = 0;
                                }
                        } else if (message_code == 0){
                                sprintf(bufferout, "401 Unauthorized. Log in using AUTH LOGIN command.\r\n");
                                printf("S%d: %s", sockfd, bufferout);
                                send(sockfd, bufferout, strlen(bufferout), 0);
                        }
                } else if (message_code == 1){ // We are inside the message after a DATA verb.
                        printf("C%d: %s\n", sockfd, buffer);

                        // Add contents to special buffer and add \n at the end
                        for (i=0; i<=BUF_SIZE; i++) {
                                if (i == BUF_SIZE) {
                                        contents[i + BUF_SIZE*line_ctr] = '\n';
                                } else {
                                        contents[i + BUF_SIZE*line_ctr] = buffer[i];
                                }
                        }

                        if (STREQU(buffer, ".")) { // A single "." signifies the end
                                message_code = 0;
                                line_ctr = 0;

                                FILE *fptr;

                                time_t rawtime;
                                struct tm *info;
                                time( &rawtime );
                                info = localtime( &rawtime );
                                char filename[60];
                                char timestamp[32];
                                char trim_timestamp[24];
                                sprintf(timestamp, "%s", asctime(info));

                                // Save without two redundant literals $'\n'
                                for(i=0;i<24;i++) {
                                        trim_timestamp[i] = timestamp[i];
                                }
                                sprintf(filename, "/lab/project/mailbox/message-%s.txt", trim_timestamp);

                                fptr = fopen(filename,"w");

                                char final_message[2*(USERNAME_MAXLEN + 1) + (BUF_SIZE + 1)*LINE_LIMIT];

                                if(fptr == NULL)
                                {
                                        sprintf(bufferout, "Error while trying to create a file to save your email.\r\n");
                                        printf("S%d: %s", sockfd, bufferout);
                                        send(sockfd, bufferout, strlen(bufferout), 0);
                                }

                                strcat(final_message, "From: ");
                                strcat(final_message, mail_sender);
                                strcat(final_message, "\n");
                                strcat(final_message, "To: ");
                                strcat(final_message, mail_recipient);
                                strcat(final_message, "\n\n");
                                strcat(final_message, contents);

                                fprintf(fptr, "%s", final_message);

                                fclose(fptr);

                                sprintf(bufferout, "250 Mail has been saved successfully!\r\n");
                                printf("S%d: %s", sockfd, bufferout);
                                send(sockfd, bufferout, strlen(bufferout), 0);
                        } else {
                                line_ctr++;
                        }
                } else if (message_code == 2){
                        if (STREQU(buffer, mail_user)) {
                                sprintf(bufferout, "334 Password:\r\n");
                                printf("S%d: %s", sockfd, bufferout);
                                send(sockfd, bufferout, strlen(bufferout), 0);
                                message_code = 3;
                        } else {
                                sprintf(bufferout, "454 Temporary authentication failure\r\n");
                                printf("S%d: %s", sockfd, bufferout);
                                send(sockfd, bufferout, strlen(bufferout), 0);
                                message_code = 0;
                        }
                } else if (message_code == 3) {
                        if (STREQU(buffer, simple_passwd)) {
                                sprintf(bufferout, "235 OK Authenticated\r\n");
                                printf("S%d: %s", sockfd, bufferout);
                                send(sockfd, bufferout, strlen(bufferout), 0);
                                is_logged = 1;
                        } else {
                                sprintf(bufferout, "454 Temporary authentication failure\r\n");
                                printf("S%d: %s", sockfd, bufferout);
                                send(sockfd, bufferout, strlen(bufferout), 0);
                        }
                        message_code = 0;
                } else {
                        sprintf(bufferout, "502 Command Not Implemented\r\n");
                        printf("S%d: %s", sockfd, bufferout);
                        send(sockfd, bufferout, strlen(bufferout), 0);
                }

                // Shift the rest of the buffer to the front
                memmove(buffer, eol+2, BUF_SIZE - (eol + 2 - buffer));
                buffer_offset -= (eol - buffer) + 2;

                // Do we already have additional lines to process? If so,
                // commit a horrid sin and goto the line processing section again.
                if (strstr(buffer, "\r\n"))
                        goto processline;
        }

        // All done. Clean up everything and exit.
        close(sockfd);
        pthread_exit(NULL);
}

// Extract the address from sockaddr depending on which family of socket it is
void * get_in_addr(struct sockaddr *sa) {
        if (sa->sa_family == AF_INET) {
                return &(((struct sockaddr_in*)sa)->sin_addr);
        }
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
}