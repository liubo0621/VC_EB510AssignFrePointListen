/*******************************************************************************
********************************************************************************
** COPYRIGHT:      (c) 1995-2004 Rohde & Schwarz, Munich
** MODULE:         sound.h
** ABBREVIATION:   
** COMPILER:       Visual C++ 5.0
** LANGUAGE:       C/C++
** AUTHOR:         Martin Hisch
** ABSTRACT:       Definition of a sound output class for Windows
** PREMISES:       
** REMARKS:        
** HISTORY:        
**	2000-5-29: (Hh)	Creation
** REVIEW:         
********************************************************************************/

/* GLOBAL DEFINES **************************************************************/
#ifndef __SOUND_DEF
#define __SOUND_DEF
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>

/* change following defines to adjust realtime behaviour according to your computer system */
#define MAX_WAVE_HEADERS    50      /* increase this if you run out of buffers */
#define BUFFER_THRESHOLD_PCM 5      /* this is the time delay between input and output
                                       measured in number of buffers for all PCM signals */
#define BUFFER_THRESHOLD_GSM 2      /*  this is the time delay between input and output
                                       measured in number of buffers for all GSM signals */
#define WAVE_BLOCK_LEN      512     /* size in frames of each output buffer */

class CSound
{
public:
    CSound();
    virtual ~CSound();
    
    void    Play(bool bIsGSM, long nSampleRate, int nBitsPerSample, int nNoOfChannels, 
                 void *pData, int nNoOfSamples);
    void    FlushBuffers(void);

protected:
    void    StartOutput(void);
    void    StopOutput(void);
    WAVEHDR   *AllocBuffer(void);
    void    SendBuffer(WAVEHDR *pBuffer, int nLen);
protected:
    HWAVEOUT        m_Handle;
    GSM610WAVEFORMAT  m_Format;
    WAVEHDR *m_pWaveHdr[MAX_WAVE_HEADERS];
    int             m_nNextBuffer;
    int             m_nReadIndex;
    int             m_nThreshold;
};

#endif
