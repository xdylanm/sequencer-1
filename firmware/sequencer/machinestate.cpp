#include "machinestate.h"
#include "board_target.h"

#define WHITE_BYTE_POS 24
#define RED_BYTE_POS 16
#define GREEN_BYTE_POS 8
#define BLUE_BYTE_POS 0

namespace {
  void xorshift32(uint32_t& x) 
  {
    /* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
  }
}

MachineState::MachineState()
: quant_(Quantization::NONE), voct_range_(OutputRange::VOCT_1), pattern_(PatternMode::LOOP),
  step_button_mode_(StepButtonMode::STEP_ACTIVE), run_button_(32, SoftButton::ACTIVE_LOW), 
  mode_button_(32, SoftButton::ACTIVE_LOW), rotary_button_(32, SoftButton::ACTIVE_LOW), 
  running_(false), tick_freq_(2000), bpm_(120), duty_pct_(50), slide_pct_(0), bounce_dir_(1),
  r_state_(2), octave_shift_(0)
{

  for (int i = 0; i < MAX_NUM_STEPS; ++i) {
    step_button_[i].set_ticks(4);    // time = MAX_NUM_STEPS/tick_freq
    step_button_[i].set_active_level(SoftButton::ACTIVE_LOW);
    step_active_[i] = 1;
    step_enable_[i] = 1;
    cv_[i] = 0;    
    pixel_wrgb_[i] = 0ul;
  }
}

void MachineState::push(int ich, uint16_t pot_val, int step_val)
{
  step_button_[ich].update(step_val);
  cv_[ich] = pot_val;
}


int MachineState::advance_step() 
{
  ki_ = ki_next_;

  for (int i = 0; i < MAX_NUM_STEPS; ++i) {
    pixel_wrgb_[i] &= 0x00FFFFFF;   // mask off white
  }
  pixel_wrgb_[ki_] |= (0x08 << WHITE_BYTE_POS);

  int safety_counter = 0;
  switch (pattern_) {
  case LOOP:
    do {
      ki_next_ = (ki_next_ + 1) % MAX_NUM_STEPS;
      safety_counter++;
    } while(!step_enable_[ki_next_] && (safety_counter < MAX_NUM_STEPS));
    break;
  case BOUNCE:
    do {
      ki_next_ += bounce_dir_;
      if (ki_next_ == (MAX_NUM_STEPS - 1)) {
        bounce_dir_ = -1;
      } else if (ki_next_ == 0) {
        bounce_dir_ = 1;
      }
      safety_counter++;
    } while(!step_enable_[ki_next_] && (safety_counter < MAX_NUM_STEPS));
    break;
  case RANDOM:
    do { 
      xorshift32(r_state_);
      ki_next_ = 0x00000007 & r_state_;
      safety_counter++;
    } while(!step_enable_[ki_next_] && (safety_counter < MAX_NUM_STEPS));
    break;
  }
  
  return ki_next_;
}

uint16_t MachineState::quant_cv(bool const at_next /*=false*/) const 
{
  // range compression -- divide the range into 5 blocks, scale & shift
  // Voct1: block 2
  // Voct2: block 2, 3
  // Voct5: block 0-4

  // scale block ranges for the number of bits of resolution in the DAC
  // baseline is a 10-bit DAC (1024 values)
  // 1024/5, 4 bits remain (2 top & bottom) == 17*12 (!!)
  int const dac_bit_scale = DAC_NUM_BITS - 10;
  int32_t const block_size = 204 << dac_bit_scale;   
  int32_t const base_offset = 2 << dac_bit_scale;
  int32_t const semitone_width = 17 << dac_bit_scale;

  int32_t const v = cv_[at_next ? ki_next_ : ki_];  // 12 bit ADC
  int32_t vo = (v * block_size * (uint32_t)voct_range_ + 2048) / 4096;  // scale to DAC block size (V/OCT), integer rounding

  // quantization
  if (quant_ != Quantization::NONE) {
    int semitone = vo / semitone_width; // 0-16 = 0, 17-33=1, ... (don't round to keep full range)
    // since vo is truncated, it's always closer to the semitone above: --|-----|-*---|--
    if ((quant_ == Quantization::MAJOR) && (semitone == 1 || semitone == 3 || semitone == 6 || semitone == 8 || semitone == 10)) {
      semitone += 1;
    } else if ((quant_ == Quantization::MINOR) && (semitone == 1 || semitone == 4 || semitone == 6 || semitone == 9 || semitone == 11)) {
      semitone += 1;
    }
    vo = semitone * semitone_width;
  }
  
  int32_t block_offset = base_offset + (voct_range_ == VOCT_5 ? 0 : 2*block_size);
  block_offset += octave_shift_ * block_size;   // +/- 2
  vo += block_offset;
  if (vo >= (1 << DAC_NUM_BITS)) { 
    return (1 << DAC_NUM_BITS) - 1;
  } else if (vo < 0) {
    return 0;
  }
  return (uint16_t)vo;  
}

void MachineState::update_npxls()
{
  for (int i = 0; i < MAX_NUM_STEPS; ++i) {
    if (running_ && step_enable_[i]) {
      uint8_t const val = step_active_[i] ? 0x08 : 0x03;
      pixel_wrgb_[i] = (val << RED_BYTE_POS) | (val << BLUE_BYTE_POS);
    } else {
      pixel_wrgb_[i] = 0;
    }
  }
}

//TODO
struct key_states_def
{
  enum ActiveModifier {NONE, RUNSTOP, MODE};
  ActiveModifier modifier;
  bool applied_runstop_modifier;
  bool applied_mode_modifier;
} key_states;


uint8_t MachineState::process_key_events(int run_val, int mode_val, int rot_sw_val, int rot_pos)
{
  run_button_.update(run_val);
  mode_button_.update(mode_val);
  rotary_button_.update(rot_sw_val);
  
  uint8_t invalidate = 0;
  
  process_run_button();
  invalidate |= process_mode_button();
  invalidate |= process_step_buttons();
  invalidate |= process_rotary(rot_pos);

  return invalidate;

}

void MachineState::process_run_button() 
{
  SoftButton::ButtonEvent const run_event = run_button_.event();
  if (run_event == SoftButton::EVENT_KEY_DOWN) {
    key_states.modifier = key_states_def::RUNSTOP;
  } else if (run_event == SoftButton::EVENT_KEY_UP) {
    if (!key_states.applied_runstop_modifier) { // tap
      running_ = !running_; // toggle run/pause if tap
      update_npxls();
      if (running_) {
        r_state_ = micros(); 
        if (r_state_ == 0) { 
          r_state_ = 2;
        }
      }
    }
    if (key_states.modifier == key_states_def::RUNSTOP) { // remove modifier
      key_states.modifier = key_states_def::NONE;
      key_states.applied_runstop_modifier = false;
    }
  }
}

uint8_t MachineState::process_mode_button() 
{
  uint8_t invalidate = 0;
  SoftButton::ButtonEvent const mode_event = mode_button_.event();
  if (mode_event == SoftButton::EVENT_KEY_DOWN) {
    key_states.modifier = key_states_def::MODE;
  } else if (mode_event == SoftButton::EVENT_KEY_UP) {
    if (!key_states.applied_mode_modifier) { // tap
      if (step_button_mode_ == StepButtonMode::STEP_ACTIVE) {
        step_button_mode_ = StepButtonMode::STEP_ENABLE;
      } else if (step_button_mode_ == StepButtonMode::STEP_ENABLE) {
        step_button_mode_ = StepButtonMode::STEP_ACTIVE;
      }
      invalidate |= INVALIDATE_OLED;
    }
    if (key_states.modifier == key_states_def::MODE) {  // remove modifier
      key_states.modifier = key_states_def::NONE;
      key_states.applied_mode_modifier = false;
    }
  }
  return invalidate;

}

uint8_t MachineState::process_step_buttons() 
{
  uint8_t invalidate = 0;

  for (int i = 0; i < MAX_NUM_STEPS; ++i) {
    SoftButton::ButtonEvent e = step_button_[i].event();  // consume
    if (e == SoftButton::EVENT_KEY_DOWN) {
      switch (key_states.modifier) {
        case key_states_def::RUNSTOP:
          if (step_active_[i] && step_enable_[i]) {
            ki_next_ = i;
            advance_step();
            invalidate |= INVALIDATE_NPXLS;
          }
          key_states.applied_runstop_modifier = true;   // avoid tap events on modifiers
          break;
        case key_states_def::MODE:
          switch (i) {
            case 0: // octave -
              if (octave_shift_ > -2) {
                --octave_shift_;
              }
              break;
            case 1: // octave +
              if (octave_shift_ < 2) {
                ++octave_shift_;
              }
              break;
            case 2: // linear
              quant_ = Quantization::NONE;
              break;
            case 3: // major
              quant_ = Quantization::MAJOR;
              break;
            case 4: // minor
              quant_ = Quantization::MINOR;
              break;
            case 5: // chromatic
              quant_ = Quantization::CHROMATIC;
              break;  
            case 6: // change pattern
              pattern_ = (PatternMode)((pattern_ + 1) % PatternMode::NUM_PATTERNS);
              break;
            case 7: // change voct range
              if (voct_range_ == OutputRange::VOCT_1) {
                voct_range_ = OutputRange::VOCT_2;
              } else if (voct_range_ == OutputRange::VOCT_2) {
                voct_range_ = OutputRange::VOCT_5;
              } else {
                voct_range_ = OutputRange::VOCT_1;
              } 
              break;
          } 
          key_states.applied_mode_modifier = true;   // avoid tap events on modifiers
          invalidate |= INVALIDATE_OLED;
          break;
        case key_states_def::NONE:
        default:
          switch (step_button_mode_) {
            case STEP_ACTIVE:
              step_active_[i] = step_active_[i] == 0 ? 1 : 0;
              break;
            case STEP_ENABLE:
              step_enable_[i] = step_enable_[i] == 0 ? 1 : 0;
              break;
          }
          update_npxls();
          invalidate |= INVALIDATE_NPXLS;
          break;
          
      }
    }
  }
  return invalidate;
}

namespace 
{
void adjust_setting(int& setting, int const delta, int const setting_min, int const setting_max)
{
  setting += delta;
  if (setting < setting_min) {
    setting = setting_min;
  } else if (setting > setting_max) {
    setting = setting_max;
  }
}

void adjust_setting_loop(int& setting, int const delta, int const setting_min, int const setting_max)
{
  int const setting_range = setting_max - setting_min + 1;
  setting += delta;
  while (setting > setting_max) {
    setting -= setting_range;
  }
  while (setting < setting_min) {
    setting += setting_range;
  }
}

}

uint8_t MachineState::process_rotary(int const new_pos)
{
  uint8_t invalidate = 0;

  SoftButton::ButtonEvent const rot_btn_event = rotary_button_.event();
  if ((menu_state_.active_menu_item >= 0) && (rot_btn_event == SoftButton::EVENT_KEY_DOWN)) {
    menu_state_.editing = !menu_state_.editing;
    invalidate |= INVALIDATE_OLED;
  }

  int const rot_change = new_pos - rotary_pos_;
  if (new_pos != rotary_pos_) {
    
    if (!menu_state_.editing) {
      adjust_setting_loop(menu_state_.active_menu_item, rot_change, -1, 6);
    } else {
      int current_voct = 0;
      if (voct_range_ == OutputRange::VOCT_2) {
        current_voct = 1;
      } else if (voct_range_ == OutputRange::VOCT_5) {
        current_voct = 2;
      } else {
        current_voct = 0;
      }
      int current_pattern = (int)pattern_;
      int current_quant = (int)quant_;
      int current_mode_sel = (int)step_button_mode_;
      switch (menu_state_.active_menu_item) 
      {
        case 0: // BPM
          adjust_setting(bpm_, rot_change, 24, 240);
          break;
        case 1: // Duty
          adjust_setting(duty_pct_, rot_change, 0, 100);
          break;
        case 2: // Slide
          adjust_setting(slide_pct_, rot_change, 0, 100);
          break;
        case 3: // Pattern
          adjust_setting_loop(current_pattern, rot_change, 0, PatternMode::NUM_PATTERNS-1);
          pattern_ = (PatternMode)(current_pattern);
          break;
        case 4: // Quantization
          adjust_setting_loop(current_quant, rot_change, 0, Quantization::NUM_QUANTIZATION-1);
          quant_ = (Quantization)(current_quant);
          break;
        case 5: // Step button mode
          adjust_setting_loop(current_mode_sel, rot_change, 0, StepButtonMode::NUM_STEP_BUTTON_MODES-1);
          step_button_mode_ = (StepButtonMode)(current_mode_sel);
          break;
        case 6: // V/Oct range (1, 2, 5)
          adjust_setting_loop(current_voct, rot_change, 0, 2);
          if (current_voct == 0) {
            voct_range_ = OutputRange::VOCT_1;
          } else if (current_voct == 1) {
            voct_range_ = OutputRange::VOCT_2;
          } else if (current_voct == 2) {
            voct_range_ = OutputRange::VOCT_5;
          }  
          break;
        default: 
          break;
      }
    }
    rotary_pos_ = new_pos;
    invalidate |= INVALIDATE_OLED;
  }  

  return invalidate;

}



