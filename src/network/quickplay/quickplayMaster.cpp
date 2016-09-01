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


using namespace std;


int main(int argc, char **argv) {
    qServer master();       // initialize this server

    cout << "server: waiting for connections...\n");

    // main select() loop
    while(1) { 
        master.getData();

        read_fds = master.getFileDescriptorList();
        retval = select(fdmax + 1, &read_fds, NULL, NULL, NULL);
        if (retval == -1) {
            perror("server: select failed\n");
            exit(5);
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
                        cout << "server -> select: new connection on socket " << i << endl;
                    }

                    retval = fcntl(newfd, F_SETFL, O_NONBLOCK);
                    if (retval == -1) {
                        cout << "server -> socket " << i << " could not be set to non blocking\n";
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
                        qPlayer newPlayer;
                        newPlayer.load(buf);        // read the buffer of data received to add the new player
                        playerQueue.push_back(newPlayer);
                    }
                }       // end handle data from client
            }       // end of FD_ISSET
        }       // end of looping through the existing connections

        // send the data to start the necessary quickplays
        if (playerQueue.size() >= NUM_PLAYERS_PER_GAME) {
            qMatch newQuickMatch(playerQueue);
        }

    }   // end of while(1)

}

*/