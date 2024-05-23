#include <stdio.h>
#include <Windows.h>
extern "C" {
#include <initguid.h>
#include <mmdeviceapi.h>
}
#include <Audioclient.h>

#include <assert.h>
#include <iostream>
#include <sstream>

typedef struct AudioCaptureFormat {
    bool formatisValid;
    unsigned int frameSize;
    unsigned int numChannels;
    unsigned int bitsPerSample;
    unsigned int samplesPerSec;
} AudioCaptureFormat;

class AudioCaptureClient {
private:
    HRESULT hr;
    IMMDeviceEnumerator* enumerator = NULL;
    IMMDevice* recorder = NULL;
    IAudioClient* recorderClient = NULL;
    IAudioCaptureClient* captureService = NULL;
    WAVEFORMATEX* format = NULL;

    UINT32 nFrames;
    DWORD flags;
    BYTE* captureBuffer;

    int iSamp=0;
    float maxval=0;

    int locked=0;

public:
    void initializeCom();
    void startCapture();
    AudioCaptureFormat getAudioFormat();
    int getBytesPerSample();
    UINT32 getNextPacketSize();
    UINT32 getBuffer(UINT32 expectedFrameCount, UINT32 maximumFrameCount, char *out);
    void stopCapture();
};

extern "C" {
    void *createCaptureClient();
    void initializeCom(void* client); // must not be called from Electron app because Electron already initializes COM
    void startCapture(void* client);
    AudioCaptureFormat getAudioFormat(void* client);
    UINT32 getNextPacketSize(void* client);
    UINT32 getBuffer(void* client, UINT32 expectedFrameCount, UINT32 maximumFrameCount, char *out);
    void stopCapture(void* client);
}
