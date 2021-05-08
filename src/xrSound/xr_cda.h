#ifndef __XR_CDAUDIO_H__
#define __XR_CDAUDIO_H__

typedef enum CDA_STATE { CDA_STATE_PLAY, CDA_STATE_STOP, CDA_STATE_PAUSE, CDA_STATE_OPEN, CDA_STATE_NOTREADY };

class CCDA
{
    char retStr[64];
    u32 retLen;
    MCIERROR err;

    u32 dwCurTrack;
    BOOL bWorking;
    BOOL bPaused;

    int lKeepTime;
    int lTotalTime;

    CDA_STATE GetState();

public:
    CCDA();
    ~CCDA();

    void Open();
    void Close();
    void SetTrack(int track);
    void Play();
    void Stop();
    void Pause();

    void OnMove();
};

#endif //__XR_CDAudio_H__
