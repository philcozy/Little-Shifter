#include "LittleShifter.h"

void LittleShifter::ringbuffer_clear(ringbuffer_t* buffer, uint32_t size)
{
  buffer->S = size;

  int q = 0;
  for (q = 0; q < size; q++)
  {
    buffer->m_buffer[q] = 0;
  }

  buffer->m_front = 0;
  buffer->m_back = buffer->S - 1;
}
void LittleShifter::ringbuffer_push(ringbuffer_t* buffer) { buffer->m_back = (buffer->m_back + 1) % buffer->S; }
void LittleShifter::ringbuffer_push_sample(ringbuffer_t* buffer, const double x)
{
  ringbuffer_push(buffer);
  buffer->m_buffer[buffer->m_back] = x;
}
double LittleShifter::ringbuffer_front(ringbuffer_t* buffer) { return buffer->m_buffer[buffer->m_front]; }