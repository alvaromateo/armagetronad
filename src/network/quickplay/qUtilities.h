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


#define SERVER_IP	"79.153.71.108"


// NETWORK

class qConnection {
	private:
		int sock; 									// the socket destined to the connection
		struct sockaddr_storage remoteaddr;			// client address
		socklen_t addrlen;							// length of the address (IPv4 or IPv6)

	public:
		qConnection(int socket, struct sockaddr_storage remoteaddress);
		void readData(char *buf); 				// fills the buffer with data from the socket of the connection
		void sendData(const char *buf);			// sends to the remote connection the data in the buffer
};


class qPlayer: qConnection {
	private:
		int PCvalue; 					// the priority assigned to that PC to act as the server of the game based on its computer
		qConnection *matchServer;		// the server to which connect to start the match, set by reading the response of the server

	public:
		qPlayer();
		qPlayer(int socket, struct sockaddr_storage remoteaddress);

		bool isMatchReady();
};


class qServer: qConnection {
	private:
		int param;

	public:
		qServer();
};


// MATCH CREATION

class qMatchCreator {
	private:
		static vector<qPlayer> playerQueue;     // Queue for holding the players that have to be paired

	public:
		qMatch();
};


#endif