#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <linux/api.h>
#include <powerpc/linux/api.h>
#include <log.h>

/*
 * Size, in chars, of the on-stack buffer used for parsing lines of /proc/cpuinfo.
 * This is also the limit on the length of a single line.
 */
#define BUFFER_SIZE 1024

struct proc_cpuinfo_parser_state {
        char* hardware;
        uint32_t processor_index;
        uint32_t max_processors_count;
        struct cpuinfo_powerpc_linux_processor* processors;
        struct cpuinfo_powerpc_linux_processor dummy_processor;
};


static bool parse_line(
	const char* line_start,
	const char* line_end,
	struct proc_cpuinfo_parser_state state[restrict static 1],
	uint64_t line_number)
{
	return true;
}

static uint32_t parse_processor_number(
	const char* processor_start,
	const char* processor_end)
{
	const size_t processor_length = (size_t) (processor_end - processor_start);

	if (processor_length == 0) {
		cpuinfo_log_warning("Processor number in /proc/cpuinfo is ignored: string is empty");
		return 0;
	}

	uint32_t processor_number = 0;
	for (const char* digit_ptr = processor_start; digit_ptr != processor_end; digit_ptr++) {
		const uint32_t digit = (uint32_t) (*digit_ptr - '0');
		if (digit > 10) {
			cpuinfo_log_warning("non-decimal suffix %.*s in /proc/cpuinfo processor number is ignored",
				(int) (processor_end - digit_ptr), digit_ptr);
			break;
		}

		processor_number = processor_number * 10 + digit;
	}

	return processor_number;
}

bool cpuinfo_powerpc_linux_parse_proc_cpuinfo(
	char hardware[restrict static CPUINFO_HARDWARE_VALUE_MAX],
	uint32_t max_processors_count,
	struct cpuinfo_powerpc_linux_processor processors[restrict static max_processors_count])
{
	struct proc_cpuinfo_parser_state state = {
		.hardware = hardware,
		.processor_index = 0,
		.max_processors_count = max_processors_count,
		.processors = processors,
	};
	return cpuinfo_linux_parse_multiline_file("/proc/cpuinfo", BUFFER_SIZE,
		(cpuinfo_line_callback) parse_line, &state);
}
