#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
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


void parse_cpu(const char *s){
	printf("Parsing CPU: %s\n", s);

}

static bool parse_line(
	const char* line_start,
	const char* line_end,
	struct proc_cpuinfo_parser_state state[restrict static 1],
	uint64_t line_number)
{
	printf("-> %s\n", line_start);
	printf("<- %s\n", line_end);

        /* Empty line. Skip. */
        if (line_start == line_end) {
                return true;
        }

        /* Search for ':' on the line. */
        const char* separator = line_start;
        for (; separator != line_end; separator++) {
                if (*separator == ':') {
                        break;
                }
        }
        /* Skip line if no ':' separator was found. */
        if (separator == line_end) {
                cpuinfo_log_warning("Line %.*s in /proc/cpuinfo is ignored: key/value separator ':' not found",
                        (int) (line_end - line_start), line_start);
                return true;
        }

        /* Skip trailing spaces in key part. */
        const char* key_end = separator;
        for (; key_end != line_start; key_end--) {
                if (key_end[-1] != ' ' && key_end[-1] != '\t') {
                        break;
                }
        }
        /* Skip line if key contains nothing but spaces. */
        if (key_end == line_start) {
                cpuinfo_log_warning("Line %.*s in /proc/cpuinfo is ignored: key contains only spaces",
                        (int) (line_end - line_start), line_start);
                return true;
        }

        /* Skip leading spaces in value part. */
        const char* value_start = separator + 1;
        for (; value_start != line_end; value_start++) {
                if (*value_start != ' ') {
                        break;
                }
        }
        /* Value part contains nothing but spaces. Skip line. */
        if (value_start == line_end) {
                cpuinfo_log_warning("Line %.*s in /proc/cpuinfo is ignored: value contains only spaces",
                        (int) (line_end - line_start), line_start);

	}

	if (memcmp(line_start, "cpu", 3) == 0) {
		parse_cpu(line_start);	
	}
		

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
