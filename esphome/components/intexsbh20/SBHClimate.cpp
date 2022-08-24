#include "SBHClimate.h"

namespace esphome {
namespace sbh20 {

void SBHClimate::update()
{
	SBH20IO* sbh = get_parent()->sbh();

	if (!sbh->isHeaterOn())
	{
		this->action = esphome::climate::CLIMATE_ACTION_OFF;
		this->mode = esphome::climate::CLIMATE_MODE_OFF;
	}
	else if (sbh->isHeaterStandby())
	{
		this->action = esphome::climate::CLIMATE_ACTION_IDLE;
		this->mode = esphome::climate::CLIMATE_MODE_HEAT;
	}
	else
	{
		this->action = esphome::climate::CLIMATE_ACTION_HEATING;
		this->mode = esphome::climate::CLIMATE_MODE_HEAT;
	}

	this->current_temperature = sbh->getActWaterTempCelsius();

	int targetTemp = sbh->getDesiredWaterTempCelsius();

	this->target_temperature = (targetTemp != SBH20IO::UNDEF::USHORT) ? targetTemp : SBH20IO::WATER_TEMP::SET_MIN;

	if (targetTemp == SBH20IO::UNDEF::USHORT)
	{
		ESP_LOGD("SBHClimate", "Target temp is undef, pressing the down button...");
		sbh->forceGetDesiredWaterTempCelsius(); // we'll learn the real target temp in near future...
	}

	publish_state();
}

void SBHClimate::control(const esphome::climate::ClimateCall &call)
{
	auto tt = call.get_target_temperature();
	if (tt)
	{
		get_parent()->sbh()->setDesiredWaterTempCelsius(*tt);
	}

	auto mode = call.get_mode();
	if (mode)
	{
		get_parent()->sbh()->setHeaterOn(*mode == climate::CLIMATE_MODE_HEAT);
	}
}

esphome::climate::ClimateTraits SBHClimate::traits()
{
	esphome::climate::ClimateTraits rv;

	rv.set_visual_min_temperature(SBH20IO::WATER_TEMP::SET_MIN);
	rv.set_visual_max_temperature(SBH20IO::WATER_TEMP::SET_MAX);
	rv.set_visual_temperature_step(1);
	rv.set_supports_current_temperature(true);
	rv.set_supports_action(true);
	rv.set_supported_modes({ climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_HEAT });

	return rv;
}

}}
