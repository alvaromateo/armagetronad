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

1) Only necessary first time: ./configure --enable-master DEBUGLEVEL=3 CODELEVEL=2
2) make && make install
3) make run

If some of the languages strings are added, then re-run ./bootstrap.sh !!

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

int main(int argc, char **argv) {
    // the players should have some type of timeout to resend automatically messages if they have
    // not been acked by the server
    qServerInstance quickServer();
    while (1) {
        // handle connections
        quickServer.getData();
        quickServer.processMessages();
        quickServer.sendMessages();
        // put players together for a game
        if (quickServer.enoughPlayersReady()) {
            quickServer.prepareMatch();
        }
    }
}


