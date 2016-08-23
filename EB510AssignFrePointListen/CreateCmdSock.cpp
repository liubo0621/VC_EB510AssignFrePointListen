/*******************************************************************************
********************************************************************************
** COPYRIGHT:      (c) 2011-2013 Rohde & Schwarz, Munich
** MODULE:         CreateCmdSock.cpp
** ABBREVIATION:   
** COMPILER:       Visual Studio 6.0
** LANGUAGE:       C/C++
** AUTHOR:         Christian Pössinger
** ABSTRACT:       
** PREMISES:       
** REMARKS:        
** HISTORY:        
**	2011-12-29: (Pö)	Creation
** REVIEW:         
********************************************************************************/

/* INCLUDE FILES ***************************************************************/

/* IMPORT */

/* EXPORT */
#include "CreateCmdSock.h"

/* GLOBAL VARIABLES DEFINITION *************************************************/

/* GLOBAL CONSTANTS DEFINITION *************************************************/

/* GLOBAL DEFINES **************************************************************/

/* LOCAL DEFINES ***************************************************************/

/* LOCAL TYPES DECLARATION *****************************************************/

/* LOCAL CLASSES DECLARATION ***************************************************/

/* LOCAL VARIABLES DEFINITION **************************************************/

/* LOCAL CONSTANTS DEFINITION **************************************************/

/* LOCAL FUNCTIONS DEFINITION **************************************************/

CCmdSock *CreateCmdSock(void)
{
    return new CCmdSock( );
}

/* GLOBAL FUNCTIONS DEFINITION *************************************************/
