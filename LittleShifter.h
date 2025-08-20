#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "RingBuffer.h"

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

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif

private:
  /* Data Members */
  RingBuffer mRingBuffer;
  double pitchRatio = 1.2;
  int mGrainSize = 512;
  int resampledGrainSize = mGrainSize * pitchRatio + 16;
  SRC_STATE* mSRCState = nullptr;
  int mSRCError = 0;
  int hopSize = 512 * 0.5;

  /* Member Functions*/
};
