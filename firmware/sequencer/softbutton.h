#ifndef softbutton_h_
#define softbutton_h_

class SoftButton
{
public:

  enum ButtonState {KEY_UP=0, KEY_DOWN};
  enum ButtonEvent {EVENT_NONE, EVENT_KEY_UP, EVENT_KEY_DOWN};
  enum ActiveLevel {ACTIVE_LOW=0, ACTIVE_HIGH=1};

  SoftButton();
  SoftButton(int n, ActiveLevel lvl);

  void set_ticks(int n);
  void set_active_level(ActiveLevel lvl);

  void update(int val);

  ButtonState state() const { return state_; }
  ButtonEvent event() {
    ButtonEvent e = event_;
    event_ = ButtonEvent::EVENT_NONE;
    return e;
  }

private:
  int current_val_;
  int n_ticks_;
  int counter_;
  int tap_timeout_; 
  int tap_counter_;
  ButtonState state_;
  ButtonEvent event_; 
  ActiveLevel active_type_;
};


#endif