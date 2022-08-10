#include "SBHClimate.h"

namespace esphome {
namespace sbh20 {

void SBHClimate::update()
{
	SBH20IO* sbh = get_parent()->sbh();

	if (!sbh->isHeaterOn())
		this->action = esphome::climate::CLIMATE_ACTION_OFF;
	else if (sbh->isHeaterStandby())
		this->action = esphome::climate::CLIMATE_ACTION_IDLE;
	else
		this->action = esphome::climate::CLIMATE_ACTION_HEATING;

	this->current_temperature = sbh->getActWaterTempCelsius();
	this->target_temperature = sbh->getDesiredWaterTempCelsius();

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

	rv.set_visual_min_temperature(10);
	rv.set_visual_max_temperature(40);
	rv.set_visual_temperature_step(1);
	rv.set_supports_current_temperature(true);
	rv.set_supports_action(true);
	rv.set_supported_modes({ climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_HEAT });

	return rv;
}

}}
