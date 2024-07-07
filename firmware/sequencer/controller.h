#ifndef controller_h_
#define controller_h_

#include "machinestate.h"

class Controller
{
public:

  Controller();

  bool tick(MachineState& state);
 
  int gate(MachineState const& state) const 
  {
    if (state.running() && state.current_step_active()) {
      return (mi_ < duty_tpi_);
    }
    return 0;
  }

  uint16_t cv(MachineState const& state) const
  {
    if (state.running() && state.current_step_active()) {
      if (mi_ < slide_tpi_) {   // slide_tpi_ <= tpi_
        return state.quant_cv();
      } else {
        return slide_.y;
      }
    } 
    return 0;
  }

  void update_parameters(MachineState const& state);

private:

  int tpi_;     // ticks per interval (step)
  int mi_;      // index in interval

  int duty_tpi_;        // gate duty cycle in ticks per interval
  int slide_tpi_;  // start of slide in ticks per interval
  
  struct slide_def {
    void reset(int y0, int y1, int deltax) 
    {
      dx = deltax;  // always +ve
      if (y0 > y1) {
        dir = -1;   // yi
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

  struct CurrentSeqParams {
    CurrentSeqParams() : f_tick(-1), bpm(-1), duty_pct(-1), slide_pct(-1) {}
    int f_tick;
    int bpm;
    int duty_pct;
    int slide_pct;
    int ki; 
  }  seq_params_;

};


#endif