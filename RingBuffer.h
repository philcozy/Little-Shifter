#pragma once
#include <vector>

class RingBuffer
{
public:
  RingBuffer(int capacity)
    : rbuffr(capacity, 0.0f), rCapacity(capacity){}

  void write(double x)
  {
    rbuffr[rWriteIndex] = x;
    rWriteIndex = (rWriteIndex + 1) % rCapacity;
  }

  void writeBlock(const double* in, int n)
  {
    for (int i = 0; i < n; i++)
      write(in[i]);
  }

  void readBlock(float* out, int n, int readIndex)
  {
    for (int i = 0; i < n; i++)
    {
      int idx = (readIndex + i) % rCapacity;
      out[i] = static_cast<float>(rbuffr[idx]);
    }
  }

  int getReadIndex(int delay)
  {
    int idx = rWriteIndex - delay;
    if (idx < 0)
      idx += rCapacity;
    return idx;
  }

private:
  std::vector<double> rbuffr;
  int rCapacity;
  int rWriteIndex = 0;
};