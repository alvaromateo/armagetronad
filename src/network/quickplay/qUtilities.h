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

		virtual int getData();												// return number of bytes read or -1 if error
		virtual unsigned short readShort(short *&buf, int &count);			// count is a reference to a number of bytes read control variable
};

/*
 * Class used to implement our quickplay server. The handle data will listen for the requests of
 * the different players and queue them or assign them to their corresponding match.
 */
class qServerInstance : public qServer {
	private:
		map<unsigned int, qPlayer> playerQueue;			// Queue for holding the players that have to be paired
		map<unsigned int, qMessage> messageQueue;		// Queue for holding the messages until they are processed
		map<unsigned int, qMessage> sendingQueue;		// Queue for holding the messages to be sent by the server

		void handleData(short *&buf, int numBytes, int sock); 		// reads the information received in a socket and returns -1 on error

	public:
		qServerInstance() { setActiveServer(&this); }		// when a qServerInstance is created is automatically set to be the quickplayActiveServer

		inline const map<unsigned int, qPlayer>& getPlayerQueue() { return playerQueue; }
		inline const map<unsigned int, qMessage>& getMessageQueue() { return messageQueue; }
		inline const map<unsigned int, qMessage>& getSendingQueue() { return sendingQueue; }

		qPlayer *getPlayer(int sock) { return &playerQueue[sock]; } 		// return the player which has sock assigned to its connection

		void processMessages();
};

/*
 * This class represents a message. 
 */
class qMessage {
	private:
		short *buffer; 			// holds the payload of the message
		size_t messLen;			// doesn't take into account the header "message length" short (2 bytes)
		size_t currentLen;
		qPlayer *owner;

		// Properties of the message
		short type;

	public:
		qMessage();								// default empty message
		qMessage(size_t messLen, int sock);
		~qMessage();
		// TODO: copy and move operators

		int addMessgePart(const short *buf, int numBytes);
		bool isMessageReadable() { return messLen == currentLen; }

		void handleMessage();
};

void setActiveServer(qServerInstance *instance);


// CPU PROPERTIES

class PlayerCPU {
	private:
		int val;

	public:
		PlayerCPU();
};


#endif