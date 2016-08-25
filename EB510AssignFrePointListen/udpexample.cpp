/*******************************************************************************
********************************************************************************
** COPYRIGHT:    (c) 1995-2011 Rohde & Schwarz, Munich
** MODULE:       UdpExample.cpp
** ABBREVIATION:
** COMPILER:     Visual C++ 5.0
** LANGUAGE:     C/C++
** AUTHOR:       Martin Hisch
** ABSTRACT:     Example for receiving EB200/ESMB UDP packets
** PREMISES:
** REMARKS:
** HISTORY:
**  2012-04-24: (ko)  Version 5.54: DFPAN results can be written to CSV file (temporarily solution)
**  2012-04-11: (Mue) Version 5.53: Support of DF Bandwidth resolution mHz
**  2012-03-23: (Mue) Version 5.51: Support of fractional step
**  2011-12-13: (Poe) Created control-socket-class to setup DUT
**  2011-05-13: (Mue) Version 5.19: Fixed bug: support changing port number now.
**                    Field Strength invalid flag
**  2010-11-17: (Ku)  Version 5.18: Ausgabe des PIFPAN - Modes (Histogram=1; Pulse=2)
**  2005-10-05: (Hh)  Jetzt auch endlich fuer EM510
**  2005-07-05: (Mue) Version 3.91 Auch EM550
**  2005-06-30: (Ob)  Version 3.91 Added WAV File support (Audiorecording)
**  2004-10-06: (Mue) Version 3.90 Ausgabe nur Channels/sec mit Parameter -c
**  2004-02-19: (Hh)  Version 3.8: Added VideoPan and PSCAN support
**  2003-09-11: (FB)  Version 3.71: Added WAV file support
**  2003-07-23: (Mue) Audio:MODE mit "E" ; CW ist nicht abhaengig von Option isCM
**                    Version 3.70
**  2001-01-31: (Ks)  Software option EB200FS, EB200CM
**  2000-05-29: (Hh)  Creation
** REVIEW:
********************************************************************************/

/* INCLUDE FILES ***************************************************************/

/* IMPORT */
#include <stdio.h>
#include <assert.h>

#include "eb200udpsock.h"
#include "CreateUdpSock.h"

#include "CmdSock.h"
#include "CreateCmdSock.h"

#include "UdpExample.h"

/* EXPORT */

/* GLOBAL VARIABLES DEFINITION *************************************************/

/* GLOBAL CONSTANTS DEFINITION *************************************************/

/* GLOBAL DEFINES **************************************************************/

/* LOCAL DEFINES ***************************************************************/
#define ASSERT  assert
//#define SWAP_DEF

/* LOCAL TYPES DECLARATION *****************************************************/

/* LOCAL CLASSES DECLARATION ***************************************************/

/* LOCAL VARIABLES DEFINITION **************************************************/

/* LOCAL CONSTANTS DEFINITION **************************************************/

/* LOCAL FUNCTIONS DEFINITION **************************************************/

/* GLOBAL FUNCTIONS DEFINITION *************************************************/

/* FUNCTION ********************************************************************/
int UdpExample::udpMain(int argc, char **argv)
/*
SPECIFICATION:
  main function for this example
PARAMETERS:
  int argc    : Number of command line arguments
  char **argv : Pointer to command line arguments
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
    {
        /* Tell the user that we couldn't find a usable */
        /* WinSock DLL.                  */
        printf("Error retrieving windows socket dll.\n");
        return 1;
    }

    /* Confirm that the WinSock DLL supports 2.0.*/
    /* Note that if the DLL supports versions greater  */
    /* than 2.0 in addition to 2.0, it will still return */
    /* 2.0 in wVersion since that is the version we    */
    /* requested.                    */
    if ( LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 0 )
    {
        /* Tell the user that we couldn't find a usable */
        /* WinSock DLL.                  */
        printf("Error retrieving correct winsock version 2.0.\n");
        WSACleanup();
        return 1;
    }

    /* The WinSock DLL is acceptable. Proceed. */

    /* if receive (only) port was not set -> setup SCPI connection for config. */
    CCmdSock *pCmdSock = CreateCmdSock();
    ASSERT(pCmdSock != NULL);

    pCmdSock->Init( argc, argv );

    /* stop data transfer from receiver device */
    pCmdSock->DeleteTraces();

    /* finally check for errors merely to sync to the TRAC:<UDP|TCP>:DEL command processing above */
    /* This is more robust than relying on the (optional) socket shutdown mechanism. */
    pCmdSock->CheckDeviceErrors();

    delete pCmdSock;
    pCmdSock = NULL;

    return 0;
}

void UdpExample::stopWrite() {
	if (pCmdSock)
	{
		pCmdSock->stopWrite();
	}
}

