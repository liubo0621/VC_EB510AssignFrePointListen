/*****************************************************************************
******************************************************************************
** COPYRIGHT:    (c) 2011-2013 Rohde & Schwarz, Munich
** MODULE:       CmdSock.h
** ABBREVIATION:   
** COMPILER:     VC++ 6.0
** LANGUAGE:     C/C++
** AUTHOR:       Christian Pössinger
** ABSTRACT:     
** PREMISES:     
** REMARKS:      
** HISTORY:      
**  2011-12-11: (Poe) Creation
** REVIEW:       
******************************************************************************/

/* INCLUDE FILES ***************************************************************/

/* IMPORT */
#include <stdio.h>
#include <assert.h>
#include <limits.h>

#include "eb200udpsock.h"
#include "CreateUdpSock.h"

/* EXPORT */
#include "CmdSock.h"

/* LOCAL TYPES DECLARATION *****************************************************/

/* GLOBAL VARIABLES DEFINITION *************************************************/

/* GLOBAL CONSTANTS DEFINITION *************************************************/

/* GLOBAL DEFINES **************************************************************/

/* LOCAL DEFINES ***************************************************************/

/* LOCAL TYPES DECLARATION *****************************************************/

/* LOCAL CLASSES DECLARATION ***************************************************/

/* LOCAL VARIABLES DEFINITION **************************************************/

/* LOCAL CONSTANTS DEFINITION **************************************************/

/* LOCAL FUNCTIONS DEFINITION **************************************************/
/* FUNCTION ********************************************************************/
CCmdSock::CCmdSock()
/*
SPECIFICATION: ctor
PARAMETERS:
PRECONDITIONS: 
SIDE_EFFECTS: 
RETURN_VALUES: 
EXCEPTIONS: 
********************************************************************************/
{
    m_Socket = INVALID_SOCKET;

    m_isESMD = false;
    m_isEM050 = false;
    m_isPR100 = false;
    m_isEB200 = false;
    m_isDS = false;
    m_isPS = false;
    m_isFS = false;
    m_isCM = false;
    m_isSL = false;
    m_isDF = false;
    m_isPIFP = false;

    m_bVerbose = false;
    m_bVerboseCmd = false;
    m_bGPSOutput = false;
    m_bTracTcp = false;
    m_bUseCmdSock = false;
    m_bIQMode = false;
    m_bIQModeShort = true;
    m_bAIfOnly = false;
    m_bDelAll = false;
    m_bAudioOnlyMode = false;
    m_bRaw = false;
    m_bVerbose = false;
    m_bChannelsOnly = false;
    m_bBytesOnly = false;
    m_bCSMode = false;
    m_bOmniPhase = false;

    m_cDDCx[0] = '\0';
    m_cTracType[0] = '\0';
    m_cTracTag[0] = '\0';
    m_cTracFlag[0] = '\0';

    m_usRecPort = 0;
    m_usCfgPort = 0;
    m_usPort = SCPI_PORT_NUM_DEF;
    m_unPort = 0;
    m_unDDC = 0;
    m_unAudioMode = 0;
    m_dwlRecordingTime = 0;
    m_ulRemoteAddress = 0;
    m_nPort = 0;

    m_lfFmax = 0.0;

    m_pcDeviceAddress = NULL;
    m_pIP = NULL;
    m_pcWAVFile = NULL;
    m_pcIQFile = NULL;
    m_pcCSFile = NULL;
    m_pTracSock = NULL;
}

/* FUNCTION ********************************************************************/
bool CCmdSock::ParseCmdLine(int argc, char **argv)
/*
SPECIFICATION:
PARAMETERS:
PRECONDITIONS: 
SIDE_EFFECTS: 
RETURN_VALUES: true if commandline OK, false otherwise
EXCEPTIONS: 
********************************************************************************/
{
    bool bCmdLineOK = true;

    /* Process command line */
    int param;
    for (param = 1; param < argc && bCmdLineOK; ++param)
    {
        if (strcmp(argv[param], "-p") == 0)
        {
            /* Evaluate TCP port parameter */
            if (++param < argc)
            {
                m_unPort = atoi(argv[param]);
                m_usPort = (unsigned short) m_unPort;
                if ((m_unPort == 0) || (m_unPort > USHRT_MAX))
                    bCmdLineOK = false;
            }
            else
                bCmdLineOK = false;
        }
        else if (strcmp(argv[param], "-d") == 0)
        {
            /* Evaluate receiver instance / DDC number */
            if (++param < argc)
            {
                m_unDDC = atoi(argv[param]);
                if (m_unDDC > MAX_DDC_NUM) m_unDDC = 0; // fallback to main receiver for invalid receiver instance
            }
            else
                bCmdLineOK = false;
        }
        else if (strcmp(argv[param], "-am") == 0)
        {
            /* Evaluate audio mode parameter */
            if (++param < argc)
            {
                m_unAudioMode = atoi(argv[param]);
                if (m_unAudioMode > 12)
                    bCmdLineOK = false;
            }
            else
                bCmdLineOK = false;
        }
        else if (strcmp(argv[param], "-af") == 0)
        {
            /* Evaluate audio file name */
            if (++param < argc)
            {
                m_pcWAVFile = argv[param];
                if (m_pcWAVFile[0] == '\0')
                bCmdLineOK = false;
//              if (m_unAudioMode == 0)
//                  m_unAudioMode = 12;
            }
            else
                bCmdLineOK = false;
        }
        else if (strcmp(argv[param], "-a") == 0)
        {
            /* Evaluate audio only mode */
            m_bAudioOnlyMode = true;
        }
        else if (strcmp(argv[param], "-c") == 0)
        {
            /* Evaluate scan performance only */
            m_bChannelsOnly = true;
        }
        else if (strcmp(argv[param], "-bs") == 0)
        {
            /* Evaluate scan performance only */
            m_bBytesOnly = true;
        }
        else if (strcmp(argv[param], "-gps") == 0)
        {
            /* Evaluate scan performance only */
            m_bGPSOutput = true;
        }
        else if (strcmp(argv[param], "-v") == 0)
        {
            /* Verbose additional testprints */
            m_bVerbose = true;
        }
        else if (strcmp(argv[param], "-vc") == 0)
        {
            m_bVerboseCmd = true;
        }
        else if (strcmp(argv[param], "-vv") == 0)
        {
            m_bVerbose = m_bVerboseCmd = true;
        }
        else if (strcmp(argv[param], "-iqm") == 0)
        {
            /* Evaluate IQ mode */
            m_bIQMode = true;
            m_bIQModeShort = true;
        }
        else if (strcmp(argv[param], "-iqml") == 0)
        {
            /* Evaluate IQ mode */
            m_bIQMode = true;
            m_bIQModeShort = false;
        }
        else if (strcmp(argv[param], "-iqf") == 0)
        {
            /* Evaluate IQ file name */
            if (++param < argc)
            {
                m_pcIQFile = argv[param];
                m_bIQMode = true;
                m_bIQModeShort = true;
                if (m_pcIQFile[0] == '\0')
                    bCmdLineOK = false;
            }
            else
                bCmdLineOK = false;
        }
        else if (strcmp(argv[param], "-iqfl") == 0)
        {
            /* Evaluate IQ file name */
            if (++param < argc)
            {
                m_pcIQFile = argv[param];
                m_bIQMode = true;
                m_bIQModeShort = false;
                if (m_pcIQFile[0] == '\0')
                    bCmdLineOK = false;
            }
            else
                bCmdLineOK = false;
        }
        else if (strcmp(argv[param], "-iqfr") == 0)
        {
            /* Evaluate IQ file name */
            if (++param < argc)
            {
                m_pcIQFile = argv[param];
                m_bIQMode = true;
                m_bRaw = true;
                if (m_pcIQFile[0] == '\0')
                    bCmdLineOK = false;
            }
            else
                bCmdLineOK = false;
        }
        else if (strcmp(argv[param], "-iqt") == 0)
        {
            /* Evaluate recording time */
            if (++param < argc)
            {
                m_dwlRecordingTime = _atoi64(argv[param]);
            }
            else
                bCmdLineOK = false;
        }
        else if (strcmp(argv[param], "-csf") == 0)
        {
            /* Evaluate Comma Separated file name */
            if (++param < argc)
            {
                m_pcCSFile = argv[param];
                m_bCSMode = true;
                if (m_pcCSFile[0] == '\0')
                    bCmdLineOK = false;
            }
            else
                bCmdLineOK = false;
        }
        else if (strcmp(argv[param], "-t") == 0)
        {
            /* TCP tracing */
            m_bTracTcp = true;
        }
        else if (strcmp(argv[param], "-rp") == 0)
        {
            /* Trace receive port */
            if (++param < argc)
            {
                m_unPort = atoi(argv[param]);
                m_usRecPort = (unsigned short) m_unPort;
                if ((m_unPort == 0) || (m_unPort > USHRT_MAX))
                    bCmdLineOK = false;
            }
            else
                bCmdLineOK = false;
        }
        else if (strcmp(argv[param], "-cp") == 0)
        {
            /* Trace config port */
            if (++param < argc)
            {
                m_unPort = atoi(argv[param]);
                m_usCfgPort = (unsigned short) m_unPort;
                if ((m_unPort == 0) || (m_unPort > USHRT_MAX))
                    bCmdLineOK = false;
            }
            else
                bCmdLineOK = false;
        }
        else if (strcmp(argv[param], "-ci") == 0)
        {
            /* Trace config ip */
            if (++param < argc)
            {
                m_pIP = argv[param];
            }
            else
                bCmdLineOK = false;
        }
        else if (strcmp(argv[param], "-caif") == 0)
        {
            /* Trace AMMOS IF format only */
            m_bAIfOnly = true;
        }
        else if (strcmp(argv[param], "-da") == 0)
        {
            m_bDelAll = true;
        }
        else if (strcmp(argv[param], "-omn") == 0)
        {
            m_bOmniPhase = true;
        }
        else if (param == argc-1)
        {
            /* Evaluate receiver device address */
            m_pcDeviceAddress = argv[argc-1];
        }
        else
            bCmdLineOK = false;
    }

    // post eval consistency
    bCmdLineOK &= !(((m_pIP != NULL) || m_bAIfOnly) && (m_usCfgPort == 0)); // -ci and -caif require -cp
    bCmdLineOK &= !(m_bDelAll && (m_usRecPort != 0));                     // -da conflicts with '-rp'

    return bCmdLineOK;
}

/* FUNCTION ********************************************************************/
void CCmdSock::PrintHelp( void )
/*
SPECIFICATION:
PARAMETERS:
PRECONDITIONS: 
SIDE_EFFECTS: 
RETURN_VALUES: 
EXCEPTIONS: 
********************************************************************************/
{
    /* Print banner */
    printf("UDPEXAMPLE 5.54, Copyright (c) 1998-2012 Rohde&Schwarz, Munich\n\n");
    printf( HELP_NOTICE );
}

/* FUNCTION ********************************************************************/
void CCmdSock::Init(int argc, char **argv)
/*
SPECIFICATION:  Init all components and setup traces/data paths
PARAMETERS:
PRECONDITIONS: 
SIDE_EFFECTS: 
RETURN_VALUES: 
EXCEPTIONS: 
********************************************************************************/
{
    // parse commandline and propagate members
    bool bCmdLineOK = ParseCmdLine( argc, argv );

    /* Output help text if no command line parameter given or command line faulty */
    if (!bCmdLineOK || m_pcDeviceAddress == NULL)
    {
        PrintHelp();
        exit( 1 );
    }

    // if receive (only) port was not set -> setup SCPI connection for config.
    if ( (m_usRecPort == 0) || ((m_usRecPort != 0) && (m_usCfgPort != 0)) )
    {
        m_bUseCmdSock = true;
        /* Create a receiver control socket */
        m_ulRemoteAddress = SetRSDevice(m_pcDeviceAddress, m_usPort);
    }

    /* retrieve local IP address in case it's not set */
    if ( m_pIP == NULL )
    {
        m_pIP = GetLocalIP();
    }
    else
    {
        unsigned long ulCfgAddr = inet_addr(m_pIP);
        if ( ulCfgAddr == INADDR_NONE )
        {
            hostent* phost = gethostbyname(m_pIP);
            if (phost && phost->h_addr_list[0] != NULL)
            {
                    ulCfgAddr = *((unsigned*) phost->h_addr_list[0]);
            }
            else
            {
                printf("Can't resolve trace host name %s.\n", m_pIP);
                exit( 1 );
            }
            struct in_addr cfgaddr;
            cfgaddr.s_addr = ulCfgAddr;
            m_pIP = inet_ntoa(cfgaddr);
        }
    }

    // if config. (only) port was not set -> setup TRACe connection for receive/record.
    if ( (m_usCfgPort == 0) || ((m_usRecPort != 0) && (m_usCfgPort != 0)) )
    {
        /* create TCP/UDP trace socket instance */
        m_pTracSock = CreateUdpSock( m_bTracTcp, m_bTracTcp ? m_usPort + TRAC_TCP_PORT_OFFS : m_usPort, m_ulRemoteAddress, m_unPort );

        m_unPort = m_pTracSock->GetTracPort();
        if ( m_pTracSock->GetTracSock() == INVALID_SOCKET )
        {
            printf("Error setting up TRACe connection to %s [TRAC port = %d]\n", m_pcDeviceAddress, m_unPort);
            exit( 1 );
        }
        printf("TRACe connection to %s [TRAC port = %d] successful !\n", m_pcDeviceAddress, m_unPort);
    }
    else
    {
        m_unPort = m_usCfgPort;
    }

    /* Configure TRAC:UDP|TCP[:DDC] ... command */
    ConfigureTraceCmd();

    /* if '-da' switch is set: delete all trace paths and leave */
    if ( m_bDelAll )
    {
        DeleteAllTraces();
        exit( 0 );
    }

    if ( m_bUseCmdSock )
    {
        /* If no multi-access to receiver device required delete all trace paths */
        // sprintf(cBuffer, "%s:DEL ALL\n", cTracType);
        // pCmdSock->SendCmd(cBuffer);

        GetDeviceID();
        GetDeviceOptions();

        /* Tell EB200 socket about our Cmd Client */
        if (m_pTracSock)
        {
            m_pTracSock->SetSCPICmdSocket(this);
        }

        /* Configure receiver device traces */
        SetupTraces();

        /* configure audio mode */
        SetupDataPaths();

        /* finally check for errors */
        CheckDeviceErrors();
    }

    if ( m_pTracSock )
    {
        /* IQ File Recording? */
        if ( m_pcIQFile )    m_pTracSock->SetIFRecording(m_pcIQFile, m_bRaw, m_dwlRecordingTime);

        /* CS File Recording? */
        if ( m_pcCSFile )    m_pTracSock->SetCSRecording(m_pcCSFile);

        /* WAV File Recording? */
        if ( m_pcWAVFile )   m_pTracSock->SetAFRecording(m_pcWAVFile);

        /* call socket reception routine */
        m_pTracSock->Init();
    }
}


/* FUNCTION ********************************************************************/
unsigned long CCmdSock::SetRSDevice(char *m_pcDeviceAddress, unsigned short nPort)
/*
SPECIFICATION: Setup communication to DUT
PARAMETERS:
PRECONDITIONS: 
SIDE_EFFECTS: 
RETURN_VALUES: 
EXCEPTIONS: 
********************************************************************************/
{
    /* Determine receiver device IP address */
    unsigned long ulRemoteAddress = inet_addr(m_pcDeviceAddress);
    if (ulRemoteAddress == INADDR_NONE)
    {
        hostent* phost = gethostbyname(m_pcDeviceAddress);
        if (phost && phost->h_addr_list[0] != NULL)
        {
            ulRemoteAddress = *((unsigned*) phost->h_addr_list[0]);
        }
        else
        {
            printf("Can't resolve host name %s.\n", m_pcDeviceAddress);
            return 1;
        }
    }

    /* Create a receiver control TCP socket */
    m_Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_Socket == SOCKET_ERROR)
    {
        printf("Error creating SCPI socket.\n");
        return 1;
    }

    int sopt = 1;
    if ( setsockopt( m_Socket, IPPROTO_TCP, TCP_NODELAY, (char *)&sopt, sizeof( sopt ) ) == SOCKET_ERROR )
    {
        printf("setsockopt (TCP_NODELAY, %d) failed - errno %d\n", sopt, WSAGetLastError());
        return 1;
    }

    /* Create socket address structure for INET address family */
    struct sockaddr_in addrDevice;
    memset(&addrDevice, 0, sizeof(addrDevice));
    addrDevice.sin_family      = AF_INET;
    addrDevice.sin_addr.s_addr = ulRemoteAddress;
    addrDevice.sin_port        = htons(nPort);

    /* connect socket to receiver device */
    if (connect(m_Socket, (struct sockaddr *)&addrDevice, sizeof(addrDevice)) != 0)
    {
        printf("Error connecting to %s [SCPI port = %d]\n", m_pcDeviceAddress, nPort);
        return 1;
    }

    return ulRemoteAddress;
}


/* FUNCTION ********************************************************************/
int CCmdSock::SendCmd(char *pBuffer)
/*
SPECIFICATION: Send string to device
PARAMETERS:
PRECONDITIONS: 
SIDE_EFFECTS: 
RETURN_VALUES: Return bytes transmitted
EXCEPTIONS: 
********************************************************************************/
{
    if ( m_bVerboseCmd )    puts( pBuffer );

    unsigned int nLen = 0;
    nLen = send(m_Socket, pBuffer, strlen(pBuffer), 0);
    if (nLen != strlen(pBuffer))
    {
        printf("Error writing to socket. Len = %d\n", nLen);
    }
    return nLen;
}


/* FUNCTION ********************************************************************/
int CCmdSock::Receive(char *pBuffer, int length, int flags)
/*
SPECIFICATION: Receive len bytes from Socket to pBuffer
PARAMETERS:
PRECONDITIONS: 
SIDE_EFFECTS: 
RETURN_VALUES: Return the number of bytes received
EXCEPTIONS: 
********************************************************************************/
{
    unsigned int nLen = 0;

    nLen = recv(m_Socket, pBuffer, length, flags);
    if (nLen == SOCKET_ERROR)
    {
        printf("Error receiving from socket. ERROR[%i]\n", WSAGetLastError());
    }

    return nLen;
}


/* FUNCTION ********************************************************************/
char *CCmdSock::GetLocalIP(void)
/*
SPECIFICATION: Get the local IP address used for connecting with this socket
PARAMETERS:
PRECONDITIONS: 
SIDE_EFFECTS: 
RETURN_VALUES: Returns a string with the local IP address, e.g. "172.17.75.1"
EXCEPTIONS: 
********************************************************************************/
{
    struct sockaddr_in addrLocal;
    int addrlen = sizeof(addrLocal);
    getsockname(m_Socket, (struct sockaddr*)&addrLocal, &addrlen);
    return inet_ntoa(addrLocal.sin_addr);
}

/* FUNCTION ********************************************************************/
char* CCmdSock::GetDeviceID(void)
/*
SPECIFICATION: Get R&S Device model
PARAMETERS:
PRECONDITIONS: 
SIDE_EFFECTS:  Changes m_isEB200, m_isEM050, m_isESMD, m_isPR100
RETURN_VALUES: 
EXCEPTIONS: 
********************************************************************************/
{
    char cBuffer[BUFFER_SIZE];
    char *pBuffer = cBuffer;

    SendCmd("*IDN?\n");

    int len = Receive(cBuffer, sizeof(cBuffer), 0);
    if( len < 0 ) len = 0; cBuffer[ len ] = '\0';

    if (strstr(pBuffer,"EB200") != NULL)      m_isEB200 = true;
    if (strstr(pBuffer,"ESMB") != NULL)       m_isEB200 = true;
    if (strstr(pBuffer,"EB110") != NULL)      m_isEB200 = true;

    if (strstr(pBuffer,"EM050") != NULL)      m_isEM050 = true;
    if (strstr(pBuffer,"EM550") != NULL)      m_isEM050 = true;
    if (strstr(pBuffer,"EM510") != NULL)      m_isEM050 = true;

    if ((strstr(pBuffer,"ESMD") != NULL) ||
        (strstr(pBuffer,"DDF255") != NULL)
       )
    {
        m_isESMD = true;
        m_isPIFP = true;
        m_isFS = true;
    }

    if (strstr(pBuffer,"NSMI") != NULL)
    {
        m_isESMD = true;
    }

    if ((strstr(pBuffer,"EB500") != NULL) ||
        (strstr(pBuffer,"EB510") != NULL) ||
        (strstr(pBuffer,"DDF205") != NULL)
       )
    {
        m_isESMD = true;
        m_isPIFP = true;
        m_isFS = true;
    }

    if ((strstr(pBuffer,"PR100") != NULL) ||
        (strstr(pBuffer,"EM100") != NULL) ||
        (strstr(pBuffer,"DDF007") != NULL)
       )
    {
        m_isPR100 = true;
    }

    return pBuffer;
}

/* FUNCTION ********************************************************************/
int CCmdSock::GetDeviceOptions(void)
/*
SPECIFICATION: Retrieve device options and max RX frequency
PARAMETERS:
PRECONDITIONS: 
SIDE_EFFECTS:  changes m_isDS, m_isPS, m_isFS, m_isCM, m_isSL, m_isDF, m_lfFmax
RETURN_VALUES: 
EXCEPTIONS: 
********************************************************************************/
{
    char cBuffer[BUFFER_SIZE];

    SendCmd("*OPT?\n");
    int len = Receive(cBuffer, sizeof(cBuffer), 0);
    if( len < 0 ) len = 0; cBuffer[ len ] = '\0';

    if (strstr(cBuffer,"DS") != NULL)   m_isDS = true;
    if (strstr(cBuffer,"PS") != NULL)   m_isPS = true;
    if (strstr(cBuffer,"FS") != NULL)   m_isFS = true;
    if (strstr(cBuffer,"CM") != NULL)   m_isCM = true;
    if (strstr(cBuffer,"SL") != NULL)   m_isSL = true;
    if (strstr(cBuffer,"DF") != NULL)   m_isDF = true;

    if (m_bChannelsOnly)
    {
        /* disable FStrength and DF option */
        m_isDF = false;
        m_isFS = false;
    }

    SendCmd("FREQ? MAX\n");
    len = Receive(cBuffer, sizeof(cBuffer), 0);
    if( len < 0 ) len = 0; cBuffer[ len ] = '\0';

    /* double */ m_lfFmax = atof(cBuffer);

    return len;
}

/* FUNCTION ********************************************************************/
int CCmdSock::CheckDeviceErrors(void)
/*
SPECIFICATION: Check device error log and print out errors
PARAMETERS:
PRECONDITIONS: 
SIDE_EFFECTS: 
RETURN_VALUES: 
EXCEPTIONS: 
********************************************************************************/
{
    int len = 0;

    if ( m_bUseCmdSock )
    {
        char cBuffer[BUFFER_SIZE];
    
        sprintf(cBuffer, "SYST:ERR%s?\n", m_isEB200 ? "" : ":ALL");   // EB200 doesn't know SYST:ERR:ALL);
        SendCmd(cBuffer);
    
        len = Receive(cBuffer, sizeof(cBuffer),0);
        if ( len < 0 ) len = 0; cBuffer[ len ] = '\0';
        puts( cBuffer );
    }

    return len;
}

/* FUNCTION ********************************************************************/
void CCmdSock::DeleteAllTraces(void)
/*
SPECIFICATION: Delete all traces on the device
PARAMETERS:
PRECONDITIONS: 
SIDE_EFFECTS: 
RETURN_VALUES: 
EXCEPTIONS: 
********************************************************************************/
{
    char cBuffer[BUFFER_SIZE];

    /* Delete all traces */
    sprintf(cBuffer, "%s:DEL ALL\n", m_cTracType);
    SendCmd(cBuffer);

    /* Get list of configured traces */
    sprintf(cBuffer, "%s?\n", m_cTracType);
    SendCmd(cBuffer);

    int len = Receive(cBuffer, sizeof(cBuffer), 0);
    if( len < 0 ) len = 0; cBuffer[ len ] = '\0';

    printf("All %s TRACes deleted:\n", m_bTracTcp ? "TCP" : "UDP");
    printf("%s", cBuffer);
}

/* FUNCTION ********************************************************************/
void CCmdSock::DeleteTraces(void)
/*
SPECIFICATION: Delete this trace on the device
PARAMETERS:
PRECONDITIONS: 
SIDE_EFFECTS: 
RETURN_VALUES: 
EXCEPTIONS: 
********************************************************************************/
{
    if ( m_bUseCmdSock )
    {
        char cBuffer[BUFFER_SIZE];
    
        sprintf(cBuffer, "%s:DEL \"%s\", %d\n", m_cTracType, GetLocalIP(), m_nPort);
        SendCmd(cBuffer);
    }
}

/* FUNCTION ********************************************************************/
void CCmdSock::ConfigureTraceCmd(void)
/*
SPECIFICATION: Configure aproriate TRAC TAG/FLAG SCPI command
PARAMETERS:
PRECONDITIONS: 
SIDE_EFFECTS: changes m_nPort, m_cDDCx, m_cTracType, m_cTracFlag, m_cTracTag
RETURN_VALUES: 
EXCEPTIONS: 
********************************************************************************/
{
    m_nPort = m_unPort;

    if (m_unDDC > 0)
    {
        sprintf(m_cDDCx, ":DDC%d", m_unDDC);
    }

    sprintf(m_cTracType, "TRAC%s:%s", m_cDDCx, m_bTracTcp ? "TCP" : "UDP");
    sprintf(m_cTracFlag, "%s:FLAG \"%s\", %d", m_cTracType, GetLocalIP(), m_nPort);
    sprintf(m_cTracTag, "%s:TAG \"%s\", %d", m_cTracType, GetLocalIP(), m_nPort);
}

/* FUNCTION ********************************************************************/
void CCmdSock::SetupTraces(void)
/*
SPECIFICATION: Configure device specific traces
PARAMETERS:
PRECONDITIONS: 
SIDE_EFFECTS: 
RETURN_VALUES: 
EXCEPTIONS: 
********************************************************************************/
{
    char cBuffer[BUFFER_SIZE];

    if (!m_bAudioOnlyMode && !m_bAIfOnly)
    {
        sprintf(cBuffer, "%s, FSC, MSC, DSC, AUD, IFP, CW\n", m_cTracTag);
        SendCmd(cBuffer);

        if ( OptionIsCM() )
        {
            sprintf(cBuffer, "%s, FAST, LIST\n", m_cTracTag);
            SendCmd(cBuffer);
        }

        if ( EquipmentisEM050() )
        {
            if ( OptionIsSL() )
            {
                sprintf(cBuffer, "%s, IF, VID, VDP, PSC, SELC\n", m_cTracTag);
            }
            else
            {
                sprintf(cBuffer, "%s, IF, VID, VDP, PSC\n", m_cTracTag);
            }
            SendCmd(cBuffer);

            if ( OptionIsDF() )
            {
                sprintf(cBuffer, "%s, DFP\n", m_cTracTag);
                SendCmd(cBuffer);
            }

            if ( m_isPIFP )
            {
                sprintf(cBuffer, "%s, PIFP\n", m_cTracTag);
                SendCmd(cBuffer);
            }
        }

        if ( m_isESMD )
        {
            if (m_bGPSOutput)
            {
                sprintf(cBuffer, "%s, GPSC\n", m_cTracTag);
                SendCmd(cBuffer);
            }
        }

        if ( m_isPR100 )
        {
            sprintf(cBuffer, "%s, IF, PSC\n", m_cTracTag);
            SendCmd(cBuffer);

            if (m_bGPSOutput)
            {
                sprintf(cBuffer, "%s, GPSC\n", m_cTracTag);
                SendCmd(cBuffer);
            }
        }

        sprintf(cBuffer, "%s, \"VOLT:AC\", \"FREQ:RX\", \"FREQ:OFFS\", \"OPT\"\n", m_cTracFlag);
        
        SendCmd(cBuffer);

        if ( OptionIsFS() || m_isESMD )
        {
            sprintf(cBuffer, "%s, \"FSTRength\"\n", m_cTracFlag);
            SendCmd(cBuffer);
            // and also set up the appropriate sensor function
            SendCmd("SENS:FUNC 'FSTR'\n");
        }

        if ( OptionIsDF() )
        {
            sprintf(cBuffer, "%s, \"AZIM\", \"DFQ\", \"DFL\"\n", m_cTracFlag);
            SendCmd(cBuffer);

            if ( OptionIsFS() )
            {
                sprintf(cBuffer, "%s, \"DFFSTRength\"\n", m_cTracFlag);
                SendCmd(cBuffer);
            }
            if (m_bOmniPhase)
            {
                sprintf(cBuffer, "%s, \"DFOMniphase\"\n", m_cTracFlag);
                SendCmd(cBuffer);
            }
        }

        if ( m_isPR100 )
        {
            sprintf(cBuffer, "%s, \"FREQ:LOW:RX\"\n", m_cTracFlag);
            SendCmd(cBuffer);
        }

        if (m_lfFmax > 4.29E9)
        {
            sprintf(cBuffer, "%s, \"FREQ:HIGH:RX\"\n", m_cTracFlag);
            SendCmd(cBuffer);
        }

        #ifdef SWAP_DEF
            sprintf(cBuffer, "%s, \"SWAP\"\n", m_cTracFlag);
            SendCmd(cBuffer);
        #endif
    }
    else if ( m_bAIfOnly )
    {
        sprintf(cBuffer, "%s, AIF\n", m_cTracTag);
        SendCmd(cBuffer);

        sprintf(cBuffer, "%s, \"OPT\"\n", m_cTracFlag);
        SendCmd(cBuffer);
    }
    else
    {
        sprintf(cBuffer, "%s, AUDIO\n", m_cTracTag);
        SendCmd(cBuffer);

        sprintf(cBuffer, "%s, \"OPT\"\n", m_cTracFlag);
        SendCmd(cBuffer);
    }
}

/* FUNCTION ********************************************************************/
void CCmdSock::SetupDataPaths(void)
/*
SPECIFICATION: Configure device data paths for audio and if
PARAMETERS:
PRECONDITIONS: 
SIDE_EFFECTS: 
RETURN_VALUES: 
EXCEPTIONS: 
********************************************************************************/
{
    char cBuffer[BUFFER_SIZE];

    /* configure remote Audio */
    sprintf(cBuffer, "SYST:AUD%s:REM:MODE %d\n", m_cDDCx, m_unAudioMode);
    SendCmd(cBuffer);

    /* configure IQ data mode */
    if ( EquipmentisEM050() )
    {
        if ( !m_bAIfOnly )
        {
            if ( m_bIQModeShort )
            {
                sprintf(cBuffer, "SYST:IF%s:REM:MODE %s\n", m_cDDCx, m_bIQMode ? "SHORT" : "OFF");
            }
            else
            {
                sprintf(cBuffer, "SYST:IF%s:REM:MODE %s\n", m_cDDCx, m_bIQMode ? "LONG" : "OFF");
            }
        }
        else
        {
            sprintf(cBuffer, "SYST:IF%s:REM:MODE %s\n", m_cDDCx, "ASHORT");
        }
        SendCmd(cBuffer);
    }
}

/* End of file ***************************************************************/
