#include "softbutton.h"

SoftButton::SoftButton() 
  : current_val_(0), n_ticks_(20), counter_(0), 
    tap_timeout_(-1), tap_counter_(0), state_(KEY_UP), 
    active_type_(ACTIVE_HIGH)
{

}

SoftButton::SoftButton(int n, SoftButton::ActiveLevel lvl) 
  : current_val_(lvl == ActiveLevel::ACTIVE_HIGH ? 0 : 1), n_ticks_(n), counter_(0), 
    tap_timeout_(-1), tap_counter_(0), state_(KEY_UP), active_type_(lvl)
{

}

void SoftButton::set_ticks(int n) 
{ 
  counter_ = 0;
  n_ticks_ = n; 
}

void SoftButton::set_active_level(SoftButton::ActiveLevel lvl) {
  active_type_ = lvl;
  tap_counter_ = 0;
  state_ = active_type_ == current_val_ ? ButtonState::KEY_DOWN : ButtonState::KEY_UP;
}

void SoftButton::update(int val) 
{
  if (val != current_val_) {
    ++counter_;
    if (counter_ >= n_ticks_) {
      // changed from key down to up or vice versa
      current_val_ = val;
      counter_ = 0;
      state_ = active_type_ == current_val_ ? ButtonState::KEY_DOWN : ButtonState::KEY_UP;
      event_ = active_type_ == current_val_ ? ButtonEvent::EVENT_KEY_DOWN : ButtonEvent::EVENT_KEY_UP;
    }
  } else {
    counter_ = 0;
    if ((tap_timeout_ > 0) && (state_ == ButtonState::KEY_DOWN) && (tap_counter_ < tap_timeout_)) {
      ++tap_counter_;
    }
  }
}