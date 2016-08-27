/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)

**************************************************************************

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
***************************************************************************

ATENTION!

This server is only guaranteed to run on GNU/Linux distributions

*/

#include <iostream>
#include <fstream>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>


// Defines
#define PORT "3490"
#define BACKLOG 20      // How many pending connections queue will hold

using namespace std;


vector<tronPlayer> playerQueue(20);     // Queue for holding the players that have to be paired


int main(int argc, char** argv)
{
    fd_set master;          // master file descriptor list
    fd_set read_fds;        // temporal file descriptor list for select() function
    int fdmax;              // maximum file descriptor number

    int listener, newfd;        // sockfd -> the socket we will listen to
                                // newfd -> new socket to send information

    struct addrinfo hints;
    struct sockaddr_storage remoteaddr;         // client address
    struct addrinfo *servinfo, *p;
    socklen_t addrlen;

    char remoteaddrbuf[INET6_ADDRSTRLEN];
    int yes = 1;
    int retval, i, j;

    char buf[1024];         // values to store the data and the number of bytes received
    int nbytes;

    p = servinfo = NULL;    // initialization of pointers

    FD_ZERO(&master);       // clear the master and read_fds sets
    FD_ZERO(&read_fds);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    retval = getaddrinfo(NULL, PORT, &hints, &servinfo);
    if (retval != 0) {
        cerr << "server getaddrinfo: " << gai_strerror(retval) << endl;
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            cout << "socked failed\n";
            continue;   // there was an error when opening the file, so we look into the next addrinfo stored in servinfo
        }

        retval = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (retval == -1) {
            cout << "setsockopt failed\n";    // this sets an option on the sockets to avoid error messages about port already in use when restarting the server
            exit(1);
        }

        retval = bind(sockfd, p->ai_addr, p->ai_addrlen);
        if (retval == -1) {
            close(sockfd);
            cout << "server bind failed\n";
            continue;
        }

        break; // if this instruction is reached it means all three system calls were successful and we can proceed
    }

    freeaddrinf(servinfo);  // this struct is no longer needed as we are already bound to the port

    if (p == NULL) {
        perror("server: failed to bind\n");
        exit(2);
    }

    retval = listen(sockfd, BACKLOG);
    if (retval == -1) {
        perror("server: listen failed\n");
        exit(3);
    }

    FD_SET(listener, master);       // adds the listener to the master set of descriptors
    fdmax = listener;               // keep track of the maximum file descriptor

    cout << "server: waiting for connections...\n");

    // main select() loop
    while(1) { 
        read_fds = master;
        retval = select(fdmax + 1, &read_fds, NULL, NULL, NULL);
        if (retval == -1) {
            perror("server: select failed\n");
            exit(4);
        }

        // loop through the existing connections looking for data to read
        for (i = 0; i <= fdmax; ++i) {
            if (FD_ISSET(i, &read_fds)) {   // there is some data ready
                if (i == listener) {        // new connection
                    addrlen = sizeof(remoteaddr);
                    newfd = accept(listener, (struct sockaddr *) &remoteaddr, &addrlen);

                    if (newfd == -1) {
                        perror("server: accept failed\n");
                    } else {
                        FD_SET(newfd, &master);
                        if (newfd > fdmax) {
                            fdmax = newfd;      // keep track of the max
                        }
                        cout << "server -> select: new connection\n";
                    }
                } else {        // handle data from a client
                    nbytes = recv(i, buf, sizeof(buf), 0);
                    if (nbytes <= 0) {      // error or connection closed by the client
                        if (nbytes == 0) { 
                            // connection closed
                            cout << "server -> select: socket hung up\n";
                        } else {
                            perror("server: recv failed\n");
                        }

                        // we don't need this socket any more
                        close(i);
                        FD_CLR(i, &master);     // remove it from master set
                    }  else {
                        // we got some data from a client
                        // store the player in the queue
                        // TODO
                    }
                }       // end handle data from client
            }       // end of FD_ISSET
        }       // end of looping through the existing connections

        // send the data to start the necessary quickplays
        // TODO

    }   // end of while(1)

}

