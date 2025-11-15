#pragma once

#include "IPlug_include_in_plug_hdr.h"

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
  void hannwindow();

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif
private:
  double pitchRatio = 1.0;
  double inQueue[M_FRAME_LENGTH];
  int frameSize = FRAMESIZE;
  int hopSize = FRAMESIZE / OSAMP;
  double window[FRAMESIZE];
  int inFIFOLatency = FRAMESIZE - hopSize;
  int writeIDX = inFIFOLatency;
  int i, j;
};

void LittleShifter::hannwindow()
{
  for (int k = 0; k < frameSize; k++)
    window[k] = -.5 * cos(2. * M_PI * (double)k / (double)frameSize) + .5;
}
