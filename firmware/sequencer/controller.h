#ifndef controller_h_
#define controller_h_

#include "machinestate.h"

class Controller
{
public:

  Controller();

  void tick(int ich, uint16_t pot_val, int step_val, int run_val, int mode_val);
  
  uint8_t next() const 
  {
    return (uint8_t)ki_next_;
  }

  int gate() const 
  {
    if (state_.running && state_.step_active[ki_]) {
      return (mi_ < duty_tpi_);
    }
    return 0;
  }

  uint16_t cv() const
  {
    if (state_.running && state_.step_active[ki_]) {
      if (mi_ < slide_tpi_) {   // slide_tpi_ <= tpi_
        return state_.quant_cv(ki_);
      } else {
        return slide_.y;
      }
    } 
    return 0;
  }

  // Duty Cycle
  int duty() const 
  {
    return state_.duty_pct;
  }

  void duty(int d);

  // Slide Range
  int slide() const 
  {
    return state_.slide_pct;
  }

  void slide(int s);

  // Speed/BPM
  void tick_freq(int f)
  {
    state_.tick_freq = f;
    bpm(state_.bpm);  // refresh
  }

  int bpm() const 
  {
    return state_.bpm;
  }

  void bpm(int b);

private:

  MachineState state_;
 
  int tpi_;     // ticks per interval (step)
  int mi_;      // index in interval
  int ki_;      // interval index (0-7)
  int ki_next_;

  int duty_tpi_;        // gate duty cycle in ticks per interval
  int slide_tpi_;  // start of slide in ticks per interval
  
  struct slide_def {
    void reset(int y0, int y1, int deltax) 
    {
      dx = deltax;  // always +ve
      if (y0 > y1) {
        dir = -1;
        dy = y0 - y1;
      } else {
        dir = 1;
        dy = y1 - y0;
      }

      if (dy > dx) {
        D = 2*dx - dy;
      } else {
        D = 2*dy - dx;
      }
      y = y0;
    }
    int dx;
    int dy;
    int dir;
    int D;
    int y;
  } slide_;

};


#endif