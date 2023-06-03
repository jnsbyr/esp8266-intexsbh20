#include <arduino_homekit_server.h>
#include <homekit/homekit.h>
#include <homekit/characteristics.h>

class SBH20IO;
class NTCThermometer;

//https://github.com/Mixiaoxiao/Arduino-HomeKit-ESP8266
class HomekitClient
{
public:
  static HomekitClient* delegate;

private:
  struct HKState
    {
      float extern_temperature = 10;
      unsigned int current_temperature = 20;
      unsigned int target_temperature = 40;
      bool heater = false;
      bool power = false;
      bool bubble = false;
      bool filter = false;
    };
  static volatile HKState hkState;

public:
  HomekitClient(SBH20IO& poolIO, NTCThermometer& thermometer);
  void setup(const char* password);
  void loop();
  bool paired();

  // TARGET STATE
  homekit_value_t getter_target_state();
  void setter_target_state(homekit_value_t value);
  void updater_target_state();

  // CURRENT TEMPERATURE
  homekit_value_t getter_current_temperature();
  void updater_current_temperature();

  // TARGET TEMPERATURE
  homekit_value_t getter_target_temperature();
  void setter_target_temperature(homekit_value_t value);
  void updater_target_temperature();

  // POWER BUTTON
  homekit_value_t getter_power_button();
  void setter_power_button(homekit_value_t value);
  void updater_power_button();

  // BUBBLE BUTTON
  homekit_value_t getter_bubble_button();
  void setter_bubble_button(homekit_value_t value);
  void updater_bubble_button();

  // FILTER BUTTON
  homekit_value_t getter_filter_button();
  void setter_filter_button(homekit_value_t value);
  void updater_filter_button();

  // EXTERN TEMPERATURE
  homekit_value_t getter_extern_temperature();
  void updater_extern_temperature();
  
private: 
  unsigned int now;
  SBH20IO& poolIO;
  NTCThermometer& thermometer;
  bool announce();
};

// TARGET STATE
homekit_value_t getter_target_state_static();
void setter_target_state_static(homekit_value_t value);

// CURRENT TEMPERATURE
homekit_value_t getter_current_temperature_static();

// TARGET TEMPERATURE
homekit_value_t getter_target_temperature_static();
void setter_target_temperature_static(homekit_value_t value);

// POWER BUTTON
homekit_value_t getter_power_button_static();
void setter_power_button_static(homekit_value_t value);

// BUBBLE BUTTON
homekit_value_t getter_bubble_button_static();
void setter_bubble_button_static(homekit_value_t value);

// FILTER BUTTON
homekit_value_t getter_filter_button_static();
void setter_filter_button_static(homekit_value_t value);

// EXTERN TEMPERATURE
homekit_value_t getter_extern_temperature_static();
