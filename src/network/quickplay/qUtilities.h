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


#include <pair>
#include <map>


// Defines
#define PORT        	"3490"
#define SERVER_IP		"79.153.71.108"
#define BACKLOG     	20      				// How many pending connections queue will hold
#define QPLAYERS    	2       				// number of players for each game
#define MAX_BUF_SIZE 	1024


typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;


// NETWORK

class qConnection {
	private:
		int sock; 									// the socket destined to the connection
		struct sockaddr_storage remoteaddr;			// client address
		socklen_t addrlen;							// length of the address (IPv4 or IPv6)

	public:
		qConnection();																	// initializes connection with the server
		qConnection(int socket, sockaddr_storage remoteaddress, socklen_t addrl);		// initializes connection to given remoteaddress
		void readData(char *buf); 														// fills the buffer with data from the socket of the connection
		void sendData(const char *buf);													// sends to the remote connection the data in the buffer
};

class qPlayer : public qConnection {
	private:
		PlayerCPU cpu; 					// the priority assigned to that PC to act as the server of the game based on its computer
		qConnection *matchServer;		// the server to which connect to start the match, set by reading the response of the server

	public:
		qPlayer();
		qPlayer(int socket, sockaddr_storage remoteaddress, socklen_t addrl);

		bool isMatchReady();
};

class qMessage;

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

		virtual void handleData(short *&buf, int numBytes, int sock) = 0;

	public:
		qServer();

		// getters
		inline fd_set getFileDescriptorList() { return master; }
		inline int getFDmax() { return fdmax; }
		inline int getListener() { return listener; }

		virtual int getData();										// return number of bytes read or -1 if error
		virtual short readShort(short *&buf, int &count);			// count is a reference to a number of bytes read control variable
};

typedef map<unsigned int, *qPlayer> PQ;
typedef map<unsigned int, *qMessage> MQ;

/*
 * Class used to implement our quickplay server. The handle data will listen for the requests of
 * the different players and queue them or assign them to their corresponding match.
 */
class qServerInstance : public qServer {
	private:
		PQ playerQueue;			// Queue for holding the players that have to be paired
		MQ messageQueue;		// Queue for holding the messages until they are processed
		MQ sendingQueue;		// Queue for holding the messages to be sent by the server

		void handleData(uchar *&buf, int numBytes, int sock); 		// reads the information received in a socket and returns -1 on error
		qMessage *createMessage(uchar type);

	public:
		qServerInstance() { setActiveServer(&this); }		// when a qServerInstance is created is automatically set to be the quickplayActiveServer
		~qServerInstance();

		inline const PQ &getPlayerQueue() { return playerQueue; }
		inline const MQ &getMessageQueue() { return messageQueue; }
		inline const MQ &getSendingQueue() { return sendingQueue; }
		inline qPlayer *getPlayer(int sock) { return playerQueue[sock]; } 			// return the player which has sock assigned to its connection

		void deleteMessage(MQ::iterator &it, MQ &queue);
		void processMessages();
};

/*
 * This class represents a message. 
 */
class qMessage {
	private:
		uchar *buffer; 			// holds the payload of the message
		short messLen;			// doesn't take into account the header "message length" short (length expressed in shorts)
		short currentLen;

		// Properties of the message stored in the derived classes once the message is handled or when it is created to be sent
		uchar type;

	public:
		qMessage();								// default empty message
		qMessage(uchar type); 					// constructor called by derived classes default constructors
		~qMessage();
		// TODO: copy and move operators

		// Getters
		inline short *getBuffer() { return buffer; }
		inline size_t getMessageLength() { return messLen; }
		inline size_t getCurrentLength() { return currentLen; }

		void addMessgePart(const short *buf, int numShorts);
		bool isMessageReadable() { return (type != 0) && isMessageComplete(); }
		bool isMessageComplete() { return messLen >= currentLen; }

		virtual void handleMessage(); 				// to read the message and create the corresponding derived class
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

		void handleMessage();
};

/*
 * This message is used to send the information about the player and is used to
 * put the new player in queue to join a game when received by the server. The server
 * should not use this type of message.
 */
class qPlayerInfoMessage : public qMessage {
	public:
		qPlayerInfoMessage();

		void handleMessage();
};

/*
 * This message is sent by the player creator of a match to the quickplay server when
 * the game is ready and the other players can join. The server should not use this type of message.
 */
class qMatchReadyMessage : public qMessage {
	public:
		qMatchReadyMessage();

		void handleMessage();
};

/*
 * This message is sent when it is needed some information that may have been lost.
 */
class qResendMessage : public qMessage {
	public:
		qResendMessage();

		void handleMessage();
};


// CPU PROPERTIES

class PlayerCPU {
	private:
		int val;

	public:
		PlayerCPU();
};


// GLOBAL FUNCTIONS

void setActiveServer(qServerInstance *instance);


#endif