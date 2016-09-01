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

This file will contain all the different classes needed.

*/


#ifndef QUTILITIES_H
#define QUTILITIES_H


// Defines
#define PORT        "3490"
#define SERVER_IP	"79.153.71.108"
#define BACKLOG     20      				// How many pending connections queue will hold
#define QPLAYERS    2       				// number of players for each game


// NETWORK

class qConnection {
	private:
		int sock; 									// the socket destined to the connection
		struct ockaddr_storage remoteaddr;				// client address
		socklen_t addrlen;							// length of the address (IPv4 or IPv6)

	public:
		qConnection();																// initializes connection with the server
		qConnection(int socket, sockaddr_storage remoteaddress);					// initializes connection to given remoteaddress
		void readData(char *buf); 													// fills the buffer with data from the socket of the connection
		void sendData(const char *buf);												// sends to the remote connection the data in the buffer
};


class qPlayer : public qConnection {
	private:
		PlayerCPU cpu; 					// the priority assigned to that PC to act as the server of the game based on its computer
		qConnection *matchServer;		// the server to which connect to start the match, set by reading the response of the server

	public:
		qPlayer();
		qPlayer(int socket, sockaddr_storage remoteaddress);

		bool isMatchReady();
};


class qServer {
	private:
		int listener;							// socket listening for connections
		static vector<qPlayer> playerQueue;		// Queue for holding the players that have to be paired

		int fdmax; 								// maximum file descriptor number
		fd_set master;          				// master file descriptor list

	public:
		qServer();

		// getters
		inline fd_set getFileDescriptorList() { return master; }
		inline int getFDmax() { return fdmax; }
		inline int getListener() { return listener; }
		inline const vector<qPlayer>& getPlayerQueue() { return playerQueue; }

		const vector<qPlayer>& addPlayer(const qPlayer &player);
		const vector<qPlayer>& removePlayer(const qPlayer &player);
		const vector<qPlayer>& removePlayer(int index);

		int getData();

		virtual int handleData() = 0;
};

class qServerInstance : public qServer {
	public:
		int handleData();
};


// CPU PROPERTIES

class PlayerCPU {
	private:
		int val;

	public:
		PlayerCPU();
};


#endif