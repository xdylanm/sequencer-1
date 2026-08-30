#include "controller.h"


Controller::Controller() 
  : tpi_(0), mi_(0), duty_tpi_(0), slide_tpi_(0)
{

}

bool Controller::tick(MachineState& state) 
{
  bool new_interval = false;
  if (state.running()) {
    ++mi_;
    if (mi_ >= tpi_) {
      state.advance_step();
      mi_ = 0;
      seq_params_.ki = state.current_step();
      new_interval = true;
    }

    // https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
    if (mi_ == slide_tpi_) {
      uint16_t const y0 = state.quant_cv();
      uint16_t const y1 = state.quant_cv(true);
      slide_.reset(y0, y1, tpi_ - slide_tpi_);
    } else if (mi_ > slide_tpi_) {
      if (slide_.dy > slide_.dx) {
        while (slide_.D < 0) {
          slide_.D += 2*slide_.dx;
          slide_.y += slide_.dir;
        }
        slide_.y += slide_.dir;
        slide_.D += 2*(slide_.dx - slide_.dy);
      } else {
        if (slide_.D > 0) {
          slide_.y += slide_.dir;
          slide_.D += 2*(slide_.dy - slide_.dx);
        } else {
          slide_.D += 2*slide_.dy;
        }
      }
    }
  } 

  return new_interval;
}

void Controller::update_parameters(MachineState const& state)
{
  bool update_slide = false;
  if (state.current_step() != seq_params_.ki) {
    mi_ = 0;
    seq_params_.ki = state.current_step();
    update_slide = true;
  }
 
  int const b = state.bpm();
  int const ft = state.tick_freq();
  if ((b != seq_params_.bpm) || (ft != seq_params_.f_tick)) {
  // mi_new/tpi_new = mi_old/tpi_old
    int const old_tpi = tpi_;
    tpi_ = (60 * ft + (b / 2)) / b; // rounded integer division
    if (tpi_ != old_tpi) {
      mi_ = (tpi_ * mi_ + (old_tpi / 2)) / old_tpi;
      slide_tpi_ = tpi_ - (tpi_ * seq_params_.slide_pct + 50) / 100;  // rounded integer divsion   
      update_slide = true;
    }
    seq_params_.bpm = b;
    seq_params_.f_tick = ft;
  }
  
  // slide doesn't change y, slope just stretches
  int const sp = state.slide_pct();
  if (sp != seq_params_.slide_pct) {
    slide_tpi_ = tpi_ - (tpi_ * sp + 50) / 100;  // rounded integer divsion   
    update_slide = true;
    seq_params_.slide_pct = sp;
  }

  // reset slide if required
  if (update_slide && (mi_ >= slide_tpi_)) {
    slide_.reset(slide_.y, state.quant_cv(true), tpi_ - mi_);
  }

  // update duty tpi
  int const dp = state.duty_pct();
  if (dp != seq_params_.duty_pct) {
    duty_tpi_ = (tpi_ * state.duty_pct() + 50) / 100;  // rounded integer divsion
    seq_params_.duty_pct = dp;
  }
}

