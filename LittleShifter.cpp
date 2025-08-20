#include "LittleShifter.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include <samplerate.h>

LittleShifter::LittleShifter(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets)), mRingBuffer(CAPACITY)
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT b = pGraphics->GetBounds();
    pGraphics->AttachControl(new ITextControl(b.GetMidVPadded(50), "Hello iPlug 2!", IText(50)));
    pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(100).GetVShifted(-100), kGain));
  };
#endif

  mSRCState = src_new(SRC_SINC_FASTEST, 1, &mSRCError);
  if (!mSRCState)
  {
    DBGMSG("libsamplerate init failed: %s\n", src_strerror(mSRCError));
  }
}

#if IPLUG_DSP
void LittleShifter::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();

  mRingBuffer.writeBlock(inputs[0], nFrames);
  /* PreProcessing...*/
  std::vector<float> grainIn(mGrainSize);
  std::vector<float> grainOut(resampledGrainSize);
  std::vector<float> window(resampledGrainSize);

  for (int n = 0; n < resampledGrainSize; ++n)
    window[n] = 0.5f - 0.5f * cos(2.0f * M_PI * n / (resampledGrainSize - 1));
  /* End PreProcessing*/

  int readIndex = mRingBuffer.getReadIndex(mGrainSize);
  mRingBuffer.readBlock(&grainIn[0], mGrainSize, readIndex);

  SRC_DATA data;
  data.data_in = grainIn.data();
  data.input_frames = mGrainSize;
  data.data_out = grainOut.data();
  data.output_frames = resampledGrainSize;
  data.src_ratio = 1.0 / pitchRatio;
  src_process(mSRCState, &data);

  std::vector<float> mAccumulator(CAPACITY); //if we initialize only one time, remember to (CAPACITY, 0.0f)
  int writerPos = 0;
  int readerPos = 0;

  for (int i = 0; i < resampledGrainSize; i++)
  {
    int idx = writerPos + i % mAccumulator.size();
    mAccumulator[idx] += grainOut[i] * window[i];
  }
  writerPos = (writerPos + hopSize) % mAccumulator.size();

  
  for (int s = 0; s < nFrames; s++)
  {
    outputs[0][s] = mAccumulator[readerPos];
    mAccumulator[readerPos] = 0;
    readerPos = (readerPos + 1) % mAccumulator.size();
  }

}
#endif
