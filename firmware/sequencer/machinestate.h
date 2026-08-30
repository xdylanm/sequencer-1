#ifndef machinestate_h_
#define machinestate_h_

#include <Arduino.h>
#include "softbutton.h"

#define MAX_NUM_STEPS 8
#define INVALIDATE_NPXLS 0x01
#define INVALIDATE_OLED  0x02

class MachineState
{
public:

  enum Quantization {NONE, MAJOR, MINOR, CHROMATIC, NUM_QUANTIZATION};
  enum PatternMode {LOOP, BOUNCE, RANDOM, NUM_PATTERNS};
  enum OutputRange {VOCT_1=1, VOCT_2=2, VOCT_5=5};
  enum StepButtonMode {STEP_ACTIVE, STEP_ENABLE, NUM_STEP_BUTTON_MODES}; // STEP_SLIDE
  struct TopMenuState
  {
    TopMenuState() : active_menu_item(-1), editing(false) {}
    int active_menu_item;
    bool editing;
  };

  MachineState();

  bool running() const { return running_; }

  Quantization quant() const { return quant_; }
  OutputRange voct_range() const { return voct_range_; }
  PatternMode pattern() const { return pattern_; }
  StepButtonMode step_button_mode() const { return step_button_mode_; }

 // Duty Cycle
  int duty_pct() const { return duty_pct_; }

  // Slide Range
  int slide_pct() const { return slide_pct_; }

  // Speed/BPM
  void tick_freq(int f)  { tick_freq_ = f; }
  int  tick_freq() const { return tick_freq_; }
  int bpm() const { return bpm_; }

  // Octave shift
  int octave_shift() const { return octave_shift_; }

  void push(int ich, uint16_t pot_val, int step_val);
  uint8_t process_key_events(int run_val, int mode_val, int rot_sw_val, int rot_pos);        // process keys and update state

  int advance_step();            // advance to ki, compute next ki
  uint16_t quant_cv(bool at_next = false) const;   // report quantized CV

  int current_step() const { return ki_; }
  bool current_step_active() const { return step_active_[ki_]; }
  bool current_step_enabled() const { return step_enable_[ki_]; }


  uint32_t pixel_color(int i) const 
  {
    if (i >= 0 && i < MAX_NUM_STEPS) {
      return pixel_wrgb_[i];
    }
    return 0;
  }  

  void set_rotary_position(int pos) { rotary_pos_ = pos; }

  TopMenuState const& menu_state() const { return menu_state_; }

  uint16_t const* cv_buf() const { return cv_; }
  uint8_t const* step_active_buf() const { return step_active_; }
  uint8_t const* step_enable_buf() const { return step_enable_; }

private:

  TopMenuState menu_state_;

  Quantization quant_;
  OutputRange voct_range_;
  PatternMode pattern_;
  StepButtonMode step_button_mode_;
  
  SoftButton step_button_[MAX_NUM_STEPS];
  SoftButton run_button_;
  SoftButton mode_button_;
  SoftButton rotary_button_;

  bool running_;

  int ki_;      // interval index (0-7)
  int ki_next_;

  uint8_t step_active_[MAX_NUM_STEPS];
  uint8_t step_enable_[MAX_NUM_STEPS];

  int tick_freq_;  // frequency of event loop (1/T_conv)
  int bpm_;        // 24-240
  int duty_pct_;   // 0-100
  int slide_pct_;  // 0-100

  int bounce_dir_;    // bounce direction for bounce pattern
  uint32_t r_state_;  // state for random interval counter
  int octave_shift_;  // transpose by octave
  int rotary_pos_;    // current position of the rotary
  
  uint16_t cv_[MAX_NUM_STEPS];   
  uint32_t pixel_wrgb_[MAX_NUM_STEPS]; 

  void process_run_button();
  uint8_t process_mode_button();
  uint8_t process_step_buttons();
  uint8_t process_rotary(int new_pos);

  void update_npxls();

};


#endif