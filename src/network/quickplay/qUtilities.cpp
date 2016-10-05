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


using namespace std;


uint qServerInstance::matchesIds = 1;


// GLOBAL functions



// qMessageStorage methods

qMessageStorage::~qMessageStorage() {
    // delete messages
    MQ::iterator receivedIt, sendIt, pendingIt;
    for (receivedIt = receivedQueue.begin(); receivedIt != receivedQueue.end(); ++receivedIt) {
        delete (receivedIt->second);
    }
    for (sendIt = sendingQueue.begin(); sendIt != sendingQueue.end(); ++sendIt) {
        delete (sendIt->second);
    }
    for (pendingIt = pendingAckQueue.begin(); pendingIt != pendingAckQueue.end(); ++pendingIt) {
        delete (pendingIt->second);
    }
    cerr << "Deleting message storage" << endl;
}

/*
 * Iterator it has to point to a valid element. No checks are made in this method.
 */
void qMessageStorage::deleteMessage(MQ::iterator &it, MQ &queue) {
    // delete the pointer to the message only if the message is only in one queue
    if (getSocketReferences(it) <= 1) {
        cerr << "message storage -> deleting message from memory" << endl;
        delete (it->second);
    }
    queue.erase(it++);
    cerr << "message storage -> deleted message from queue" << endl;
}

void qMessageStorage::deleteMessage(int sock, MQ &queue) {
    MQ::iterator it = queue.find(sock);
    if (it != queue.end()) {
        deleteMessage(it, queue);
    }
}

/*
 * Move element pointed by it from originQueue to destQueue. Increments iterator to next element
 */
void qMessageStorage::moveMessage(MQ::iterator &it, MQ &originQueue, MQ &destQueue) {
    cerr << "message storage -> moving message from queue" << endl;
    if (it != originQueue.end()) {
        addMessage(messElem(it->first, it->second), destQueue);
        deleteMessage(it, originQueue);
    }
}

/*
 * Count the number of times a same message appears in the queues.
 */
int qMessageStorage::getSocketReferences(const MQ::iterator &it) {
    int count = 0;
    MQ::iterator reference = receivedQueue.find(it->first);
    // Check receivedQueue
    if (reference != receivedQueue.end()) {
        if (reference->second == it->second) {
            ++count;
        }
    }
    // Check sendingQueue
    reference = sendingQueue.find(it->first);
    if (reference != sendingQueue.end()) {
        if (reference->second == it->second) {
            ++count;
        }
    }
    // Check pendingAckQueue
    reference = pendingAckQueue.find(it->first);
    if (reference != pendingAckQueue.end()) {
        if (reference->second == it->second) {
            ++count;
        }
    }

    return count;
}

MQ::iterator qMessageStorage::getMessageFromQueue(int sock, MQ &queue) {
    cerr << "message storage -> retrieving message for socket " << sock << endl;
    return queue.find(sock);
}

/*
 * Adds a message to the given queue
 */
void qMessageStorage::addMessage(const messElem &elem, MQ &queue) {
    cerr << "message storage -> adding message to queue" << endl;
    queue.insert(elem);
}

void qMessageStorage::sendMessages() {
    cerr << "message storage -> sending messages (" << sendingQueue.size() << " messages in queue)" << endl;
    MQ::iterator it = sendingQueue.begin();
    while (it != sendingQueue.end()) {
        if (it->second->send(it->first) < 0) {      // send message and get error status
            cerr << "sending message -> some error occurred - trying to resend..." << endl;
            ++it;
        } else {
            // move message to ack queue
            cerr << "message storage -> moving message to ack queue" << endl;
            moveMessage(it, sendingQueue, pendingAckQueue);
        }
    }
}

void qMessageStorage::resendUnacked() {
    cerr << "message storage -> resending messages waiting for ack" << endl;
    MQ::iterator it = pendingAckQueue.begin();
    while (it != pendingAckQueue.end()) {
        // move message to sending queue
        moveMessage(it, pendingAckQueue, sendingQueue);
    }
}

// creates each type of different messages
qMessage *qMessageStorage::createMessage(uchar type) {
    qMessage *ret = NULL;
    switch (type) {
        case ACK:         // ack message
            ret = new qAckMessage();
            break;
        case PLAYER_INFO:           // player sends info and requests to join game
            ret = new qPlayerInfoMessage();
            break;
        case MATCH_READY:           // match host tells server it is ready 
            ret = new qMatchReadyMessage();
            break;
        case RESEND:                // tells the master to resend the last information sent (cause it wasn't received)
            ret = new qResendMessage();
            break;
        case SEND_HOST:             // server tells player that he/she has to be the host of the match 
            ret = new qSendHostingOrder();
            break;
        case SEND_CONNECT:          // server tells player to which address he has to connect play a match
            ret = new qSendConnectInfo();
            break;
        case SEND_PEERS:            // server send player his peers info so that he can measure the ping to them
            ret = new qSendPeersInfo();
            break;
        default:        // the type read doesn't match with any of the possible messages
            ret = new qMessage();
    }
    cerr << "message storage -> NEW message created" << endl;
    return ret;
}

void qMessageStorage::handleData(const uchar *buf, int numBytes, int sock) {
    uchar type = 0;
    cerr << "message storage -> received data (" << numBytes << " bytes received)" << endl;

    // find if there is already a message started in the socket
    MQ::iterator it = getReceivedQueue().find(sock);
    if (it != getReceivedQueue().end()) {
        cerr << "message storage -> the data was part of a previously received message" << endl;
        // message already exists -> add the new part to it
        (it->second)->addMessagePart(buf, numBytes);
    } else {
        // new message to handle -> create it and add the just received part
        type = buf[0];
        cerr << "message storage -> received new message of type " << static_cast<int> (type) << endl;

        qMessage *message = createMessage(type);
        message->addMessagePart(buf, numBytes);
        // add the message to the map
        addMessage(messElem(sock, message), getReceivedQueue());
    }
}


// PlayerInfo methods

PlayerInfo::PlayerInfo() : numCores(0), cpuSpeedInteger(0), cpuSpeedFractional(0), ping(0) {}

PlayerInfo::PlayerInfo(uchar numCores, uchar cpuSpeedInteger, uchar cpuSpeedFractional, ushort ping) 
    : numCores(numCores), cpuSpeedInteger(cpuSpeedInteger), cpuSpeedFractional(cpuSpeedFractional), ping(ping) {}

void PlayerInfo::setProperties(uchar nCores, uchar cpuSI, uchar cpuSF, ushort p) {
    numCores = nCores;
    cpuSpeedInteger = cpuSI;
    cpuSpeedFractional = cpuSF;
    ping = p;
}

int PlayerInfo::setCpuInfo() {
    cerr << "PlayerInfo -> setting information about the CPU and ping" << endl;
    uchar nCores, cpuSI, cpuSF;
    ushort p;
    int res = getCpuInfo(&nCores, &cpuSI, &cpuSF, &p);
    if (res >= 0) {
        setProperties(nCores, cpuSI, cpuSF, p);
    }
    return res;
}

int PlayerInfo::getCpuInfo(uchar *nCores, uchar *cpuSI, uchar *cpuSF, ushort *p) {
    // TODO: implement correctly this method so that it reads from the cpu information of the system
    cerr << "PlayerInfo -> getting information" << endl;
    *nCores = 1;
    *cpuSI = 0;
    *cpuSF = 0;
    *p = 0;
    return 0;
}


// qConnection methods

qConnection::qConnection() {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int retval;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    retval = getaddrinfo(qSERVER_IP, qPORT, &hints, &servinfo);
    if (retval != 0) {
        cerr << "client -> getaddrinfo: " << gai_strerror(retval) << endl;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            cerr << "client -> socket failed" << endl;
            continue;
        }
        retval = connect(sockfd, p->ai_addr, p->ai_addrlen);
        if (retval == -1) {
            close(sockfd);
            cerr << "client -> connect failed" << endl;
            continue;
        }
        break;
    }

    if (p == NULL) {
        cerr << "client -> failed to connect" << endl;
    } else {
        // print IP address
        char str[qMAX_NET_ADDR];
        if (p->ai_family == AF_INET) {
            struct sockaddr_in *pV4Addr = reinterpret_cast<sockaddr_in *> (p->ai_addr);
            struct in_addr ipAddr = pV4Addr->sin_addr;
            inet_ntop(AF_INET, &ipAddr, str, qMAX_NET_ADDR);
        } else {
            struct sockaddr_in6 *pV6Addr = reinterpret_cast<sockaddr_in6 *> (p->ai_addr);
            struct in6_addr ipAddr = pV6Addr->sin6_addr;
            inet_ntop(AF_INET6, &ipAddr, str, qMAX_NET_ADDR);
        }
        cerr << "Client: connecting to " << str << endl;

        freeaddrinfo(servinfo);         // all done with this structure

        // set the class' variables -> only one used is sock
        // the remoteaddr is just useful to connect to other players
        sock = sockfd;
        remoteaddr.ss_family = 0;
        cerr << "client -> connected to socket " << sock << endl;
    }
}

qConnection::qConnection(int socket, sockaddr_storage remoteaddress) 
    : sock(socket), remoteaddr(remoteaddress) {}

qConnection::~qConnection() {
    close(sock);
}


// qPlayer methods

qPlayer::qPlayer() : info(), matchId(0), master(false) {}

qPlayer::qPlayer(int socket, sockaddr_storage remoteaddress)
    : qConnection(socket, remoteaddress), info(), matchId(0), master(false) {}

qPlayer::~qPlayer() {}

int qPlayer::getData() {
    int nbytes, retval;
    uchar buf[qMAX_BUF_SIZE];
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;

    fd_set read_fds;        // temporal file descriptor list for select() function
    FD_ZERO(&read_fds);
    
    // values from parent class
    int *fdmax = getSockAddr();
    FD_SET(*fdmax, &read_fds);

    retval = select(*fdmax + 1, &read_fds, NULL, NULL, &tv);
    if (retval == -1) {
        perror(strerror(errno));
    }
    cerr << "player -> returned from select" << endl;

    if (FD_ISSET(*fdmax, &read_fds)) {   // there is some data ready
        nbytes = recv(*fdmax, buf, sizeof(buf), 0);
        if (nbytes <= 0) {      // error or connection closed by the client
            if (nbytes == 0) { 
                // connection closed
                cerr << "client -> select: socket hung up" << endl;
                // Server closed connection
                retval = -1;
            } else {
                perror("client -> recv failed\n");
            }

            // we don't need this socket any more
            close(*fdmax);
            *fdmax = -1;
        }  else {
            // we got some data from a client -> handle it
            cerr << "client -> data ready" << endl;
            handleData(buf, nbytes, *fdmax);
            retval = 1;
        }
    } else {
        cerr << "player -> no data was available" << endl;
        retval = 0;     // if 10 seconds pass waiting we return 0 and print a message
    }

    return retval;
}

void qPlayer::processMessages() {
    cerr << "player -> processing " << getReceivedQueue().size() << " messages" << endl;
    MQ::iterator it = getReceivedQueue().begin();
    while (it != getReceivedQueue().end()) {
        if ((it->second)->isMessageReadable()) {
            (it->second)->handleMessage(it, this);

            switch(it->second->getType()) {
                case SEND_HOST:
                    cerr << "player -> message ordering to host the game received" << endl;
                    this->master = true;
                    break;
                case SEND_CONNECT:
                    cerr << "player -> message with information to connect to another player received" << endl;
                    this->setConnection(dynamic_cast<qSendConnectInfo *>(it->second));
                    break;
                default:
                    cerr << "player -> message type did not match qSendHostingOrder nor qSendConnectInfo" << endl;
            }
            deleteMessage(it, getReceivedQueue());      // the message has to be deleted to free memory
        } else {
            ++it;
        }
    }
}

void qPlayer::setConnection(qSendConnectInfo *message) {
    cerr << "player -> setting information of where to connect to the match" << endl;
    if (message->getFamily()) {         // ipV6 family == 1
        struct sockaddr_in6 ipV6Addr;
        memset(&ipV6Addr, 0, sizeof(ipV6Addr));
        ipV6Addr.sin6_family = AF_INET6;
        inet_pton(AF_INET6, message->getAddress(), &(ipV6Addr.sin6_addr));
        *(getSockaddrStorage()) = *(reinterpret_cast<sockaddr_storage *> (&ipV6Addr));
    } else {                            // ipV4 family == 0
        struct sockaddr_in ipV4Addr;
        memset(&ipV4Addr, 0, sizeof(ipV4Addr));
        ipV4Addr.sin_family = AF_INET;
        inet_pton(AF_INET, message->getAddress(), &(ipV4Addr.sin_addr));
        *(getSockaddrStorage()) = *(reinterpret_cast<sockaddr_storage *> (&ipV4Addr));
    }
}

void qPlayer::setInfo(qPlayerInfoMessage *message) {
    info = message->getInfo();
}

void qPlayer::sendMyInfoToServer() {
    info.setCpuInfo();
    cerr << "player -> cpu info read and set" << endl;
    qPlayerInfoMessage *message = new qPlayerInfoMessage();
    message->setProperties(info);
    message->prepareToSend();
    addMessage(messElem(getSock(), message), getSendingQueue());
    cerr << "player -> added qPlayerInfoMessage to sendingQueue" << endl;
    this->sendMessages();
}

bool qPlayer::hasBetterPC(qPlayer *other) {
    // TODO
    return false;
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

    retval = getaddrinfo(NULL, qPORT, &hints, &servinfo);
    if (retval != 0) {
        cerr << "server -> getaddrinfo: " << gai_strerror(retval) << endl;
        exit(1);
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        this->listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        cerr << "Socket opened in " << this->listener << endl;
        if (this->listener == -1) {
            cerr << "server -> socked failed" << endl;
            continue;   // there was an error when opening the file, so we look into the next addrinfo stored in servinfo
        }

        retval = fcntl(this->listener, F_SETFL, O_NONBLOCK);
        if (retval == -1) {
            perror("server -> fcntl failed\n");
            exit(2);
        }

        retval = setsockopt(this->listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (retval == -1) {
            perror("server -> setsockopt failed\n");    // this sets an option on the sockets to avoid error messages about port already in use when restarting the server
            exit(3);
        }

        retval = bind(this->listener, p->ai_addr, p->ai_addrlen);
        if (retval == -1) {
            close(this->listener);
            cerr << "server -> bind failed" << endl;
            continue;
        }

        // print IP address
        char str[qMAX_NET_ADDR];
        if (p->ai_family == AF_INET) {
            struct sockaddr_in *pV4Addr = reinterpret_cast<sockaddr_in *> (p->ai_addr);
            struct in_addr ipAddr = pV4Addr->sin_addr;
            inet_ntop(AF_INET, &ipAddr, str, qMAX_NET_ADDR);
        } else {
            struct sockaddr_in6 *pV6Addr = reinterpret_cast<sockaddr_in6 *> (p->ai_addr);
            struct in6_addr ipAddr = pV6Addr->sin6_addr;
            inet_ntop(AF_INET6, &ipAddr, str, qMAX_NET_ADDR);
        }
        cerr << "Server IP address: " << str << endl;

        break; // if this instruction is reached it means all three system calls were successful and we can proceed
    }

    freeaddrinfo(servinfo);  // this struct is no longer needed as we are already bound to the port

    if (p == NULL) {
        perror("server -> failed to bind\n");
        exit(4);
    }

    retval = listen(this->listener, qBACKLOG);
    if (retval == -1) {
        perror("server -> listen failed\n");
        exit(5);
    }

    // initialize this
    FD_ZERO(&this->master);       				// clear the master set
	FD_SET(this->listener, &this->master);      // adds the listener to the master set of descriptors
    this->fdmax = this->listener;         		// keep track of the maximum file descriptor

    cerr << "Server started" << endl;
}

qServer::~qServer() {
    close(listener);
}


// qServerInstance methods

qServerInstance::qServerInstance() {}

qServerInstance::~qServerInstance() {
    // delete all memory reserved
    PQ::iterator pqIt;
    for (pqIt = playerQueue.begin(); pqIt != playerQueue.end(); ++pqIt) {
        delete pqIt->second;
    }
}

void qServerInstance::getData() {
    fd_set read_fds;        // temporal file descriptor list for select() function
    FD_ZERO(&read_fds);

    int newfd;              // newfd -> new socket to send information
    int i, retval;
    int nbytes;
    uchar buf[qMAX_BUF_SIZE];        // values to store the data and the number of bytes received
    
    // values from parent class
    int *fdmax = getFDmax();
    int *listener = getListener();
    fd_set *master = getFileDescriptorList();

    read_fds = *master;
    retval = select(*fdmax + 1, &read_fds, NULL, NULL, NULL);
    if (retval == -1) {
        perror("server -> select failed\n");
        exit(6);
    }

    // loop through the existing connections looking for data to read
    for (i = 0; i <= *fdmax; ++i) {
        if (FD_ISSET(i, &read_fds)) {   // there is some data ready
            if (i == *listener) {        // new connection
                socklen_t addrlen;
                struct sockaddr_storage remoteaddr;

                addrlen = sizeof(remoteaddr);
                newfd = accept(*listener, (struct sockaddr *) &remoteaddr, &addrlen);

                if (newfd == -1) {
                    perror("server -> accept failed\n");
                } else {
                    FD_SET(newfd, master);
                    if (newfd > *fdmax) {
                        *fdmax = newfd;      // keep track of the max
                    }
                    cerr << "server -> select: new connection on socket " << newfd << endl;

                    qPlayer *newPlayer = new qPlayer(newfd, remoteaddr);        // create new player
                    addPlayer(newPlayer, newfd);                                // add player to the queue of players
                    cerr << "server -> NEW player created" << endl;
                }

                retval = fcntl(newfd, F_SETFL, O_NONBLOCK);
                if (retval == -1) {
                    cerr << "server -> socket " << i << " could not be set to non blocking" << endl;
                }

            } else {        // handle data from a client
                nbytes = recv(i, buf, sizeof(buf), 0);
                if (nbytes <= 0) {      // error or connection closed by the client
                    if (nbytes == 0) { 
                        // connection closed
                        cerr << "server -> select: socket hung up" << endl;
                        // TODO: delete the player from the playerQueue and from possible matchesQueue (in case the client disconnected)
                    } else {
                        perror("server -> recv failed\n");
                    }

                    // we don't need this socket any more
                    close(i);
                    FD_CLR(i, master);     // remove it from master set
                }  else {
                    // we got some data from a client -> handle it
                    cerr << "server -> data ready for socket " << i << endl;
                    handleData(buf, nbytes, i);
                }
            }       // end handle data from client
        }       // end of FD_ISSET
    }       // end of looping through the existing connections

}

void qServerInstance::deletePlayerFromMatches(int sock, uint id) {
    cerr << "server -> deleting player in socket " << sock << " from match with ID " << id << endl;
    bool notFound = true;
    pair<MatchQ::iterator, MatchQ::iterator> range = matchesQueue.equal_range(id);
    for (MatchQ::iterator it = range.first; it != range.second && notFound; ++it) {
        if (it->second->getSock() == sock) {
            matchesQueue.erase(it++);
            notFound = false;
        }
    } 
}

void qServerInstance::addPlayerToMatches(qPlayer *player, uint id) {
    cerr << "server -> adding player to match with ID " << id << endl;
    matchesQueue.insert(pair<uint, qPlayer*>(id, player));
}

void qServerInstance::processMessages() {
    cerr << "server -> processing " << getReceivedQueue().size() << " messages" << endl;
    MQ::iterator it = getReceivedQueue().begin();
    while (it != getReceivedQueue().end()) {
        if ((it->second)->isMessageReadable()) {
            (it->second)->handleMessage(it, this);
            // for some cases of messages received the server has to do extra work
            vector<qPlayer *> clientPlayers;
            qPlayer *player;
            switch(it->second->getType()) {
                case PLAYER_INFO:
                    cerr << "server -> message with player information received" << endl;
                    // get player and add info to it
                    player = playerQueue.find(it->first)->second;
                    player->setInfo(dynamic_cast<qPlayerInfoMessage *>(it->second));
                    break;
                case MATCH_READY:
                    cerr << "server -> message giving permision to send connect information to other players."
                        << "Match ready!" << endl;
                    // send connect info to the players in the match
                    // with the socket (it->first), get the player and its playerMatchId
                    player = getPlayer(it->first);
                    if (player) {
                        // search the rest of the players with that playerMatchId
                        fillClientPlayers(clientPlayers, player->getMatchId());
                        // send them connectInfo message
                        sendConnectMessages(clientPlayers);
                    }
                    // delete the master from the players with that matchId
                    deletePlayerFromMatches(it->first, player->getMatchId());
                    break;
                default:
                    cerr << "server -> message type did not match qPlayerInfoMessage nor qMatchReadyMessage" << endl;
                    break;
            }
            deleteMessage(it, getReceivedQueue());        // the message has to be deleted to free memory
        } else {
            ++it;
        }
    }
}

qPlayer *qServerInstance::getPlayer(int sock) {
    PQ::iterator it = playerQueue.find(sock);
    if (it != playerQueue.end()) {
        return it->second;
    }
    return NULL;
}

void qServerInstance::fillClientPlayers(vector<qPlayer *> &cPlayers, uint matchId) {
    pair<MatchQ::iterator, MatchQ::iterator> range = matchesQueue.equal_range(matchId);
    for (MatchQ::iterator it = range.first; it != range.second; ++it) {
        if (!(it->second->isMaster())) {
            cPlayers.push_back(it->second);
        }
    }
}

void qServerInstance::sendConnectMessages(const vector<qPlayer *> &cPlayers) {
    cerr << "server -> sending messages to players to connect to the game created by other user" << endl;
    for (vector<qPlayer *>::const_iterator it = cPlayers.cbegin(); it != cPlayers.cend(); ++it) {
        cerr << "server -> storing connection information for client" << endl;

        // fill message with information
        uchar nFamily;
        char nAddress[qMAX_NET_ADDR];
        struct sockaddr_storage *connection = (*it)->getSockaddrStorage();
        if (connection->ss_family == AF_INET) {
            // ipV4
            nFamily = 0;
            struct sockaddr_in *dest = (struct sockaddr_in *) connection;
            inet_ntop(AF_INET, &(dest->sin_addr), nAddress, qMAX_NET_ADDR);
        } else {
            // ipV6
            nFamily = 1;
            struct sockaddr_in6 *dest = (struct sockaddr_in6 *) connection;
            inet_ntop(AF_INET6, &(dest->sin6_addr), nAddress, qMAX_NET_ADDR);
        }

        // send message to player
        qSendConnectInfo *message = new qSendConnectInfo();
        message->setProperties(nFamily, nAddress);
        message->prepareToSend();
        addMessage(messElem((*it)->getSock(), message), getSendingQueue());
        cerr << "NEW qSendConnectInfo message created" << endl;
    }
}

int qServerInstance::prepareMatch() {
    int count = 0;
    vector<qPlayer *> playersAvailable;

    // get the players available for a game
    for (PQ::iterator it = playerQueue.begin(); it != playerQueue.end(); ++it) {
        qPlayer *tmp = it->second;
        if (tmp->isPlayerAvailable()) {
            playersAvailable.push_back(tmp);
        }
    }

    // pair the players in the order they were put in the vector 
    // there is no sophisticated matchmaking system based on player skill or something similar
    for (int i = 0; (unsigned long) (i + qPLAYERS) <= playersAvailable.size(); i += qPLAYERS) {
        uint id = getNextMatchId();
        qPlayer *master = playersAvailable[i];
        for (int j = 0; j < qPLAYERS; ++j) {
            playersAvailable[i + j]->setMatchId(id);
            addPlayerToMatches(playersAvailable[i + j], id);
            if ((playersAvailable[i + j])->hasBetterPC(master)) {
                master = playersAvailable[i + j];
            }
        }
        master->setMaster(true);

        // send message to master to order the creation of the game
        qSendHostingOrder *message = new qSendHostingOrder();
        message->prepareToSend();
        addMessage(messElem(master->getSock(), message), getSendingQueue());
        ++count;
        cerr << "NEW qSendHostingOrder message created" << endl;
    }

    return count;
}


// qMessage methods

qMessage::qMessage() : buffer(NULL), messLen(0), currentLen(0), type(0) {
    buffer = new uchar[qMAX_BUF_SIZE]();
}

qMessage::qMessage(uchar type) : 
    buffer(NULL), messLen(0), currentLen(0), type(type) {
        buffer = new uchar[qMAX_BUF_SIZE]();
}

qMessage::~qMessage() {
    // delete the allocated memory
    delete[] buffer;
}

void qMessage::addMessagePart(const uchar *buf, int numBytes) {
    int bytesRead = 1;
    if (messLen == 0 && (currentLen + numBytes) >= qSHORT_BYTES + 1) {
        messLen = readShort(buf, bytesRead);        // the first byte is the type
        cerr << "addMessagePart -> length = " << messLen << endl;
    }
    numBytes = numBytes > qMAX_BUF_SIZE ? qMAX_BUF_SIZE : numBytes;
    cerr << "addMessagePart -> numBytes = " << numBytes << endl;
    cerr << "addMessagePart -> currentLen = " << currentLen << endl;
    std::copy(buf, buf + numBytes, buffer + currentLen);
    currentLen += numBytes;
}

bool qMessage::isMessageReadable() {
    bool readable = (messLen == currentLen) || !type;
    cerr << string("message -> ") + (readable ? "" : "not ") + "readable" << endl;
    return readable;
}

// handles partial sends
int qMessage::send(int sock) {
    cerr << "message -> sending out " << currentLen << " bytes" << endl;
    int total = 0;
    int bytesLeft = currentLen;
    int n;
    while (total < currentLen) {
        n = ::send(sock, buffer + total, bytesLeft, 0);
        if (n == -1) {
            break;
        }
        total += n;
        bytesLeft -= n;
    }
    return (n == -1) ? -1 : 0;      // return -1 on error and 0 on success
}

ushort qMessage::readShort(const uchar *buf, int &bytesRead) {
    if (qMAX_BUF_SIZE - bytesRead >= (int) sizeof(short)) {
        ushort val;
        val = buf[bytesRead];
        val += buf[bytesRead + 1] << qCHAR_BITS;
        bytesRead += qSHORT_BYTES;
        return val;
    }
    return 0;              // the short could not be read
}

void qMessage::writeShort(ushort val, int &position) {
    if (qMAX_BUF_SIZE - position >= (int) sizeof(short)) {
        uchar low, high;
        low = val & 0xFF;
        high = val & 0xFF00;
        buffer[position] = low;
        buffer[position + 1] = high;
        position += qSHORT_BYTES;
    }
}

void qMessage::prepareToSend() {
    // add the type and the length to the buffer
    int position = 0;
    buffer[position++] = type;
    messLen = currentLen = qHEADER_LEN;
    writeShort(currentLen, position);
}

void qMessage::handleMessage(const MQ::iterator &it, qMessageStorage *ms) {
    // send to the player a message asking to resend as there was some error when reading the message
    // the error is the reason we are here (the type could not be read or was wrong)
    cerr << "message -> unidentified message received" << endl;
    qMessage *message = new qResendMessage();
    message->prepareToSend();
    ms->addMessage(messElem(it->first, message), ms->getSendingQueue());
}

void qMessage::acknowledgeMessage(const MQ::iterator &it, qMessageStorage *ms) {
    qMessage *message = new qAckMessage();
    message->prepareToSend();
    ms->addMessage(messElem(it->first, message), ms->getSendingQueue());
    cerr << "message -> read and aknowledged" << endl;
}


// qMessage classes

qAckMessage::qAckMessage() : qMessage(static_cast<uchar> (ACK)) {}

void qAckMessage::handleMessage(const MQ::iterator &it, qMessageStorage *ms) {
    ms->deleteMessage(it->first, ms->getPendingAckQueue());
    cerr << "message -> acked" << endl;
}


qPlayerInfoMessage::qPlayerInfoMessage() : qMessage(static_cast<uchar> (PLAYER_INFO)) {}

void qPlayerInfoMessage::handleMessage(const MQ::iterator &it, qMessageStorage *ms) {
    // readInformation
    int bytesRead = qHEADER_LEN;
    // 3 uchar, 1 ushort
    uchar cores, cpuInt, cpuFrac, *buf;
    ushort ping;
    buf = getBuffer();
    cores = buf[bytesRead++];
    cpuInt = buf[bytesRead++];
    cpuFrac = buf[bytesRead++];
    ping = readShort(buf, bytesRead);
    info.setProperties(cores, cpuInt, cpuFrac, ping);

    acknowledgeMessage(it, ms);
    cerr << "message -> cores = " << static_cast<int> (cores) << ", cpuInt = " << 
        static_cast<int> (cpuInt) << ", cpuFrac = " <<
        static_cast<int> (cpuFrac) << ", ping = " << 
        static_cast<int> (ping) << endl;
}

void qPlayerInfoMessage::setProperties(const PlayerInfo &information) {
    info.setProperties(information.getNumCores(), information.getCpuSpeedInt(), 
        information.getCpuSpeedFrac(), information.getPing());
}

void qPlayerInfoMessage::prepareToSend() {
    // write to the buffer
    int position = 0;
    uchar *buf = getBuffer();
    buf[position++] = getType();
    setMessageLength(qHEADER_LEN + 5);
    setCurrentLength(qHEADER_LEN + 5);
    writeShort(getCurrentLength(), position);
    buf[position++] = info.getNumCores();
    buf[position++] = info.getCpuSpeedInt();
    buf[position++] = info.getCpuSpeedFrac();
    writeShort(info.getPing(), position);
}


qMatchReadyMessage::qMatchReadyMessage() : qMessage(static_cast<uchar> (MATCH_READY)) {}

void qMatchReadyMessage::handleMessage(const MQ::iterator &it, qMessageStorage *ms) {
    // do nothing -> the server will then send the connection information to all the other players in the match
    acknowledgeMessage(it, ms);
}


qResendMessage::qResendMessage() : qMessage(static_cast<uchar> (RESEND)) {}

void qResendMessage::handleMessage(const MQ::iterator &it, qMessageStorage *ms) {
    // get the message waiting for ack and move it again to the sending queue
    MQ::iterator messageToResend = ms->getMessageFromQueue(it->first, ms->getPendingAckQueue());    // check that there is some message waiting for ack
    if (messageToResend != ms->getPendingAckQueue().end()) {
        ms->moveMessage(messageToResend, ms->getPendingAckQueue(), ms->getSendingQueue());
    }
}


qSendHostingOrder::qSendHostingOrder() : qMessage(static_cast<uchar> (SEND_HOST)) {}

void qSendHostingOrder::handleMessage(const MQ::iterator &it, qMessageStorage *ms) {
    // do nothing -> the player will start the game on the default port in its processMessages() method
    acknowledgeMessage(it, ms);
}


qSendConnectInfo::qSendConnectInfo() : qMessage(static_cast<uchar> (SEND_CONNECT)) {}

void qSendConnectInfo::handleMessage(const MQ::iterator &it, qMessageStorage *ms) {
    // readInformation
    int bytesRead = qHEADER_LEN;
    uchar *buf = getBuffer();
    family = buf[bytesRead++];
    cerr << "message -> family = " << family << ", address = ";
    for (int i = 0; i < qMAX_NET_ADDR; ++i) {
        address[i] = (char) buf[bytesRead + i];
        cerr << address[i];
    }
    cerr << endl;

    // do nothing -> the player method processMessages() will take care of connecting to the game specified
    acknowledgeMessage(it, ms);
}

void qSendConnectInfo::setProperties(uchar f, char *addr) {
    family = f;
    for (int i = 0; i < qMAX_NET_ADDR; ++i) {
        address[i] = addr[i];
    }
}

void qSendConnectInfo::prepareToSend() {
    // write to the buffer
    int position = 0;
    uchar *buf = getBuffer();
    buf[position++] = getType();
    setMessageLength(qHEADER_LEN + qMAX_NET_ADDR + 1);
    setCurrentLength(qHEADER_LEN + qMAX_NET_ADDR + 1);
    writeShort(getCurrentLength(), position);
    buf[position++] = family;
    for (int i = 0; i < qMAX_NET_ADDR; ++i) {
        buf[position + i] = (uchar) address[i];
    }
}


// NOT YET IMPLEMENTED! --> the ping will be the last thing to add

qSendPeersInfo::qSendPeersInfo() : qMessage(static_cast<uchar> (SEND_PEERS)) {}

void qSendPeersInfo::handleMessage(const MQ::iterator &it, qMessageStorage *ms) {
    // readInformation

    // do nothing -> the player method processMessges() will take care of measuring the ping to the other peers
    // and send back the server a message of qPlayerInfoMessage with the new information added
}

