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

This file will contain all the different classes needed.

*/


#ifndef QUTILITIES_H
#define QUTILITIES_H


#include <iostream>
#include <fstream>
#include <vector>
#include <utility>
#include <map>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

// Timer
#include <functional>
#include <chrono>
#include <future>
#include <cstdio>

// Network
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>


// Defines
#define qPORT        	"3490"
#define qSERVER_IP		"127.0.0.1"
#define qBACKLOG     	20      				// How many pending connections queue will hold
#define qPLAYERS    	2       				// number of players for each game
#define qMAX_BUF_SIZE 	256
#define qMAX_NET_ADDR	128
#define qCHAR_BITS 		8
#define qSHORT_BYTES	2
#define qHEADER_LEN		3


typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;


class qMessage;
class qSendConnectInfo;
class qPlayerInfoMessage;
class qPlayer;


typedef std::map<int, qPlayer*> PQ;
typedef std::map<int, qMessage*> MQ;
typedef std::pair<int, qMessage*> messElem; 		// messElem -> messageElement
typedef std::multimap<uint, qPlayer*> MatchQ; 		// uint -> matchId


enum MessageTypes { 
	ACK = 1,
	PLAYER_INFO,
	MATCH_READY,
	RESEND,
	SEND_HOST,
	SEND_CONNECT,
	SEND_PEERS
};


class qMessageStorage {
	private:
		// a message can only be in one queue at a time
		MQ receivedQueue;		// Queue for holding the messages until they are processed
		MQ sendingQueue;		// Queue for holding the messages to be sent
		MQ pendingAckQueue; 	// Queue for holding the messages awaiting to be acked

		int getSocketReferences(const MQ::iterator &it);

	public:
		qMessageStorage() {}
		virtual ~qMessageStorage();

		inline MQ &getReceivedQueue() { return receivedQueue; }
		inline MQ &getSendingQueue() { return sendingQueue; }
		inline MQ &getPendingAckQueue() { return pendingAckQueue; }
		MQ::iterator getMessageFromQueue(int sock, MQ &queue);

		void deleteAllMessages(int sock);
		void deleteMessage(MQ::iterator &it, MQ &queue);
		void deleteMessage(int sock, MQ &queue);
		void addMessage(const messElem &elem, MQ &queue);
		void moveMessage(MQ::iterator &it, MQ &originQueue, MQ &destQueue);
		void handleData(const uchar *buf, int numBytes, int sock);

		qMessage *createMessage(uchar type);
		virtual void processMessages() = 0;
		virtual void sendMessages();
		virtual void resendUnacked();
};


// CPU PROPERTIES

class PlayerInfo {
	private:
		uchar numCores;
		uchar cpuSpeedInteger;
		uchar cpuSpeedFractional;

		ushort ping; 	// averaged from the player to the rest of the players

		int getCpuInfo(uchar *nCores, uchar *cpuSI, uchar *cpuSF, ushort *p); 			// sets the values according to /proc/cpuinfo in the given pointers -> returns -1 on error

	public:
		PlayerInfo();
		PlayerInfo(uchar numCores, uchar cpuSpeedInteger, uchar cpuSpeedFractional, ushort ping);

		// getters
		inline uchar getNumCores() const { return numCores; }
		inline uchar getCpuSpeedInt() const { return cpuSpeedInteger; }
		inline uchar getCpuSpeedFrac() const { return cpuSpeedFractional; }
		inline ushort getPing() const { return ping; }

		bool isInitialized() { return numCores > 0; }
		void setProperties(uchar nCores, uchar cpuSI, uchar cpuSF, ushort p);
		int setCpuInfo();
};


// NETWORK

class qConnection {
	private:
		int sock; 									// the socket destined to the connection
		struct sockaddr_storage remoteaddr;			// client address

	public:
		qConnection();													// initializes connection with the server
		qConnection(int socket, sockaddr_storage remoteaddress);		// initializes connection to given remoteaddress
		virtual ~qConnection();

		inline int getSock() { return sock; }
		inline int *getSockAddr() { return &sock; }
		inline sockaddr_storage *getSockaddrStorage() { return &remoteaddr; }
		inline bool active() { return sock >= 0; }
};

class qPlayer : public qConnection, public qMessageStorage {
	private:
		PlayerInfo info; 			// the priority assigned to that PC to act as the server of the game based on its computer
		uint matchId; 				// the matchId of the player -> information set and used by the server only to handle pairing of players
		bool master;				// if the player has to act as master -> information set and used by the server only

	public:
		qPlayer(); 			// set current player
		qPlayer(int socket, sockaddr_storage remoteaddress);
		~qPlayer();

		inline uint getMatchId() { return matchId; }
		inline bool isMaster() { return master; }
		inline bool isPlayerAvailable() { return (matchId == 0) && info.isInitialized(); }
		inline bool gameFound() { return master || getSockaddrStorage()->ss_family; }		// if we are the master or remoteaddr to connect is initialized

		void setMatchId(uint id) { matchId = id; }
		void setMaster(bool m) { master = m; }
		void setInfo(qPlayerInfoMessage *message);
		void setConnection(qSendConnectInfo *message);
		void sendMyInfoToServer();

		void processMessages();
		bool hasBetterPC(qPlayer *other);
		int getData();

};

/*
 * This class provides all common methods to create a server. It creates a server capable of listening
 * and answering to different connections and relies in the handle data method of its derived classes
 * to perform each different task needed.
 */ 
class qServer {
	private:
		int listener;							// socket listening for connections
		
		int fdmax; 								// maximum file descriptor number
		fd_set master;          				// master file descriptor list

	public:
		qServer();
		virtual ~qServer();

		// getters
		inline fd_set *getFileDescriptorList() { return &master; }
		inline int *getFDmax() { return &fdmax; }
		inline int *getListener() { return &listener; }

		virtual void getData() = 0;											// return number of bytes read or -1 if error
};


/*
 * Class used to implement our quickplay server. The handle data will listen for the requests of
 * the different players and queue them or assign them to their corresponding match.
 */
class qServerInstance : public qServer, public qMessageStorage {
	private:
		static uint matchesIds;

		PQ playerQueue;			// Queue for holding the players that have to be paired
		MatchQ matchesQueue; 	// Queue for holding the matches before they start

		void fillClientPlayers(std::vector<qPlayer *> &cPlayers, uint matchId);
		void sendConnectMessages(const std::vector<qPlayer *> &cPlayers);

	public:
		qServerInstance();
		~qServerInstance();

		inline const PQ &getPlayerQueue() { return playerQueue; }
		inline uint getNextMatchId() { return matchesIds++; }
		inline void addPlayer(qPlayer *&newPlayer, int sock) { 
			playerQueue.insert(std::pair<int, qPlayer*>(sock, newPlayer));
		}
		inline void deletePlayer(sock) {
			PQ::iterator it = playerQueue.find(sock);
			playerQueue.erase(it);
		}
		
		qPlayer *getPlayer(int sock);

		void deletePlayerFromMatches(int sock, uint id);
		void addPlayerToMatches(qPlayer *player, uint id);
		void getData();
		void processMessages();
		int prepareMatch();					// pairs the players and puts them in the matchesQueue - returns the number of matches created
};

/*
 * This class represents a message. 
 */
class qMessage {
	private:
		uchar *buffer; 			// holds the payload of the message
		ushort messLen;			// doesn't take into account the header "message length" ushort and uchar type (length expressed in ushorts)
		ushort currentLen;		// the actual length of the message (considering also the header)

		// Properties of the message stored in the derived classes once the message is handled or when it is created to be sent
		uchar type;

	public:
		qMessage();								// default empty message
		explicit qMessage(uchar type); 			// constructor called by derived classes default constructors
		virtual ~qMessage();

		// Getters
		inline uchar *getBuffer() { return buffer; }
		inline ushort getMessageLength() { return messLen; }
		inline ushort getCurrentLength() { return currentLen; }
		inline uchar getType() { return type; }

		// Setters
		inline void setMessageLength(ushort mLen) { messLen = mLen; }
		inline void setCurrentLength(ushort cLen) { currentLen = cLen; }

		ushort readShort(const uchar *buf, int &bytesRead);			// bytesRead is a reference to a number of bytes read control variable
		void writeShort(ushort val, int &position);

		// addMessagePart is in charge to build the message when it has been divided in different parts by the network
		// it just adds new information received to the buffer until it has the specified message length
		void addMessagePart(const uchar *buf, int numShorts);			
		bool isMessageReadable();
		void acknowledgeMessage(const MQ::iterator &it, qMessageStorage *ms);

		virtual void handleMessage(const MQ::iterator &it, qMessageStorage *ms); 			// to read the message and create the corresponding derived class
		virtual int send(int sock); 														// send the message
		virtual void prepareToSend();
};

/*
 * Different classes for each type of message
 */

/*
 * This message is used to indicate acknowledge of the previous message received.
 */
class qAckMessage : public qMessage {
	public:
		qAckMessage();
		~qAckMessage() {}
		void handleMessage(const MQ::iterator &it, qMessageStorage *ms);
};

/*
 * This message is used to send the information about the player and is used to
 * put the new player in queue to join a game when received by the server. The server
 * should not use this type of message.
 */
class qPlayerInfoMessage : public qMessage {
	private:
		PlayerInfo info;

	public:
		qPlayerInfoMessage();
		~qPlayerInfoMessage() {}
		PlayerInfo getInfo() { return info; }
		void setProperties(const PlayerInfo &information);
		void prepareToSend();
		void handleMessage(const MQ::iterator &it, qMessageStorage *ms);
};

/*
 * This message is sent by the player creator of a match to the quickplay server when
 * the game is ready and the other players can join. The server should not use this type of message.
 */
class qMatchReadyMessage : public qMessage {
	public:
		qMatchReadyMessage();
		~qMatchReadyMessage() {}
		void handleMessage(const MQ::iterator &it, qMessageStorage *ms);
};

/*
 * This message is sent when it is needed some information that may have been lost.
 */
class qResendMessage : public qMessage {
	public:
		qResendMessage();
		~qResendMessage() {}
		void handleMessage(const MQ::iterator &it, qMessageStorage *ms);
};

/*
 * This message is sent by the server only to the player in charge of creating the game and hosting it.
 */
class qSendHostingOrder: public qMessage {
	public:
		qSendHostingOrder();
		~qSendHostingOrder() {}
		void handleMessage(const MQ::iterator &it, qMessageStorage *ms);
};

/*
 * This message is sent by the server only to the players that will join a game started by another player.
 */
class qSendConnectInfo: public qMessage {
	private:
		uchar family;
		char address[qMAX_NET_ADDR]; 		// 16 bytes is for ipV6 addresses, which is the maximum
		// The port is not necessary -> always the default one

	public:
		qSendConnectInfo();
		~qSendConnectInfo() {}
		void handleMessage(const MQ::iterator &it, qMessageStorage *ms);
		void setProperties(uchar f, char *addr);
		void prepareToSend();

		inline uchar getFamily() { return family; }
		inline char *getAddress() { return address; }
};

/*
 * This message is sent by the server to inform a player of his peers in the game, so that he/she
 * can measure the ping between him and them and do the average, which is sent back to the server
 * in a qPlayerInfo message.
 */
class qSendPeersInfo: public qMessage {
	public:
		qSendPeersInfo();
		~qSendPeersInfo() {}
		void handleMessage(const MQ::iterator &it, qMessageStorage *ms);
};


#endif
