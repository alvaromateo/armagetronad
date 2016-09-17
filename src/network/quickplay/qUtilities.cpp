/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2016  Alvaro Mateo (alvaromateo9@gmail.com)

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

*/


#include "qUtilities.h"


static qServerInstance *quickplayActiveServer;

void setActiveServer(qServerInstance *instance) {
    quickplayActiveServer = instance;
}


// qConnection methods

qConnection::qConnection() {

}

qConnection::qConnection(int socket, sockaddr_storage remoteaddress) {

}


// qPlayer methods

qPlayer::qPlayer() {

}


// qServer methods

qServer::qServer() {
	// temporal variables
	struct addrinfo hints;
	struct addrinfo *servinfo, *p;
	int yes = 1;
    int retval;

    p = servinfo = NULL;    // initialization of pointers

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
        this.listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (this.listener == -1) {
            cout << "socked failed\n";
            continue;   // there was an error when opening the file, so we look into the next addrinfo stored in servinfo
        }

        retval = fcntl(socket, F_SETFL, O_NONBLOCK);
        if (retval == -1) {
            perror("fcntl failed\n");
            exit(1);
        }

        retval = setsockopt(this.listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (retval == -1) {
            perror("setsockopt failed\n");    // this sets an option on the sockets to avoid error messages about port already in use when restarting the server
            exit(2);
        }

        retval = bind(this.listener, p->ai_addr, p->ai_addrlen);
        if (retval == -1) {
            close(this.listener);
            cout << "server bind failed\n";
            continue;
        }

        break; // if this instruction is reached it means all three system calls were successful and we can proceed
    }

    freeaddrinf(servinfo);  // this struct is no longer needed as we are already bound to the port

    if (p == NULL) {
        perror("server: failed to bind\n");
        exit(3);
    }

    retval = listen(this.listener, BACKLOG);
    if (retval == -1) {
        perror("server: listen failed\n");
        exit(4);
    }

    // initialize this
    FD_ZERO(&this.master);       				// clear the master set
	FD_SET(this.listener, this.master);       	// adds the listener to the master set of descriptors
    this.fdmax = this.listener;         		// keep track of the maximum file descriptor

    cout << "server started\n");
}

int qServer::getData() {
	fd_set read_fds;        // temporal file descriptor list for select() function
	FD_ZERO(&read_fds);

    int newfd;              // newfd -> new socket to send information
    int i, j, retval;
    int nbytes;
    unsigned char buf[MAX_BUF_SIZE](); 		// values to store the data and the number of bytes received
    

	read_fds = this.master;
    retval = select(this.fdmax + 1, &read_fds, NULL, NULL, NULL);
    if (retval == -1) {
        perror("server: select failed\n");
        exit(5);
    }

    // loop through the existing connections looking for data to read
    for (i = 0; i <= this.fdmax; ++i) {
        if (FD_ISSET(i, &read_fds)) {   // there is some data ready
            if (i == listener) {        // new connection
            	socklen_t addrlen;
            	struct sockaddr_storage remoteaddr;

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

                    qPlayer newPlayer(newfd, remoteaddr, addrlen); 		// create new player
                    addPlayer(newPlayer);								// add player to the queue of players
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
                    // we got some data from a client -> handle it
                    handleData(buf, nbytes, i);
                }
            }       // end handle data from client
        }       // end of FD_ISSET
    }       // end of looping through the existing connections

}

short qServer::readShort(const uchar *buf, int &bytesRead) {
	if (MAX_BUF_SIZE - bytesRead > sizeof(short)) {
        short *reader = (short *)(buf + bytesRead);
		bytesRead += sizeof(short);
		return ntohs(*reader);                    // use ntohs to parse the data from the socket
	}
    return -1;              // the short could not be read
}


// qServerInstance methods

void qServerInstance::handleData(uchar *&buf, int numBytes, int sock) {
    uchar type = 0;

    // find if there is already a message started in the socket
    MQ::iterator it = messageQueue.find(sock);
    if (it != messageQueue.end()) {
        // message already exists -> add the new part to it
        (it->second)->addMessagePart(buf, numBytes);
    } else {
        // new message to handle -> create it and add the just received part
        type = buf[0];

        qMessage *message = createMessage(type);
        message->addMessagePart(buf + 1, numBytes - 1);
        // add the message to the map
        messageQueue.insert(pair<uint, *qMessage>(sock, message));
    }
}

void qServerInstance::processMessages() {
    MQ::iterator it = messageQueue.begin();
    while (it != messageQueue.end()) {
        if ((it->second)->isMessageReadable()) {
            (it->second)->handleMessage(it->first);
            deleteMessage(it, messageQueue);        // the message has to be deleted to free memory
        } else {
            ++it;
        }
    }
}

/*
 * Iterator it has to point to a valid element. No checks are made in this method.
 */
void qServerInstance::deleteMessage(MQ::iterator &it, MQ &queue) {
    delete it->second;
    queue.erase(it++);
}

// creates each type of different messages
qMessage *qServerInstance::createMessage(uchar type) {
    qMessage *ret = NULL;
    switch (type) {
        case 1:         // ack message
            ret = new qAckMessage();
            break;
        case 2:         // player sends info and requests to join game
            ret = new qPlayerInfoMessage();
            break;
        case 3:         // match host tells server it is ready 
            ret = new qMatchReadyMessage();
            break;
        case 4:         // tells the master to resend the last information sent (cause it wasn't received)
            ret = new qResendMessage();
            break;
        case 5:         // server tells player that he/she has to be the host of the match 
            ret = new qSendHostingOrder();
            break;
        case 6:         // server tells player to which address he has to connect play a match
            ret = new qSendConnectInfo();
            break;
        default:        // the type read doesn't match with any of the possible messages
            ret = new qMessage();
    }
    return ret;
}


// qMessage methods

qMessage::qMessage() : buffer(NULL), messLen(0), currentLen(0), type(0) {
    buffer = new uchar[MAX_BUF_SIZE]();
}

qMessage::qMessage(uchar type) : 
    buffer(NULL), messLen(0), currentLen(0), type(type) {
        buffer = new uchar[MAX_BUF_SIZE]();
}

qMessage::~qMessage() {
    // delete the allocated memory
    delete[] buffer;
}

void qMessage::addMessgePart(uchar *&buf, int numBytes) {
    if (messLen == 0 && currentLen >= sizeof(ushort)) {
        messLen = readShort(buf, 0);
    }
    numBytes = numBytes > MAX_BUF_SIZE ? MAX_BUF_SIZE : numBytes;
    std::copy(buf, buf + numBytes, buffer + currentLen);
    currentLen += numBytes;
}

void qMessage::handleMessage(int sock) {
    // send to the player a message asking to resend as there was some error when reading the message
}


// qMessage classes

qAckMessage::qAckMessage() : qMessage(1) {}

void qAckMessage::handleMessage(int sock) {

}


qPlayerInfoMessage::qPlayerInfoMessage() : qMessage(2) {}

void qPlayerInfoMessage::handleMessage(int sock) {
    
}


qMatchReadyMessage::qMatchReadyMessage() : qMessage(3) {}

void qMatchReadyMessage::handleMessage(int sock) {
    
}


qResendMessage::qResendMessage() : qMessage(4) {}

void qResendMessage::handleMessage(int sock) {
    
}


qSendHostingOrder::qSendHostingOrder() : qMessage(5) {}

void qSendHostingOrder::handleMessage(int sock) {
    
}


qSendConnectInfo::qSendConnectInfo() : qMessage(6) {}

void qSendConnectInfo::handleMessage(int sock) {
    
}
// PlayerCPU methods


nMessage& nMessage::ReadRaw(tString &s )
{
    s.Clear();
    unsigned short w,len;
    Read(len);
    if ( len > 0 )
    {
        s[len-1] = 0;
        for(int i=0;i<len;i+=2){
            Read(w);
            
            // carefully reverse the signed
            // encoding logic
            signed char lo = w & 0xff;
            signed short hi = ((short)w) - lo;
            hi >>= 8;

            s[i] = lo;
            if (i+1<len)
                s[i+1] = hi;
        }
    }

    return *this;
}

void nMessage::Read(unsigned short &x){
    if (End()){
        tOutput o;
        st_Breakpoint();
        o.SetTemplateParameter(1, senderID);
        o << "$network_error_shortmessage";
        con << o;
        // sn_DisconnectUser(senderID, "$network_kill_error");
        nReadError( false );
    }
    else
        x=data(readOut++);
}
