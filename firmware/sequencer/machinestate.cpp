#include "machinestate.h"

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
: quant(Quantization::NONE), voct_range(OutputRange::VOCT_1), pattern(PatternMode::LOOP),
  step_button_mode(StepButtonMode::STEP_ACTIVE), run_button(32, SoftButton::ACTIVE_LOW), 
  mode_button(32, SoftButton::ACTIVE_LOW), running(false), tick_freq(2000), bpm(60), 
  duty_pct(50), slide_pct(0), bounce_dir_(1), r_state_(2), octave_shift_(0)
{

  for (int i = 0; i < MAX_NUM_STEPS; ++i) {
    step_button[i].set_ticks(4);    // time = MAX_NUM_STEPS/tick_freq
    step_button[i].set_active_level(SoftButton::ACTIVE_LOW);
    step_active[i] = 1;
    step_enable[i] = 1;
    cv_[i] = 0;    
    pixel_wrgb_[i] = 0ul;
  }
}

void MachineState::push(int ich, uint16_t pot_val, int step_val, int run_val, int mode_val)
{
  run_button.update(run_val);
  mode_button.update(mode_val);
  step_button[ich].update(step_val);
  cv_[ich] = pot_val;
}

int MachineState::next_step(int ki) 
{
  for (int i = 0; i < MAX_NUM_STEPS; ++i) {
    pixel_wrgb_[i] &= 0x00FFFFFF;   // mask off white
  }
  pixel_wrgb_[ki] |= (0x08 << WHITE_BYTE_POS);

  switch (pattern) {
  case LOOP:
    do {
      ki = (ki + 1) % MAX_NUM_STEPS;
    } while(!step_enable[ki]);
    break;
  case BOUNCE:
    do {
      ki += bounce_dir_;
      if (ki == (MAX_NUM_STEPS - 1)) {
        bounce_dir_ = -1;
      } else if (ki == 0) {
        bounce_dir_ = 1;
      }
    } while(!step_enable[ki]);
    break;
  case RANDOM:
    do { 
      xorshift32(r_state_);
      ki = 0x00000007 & r_state_;
    } while(!step_enable[ki]);
    break;
  }
  
  return ki;
}

uint16_t MachineState::quant_cv(int i) const 
{
  // range compression -- divide the range into 5 blocks, scale & shift
  // Voct1: block 2
  // Voct2: block 2, 3
  // Voct5: block 0-4
  int32_t const block_size = 204;   // 1024/5, 4 bits remain (2 top & bottom) == 17*12 (!!)
  int32_t const semitone_width = 17;

  int32_t const v = cv_[i];  // 12 bit ADC, 10 bit DAC
  int32_t vo = (v * block_size * (uint32_t)voct_range + 2048) / 4096;  // integer rounding

  // quantization
  if (quant != Quantization::NONE) {
    int semitone = vo / semitone_width; // 0-16 = 0, 17-33=1, ... (don't round to keep full range)
    // since vo is truncated, it's always closer to the semitone above: --|-----|-*---|--
    if ((quant == Quantization::MAJOR) && (semitone == 1 || semitone == 3 || semitone == 6 || semitone == 8 || semitone == 10)) {
      semitone += 1;
    } else if ((quant == Quantization::MINOR) && (semitone == 1 || semitone == 4 || semitone == 6 || semitone == 9 || semitone == 11)) {
      semitone += 1;
    }
    vo = semitone * semitone_width;
  }

  int32_t block_offset = (voct_range == VOCT_5 ? 2 : 410);
  block_offset += octave_shift_ * block_size;   // +/- 2
  vo += block_offset;
  if (vo > 4095) { 
    return 4095;
  } else if (vo < 0) {
    return 0;
  }
  return (uint16_t)vo;  
}

struct key_states_def
{
  enum ActiveModifier {NONE, RUNSTOP, MODE};
  ActiveModifier modifier;
  bool applied_runstop_modifier;
  bool applied_mode_modifier;
} key_states;

void MachineState::process_key_events()
{
  SoftButton::ButtonEvent const run_event = run_button.event();
  if (run_event == SoftButton::EVENT_KEY_DOWN) {
    key_states.modifier = key_states_def::RUNSTOP;
  } else if (run_event == SoftButton::EVENT_KEY_UP) {
    if (!key_states.applied_runstop_modifier) { // tap
      running = !running; // toggle run/pause if tap
      if (running) {
        for (int i = 0; i < MAX_NUM_STEPS; ++i) {
          if (step_enable[i]) {
            uint8_t const val = step_active[i] ? 0x08 : 0x03;
            pixel_wrgb_[i] = (val << RED_BYTE_POS) | (val << BLUE_BYTE_POS);
          } else {
            pixel_wrgb_[i] = 0;
          }
        }
      } else {
        for (int i = 0; i < MAX_NUM_STEPS; ++i) {
          pixel_wrgb_[i] = 0;
        }
      }
      // TODO reset random seed
    }
    if (key_states.modifier == key_states_def::RUNSTOP) { // remove modifier
      key_states.modifier = key_states_def::NONE;
      key_states.applied_runstop_modifier = false;
    }
  }
  
  SoftButton::ButtonEvent const mode_event = mode_button.event();
  if (mode_event == SoftButton::EVENT_KEY_DOWN) {
    key_states.modifier = key_states_def::MODE;
  } else if (mode_event == SoftButton::EVENT_KEY_UP) {
    if (!key_states.applied_mode_modifier) { // tap
      if (step_button_mode == StepButtonMode::STEP_ACTIVE) {
        step_button_mode = StepButtonMode::STEP_ENABLE;
      } else if (step_button_mode == StepButtonMode::STEP_ENABLE) {
        step_button_mode = StepButtonMode::STEP_ACTIVE;
      }
    }
    if (key_states.modifier == key_states_def::MODE) {  // remove modifier
      key_states.modifier = key_states_def::NONE;
      key_states.applied_mode_modifier = false;
    }
  }

  for (int i = 0; i < MAX_NUM_STEPS; ++i) {
    SoftButton::ButtonEvent e = step_button[i].event();  // consume
    if (e == SoftButton::EVENT_KEY_DOWN) {
      switch (key_states.modifier) {
        case key_states_def::RUNSTOP:
          // TODO re-start sequence here if this step is enabled/active
          key_states.applied_runstop_modifier = true;   // avoid tap events on modifiers
          break;
        case key_states_def::MODE:
          switch (i) {
            case 0: // octave +
              if (octave_shift_ < 2) {
                ++octave_shift_;
              }
              break;
            case 1: // octave -
              if (octave_shift_ > -2) {
                --octave_shift_;
              }
              break;
            case 2: // linear
              quant = Quantization::NONE;
              break;
            case 3: // major
              quant = Quantization::MAJOR;
              break;
            case 4: // minor
              quant = Quantization::MINOR;
              break;
            case 5: // chromatic
              quant = Quantization::CHROMATIC;
              break;  
            case 6: // change pattern
              pattern = (PatternMode)((pattern + 1) % PatternMode::NUM_PATTERNS);
              break;
            case 7: // change voct range
              if (voct_range == OutputRange::VOCT_1) {
                voct_range = OutputRange::VOCT_2;
              } else if (voct_range == OutputRange::VOCT_2) {
                voct_range = OutputRange::VOCT_5;
              } else {
                voct_range = OutputRange::VOCT_1;
              } 
              break;
          } 
          key_states.applied_mode_modifier = true;   // avoid tap events on modifiers
          break;
        case key_states_def::NONE:
        default:
          switch (step_button_mode) {
            case STEP_ACTIVE:
              step_active[i] = step_active[i] == 0 ? 1 : 0;
              break;
            case STEP_ENABLE:
              step_enable[i] = step_enable[i] == 0 ? 1 : 0;
              break;
          }
          break;
          
      }
    }
  }


}


