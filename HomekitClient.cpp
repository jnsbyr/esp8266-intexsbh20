#include "HomekitClient.h"

#include <ESP8266mDNS.h>

#include "SBH20IO.h"
#include "NTCThermometer.h"

extern "C" homekit_server_config_t homekit_config;
extern "C" homekit_accessory_t *accessories[];
extern "C" homekit_characteristic_t spa_target_heating_cooling_state;
extern "C" homekit_characteristic_t spa_current_temperature;
extern "C" homekit_characteristic_t spa_target_temperature;
extern "C" homekit_characteristic_t spa_switch_power;
extern "C" homekit_characteristic_t spa_switch_bubble;
extern "C" homekit_characteristic_t spa_switch_filter;
extern "C" homekit_characteristic_t spa_extern_temperature;

volatile HomekitClient::HKState HomekitClient::hkState;

#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);

HomekitClient* HomekitClient::delegate = nullptr;

HomekitClient::HomekitClient(SBH20IO& poolIO, NTCThermometer& thermometer):
  poolIO(poolIO),
  thermometer(thermometer)
{
  delegate = this;
}

void HomekitClient::setup(const char* password) {
  //homekit_storage_reset();
  LOG_D("HomekitClient setup with password : %s", password);

  // TARGET STATE
  spa_target_heating_cooling_state.getter = getter_target_state_static;
  spa_target_heating_cooling_state.setter = setter_target_state_static;

  // CURRENT TEMPERATURE
  spa_current_temperature.getter = getter_current_temperature_static;

  // TARGET TEMPERATURE
  spa_target_temperature.getter = getter_target_temperature_static;
  spa_target_temperature.setter = setter_target_temperature_static;

  // POWER BUTTON
  spa_switch_power.getter = getter_power_button_static;
  spa_switch_power.setter = setter_power_button_static;

  // BUBBLE BUTTON
  spa_switch_bubble.getter = getter_bubble_button_static;
  spa_switch_bubble.setter = setter_bubble_button_static;

  // FILTER BUTTON
  spa_switch_filter.getter = getter_filter_button_static;
  spa_switch_filter.setter = setter_filter_button_static;

  // EXTERN TEMPERATURE
  spa_extern_temperature.getter = getter_extern_temperature_static;

  homekit_config.password = const_cast<char*>(password);
  
  arduino_homekit_setup(&homekit_config);
}

bool HomekitClient::paired(){
  return homekit_is_paired();
}

void HomekitClient::loop() {
  //LOG_D("HomekitClient loop");
  arduino_homekit_loop();
  const uint32_t t = millis();
  if (t > now) {
    now = t + 10 * 1000;
    
    // https://github.com/Mixiaoxiao/Arduino-HomeKit-ESP8266/issues/154#issuecomment-996625853
    //announce();
    //LOG_D("isPaired : %s", homekit_is_paired()?"yes":"no");
    LOG_D("Free heap: %d, HomeKit clients: %d", ESP.getFreeHeap(), arduino_homekit_connected_clients_count());
  }

  if(homekit_is_paired()){
    
    updater_target_state();
    updater_current_temperature();
    updater_target_temperature();
    updater_power_button();
    updater_bubble_button();
    updater_filter_button();
    updater_extern_temperature();

    if(hkState.need_update){
      LOG_D("NEED_UPDATE %i", hkState.update_temperature);
      hkState.need_update = false;
      poolIO.setDesiredWaterTempCelsius(hkState.update_temperature);
    }
  }
  
}


///////// TARGET STATE ///////////

// GETTER

homekit_value_t getter_target_state_static() {
  //LOG_D("getter_target_state_static");
  return HomekitClient::delegate->getter_target_state();
}

homekit_value_t HomekitClient::getter_target_state() {
  bool heater = poolIO.isHeaterOn();
  if (heater == SBH20IO::UNDEF::BOOL) {
    heater = false;
  }
  hkState.heater = heater;
  //LOG_D("getter_target_state %i", heater?HOMEKIT_TARGET_HEATING_COOLING_STATE_HEAT:HOMEKIT_TARGET_HEATING_COOLING_STATE_OFF);
  return HOMEKIT_UINT8_CPP(heater ? HOMEKIT_TARGET_HEATING_COOLING_STATE_HEAT : HOMEKIT_TARGET_HEATING_COOLING_STATE_OFF);
}

// SETTER

void setter_target_state_static(homekit_value_t value) {
  //LOG_D("setter_current_state_static");
  return HomekitClient::delegate->setter_target_state(value);
}

void HomekitClient::setter_target_state(homekit_value_t value) {
  uint8 state = value.uint8_value;
  hkState.heater = (state == HOMEKIT_TARGET_HEATING_COOLING_STATE_HEAT);
  poolIO.setHeaterOn(hkState.heater);
  LOG_D("setter_target_state %i", state);
}

// UPDATER

void HomekitClient::updater_target_state() {
  bool heater = poolIO.isHeaterOn();
  if (heater == SBH20IO::UNDEF::BOOL) {
    heater = false;
  }
  if (heater != hkState.heater) {
    LOG_D("updater_target_state %d", heater);
    hkState.heater = heater;
    spa_target_heating_cooling_state.value = heater ? HOMEKIT_UINT8_CPP(HOMEKIT_TARGET_HEATING_COOLING_STATE_HEAT) : HOMEKIT_UINT8_CPP(HOMEKIT_TARGET_HEATING_COOLING_STATE_OFF);
    homekit_characteristic_notify(&spa_target_heating_cooling_state, spa_target_heating_cooling_state.value);
  }
}

///////// CURRENT TEMPERATURE ///////////

// GETTER

homekit_value_t getter_current_temperature_static() {
  //LOG_D("getter_current_temperature_static");
  return HomekitClient::delegate->getter_current_temperature();
}

homekit_value_t HomekitClient::getter_current_temperature() {
  int current_temperature = poolIO.getActWaterTempCelsius();
  if (current_temperature == SBH20IO::UNDEF::USHORT) {
    current_temperature = 20;
  }
  hkState.current_temperature = current_temperature;
  //LOG_D("getter_current_temperature %f", float(current_temperature));
  return HOMEKIT_FLOAT_CPP(float(current_temperature));
}

// UPDATER

void HomekitClient::updater_current_temperature() {
  int current_temperature = poolIO.getActWaterTempCelsius();
  if (current_temperature == SBH20IO::UNDEF::USHORT) {
    current_temperature = 20;
  }
  if (current_temperature != hkState.current_temperature) {
    hkState.current_temperature = current_temperature;
    LOG_D("updater_current_temperature %f", float(current_temperature));
    spa_current_temperature.value = HOMEKIT_FLOAT_CPP(float(current_temperature));
    homekit_characteristic_notify(&spa_current_temperature, spa_current_temperature.value);
  }
}

///////// TARGET TEMPERATURE ///////////

// GETTER

homekit_value_t getter_target_temperature_static() {
  //LOG_D("getter_target_temperature_static");
  return HomekitClient::delegate->getter_target_temperature();
}

homekit_value_t HomekitClient::getter_target_temperature() {
  int target_temperature = poolIO.getDesiredWaterTempCelsius();
  if (target_temperature == SBH20IO::UNDEF::USHORT) {
    target_temperature = 40;
  }
  hkState.target_temperature = target_temperature;
  //LOG_D("getter_target_temperature %f", float(target_temperature));
  return HOMEKIT_FLOAT_CPP(float(target_temperature));
}

// SETTER

void setter_target_temperature_static(homekit_value_t value) {
  //LOG_D("setter_target_temperature_static");
  return HomekitClient::delegate->setter_target_temperature(value);
}

void HomekitClient::setter_target_temperature(homekit_value_t value) {
  int target_temperature = int(value.float_value);
  //poolIO.setDesiredWaterTempCelsius(target_temperature);
  hkState.update_temperature = target_temperature;
  hkState.need_update = true;
  LOG_D("setter_target_temperature %i", target_temperature);
}


// UPDATER

void HomekitClient::updater_target_temperature() {
  int target_temperature = poolIO.getDesiredWaterTempCelsius();
  if (target_temperature == SBH20IO::UNDEF::USHORT) {
    target_temperature = 40;
  }
  if (target_temperature != hkState.target_temperature) {
    hkState.target_temperature = target_temperature;
    LOG_D("updater_target_temperature %f", float(target_temperature));
    spa_target_temperature.value = HOMEKIT_FLOAT_CPP(float(target_temperature));
    homekit_characteristic_notify(&spa_target_temperature, spa_target_temperature.value);
  }
}

///////// POWER BUTTON ///////////

// GETTER
homekit_value_t getter_power_button_static() {
  //LOG_D("getter_power_button");
  return HomekitClient::delegate->getter_power_button();
}

homekit_value_t HomekitClient::getter_power_button() {
  bool power = poolIO.isPowerOn();
  if (power == SBH20IO::UNDEF::BOOL) {
    power = false;
  }
  hkState.power = power;
  //LOG_D("getter_power_button %d", power);
  return HOMEKIT_BOOL_CPP(power);
}

// SETTER
void setter_power_button_static(homekit_value_t value) {
  //LOG_D("setter_power_button_static");
  return HomekitClient::delegate->setter_power_button(value);
}

void HomekitClient::setter_power_button(homekit_value_t value) {
  bool power = value.bool_value;
  poolIO.setPowerOn(power);
  hkState.power = power;
  LOG_D("setter_power_button %d", power);
}

// UPDATER
void HomekitClient::updater_power_button() {
  bool power = poolIO.isPowerOn();
  if (power == SBH20IO::UNDEF::BOOL) {
    power = false;
  }
  if (power != hkState.power) {
    hkState.power = power;
    LOG_D("updater_power_button %d", power);
    spa_switch_power.value = HOMEKIT_BOOL_CPP(power);
    homekit_characteristic_notify(&spa_switch_power, spa_switch_power.value);
  }
}

///////// BUBBLE BUTTON ///////////

// GETTER
homekit_value_t getter_bubble_button_static() {
  //LOG_D("getter_bubble_button");
  return HomekitClient::delegate->getter_bubble_button();
}

homekit_value_t HomekitClient::getter_bubble_button() {
  bool bubble = poolIO.isBubbleOn();
  if (bubble == SBH20IO::UNDEF::BOOL) {
    bubble = false;
  }
  hkState.bubble = bubble;
  //LOG_D("getter_bubble_button %d", bubble);
  return HOMEKIT_BOOL_CPP(bubble);
}

// SETTER
void setter_bubble_button_static(homekit_value_t value) {
  //LOG_D("setter_bubble_button_static");
  return HomekitClient::delegate->setter_bubble_button(value);
}

void HomekitClient::setter_bubble_button(homekit_value_t value) {
  bool bubble = value.bool_value;
  poolIO.setBubbleOn(bubble);
  hkState.bubble = bubble;
  LOG_D("setter_bubble_button %d", bubble);
}

// UPDATER
void HomekitClient::updater_bubble_button() {
  bool bubble = poolIO.isBubbleOn();
  if (bubble == SBH20IO::UNDEF::BOOL) {
    bubble = false;
  }
  if (bubble != hkState.bubble) {
    hkState.bubble = bubble;
    LOG_D("updater_bubble_button %d", bubble);
    spa_switch_bubble.value = HOMEKIT_BOOL_CPP(bubble);
    homekit_characteristic_notify(&spa_switch_bubble, spa_switch_bubble.value);
  }
}

///////// FILTER BUTTON ///////////

// GETTER
homekit_value_t getter_filter_button_static() {
  //LOG_D("getter_filter_button");
  return HomekitClient::delegate->getter_filter_button();
}

homekit_value_t HomekitClient::getter_filter_button() {
  bool filter = poolIO.isFilterOn();
  if (filter == SBH20IO::UNDEF::BOOL) {
    filter = false;
  }
  hkState.filter = filter;
  //LOG_D("getter_filter_button %d", filter);
  return HOMEKIT_BOOL_CPP(filter);
}

// SETTER
void setter_filter_button_static(homekit_value_t value) {
  //LOG_D("setter_filter_button_static");
  return HomekitClient::delegate->setter_filter_button(value);
}

void HomekitClient::setter_filter_button(homekit_value_t value) {
  bool filter = value.bool_value;
  poolIO.setFilterOn(filter);
  hkState.filter = filter;
  LOG_D("setter_filter_button %d", filter);
}

// UPDATER
void HomekitClient::updater_filter_button() {
  bool filter = poolIO.isFilterOn();
  if (filter == SBH20IO::UNDEF::BOOL) {
    filter = false;
  }
  if (filter != hkState.filter) {
    hkState.filter = filter;
    LOG_D("updater_filter_button %d", filter);
    spa_switch_filter.value = HOMEKIT_BOOL_CPP(filter);
    homekit_characteristic_notify(&spa_switch_filter, spa_switch_filter.value);
  }
}

///////// EXTERN TEMPERATURE ///////////

// GETTER

homekit_value_t getter_extern_temperature_static() {
  //LOG_D("getter_extern_temperature_static");
  return HomekitClient::delegate->getter_extern_temperature();
}

homekit_value_t HomekitClient::getter_extern_temperature() {
  float extern_temperature = thermometer.getTemperature();
  if(extern_temperature < 0){
    extern_temperature = 0; 
  }
  hkState.extern_temperature = extern_temperature;
  //LOG_D("getter_extern_temperature %f", extern_temperature);
  return HOMEKIT_FLOAT_CPP(extern_temperature);
}

// UPDATER

void HomekitClient::updater_extern_temperature() {
  float extern_temperature = thermometer.getTemperature();
  if(extern_temperature < 0){
    extern_temperature = 0; 
  }
  float diff = abs(hkState.extern_temperature - extern_temperature);
  if (diff >= 0.5) {
    hkState.extern_temperature = extern_temperature;
    LOG_D("updater_extern_temperature %f %f", extern_temperature, hkState.extern_temperature);
    spa_extern_temperature.value = HOMEKIT_FLOAT_CPP(extern_temperature);
    homekit_characteristic_notify(&spa_extern_temperature, spa_extern_temperature.value);
  }
}


////////////////////////////////


bool HomekitClient::announce() {
  MDNS.announce();
  return true;
}
