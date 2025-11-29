#include "LittleShifter.h"
#include "IControls.h"
#include "IPlug_include_in_plug_src.h"


LittleShifter::LittleShifter(const InstanceInfo& info)
  : iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kPitchRatio)->InitDouble("PitchRatio", 1.0, 0.5, 1.5, 0.1);

  fftFrameSize = 2048;
  osamp = 4;
  sampleRate = GetSampleRate();

  fftFrameSize2 = fftFrameSize / 2;
  stepSize = fftFrameSize / osamp;
  freqPerBin = sampleRate / (double)fftFrameSize;
  expct = 2. * M_PI * (double)stepSize / (double)fftFrameSize;
  inFifoLatency = fftFrameSize - stepSize;
  gRover = 0;
  hannwindow(window);

  memset(gInFIFO, 0, MAX_FRAME_LENGTH * sizeof(double));
  memset(gOutFIFO, 0, MAX_FRAME_LENGTH * sizeof(double));
  memset(gFFTworksp, 0, 2 * MAX_FRAME_LENGTH * sizeof(double));
  memset(gLastPhase, 0, (MAX_FRAME_LENGTH / 2 + 1) * sizeof(double));
  memset(gSumPhase, 0, (MAX_FRAME_LENGTH / 2 + 1) * sizeof(double));
  memset(gAnaFreq, 0, MAX_FRAME_LENGTH * sizeof(double));
  memset(gAnaMagn, 0, MAX_FRAME_LENGTH * sizeof(double));

  /* initialize ring buffer */
  ringbuffer_clear(&gInRingBuffer, MAX_FRAME_LENGTH);
  ringbuffer_clear(&gOutputAccum, MAX_FRAME_LENGTH);
  gOutputAccum.m_back = 0; //block processing expects start at 0


#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() { return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT)); };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT b = pGraphics->GetBounds();
    pGraphics->AttachControl(new IVKnobControl(b.GetMidVPadded(50), kPitchRatio));
  };
#endif
}

void LittleShifter::OnReset()
{
  sampleRate = GetSampleRate();
  freqPerBin = sampleRate / (double)fftFrameSize;
  SetLatency(inFifoLatency);
}

#if IPLUG_DSP
void LittleShifter::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double pitchShift = GetParam(kPitchRatio)->Value();
  const int nChans = NOutChansConnected();

  for (i = 0; i < nFrames; i++)
  {
    /* As long as we have not yet collected enough data just read in */
    ringbuffer_push_sample(&gInRingBuffer, inputs[0][i]);

    /* Before the first frame, index might be negative */
    if (gRover < inFifoLatency) 
    {
      outputs[0][i] = 0.0;
    }
    else
    {
      outputs[0][i] = gOutFIFO[gRover - inFifoLatency];
    }

    outputs[1][i] = outputs[0][i];
    gRover++;

    /* now we have enough data for processing */
    if (gRover >= fftFrameSize)
    {
      gRover = inFifoLatency;

      /* do windowing and re,im interleave */
      for (k = 0; k < fftFrameSize; k++)
      {
        read_idx = (gInRingBuffer.m_front + k) % gInRingBuffer.S;
        gFFTworksp[2 * k] = gInRingBuffer.m_buffer[read_idx] * window[k]; // keywords in article: "Interleave", real part
        gFFTworksp[2 * k + 1] = 0.;                                       // imagine part
      }
      gInRingBuffer.m_front = (gInRingBuffer.m_front + stepSize) % gInRingBuffer.S;

      /* ***************** ANALYSIS ******************* */
      /* do transform */
      smbFft(gFFTworksp, fftFrameSize, -1);

      /* this is the analysis step */
      for (k = 0; k <= fftFrameSize2; k++)
      {

        /* de-interlace FFT buffer */
        real = gFFTworksp[2 * k];
        imag = gFFTworksp[2 * k + 1];

        /* compute magnitude and phase */
        magn = 2. * sqrt(real * real + imag * imag);
        phase = atan2(imag, real);

        /* compute phase difference */
        tmp = phase - gLastPhase[k];
        gLastPhase[k] = phase;

        /* subtract expected phase difference */
        tmp -= (double)k * expct;

        /* map delta phase into +/- Pi interval */
        qpd = tmp / M_PI;
        if (qpd >= 0)
          qpd += qpd & 1;
        else
          qpd -= qpd & 1;
        tmp -= M_PI * (double)qpd;

        /* get deviation from bin frequency from the +/- Pi interval */
        tmp = osamp * tmp / (2. * M_PI);

        /* compute the k-th partials' true frequency */
        tmp = (double)k * freqPerBin + tmp * freqPerBin;

        /* store magnitude and true frequency in analysis arrays */
        gAnaMagn[k] = magn;
        gAnaFreq[k] = tmp;
      }

      /* ***************** PROCESSING ******************* */
      /* this does the actual pitch shifting */
      memset(gSynMagn, 0, fftFrameSize * sizeof(double));
      memset(gSynFreq, 0, fftFrameSize * sizeof(double));
      for (k = 0; k <= fftFrameSize2; k++)
      {
        indexFloat = k * pitchShift;
        index = (long)indexFloat;
        frac = indexFloat - index;
        if (index <= fftFrameSize2)
        {
          gSynMagn[index] += gAnaMagn[k] * (1. - frac);
          gSynFreq[index] = gAnaFreq[k] * pitchShift;
          if (index + 1 <= fftFrameSize2)
          {
            gSynMagn[index + 1] += gAnaMagn[k] * frac;
            gSynFreq[index + 1] = gAnaFreq[k] * pitchShift; // both bins should represent the same frequency
          }
        }
      }

      /* ***************** SYNTHESIS ******************* */
      /* this is the synthesis step */
      for (k = 0; k <= fftFrameSize2; k++)
      {

        /* get magnitude and true frequency from synthesis arrays */
        magn = gSynMagn[k];
        tmp = gSynFreq[k];

        /* subtract bin mid frequency */
        tmp -= (double)k * freqPerBin;

        /* get bin deviation from freq deviation */
        tmp /= freqPerBin;

        /* take osamp into account */
        tmp = 2. * M_PI * tmp / osamp;

        /* add the overlap phase advance back in */
        tmp += (double)k * expct;

        /* accumulate delta phase to get bin phase */
        gSumPhase[k] += tmp;
        phase = gSumPhase[k];

        /* get real and imag part and re-interleave */
        gFFTworksp[2 * k] = magn * cos(phase);
        gFFTworksp[2 * k + 1] = magn * sin(phase);
      }

      /* zero negative frequencies */
      for (k = fftFrameSize + 2; k < 2 * fftFrameSize; k++)
        gFFTworksp[k] = 0.;

      /* do inverse transform */
      smbFft(gFFTworksp, fftFrameSize, 1);

      /* do windowing and add to output accumulator */
      for (k = 0; k < fftFrameSize; k++)
      {
        gOutputAccum.m_buffer[(gOutputAccum.m_back + k) % (gOutputAccum.S)] += 2. * window[k] * gFFTworksp[2 * k] / (fftFrameSize2 * osamp);
      }
      gOutputAccum.m_back = (gOutputAccum.m_back + stepSize) % gOutputAccum.S;

      /* We can delete this after ring buffer implementation*/
      for (k = 0; k < stepSize; k++)
      {
        gOutFIFO[k] = gOutputAccum.m_buffer[(gOutputAccum.m_front + k) % (gOutputAccum.S)];
        gOutputAccum.m_buffer[(gOutputAccum.m_front + k) % (gOutputAccum.S)] = 0.0; // clear after reading
      }
      gOutputAccum.m_front = (gOutputAccum.m_front + stepSize) % gOutputAccum.S;
    }
  }
}
#endif
