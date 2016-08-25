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

*/

#include "tConfiguration.h"
#include "tDirectories.h"
#include "nNetwork.h"
#include "nServerInfo.h"
#include "tSysTime.h"
#include "tLocale.h"
#include "tCommandLine.h"
#include "nSocket.h"
#include  <time.h>

REAL save_interval = 300.0f;
static tSettingItem< REAL > si( "MASTER_SAVE_INTERVAL", save_interval );

REAL query_interval = 10.0f;
static tSettingItem< REAL > qi( "MASTER_QUERY_INTERVAL", query_interval );

int master_port = 4533;
static tSettingItem< int > mp( "MASTER_PORT", master_port );

REAL master_idle = 2;
static tSettingItem< REAL > mi( "MASTER_IDLE", master_idle );

// Console with filter for better machine readable log format
class nConsoleDateFilter:public tConsoleFilter{
private:
    virtual void DoFilterLine( tString &line )
    {
        char szTemp[128];
        time_t     now;
        struct tm *pTime;
        now = time(NULL);
        pTime = localtime(&now);
        strftime(szTemp,sizeof(szTemp),"[%Y/%m/%d %H:%M:%S] ",pTime);

        tString orig = line;
        line = "";
        line << szTemp  << orig;
    }

    virtual int DoGetPriority() const{ return 1; }
};

static nConsoleDateFilter sn_consoleFilter;

int main(int argc, char** argv)
{
    tCommandLineData commandLine;
    commandLine.programVersion_ = &sn_programVersion;
    commandLine.Analyse(argc, argv);
    tLocale::Load("languages.txt");
    atexit(tLocale::Clear);

    // Loads configuration from files
    st_LoadConfig();
    // Loads different master servers from file "master.srv" --> not needed for quickplay
    // nServerInfo::GetMasters();

    nServerInfo::Load( tDirectories::Config(), "master_quickplay.srv" );
    // Set query type to get the info for a server to play
    nServerInfo::StartQueryAll(QUERY_QUICKPLAY);

    sn_serverPort = master_port;
    // sn_SetNetState starts listener sockets
    sn_SetNetState(nSERVER);

    nTimeAbsolute savetimeout  = tSysTimeFloat();
    nTimeAbsolute querytimeout = tSysTimeFloat();
    // won't put quit timeout so that the server doesn't shut down while in the middle of the presentation
    // nTimeAbsolute quitTimeout  = tSysTimeFloat() + master_idle * 3600;

    bool goon = true;
    while ( goon )
    {
        nServerInfo::RunMaster();

        sn_BasicNetworkSystem.Select( .1f );
        tAdvanceFrame();
        nTimeAbsolute time = tSysTimeFloat();

        sn_Receive();
        sn_ReceiveFromControlSocket();
        sn_SendPlanned();

        static bool queryGoesOn = true;
        if (queryGoesOn && time > querytimeout)
        {
            queryGoesOn = nServerInfo::DoQueryAll(1);
            querytimeout = time + query_interval;
        }

        if (time > savetimeout)
        {
            nServerInfo::Save( tDirectories::Var(), "master_quickplay_list.srv" );
            if (!queryGoesOn)
            {
                nServerInfo::StartQueryAll();
                queryGoesOn = true;
            }
            savetimeout = time + save_interval;

            // goon = time < quitTimeout;
        }
    }

    nServerInfo::DeleteAll();

    return(0);
}
