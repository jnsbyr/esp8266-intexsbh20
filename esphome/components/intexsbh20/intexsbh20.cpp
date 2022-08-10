#include "intexsbh20.h"
#include "SBHSwitch.h"
#include "SBHClimate.h"

namespace esphome {
namespace sbh20 {

void IntexSBH20::setup()
{
	sbh_.setup(LANG::EN);
}

void IntexSBH20::loop()
{
	sbh_.loop();
}

void IntexSBH20::update()
{
	if (!sbh_.isOnline())
	{
		status_set_error();
		return;
	}

	if (status_has_error())
		status_clear_error();

	int errorValue = sbh_.getErrorValue();
	if (errorValue != 0)
	{
		status_set_warning();

		if (error_text_)
			error_text_->publish_state(sbh_.getErrorMessage(errorValue).c_str());
	}
	else if (status_has_warning())
	{
		status_clear_warning();

		if (error_text_)
			error_text_->publish_state("");
	}

	if (switch_bubble_)
		switch_bubble_->publish_state(sbh_.isBubbleOn());
	if (switch_filter_)
		switch_filter_->publish_state(sbh_.isFilterOn());
	if (switch_power_)
		switch_power_->publish_state(sbh_.isPowerOn());

	if (climate_)
		climate_->update();
}


}}
