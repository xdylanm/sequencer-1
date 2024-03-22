#include "controller.h"


Controller::Controller() 
  : tpi_(0), mi_(0), ki_(0), ki_next_(1), duty_tpi_(0), slide_tpi_(0)
{

}

void Controller::tick(int ich, uint16_t pot_val, int step_val, int run_val, int mode_val) 
{
  if (state_.running) {
    ++mi_;
    if (mi_ >= tpi_) {
      ki_ = ki_next_; 
      ki_next_ = state_.next_step(ki_next_);
      mi_ = 0;
    }

    // https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
    if (mi_ == slide_tpi_) {
      uint16_t const y0 = state_.quant_cv(ki_);
      uint16_t const y1 = state_.quant_cv(ki_next_);
      slide_.reset(y0, y1, tpi_ - slide_tpi_);
    } else if (mi_ > slide_tpi_) {
      if (slide_.dy > slide_.dx) {
        while (slide_.D <= 0) {
          slide_.D += 2*slide_.dx;
          slide_.y += slide_.dir;
        }
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

  state_.push(ich, pot_val, step_val, run_val, mode_val);
}

void Controller::duty(int d) 
{
  if (d < 0 || d > 100) {
    return;
  }
  state_.duty_pct = d;
  duty_tpi_ = (tpi_ * d + 50) / 100;  // rounded integer divsion
}

void Controller::slide(int s) 
{
  if (s < 0 || s > 100) {
    return;
  }
  state_.slide_pct = s;
  slide_tpi_ = tpi_ - (tpi_ * s + 50) / 100;  // rounded integer divsion   

  // reset 
  if (mi_ >= slide_tpi_) {
    slide_.reset(slide_.y, state_.quant_cv(ki_next_), tpi_ - mi_);
  }
}

void Controller::bpm(int b) 
{
  if (b < 24 || b > 240) {
    return;
  }
  state_.bpm = b;
  // mi_new/tpi_new = mi_old/tpi_old
  int const old_tpi = tpi_;
  tpi_ = (60 * state_.tick_freq + (b >> 1)) / b; // rounded integer division
  mi_ = (tpi_ * mi_ + (old_tpi >> 1)) / old_tpi;
  
  // reset duty & slide
  slide(state_.slide_pct);  // slide doesn't change y, slope just stretches
  duty(state_.duty_pct);
}


