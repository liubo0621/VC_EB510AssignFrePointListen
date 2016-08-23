/*******************************************************************************
********************************************************************************
** COPYRIGHT:    (c) 1995-2004 Rohde & Schwarz, Munich
** MODULE:       Sound.cpp
** ABBREVIATION:   
** COMPILER:     Visual C++ 5.0
** LANGUAGE:     C/C++
** AUTHOR:       Martin Hisch
** ABSTRACT:     
** PREMISES:     
** REMARKS:      
** HISTORY:      
**  2000-5-29: (Hh) Creation
** REVIEW:       
********************************************************************************/

/* INCLUDE FILES ***************************************************************/

/* IMPORT */
#include <stdio.h>
#include <assert.h>
/* EXPORT */
#include "sound.h"

/* GLOBAL VARIABLES DEFINITION *************************************************/

/* GLOBAL CONSTANTS DEFINITION *************************************************/

/* GLOBAL DEFINES **************************************************************/

/* LOCAL DEFINES ***************************************************************/

#define ASSERT assert

/* LOCAL TYPES DECLARATION *****************************************************/

/* LOCAL CLASSES DECLARATION ***************************************************/

/* LOCAL VARIABLES DEFINITION **************************************************/

/* LOCAL CONSTANTS DEFINITION **************************************************/

/* LOCAL FUNCTIONS DEFINITION **************************************************/
/* FUNCTION ********************************************************************/
CSound::CSound()

/*
SPECIFICATION:
Construktor. Initializing member variables
PARAMETERS:
PRECONDITIONS: 
SIDE_EFFECTS: 
RETURN_VALUES: 
EXCEPTIONS: 
********************************************************************************/
{
  m_Handle = (HWAVEOUT) -1;
  m_Format.wfx.wFormatTag = -1;
  int i;
  for (i=0; i<MAX_WAVE_HEADERS; i++)
  {
    m_pWaveHdr[i] = new WAVEHDR;
  }

}

/* FUNCTION ********************************************************************/
CSound::~CSound()

/*
SPECIFICATION:
PARAMETERS:
PRECONDITIONS: 
SIDE_EFFECTS: 
RETURN_VALUES: 
EXCEPTIONS: 
********************************************************************************/
{
  int i;
  for (i=0; i<MAX_WAVE_HEADERS; i++)
  {
    delete m_pWaveHdr[i];
  }

}

/* FUNCTION ********************************************************************/
void CSound::StartOutput(void)

/*
SPECIFICATION:
Configuring Sound Output Device 
PARAMETERS:
void : 
PRECONDITIONS: 
SIDE_EFFECTS: 
RETURN_VALUES: 
EXCEPTIONS: 
********************************************************************************/
{
  MMRESULT mmResult = waveOutOpen(&m_Handle, WAVE_MAPPER, (WAVEFORMATEX*)&m_Format,
        0, 0, CALLBACK_NULL); 
  if (mmResult == MMSYSERR_ALLOCATED)
  {
    printf("No sound output device available !\n");
    m_Handle = (HWAVEOUT) -1;
    return;
  }
  int i;
  for (i=0; (mmResult == MMSYSERR_NOERROR) && (i<MAX_WAVE_HEADERS); i++)
  {
    m_pWaveHdr[i]->dwBufferLength = WAVE_BLOCK_LEN * m_Format.wfx.nBlockAlign;
    m_pWaveHdr[i]->lpData = new char[m_pWaveHdr[i]->dwBufferLength];
    m_pWaveHdr[i]->dwFlags = 0;
    m_pWaveHdr[i]->dwUser = 0;
    mmResult = waveOutPrepareHeader(m_Handle, m_pWaveHdr[i], sizeof(WAVEHDR));
    m_pWaveHdr[i]->dwFlags |= WHDR_DONE;
  }
  m_nNextBuffer = 0;
  m_nReadIndex = 0;
}

/* FUNCTION ********************************************************************/
void CSound::StopOutput(void)

/*
SPECIFICATION:
Stopping sound output
PARAMETERS:
void : 
PRECONDITIONS: 
SIDE_EFFECTS: 
RETURN_VALUES: 
EXCEPTIONS: 
********************************************************************************/
{
  MMRESULT mmResult;
  int i;

  mmResult = waveOutReset(m_Handle);
  for (i=0; (mmResult == MMSYSERR_NOERROR) && (i<MAX_WAVE_HEADERS); i++)
  {
    mmResult = waveOutUnprepareHeader(m_Handle, m_pWaveHdr[i], sizeof(WAVEHDR));
    if (mmResult == MMSYSERR_NOERROR)
    {
      delete m_pWaveHdr[i]->lpData;
    }
  }
  ASSERT(mmResult == MMSYSERR_NOERROR);
  mmResult = waveOutClose(m_Handle);
  m_Handle = (HWAVEOUT) -1;
  ASSERT(mmResult == MMSYSERR_NOERROR);
}

/* FUNCTION ********************************************************************/
WAVEHDR   *CSound::AllocBuffer(void)

/*
SPECIFICATION:
Search for next free buffer in m_pWaveHdr[]
PARAMETERS:
void : 
PRECONDITIONS: 
SIDE_EFFECTS: 
RETURN_VALUES: 
EXCEPTIONS: 
********************************************************************************/
{
  if (m_pWaveHdr[m_nNextBuffer]->dwFlags & WHDR_DONE)
  {
    WAVEHDR *pTemp = m_pWaveHdr[m_nNextBuffer++];
    if (m_nNextBuffer >= MAX_WAVE_HEADERS)
    {
      m_nNextBuffer -= MAX_WAVE_HEADERS;
    }
    return pTemp;
  }
  else
  {
    // No more buffers available. Increase MAX_WAVE_HEADERS !!!!!!!!
    printf("No more buffers for sound output available. Increase MAX_WAVE_HEADERS !!!!!!!!\n");
    return NULL;
  }
}

/* FUNCTION ********************************************************************/
void CSound::SendBuffer(WAVEHDR *pWaveHdr, int nLen)

/*
SPECIFICATION:
Send buffer to sound output device
PARAMETERS:
WAVEHDR *pWaveHdr : Buffer to be sent 
int nLen : Number of Databytes in Buffer
PRECONDITIONS: 
SIDE_EFFECTS: 
RETURN_VALUES: 
EXCEPTIONS: 
********************************************************************************/
{
  pWaveHdr->dwFlags &= ~WHDR_DONE;
  pWaveHdr->dwBufferLength = nLen;
  int nBuffersInQueue = m_nNextBuffer - m_nReadIndex;
  if (nBuffersInQueue < 0)
  {
    nBuffersInQueue += MAX_WAVE_HEADERS;
  }
  if (nBuffersInQueue >= m_nThreshold)
  {
    // output buffer to sound output device
    waveOutWrite(m_Handle, m_pWaveHdr[m_nReadIndex++], sizeof(WAVEHDR));
    if (m_nReadIndex >= MAX_WAVE_HEADERS)
    {
      m_nReadIndex -= MAX_WAVE_HEADERS;
    }
  }
}

/* FUNCTION ********************************************************************/
void  CSound::FlushBuffers(void)

/*
SPECIFICATION:
Send all remaining buffers in sound output queue
PARAMETERS:
void : 
PRECONDITIONS: 
SIDE_EFFECTS: 
RETURN_VALUES: 
EXCEPTIONS: 
********************************************************************************/
{
  while (m_nReadIndex != m_nNextBuffer && m_Handle != (void*)-1)
  {
    // output buffer to sound output device
    waveOutWrite(m_Handle, m_pWaveHdr[m_nReadIndex++], sizeof(WAVEHDR));
    if (m_nReadIndex >= MAX_WAVE_HEADERS)
    {
      m_nReadIndex -= MAX_WAVE_HEADERS;
    }
  }
}

/* FUNCTION ********************************************************************/
void  CSound::Play(bool bIsGSM, long nSampleRate, int nBitsPerSample, int nNoOfChannels, 
           void *pData, int nNoOfFrames)

/*
SPECIFICATION:
Output Data to sound output device
PARAMETERS:
bool bIsGSM : true -> GSM 6.10 Data
long nSampleRate : Sampling rate in Hz
int nBitsPerSample : Number of Bits per Sample (8 or 16)
int nNoOfChannels : Number of Channels (1 or 2)
void *pData : Pointer to data
int nNoOfFrames : Number of Frames
PRECONDITIONS: 
SIDE_EFFECTS: 
RETURN_VALUES: 
EXCEPTIONS: 
********************************************************************************/
{
  if (nNoOfFrames == 0)
  {
    FlushBuffers();
    return;
  }

  bool bFormatChanged = false;
  // Check already opened output device format
  if (bIsGSM && (m_Format.wfx.wFormatTag != WAVE_FORMAT_GSM610))
  {
    bFormatChanged = true;
  }
  if (!bIsGSM && (m_Format.wfx.wFormatTag != WAVE_FORMAT_PCM))
  {
    bFormatChanged = true;
  }
  if (!bIsGSM && (m_Format.wfx.nSamplesPerSec != (unsigned long)nSampleRate))
  {
    bFormatChanged = true;
  }
  if (!bIsGSM && (m_Format.wfx.wBitsPerSample != nBitsPerSample))
  {
    bFormatChanged = true;
  }
  if (!bIsGSM && (m_Format.wfx.nChannels != nNoOfChannels))
  {
    bFormatChanged = true;
  }
  if (bFormatChanged)
  {
    // some parameters changed in audio format
    // stop output
    if (m_Handle != (void*)-1)
    {
      StopOutput();
    }
    // setup new format
    if (!bIsGSM)
    {
      m_Format.wfx.wFormatTag = WAVE_FORMAT_PCM;
      m_Format.wfx.nChannels = nNoOfChannels; 
      m_Format.wfx.nSamplesPerSec = nSampleRate; 
      m_Format.wfx.wBitsPerSample = nBitsPerSample;
      m_Format.wfx.nBlockAlign = nBitsPerSample/8 * nNoOfChannels; 
      m_Format.wfx.nAvgBytesPerSec = m_Format.wfx.nBlockAlign*m_Format.wfx.nSamplesPerSec;
      m_Format.wfx.cbSize = 0;
      m_nThreshold = BUFFER_THRESHOLD_PCM;
      StartOutput();
    }
    else
    {
      m_Format.wfx.wFormatTag = WAVE_FORMAT_GSM610;
      m_Format.wfx.nSamplesPerSec = nSampleRate;
      m_Format.wfx.nChannels = nNoOfChannels;
      m_Format.wfx.nBlockAlign = 65;
      m_Format.wfx.nAvgBytesPerSec = 1625;
      m_Format.wfx.wBitsPerSample = 0;
      m_Format.wfx.cbSize = 2;
      m_Format.wSamplesPerBlock = 320;
      m_nThreshold = BUFFER_THRESHOLD_GSM;
      StartOutput();
    }
  }

  if (m_Handle != (void*)-1)
  {
    // Copy data to buffers
    char *pTemp = (char *)pData;
    while (nNoOfFrames)
    {
      WAVEHDR *pBuffer = AllocBuffer();
      if (pBuffer != NULL)
      {
        int nCount = nNoOfFrames;
        if (nNoOfFrames > WAVE_BLOCK_LEN)
          nCount = WAVE_BLOCK_LEN;
        memcpy(pBuffer->lpData, pTemp, nCount * m_Format.wfx.nBlockAlign);
        SendBuffer(pBuffer, nCount * m_Format.wfx.nBlockAlign);
        nNoOfFrames -= nCount;
        pTemp += nCount * m_Format.wfx.nBlockAlign;
      }
      else
      {
        // not enough buffers
        nNoOfFrames = 0;
      }
    }
  }
}

