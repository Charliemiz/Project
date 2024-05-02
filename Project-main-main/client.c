/*
 * Auth: Charlie Misbach
 * Date: 04-18-24 (Due: 05-05-24)
 * Course: CSCI-3550 (Sec: 001)
 * Desc:  Project #1, TCP/IP Project
 */

#include<unistd.h>
#include<stdio.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<signal.h>  /* Needed for 'signal()' function. */
#include<arpa/inet.h> /* Needed for inet_aton() function */

#define BUF_SIZE  (100*1024*1024)   /* 100 MiB buffer size */

/* Function prototypes */
void SIGINT_handler( int sig );
void cleanup(void);

int sockfd = -1;    /* Socket file descriptor (marked unused) */

int fd = -1;   /* File descriptor for input file(s) */

/* Buffer pointers */
char *inbuf  = NULL;    /* For storing incoming data from 'recv()' */
char *outbuf = NULL;    /* For storing outgoing data using 'send()' */

int bytes_sent; /* keep track of how many bytes we've sent */
int bytes_read; /* keeps track of how many bytes we've read */
int file_read; /* keeps track of how many bytes read from file */


int main(int argc, char *argv[] ) {

    struct in_addr ia; /* Declare the structure for storing IPv4 address. */
    struct sockaddr_in sa; /* Declare for storing socket address */

    const char *port_str;   /* Use later to store port from command line arg (STR)*/
    unsigned short int port; /* Store port number (not str) */

    int i; /* Iteration variable for cmd line args */

    /* Install our function to catch 'CTRL+C' */ 
    signal(SIGINT, SIGINT_handler);

    /* If no file names (or port or IP) are provided, enter if */
    if (argc <= 3) {
        /* print error telling user whats needed */
        fprintf(stderr, "USAGE: %s <server_IP> <server_Port> file1 file2 ...", argv[0]);

        /* exit with error code */
        return 1;
    }

    /* Fill it with an IP address from a dotted-decimal string given as 
    a command-line argument stored in ‘argv[1]’ */
    if (inet_aton(argv[1], &ia) == 0) {
        fprintf(stderr, "ERROR: Setting the IP address.\n");
        return 1;
    }

    /* Store port as string in port_str and use atoi for ASCII-to-integer */
    port_str = argv[2];
    port = (unsigned short int)atoi(port_str);

    if (port <= 1023) {
        fprintf(stderr, "client: ERROR: Port number %u is privileged.\n", port);
        return 1;
    }

    sa.sin_family = AF_INET;    /* Use IPv4 addresses */
    sa.sin_addr = ia;           /* Attach IP address structure */
    sa.sin_port = htons( port ); /* Set port number in Network Byte Order */\
    /* socket address stored in sa is now ready! */
    
    /* After allocation we read the files from command line */
    for (i = 3; i < argc; i++) {
        /* Attempt to connect message */
        printf("client: Connecting to %s:%d...\n", inet_ntoa(ia), ntohs(sa.sin_port));
        /* Create a new TCP socket */
        sockfd = socket( AF_INET, SOCK_STREAM, 0);

        /* Check that it succeeded */
        if( sockfd < 0 ) {
            fprintf( stderr, "ERROR: Unable to obtain a new socket.\n\n" );
            continue;
        }

        if( connect( sockfd, (struct sockaddr *) &sa, sizeof( sa ) ) != 0 ) {
            fprintf( stderr, "ERROR: Attempting to connect with the server.\n\n" );
            close(sockfd);
            continue;
        } else {
            printf("client: Success!\n");
        } /* end if else () */

        printf("client: Sending: \"%s\"...\n", argv[i]);
        fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "ERROR: Unable to open file / Empty file %s.\n\n", argv[i]);
            close(sockfd);
            close(fd);
            continue;
        }

        /* After connecting to socket, begin to allocate memory to out buffer */
        outbuf = (void *) malloc( BUF_SIZE );

        /* Check if allocation was successful. */
        if( outbuf == NULL ) {
            fprintf( stderr, "ERROR: Failed to allocate memory for output buffer.\n\n" );
            close(fd);
            close(sockfd);
            continue;
        }

        /* Read the file content(s) into proper buffer */
        file_read = read(fd, outbuf, BUF_SIZE);
        close(fd);
        if (file_read < 0) {
            fprintf(stderr, "Failed to read the file %s.\n\n", argv[i]);
            /* Close file */
            close(fd);
            /* Mark it as unused */
            sockfd = -1;         
            /* Attempt next file */
            continue;
        }

        /* Close file if we successfully read from it */
        close(fd);
        fd = -1;

        /* Send the data out a given socket */
        bytes_sent = send(sockfd, (const void *) outbuf, file_read, 0);

        /* Verify that we sent what we asked to send */
        if (bytes_sent != file_read) {
            fprintf( stderr, "ERROR: send(): Failed to send all data for file %s.\n\n", argv[i] );
        } else {
            printf("client: Done sending %s.\n\n", argv[i]);
        }

        cleanup();
        close(sockfd);

    }

    printf("client: File transfer(s) complete.\n");
    printf("client: Goodbye!\n");

    cleanup();
    return 0;
}



/* SIGINT handler for the client */
void SIGINT_handler( int sig ) {

   /* Issue an error */
   fprintf( stderr, "client: Client interrupted. Shutting down.\n" );

   /* Cleanup after yourself */
   cleanup();

   /* Exit for 'reals' */
   exit( EXIT_FAILURE );

} /* end SIGINT_handler() */

void cleanup ( void ) {
    /* Free ONLY if it's safe to do so... */
    if( outbuf != NULL) {
        free( outbuf );
        outbuf = NULL;
    }
    /* Free a file or socket descriptor */
    if ( fd > -1 ) {
        close( fd );        /* Close the file or socket. */
        fd = -1;            /* Mark it as such. */
    }

}
