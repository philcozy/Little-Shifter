#pragma once

#include "IPlug_include_in_plug_hdr.h"

#define M_PI 3.14159265358979323846
#define MAX_FRAME_LENGTH 8192

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class LittleShifter final : public Plugin
{
public:
  LittleShifter(const InstanceInfo& info);
  void hannwindow(double* window);
  void smbFft(double* fftBuffer, long fftFrameSize, long sign);

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif
private:
  double pitchShift = 1.5;
  long osamp;
  double sampleRate;
  long fftFrameSize = 2048;
  double window[MAX_FRAME_LENGTH];
  double gInFIFO[MAX_FRAME_LENGTH];
  double gOutFIFO[MAX_FRAME_LENGTH];
  double gFFTworksp[2 * MAX_FRAME_LENGTH];
  double gLastPhase[MAX_FRAME_LENGTH / 2 + 1];
  double gSumPhase[MAX_FRAME_LENGTH / 2 + 1];
  double gOutputAccum[2 * MAX_FRAME_LENGTH];
  double gAnaFreq[MAX_FRAME_LENGTH];
  double gAnaMagn[MAX_FRAME_LENGTH];
  double gSynFreq[MAX_FRAME_LENGTH];
  double gSynMagn[MAX_FRAME_LENGTH];
  long gRover;
  double magn, phase, tmp, real, imag;
  double freqPerBin, expct;
  long i, k, qpd, index, inFifoLatency, stepSize, fftFrameSize2;
};

