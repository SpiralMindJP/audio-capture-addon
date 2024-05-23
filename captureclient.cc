#include "captureclient.h"
#include <stdio.h>
#include <inttypes.h> 

// see https://learn.microsoft.com/en-us/windows/win32/coreaudio/capturing-a-stream
// https://github.com/microsoft/Windows-classic-samples/blob/main/Samples/ApplicationLoopback/cpp/LoopbackCapture.cpp
// https://learn.microsoft.com/en-us/answers/questions/786447/recording-desktop-audio?orderBy=Helpful
// https://github.com/microsoft/windows-classic-samples/tree/main/Samples/ApplicationLoopback#application-loopback-api-capture-sample

void AudioCaptureClient::initializeCom() {
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    assert(SUCCEEDED(hr));
}

/* for debugging

std::string ToString(GUID *guid) {
    char guid_string[37]; // 32 hex chars + 4 hyphens + null terminator
    snprintf(
          guid_string, sizeof(guid_string),
          "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
          guid->Data1, guid->Data2, guid->Data3,
          guid->Data4[0], guid->Data4[1], guid->Data4[2],
          guid->Data4[3], guid->Data4[4], guid->Data4[5],
          guid->Data4[6], guid->Data4[7]);
    return guid_string;
}
*/

void AudioCaptureClient::startCapture() {


    // see https://stackoverflow.com/questions/12844431/linking-wasapi-in-vs-2010
    // for some reason using CLSID_MMDeviceEnumerator and IID_IMMDeviceEnumerator
    // leads to link error when compiling with MS visual studio. instead we have to
    // use __uuidof(xxx) as below to fix the link eror.
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), /* CLSID_MMDeviceEnumerator, */
        NULL,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), /* IID_IMMDeviceEnumerator, */
        (void**)&enumerator
    );

    assert(SUCCEEDED(hr));

    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &recorder);
    assert(SUCCEEDED(hr));

    hr = enumerator->Release();
    assert(SUCCEEDED(hr));

    hr = recorder->Activate(
        __uuidof(IAudioClient), /* IID_IAudioClient, */
        CLSCTX_ALL, NULL, (void**)&recorderClient);
    assert(SUCCEEDED(hr));

    hr = recorderClient->GetMixFormat(&format);
    assert(SUCCEEDED(hr));

    /*
    printf("Mix format:\n");
    printf("  Frame size     : %d\n", format->nBlockAlign);
    printf("  Channels       : %d\n", format->nChannels);
    printf("  Bits per second: %d\n", format->wBitsPerSample);
    printf("  Sample rate:   : %d\n", format->nSamplesPerSec);
    printf("  wFormatTag:   : %d\n", format->wFormatTag);
    printf("  cbSize:   : %d\n", format->cbSize);
    */

    // NOTE: for audio capture, format seems to be WAVE_FORMAT_EXTENSIBLE
    // and WAVEFORMATEXTENSIBLE.SubType is expected to be KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
    // see https://stackoverflow.com/questions/41876857/interpreting-waveformatextensible-from-iaudioclientgetmixformat
    // https://learn.microsoft.com/en-us/windows/win32/api/mmreg/ns-mmreg-waveformatextensible?redirectedfrom=MSDN
    // https://stackoverflow.com/questions/30692623/wasapi-loopback-save-wave-file

    hr = recorderClient->Initialize(AUDCLNT_SHAREMODE_SHARED,  
      AUDCLNT_STREAMFLAGS_LOOPBACK, 
      10000000, 
      0, 
      format, 
      NULL);
    assert(SUCCEEDED(hr));

    hr = recorderClient->GetService(
        __uuidof(IAudioCaptureClient), /* IID_IAudioCaptureClient, */
        (void**)&captureService);
    assert(SUCCEEDED(hr));

    hr = recorderClient->Start();
    assert(SUCCEEDED(hr));

  locked=0;
}

AudioCaptureFormat AudioCaptureClient::getAudioFormat() {
  assert(format != NULL);
  // valid format should be WAVE_FORMAT_EXTENSIBLE with subformat KSDATAFORMAT_SUBTYPE_IEEE_FLOAT.
  // if format is not valid, then assumptions about parsing the data (4 byte floats, etc.) may fail.
  bool formatIsValid =
    (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    && IsEqualGUID(((WAVEFORMATEXTENSIBLE*)format)->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
  AudioCaptureFormat returnValue = {
    formatIsValid,
    format->nBlockAlign,
    format->nChannels,
    format->wBitsPerSample,
    format->nSamplesPerSec
  };

  return returnValue;
}

int AudioCaptureClient::getBytesPerSample() {
  return format->wBitsPerSample / sizeof(BYTE);
}

UINT32 AudioCaptureClient::getNextPacketSize() {
  UINT32 framesAvailable;
  hr = captureService->GetNextPacketSize(&framesAvailable);
  //printf("C++: %d frames available\n", framesAvailable);
  assert(SUCCEEDED(hr));
  return(framesAvailable);
}

UINT32 AudioCaptureClient::getBuffer(UINT32 expectedFrameCount, UINT32 maximumFrameCount, char *out) {
  // expectedFrameCount comes from previous call to getNextPacketSize().
  // frameCount must be known in advance so the caller can allocate the properly-sized
  // buffer for the *out parameter
  hr = captureService->GetBuffer(&captureBuffer, &nFrames, &flags, NULL, NULL);
  assert(SUCCEEDED(hr));
  if(expectedFrameCount != nFrames) {
    // printf("C++: expected %d (max %d) and got %d frames\n", expectedFrameCount, maximumFrameCount, nFrames);
  } 

  if (nFrames > maximumFrameCount) {
      // caller has only allocated buffer to hold up to maximumFrameCount frames.
      // if more data was received, discard the extra data.
      nFrames = maximumFrameCount; 
  }
  if(nFrames > 0) {
    memcpy(out, captureBuffer, nFrames * format->nBlockAlign);
  }

  hr = captureService->ReleaseBuffer(nFrames);
  assert(SUCCEEDED(hr));

  return nFrames;
}

void AudioCaptureClient::stopCapture() {

    recorderClient->Stop();
    captureService->Release();
    recorderClient->Release();
    recorder->Release();

    CoUninitialize();
}

// ---------------------------------------------
// C interface

void *createCaptureClient() {
  return new AudioCaptureClient();
}

void initializeCom(void* client) {
  ((AudioCaptureClient*)client)->initializeCom();
}

void startCapture(void *client) {
  ((AudioCaptureClient*)client)->startCapture();
}

AudioCaptureFormat getAudioFormat(void *client) {
  return ((AudioCaptureClient*)client)->getAudioFormat();
}

UINT32 getNextPacketSize(void *client) {
  return ((AudioCaptureClient*)client)->getNextPacketSize();
}

UINT32 getBuffer(void *client, UINT32 expectedFrameCount, UINT32 maximumFrameCount, char *out) {
  return ((AudioCaptureClient*)client)->getBuffer(expectedFrameCount, maximumFrameCount, out);
}

void stopCapture(void *client) {
  ((AudioCaptureClient*)client)->stopCapture();
}