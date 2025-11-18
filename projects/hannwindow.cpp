#include "LittleShifter.h"

void LittleShifter::hannwindow(double* window)
{
  for (int k = 0; k < fftFrameSize; k++)
    window[k] = -.5 * cos(2. * M_PI * (double)k / (double)fftFrameSize) + .5;
}