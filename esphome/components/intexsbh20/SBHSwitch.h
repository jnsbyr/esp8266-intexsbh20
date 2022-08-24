#ifndef SBHSWITCH_H_
#define SBHSWITCH_H_
#include "esphome/components/switch/switch.h"
#include "esphome/core/log.h"
#include "intexsbh20.h"

namespace esphome {
namespace sbh20 {

class SBHSwitch : public esphome::switch_::Switch, public esphome::Parented<IntexSBH20>
{
public:
	void set_type(const char* type)
	{
		type_ = type;
	}
	void write_state(bool state) override
	{
		if (!type_)
		{
			ESP_LOGE("SBHSwitch", "No switch type set!");
			return;
		}

		if (strcmp(type_, "bubble") == 0)
		{
			get_parent()->sbh()->setBubbleOn(state);
		}
		else if (strcmp(type_, "filter") == 0)
		{
			get_parent()->sbh()->setFilterOn(state);
		}
		else if (strcmp(type_, "power") == 0)
		{
			get_parent()->sbh()->setPowerOn(state);
		}
		else
		{
			ESP_LOGE("SBHSwitch", "Unknown switch type: %s", type_);
			return;
		}

		if (get_parent()->sbh()->isOnline())
			publish_state(state);
	}
private:
	const char* type_ = nullptr;
};

}}

#endif
