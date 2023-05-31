#include <homekit/homekit.h>
#include <homekit/characteristics.h>

const int CELSIUS = 0;
const int FAHRENHEIT = 1;

homekit_characteristic_t spa_current_heating_cooling_state = HOMEKIT_CHARACTERISTIC_(CURRENT_HEATING_COOLING_STATE, HOMEKIT_CURRENT_HEATING_COOLING_STATE_HEAT, .valid_values={.count=2, .values=(uint8_t[]) {HOMEKIT_CURRENT_HEATING_COOLING_STATE_OFF,HOMEKIT_CURRENT_HEATING_COOLING_STATE_HEAT}});
homekit_characteristic_t spa_target_heating_cooling_state = HOMEKIT_CHARACTERISTIC_(TARGET_HEATING_COOLING_STATE, HOMEKIT_CURRENT_HEATING_COOLING_STATE_HEAT, .valid_values={.count=2, .values=(uint8_t[]) {HOMEKIT_TARGET_HEATING_COOLING_STATE_OFF,HOMEKIT_TARGET_HEATING_COOLING_STATE_HEAT}});
homekit_characteristic_t spa_current_temperature = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 20.f, .min_step = (float[]) {1.0});
homekit_characteristic_t spa_target_temperature = HOMEKIT_CHARACTERISTIC_(TARGET_TEMPERATURE, 25.f, .min_step = (float[]) {1.0},.max_value = (float[]) {40.0}, .min_value = (float[]) {20.0});
homekit_characteristic_t spa_temperature_display_units = HOMEKIT_CHARACTERISTIC_(TEMPERATURE_DISPLAY_UNITS, CELSIUS);

homekit_characteristic_t spa_extern_temperature = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 0);

homekit_characteristic_t spa_switch_power = HOMEKIT_CHARACTERISTIC_(ON, false);
homekit_characteristic_t spa_switch_bubble = HOMEKIT_CHARACTERISTIC_(ON, false);
homekit_characteristic_t spa_switch_filter = HOMEKIT_CHARACTERISTIC_(ON, false);

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_thermostat, .services=(homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Thermostat - Intex SBH20"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Arduino HomeKit"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "0123456"),
            HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            //HOMEKIT_CHARACTERISTIC(IDENTIFY, accessory_identify),
            NULL
        }),
        HOMEKIT_SERVICE(THERMOSTAT, .primary=true, .characteristics=(homekit_characteristic_t*[]) {
          HOMEKIT_CHARACTERISTIC(NAME, "Thermostat"),
          &spa_current_heating_cooling_state,
          &spa_target_heating_cooling_state,
          &spa_current_temperature,
          &spa_target_temperature,
          &spa_temperature_display_units,
          NULL
        }),
        NULL
    }),
    HOMEKIT_ACCESSORY(.id=2, .category=homekit_accessory_category_sensor, .services=(homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "External Temperature"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Arduino HomeKit"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "0123456"),
            HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            //HOMEKIT_CHARACTERISTIC(IDENTIFY, my_accessory_identify),
            NULL
        }),
        HOMEKIT_SERVICE(TEMPERATURE_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]) {
          HOMEKIT_CHARACTERISTIC(NAME, "Extern Temperature"),
          &spa_extern_temperature,
          NULL
        }),
        NULL
    }),
    HOMEKIT_ACCESSORY(.id=3, .category=homekit_accessory_category_switch, .services=(homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Power"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Arduino HomeKit"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "0123456"),
            HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            //HOMEKIT_CHARACTERISTIC(IDENTIFY, my_accessory_identify),
            NULL
        }),
        HOMEKIT_SERVICE(SWITCH, .primary=true, .characteristics=(homekit_characteristic_t*[]){
          HOMEKIT_CHARACTERISTIC(NAME, "Power"),
          &spa_switch_power,
          NULL
        }),
        NULL
    }),
    HOMEKIT_ACCESSORY(.id=4, .category=homekit_accessory_category_switch, .services=(homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Bubbles"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Arduino HomeKit"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "0123456"),
            HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            //HOMEKIT_CHARACTERISTIC(IDENTIFY, my_accessory_identify),
            NULL
        }),
        HOMEKIT_SERVICE(SWITCH, .primary=true, .characteristics=(homekit_characteristic_t*[]){
          HOMEKIT_CHARACTERISTIC(NAME, "Bubbles"),
          &spa_switch_bubble,
          NULL
        }),
        NULL
    }),
    HOMEKIT_ACCESSORY(.id=5, .category=homekit_accessory_category_switch, .services=(homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Filter"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Arduino HomeKit"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "0123456"),
            HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            //HOMEKIT_CHARACTERISTIC(IDENTIFY, my_accessory_identify),
            NULL
        }),
        HOMEKIT_SERVICE(SWITCH, .primary=true, .characteristics=(homekit_characteristic_t*[]){
          HOMEKIT_CHARACTERISTIC(NAME, "Filter"),
          &spa_switch_filter,
          NULL
        }),
        NULL
    }),
    NULL
};


homekit_server_config_t homekit_config = {
    .accessories = accessories,
    .password = "000-00-000"
  };
