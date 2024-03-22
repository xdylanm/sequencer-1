#ifndef machinestate_h_
#define machinestate_h_

#include <Arduino.h>
#include "softbutton.h"

#define MAX_NUM_STEPS 8

class MachineState
{
public:

  enum Quantization {NONE, MAJOR, MINOR, CHROMATIC, NUM_QUANTIZATION};
  enum PatternMode {LOOP, BOUNCE, RANDOM, NUM_PATTERNS};
  enum OutputRange {VOCT_1=1, VOCT_2=2, VOCT_5=5};
  enum StepButtonMode {STEP_ACTIVE, STEP_ENABLE, NUM_STEP_BUTTON_MODES}; // STEP_SLIDE

  MachineState();

  Quantization quant;
  OutputRange voct_range;
  PatternMode pattern;
  StepButtonMode step_button_mode;
  
  SoftButton step_button[MAX_NUM_STEPS];
  SoftButton run_button;
  SoftButton mode_button;

  bool running;

  uint8_t step_active[MAX_NUM_STEPS];
  uint8_t step_enable[MAX_NUM_STEPS];

  int tick_freq;   // frequency of event loop (1/T_conv)
  int bpm;         // 24-240
  int duty_pct;    // 0-100
  int slide_pct;   // 0-100

  void push(int ich, uint16_t pot_val, int step_val, int run_val, int mode_val);

  int next_step(int ki);
  uint16_t quant_cv(int i) const;

  void process_key_events();


private:

  int bounce_dir_;    // bounce direction for bounce pattern
  uint32_t r_state_;  // state for random interval counter
  int octave_shift_;  // transpose by octave
  
  uint16_t cv_[MAX_NUM_STEPS];    


};


#endif