/*******************************************************************************
********************************************************************************
** COPYRIGHT:    (c) 2004 Rohde & Schwarz, Munich
** MODULE:       wavfile.cpp
** ABBREVIATION:   
** COMPILER:     VC++ 5.0
** LANGUAGE:     C/C++
** AUTHOR:       Florian Behrens
** ABSTRACT:     
** PREMISES:     
** REMARKS:      
** HISTORY:      
**  2005-07-11: (Ob) Version 3.72: 8 Bit support added
**  2003-09-11: (FB) Version 3.71: Creation
** REVIEW:       
********************************************************************************/

/* INCLUDE FILES ***************************************************************/

/* IMPORT */
#include <memory.h>
#include <windows.h>
#include <time.h>

/* EXPORT */
#include "wavfile.h"

/* LOCAL TYPES DECLARATION *****************************************************/

/* Strukturen fuer das WAV-Format */
#pragma pack(push, 1)
struct struRIFFHeader
{
  char     cRIFFID[4];     // RIFF file id ('RIFF')
	unsigned unLength;       // Length in bytes after RIFF header
	char     cFormatID[4];   // Format id ('WAVE' for .WAV files)
};

struct struChunkHeader
{
  char     cChunkID[4];
  unsigned unChunkSize;    // Chunk length in bytes excluding header
};

struct struFormatChunkBody
{
  short          sFormatTag;
  unsigned short usChannels;
  unsigned       unSamplesPerSec;
  unsigned       unAvgBytesPerSec;
  short          sBlockAlign;
  short          sBitsPerSample;
  unsigned short usCbSize;
  // Note: there may be additional fields here, depending upon wFormatTag
};
#pragma pack(pop)

/* GLOBAL VARIABLES DEFINITION *************************************************/

/* GLOBAL CONSTANTS DEFINITION *************************************************/

/* GLOBAL DEFINES **************************************************************/

/* LOCAL DEFINES ***************************************************************/

/* flags for sFormatTag field of struFormatChunkBody */
#define WAVE_FORMAT_PCM 1

/* LOCAL TYPES DECLARATION *****************************************************/

/* LOCAL CLASSES DECLARATION ***************************************************/

/* LOCAL VARIABLES DEFINITION **************************************************/

/* LOCAL CONSTANTS DEFINITION **************************************************/

/* LOCAL FUNCTIONS DEFINITION **************************************************/

CWAVFile::CWAVFile()
: m_pcFilename(NULL),
  m_fFile(NULL),
  m_unFmtChunkPosition(0),
  m_unDataChunkPosition(0),
  m_unByteCount(0),
  m_unSampleRate(0),
  m_unChannelCount(0),
  m_unSampleWidth(0),
  m_pfData(NULL),
  m_unDataLen(0),
  m_dwlRecordingTime(0),
  m_pMetaFilename(NULL),
  m_dwlTimestamp(0)
{}

CWAVFile::~CWAVFile()
{
  if (m_fFile)
    Close();
  if (m_pMetaFilename != NULL)
  {
      delete [] m_pMetaFilename;
  }
  if (m_pfData != NULL)
  {
      delete [] m_pfData;
  }
}

/***** FUNCTION *************************************************************/

bool CWAVFile::PrepareFile()

/****************************************************************************/
{
  /* Leeren Kopf der WAV-Datei als Platzhalter schreiben */
  struRIFFHeader ctRIFFHeader;
  if (fwrite(&ctRIFFHeader, sizeof(struRIFFHeader), 1, m_fFile) != 1)
    return false;

  /* Leeren 'fmt'-Chunk als Platzhalter schreiben */
  struChunkHeader ctChunckHeader;
  struFormatChunkBody ctFmtChunkBody;
  m_unFmtChunkPosition = ftell(m_fFile);
  if (fwrite(&ctChunckHeader, sizeof(struChunkHeader), 1, m_fFile) != 1)
    return false;
  if (fwrite(&ctFmtChunkBody, sizeof(struFormatChunkBody), 1, m_fFile) != 1)
    return false;
  
  /* Leeren 'data'-Chunk-Header als Platzhalter schreiben */
  m_unDataChunkPosition = ftell(m_fFile);
  memcpy(ctChunckHeader.cChunkID, "data", sizeof(ctChunckHeader.cChunkID));
  ctChunckHeader.unChunkSize = 0;
  if (fwrite(&ctChunckHeader, sizeof(struChunkHeader), 1, m_fFile) != 1)
    return false;

  return true;
}

/***** FUNCTION *************************************************************/

bool CWAVFile::PostprocWaveFile()

/****************************************************************************/
{
  fseek(m_fFile, 0, SEEK_SET);

  /* Kopf der WAV-Datei schreiben */
  struRIFFHeader ctRIFFHeader;
  memcpy(ctRIFFHeader.cRIFFID, "RIFF", sizeof(ctRIFFHeader.cRIFFID));
  ctRIFFHeader.unLength = sizeof(struRIFFHeader) - sizeof(ctRIFFHeader.cRIFFID) +
                          2 * sizeof(struChunkHeader) + sizeof(struFormatChunkBody) +
                          m_unByteCount;
  memcpy(ctRIFFHeader.cFormatID, "WAVE", sizeof(ctRIFFHeader.cFormatID));
  if (fwrite(&ctRIFFHeader, sizeof(struRIFFHeader), 1, m_fFile) != 1)
    return false;

  /* 'fmt'-Chunk schreiben */
  fseek(m_fFile, m_unFmtChunkPosition, SEEK_SET);
  struChunkHeader ctChunckHeader;
  struFormatChunkBody ctFmtChunkBody;
  memcpy(ctChunckHeader.cChunkID, "fmt ", sizeof(ctChunckHeader.cChunkID));
  ctChunckHeader.unChunkSize = sizeof(struFormatChunkBody);
  if (fwrite(&ctChunckHeader, sizeof(struChunkHeader), 1, m_fFile) != 1)
    return false;

  ctFmtChunkBody.sFormatTag       = WAVE_FORMAT_PCM;
  ctFmtChunkBody.usChannels       = m_unChannelCount; // 2-Kanal wg. I und Q
  ctFmtChunkBody.unSamplesPerSec  = m_unSampleRate;
  ctFmtChunkBody.sBlockAlign      = m_unSampleWidth; // 4 Bytes pro (komplexem) Abtastwert
  ctFmtChunkBody.unAvgBytesPerSec = m_unSampleRate * m_unSampleWidth;
  ctFmtChunkBody.sBitsPerSample   = 8*m_unSampleWidth/m_unChannelCount;
  ctFmtChunkBody.usCbSize         = 0;
#if (1)
  if (ctFmtChunkBody.sBitsPerSample == 32)
  {
      // float !!!!! wegen MATLAB
      ctFmtChunkBody.sFormatTag = 3;

  }
#endif
  if (fwrite(&ctFmtChunkBody, sizeof(struFormatChunkBody), 1, m_fFile) != 1)
    return false;

  /* 'data'-Chunk-Header schreiben */
  fseek(m_fFile, m_unDataChunkPosition, SEEK_SET);
  memcpy(ctChunckHeader.cChunkID, "data", sizeof(ctChunckHeader.cChunkID));
  ctChunckHeader.unChunkSize = m_unByteCount;
  if (fwrite(&ctChunckHeader, sizeof(struChunkHeader), 1, m_fFile) != 1)
    return false;
  
  return true;
}

/* FUNCTION ********************************************************************/
void CWAVFile::PrintTimestamp(DWORDLONG ts)

/*
SPECIFICATION:
Prints a timestamp
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
        sprintf(m_cFilename, "%04d%02d%02d_%02d%02d%02d_%09d.wav", tms.tm_year+1900, tms.tm_mon+1, tms.tm_mday, tms.tm_hour, tms.tm_min, tms.tm_sec, tns);
    }
}

/***** FUNCTION *************************************************************/

bool CWAVFile::Open(unsigned unSampleRate, unsigned unChannelCount, 
                    unsigned unSampleWidth, DWORDLONG ts, META_INFO_TYPE *pMetaInfo)

/****************************************************************************/
{
  if (m_fFile != NULL ||
      unSampleRate    == 0 || 
      (unChannelCount != 1 && unChannelCount != 2) || 
      (unSampleWidth  != 1 && unSampleWidth  != 2 && unSampleWidth  != 4 && unSampleWidth != 8))
    return false;


  /* Open WAV file for binary writing */
  if (m_pcFilename == NULL || (m_pcFilename == &(m_cFilename[0])))
  {
      // generate filename out of ts 
    PrintTimestamp(ts);
    m_pcFilename = &(m_cFilename[0]);
  }
  m_fFile = fopen(m_pcFilename, "wb");
  if (m_fFile == NULL)
    return false;

  m_unByteCount = 0;
  time(&m_RecordingTime);

  m_dwlTimestamp = ts;
  m_unSampleRate   = unSampleRate;
  m_unChannelCount = unChannelCount;
  m_unSampleWidth  = unSampleWidth;

  if (pMetaInfo != NULL)
  {
      // open metafile
      // search for fileextension
      if (m_pMetaFilename != NULL)
      {
          delete [] m_pMetaFilename;
      }
      m_pMetaFilename = new char[strlen(m_pcFilename)+10];
      strcpy(m_pMetaFilename, m_pcFilename);

      char *pExtension = strrchr(m_pMetaFilename, '.');
      if (pExtension != NULL)
      {
          strcpy(pExtension, ".met");
      }
      else
      {
          strcat(m_pMetaFilename, ".met");
      }
      FILE *pMetaFile = fopen(m_pMetaFilename, "wt");
      if (pMetaFile != NULL)
      {
          fprintf(pMetaFile, "Name: %s\n", pMetaInfo->sName);
          fprintf(pMetaFile, "Frequency: %I64u\n", pMetaInfo->freq);
          fprintf(pMetaFile, "Bandwidth: %d\n", pMetaInfo->bandwidth);
          fprintf(pMetaFile, "Span: %d\n", pMetaInfo->span);
          fprintf(pMetaFile, "Demodulation: %s\n", pMetaInfo->sDemod);
          fprintf(pMetaFile, "Samplerate: %u\n", unSampleRate);
          fprintf(pMetaFile, "Timestamp: %I64u\n", ts);
          MetafileCallback(pMetaFile);
          fclose(pMetaFile);
      }
  }

  return PrepareFile();
}

/***** FUNCTION *************************************************************/

bool CWAVFile::Close(bool bRemove)

/****************************************************************************/
{
    bool bResult = false;
    if (m_fFile != NULL)
    {
      bResult = PostprocWaveFile();
      fclose(m_fFile);
      m_fFile = NULL;
      if (bRemove)
      {
          // delete file, b/c it is corrupt
          remove(m_pcFilename);
          if (m_pMetaFilename != NULL)
          {
              remove(m_pMetaFilename);
          }
      }
    }
    return bResult;
}

/***** FUNCTION *************************************************************/

unsigned CWAVFile::write(void* pvData, unsigned unByteCount)

/****************************************************************************/
{
  unsigned unBytesWritten = 0;

    if (m_dwlRecordingTime > 0)
    {
        // check if recording limit is reached
        time_t temp;
        time(&temp);
        time_t temp2 = (time_t)m_dwlRecordingTime;
        if ((temp - m_RecordingTime) > temp2)
        {
            // stop recording
            return unBytesWritten;
        }
    }

  if (m_fFile)
  {
#if (1)
      if ((8*m_unSampleWidth/m_unChannelCount) == 32)
      {
          // umwandlung 32 Bit fixed format -> float format
          unsigned int nCount = unByteCount/sizeof(long);
          if (m_unDataLen < unByteCount)
          {
              if (m_pfData != NULL)
              {
                  delete [] m_pfData;
              }

              m_pfData = new float[nCount];
              m_unDataLen = unByteCount;
          }

          if (m_pfData != NULL)
          {
              unsigned i;
              long *pLong = (long*)pvData;
              for (i=0; i<nCount; i++)
              {
                  m_pfData[i] = (float)(pLong[i]);
                  m_pfData[i] /= 0x7FFFFFFF;
              }
              unBytesWritten = sizeof(float) * fwrite(m_pfData, sizeof(float), i, m_fFile);
          }
      }
      else
#endif
      {
        unBytesWritten = fwrite(pvData, sizeof(char), unByteCount, m_fFile);
      }
    m_unByteCount += unBytesWritten;
  }

  return unBytesWritten;
}

/***** FUNCTION *************************************************************/

void CWAVFile::SetFilename(char* pcFilename, DWORDLONG dwlRecordingTime)

/****************************************************************************/
{
  m_pcFilename = pcFilename;
  m_dwlRecordingTime = dwlRecordingTime;
}


/***** FUNCTION *************************************************************/

char* CWAVFile::GetFilename() const

/****************************************************************************/
{
  return m_pcFilename;
}

/***** FUNCTION *************************************************************/

unsigned CWAVFile::GetSampleRate() const

/****************************************************************************/
{
  return m_unSampleRate;
}

/***** FUNCTION *************************************************************/

unsigned CWAVFile::GetChannelCount() const

/****************************************************************************/
{
  return m_unChannelCount;
}

/***** FUNCTION *************************************************************/

unsigned CWAVFile::GetSampleWidth() const

/****************************************************************************/
{
  return m_unSampleWidth;
}

/***** FUNCTION *************************************************************/

bool CWAVFile::IsOpen() const

/****************************************************************************/
{
  if (m_fFile)
    return true;
  return false;
}

/* End of file ***************************************************************/
