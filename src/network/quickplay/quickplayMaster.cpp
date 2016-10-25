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

ATENTION!

This server is only guaranteed to run on GNU/Linux distributions

*/

/*
    BUILD!

1) Only necessary first time: ./configure --enable-master CODELEVEL=2 CPPFLAGS="-std=c++11 -stdlib=libc++ -pthread"
    To debug add -> DEBUGLEVEL=3
    On mac add -> LIBS="-framework OpenGL -framework GLUT" 
2) make && make install
3) make run

If some of the languages strings are added, then re-run ./bootstrap.sh !!

Current build command:
    ./configure --enable-master CODELEVEL=2 CPPFLAGS="-std=c++11 -stdlib=libc++ -pthread" LIBS="-framework OpenGL -framework GLUT" CXX="clang++" CC="clang++"

*/


#include "qUtilities.h"

/*
 * Implementar ping entre jugadors per ajudar a decidir al servidor
 * Ack system
 * Maquines virtuals per fer demo
 */

/*
 * No se hace multithreading porque los paquetes que se envían se procesan muy rápidamente, ya que no 
 * hay que hacer cálculos ni renderizar nada. Es por esto, que en lugar de crear un thread para atender
 * cada conexión con un cliente se usa un select(), que se encarga de mirar cuando hay disponible
 * información en alguno de los sockets usados. Cuando hay información disponible se procesa rápidamente
 * y se continua esperando a recibir más información.
 */

 /* 
 * 1) if we are the server we don't care about ack's -> if the client has not received the message he will send a qResendMessage
 *      when a timeout expires
 * 2) if we are the client and we receive an ack -> we remove the message from pending ack
 * 3) if we are the client we set a timeout after sending a message -> if the ack doesn't arrive after the timeout finishes
 *      we resend the package --> repeat X number of times before stop trying
 * 
 * http://stackoverflow.com/questions/14650885/how-to-create-timer-events-using-c-11
 * http://stackoverflow.com/questions/19022320/implementing-timer-with-timeout-handler-in-c
 *
 */

int main(int argc, char **argv) {
    // the players should have some type of timeout to resend automatically messages if they have
    // not been acked by the server
    int matchesCreated;
    qServerInstance quickServer;
    while (1) {
        // put players together for a game
        matchesCreated = quickServer.prepareMatch();
        quickServer.processMessages();
        quickServer.sendMessages();
        if (matchesCreated > 0) {
            std::cout << "server -> created " << matchesCreated << " new match/es\n";
        }
        // handle connections
        quickServer.getData();
    }
}


