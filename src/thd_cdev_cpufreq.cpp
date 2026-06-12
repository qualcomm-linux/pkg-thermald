/*
 * thd_cdev_pstates.cpp: thermal cooling class implementation
 *
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 or later as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 *
 * Author Name <Srinivas.Pandruvada@linux.intel.com>
 *
 */

/* Control P states using cpufreq. Each step reduces to next lower frequency
 *
 */

#include "thd_cdev_cpufreq.h"
#include "thd_engine.h"

int cthd_cdev_cpufreq::init() {
	// Get number of CPUs
	if (cdev_sysfs.exists("present")) {
		std::string count_str;
		size_t p0 = 0, p1;

		cdev_sysfs.read("present", count_str);
		p1 = count_str.find_first_of('-', p0);
		if (p1 == std::string::npos)
			return THD_ERROR;

		std::string token1 = count_str.substr(p0, p1 - p0);
		if (token1.empty())
			return THD_ERROR;

		std::istringstream iss1(token1);
		if (!(iss1 >> cpu_start_index) || !iss1.eof()) {
			thd_log_warn("Invalid CPU start index format\n");
			return THD_ERROR;
		}
		if (cpu_start_index < 0 || cpu_start_index > 63) {
			thd_log_warn("CPU start index out of range: %d\n", cpu_start_index);
			return THD_ERROR;
		}
		if ((p1 + 1) >= count_str.size())
			return THD_ERROR;

		std::string token2 = count_str.substr(p1 + 1);
		if (token2.empty())
			return THD_ERROR;

		std::istringstream iss2(token2);
		if (!(iss2 >> cpu_end_index)) {
			thd_log_warn("Invalid CPU end index format\n");
			return THD_ERROR;
		}
		iss2 >> std::ws;
		if (!iss2.eof()) {
			thd_log_warn("Invalid CPU end index trailing characters\n");
			return THD_ERROR;
		}
		if ((cpu_end_index <= 0) || (cpu_end_index < cpu_start_index)
				|| cpu_end_index > 63)
			return THD_ERROR;
	} else {
		return THD_ERROR;
	}
	thd_log_debug("pstate CPU present %d-%d\n", cpu_start_index, cpu_end_index);

	// Get list of available frequencies for each CPU
	// Assuming every core supports same sets of frequencies, so
	// just reading for cpu0
	std::vector<std::string> _cpufreqs;
	if (cdev_sysfs.exists("cpu0/cpufreq/scaling_available_frequencies")) {
		std::string p =
				"/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies";

		std::ifstream f(p.c_str(), std::fstream::in);
		if (f.fail())
			return -EINVAL;

		while (!f.eof()) {
			std::string token;

			f >> token;
			if (!f.bad()) {
				if (!token.empty())
					_cpufreqs.push_back(std::move(token));
			}
		}
		f.close();
	} else
		return THD_ERROR;

	// Check scaling max frequency and min frequency
	// Remove frequencies above and below this in the freq list
	// The available list contains these frequencies even if they are not allowed
	int scaling_min_frequency = 0;
	int scaling_max_frequency = 0;
	for (int i = cpu_start_index; i <= cpu_end_index; ++i) {
		std::ostringstream str;
		int freq_int = 0;

		str << "cpu" << i << "/cpufreq/scaling_min_freq";
		if (cdev_sysfs.exists(str.str())) {
			int ret = cdev_sysfs.read(str.str(), &freq_int);
			// Only use freq_int if read returned data (ret > 0)
			if (ret > 0 && (scaling_min_frequency == 0 || freq_int < scaling_min_frequency))
				scaling_min_frequency = freq_int;
		}
	}

	for (int i = cpu_start_index; i <= cpu_end_index; ++i) {
		std::ostringstream str;
		int freq_int = 0;

		str << "cpu" << i << "/cpufreq/scaling_max_freq";
		if (cdev_sysfs.exists(str.str())) {
			int ret = cdev_sysfs.read(str.str(), &freq_int);
			// Only use freq_int if read returned data (ret > 0)
			if (ret > 0 && (scaling_max_frequency == 0 || freq_int > scaling_max_frequency))
				scaling_max_frequency = freq_int;
		}
	}

	thd_log_debug("cpu freq max %d min %d\n", scaling_max_frequency,
			scaling_min_frequency);

	for (unsigned int i = 0; i < _cpufreqs.size(); ++i) {
		thd_log_debug("cpu freq Add %d: %s\n", i, _cpufreqs[i].c_str());

		int freq_int = 0;
		std::istringstream iss(_cpufreqs[i]);
		if (!(iss >> freq_int) || !iss.eof()) {
			thd_log_warn("Invalid frequency format: %s\n", _cpufreqs[i].c_str());
			continue;
		}

		if (freq_int >= scaling_min_frequency
				&& freq_int <= scaling_max_frequency) {
			add_frequency(freq_int);
		}
	}

	for (unsigned int i = 0; i < cpufreqs.size(); ++i) {
		thd_log_debug("cpu freq %d: %d\n", i, cpufreqs[i]);
	}

	if (cpufreqs.size())
		max_state = cpufreqs.size() - 1;

	pstate_active_freq_index = 0;

	return THD_SUCCESS;
}

void cthd_cdev_cpufreq::add_frequency(unsigned int freq_int) {
	if (cpufreqs.empty() || cpufreqs.at(0) > (int) freq_int)
		cpufreqs.push_back(freq_int);
	else {
		std::vector<int>::iterator it;
		it = cpufreqs.begin();
		cpufreqs.insert(it, freq_int);
	}
}

void cthd_cdev_cpufreq::set_curr_state(int state, int arg) {

	if (state >=0 && state < (int) cpufreqs.size()) {
		thd_log_debug("cpu freq set_curr_stat %d: %d\n", state,
				cpufreqs[state]);

		if (cpu_index == -1) {
			for (int i = cpu_start_index; i <= cpu_end_index; ++i) {
				std::ostringstream str;
				str << "cpu" << i << "/cpufreq/scaling_max_freq";
				if (cdev_sysfs.exists(str.str())) {
					std::ostringstream speed;
					speed << cpufreqs[state];
					cdev_sysfs.write(str.str(), speed.str());
				}
				pstate_active_freq_index = state;
				curr_state = state;
			}
		} else {
			if (thd_engine->apply_cpu_operation(cpu_index)) {
				std::ostringstream str;
				str << "cpu" << cpu_index << "/cpufreq/scaling_max_freq";
				if (cdev_sysfs.exists(str.str())) {
					std::ostringstream speed;
					speed << cpufreqs[state];
					cdev_sysfs.write(str.str(), speed.str());
				}
				pstate_active_freq_index = state;
				curr_state = state;
			}
		}
	}

}

int cthd_cdev_cpufreq::get_max_state() {
	return cpufreqs.size() - 1;
}

int cthd_cdev_cpufreq::update() {
	return init();
}
