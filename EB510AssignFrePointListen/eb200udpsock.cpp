/*******************************************************************************
********************************************************************************
** COPYRIGHT:      (c) 1995-2004 Rohde & Schwarz, Munich
** MODULE:         EB200UdpSock.cpp
** ABBREVIATION:
** COMPILER:       VC++ 5.0
** LANGUAGE:       C/C++
** AUTHOR:         Martin Hisch
** ABSTRACT:
** PREMISES:
** REMARKS:
** HISTORY:
**  2004-06-30: (Ob)  Version 3.91 Added WAV File support (Audiorecording)
**  2004-10-06: (Mue) Version 3.90 Ausgabe nur Channels/sec mit Parameter -c
**  2004-03-26: (Mue) #define _CHANNELS_ONLY
**  2003-09-11: (FB)  Version 3.71: Added WAV file support
**  2003-08-06: (Mue) Ausgaben nur, wenn die entsprechende Option vorhanden ist.
**  2003-07-22: (Hh)  CEB200UdpSock::IF() extended with IQ Sample capture
**  2002-06-03: (Mu)  newStepScheme in OptHeaderDScan with VersionMinor >= 0x24
**  2002-03-04: (Mu)  Channels / sec ,m_nChannels
**  2001-01-31: (ks)  Field Strength, Mode print, VersionMinor, Comments
**  2000-11-30: (ks)  Include Not-Swapping.
**  2000-05-29: (Hh)  Creation
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
#include <time.h>
#include <process.h>

/* EXPORT */
#include "EB200UdpSock.h"

/* GLOBAL VARIABLES DEFINITION *************************************************/

/* GLOBAL CONSTANTS DEFINITION *************************************************/

/* GLOBAL DEFINES **************************************************************/

/* LOCAL DEFINES ***************************************************************/

/* LOCAL TYPES DECLARATION *****************************************************/

/* LOCAL CLASSES DECLARATION ***************************************************/

/* LOCAL VARIABLES DEFINITION **************************************************/

/* LOCAL CONSTANTS DEFINITION **************************************************/

// following list must be sorted in ascending tag order
struct TagList TagDispatch[] =
{                                                           /* tag , virtual member function */
    { FSCAN,      &CEB200UdpSock::FScan },
#ifdef _STATISTICS
    { MSCAN,      &CEB200UdpSock::MScan },
#else
    /* could be called directely if no statistics and no optionalHeader-processing: */
    { MSCAN,      &CEB200UdpSock::Scan },
#endif
    { DSCAN,      &CEB200UdpSock::DScan },
    { AUDIO,      &CEB200UdpSock::Audio },
    { IFPAN,      &CEB200UdpSock::IFPan },
    { FASTL,      &CEB200UdpSock::FastLev },
    { LISTF,      &CEB200UdpSock::List },
    { CW,         &CEB200UdpSock::Cw },
    { IF,         &CEB200UdpSock::IF },
    { VIDEO,      &CEB200UdpSock::Video },
    { VIDEOPAN,   &CEB200UdpSock::VideoPan },
    { PSCAN,      &CEB200UdpSock::PScan },
    { SELCALL,    &CEB200UdpSock::Selcall },
    { DFPAN,      &CEB200UdpSock::DFPan },
    { PHD,        &CEB200UdpSock::PHD },
    { GPSCompass, &CEB200UdpSock::GPSCompass },
    { LAST_TAG,   NULL } // must be the last entry
};
int TagDispatchLen = sizeof(TagDispatch)/sizeof(TagDispatch[0]);

const AUDIO_FORMAT_TYPE CEB200UdpSock::m_AudioFormats[] =
{
    /* fs,    Bits,   Chan,   Rate, FrameLength */
    {      0,    0,      0,      0,      0}                 // off
    ,{ 32000,   16,      2, 128000,      4}
    ,{ 32000,   16,      1,  64000,      2}
    ,{ 32000,    8,      2,  64000,      2}
    ,{ 32000,    8,      1,  32000,      1}
    ,{ 16000,   16,      2,  64000,      4}
    ,{ 16000,   16,      1,  32000,      2}
    ,{ 16000,    8,      2,  32000,      2}
    ,{ 16000,    8,      1,  16000,      1}
    ,{  8000,   16,      2,  32000,      4}
    ,{  8000,   16,      1,  16000,      2}
    ,{  8000,    8,      2,  16000,      2}
    ,{  8000,    8,      1,   8000,      1}
    ,{  8000,   16,      1,   1625,     65}                 // GSM 6.10
};


/* LOCAL FUNCTIONS DEFINITION **************************************************/

/* FUNCTION ********************************************************************/
int CEB200UdpSock::Vprintf( const char *format, ...)
/*
SPECIFICATION:
PARAMETERS:
 const char *format :
... :
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    va_list pvar;
    int ret = 0;

    if ( m_pCmdSock->Verbose() )
    {
        va_start(pvar, format);
        fprintf( stderr, "*" );
        ret = vfprintf(stderr, format, pvar);
        va_end(pvar);
    }
    return ret;
}


/* FUNCTION ********************************************************************/
float CEB200UdpSock::ntohf( float netfloat )
/*
SPECIFICATION:
PARAMETERS:
const char *format :
... :
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    float ReturnValue;

    u_long tmp = ntohl(*((u_long*)&( netfloat )));
    ReturnValue = *((float*)&tmp);
    return ReturnValue;
}


/* FUNCTION ********************************************************************/
CEB200UdpSock::CEB200UdpSock( bool bTcp, unsigned short TracPort, unsigned long ulRemoteAddress, unsigned short RecvPort )
/*
SPECIFICATION:
Constructor. Initializing of member variables
PARAMETERS:
bTcp           : true when using TCP tracing
TracPort       : port number (only used for TCP tracing to connect to receiver)
ulRemoteAddress: IPv4 address (only used for TCP tracing to connect to receiver)
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    m_nSequenceNumber = -1;
    m_nPort = TracPort;
    m_nRecvPort = RecvPort;
    m_bTcp  = bTcp;
    m_bEnableStatistics = true;
    m_bStop = FALSE;
    m_bIFRecording = false;
    m_bAFRecording = false;
    m_bRaw = false;
    m_pRawFile = NULL;
    m_pCSFile = NULL;
    m_pctIQFile = NULL;
    m_b64BitFrequencies = false;
    m_bTimestamp = false;
    m_bNewGenericAtt = false;
    m_nDDCx = 0;
    m_ThreadHandle = 0;
    m_pCmdSock = NULL;
    strncpy( m_cDDCx, "Main RX", sizeof(m_cDDCx) );

    // create trace socket here
    m_Sock = socket(AF_INET, bTcp ? SOCK_STREAM : SOCK_DGRAM, 0);

    int optval=(32 * 1024 * 1024);
    int optlen=sizeof( optval );

    if ( setsockopt(m_Sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, optlen) == SOCKET_ERROR )
    {
        printf("Could not set receive buffer len to %d bytes - errno = %d !\n", optval, WSAGetLastError());
    }
    if ( getsockopt(m_Sock, SOL_SOCKET, SO_RCVBUF, (char*)&optval, &optlen) != SOCKET_ERROR )
    {
        // printf("Receive buffer len is actually set to %d bytes.\n", optval);
    }
    else printf("Could not verify receive buffer len - errno = %d !\n", WSAGetLastError());

    optval=1;
    if ( setsockopt(m_Sock, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, optlen) == SOCKET_ERROR )
    {
        printf("Could not set SO_REUSEADDR flag = %d - errno = %d !\n", optval, WSAGetLastError());
    }

    // bind it to given RecvPort or let the system choose an arbitrary (local) port number if == 0
    memset(&m_Addr, 0, sizeof(m_Addr));
    m_Addr.sin_family = AF_INET;
    m_Addr.sin_port   = htons( RecvPort );

    if ( bind(m_Sock, (struct sockaddr *)&m_Addr, sizeof(m_Addr)) == SOCKET_ERROR)
    {
      printf("Couldn't bind TRACe socket - errno = %d\n", WSAGetLastError());
      closesocket(m_Sock), m_Sock = INVALID_SOCKET;
      return;
    }

    if ( bTcp )
    {
        m_Addr.sin_addr.s_addr = ulRemoteAddress;
        m_Addr.sin_port        = htons( TracPort );

        /* connect socket to receiver device TCP trace service */
        if ( connect(m_Sock, (struct sockaddr *)&m_Addr, sizeof(m_Addr)) == SOCKET_ERROR)
        {
            printf("Error retrieving connection attributes - errno = %d\n", WSAGetLastError());
            closesocket(m_Sock), m_Sock = INVALID_SOCKET;
            return;
        }
    }

    // finally determine the port number used
    struct sockaddr_in laddr;
    int laddrlen = sizeof(laddr);
    if ( getsockname(m_Sock, (struct sockaddr*)&laddr, &laddrlen) == SOCKET_ERROR )
    {
        printf("Error retrieving connection attributes - errno = %d\n", WSAGetLastError());
        closesocket(m_Sock), m_Sock = INVALID_SOCKET;
        return;
    }
    m_nPort = ntohs(laddr.sin_port);

    if (m_pctIQFile == NULL)
    {
        // create default wavfile object
        m_pctIQFile = new CWAVFile;
    }
}

/* FUNCTION ********************************************************************/
CEB200UdpSock::~CEB200UdpSock()
/*
SPECIFICATION: dtor
PARAMETERS:
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    WaitOnThread();
    CloseFiles();
    CloseTrace();
    if (m_pctIQFile != NULL)
    {
        delete m_pctIQFile;
        m_pctIQFile = NULL;
    }
}

/* FUNCTION ********************************************************************/
void CEB200UdpSock::Init(bool bUseWin32Thread)
/*
SPECIFICATION:
Initialize UDP Socket instance and starting receiption.
PARAMETERS:
bool bUseWin32Thread : true -> use a real thread instead of doing everything in the main thread
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    m_bStop = FALSE;
    m_nSequenceNumber = -1;
    m_nFScanPackets = 0;
    m_nMScanPackets = 0;
    m_nDScanPackets = 0;
    m_nIFPanPackets = 0;
    m_nDFPanPackets = 0;
    m_nPHDPackets = 0;
    m_nAudioPackets = 0;
    m_nIFPackets = 0;
    m_nVideoPackets = 0;
    m_nFastLevPackets = 0;
    m_nListPackets = 0;
    m_nCwPackets = 0;
    m_nPScanPackets = 0;
    m_nVideoPanPackets = 0;
    m_nSelcallPackets = 0;
    m_oldtime = GetTickCount();
    m_nCount = 0;
    m_nChannels = 0;
    m_nTotalLen = 0;
    m_lfRms = 0.0;
    m_nRxAtt = 0;
    m_kFactor = 0x7FFF; // means invalid
    m_flags = 0;
    m_nIFSampleCounter = m_nVideoSampleCounter = 0;
    m_nIFSampleRate = 1;
    strcpy(m_sDemod, "");

    if (bUseWin32Thread)
    {
        typedef unsigned (__stdcall THREAD_FUNCTION_TYPE)(void *);
        unsigned int newPid;
        unsigned int nStacksize = 32000;

        m_ThreadHandle = (HANDLE)_beginthreadex(NULL, nStacksize, (THREAD_FUNCTION_TYPE*)(EB200UdpThreadProc), (void*)this,
                       0, (unsigned int*)&newPid);
        assert(m_ThreadHandle != 0);
    }
    else
    {
        m_ThreadHandle = 0;
        EB200UdpThreadProc(this);
    }
}

void    CEB200UdpSock::WaitOnThread(void)
{
    if (m_ThreadHandle != 0)
    {
        WaitForSingleObject(m_ThreadHandle, INFINITE);
        CloseHandle(m_ThreadHandle);
        m_ThreadHandle = 0;
    }
}

/* FUNCTION ********************************************************************/
void    CEB200UdpSock::CloseTrace(void)
/*
SPECIFICATION:
Closing trace socket.
PARAMETERS:
void :
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    m_bStop = TRUE;
    if ( m_Sock != INVALID_SOCKET ) closesocket( m_Sock ), m_Sock = INVALID_SOCKET;
}


/* FUNCTION ********************************************************************/
BOOL    CEB200UdpSock::CheckDGram(struct EB200DatagramFormat *pDGram, unsigned long nLen)
/*
SPECIFICATION:
Check if datagram is confirm to EB200/ESMB UDP version 2.22
PARAMETERS:
struct EB200DatagramFormat *pDGram : Pointer to datagram
unsigned short nLen : Length of datagram
PRECONDITIONS:
SIDE_EFFECTS:
Data in struct EB200DatagramFormat will be converted from network byte order in
host byte order
RETURN_VALUES:
TRUE -> this is a valid EB200 datagram of version 2.22
FALSE -> no valid EB200 datagram of version 2.22
EXCEPTIONS:
********************************************************************************/
{
    BOOL bOK = FALSE;
    // check header length
    if (nLen >= (EB200_DATAGRAM_HEADER_SIZE))
    {
        // change byteordering
        pDGram->MagicNumber = ntohl(pDGram->MagicNumber);
        pDGram->VersionMinor = ntohs(pDGram->VersionMinor);
        pDGram->VersionMajor = ntohs(pDGram->VersionMajor);
        pDGram->SequenceNumber = ntohs(pDGram->SequenceNumber);
        pDGram->DataSize = ntohl(pDGram->DataSize);
        if (pDGram->MagicNumber == 0x000EB200)
        {
            if (pDGram->VersionMajor == 0x2)
            {
                if (pDGram->VersionMinor >= 0x22)           // first implemented version
                {
                    // As of this implementation: current minor version is 0x40.

                    // Minor number changes should not affect functionality
                    // since these are meant to be backward compatible.
                    // But a value > current_implemented_version would tell
                    // me that there is added functionality and that this
                    // program could be adapted to cope with the new features.
                    //
                    if (pDGram->VersionMinor >= 0x30)
                    {
                        // 64 Bit Frequencies available
                        m_b64BitFrequencies = true;

                        // DataSize is available since v2.49 !
                        if (pDGram->VersionMinor < 0x31) pDGram->DataSize = 0;

                        // Timestamp (and channel number) is available since v2.64 !
                        if (pDGram->VersionMinor >= 0x40)
                        {
                            m_bTimestamp = true;
                        }
                        // new generic attribute is available since v2.80 !
                        if (pDGram->VersionMinor >= 0x50)
                        {
                            m_bNewGenericAtt = true;
                        }
                    }
                    else
                    {
                        // 64 Bit Frequencies not available
                        m_b64BitFrequencies = false;
                        // Timestamp not available
                        m_bTimestamp = false;
                        // new generic attribute is not available
                        m_bNewGenericAtt = false;
                    }

                    bOK = TRUE;

                    m_nSequenceNumber++;
                    if (m_nSequenceNumber != pDGram->SequenceNumber)
                    {
                        if (m_nSequenceNumber != 0)
                        {
                            printf("%d Datagram lost\n", abs(m_nSequenceNumber-pDGram->SequenceNumber));
                            printf("*****************************************************************************\n");
                        }
                    }
                    m_nSequenceNumber = pDGram->SequenceNumber;
                }
            }
        }
        else
        {
            // rechange byteordering
            pDGram->MagicNumber = ntohl(pDGram->MagicNumber);
            pDGram->VersionMinor = ntohs(pDGram->VersionMinor);
            pDGram->VersionMajor = ntohs(pDGram->VersionMajor);
            pDGram->SequenceNumber = ntohs(pDGram->SequenceNumber);
            pDGram->DataSize = ntohl(pDGram->DataSize);

//            printf("Datagram header wrong MagicNumber = 0x%08X !\n", pDGram->MagicNumber);
        }
    }
    else
    {
        printf("Datagram header too short [%d < %d] !\n", nLen, (EB200_DATAGRAM_HEADER_SIZE));
    }
    return bOK;
}

/* FUNCTION ********************************************************************/
void    CEB200UdpSock::ParseData(struct EB200DatagramFormat *pDGram, int nLen)
/*
SPECIFICATION:
Parsing and dispatching datagrams
PARAMETERS:
struct EB200DatagramFormat *pDGram : Pointer to datagram, the EB200DatagramFormat header
is already converted from network to host byte order
int nLen : Length of datagram
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    int nTempLen = nLen;
    struct GenericAttribute *pAttr = &(pDGram->Attribute[0]);
    int length;
    int GenericAttributeLength;

    nLen -=  ((unsigned char*)pAttr) - ((unsigned char*)pDGram);
    while (nLen > 0)
    {
        struct NewGenericAttribute *pNewAttr = (struct NewGenericAttribute *)pAttr;
        // change byte order of generic attribute
        pAttr->tag = ntohs(pAttr->tag);
        if (pAttr->tag > TAG_NEW_GENERICATT_LIMIT)
        {
            /* new generic attribute used */
            ASSERT(m_bNewGenericAtt);
            length = pNewAttr->length = ntohl(pNewAttr->length);
            GenericAttributeLength = offsetof(NewGenericAttribute, data);
        }
        else
        {
            /* old generic attribute used */
            length = pAttr->length = ntohs(pAttr->length);
            GenericAttributeLength = offsetof(GenericAttribute, data);
        }
        // dispatch according to tag
        int i;
        for (i=(TagDispatchLen-1); i >= 0; i--)
        {
            if (pAttr->tag >= TagDispatch[i].nTag)
            {
                // we found the tag in the TagDispatch[] list
                break;
            }
        }
        if (TagDispatch[i].pMethode != NULL)
        {
            PMEMBER fp;
            fp = TagDispatch[i].pMethode;
            // call virtual member function for this generic attribute
            (this->*fp)(pAttr);
        }
        nLen -= length + GenericAttributeLength;
        // address next GenericAttribute
        pAttr = (struct GenericAttribute *)(((unsigned char *)pAttr) + length + GenericAttributeLength);
    }
    ASSERT(nLen >= 0);
}

/* FUNCTION ********************************************************************/
void    CEB200UdpSock::ParseSelectorFlags(unsigned long nSel, void *pData, int nCount)
/*
SPECIFICATION:
Parse selector Flags and setup pointers to data members
PARAMETERS:
unsigned long nSel : selector Flags
void *pData : Pointer to first data byte
int nCount : Number of trace values
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    unsigned int flag = LEVEL;
    m_pLevel = NULL;
    m_pOffset = NULL;
    m_pFStrength = NULL;
    m_pAM = NULL;
    m_pAMPos = NULL;
    m_pAMNeg = NULL;
    m_pFM = NULL;
    m_pFMPos = NULL;
    m_pFMNeg = NULL;
    m_pPM = NULL;
    m_pBW = NULL;
    m_pAzimuth = NULL;
    m_pQuality = NULL;
    m_pDFLevel = NULL;
    m_pDFFStrength = NULL;
    m_pElevation = NULL;
    m_pOmniphase = NULL;
    m_pChannel = NULL;
    m_pFreq = NULL;
    m_pFreqHigh = NULL;

    m_nChannels += nCount;

    while (flag != SIGGTSQU)
    {
        if (nSel & flag)
        {
            switch (flag)
            {
                case LEVEL:
                    m_pLevel = (short*)pData;
                if (! (nSel & SWAP)) {
                        SwapBuffer(m_pLevel, nCount);
                    }
                    pData = m_pLevel + nCount;
                    break;
                case OFFSET:
                    m_pOffset = (long*)pData;
                if (! (nSel & SWAP)) {
                        SwapBuffer(m_pOffset, nCount);
                    }
                    pData = m_pOffset + nCount;
                    break;
                case FSTRENGTH:
                    m_pFStrength = (short*)pData;
                if (! (nSel & SWAP)) {
                        SwapBuffer(m_pFStrength, nCount);
                    }
                    pData = m_pFStrength + nCount;
                    break;
                case AM:
                    m_pAM = (short*)pData;
                if (! (nSel & SWAP)) {
                        SwapBuffer(m_pAM, nCount);
                    }
                    pData = m_pAM + nCount;
                    break;
                case AMPOS:
                    m_pAMPos = (short*)pData;
                if (! (nSel & SWAP)) {
                        SwapBuffer(m_pAMPos, nCount);
                    }
                    pData = m_pAMPos + nCount;
                    break;
                case AMNEG:
                    m_pAMNeg = (short*)pData;
                if (! (nSel & SWAP)) {
                        SwapBuffer(m_pAMNeg, nCount);
                    }
                    pData = m_pAMNeg + nCount;
                    break;
                case FM:
                    m_pFM = (long*)pData;
                if (! (nSel & SWAP)) {
                        SwapBuffer(m_pFM, nCount);
                    }
                    pData = m_pFM + nCount;
                    break;
                case FMPOS:
                    m_pFMPos = (long*)pData;
                if (! (nSel & SWAP)) {
                        SwapBuffer(m_pFMPos, nCount);
                    }
                    pData = m_pFMPos + nCount;
                    break;
                case FMNEG:
                    m_pFMNeg = (long*)pData;
                if (! (nSel & SWAP)) {
                        SwapBuffer(m_pFMNeg, nCount);
                    }
                    pData = m_pFMNeg + nCount;
                    break;
                case PM:
                    m_pPM = (short*)pData;
                if (! (nSel & SWAP)) {
                        SwapBuffer(m_pPM, nCount);
                    }
                    pData = m_pPM + nCount;
                    break;
                case BAND:
                    m_pBW = (long*)pData;
                if (! (nSel & SWAP)) {
                        SwapBuffer(m_pBW, nCount);
                    }
                    pData = m_pBW + nCount;
                    break;
                case AZIMUTH:
                    m_pAzimuth = (short*)pData;
                if (! (nSel & SWAP)) {
                        SwapBuffer(m_pAzimuth, nCount);
                    }
                    pData = m_pAzimuth + nCount;
                    break;
                case DF_QUALITY:
                    m_pQuality = (short*)pData;
                if (! (nSel & SWAP)) {
                        SwapBuffer(m_pQuality, nCount);
                    }
                    pData = m_pQuality + nCount;
                    break;
                case DF_LEVEL:
                    m_pDFLevel = (short*)pData;
                if (! (nSel & SWAP)) {
                        SwapBuffer(m_pDFLevel, nCount);
                    }
                    pData = m_pDFLevel + nCount;
                    break;
                case DF_FSTRENGTH:
                    m_pDFFStrength = (short*)pData;
                if (! (nSel & SWAP)) {
                        SwapBuffer(m_pDFFStrength, nCount);
                    }
                    pData = m_pDFFStrength + nCount;
                    break;
                case ELEVATION:
                    m_pElevation = (short*)pData;
                if (! (nSel & SWAP)) {
                        SwapBuffer(m_pElevation, nCount);
                    }
                    pData = m_pElevation + nCount;
                    break;
                case OMNIPHASE:
                    m_pOmniphase = (short*)pData;
                if (! (nSel & SWAP)) {
                        SwapBuffer(m_pOmniphase, nCount);
                    }
                    pData = m_pOmniphase + nCount;
                    break;
                case CHANNEL:
                    m_pChannel = (unsigned short*)pData;
                if (! (nSel & SWAP)) {
                        SwapBuffer((short*)m_pChannel, nCount);
                    }
                    pData = m_pChannel + nCount;
                    break;
                case FREQ_LOW:
                    m_pFreq = (unsigned long*)pData;
                    if (!(nSel & SWAP))
                    {
                        SwapBuffer((long*)m_pFreq, nCount);
                    }
                    pData = m_pFreq + nCount;
                    break;
                case FREQ_HIGH:
                    m_pFreqHigh = (unsigned long*)pData;
                    if (!(nSel & SWAP))
                    {
                        SwapBuffer((long*)m_pFreqHigh, nCount);
                    }
                    pData = m_pFreqHigh + nCount;
                    break;
            }
        }
        flag <<= 1;
    }
}

/* FUNCTION ********************************************************************/
void    CEB200UdpSock::ParseCommonHeader(struct CommonHeader *pCommon)
/*
SPECIFICATION:  Parse common attribute header and retrieves receiver instance (ChannelNumber)
PARAMETERS:     pCommon -   common header of generic attribute
PRECONDITIONS:
SIDE_EFFECTS:   changes byte order of NumberOfTraceItems and SelectorFlags
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    // change byte order
    pCommon->NumberOfTraceItems = ntohs(pCommon->NumberOfTraceItems);
    pCommon->SelectorFlags = ntohl(pCommon->SelectorFlags);

    // handle legacy reserved value
    if ( pCommon->ChannelNumber == 0x77 ) pCommon->ChannelNumber = 0;
    if ( pCommon->ChannelNumber != m_nDDCx )
    {
        m_nDDCx = pCommon->ChannelNumber;
        if (m_nDDCx > 0)
        {
            sprintf(m_cDDCx, "DDC%d", m_nDDCx);
        }
    }
}

/* FUNCTION ********************************************************************/
void    CEB200UdpSock::ParseCommonHeader(struct NewCommonHeader *pCommon)
/*
SPECIFICATION:  Parse common attribute header and retrieves receiver instance (ChannelNumber)
PARAMETERS:     pCommon -   common header of generic attribute
PRECONDITIONS:
SIDE_EFFECTS:   changes byte order of NumberOfTraceItems and SelectorFlags
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    // change byte order
    pCommon->NumberOfTraceItems = ntohl(pCommon->NumberOfTraceItems);
    pCommon->OptionalHeaderLength = ntohl(pCommon->OptionalHeaderLength);
    pCommon->SelectorFlagsLow = ntohl(pCommon->SelectorFlagsLow);
    pCommon->SelectorFlagsHigh = ntohl(pCommon->SelectorFlagsHigh);
    pCommon->ChannelNumber = ntohl(pCommon->ChannelNumber);

    if ( pCommon->ChannelNumber != m_nDDCx )
    {
        m_nDDCx = pCommon->ChannelNumber;
        if (m_nDDCx > 0)
        {
            sprintf(m_cDDCx, "DDC%d", m_nDDCx);
        }
    }
}

/* FUNCTION ********************************************************************/
void CEB200UdpSock::fprint_short_formated(short Data)
/*
SPECIFICATION:
PARAMETERS:
(short* :
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    if (Data < 0)
    {
        if (Data/10 == 0)
        {
            fprintf(m_pCSFile,"-0,%01d;", abs(Data)%10);
        }
        else
        {
            fprintf(m_pCSFile,"%d,%01d;", Data/10, abs(Data)%10);
        }
    }
    else
    {
        fprintf(m_pCSFile,"%d,%01d;", Data/10, Data%10);
    }
}


/* FUNCTION ********************************************************************/
void    CEB200UdpSock::Scan(struct GenericAttribute *pAttr)
/*
SPECIFICATION:
Generic part of FScan and MScan and CW.
This should be processed before OptionalHeader is processed as here are
the SelectorFlags swapped.
PARAMETERS:
struct GenericAttribute *pAttr : Pointer to generic attribute of type FSCAN
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    char cbuffer[200];
    char* p_cbuffer = cbuffer;

    struct CommonHeader *pCommon = (struct CommonHeader*)(pAttr->data);

    ParseCommonHeader(pCommon);
    ParseSelectorFlags(pCommon->SelectorFlags,
                       (void *)(((unsigned char*)pCommon)
                                + sizeof(struct CommonHeader)
                                + pCommon->OptionalHeaderLength),
                       pCommon->NumberOfTraceItems);

    DWORDLONG Freq;

    for (int i=0; i<pCommon->NumberOfTraceItems; i++)
    {
        if (!ChannelsOnly() && !BytesOnly())
        {
            printf ("%d: ", i);

            if (!((m_bCSRecording) && (m_pCSFile != NULL)))
            {
                m_bCSRecording = false;
            }
            else
            {
                fprintf (m_pCSFile,"\n");
            }

            Freq = 0;
            if (m_pFreq)
            {
                Freq = m_pFreq[i];
            }
            if (m_pFreqHigh)
            {
                Freq |= ((DWORDLONG)(m_pFreqHigh[i])) << 32;
            }
            if (m_pFreq || m_pFreqHigh)
            {
                unsigned long FreqTemp = (unsigned long)Freq;
                if (FreqTemp == Freq)
                {
                    printf(" Freq: %u", m_pFreq[i]);
                    if (m_bCSRecording)
                    {
                        fprintf (m_pCSFile,"%u;",m_pFreq[i]);
                    }
                }
                else
                {
                    FreqTemp = (unsigned long)(Freq / 1000000000);
                    unsigned long FreqRest = (unsigned long)(Freq % 1000000000);
                    printf(" Freq: %u%09u", FreqTemp, FreqRest);
                    if (m_bCSRecording)
                    {
                        fprintf (m_pCSFile,"%u%09u;",FreqTemp, FreqRest);
                    }
                }
            }
            if (m_pChannel)
                printf(" channel: %hd", m_pChannel[i]);
            if (m_pLevel)
            {
                printf(" level: %hd", m_pLevel[i]);
                if (m_bCSRecording)
                {
                    fprint_short_formated(m_pLevel[i]);
                }
            }
            if (m_pOffset)
                printf(" offset: %d", m_pOffset[i]);
            if (m_pFStrength)
            {
                printf(" fstrength: %hd", m_pFStrength[i]);
                if (m_bCSRecording)
                {
                    fprint_short_formated(m_pFStrength[i]);
                }
            }
            if (m_pAM)
                printf(" am: %hd", m_pAM[i]);
            if (m_pAMPos)
                printf(" ampos: %hd", m_pAMPos[i]);
            if (m_pAMNeg)
                printf(" amneg: %hd", m_pAMNeg[i]);
            if (m_pFM)
                printf(" fm: %d", m_pFM[i]);
            if (m_pFMPos)
                printf(" fmpos: %d", m_pFMPos[i]);
            if (m_pFMNeg)
                printf(" fmneg: %d", m_pFMNeg[i]);
            if (m_pPM)
                printf(" pm: %hd", m_pPM[i]);
            if (m_pBW)
                printf(" bw: %d", m_pBW[i]);
            if (m_pAzimuth)
            {
                printf(" azim: %hd", m_pAzimuth[i]);
                if (m_bCSRecording)
                {
                    fprint_short_formated(m_pAzimuth[i]);
                }
            }
            if (m_pQuality)
            {
                printf(" dfqual: %hd", m_pQuality[i]);
                if (m_bCSRecording)
                {
                    fprint_short_formated(m_pQuality[i]);
                }
            }
            if (m_pDFLevel)
            {
                printf(" dflevel: %hd", m_pDFLevel[i]);
                if (m_bCSRecording)
                {
                    fprint_short_formated(m_pDFLevel[i]);
                }
            }
            if (m_pDFFStrength)
            {
                printf(" dffstrength: %hd", m_pDFFStrength[i]);
                if (m_bCSRecording)
                {
                    fprint_short_formated(m_pDFFStrength[i]);
                }
            }
            if (m_pElevation)
            {
                printf(" elev: %hd", m_pElevation[i]);
                if (m_bCSRecording)
                {
                    fprint_short_formated(m_pElevation[i]);
                }
            }
            if (m_pOmniphase)
            {
                printf(" Omniphase: %hd", m_pOmniphase[i]);
                if (m_bCSRecording)
                {
                    fprint_short_formated(m_pOmniphase[i]);
                }
            }
            printf("\n");

        }
    }
}

/* FUNCTION ********************************************************************/
void    CEB200UdpSock::FScan(struct GenericAttribute *pAttr)
/*
SPECIFICATION:
Here are all actions coded which should be done in case of FSCAN data. This
member function can be overwritten in derived class to change default behaviour.
PARAMETERS:
struct GenericAttribute *pAttr : Pointer to generic attribute of type FSCAN
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    struct CommonHeader *pCommon = (struct CommonHeader*)(pAttr->data);
    struct OptHeaderFScan *pOptHeader;

    if (!ChannelsOnly() && !BytesOnly())
    {
        puts("FSCAN:\n");
    }
    Scan(pAttr);

    if (pCommon->SelectorFlags & OPTHEADER )
    {
        pOptHeader = (struct OptHeaderFScan *)(((unsigned char *)pCommon)+sizeof(*pCommon));
        if (! (pCommon->SelectorFlags & SWAP) )
        {
            // change byte order of optional header
            pOptHeader->CycleCount = ntohs(pOptHeader->CycleCount);
            pOptHeader->HoldTime = ntohs(pOptHeader->HoldTime);;
            pOptHeader->DwellTime = ntohs(pOptHeader->DwellTime);;
            pOptHeader->DirectionUp = ntohs(pOptHeader->DirectionUp);;
            pOptHeader->StopSignal = ntohs(pOptHeader->StopSignal);;
            pOptHeader->StartFreq_low = ntohl(pOptHeader->StartFreq_low);
            pOptHeader->StopFreq_low = ntohl(pOptHeader->StopFreq_low);
            pOptHeader->StepFreq = ntohl(pOptHeader->StepFreq);
            if (m_b64BitFrequencies)
            {
                pOptHeader->StartFreq_high = ntohl(pOptHeader->StartFreq_high);
                pOptHeader->StopFreq_high = ntohl(pOptHeader->StopFreq_high);
            }
            if (m_bTimestamp)
            {
                pOptHeader->OutputTimestamp = Swap64(pOptHeader->OutputTimestamp);
            }
        }
        if (m_bTimestamp)
        {
            Vprintf("Output time : %I64u\n", (pOptHeader->OutputTimestamp));
        }
    }
    m_nFScanPackets++;
}

/* FUNCTION ********************************************************************/
void    CEB200UdpSock::MScan(struct GenericAttribute *pAttr)
/*
SPECIFICATION:
Here are all actions coded which should be done in case of MSCAN data. This
member function can be overwritten in derived class to change default behaviour.
PARAMETERS:
struct GenericAttribute *pAttr : Pointer to generic attribute of type MSCAN
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    struct CommonHeader *pCommon = (struct CommonHeader*)(pAttr->data);
    struct OptHeaderMScan *pOptHeader;

    if (!ChannelsOnly() && !BytesOnly())
    {
        puts("MSCAN:\n");
    }
    Scan(pAttr);

    if (pCommon->SelectorFlags & OPTHEADER )
    {
        pOptHeader = (struct OptHeaderMScan *)(((unsigned char *)pCommon)+sizeof(*pCommon));
        if (! (pCommon->SelectorFlags & SWAP) )
        {
            // change byte order of optional header
            if (m_bTimestamp)
            {
                pOptHeader->OutputTimestamp = Swap64(pOptHeader->OutputTimestamp);
            }
        }
        if (m_bTimestamp)
        {
            Vprintf("Output time : %I64u\n", (pOptHeader->OutputTimestamp));
        }
    }
    m_nMScanPackets++;
}

/* FUNCTION ********************************************************************/
void    CEB200UdpSock::DScan(struct GenericAttribute *pAttr)
/*
SPECIFICATION:
Here are all actions coded which should be done in case of DSCAN data. This
member function can be overwritten in derived class to change default behaviour.
PARAMETERS:
struct GenericAttribute *pAttr : Pointer to generic attribute of type DSCAN
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    struct CommonHeader *pCommon = (struct CommonHeader*)(pAttr->data);
    struct OptHeaderDScan *pOptHeader;

    if (!ChannelsOnly() && !BytesOnly())
    {
        puts("DSCAN:\n");
    }
    Scan(pAttr);

    //unsigned short *pLevel;
    //unsigned long *pFreq;

    // change byte order
    //pCommon->n = ntohs(pCommon->n);
    //pCommon->SelectorFlags = ntohl(pCommon->SelectorFlags);
    if (pCommon->SelectorFlags & OPTHEADER )
    {
        pOptHeader = (struct OptHeaderDScan *)(((unsigned char *)pCommon)+sizeof(*pCommon));
        if (! (pCommon->SelectorFlags & SWAP) )
        {
            // change byte order of optional header
            pOptHeader->StartFreq_low = ntohl(pOptHeader->StartFreq_low);
            pOptHeader->StopFreq_low = ntohl(pOptHeader->StopFreq_low);
            pOptHeader->StepFreq = ntohl(pOptHeader->StepFreq);
            pOptHeader->FMark_low = ntohl(pOptHeader->FMark_low);
            pOptHeader->BWZoom = ntohs(pOptHeader->BWZoom);
            pOptHeader->RefLevel = ntohs(pOptHeader->RefLevel);
            // newStepScheme is included form VersionMinor >= 0x24)
            pOptHeader->newStepScheme = ntohs(pOptHeader->newStepScheme);
            if (m_b64BitFrequencies)
            {
                pOptHeader->StartFreq_high = ntohl(pOptHeader->StartFreq_high);
                pOptHeader->StopFreq_high = ntohl(pOptHeader->StopFreq_high);
                pOptHeader->FMark_high = ntohl(pOptHeader->FMark_high);
            }
        }
    }
    m_nDScanPackets++;
}

/* FUNCTION ********************************************************************/
void    CEB200UdpSock::PScan(struct GenericAttribute *pAttr)
/*
SPECIFICATION:
Here are all actions coded which should be done in case of DSCAN data. This
member function can be overwritten in derived class to change default behaviour.
PARAMETERS:
struct GenericAttribute *pAttr : Pointer to generic attribute of type DSCAN
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    struct CommonHeader *pCommon = (struct CommonHeader*)(pAttr->data);
    struct OptHeaderPScan *pOptHeader;

    if (!ChannelsOnly() && !BytesOnly())
    {
        puts("PSCAN:\n");
    }
    Scan(pAttr);

    if (pCommon->SelectorFlags & OPTHEADER )
    {
        pOptHeader = (struct OptHeaderPScan *)(((unsigned char *)pCommon)+sizeof(*pCommon));
        if (! (pCommon->SelectorFlags & SWAP) )
        {
            // change byte order of optional header
            pOptHeader->StartFreq_low = ntohl(pOptHeader->StartFreq_low);
            pOptHeader->StopFreq_low = ntohl(pOptHeader->StopFreq_low);
            pOptHeader->StepFreq = ntohl(pOptHeader->StepFreq);
            if (m_b64BitFrequencies)
            {
                pOptHeader->StartFreq_high = ntohl(pOptHeader->StartFreq_high);
                pOptHeader->StopFreq_high = ntohl(pOptHeader->StopFreq_high);
            }
            if (m_bTimestamp)
            {
                pOptHeader->OutputTimestamp = Swap64(pOptHeader->OutputTimestamp);
            }
            if (pCommon->OptionalHeaderLength > (offsetof(OptHeaderPScan, OutputTimestamp) + sizeof(pOptHeader->OutputTimestamp)))
            {
                pOptHeader->FStepNumerator   = ntohl(pOptHeader->FStepNumerator);
                pOptHeader->FStepDenominator = ntohl(pOptHeader->FStepDenominator);
                pOptHeader->FreqOfFirstStep  = Swap64(pOptHeader->FreqOfFirstStep);
                Vprintf("FStep Numerator: %d   Denominator: %d   Value: %f Hz\n", 
                    (pOptHeader->FStepNumerator), (pOptHeader->FStepDenominator), 
                    ((float)pOptHeader->FStepNumerator / (float)pOptHeader->FStepDenominator));
                Vprintf("FreqOfFirstStep : %I64u\n", (pOptHeader->FreqOfFirstStep));
            }
        }

        if (m_bTimestamp)
        {
            Vprintf("Output time : %I64u\n", (pOptHeader->OutputTimestamp));
        }
    }
    m_nPScanPackets++;
}

/* FUNCTION ********************************************************************/
void    CEB200UdpSock::Audio(struct GenericAttribute *pAttr)
/*
SPECIFICATION:
Here are all actions coded which should be done in case of AUDIO data. This
member function can be overwritten in derived class to change default behaviour
PARAMETERS:
struct GenericAttribute *pAttr : Pointer to generic attribute of type AUDIO
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    struct CommonHeader *pCommon = (struct CommonHeader*)(pAttr->data);
    struct OptHeaderAudio *pOptHeader;
    unsigned uStride;

    ParseCommonHeader(pCommon);
    if (pCommon->SelectorFlags & OPTHEADER)
    {
        // change byte order of optional header
        pOptHeader = (struct OptHeaderAudio *)(((unsigned char *)pCommon)+sizeof(*pCommon));
        if (! (pCommon->SelectorFlags & SWAP))
        {
            pOptHeader->AudioMode = ntohs(pOptHeader->AudioMode);
            pOptHeader->FrameLength = ntohs(pOptHeader->FrameLength);
            pOptHeader->Freq_low = ntohl(pOptHeader->Freq_low);
            pOptHeader->Bandwidth = ntohl(pOptHeader->Bandwidth);
            pOptHeader->Demodulation = ntohs(pOptHeader->Demodulation);
            if (m_b64BitFrequencies)
            {
                pOptHeader->Freq_high = ntohl(pOptHeader->Freq_high);
            }
            if (m_bTimestamp)
            {
                pOptHeader->OutputTimestamp = Swap64(pOptHeader->OutputTimestamp);
            }
        }
        if (m_bTimestamp)
        {
            Vprintf("Output time : %I64u\n", (pOptHeader->OutputTimestamp));
        }
        strncpy(m_sDemod, pOptHeader->DemodulationString, sizeof(m_sDemod));
        if (pOptHeader->AudioMode > 0)
        {
            ASSERT(pOptHeader->AudioMode < (sizeof(m_AudioFormats)/sizeof(m_AudioFormats[0])));
            ASSERT(m_AudioFormats[pOptHeader->AudioMode].cFrameLength == pOptHeader->FrameLength);

            int nSamples = pCommon->NumberOfTraceItems * m_AudioFormats[pOptHeader->AudioMode].nNoOfChannels;
            // any samples in datagram ?
            if (nSamples == 0)
                return;                                     // no, nothing to do

            unsigned short *pAudioData = (unsigned short*)(((unsigned char*)pCommon) + sizeof(struct CommonHeader) + pCommon->OptionalHeaderLength);
            if (pOptHeader->AudioMode <= PCM_MAX_AUDIO)
            {
                if (! (pCommon->SelectorFlags & SWAP))
                {
                    // for all PCM formats
                    if (m_AudioFormats[pOptHeader->AudioMode].nBitsPerSample != 8)
                    {
                        // perform byte swap, because we got 16 bits samples
                        int i;
                        for (i=0; i<nSamples; i++)
                        {
                            pAudioData[i] = ntohs(pAudioData[i]);
                        }
                    }
                }

                if (m_bAFRecording)
                {
                    uStride = (m_AudioFormats[pOptHeader->AudioMode].nBitsPerSample / 8) * m_AudioFormats[pOptHeader->AudioMode].nNoOfChannels;

                    /* Check if file already open */
                    if (!m_pctIQFile->IsOpen())
                    {
                        /* Open WAV file */
                        if (!m_pctIQFile->Open(m_AudioFormats[pOptHeader->AudioMode].nSampleRate,
                                             m_AudioFormats[pOptHeader->AudioMode].nNoOfChannels,
                                             uStride, pOptHeader->OutputTimestamp))
                        {
                            printf("Can't write to file %s.\n", m_pctIQFile->GetFilename());
                            m_bAFRecording = false;
                        }
                    }

                    /* Write data to WAV file */
                    m_pctIQFile->write(pAudioData, (pCommon->NumberOfTraceItems) * uStride);
                }
                else
                {
                    // output samples to sound output device
                    m_Sound.Play(false, m_AudioFormats[pOptHeader->AudioMode].nSampleRate,
                                 m_AudioFormats[pOptHeader->AudioMode].nBitsPerSample,
                                 m_AudioFormats[pOptHeader->AudioMode].nNoOfChannels,
                                 pAudioData, pCommon->NumberOfTraceItems);
                }
            }
            else if (pOptHeader->AudioMode == GSM_AUDIO)
            {
                // output samples to sound output device
                m_Sound.Play(true, m_AudioFormats[pOptHeader->AudioMode].nSampleRate,
                             m_AudioFormats[pOptHeader->AudioMode].nBitsPerSample,
                             m_AudioFormats[pOptHeader->AudioMode].nNoOfChannels,
                             pAudioData, pCommon->NumberOfTraceItems);

            }
        }
        else
        {
            m_Sound.FlushBuffers();
        }
    }
    m_nAudioPackets++;
}

/* FUNCTION ********************************************************************/
void    CEB200UdpSock::IF(struct GenericAttribute *pAttr)
/*
SPECIFICATION:
Here are all actions coded which should be done in case of IF data. This
member function can be overwritten in derived class to change default behaviour
PARAMETERS:
struct GenericAttribute *pAttr : Pointer to generic attribute of type IF
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    struct CommonHeader *pCommon = (struct CommonHeader*)(pAttr->data);
    struct OptHeaderIF *pOptHeader;

    ParseCommonHeader(pCommon);
    if (pCommon->SelectorFlags & OPTHEADER)
    {
        // change byte order of optional header
        pOptHeader = (struct OptHeaderIF *)(((unsigned char *)pCommon)+sizeof(*pCommon));
        if (! (pCommon->SelectorFlags & SWAP))
        {
            pOptHeader->IFMode = ntohs(pOptHeader->IFMode);
            pOptHeader->FrameLength = ntohs(pOptHeader->FrameLength);
            pOptHeader->Freq_low = ntohl(pOptHeader->Freq_low);
            pOptHeader->Bandwidth = ntohl(pOptHeader->Bandwidth);
            pOptHeader->Demodulation = ntohs(pOptHeader->Demodulation);
            pOptHeader->RxAttenuation = ntohs(pOptHeader->RxAttenuation);
            pOptHeader->Flags = ntohs(pOptHeader->Flags);
            pOptHeader->kFactor = ntohs(pOptHeader->kFactor);
            pOptHeader->SamplerRate = ntohl(pOptHeader->SamplerRate);
            pOptHeader->SampleCount = Swap64(pOptHeader->SampleCount);
            if (m_b64BitFrequencies)
            {
                pOptHeader->Freq_high = ntohl(pOptHeader->Freq_high);
            }
            if (m_bTimestamp)
            {
                pOptHeader->StartTimestamp = Swap64(pOptHeader->StartTimestamp);
            }
        }
        if (m_bTimestamp)
        {
//            Vprintf("Start time : %I64u\n", (pOptHeader->StartTimestamp));
            m_nIFStartTimestamp = pOptHeader->StartTimestamp;
        }
        m_nIFSampleRate = pOptHeader->SamplerRate;
        strncpy(m_sDemod, pOptHeader->DemodulationString, sizeof(m_sDemod));
        if (pOptHeader->SampleCount != m_nIFSampleCounter)
        {
            if (m_nIFSampleCounter != 0)
            {
                printf("Lost %I64d samples\n", pOptHeader->SampleCount - m_nIFSampleCounter);
                if (m_pctIQFile->IsOpen())
                {
                    /* close file, b/c it's corrupted -> it will be reopened (see below) */
                    m_pctIQFile->Close(true);
                }
            }
            m_nIFSampleCounter = pOptHeader->SampleCount;
        }

        if (pOptHeader->IFMode > 0)
        {
            m_flags = pOptHeader->Flags;
            if (!(m_flags & IF_DATA_SIGNAL_VALID))
            {
                printf("Signal invalid for %d samples !!!!!!!!!!!!!!!!\n", pCommon->NumberOfTraceItems);
            }
            if ((m_flags & IF_DATA_BLANKING))
            {
                printf("Blanking activ for %d samples !!!!!!!!!!!!!!!!\n", pCommon->NumberOfTraceItems);
            }
            m_nRxAtt = pOptHeader->RxAttenuation;
            if (m_flags & IF_DATA_ANT_FACTOR_VALID)
            {
                m_kFactor = pOptHeader->kFactor;
            }
            else
            {
                m_kFactor = 0x7FFF; // means invalid
            }

            int nSamples = pCommon->NumberOfTraceItems * 2;                  // two Channels;
            // any samples in datagram ?
            if (nSamples == 0)
                return;                                     // no, nothing to do
            if (pOptHeader->FrameLength == 4)
            {
                // 16 Bit I and 16 Bit Q Samples
                short *pIQ = (short*)(((char*)pCommon) + sizeof(struct CommonHeader) + pCommon->OptionalHeaderLength);
                // change byte order of data
                int i;
                if (!(pCommon->SelectorFlags & SWAP))
                {
                    for (i=0; i<nSamples; i++)
                    {
                        pIQ[i] = ntohs(pIQ[i]);
                    }
                }
                // pIQ points to IQ Samples
                // pCommon->n gives the number of complex samples
                // only for test purposes: calculate rms value over received IQ samples
                double lfRms = 0;
                double lfI;
                double lfQ;
                for (i=0; i<nSamples; i+=2)
                {
                    // I Sample
                    // INT16 -> double conversion
                    lfI = pIQ[i];
                    // norm to max. value 1.0
                    lfI /= 32768.0                          /* 2^15 */;
                    // Q Sample
                    // INT16 -> double conversion
                    lfQ = pIQ[i+1];
                    // norm to max. value 1.0
                    lfQ /= 32768.0                          /* 2^15 */;

                    // now sum of power
                    lfRms += lfI * lfI + lfQ * lfQ;
                }
                // RMS calculation
                lfRms /= pCommon->NumberOfTraceItems;
                // conversion to dBFS (full scale)
                lfRms = 10.0 * log10(lfRms);
                m_lfRms = lfRms;
            }
            else
            {
                // 32 Bit I and 32 Bit Q Samples
                long *pIQ = (long*)(((char*)pCommon) + sizeof(struct CommonHeader) + pCommon->OptionalHeaderLength);
                // change byte order of data
                int i;
                if (!(pCommon->SelectorFlags & SWAP))
                {
                    for (i=0; i<nSamples; i++)
                    {
                        pIQ[i] = ntohl(pIQ[i]);
                    }
                }
                // pIQ points to IQ Samples
                // pCommon->n gives the number of complex samples
                // only for test purposes: calculate rms value over received IQ samples
                double lfRms = 0;
                double lfI;
                double lfQ;
                for (i=0; i<nSamples; i+=2)
                {
                    // I Sample
                    // INT32 -> double conversion
                    lfI = pIQ[i];
                    // norm to max. value 1.0
                    lfI /=  2147483648.0                    /* 2^31 */;
                    // Q Sample
                    // INT32 -> double conversion
                    lfQ = pIQ[i+1];
                    // norm to max. value 1.0
                    lfQ /= 2147483648.0                     /* 2^31 */;

                    // now sum of power
                    lfRms += lfI * lfI + lfQ * lfQ;
                }
                // RMS calculation
                lfRms /= pCommon->NumberOfTraceItems;
                // conversion to dBFS (full scale)
                lfRms = 10.0 * log10(lfRms);
                m_lfRms = lfRms;
            }

            if (m_bIFRecording)
            {
                if (!m_bRaw)
                {
                    /* create metainfo */
                    META_INFO_TYPE metainfo;
                    metainfo.bandwidth = pOptHeader->Bandwidth;
                    metainfo.freq = pOptHeader->Freq_high;
                    metainfo.freq <<= 32;
                    metainfo.freq |= pOptHeader->Freq_low;
                    strncpy(metainfo.sDemod, m_sDemod, sizeof(metainfo.sDemod));
                    /* Check if file already open */
                    if (!m_pctIQFile->IsOpen())
                    {
              /* Open WAV file with given sample rate, double channel for I and Q,
               * and the given number of bytes per sample */
                        if (!m_pctIQFile->Open(pOptHeader->SamplerRate, 2, pOptHeader->FrameLength,
                                             CalcTimestamp(m_nIFStartTimestamp, m_nIFSampleCounter, (DWORD)m_nIFSampleRate)
                                             , &metainfo
                                             ))
                        {
                            printf("Can't write to file %s.\n", m_pctIQFile->GetFilename());
                            m_bIFRecording = false;
                        }
                    }
                    else
                    {
                        /* file is already open */
                        /* Check if settings have changed */
                        if (pOptHeader->SamplerRate != m_pctIQFile->GetSampleRate() ||
                            pOptHeader->FrameLength != m_pctIQFile->GetSampleWidth())
                        {
                            /* paramters were changed -> close file */
                            m_pctIQFile->Close();
                            /* reopen with new parameters */
                            if (!m_pctIQFile->Open(pOptHeader->SamplerRate, 2, pOptHeader->FrameLength,
                                                CalcTimestamp(m_nIFStartTimestamp, m_nIFSampleCounter, (DWORD)m_nIFSampleRate)
                                                , &metainfo
                                                ))
                            {
                                printf("Can't write to file %s.\n", m_pctIQFile->GetFilename());
                                m_bIFRecording = false;
                            }
                        }
                    }

                    if (m_bIFRecording)
                    {
                        /* Write data to WAV file */
                        if (m_pctIQFile->write((void *)((char *)pOptHeader + pCommon->OptionalHeaderLength),
                                         pCommon->NumberOfTraceItems * pOptHeader->FrameLength) == 0)
                        {
                            // no bytes written -> stop program
                            m_bStop = TRUE;
                        }
                    }
                }
                else
                {
                    if (m_pRawFile != NULL)
                    {
                        fwrite((void *)pAttr, pAttr->length, 1, m_pRawFile);
                    }
                }
            }
        }
        m_nIFSampleCounter += pCommon->NumberOfTraceItems;

    }
    m_nIFPackets++;
}
/* FUNCTION ********************************************************************/
void    CEB200UdpSock::Video(struct GenericAttribute *pAttr)
/*
SPECIFICATION:
Here are all actions coded which should be done in case of Video data. This
member function can be overwritten in derived class to change default behaviour
PARAMETERS:
struct GenericAttribute *pAttr : Pointer to generic attribute of type Video
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    struct CommonHeader *pCommon = (struct CommonHeader*)(pAttr->data);
    OPT_HEADER_VIDEO_TYPE *pOptHeader;

    ParseCommonHeader(pCommon);
    if (pCommon->SelectorFlags & OPTHEADER)
    {
        // change byte order of optional header
        pOptHeader = (OPT_HEADER_VIDEO_TYPE*)(((unsigned char *)pCommon)+sizeof(*pCommon));
        if (! (pCommon->SelectorFlags & SWAP))
        {
            pOptHeader->IFMode = ntohs(pOptHeader->IFMode);
            pOptHeader->FrameLength = ntohs(pOptHeader->FrameLength);
            pOptHeader->Freq_low = ntohl(pOptHeader->Freq_low);
            pOptHeader->Bandwidth = ntohl(pOptHeader->Bandwidth);
            pOptHeader->Demodulation = ntohs(pOptHeader->Demodulation);
            pOptHeader->RxAttenuation = ntohs(pOptHeader->RxAttenuation);
            pOptHeader->Flags = ntohs(pOptHeader->Flags);
            pOptHeader->SamplerRate = ntohl(pOptHeader->SamplerRate);
            pOptHeader->SampleCount = Swap64(pOptHeader->SampleCount);
            if (m_b64BitFrequencies)
            {
                pOptHeader->Freq_high = ntohl(pOptHeader->Freq_high);
            }
            if (m_bTimestamp)
            {
                pOptHeader->StartTimestamp = Swap64(pOptHeader->StartTimestamp);
            }
        }
        if (m_bTimestamp)
        {
            Vprintf("Start time : %I64u\n", (pOptHeader->StartTimestamp));
        }
        strncpy(m_sDemod, pOptHeader->DemodulationString, sizeof(m_sDemod));
        if (pOptHeader->SampleCount != m_nVideoSampleCounter)
        {
            printf("Lost %I64d samples\n", pOptHeader->SampleCount - m_nVideoSampleCounter);
            m_nVideoSampleCounter = pOptHeader->SampleCount;
        }

        m_nVideoSampleCounter += pCommon->NumberOfTraceItems;
        if (pOptHeader->IFMode > 0)
        {
            int nSamples = pCommon->NumberOfTraceItems * 2;                  // two Channels;
            // any samples in datagram ?
            if (nSamples == 0)
                return;                                     // no, nothing to do
            if (pOptHeader->FrameLength == 4)
            {
                // 16 Bit I and 16 Bit Q Samples
                short *pIQ = (short*)(((char*)pCommon) + sizeof(struct CommonHeader) + pCommon->OptionalHeaderLength);
                // change byte order of data
                int i;
                if (!(pCommon->SelectorFlags & SWAP))
                {
                    for (i=0; i<nSamples; i++)
                    {
                        pIQ[i] = ntohs(pIQ[i]);
                    }
                }
                // pIQ points to IQ Samples
                // pCommon->n gives the number of complex samples
            }
            else
            {
                // 32 Bit I and 32 Bit Q Samples
                long *pIQ = (long*)(((char*)pCommon) + sizeof(struct CommonHeader) + pCommon->OptionalHeaderLength);
                // change byte order of data
                int i;
                if (!(pCommon->SelectorFlags & SWAP))
                {
                    for (i=0; i<nSamples; i++)
                    {
                        pIQ[i] = ntohl(pIQ[i]);
                    }
                }
                // pIQ points to IQ Samples
                // pCommon->n gives the number of complex samples
            }
        }
        else
        {
            m_nVideoSampleCounter = 0;
        }
    }
    m_nVideoPackets++;
}

/* FUNCTION ********************************************************************/
void    CEB200UdpSock::IFPan(struct GenericAttribute *pAttr)
/*
SPECIFICATION:
Here are all actions coded which should be done in case of IFPAN data. This
member function can be overwritten in derived class to change default behaviour.
PARAMETERS:
struct GenericAttribute *pAttr : Pointer to generic attriubute of type IFPAN
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    struct CommonHeader *pCommon = (struct CommonHeader*)(pAttr->data);
    struct OptHeaderIFPan *pOptHeader;
    unsigned short *pLevel;

    ParseCommonHeader(pCommon);
    if (pCommon->SelectorFlags & OPTHEADER )
    {
        pOptHeader = (struct OptHeaderIFPan *)(((unsigned char *)pCommon)+sizeof(*pCommon));
        if (! (pCommon->SelectorFlags & SWAP) )
        {
            // change byte order of optional header
            pOptHeader->Freq_low = ntohl(pOptHeader->Freq_low);
            pOptHeader->FSpan = ntohl(pOptHeader->FSpan);
            pOptHeader->AvgTime = ntohs(pOptHeader->AvgTime);
            pOptHeader->AvgType = ntohs(pOptHeader->AvgType);
            pOptHeader->MeasureTime = ntohl(pOptHeader->MeasureTime);
            if (m_b64BitFrequencies)
            {
                pOptHeader->Freq_high = ntohl(pOptHeader->Freq_high);
            }
            if (pCommon->OptionalHeaderLength >= offsetof(OptHeaderIFPan, OutputTimestamp))
            {
                // change byte order of new elements
                pOptHeader->DemodFreqChannel = ntohl(pOptHeader->DemodFreqChannel);
                pOptHeader->DemodFreq_low = ntohl(pOptHeader->DemodFreq_low);
                pOptHeader->DemodFreq_high = ntohl(pOptHeader->DemodFreq_high);
            }
            if (m_bTimestamp)
            {
                pOptHeader->OutputTimestamp = Swap64(pOptHeader->OutputTimestamp);
            }
            
            if (pCommon->OptionalHeaderLength > (offsetof(OptHeaderIFPan, OutputTimestamp) + sizeof(pOptHeader->OutputTimestamp)))
            {
                pOptHeader->FStepNumerator   = ntohl(pOptHeader->FStepNumerator);
                pOptHeader->FStepDenominator = ntohl(pOptHeader->FStepDenominator);
                Vprintf("FStep Numerator: %d   Denominator: %d   Value: %f Hz\n", 
                    (pOptHeader->FStepNumerator), (pOptHeader->FStepDenominator), 
                    ((float)pOptHeader->FStepNumerator / (float)pOptHeader->FStepDenominator));
            }
        }

        if (m_bTimestamp)
        {
            Vprintf("Output time : %I64u\n", pOptHeader->OutputTimestamp);
        }

        INT64 Freq = pOptHeader->Freq_high;
        Freq <<= 32;
        Freq |= pOptHeader->Freq_low;

        INT64 DemodFreq = pOptHeader->DemodFreq_high;
        DemodFreq <<= 32;
        DemodFreq |= pOptHeader->DemodFreq_low;

        Vprintf("IFPan   Freq: %I64u Hz Span: %d Hz DispVar: %d  MeasureTime: %d us  DemFreq: %I64u Hz\n", Freq, pOptHeader->FSpan, pOptHeader->AvgType, pOptHeader->MeasureTime, DemodFreq);

    }
    if (pCommon->SelectorFlags & LEVEL)
    {
        pLevel = (unsigned short*)(((unsigned char*)pCommon) + sizeof(struct CommonHeader) + pCommon->OptionalHeaderLength);
        // change byte order of IFPAN data
        int i;
        for (i=0; i<pCommon->NumberOfTraceItems; i++)
        {
            if (! (pCommon->SelectorFlags & SWAP)) {
                pLevel[i] = ntohs(pLevel[i]);
            }
//          Vprintf("IFP level: %d\n", pLevel[i]);
        }
    }
    m_nIFPanPackets++;
}

/* FUNCTION ********************************************************************/
void    CEB200UdpSock::DFPan(struct GenericAttribute *pAttr)
/*
SPECIFICATION:
Here are all actions coded which should be done in case of DFPAN data. This
member function can be overwritten in derived class to change default behaviour.
PARAMETERS:
struct GenericAttribute *pAttr : Pointer to generic attriubute of type DFPAN
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    struct CommonHeader *pCommon = (struct CommonHeader*)(pAttr->data);
    struct OptHeaderDFPan *pOptHeader;
    struct GPSHeader *pGPSHeader;
    signed short *pLevel;
    signed short *pAzi;
    signed short *pQual;
    signed short *pElev;
    signed short *pOmni;
    bool bNoDemodFreq = true;

    DWORDLONG Freq = 0;
    DWORDLONG DemodFreq = 0;

    if (!((m_bCSRecording) && (m_pCSFile != NULL)))
    {
        m_bCSRecording = false;
    }
    else
    {
        fprintf (m_pCSFile,"\n");
    }

    ParseCommonHeader(pCommon);
    if (pCommon->SelectorFlags & OPTHEADER )
    {
        pOptHeader = (struct OptHeaderDFPan *)(((unsigned char *)pCommon)+sizeof(*pCommon));
        pGPSHeader = (struct GPSHeader *)(((unsigned char *) pOptHeader)+offsetof(OptHeaderDFPan, GPSHeader));

        if (! (pCommon->SelectorFlags & SWAP) )
        {
            // change byte order of optional header
            pOptHeader->Freq_low = ntohl(pOptHeader->Freq_low);
            pOptHeader->Freq_high = ntohl(pOptHeader->Freq_high);
            pOptHeader->FreqSpan = ntohl(pOptHeader->FreqSpan);
            pOptHeader->DFThresholdMode = ntohl(pOptHeader->DFThresholdMode);
            pOptHeader->DFThresholdValue = ntohl(pOptHeader->DFThresholdValue);
            pOptHeader->DfBandwidth = ntohl(pOptHeader->DfBandwidth);
            pOptHeader->StepWidth = ntohl(pOptHeader->StepWidth);
            pOptHeader->DFMeasureTime = ntohl(pOptHeader->DFMeasureTime);
            pOptHeader->DFOption = ntohl(pOptHeader->DFOption);
            Freq = pOptHeader->Freq_low;
            Freq |= ((DWORDLONG)(pOptHeader->Freq_high)) << 32;
            if (pCommon->OptionalHeaderLength >= offsetof(OptHeaderDFPan, GPSHeader))
            {
                pOptHeader->CompassHeading = ntohs(pOptHeader->CompassHeading);
                pOptHeader->CompassHeadingType = ntohs(pOptHeader->CompassHeadingType);
                pOptHeader->AntennaFactor = ntohl(pOptHeader->AntennaFactor);
                pOptHeader->DemodFreqChannel = ntohl(pOptHeader->DemodFreqChannel);
                pOptHeader->DemodFreq_low = ntohl(pOptHeader->DemodFreq_low);
                pOptHeader->DemodFreq_high = ntohl(pOptHeader->DemodFreq_high);
                DemodFreq = pOptHeader->DemodFreq_low;
                DemodFreq |= ((DWORDLONG)(pOptHeader->DemodFreq_high)) << 32;
                bNoDemodFreq = false;

                pGPSHeader->bValid = ntohs(pGPSHeader->bValid);
                pGPSHeader->iNoOfSatInView = ntohs(pGPSHeader->iNoOfSatInView);
                pGPSHeader->iLatRef = ntohs(pGPSHeader->iLatRef);
                pGPSHeader->iLatDeg = ntohs(pGPSHeader->iLatDeg);
                unsigned long *pHelp = (unsigned long *)&(pGPSHeader->fLatMin);
                *pHelp = ntohl(*pHelp);
                pGPSHeader->iLonRef = ntohs(pGPSHeader->iLonRef);
                pGPSHeader->iLonDeg = ntohs(pGPSHeader->iLonDeg);
                pHelp = (unsigned long *)&(pGPSHeader->fLonMin);
                *pHelp = ntohl(*pHelp);

                if (pCommon->OptionalHeaderLength > (offsetof(OptHeaderDFPan, GPSHeader) + offsetof(GPSHeader, fPdop)))
                {
                    pGPSHeader->fPdop            =   ntohf(pGPSHeader->fPdop);
                }

                if (pCommon->OptionalHeaderLength > (offsetof(OptHeaderDFPan, GPSHeader) + sizeof(GPSHEADER_TYPE)))
                {
                    pOptHeader->FStepNumerator   = ntohl(pOptHeader->FStepNumerator);
                    pOptHeader->FStepDenominator = ntohl(pOptHeader->FStepDenominator);
                    Vprintf("FStep Numerator: %d   Denominator: %d   Value: %f Hz\n", 
                        (pOptHeader->FStepNumerator), (pOptHeader->FStepDenominator), 
                        ((float)pOptHeader->FStepNumerator / (float)pOptHeader->FStepDenominator));
                }
                if (pCommon->OptionalHeaderLength > (offsetof(OptHeaderDFPan, FStepDenominator) + sizeof(pOptHeader->FStepDenominator)))
                {
                    pOptHeader->DFBandwidthHighRes   = Swap64(pOptHeader->DFBandwidthHighRes);
                    Vprintf("DF Bandwidth : %4.3lf Hz\n", ((double)(signed)(pOptHeader->DFBandwidthHighRes))/1000);
                }
            }
            if (m_bTimestamp)
            {
                pOptHeader->OutputTimestamp = Swap64(pOptHeader->OutputTimestamp);
            }


            printf("DF Antenna Factor : %4.1lf dB/m\n", (double)((pOptHeader->AntennaFactor))/10);


        }
        if (m_bTimestamp)
        {
            printf("Position:  %c,%hd,%-2.4f,%c,%hd,%-2.4f\n",
                pGPSHeader->iLatRef,
                pGPSHeader->iLatDeg,
                pGPSHeader->fLatMin,
                pGPSHeader->iLonRef,
                pGPSHeader->iLonDeg,
                pGPSHeader->fLonMin);
                if (pCommon->OptionalHeaderLength > (offsetof(OptHeaderDFPan, GPSHeader) + offsetof(GPSHeader, fPdop)))
                {
                    printf("PDOP: %.2f\n", pGPSHeader->fPdop);
                }
        }

        if (m_bTimestamp)
        {
            Vprintf("Output time : %I64u\n", pOptHeader->OutputTimestamp);
        }

    }
    if ((pCommon->SelectorFlags & (DF_LEVEL|AZIMUTH|DF_QUALITY)) == (DF_LEVEL|AZIMUTH|DF_QUALITY))
    {
        pLevel = (short*)(((unsigned char*)pCommon) + sizeof(struct CommonHeader) + pCommon->OptionalHeaderLength);
        pAzi = pLevel + pCommon->NumberOfTraceItems;
        pQual = pAzi + pCommon->NumberOfTraceItems;
        if (pCommon->SelectorFlags & ELEVATION)
        {
            pElev = pQual + pCommon->NumberOfTraceItems;

            if (pCommon->SelectorFlags & OMNIPHASE)
            {
                pOmni = pElev + pCommon->NumberOfTraceItems;
            }
            else
            {
                pOmni = NULL;
            }
        }
        else
        {
            pElev = NULL;

            if (pCommon->SelectorFlags & OMNIPHASE)
            {
                pOmni = pQual + pCommon->NumberOfTraceItems;
            }
            else
            {
                pOmni = NULL;
            }
        }

        // change byte order of DFPAN data
        int i;
        signed short maxLevel = -10000;
        signed short maxAzi;
        signed short maxQual;
        signed short maxDFFS = 0;
        signed short maxElev = 0;
        signed short maxOmni = 0;
        int maxpos = -1;
        for (i=0; i<pCommon->NumberOfTraceItems; i++)
        {
            if (! (pCommon->SelectorFlags & SWAP)) {
                pLevel[i] = ntohs(pLevel[i]);
                pAzi[i] = ntohs(pAzi[i]);
                pQual[i] = ntohs(pQual[i]);
                if (pElev != NULL)
                {
                    pElev[i] = ntohs(pElev[i]);
                }
                if (pOmni != NULL)
                {
                    pOmni[i] = ntohs(pOmni[i]);
                }
            }
            if ((maxLevel < pLevel[i]) && (pLevel[i] < 2000) && (pLevel[i] > -2000))
            {
                maxLevel = pLevel[i];
                maxAzi = pAzi[i];
                maxQual = pQual[i];
                if (pElev != NULL)
                {
                    maxElev = pElev[i];
                }
                if (pOmni != NULL)
                {
                    maxOmni = pOmni[i];
                }
                maxpos = i;
            }
        }

        if (pCommon->OptionalHeaderLength >= offsetof(OptHeaderDFPan, GPSHeader))
        {
            maxpos = pOptHeader->DemodFreqChannel;
            maxLevel = pLevel[maxpos];
            maxAzi = pAzi[maxpos];
            maxQual = pQual[maxpos];
            if (pElev != NULL)
            {
                maxElev = pElev[maxpos];
            }
            if (pOmni != NULL)
            {
                maxOmni = pOmni[maxpos];
            }
        }


        if (maxpos >= 0)
        {
            double lfLevel, lfAzi, lfQual, lfElev, lfDFFS, lfOmni;
            if (bNoDemodFreq)
            {
                Freq = Freq + ((int)pOptHeader->StepWidth) * (maxpos - pCommon->NumberOfTraceItems/2);
            }
            else
            {
                Freq = DemodFreq;
            }
            lfLevel = maxLevel / 10.0;
            lfAzi = maxAzi/10.0;
            lfQual = maxQual / 10.0;
            lfElev = maxElev / 10.0;
            lfOmni = maxOmni / 10.0;

            maxDFFS = (short)(pOptHeader->AntennaFactor);
            if (maxDFFS < 0x7FFE)
            {
              maxDFFS+=  maxLevel;
            }
            //char(248);
            lfDFFS = maxDFFS / 10.0;
            printf("DFPan F:%I64uMHz LEV:%4.1lfdBuV AZI:%4.1lf%c ELEV:%3.1lf%c DFQ:%4.1lf%% DFFSTR:%4.1lfdBuV/m OMN:%3.1lf%c \n", Freq, lfLevel, lfAzi, char(248), lfElev, char(248), lfQual, lfDFFS, lfOmni, char(248));
            if (m_bCSRecording)
            {
                fprintf (m_pCSFile,"%I64u;",Freq);
                fprint_short_formated(maxLevel);
                fprint_short_formated(maxAzi); 
                fprint_short_formated(maxQual);
                fprint_short_formated(maxDFFS);
                fprint_short_formated(maxElev);
                fprint_short_formated(maxOmni);
            }
        }
    }
    m_nDFPanPackets++;
}

/* FUNCTION ********************************************************************/
void    CEB200UdpSock::PHD(struct GenericAttribute *pAttr)
/*
SPECIFICATION:
Here are all actions coded which should be done in case of PHD data. This
member function can be overwritten in derived class to change default behaviour.
PARAMETERS:
struct GenericAttribute *pAttr : Pointer to generic attriubute of type PHD
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    struct CommonHeader *pCommon = (struct CommonHeader*)(pAttr->data);
    struct OptHeaderPIFPan *pOptHeader;

    ParseCommonHeader(pCommon);
    if (pCommon->SelectorFlags & OPTHEADER )
    {
        pOptHeader = (struct OptHeaderPIFPan *)(((unsigned char *)pCommon)+sizeof(*pCommon));
        if (! (pCommon->SelectorFlags & SWAP) )
        {
            // change byte order of optional header
            pOptHeader->Freq_low = ntohl(pOptHeader->Freq_low);
            pOptHeader->Freq_high = ntohl(pOptHeader->Freq_high);
            pOptHeader->FreqSpan = ntohl(pOptHeader->FreqSpan);
            pOptHeader->nHeightLevel = ntohl(pOptHeader->nHeightLevel);
            pOptHeader->nHeight = ntohl(pOptHeader->nHeight);
            pOptHeader->nWidth = ntohl(pOptHeader->nWidth);
            pOptHeader->nMaxY = ntohl(pOptHeader->nMaxY);
            pOptHeader->nMinY = ntohl(pOptHeader->nMinY);
            pOptHeader->nStartY = ntohl(pOptHeader->nStartY);
            pOptHeader->nStopY = ntohl(pOptHeader->nStopY);
            pOptHeader->nPictureNumber = ntohl(pOptHeader->nPictureNumber);
            if (m_bTimestamp)
            {
                pOptHeader->OutputTimestamp = Swap64(pOptHeader->OutputTimestamp);
            }

            if (m_bNewGenericAtt)
            {
                pOptHeader->nMode = ntohl(pOptHeader->nMode);
                Vprintf("PHD Mode = %u\n", pOptHeader->nMode);
            }


        }
        static DWORD nPictureNumber = 0;
        if (nPictureNumber != pOptHeader->nPictureNumber)
        {
            Vprintf("New Picture [%u]!\n", pOptHeader->nPictureNumber);
            nPictureNumber = pOptHeader->nPictureNumber;
        }
        Vprintf("PIFPan: Pixel = %6d, HxB = %d x %d, nMinY = %3d, nMaxY = %3d, nStartY = %3d, nStopY = %3d\n",
               pCommon->NumberOfTraceItems, pOptHeader->nHeight, pOptHeader->nWidth, pOptHeader->nMinY, pOptHeader->nMaxY,
               pOptHeader->nStartY, pOptHeader->nStopY);
        if (m_bTimestamp)
        {
            Vprintf("Output time : %I64u\n", pOptHeader->OutputTimestamp);
        }

    }


    m_nPHDPackets++;
}

/* FUNCTION ********************************************************************/
void    CEB200UdpSock::VideoPan(struct GenericAttribute *pAttr)
/*
SPECIFICATION:
Here are all actions coded which should be done in case of VIDEOPAN data. This
member function can be overwritten in derived class to change default behaviour.
PARAMETERS:
struct GenericAttribute *pAttr : Pointer to generic attriubute of type VIDEOPAN
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    struct CommonHeader *pCommon = (struct CommonHeader*)(pAttr->data);
    struct OptHeaderIFPan *pOptHeader;
    unsigned short *pLevel;

    ParseCommonHeader(pCommon);
    if (pCommon->SelectorFlags & OPTHEADER )
    {
        pOptHeader = (struct OptHeaderIFPan *)(((unsigned char *)pCommon)+sizeof(*pCommon));
        if (! (pCommon->SelectorFlags & SWAP) )
        {
            // change byte order of optional header
            pOptHeader->Freq_low = ntohl(pOptHeader->Freq_low);
            pOptHeader->FSpan = ntohl(pOptHeader->FSpan);
            pOptHeader->AvgTime = ntohs(pOptHeader->AvgTime);
            pOptHeader->AvgType = ntohs(pOptHeader->AvgType);
            pOptHeader->MeasureTime = ntohl(pOptHeader->MeasureTime);
            if (m_b64BitFrequencies)
            {
                pOptHeader->Freq_high = ntohl(pOptHeader->Freq_high);
            }
            if (pCommon->OptionalHeaderLength >= offsetof(OptHeaderIFPan, OutputTimestamp))
            {
                // change byte order of new elements
                pOptHeader->DemodFreqChannel = ntohl(pOptHeader->DemodFreqChannel);
                pOptHeader->DemodFreq_low = ntohl(pOptHeader->DemodFreq_low);
                pOptHeader->DemodFreq_high = ntohl(pOptHeader->DemodFreq_high);
            }
            if (m_bTimestamp)
            {
                pOptHeader->OutputTimestamp = Swap64(pOptHeader->OutputTimestamp);
            }
            if (pCommon->OptionalHeaderLength > (offsetof(OptHeaderIFPan, OutputTimestamp) + sizeof(pOptHeader->OutputTimestamp)))
            {

                Vprintf("NumberOfTraceItems: %d\n", pCommon->NumberOfTraceItems);
                pOptHeader->FStepNumerator   = ntohl(pOptHeader->FStepNumerator);
                pOptHeader->FStepDenominator = ntohl(pOptHeader->FStepDenominator);
                Vprintf("FStep Numerator: %d   Denominator: %d   Value: %f Hz\n", 
                    (pOptHeader->FStepNumerator), (pOptHeader->FStepDenominator), 
                    ((float)pOptHeader->FStepNumerator / (float)pOptHeader->FStepDenominator));
            }
        }
        if (m_bTimestamp)
        {
            Vprintf("Output time : %I64u\n", pOptHeader->OutputTimestamp);
        }

        INT64 Freq = pOptHeader->Freq_high;
        Freq <<= 32;
        Freq |= pOptHeader->Freq_low;

        INT64 DemodFreq = pOptHeader->DemodFreq_high;
        DemodFreq <<= 32;
        DemodFreq |= pOptHeader->DemodFreq_low;

        Vprintf("VDPan   Freq: %I64u Hz Span: %d Hz DispVar: %d  MeasureTime: %d us  DemFreq: %I64u Hz\n", Freq, pOptHeader->FSpan, pOptHeader->AvgType, pOptHeader->MeasureTime, DemodFreq);
    }
    if (pCommon->SelectorFlags & LEVEL)
    {
        pLevel = (unsigned short*)(((unsigned char*)pCommon) + sizeof(struct CommonHeader) + pCommon->OptionalHeaderLength);
        // change byte order of IFPAN data
        int i;
        for (i=0; i<pCommon->NumberOfTraceItems; i++)
        {
            if (! (pCommon->SelectorFlags & SWAP)) {
                pLevel[i] = ntohs(pLevel[i]);
            }
//            Vprintf("VDP level: %d\n", pLevel[i]);
        }
    }
    m_nVideoPanPackets++;
}

/* FUNCTION ********************************************************************/
void    CEB200UdpSock::GPSCompass(struct GenericAttribute *pAttr)
/*
SPECIFICATION:
Here are all actions coded which should be done in case of FSCAN data. This
member function can be overwritten in derived class to change default behaviour.
PARAMETERS:
struct GenericAttribute *pAttr : Pointer to generic attribute of type FSCAN
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    struct CommonHeader *pCommon = (struct CommonHeader*)(pAttr->data);
    struct OptHeaderGPSCompass *pOptHeader;
    struct GPSCompassSample *pGPS_Sample;


    ParseCommonHeader(pCommon);


    if (pCommon->SelectorFlags & OPTHEADER )
    {
        pOptHeader = (struct OptHeaderGPSCompass *)(((unsigned char *)pCommon)+ sizeof(*pCommon));
        pGPS_Sample = (struct GPSCompassSample *)(((unsigned char *) pCommon)+ sizeof(*pCommon) + sizeof(*pOptHeader));

        if (! (pCommon->SelectorFlags & SWAP) )
        {
            // change byte order of optional header


            if(m_bTimestamp)
            {
                pOptHeader->OutputTimestamp = Swap64(pOptHeader->OutputTimestamp);
            }



        }

        if (! (pCommon->SelectorFlags & SWAP) )
        {
            // change byte order of GPS sample data
            pGPS_Sample->Heading                    =   ntohs(pGPS_Sample->Heading);
            pGPS_Sample->HeadingType                =   ntohs(pGPS_Sample->HeadingType);
            pGPS_Sample->GPSHeader.bValid           =   ntohs(pGPS_Sample->GPSHeader.bValid);

            pGPS_Sample->GPSHeader.fLatMin          =   ntohf(pGPS_Sample->GPSHeader.fLatMin);
            pGPS_Sample->GPSHeader.fLonMin          =   ntohf(pGPS_Sample->GPSHeader.fLonMin);
            pGPS_Sample->GPSHeader.fPdop            =   ntohf(pGPS_Sample->GPSHeader.fPdop);

            pGPS_Sample->GPSHeader.iLatDeg          =   ntohs(pGPS_Sample->GPSHeader.iLatDeg);
            pGPS_Sample->GPSHeader.iLatRef          =   ntohs(pGPS_Sample->GPSHeader.iLatRef);

            pGPS_Sample->GPSHeader.iLonDeg          =   ntohs(pGPS_Sample->GPSHeader.iLonDeg);
            pGPS_Sample->GPSHeader.iLonRef          =   ntohs(pGPS_Sample->GPSHeader.iLonRef);
            pGPS_Sample->GPSHeader.iNoOfSatInView   =   ntohs(pGPS_Sample->GPSHeader.iNoOfSatInView);
            pGPS_Sample->AntennaValid               =   ntohs(pGPS_Sample->AntennaValid);
            pGPS_Sample->AntennaTiltOver            =   ntohs(pGPS_Sample->AntennaTiltOver);
            pGPS_Sample->AntennaElevation           =   ntohs(pGPS_Sample->AntennaElevation);
            pGPS_Sample->AntennaRoll                =   ntohs(pGPS_Sample->AntennaRoll);
        }

        {

            printf("\nPosition:  %c, %hd Degree, %-2.5f, %c, %hd Degree, %-2.5f\n",
                pGPS_Sample->GPSHeader.iLatRef,
                pGPS_Sample->GPSHeader.iLatDeg,
                pGPS_Sample->GPSHeader.fLatMin,
                pGPS_Sample->GPSHeader.iLonRef,
                pGPS_Sample->GPSHeader.iLonDeg,
                pGPS_Sample->GPSHeader.fLonMin);

            printf("Number Of Satellites in view: %i\n", pGPS_Sample->GPSHeader.iNoOfSatInView);
            printf("Mean Dilution Of Precision: %f\n\n", pGPS_Sample->GPSHeader.fPdop);

            printf("Heading: %i [Degree]\n", (pGPS_Sample->Heading)/10);
            printf("Heading Type: %i\n", pGPS_Sample->HeadingType);
            printf("GPS Valid: %i\n", pGPS_Sample->GPSHeader.bValid);

            printf("Antenna Valid: %i\n", pGPS_Sample->AntennaValid);
            printf("Antenna Tilt Over: %i\n", pGPS_Sample->AntennaTiltOver);
            printf("Antenna Elevation: %i\n", pGPS_Sample->AntennaElevation);
            printf("Antenna Roll: %i\n\n", pGPS_Sample->AntennaRoll);

        }

    }

}


/* FUNCTION ********************************************************************/
void    CEB200UdpSock::Selcall(struct GenericAttribute *pAttr)
/*
SPECIFICATION:
Here are all actions coded which should be done in case of SELCALL data. This
member function can be overwritten in derived class to change default behaviour.
PARAMETERS:
struct GenericAttribute *pAttr : Pointer to generic attriubute of type SELCALL
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    struct CommonHeader *pCommon = (struct CommonHeader*)(pAttr->data);
    struct OptHeaderSelcall *pOptHeader;
    long *pData;

    ParseCommonHeader(pCommon);
    if (pCommon->SelectorFlags & OPTHEADER)
    {
        pOptHeader = (struct OptHeaderSelcall *)(((unsigned char *)pCommon)+sizeof(*pCommon));
        if (! (pCommon->SelectorFlags & SWAP))
        {
            // change byte order of optional header
            pOptHeader->Freq_low = ntohl(pOptHeader->Freq_low);
            pOptHeader->Bandwidth = ntohl(pOptHeader->Bandwidth);
            pOptHeader->Demodulation = ntohs(pOptHeader->Demodulation);
            pOptHeader->SelcallMode = ntohs(pOptHeader->SelcallMode);
            if (m_b64BitFrequencies)
            {
                pOptHeader->Freq_high = ntohl(pOptHeader->Freq_high);
            }
            if (m_bTimestamp)
            {
                pOptHeader->OutputTimestamp = Swap64(pOptHeader->OutputTimestamp);
            }
        }
        printf("%s (%d) : ", pOptHeader->SelcallModeString, pOptHeader->SelcallMode);
    }
    int i;
    pData = (long*)(((unsigned char*)pCommon) + sizeof(struct CommonHeader) + pCommon->OptionalHeaderLength);
    for (i=0; i<pCommon->NumberOfTraceItems; i++)
    {
        if (! (pCommon->SelectorFlags & SWAP))
        {
            pData[i] = ntohl(pData[i]);
        }
        if (pOptHeader->SelcallMode != CTCSS)
        {
            if (pOptHeader->SelcallMode != DCS)
            {
                printf("%X", pData[i]);
            }
            else
            {
                /* DCS */
                printf("%03o", pData[i]);
            }
        }
        else
        {
            /* CTCSS */
            printf("%d.%01d", pData[0]/10, pData[0]%10);
        }
    }
    printf("\n");
    m_nSelcallPackets++;
}

/* FUNCTION ********************************************************************/
void    CEB200UdpSock::List(struct GenericAttribute *pAttr)
/*
SPECIFICATION:
Here are all actions coded which should be done in case of LISTF data. This
member function can be overwritten in derived class to change default behaviour.
PARAMETERS:
struct GenericAttribute *pAttr : Pointer to generic attribute of type LISTF
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    struct CommonHeader *pCommon = (struct CommonHeader*)(pAttr->data);
    unsigned short *pLevel;

    ParseCommonHeader(pCommon);
    if (pCommon->SelectorFlags & LEVEL )
    {
        pLevel = (unsigned short*)(((unsigned char*)pCommon) + sizeof(struct CommonHeader) + pCommon->OptionalHeaderLength);
        // change byte order of listf data
        int i, channel = 1;
        if (!ChannelsOnly())
        {
            puts("LIST:\n");
        }
        for (i=0; i<pCommon->NumberOfTraceItems; i++)
        {
            if (! (pCommon->SelectorFlags & SWAP)) {
                pLevel[i] = ntohs(pLevel[i]);
            }
            if (pLevel[i] != 2000)
            {
                // level
                printf("%d lev: %4hd   ", channel++, pLevel[i]);
            }
            else
            {
                // wrap around
                printf("\n");
                channel=1;
            }
        }
    }
    m_nListPackets++;
}

/* FUNCTION ********************************************************************/
void    CEB200UdpSock::FastLev(struct GenericAttribute *pAttr)
/*
SPECIFICATION:
Here are all actions coded which should be done in case of FASTL data. This
member function can be overwritten in derived class to change default behaviour.
PARAMETERS:
struct GenericAttribute *pAttr : Pointer to generic attribute of type FASTL
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    struct CommonHeader *pCommon = (struct CommonHeader*)(pAttr->data);
    signed short *pLevel;

    ParseCommonHeader(pCommon);
    if (pCommon->SelectorFlags & LEVEL )
    {
        pLevel = (signed short*)(((unsigned char*)pCommon) + sizeof(struct CommonHeader) + pCommon->OptionalHeaderLength);
        // change byte order of fast data
        int i;
        int nMinLev = +10000, nMaxLev = -10000;
        if (!ChannelsOnly())
        {
            puts("FAST:\n");
        }
        for (i=0; i<pCommon->NumberOfTraceItems; i++)
        {
            if (! (pCommon->SelectorFlags & SWAP)) {
                pLevel[i] = ntohs(pLevel[i]);
            }
            if (pCommon->NumberOfTraceItems < 2)
            {
                printf("%d level: %hd\n", i, pLevel[i]);
            }
            else
            {
                if (pLevel[i] > nMaxLev)
                {
                    nMaxLev = pLevel[i];
                }
                if (pLevel[i] < nMinLev)
                {
                    nMinLev = pLevel[i];
                }
            }
        }
        if (pCommon->NumberOfTraceItems >= 2)
        {
            printf("n = %d, MinLev = %4hd   MaxLev = %4hd\n", pCommon->NumberOfTraceItems, nMinLev, nMaxLev);
        }
    }
    m_nFastLevPackets++;
}


/* FUNCTION ********************************************************************/
void    CEB200UdpSock::Cw(struct GenericAttribute *pAttr)
/*
SPECIFICATION:
Here are all actions coded which should be done in case of CW data. This
member function can be overwritten in derived class to change default behaviour.
PARAMETERS:
struct GenericAttribute *pAttr : Pointer to generic attribute of type CW
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    struct CommonHeader *pCommon = (struct CommonHeader*)(pAttr->data);
    DWORDLONG Freq = 0;

    printf("CW:\n");
    Scan(pAttr);

    if (pCommon->SelectorFlags & OPTHEADER)
    {
        struct OptHeaderCw *pOptHeader = (struct OptHeaderCw *)(((unsigned char *)pCommon)+sizeof(*pCommon));
        if (! (pCommon->SelectorFlags & SWAP)) {
            Freq = ntohl(pOptHeader->Freq_low);
            if (m_b64BitFrequencies)
            {
                pOptHeader->Freq_high = ntohl(pOptHeader->Freq_high);
                Freq |= ((DWORDLONG)(pOptHeader->Freq_high)) << 32;
            }
            if (m_bTimestamp)
            {
                pOptHeader->OutputTimestamp = Swap64(pOptHeader->OutputTimestamp);
            }

        } else {
            Freq = pOptHeader->Freq_low;
            Freq |= ((DWORDLONG)(pOptHeader->Freq_high)) << 32;
        }
        printf("FREQ (from OptHeader): %I64u\n", Freq);
        if (m_bTimestamp)
        {
            Vprintf("Output time : %I64u\n", pOptHeader->OutputTimestamp);
        }
    }

    m_nCwPackets++;
}

/* FUNCTION ********************************************************************/
void CEB200UdpSock::PrintTimestamp(DWORDLONG ts)

/*
SPECIFICATION: Prints a timestamp
PARAMETERS:
DWORDLONG ts : timestamp in ns since 1.1.1970
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    time_t t = (time_t)(ts / 1000000000);
    time_t tns = (time_t)(ts % 1000000000);
    struct tm tms;
    struct tm *ptms;

    ptms = localtime(&t);
    if (ptms != NULL)
    {
        tms = *ptms;
        printf("%04d-%02d-%02d %02d:%02d:%02d %09d ns\n", tms.tm_year+1900, tms.tm_mon+1, tms.tm_mday, tms.tm_hour, tms.tm_min, tms.tm_sec, tns);
    }
}

/* FUNCTION ********************************************************************/
DWORDLONG CEB200UdpSock::CalcTimestamp(DWORDLONG starttime, DWORDLONG count, DWORD samplerate)

/*
SPECIFICATION: calculates the timestamp of a sample specified by starttime, samplecount and samplerate
PARAMETERS:
DWORDLONG starttime : time in ns since 1.1.1970 of very first sample
DWORDLONG count : samplecout of current sample
DWORD samplerate : samplerate in Hz
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    double lfTs = 1E9;
    lfTs /= (double)samplerate;
    starttime += (DWORDLONG)(0.5 + lfTs * (double)((__int64)count));
    return starttime;
}

/* FUNCTION ********************************************************************/
bool CEB200UdpSock::DoStatistics(int rcvlen)
/*
SPECIFICATION: Statistical update
PARAMETERS:
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES: true if statistical update has been run, false otherwise
EXCEPTIONS:
********************************************************************************/
{
    bool bOutput = false;

    // do some statistics
    unsigned long acttime;
    acttime = GetTickCount();
    m_nCount++;
    m_nTotalLen += rcvlen;
    if ((acttime - m_oldtime) >= 1000)
    {
        bOutput = true;
        if ( !ChannelsOnly() && !BytesOnly() )
        {
            printf("Number of DGrams/sec = %ld\n", m_nCount);
        }

        if ( !BytesOnly() )
        {
            printf("Number of Channels/sec = %ld [%s]\n", m_nChannels, m_cDDCx);
        }

        if ( !ChannelsOnly() )
        {
            printf("Number of Bytes/sec = %ld\n", m_nTotalLen);
        }


        if ( !ChannelsOnly() && !BytesOnly() )
        {

            //printf("Number of Bytes/sec = %ld\n", m_nTotalLen);
            printf("Number of FSCAN attributes / sec = %ld\n", m_nFScanPackets);
            printf("Number of MSCAN attributes / sec = %ld\n", m_nMScanPackets);
            if ( OptionIsDS() )
            {
                printf("Number of DSCAN attributes / sec = %ld\n", m_nDScanPackets);
            }
            if ( OptionIsPS() )
            {
                printf("Number of PSCAN attributes / sec = %ld\n", m_nPScanPackets);
            }
            printf("Number of IFPAN attributes / sec = %ld\n", m_nIFPanPackets);
            printf("Number of DFPAN attributes / sec = %ld\n", m_nDFPanPackets);
            printf("Number of PHD   attributes / sec = %ld\n", m_nPHDPackets);
            printf("Number of AUDIO attributes / sec = %ld\n", m_nAudioPackets);
            if ( OptionIsCM() )
            {
                printf("Number of FASTLEV attributes / sec = %ld\n", m_nFastLevPackets);
                printf("Number of LIST attributes / sec = %ld\n", m_nListPackets);
            }
            printf("Number of CW attributes / sec = %ld\n", m_nCwPackets);
            if ( EquipmentisEM050() )
            {
                printf("Number of IF attributes / sec = %ld\n", m_nIFPackets);
                if ( m_nIFPackets )
                {
                    printf("Start timestamp of last IF attribute: ");
                    PrintTimestamp(m_nIFStartTimestamp);
                    printf("Timestamp of next IF attribute: ");
                    DWORDLONG ts;
                    ts = CalcTimestamp(m_nIFStartTimestamp, m_nIFSampleCounter, (DWORD)m_nIFSampleRate);
                    PrintTimestamp(ts);


                    printf("RMS level of last IF attribute = %.1lf dBFS", m_lfRms);
                    if (m_flags & IF_DATA_SIGNAL_VALID)
                    {
                        printf(", RMS ant. level = %.1lf dBuV", m_lfRms + m_nRxAtt);
                    }
                    if (m_flags & IF_DATA_ANT_FACTOR_VALID)
                    {
                        double lfFieldStrength = m_kFactor;
                        lfFieldStrength /= 10.0;
                        lfFieldStrength += m_lfRms + m_nRxAtt;
                        printf(" = %.1lf dBuV/m", lfFieldStrength);
                    }
                    printf("\n");
                }
                printf("Number of VIDEO attributes / sec = %ld\n", m_nVideoPackets);
                printf("Number of VIDEOPAN attributes / sec = %ld\n", m_nVideoPanPackets);
                printf("Number of SELCALL attributes / sec = %ld\n", m_nSelcallPackets);
            }
        }
        //        printf("Last Demodulation: %s\n", m_sDemod);
        //fflush(stdout);
        m_oldtime = acttime;
        m_nCount = 0;
        m_nChannels = 0;
        m_nTotalLen = 0;
        m_nFScanPackets = 0;
        m_nMScanPackets = 0;
        m_nDScanPackets = 0;
        m_nIFPanPackets = 0;
        m_nDFPanPackets = 0;
        m_nAudioPackets = 0;
        m_nFastLevPackets = 0;
        m_nListPackets = 0;
        m_nCwPackets = 0;
        m_nIFPackets = 0;
        m_nVideoPackets = 0;
        m_nPScanPackets = 0;
        m_nVideoPanPackets = 0;
        m_nSelcallPackets = 0;
        m_nPHDPackets = 0;
    }

    return bOutput;
}

/* FUNCTION ********************************************************************/
void CEB200UdpSock::SetIFRecording(char *pcFilename, bool bRaw, DWORDLONG dwlRecordingTime)
/*
SPECIFICATION:
PARAMETERS:
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    if (!_stricmp(pcFilename, "TS"))
    {
        // automatically generate filename out of timestamp, but only in non raw mode
        if (!bRaw)
        {
            pcFilename = NULL;
        }
    }

    m_pctIQFile->SetFilename(pcFilename, dwlRecordingTime);
    m_bIFRecording = true;
    m_bRaw = bRaw;
    if (m_bRaw)
    {
        m_pRawFile = fopen(pcFilename, "wb");
    }
}

/* FUNCTION ********************************************************************/
void CEB200UdpSock::SetCSRecording(char *pcFilename)
/*
SPECIFICATION:
PARAMETERS:
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    m_bCSRecording = true;

    m_pCSFile = fopen(pcFilename, "w");
}

/* FUNCTION ********************************************************************/
void CEB200UdpSock::SetAFRecording(char *pcFilename, unsigned __int64 waveFileTime)
/*
SPECIFICATION:
PARAMETERS:
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    m_pctIQFile->SetFilename(pcFilename, waveFileTime);
	m_waveFileTime = waveFileTime;

	m_bAFRecording = true;
    m_bRaw = false;
}

/* FUNCTION ********************************************************************/
unsigned int CEB200UdpSock::EB200UdpThreadProc(void *pParam)
/*
SPECIFICATION:
Thread procedure wrapper of UDP socket class.
PARAMETERS:
LPVOID *pParam : Pointer to instance
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    CEB200UdpSock *pThis = (CEB200UdpSock*)(pParam);
    // switch to object scope
    return (pThis->doThreadProc());
}

/* FUNCTION ********************************************************************/
unsigned int CEB200UdpSock::doThreadProc(void)
/*
SPECIFICATION:
Thread procedure of UDP socket class.
PARAMETERS:
LPVOID *pParam : Pointer to instance
PRECONDITIONS:
SIDE_EFFECTS:
RETURN_VALUES:
EXCEPTIONS:
********************************************************************************/
{
    int err = NO_ERROR;
    int addrlen = sizeof (m_Addr);
    int rcvlen = 1;
    int curlen, reqlen, datlen;

	time(&m_RecordingTime);
	time_t temp2 = (time_t)m_waveFileTime;

    // alloc some space for receiving datagrams
    char *pData = new char[0x100000];
    EB200_DATAGRAM_TYPE *pDGram = (EB200_DATAGRAM_TYPE *)pData;

    // loop until we receive 0 bytes or get an error
    while (rcvlen > 0)
    {
		//check recording time, if grater than waveFile time then stop thread
		if (m_waveFileTime > 0)
		{
			// check if recording limit is reached
			time_t temp;
			time(&temp);
			
			if ((temp - m_RecordingTime) > temp2)
			{
				// stop recording
				CloseTrace();
			}
		}


        // receiving is done here
        bool bIsEB200DGram;
        bIsEB200DGram = false;
        if ( !m_bTcp )
        {
            rcvlen = recvfrom(m_Sock, (char*)pData, 0x10000, 0, (struct sockaddr *)&m_Addr, &addrlen);
            if (rcvlen == SOCKET_ERROR)
            {
                err = WSAGetLastError();
                continue;
            }
            else datlen = rcvlen;
        }
        else
        {
            reqlen = EB200_DATAGRAM_HEADER_SIZE;
            curlen = 0;
            do
            {
                rcvlen = recv(m_Sock, &pData[curlen], reqlen-curlen, 0);
                if ( rcvlen > 0 )
                {
                    curlen += rcvlen;
                }
            } while ( ( rcvlen > 0 ) && (curlen < reqlen) );

            // verify EB200/ESMB datagram of version 2.?? and retrieve (remaining) data size
            if ((rcvlen <= 0) || !CheckDGram(pDGram, curlen))
            {
                if (rcvlen <= 0)
                {
                    printf("recv ERROR [rcvlen = %d - errno = %d] !\n", rcvlen, WSAGetLastError());
                    continue;
                }
                else
                {
                //  printf("CheckDGram failed [curlen = %d / rcvlen = %d] -> continue...\n", curlen, rcvlen);
                    // call the callback function to read an unknown data packet
                    datlen = GetUnknownPacketLen(m_Sock, pData, &curlen);
                    if (datlen <= 0)
                    {
                        continue;
                    }
                }
            }
            else
            {
                datlen = pDGram->DataSize;
                bIsEB200DGram = true;
            }
            reqlen = datlen;

            do
            {
                rcvlen = recv(m_Sock, &pData[curlen], reqlen-curlen, 0);
                if ( rcvlen > 0 )
                {
                    curlen += rcvlen;
                }
            } while ( ( rcvlen > 0 ) && (curlen < reqlen) );

            // just for DoStatistics()
            rcvlen = datlen;
        }

#ifdef _STATISTICS
        if (m_bEnableStatistics)
        {
            DoStatistics(datlen);
        }
#endif
        if ((err != NO_ERROR) || IsStopped() || _kbhit())
        {
            // someone outside has told us to stop our thread
            break;
        }
        else
        {
            if (datlen > 0)
            {
                // Is it a valid EB200/ESMB datagram of version 2.??
                // Note: This is already done for TCP

                if (bIsEB200DGram || CheckDGram(pDGram, datlen))
                {
                    // yes it is ! Parse and dispatch all generic attributes
                    ParseData(pDGram, datlen);
                }
                else
                {
                    // it's an unknown data packet
                    ParseOtherData(pData, datlen);
                }
            }
        }
    }
    delete pData;

    CloseFiles();
    CloseTrace();
    return (0);
}

/* UDP AUDIO CLASS *************************************************************/

/* GLOBAL FUNCTIONS DEFINITION *************************************************/

