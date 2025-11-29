#include "LittleShifter.h"

void LittleShifter::BlackmanWindow(double* window)
{
  const double a0 = 0.35875;
  const double a1 = 0.48829;
  const double a2 = 0.14128;
  const double a3 = 0.01168;
  double angle_factor = 2.0 * M_PI / (fftFrameSize - 1);

  for (int k = 0; k < fftFrameSize; k++)
  {
    window[k] = a0 
                - a1 * std::cos(1.0 * k * angle_factor)
                + a2 * std::cos(2.0 * k * angle_factor)
                - a3 * std::cos(3.0 * k * angle_factor);
  }
}