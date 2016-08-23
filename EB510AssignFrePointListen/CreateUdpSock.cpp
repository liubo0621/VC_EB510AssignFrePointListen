/*******************************************************************************
********************************************************************************
** COPYRIGHT:      (c) 1995-1997 Rohde & Schwarz, Munich
** MODULE:         CreateUdpSock.cpp
** ABBREVIATION:   
** COMPILER:       DiabData V4.0b, V4.1a
** LANGUAGE:       C/C++
** AUTHOR:         Martin Hisch
** ABSTRACT:       
** PREMISES:       
** REMARKS:        
** HISTORY:        
**	2011-2-21: (Hh)	Creation
** REVIEW:         
********************************************************************************/

/* INCLUDE FILES ***************************************************************/

/* IMPORT */
#include <stdio.h>
#include <assert.h>
#include <winsock.h>
#include <math.h>
#include <conio.h>
#include <stddef.h>
#include <cstdarg>
#include "Eb200UdpSock.h"
/* EXPORT */
#include "CreateUdpSock.h"

/* GLOBAL VARIABLES DEFINITION *************************************************/

/* GLOBAL CONSTANTS DEFINITION *************************************************/

/* GLOBAL DEFINES **************************************************************/

/* LOCAL DEFINES ***************************************************************/

/* LOCAL TYPES DECLARATION *****************************************************/

/* LOCAL CLASSES DECLARATION ***************************************************/

/* LOCAL VARIABLES DEFINITION **************************************************/

/* LOCAL CONSTANTS DEFINITION **************************************************/

/* LOCAL FUNCTIONS DEFINITION **************************************************/

CEB200UdpSock *CreateUdpSock( bool bTcp, unsigned short TracPort, unsigned long ulRemoteAddress, unsigned short RecvPort)
{
    return new CEB200UdpSock( bTcp, TracPort, ulRemoteAddress, RecvPort);
}


/* GLOBAL FUNCTIONS DEFINITION *************************************************/
