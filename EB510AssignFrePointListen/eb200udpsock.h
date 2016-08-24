/*******************************************************************************
********************************************************************************
** COPYRIGHT:      (c) 1995-2004 Rohde & Schwarz, Munich
** MODULE:         EB200UdpSock.H
** ABBREVIATION:
** COMPILER:       VC++ 6.0
** LANGUAGE:       C/C++
** AUTHOR:         Martin Hisch
** ABSTRACT:
** PREMISES:
** REMARKS:
** HISTORY:
**  2002-6-3: (Mu)  newStepScheme in OptHeaderDScan with VersionMinor >= 0x24
**  2002-3-4: (Mu)  Channels / sec ,m_nChannels
**  2000-11-30:(ks) Include Not-Swapping.
**  2000-5-29: (Hh) Creation
** REVIEW:
********************************************************************************/

/* INCLUDE FILES ***************************************************************/

/* IMPORT */
#include "CmdSock.h"

/* EXPORT */

/* LOCALL DEFINES ********************************************************/
#define ASSERT  assert
// some statistics ???
#define _STATISTICS

/* GLOBAL DEFINES ********************************************************/
#ifndef EB200UdpSock_DEF
#define EB200UdpSock_DEF

#define TAG_NEW_GENERICATT_LIMIT    5000

enum TAGS
{
    FSCAN           = 101,
    MSCAN           = 201,
    DSCAN           = 301,
    AUDIO           = 401,
    IFPAN           = 501,
    FASTL           = 601,
    LISTF           = 701,
    CW              = 801,
    IF              = 901,
    VIDEO           = 1001,
    VIDEOPAN        = 1101,
    PSCAN           = 1201,
    SELCALL         = 1301,
    DFPAN           = 1401,
    PHD             = 1601,
    GPSCompass      = 1801,
    LAST_TAG
};

enum FLAGS
{
    LEVEL           = 0x1,
    OFFSET          = 0x2,
    FSTRENGTH       = 0x4,
    AM              = 0x8,
    AMPOS           = 0x10,
    AMNEG           = 0x20,
    FM              = 0x40,
    FMPOS           = 0x80,
    FMNEG           = 0x100,
    PM              = 0x200,
    BAND            = 0x400,
    DF_LEVEL        = 0x800,
    AZIMUTH         = 0x1000,
    DF_QUALITY      = 0x2000,
    DF_FSTRENGTH    = 0x4000,
    CHANNEL         = 0x00010000,
    FREQ_LOW        = 0x00020000,
    ELEVATION       = 0x00040000,
    OMNIPHASE       = 0x00100000,
    FREQ_HIGH       = 0x00200000,
    SWAP            = 0x20000000, // swap ON means: do NOT swap (for little endian machines)
    SIGGTSQU        = 0x40000000,
    OPTHEADER       = 0x80000000
};


#pragma pack(push, 1)
typedef struct GenericAttribute
{
    unsigned short      tag;
    unsigned short      length;
    unsigned char       data[1];
} GENERIC_ATTRIBUTE_TYPE;

typedef struct NewGenericAttribute
{
    unsigned short      tag;
    unsigned short      reserved1;
    unsigned long       length;
    unsigned long       reserved2[4];
    unsigned char       data[1];
} NEW_GENERIC_ATTRIBUTE_TYPE;

typedef struct EB200DatagramFormat
{
    unsigned int        MagicNumber;
    unsigned short      VersionMinor;
    unsigned short      VersionMajor;
    unsigned short      SequenceNumber;
    unsigned short      reserved;
    unsigned long       DataSize; // sizeof complete datagram incl. header (32 bit aligned)
    struct GenericAttribute Attribute[1];
} EB200_DATAGRAM_TYPE;

#define EB200_DATAGRAM_HEADER_SIZE  (sizeof( EB200_DATAGRAM_TYPE ) - sizeof( GENERIC_ATTRIBUTE_TYPE ))

typedef struct CommonHeader
{
    unsigned short      NumberOfTraceItems;
    unsigned char       ChannelNumber; /* 0 = main RX; 0x77 is also main RX (old value); 1 = DDC1; 2 = DDC2; ... */
    unsigned char       OptionalHeaderLength;
    unsigned long       SelectorFlags;
} COMMON_HEADER_TYPE;

typedef struct NewCommonHeader
{
    unsigned long       NumberOfTraceItems;
    unsigned long       ChannelNumber; /* 0 = main RX; 0x77 is also main RX (old value); 1 = DDC1; 2 = DDC2; ... */
    unsigned long       OptionalHeaderLength;
    unsigned long       SelectorFlagsLow;
    unsigned long       SelectorFlagsHigh;
    unsigned long       reserved[4];
} NEW_COMMON_HEADER_TYPE;

/* Optional Headers */
typedef struct OptHeaderDScan
{
    unsigned long       StartFreq_low;
    unsigned long       StopFreq_low;
    unsigned long       StepFreq;
    unsigned long       FMark_low;
    unsigned short      BWZoom;
    short               RefLevel;
    unsigned short      newStepScheme;
    unsigned long       StartFreq_high;
    unsigned long       StopFreq_high;
    unsigned long       FMark_high;
} OPT_HEADER_DSCAN_TYPE;

typedef struct OptHeaderPScan
{
    unsigned long       StartFreq_low;
    unsigned long       StopFreq_low;
    unsigned long       StepFreq;
    unsigned long       StartFreq_high;
    unsigned long       StopFreq_high;
    char                reserved[4];
    DWORDLONG           OutputTimestamp; /* nanoseconds since Jan 1st, 1970, without leap seconds */
    unsigned long       FStepNumerator;
    unsigned long       FStepDenominator;
    DWORDLONG           FreqOfFirstStep;
} OPT_HEADER_PSCAN_TYPE;

typedef struct OptHeaderFScan
{
    short               CycleCount;
    short               HoldTime;
    short               DwellTime;
    short               DirectionUp;
    short               StopSignal;
    unsigned long       StartFreq_low;
    unsigned long       StopFreq_low;
    unsigned long       StepFreq;
    unsigned long       StartFreq_high;
    unsigned long       StopFreq_high;
    char                reserved[2];
    DWORDLONG           OutputTimestamp; /* nanoseconds since Jan 1st, 1970, without leap seconds */
} OPT_HEADER_FSCAN_TYPE;

typedef struct OptHeaderMScan
{
    short               CycleCount;
    short               HoldTime;
    short               DwellTime;
    short               DirectionUp;
    short               StopSignal;
    char                reserved[6];
    DWORDLONG           OutputTimestamp; /* nanoseconds since Jan 1st, 1970, without leap seconds */
} OPT_HEADER_MSCAN_TYPE;

typedef struct OptHeaderIFPan
{
    unsigned long       Freq_low;
    unsigned long       FSpan;
    short               AvgTime;
    short               AvgType;
    unsigned long       MeasureTime;
    unsigned long       Freq_high;
    signed long         DemodFreqChannel;
    unsigned long       DemodFreq_low;
    unsigned long       DemodFreq_high;
    DWORDLONG           OutputTimestamp; /* nanoseconds since Jan 1st, 1970, without leap seconds */
    unsigned long       FStepNumerator;
    unsigned long       FStepDenominator;
} OPT_HEADER_IFPAN_TYPE;

typedef struct OptHeaderAudio
{
    unsigned short      AudioMode;
    unsigned short      FrameLength;
    unsigned long       Freq_low;
    unsigned long       Bandwidth;
    unsigned short      Demodulation;
    char                DemodulationString[8];
    unsigned long       Freq_high;
    char                reserved[6];
    DWORDLONG           OutputTimestamp; /* nanoseconds since Jan 1st, 1970, without leap seconds */
} OPT_HEADER_AUDIO_TYPE;

// Flagdefinition for IF data via LAN
#define IF_DATA_SIGNAL_VALID        (0x01)
#define IF_DATA_BLANKING            (0x02)
#define IF_DATA_ANT_FACTOR_VALID    (0x8000)

typedef struct OptHeaderIF
{
    unsigned short      IFMode;
    unsigned short      FrameLength;
    unsigned long       SamplerRate;
    unsigned long       Freq_low;
    unsigned long       Bandwidth;
    unsigned short      Demodulation;
    short               RxAttenuation;
    unsigned short      Flags;
    short               kFactor;
    char                DemodulationString[8];
    DWORDLONG           SampleCount;
    unsigned long       Freq_high;
    char                reserved2[4];
    DWORDLONG           StartTimestamp; /* nanoseconds since Jan 1st, 1970, without leap seconds */
} OPT_HEADER_IF_TYPE;

typedef OPT_HEADER_IF_TYPE OPT_HEADER_VIDEO_TYPE;

typedef struct OptHeaderCw
{
    unsigned long       Freq_low;
    unsigned long       Freq_high;
    DWORDLONG           OutputTimestamp; /* nanoseconds since Jan 1st, 1970, without leap seconds */
} OPT_HEADER_CW_TYPE;

typedef struct OptHeaderSelcall
{
    unsigned long       Freq_low;
    unsigned long       Bandwidth;
    unsigned short      Demodulation;
    char                DemodulationString[8];
    unsigned short      SelcallMode;
    char                SelcallModeString[10];
    unsigned long       Freq_high;
    char                reserved[6];
    DWORDLONG           OutputTimestamp; /* nanoseconds since Jan 1st, 1970, without leap seconds */
} OPT_HEADER_SELCALL_TYPE;

typedef struct GPSHeader
{
    signed short        bValid;         /* denotes whether GPS data are to be considered valid  */
    signed short        iNoOfSatInView; /* number of satellites in view 0-12; only valid, if GGA msg is received, else -1 (GPS_UNDEFINDED) */
    signed short        iLatRef;        /* latitude direction ('N' or 'S')                      */
    signed short        iLatDeg;        /* latitude degrees                                     */
    float               fLatMin;        /* geographical latitude: minutes                       */
    signed short        iLonRef;        /* longitude direction ('E' or 'W')                     */
    signed short        iLonDeg;        /* longitude degrees                                    */
    float               fLonMin;        /* geographical longitude: minutes                      */
    float               fPdop;          /* Mean (Position) Dilution Of Precision;               */
} GPSHEADER_TYPE;

typedef struct GPSCompassSample
{
    unsigned short      Heading;
    short               HeadingType;
    GPSHEADER_TYPE      GPSHeader;
    short               AntennaValid;
    short               AntennaTiltOver;
    short               AntennaElevation;
    short               AntennaRoll;
} GPS_SAMPLE_TYPE;

#define MAX_ANTVOLT_COUNT   9

typedef struct
{
    float x;
    float y;
} CPLX_TYPE;

typedef struct OptHeaderDFPan
{
    unsigned long       Freq_low;
    unsigned long       Freq_high;
    unsigned long       FreqSpan;
    signed long         DFThresholdMode;
    signed long         DFThresholdValue;
    unsigned long       DfBandwidth;
    unsigned long       StepWidth;
    unsigned long       DFMeasureTime;
    signed long         DFOption;
    unsigned short      CompassHeading;
    signed short        CompassHeadingType;
    signed long         AntennaFactor;
    signed long         DemodFreqChannel;
    unsigned long       DemodFreq_low;
    unsigned long       DemodFreq_high;
    DWORDLONG           OutputTimestamp; /* nanoseconds since Jan 1st, 1970, without leap seconds */
    GPSHEADER_TYPE      GPSHeader;
    unsigned long       FStepNumerator;
    unsigned long       FStepDenominator;
    DWORDLONG           DFBandwidthHighRes;
} OPT_HEADER_DFPAN_TYPE;

typedef struct OptHeaderGPSCompass
{
    DWORDLONG           OutputTimestamp; /* nanoseconds since Jan 1st, 1970, without leap seconds */
} OPT_HEADER_GPS_TYPE;

typedef struct OptHeaderPIFPan
{
    unsigned int        Freq_low;
    unsigned int        Freq_high;
    unsigned int        FreqSpan;
    int                 nReferenceLevel;    /* 1/10 dBuV */
    int                 nHeightLevel;       /* 1/10 dB */
    int                 nHeight;            /* Pixel */
    int                 nWidth;             /* Pixel */
    int                 nMaxY;              /* Y-Koordinate */
    int                 nMinY;              /* Y-Koordinate */
    int                 nStartY;            /* Y-Koordinate */
    int                 nStopY;             /* Y-Koordinate */
    unsigned int        nPictureNumber;     /* No of current picture */
    DWORDLONG           OutputTimestamp;    /* nanoseconds since Jan 1st, 1970, without leap seconds */
    int                 nMode;              /* PIFPAN mode (histogram or pulse) */
} OPT_HEADER_PIFPAN_TYPE;

#pragma pack(pop)

/* dig. AF:  Audioformat description*/
typedef struct
{
    unsigned long       nSampleRate;
    unsigned short      nBitsPerSample;
    unsigned short      nNoOfChannels;
    unsigned long       nRateInBytesPerSec;
    unsigned char       cFrameLength;
} AUDIO_FORMAT_TYPE;

class CEB200UdpSock;

typedef void (CEB200UdpSock::*PMEMBER)(struct GenericAttribute *);

typedef struct TagList
{
    enum TAGS   nTag;
    PMEMBER     pMethode;
} TAGLIST_TYPE;

#include "sound.h"
#include "wavfile.h"

#define  PCM_MAX_AUDIO  12
#define  GSM_AUDIO  13

class CEB200UdpSock
{
public:
    CEB200UdpSock( bool bTcp, unsigned short TracPort, unsigned long ulRemoteAddress, unsigned short RecvPort = 0 );
    virtual ~CEB200UdpSock();

    virtual void    Init(bool bUseWin32Thread = false);
    virtual void    WaitOnThread(void);
    virtual void    SetSCPICmdSocket(CCmdSock *pCmdSock) { m_pCmdSock = pCmdSock; };
    virtual void    CloseTrace(void);
            BOOL    CheckDGram(struct EB200DatagramFormat *pDGram, unsigned long nLen);
    virtual void    ParseData(struct EB200DatagramFormat *pDGram, int nLen);
    virtual void    ParseOtherData(void *pData, int nLen) {};
    virtual int     GetUnknownPacketLen(SOCKET nSock, void *pData, int *pLen) { return 0; } ;

    virtual void    Scan(struct GenericAttribute *pAttr);
    virtual void    FScan(struct GenericAttribute *pAttr);
    virtual void    MScan(struct GenericAttribute *pAttr);
    virtual void    DScan(struct GenericAttribute *pAttr);
    virtual void    IFPan(struct GenericAttribute *pAttr);
    virtual void    DFPan(struct GenericAttribute *pAttr);
    virtual void    Audio(struct GenericAttribute *pAttr);
    virtual void    IF(struct GenericAttribute *pAttr);
    virtual void    Video(struct GenericAttribute *pAttr);
    virtual void    List(struct GenericAttribute *pAttr);
    virtual void    FastLev(struct GenericAttribute *pAttr);
    virtual void    Cw(struct GenericAttribute *pAttr);
    virtual void    PScan(struct GenericAttribute *pAttr);
    virtual void    VideoPan(struct GenericAttribute *pAttr);
    virtual void    Selcall(struct GenericAttribute *pAttr);
    virtual void    PHD(struct GenericAttribute *pAttr);
    virtual void    GPSCompass(struct GenericAttribute *pAttr);

            int     GetTracPort() { return m_nPort; };
            SOCKET  GetTracSock() { return m_Sock; };
            bool    IsStopped() { return m_bStop; };

    virtual bool    DoStatistics(int);

            void    SetWavFileObject(CWAVFile *pctIQFile) { m_pctIQFile = pctIQFile; };
            CWAVFile *GetWavFileObject(void) { return m_pctIQFile; };

    virtual void    SetIFRecording(char *pcFilename, bool bRaw, DWORDLONG dwlRecordingTime);
    virtual void    SetCSRecording(char *pcFilename);
    virtual void    SetAFRecording(char *pcFilename, unsigned __int64 waveFileTime = 0);

protected:
            void    fprint_short_formated(short pData);
            void    PrintTimestamp(DWORDLONG ts);
            DWORDLONG CalcTimestamp(DWORDLONG starttime, DWORDLONG count, DWORD samplerate);
            int     Vprintf(const char *format, ...);
    static  float   ntohf( float netfloat);
    static  unsigned int EB200UdpThreadProc(void *pParam);
            unsigned int doThreadProc(void);

            void    ParseSelectorFlags(unsigned long nSel, void *pData, int nCount);
            void    ParseCommonHeader( struct CommonHeader *pCommon );
            void    ParseCommonHeader( struct NewCommonHeader *pCommon );

    virtual void    CloseFiles(void)
                    {
                        if ((m_pctIQFile != NULL) && m_pctIQFile->IsOpen())
                        {
                            m_pctIQFile->Close();
                        }

                        if (m_pRawFile != NULL)       
                        {
                            fclose(m_pRawFile);
                            m_pRawFile = NULL;
                        }
                    };
            bool    ChannelsOnly(void)
                    {
                        if (m_pCmdSock != NULL)    return m_pCmdSock->ChannelsOnly();
                        else                       return false;
                    };
            bool    BytesOnly(void)
                    {
                        if (m_pCmdSock != NULL)    return m_pCmdSock->BytesOnly();
                        else                       return false;
                    };
            bool    OptionIsDS(void)
                    {
                        if (m_pCmdSock != NULL)    return m_pCmdSock->OptionIsDS();
                        else                       return false;
                    };
            bool    OptionIsPS(void)
                    {
                        if (m_pCmdSock != NULL)    return m_pCmdSock->OptionIsPS();
                        else                       return false;
                    };
            bool    OptionIsCM(void)
                    {
                        if (m_pCmdSock != NULL)    return m_pCmdSock->OptionIsCM();
                        else                       return false;
                    };
            bool    EquipmentisEM050(void)
                    {
                        if (m_pCmdSock != NULL)    return m_pCmdSock->EquipmentisEM050();
                        else                       return true; // fuer udpammosexample
                    };
private:
            CEB200UdpSock();    // hide default ctor since we require args

protected:
            void    SwapBuffer(short *pData, int nCount)
                    {
                        for (int ii = 0; ii < nCount; ii++)
                            pData[ii] = ntohs(pData[ii]);
                    }

            void    SwapBuffer(long *pData, int nCount)
                    {
                        for (int ii = 0; ii < nCount; ii++)
                            pData[ii] = ntohl(pData[ii]);
                    }

            DWORDLONG Swap64(DWORDLONG data)
                               {
                                   DWORD *pLong = (DWORD*)&data;
                                   DWORD nLong0, nLong1;
                                   nLong0 = pLong[0];
                                   nLong1 = pLong[1];
                                   pLong[0] = ntohl(nLong1);
                                   pLong[1] = ntohl(nLong0);
                                   return data;
                               }

protected: // Members
    static const AUDIO_FORMAT_TYPE m_AudioFormats[GSM_AUDIO+1];

    unsigned short          m_nSequenceNumber;  // sequence counter
           bool             m_bEnableStatistics; // flag for enabling statistics
           bool             m_bTcp;             // flag for using TCP
           int              m_nPort;            // trace port number
           int              m_nRecvPort;        // local port number (optional)
           SOCKET           m_Sock;             // trace socket descriptor
           struct sockaddr_in  m_Addr;
           bool             m_bStop;            // flag for closing
           CSound           m_Sound;            // Sound output class
           bool             m_bIFRecording;     // Indicates if IF recording is active
           bool             m_bCSRecording;     // Indicates if CS recording is active
           bool             m_bAFRecording;     // Indicates if AF recording is active
           bool             m_bRaw;             // Raw data recording
           CWAVFile        *m_pctIQFile;        // File object for IQ recording
           FILE            *m_pCSFile;          // File pointer to CS data file
           FILE            *m_pRawFile;         // File pointer to raw data file
           bool             m_b64BitFrequencies;
           bool             m_bTimestamp;       // timestamp and channel number available
           bool             m_bNewGenericAtt;   // new generic attribute available
           unsigned long    m_nDDCx;            // receiver instance / DDC number
           char             m_cDDCx[8];         // and it's name

           // Pointers to different data types
           short            *m_pLevel;
           long             *m_pOffset;
           short            *m_pFStrength;
           short            *m_pAM;
           short            *m_pAMPos;
           short            *m_pAMNeg;
           long             *m_pFM;
           long             *m_pFMPos;
           long             *m_pFMNeg;
           short            *m_pPM;
           long             *m_pBW;
           short            *m_pAzimuth;
           short            *m_pQuality;
           short            *m_pDFLevel;
           short            *m_pDFFStrength;
           short            *m_pElevation;
           short            *m_pOmniphase;
           unsigned short   *m_pChannel;
           unsigned long    *m_pFreq;
           unsigned long    *m_pFreqHigh;

           // some statistics
           int              m_nFScanPackets;
           int              m_nMScanPackets;
           int              m_nDScanPackets;
           int              m_nIFPanPackets;
           int              m_nAudioPackets;
           int              m_nIFPackets;
           int              m_nVideoPackets;
           int              m_nFastLevPackets;
           int              m_nListPackets;
           int              m_nCwPackets;
           int              m_nPScanPackets;
           int              m_nVideoPanPackets;
           int              m_nSelcallPackets;
           int              m_nDFPanPackets;
           int              m_nPHDPackets;
           unsigned long    m_oldtime;
		   unsigned __int64 m_waveFileTime;
           int              m_nCount;
           int              m_nChannels;
           int              m_nTotalLen;
           double           m_lfRms;
           short            m_nRxAtt;
           unsigned short   m_flags;
           short            m_kFactor;
           char             m_sDemod[8];
		   time_t           m_RecordingTime;
           DWORDLONG  m_nIFSampleCounter;
           DWORDLONG  m_nIFStartTimestamp;
           DWORDLONG  m_nIFSampleRate;
           DWORDLONG  m_nVideoSampleCounter;
           HANDLE     m_ThreadHandle;

private:
           CCmdSock        *m_pCmdSock;         // pointer to SCPI command socket holding device information
};

enum
{
    CCIR7,
    CCIR1,
    CCITT,
    EEA,
    EIA,
    EURO,
    NATEL,
    VDEW,
    ZVEI1,
    ZVEI2,
    DTMF,
    CTCSS,
    DCS,
    MAX_SELCALL_MODES   // maximale Anzahl von Selcall Verfahren
};

#endif


/* GLOBAL VARIABLES **********************************************************/

/* GLOBAL CONSTANTS **********************************************************/

/* GLOBAL FUNCTIONS **********************************************************/

