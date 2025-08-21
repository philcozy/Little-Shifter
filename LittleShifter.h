#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "RingBuffer.h"
#include <samplerate.h>

const int kNumPresets = 1;

enum EParams
{
  kPitchShift = 0,
  kMix,
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
  /* Ring Buffer */
  RingBuffer mRingBuffer;

  /* About pitchratio if statement */
  double lastPitchRatio = 1.0;

  /* About Grain */
  int mGrainSize = MGRAIN_SIZE;
  int resampledGrainSize = 0;
  int sampleCounter = 0;
  std::vector<float> grainIn;
  std::vector<float> grainOut;

  /* About Hann_Window*/
  std::vector<float> window;

  /* About resampling using libsamplerate library memebers */
  SRC_STATE* mSRCState = nullptr;
  int mSRCError = 0;
  SRC_DATA data;

  /* About Accumulator and output */
  std::vector<float> mAccumulator;
  int writerPos = 0; 
  int readerPos = 0;
  int hopSize;

  /* Process new grain function */
  void processNewGrain();
  void hann_window(std::vector<float>& window, int size);
};
