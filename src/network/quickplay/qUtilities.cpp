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


// qMessageStorage methods

qMessageStorage::~qMessageStorage() {
    // delete messages
    MQ::iterator receivedIt, sendIt, pendingIt;
    for (receivedIt = receivedQueue.begin(); receivedIt != receivedQueue.end(); ++receivedIt) {
        delete receivedIt->second;
    }
    for (sendIt = sendingQueue.begin(); sendIt != sendingQueue.end(); ++sendIt) {
        delete sendIt->second;
    }
    for (pendingIt = pendingAckQueue.begin(); pendingIt != pendingAckQueue.end(); ++pendingIt) {
        delete pendingIt->second;
    }
}

/*
 * Iterator it has to point to a valid element. No checks are made in this method.
 */
void qMessageStorage::deleteMessage(MQ::iterator it, MQ &queue) {
    // delete the pointer to the message only if the message is only in one queue
    if (getSocketReferences(it) <= 1) {
        cout << "message storage -> deleting message from memory\n";
        delete it->second;
        it->second = NULL;
    }
    cout << "message storage -> deleting message from queue\n";
    queue.erase(it);
}

void qMessageStorage::deleteMessage(int sock, MQ &queue) {
    MQ::iterator it = queue.find(sock);
    if (it != queue.end()) {
        deleteMessage(it, queue);
    }
}

/*
 * Move element pointed by it from originQueue to destQueue.
 */
void qMessageStorage::moveMessage(MQ::iterator it, MQ &originQueue, MQ &destQueue) {
    cout << "message storage -> moving message from queue:\n";
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
    return queue.find(sock);
}

/*
 * Adds a message to the given queue
 */
void qMessageStorage::addMessage(const messElem &elem, MQ &queue) {
    cout << "message storage -> adding message to queue\n";
    queue.insert(elem);
}

// qPlayer has to redefine this method to move the message to the ack queue and set a timeout
// moveMessage(it, sendingQueue, pendingAckQueue);
void qMessageStorage::sendMessages() {
    MQ::iterator it = sendingQueue.begin();
    while (it != sendingQueue.end()) {
        if (it->second->send(it->first) < 0) {      // send message and get error status
            cout << "sending message -> some error occurred - trying to resend...\n";
        }
        // move message to ack queue
        moveMessage(it, sendingQueue, pendingAckQueue);
        ++it;
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
    return ret;
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
    uchar *nCores, *cpuSI, *cpuSF;
    ushort *p;
    int res = getCpuInfo(nCores, cpuSI, cpuSF, p);
    if (res >= 0) {
        setProperties(*nCores, *cpuSI, *cpuSF, *p);
    }
    return res;
}

int PlayerInfo::getCpuInfo(uchar *nCores, uchar *cpuSI, uchar *cpuSF, ushort *p) {
    // TODO: implement correctly this method so that it reads from the cpu information of the system
    *nCores = 1;
    *cpuSI = 0;
    *cpuSF = 0;
    *p = 0;
}


// qConnection methods

qConnection::qConnection() {

}

qConnection::qConnection(int socket, sockaddr_storage remoteaddress, socklen_t addrl) {

}


// qPlayer methods

qPlayer::qPlayer() {}

qPlayer::qPlayer(int socket, sockaddr_storage remoteaddress, socklen_t addrl) {}

qPlayer::~qPlayer() {}

void qPlayer::processMessages() {}

void qPlayer::setInfo(qPlayerInfoMessage *message) {
    info = message->getInfo();
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
        cerr << "server getaddrinfo: " << gai_strerror(retval) << endl;
        exit(1);
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        this->listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (this->listener == -1) {
            cout << "socked failed\n";
            continue;   // there was an error when opening the file, so we look into the next addrinfo stored in servinfo
        }

        retval = fcntl(this->listener, F_SETFL, O_NONBLOCK);
        if (retval == -1) {
            perror("fcntl failed\n");
            exit(2);
        }

        retval = setsockopt(this->listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (retval == -1) {
            perror("setsockopt failed\n");    // this sets an option on the sockets to avoid error messages about port already in use when restarting the server
            exit(3);
        }

        retval = bind(this->listener, p->ai_addr, p->ai_addrlen);
        if (retval == -1) {
            close(this->listener);
            cout << "server bind failed\n";
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
        printf("server IP address: %s", str);

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

    cout << "Server started\n";
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
                    cout << "server -> select: new connection on socket " << i << endl;

                    qPlayer *newPlayer = new qPlayer(newfd, remoteaddr, addrlen);     // create new player
                    addPlayer(newPlayer, newfd);                                     // add player to the queue of players
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
                        // TODO: delete the player from the playerQueue and from possible matchesQueue (in case the client disconnected)
                    } else {
                        perror("server -> recv failed\n");
                    }

                    // we don't need this socket any more
                    close(i);
                    FD_CLR(i, master);     // remove it from master set
                }  else {
                    // we got some data from a client -> handle it
                    cout << "server -> data ready for socket " << i << "\n";
                    handleData(buf, nbytes, i);
                }
            }       // end handle data from client
        }       // end of FD_ISSET
    }       // end of looping through the existing connections

}

void qServerInstance::handleData(const uchar *buf, int numBytes, int sock) {
    uchar type = 0;

    // find if there is already a message started in the socket
    MQ::iterator it = getReceivedQueue().find(sock);
    if (it != getReceivedQueue().end()) {
        // message already exists -> add the new part to it
        (it->second)->addMessagePart(buf, numBytes);
    } else {
        // new message to handle -> create it and add the just received part
        type = buf[0];

        qMessage *message = createMessage(type);
        message->addMessagePart(buf, numBytes);
        // add the message to the map
        addMessage(messElem(sock, message), getReceivedQueue());
    }
}

void qServerInstance::deletePlayerFromMatches(int sock, uint id) {
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
    matchesQueue.insert(pair<uint, qPlayer*>(id, player));
}

void qServerInstance::processMessages() {
    MQ::iterator it = getReceivedQueue().begin();
    while (it != getReceivedQueue().end()) {
        if ((it->second)->isMessageReadable()) {
            (it->second)->handleMessage(it, this);
            // for some cases of messages received the server has to do extra work
            vector<qPlayer *> clientPlayers;
            qPlayer *player;
            switch(it->second->getType()) {
                case PLAYER_INFO:
                    // get player and add info to it
                    player = playerQueue.find(it->first)->second;
                    player->setInfo(dynamic_cast<qPlayerInfoMessage *>(it->second));
                    break;
                case MATCH_READY:
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
            }
            deleteMessage(it, getReceivedQueue());        // the message has to be deleted to free memory
        }
        ++it;
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
    for (vector<qPlayer *>::const_iterator it = cPlayers.cbegin(); it != cPlayers.cend(); ++it) {
        cout << "server -> storing connection information for client\n";

        // fill message with information
        uchar nFamily;
        char nAddress[qMAX_NET_ADDR];
        struct sockaddr_storage connection = (*it)->getSockaddrStorage();
        if (connection.ss_family == AF_INET) {
            // ipV4
            nFamily = 0;
            struct sockaddr_in *dest = (struct sockaddr_in *) &connection;
            inet_ntop(AF_INET, &(dest->sin_addr), nAddress, qMAX_NET_ADDR);
        } else {
            // ipV6
            nFamily = 1;
            struct sockaddr_in6 *dest = (struct sockaddr_in6 *) &connection;
            inet_ntop(AF_INET6, &(dest->sin6_addr), nAddress, qMAX_NET_ADDR);
        }

        // send message to player
        qSendConnectInfo *message = new qSendConnectInfo();
        message->setProperties(nFamily, nAddress);
        message->prepareToSend();
        addMessage(messElem((*it)->getSock(), message), getSendingQueue());
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
    }
    numBytes = numBytes > qMAX_BUF_SIZE ? qMAX_BUF_SIZE : numBytes;
    std::copy(buf, buf + numBytes, buffer + currentLen);
    currentLen += numBytes;
}

// handles partial sends
int qMessage::send(int sock) {
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
    qMessage *message = new qResendMessage();
    message->prepareToSend();
    ms->addMessage(messElem(it->first, message), ms->getSendingQueue());
}

void qMessage::acknowledgeMessage(const MQ::iterator &it, qMessageStorage *ms) {
    qMessage *message = new qAckMessage();
    message->prepareToSend();
    ms->addMessage(messElem(it->first, message), ms->getSendingQueue());
    cout << "message - read and aknowledged\n";
}


// qMessage classes

qAckMessage::qAckMessage() : qMessage(1) {}

void qAckMessage::handleMessage(const MQ::iterator &it, qMessageStorage *ms) {
    ms->deleteMessage(it->first, ms->getPendingAckQueue());
}


qPlayerInfoMessage::qPlayerInfoMessage() : qMessage(2) {}

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
}


qMatchReadyMessage::qMatchReadyMessage() : qMessage(3) {}

void qMatchReadyMessage::handleMessage(const MQ::iterator &it, qMessageStorage *ms) {
    // do nothing -> the server will then send the connection information to all the other players in the match
    acknowledgeMessage(it, ms);
}


qResendMessage::qResendMessage() : qMessage(4) {}

void qResendMessage::handleMessage(const MQ::iterator &it, qMessageStorage *ms) {
    // get the message waiting for ack and move it again to the sending queue
    ms->moveMessage(ms->getMessageFromQueue(it->first, ms->getPendingAckQueue()), 
        ms->getPendingAckQueue(), ms->getSendingQueue());
}


qSendHostingOrder::qSendHostingOrder() : qMessage(5) {}

void qSendHostingOrder::handleMessage(const MQ::iterator &it, qMessageStorage *ms) {
    // do nothing -> the player will start the game on the default port in its processMessages() method
    acknowledgeMessage(it, ms);
}


qSendConnectInfo::qSendConnectInfo() : qMessage(6) {}

void qSendConnectInfo::handleMessage(const MQ::iterator &it, qMessageStorage *ms) {
    // readInformation
    int bytesRead = qHEADER_LEN;
    uchar *buf = getBuffer();
    family = buf[bytesRead++];
    for (int i = 0; i < qMAX_NET_ADDR; ++i) {
        address[i] = (char) buf[bytesRead + i];
    }

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

qSendPeersInfo::qSendPeersInfo() : qMessage(7) {}

void qSendPeersInfo::handleMessage(const MQ::iterator &it, qMessageStorage *ms) {
    // readInformation

    // do nothing -> the player method processMessges() will take care of measuring the ping to the other peers
    // and send back the server a message of qPlayerInfoMessage with the new information added
}


// PlayerInfo methods





// Timer

Timer::Timer(int after, TimerCallbacks tc) {           // add arguments after tc, if needed
    std::this_thread::sleep_for(std::chrono::milliseconds(after));
    switch (tc) {
        case RESEND_TIMEOUT:
            task();
        default:
            defaultTask();
    }
}


