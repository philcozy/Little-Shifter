#include "LittleShifter.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

LittleShifter::LittleShifter(const InstanceInfo& info)
  : iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
  , mRingBuffer(CAPACITY)
  , mAccumulator(CAPACITY, 0.0f)
{
  GetParam(kPitchShift)->InitDouble("Pitch Shift", 1.0, 0.5, 2.0, 0.01, "x");
  GetParam(kMix)->InitDouble("Mix", 1.0, 0.0, 1.0, 0.01, "");

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);

    const IRECT b = pGraphics->GetBounds();
    const IRECT knob1Bounds = b.GetGridCell(0, 2, 2).GetCentredInside(100);
    const IRECT knob2Bounds = b.GetGridCell(1, 2, 2).GetCentredInside(100);

    pGraphics->AttachControl(new IVKnobControl(knob1Bounds, kPitchShift));
    pGraphics->AttachControl(new IVKnobControl(knob2Bounds, kMix));
  };
#endif

  mSRCState = src_new(SRC_SINC_FASTEST, 1, &mSRCError);
  if (!mSRCState)
  {
    DBGMSG("libsamplerate init failed: %s\n", src_strerror(mSRCError));
  }

  data.input_frames = mGrainSize;
  grainIn.resize(mGrainSize);
  grainOut.resize(MAX_GRAIN_SIZE);
  window.resize(MAX_GRAIN_SIZE);
  hopSize = mGrainSize * 0.5;
}

#if IPLUG_DSP
void LittleShifter::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  /* Gets value from knobs */
  const double pitchRatio  = GetParam(kPitchShift)->Value();
  const double wet = GetParam(kMix)->Value();
  const double dry = 1.0 - wet;
  const int nChans = NOutChansConnected();

  /* Write into RingBuffer first*/
  mRingBuffer.writeBlock(inputs[0], nFrames);

  /* Changing value only if pitchratio changed */
  if (pitchRatio != lastPitchRatio)
  {
    /* About grains */
    resampledGrainSize = static_cast<int>(mGrainSize * pitchRatio + 16);
    fill(grainOut.begin(), grainOut.begin() + resampledGrainSize, 0.0f);

    /* About Hann_Window */
    hann_window(window, resampledGrainSize);

    /* About resampling using libsamplerate library memebers */
    data.data_out = grainOut.data();
    data.output_frames = resampledGrainSize;
    data.src_ratio = 1.0 / pitchRatio;

    lastPitchRatio = pitchRatio;
  }

  /* Process each sample */
  for (int s = 0; s < nFrames; s++)
  {
    if (sampleCounter >= hopSize)
    {
      processNewGrain();
      sampleCounter = 0;
    }

    /* Write into ouput from mAccumulator*/
    outputs[0][s] = inputs[0][s] * dry + mAccumulator[readerPos] * wet;
    outputs[1][s] = outputs[0][s];
    mAccumulator[readerPos] = 0;
    readerPos = (readerPos + 1) % mAccumulator.size();

    sampleCounter++;
  }
}
#endif

/* get called only when pitchratio changed, so we don't have to recalculate everytime*/
void LittleShifter::hann_window(std::vector<float>& window, int size)
{
  for (int n = 0; n < size; ++n)
    window[n] = 0.5f - 0.5f * cos(2.0f * M_PI * n / (size - 1));
}

void LittleShifter::processNewGrain()
{
  /* Read from RingBuffer */
  int readIndex = mRingBuffer.getReadIndex(mGrainSize);
  mRingBuffer.readBlock(&grainIn[0], mGrainSize, readIndex);

  /* Resampling */
  data.data_in = grainIn.data();
  src_process(mSRCState, &data);

  /* Write and Overlaps in mAccumulator from grainOut  */
  for (int i = 0; i < resampledGrainSize; i++)
  {
    int idx = (writerPos + i) % mAccumulator.size();
    mAccumulator[idx] += grainOut[i] * window[i];
  }
  writerPos = (writerPos + hopSize) % mAccumulator.size();
}