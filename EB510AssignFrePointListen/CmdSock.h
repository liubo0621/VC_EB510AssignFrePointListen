/*******************************************************************************
********************************************************************************
** COPYRIGHT:    (c) 2011-2012 Rohde & Schwarz, Munich
** MODULE:       CmdSocket.h
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
********************************************************************************/

/* INCLUDE FILES ***************************************************************/

/* IMPORT */
#ifndef _WINSOCKAPI_
#include <winsock2.h>
#endif

/* EXPORT */

/* LOCAL TYPES DECLARATION *****************************************************/

/* GLOBAL VARIABLES DEFINITION *************************************************/

/* GLOBAL CONSTANTS DEFINITION *************************************************/
#ifndef _CMDSOCK_DEF_
#define _CMDSOCK_DEF_

#define BUFFER_SIZE 200

class CEB200UdpSock;

class CCmdSock
{
public:
	CCmdSock( );
	virtual ~CCmdSock( ) { };
    
    virtual void   Init( int argc, char **argv );
    virtual void   DeleteTraces( void );
	        /* stop current task*/
	        void   stopWrite();
            int    CheckDeviceErrors( void );

            /* get device type */
            bool   EquipmentisEM050( void ) { return (m_isEM050 || m_isESMD); };

            /* device options */
            bool   OptionIsDS( void ) { return m_isDS; };
            bool   OptionIsPS( void ) { return m_isPS; };
            bool   OptionIsFS( void ) { return m_isFS; };
            bool   OptionIsCM( void ) { return m_isCM; };
            bool   OptionIsSL( void ) { return m_isSL; };
            bool   OptionIsDF( void ) { return m_isDF; };

            /* cmdline options, used by EB200UdpSock */
            bool   Verbose( void ) { return m_bVerbose; };
            bool   ChannelsOnly( void ) { return m_bChannelsOnly; };
            bool   BytesOnly( void ) { return m_bBytesOnly; };

protected:
    virtual bool   ParseCmdLine( int argc, char **argv );
    virtual void   PrintHelp( void );

    /* retrieve R&S device information */
    virtual char*  GetDeviceID( void );
    virtual int    GetDeviceOptions( void );
    virtual unsigned long GetRemoteAddress( void ) { return m_ulRemoteAddress; };

    /* TRACe subsystem, TAGs, FLAGs and data paths */
    virtual void   ConfigureTraceCmd( void);
    virtual void   SetupTraces( void );
    virtual void   SetupDataPaths( void );

    /* send/receive srings to/from device */
            int    SendCmd( char *pBuffer );
            int    Receive( char *pBuffer, int len, int flags );

            char*  GetLocalIP( void );
            void   DeleteAllTraces( void );

            unsigned long SetRSDevice( char *pcDeviceAddress, unsigned short nPort );
protected:
            unsigned       m_nPort;
            unsigned       m_unPort;
            unsigned       m_unDDC;
            unsigned       m_unAudioMode;
			unsigned __int64   m_waveFileTime;

            double         m_lfFmax;

            bool           m_isESMD;
            bool           m_isEM050;
            bool           m_isPR100;
            bool           m_isEB200;

            bool           m_isDS;
            bool           m_isPS;
            bool           m_isFS;
            bool           m_isCM;
            bool           m_isSL;
            bool           m_isDF;
            bool           m_isPIFP;

            bool           m_bAudioOnlyMode;
            bool           m_bRaw;
            bool           m_bChannelsOnly;
            bool           m_bBytesOnly;
            bool           m_bUseCmdSock;
            bool           m_bIQMode;
            bool           m_bIQModeShort;
            bool           m_bAIfOnly;
            bool           m_bDelAll;
            bool           m_bVerboseCmd;
            bool           m_bVerbose;
            bool           m_bGPSOutput;
            bool           m_bTracTcp;
            bool           m_bCSMode;
            bool           m_bOmniPhase;

            char           m_cTracType[16];
            char           m_cTracFlag[BUFFER_SIZE];
            char           m_cTracTag[BUFFER_SIZE];
            char           m_cDDCx[8];

            char          *m_pcDeviceAddress;
            char          *m_pcWAVFile;
            char          *m_pcIQFile;
            char          *m_pcCSFile;
            char          *m_pIP;

            unsigned long  m_ulRemoteAddress;

            unsigned short m_usRecPort;
            unsigned short m_usCfgPort;
            unsigned short m_usPort;

            DWORDLONG      m_dwlRecordingTime;
            SOCKET         m_Socket;
            CEB200UdpSock *m_pTracSock;
};

#endif // _CMDSOCK_DEF_

/* LOCAL DEFINES ***************************************************************/
#define SCPI_PORT_NUM_DEF   5555    // (default) SCPI port numver
#define TRAC_TCP_PORT_OFFS  10      // offset rel. to SCPI port
#define MAX_DDC_NUM         4       // max. receiver instances / DDCs (curr.) supported

#define HELP_NOTICE \
"Usage:\n" \
"  UDPEXAMPLE [OPTIONS] <device>\n" \
"\n" \
"  <device> Network name or IP address of receiver device.\n" \
"\n" \
"OPTIONS:\n" \
"  -p <device IP port> TCP port of receiver device, defaults to 5555.\n" \
"\n" \
"  -d <device instance> receiver instance i.e. DDC number [0, 1..4]\n" \
"        Defaults to 0. Implies -iqm[l] if instance > 0.\n" \
"\n" \
"  -am <audio mode> Audio mode. Refer to receiver manual for valid options.\n" \
"        Defaults to 0 (no audio).\n" \
"\n" \
"  -af <audio file> Name of file for the audio data to be saved in .WAV format. \n" \
"        If omitted, audio data will be forwarded to sound card, if installed.\n" \
"\n" \
"  -a    Audio only mode. All other datagrams will be ignored. Implies -am.\n" \
"\n" \
"  -c    Channels/sec statistics output only.\n" \
"\n" \
"  -bs   Bytes/sec statistics output only.\n" \
"\n" \
"  -gps  GPS Data Stream.\n" \
"\n" \
"  -omn  Omniphase output.\n" \
"\n" \
"  -v    Verbose. Output of additional information marked with '*'.\n" \
"\n" \
"  -vc   Verbose SCPI commands send to receiver device\n" \
"\n" \
"  -vv   Verbose incl. SCPI commands send to receiver device\n" \
"\n" \
"  -iqm  Activates IQ data transmission in SHORT format.\n\n" \
"  -iqml Activates IQ data transmission in LONG format.\n\n" \
"  -iqf[l] <IQ data file> Name of file for IQ data to be saved in stereo .WAV \n" \
"        format. If <IQ data file> == \"TS\", filename will be automatically\n" \
"        generated with timestamp information. In that case, changes in sample-\n" \
"        rate will close old file and create new file automatically.\n" \
"        Implies -iqm[l]. Generates file with .MET extension with meta info.\n\n" \
"  -iqfr <IQ data file> Name of file for IQ data to be saved in raw \n" \
"        format. Recording will be abandoned if a sample rate change occurs.\n" \
"        Implies -iqm.\n" \
"  -iqt  Max. recording time in sec. Default is infinite\n" \
"\n" \
"  -csf <CSV data file> Name of file for CSV data to be saved in ascii\n" \
"        format for Excel input.\n" \
"\n" \
"  -t    TCP tracing\n" \
"\n" \
"  -rp <trace IP port> Optional trace receive port.\n" \
"        No trace configuration is done - just receive/record.\n" \
"\n" \
"  -cp <trace IP port> Optional trace configuration port.\n" \
"        No receiving/recording is done - just configuration.\n" \
"  -ci <trace IP addr> Optional trace configuration IPv4 address.\n" \
"        Requires '-cp' option !\n" \
"  -caif <trace AMMOS IQ data format> Optional trace configuration AMMOS IQ\n" \
"        data format.\n" \
"        Requires '-cp' option ! If not given EB200 datagram format is used.\n" \
"\n" \
"  -da   Delete (all) traces. Defaults to 'UDP' traces - use '-t' for 'TCP'.\n" \
"\n" \
"\n" \
"  Press any key to terminate UDPEXAMPLE. Don't use Ctrl-C, because this\n" \
"  prevents UDPEXAMPLE from proper closing the dump files.\n"

/* LOCAL TYPES DECLARATION *****************************************************/

/* LOCAL CLASSES DECLARATION ***************************************************/

/* LOCAL VARIABLES DEFINITION **************************************************/

/* LOCAL CONSTANTS DEFINITION **************************************************/

/* LOCAL FUNCTIONS DEFINITION **************************************************/

/* End of file ***************************************************************/
