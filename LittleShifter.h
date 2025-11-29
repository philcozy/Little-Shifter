#pragma once

#include "IPlug_include_in_plug_hdr.h"

#define M_PI 3.14159265358979323846
#define MAX_FRAME_LENGTH 8192

const int kNumPresets = 1;

enum EParams
{
  kPitchRatio = 0,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

typedef struct RINGBUFFER
{
  int S; // size of buffer
  double m_buffer[MAX_FRAME_LENGTH];
  int m_front; // read pointer
  int m_back; // write pointer
}ringbuffer_t;

class LittleShifter final : public Plugin
{
public:
  LittleShifter(const InstanceInfo& info);
  void OnReset() override;
  void BlackmanWindow(double* window);
  void smbFft(double* fftBuffer, long fftFrameSize, long sign);

  void ringbuffer_clear(ringbuffer_t* buffer, uint32_t size); // initialize ring buffer
  void ringbuffer_push(ringbuffer_t* buffer); // shift write pointer
  void ringbuffer_push_sample(ringbuffer_t* buffer, const double x); // write sample to buffer
  double ringbuffer_front(ringbuffer_t* buffer); // read oldest buffer

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif
private:
  long osamp;
  double sampleRate;
  long fftFrameSize = 2048;
  double window[MAX_FRAME_LENGTH];
  double gInFIFO[MAX_FRAME_LENGTH];
  double gOutFIFO[MAX_FRAME_LENGTH];
  double gFFTworksp[2 * MAX_FRAME_LENGTH];
  double gLastPhase[MAX_FRAME_LENGTH / 2 + 1];
  double gSumPhase[MAX_FRAME_LENGTH / 2 + 1];
  double gAnaFreq[MAX_FRAME_LENGTH];
  double gAnaMagn[MAX_FRAME_LENGTH];
  double gSynFreq[MAX_FRAME_LENGTH];
  double gSynMagn[MAX_FRAME_LENGTH];
  long gRover;
  double magn, phase, tmp, real, imag;
  double freqPerBin, expct;
  long i, k, qpd, index, inFifoLatency, stepSize, fftFrameSize2, read_idx;
  double indexFloat, frac;
  ringbuffer_t gInRingBuffer, gOutputAccum;
};

