#pragma once
#include<vector.hpp>
#include<kernel/monitor/command.hpp>

namespace Monitor {
	void start();
	vector<Command*>& cmd_list();
}
