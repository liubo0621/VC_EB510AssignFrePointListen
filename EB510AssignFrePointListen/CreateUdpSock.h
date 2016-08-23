/*******************************************************************************
********************************************************************************
** COPYRIGHT:      (c) 1995-1997 Rohde & Schwarz, Munich
** MODULE:         CreateUdpSock.h
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

/* EXPORT */

/* GLOBAL VARIABLES DEFINITION *************************************************/

/* GLOBAL CONSTANTS DEFINITION *************************************************/

/* GLOBAL DEFINES **************************************************************/
#ifndef __CREATE_UDP_SOCK_DEF
#define __CREATE_UDP_SOCK_DEF

class CEB200UdpSock;

CEB200UdpSock *CreateUdpSock( bool bTcp, unsigned short TracPort, unsigned long ulRemoteAddress, unsigned short RecvPort = 0 );

#endif
/* LOCAL DEFINES ***************************************************************/

/* LOCAL TYPES DECLARATION *****************************************************/

/* LOCAL CLASSES DECLARATION ***************************************************/

/* LOCAL VARIABLES DEFINITION **************************************************/

/* LOCAL CONSTANTS DEFINITION **************************************************/

/* LOCAL FUNCTIONS DEFINITION **************************************************/

/* GLOBAL FUNCTIONS DEFINITION *************************************************/
