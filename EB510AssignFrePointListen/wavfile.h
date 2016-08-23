/*****************************************************************************
******************************************************************************
** COPYRIGHT:    (c) 2004 Rohde & Schwarz, Munich
** MODULE:       wavfile.h
** ABBREVIATION:   
** COMPILER:     VC++ 5.0
** LANGUAGE:     C/C++
** AUTHOR:       Florian Behrens
** ABSTRACT:     
** PREMISES:     
** REMARKS:      
** HISTORY:      
**  2003-09-11: (FB) Version 3.71: Creation
** REVIEW:       
******************************************************************************/

#ifndef _WAVFILE_H_
#define _WAVFILE_H_

/* INCLUDE FILES ***************************************************************/

#include <stdio.h>
#include <time.h>

/* GLOBAL DEFINES ************************************************************/

typedef struct
{
    char        sName[100];
    DWORDLONG   freq;
    DWORD       bandwidth;
    DWORD       span;
    char        sDemod[8];
} META_INFO_TYPE;

class CWAVFile  
{
public:
	CWAVFile();
	virtual ~CWAVFile();

  bool Open(unsigned unSampleRate, unsigned unChannelCount, 
            unsigned unSampleWidth, DWORDLONG ts, META_INFO_TYPE *pMetaInfo = NULL);
  bool Close(bool bRemove = false);

  virtual unsigned write(void* pvData, unsigned unByteCount);

  void     SetFilename(char* pcFilename, DWORDLONG dwlRecordingTime = 0);
  char*    GetFilename() const;
  unsigned GetSampleRate() const;
  unsigned GetChannelCount() const;
  unsigned GetSampleWidth() const;
  bool     IsOpen() const;

  // overrideable callback for additional meta information
  virtual  void MetafileCallback(FILE *pMetafile) { };

protected:
  char    *m_pcFilename;
  char    *m_pMetaFilename;
  FILE    *m_fFile;
  unsigned m_unFmtChunkPosition;
  unsigned m_unDataChunkPosition;
  unsigned m_unByteCount;
  unsigned m_unSampleRate;
  unsigned m_unChannelCount;
  unsigned m_unSampleWidth;
  float    *m_pfData;
  unsigned m_unDataLen;
  char     m_cFilename[1024];
  DWORDLONG m_dwlRecordingTime;
  DWORDLONG m_dwlTimestamp;
  time_t   m_RecordingTime;

  bool PrepareFile();
  bool PostprocWaveFile();
  void PrintTimestamp(DWORDLONG ts);
};

#endif // _WAVFILE_H_
/* End of file ***************************************************************/
