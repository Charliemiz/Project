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
#include<string.h>

#define BUF_SIZE  (100*1024*1024)   /* 100 MiB buffer size */

/* Function prototypes */
void SIGINT_handler( int sig );
void cleanup(void);

int sockfd = -1;    /* Socket file descriptor (marked unused) */
int fd_out = -1;   /* File descriptor for output file(s) */

/* Buffer pointers */
char *inbuf  = NULL;    /* For storing incoming data from 'recv()' */
char *outbuf = NULL;    /* For storing outgoing data using 'send()' */

int bytes_written; /* keep track of how many bytes we've written */
int bytes_read; /* keeps track of how many bytes we've read */
int file_read; /* keeps track of how many bytes read from file */

int main(int argc, char *argv[] ) {

    struct in_addr ia; /* Declare the structure for storing IPv4 address. */
    struct sockaddr_in sa; /* Declare for storing socket address */

    struct sockaddr_in cl_sa; /* Socket address of the client, once connected */
    socklen_t cl_sa_size; /* Size of the client socket address structure */
    int cl_sockfd; /* Socket descriptor for accepted client */

    const char *port_str; /* Used later to store port from command line arg (STR) */
    unsigned short int port; /* Store port number (not str) */

    int val = 1; /* Setting for one of the socket options */
    int file_counter = 1; /* counts what # file were on */
    char fname[80]; /* array that holds the dynamic filenames we construct */

    /* Clear all the bytes of this structure to zero */
    memset((void *)&cl_sa, 0, sizeof(cl_sa));

    /* Compute and store the size of the socket address structure */
    cl_sa_size = sizeof(cl_sa);


    /* if not enough args provided, error */
    if ( argc <= 1) {
        printf("server: USAGE: server <listen_Port>");
        return 1;
    }

    ia.s_addr = inet_addr( "127.0.0.1" );
    port_str = argv[1];

    /* Store port as string in port_str and use atoi for ASCII-to-integer */
    port = (unsigned short int)atoi(port_str);

    /* avoid priveledged port numbers */
    if (port <= 1023) {
        fprintf(stderr, "server: ERROR: Port number %u is privileged.\n", port);
        return 1;
    }

    sa.sin_family = AF_INET;    /* Use IPv4 addresses */
    sa.sin_addr = ia;           /* Attach IP address structure */
    sa.sin_port = htons( port ); /* Set port number in Network Byte Order */
    /* socket address stored in sa is now ready! */


    /* Create a new TCP socket */
    sockfd = socket( AF_INET, SOCK_STREAM, 0 );

    printf("server: Server running");
    printf("server: Awaiting TCP connections over port %d...\n", port);

    /* make socket reusable post creation of a socket */
    if( setsockopt( sockfd,
        SOL_SOCKET, SO_REUSEADDR, (const void *) &val, sizeof( int ) ) != 0 ) {
            fprintf( stderr, "server: ERROR: setsockopt() failed.\n" );
    } /* end if() */

    /* Check that it succeeded */
    if( sockfd < 0 ) {
        fprintf( stderr, "server: ERROR: Unable to obtain a new socket.\n" );
    }

    /* Bind our listening socket to the socket address specified by 'sa'
    on the server side */
    if( bind( sockfd, (struct sockaddr *) &sa, sizeof( sa ) ) != 0 ) {
        fprintf( stderr, "server: ERROR: Socket binding failed.\n" );
        return 1;
    }

    /* Turn an already-bound socket into a 'listening' socket
   to start accepting connection requests from clients. */
    if ( listen( sockfd, 32 ) != 0 ) {
        fprintf( stderr, "server: ERROR: Could not turn socket into a 'listening' socket.\n" );
        return 1;
    }  

    while(1) {
        /* sit and wait for a connection request from the client */
        cl_sockfd = accept( sockfd, (struct sockaddr *) &cl_sa, &cl_sa_size );

        if( cl_sockfd > 0 ) {
            printf("server: Connection accepted!\n");
            /* After accepting connection allocate mem to inbuf */
            inbuf = (void *) malloc( BUF_SIZE );

            /* Check if allocation was successful. */
            if( inbuf == NULL ) {
                fprintf( stderr, "ERROR: Failed to allocate memory for output buffer.\n\n" );
                close(cl_sockfd);
                return 1;
            }

            printf("server: Receiving file...\n");
            /* read from client socket*/
            bytes_read = recv(cl_sockfd, inbuf, BUF_SIZE, MSG_WAITALL);

            /* check if recieved bytes properly */
            if ( bytes_read > 0 ) {
                /* Construct a name dynamically. (Will result in "file-01.dat") */
                sprintf( fname, "file-%02d.dat", file_counter );
                fd_out = open(fname, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
                if( fd_out < 0 ) {
                    /* Issue the error */
                    fprintf( stderr, "server: ERROR: Could not create: %s\n", fname );
                    /* Clean up. */
                    cleanup();
                    /* Exit. */
                    return 1;
                } 
                /* Write the data in the buffer... */
                bytes_written = write( fd_out, inbuf, (size_t) bytes_read );

                /* Print that we are writing to the file */
                printf("server: Saving: %s\n", fname);

            } else if ( bytes_read == 0 ){
                printf("server: Connection closed\n");
            }
        } else {
            fprintf( stderr, "server: ERROR: accept(): Connection request failed.\n" );
        }   

        printf("server: Done\n\n");

        /* Successfully written to the file, now increment the file counter */
        file_counter++; 

        cleanup();
        close(cl_sockfd);

    }

    return 0;
}

/* SIGINT handler for the server */
void SIGINT_handler( int sig ) {

   /* Issue an error */
   fprintf( stderr, "server: Server interrupted. Shutting down.\n" );

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
    if( inbuf != NULL) {
        free( inbuf );
        outbuf = NULL;
    }
    if (sockfd > -1) {
        close(sockfd);
        sockfd = -1;
    }
    /* Free a file or socket descriptor */
    if ( fd_out > -1 ) {
        close( fd_out );        
        fd_out = -1;            
    }

} /* end cleanup() */