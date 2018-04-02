#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <powerpc/linux/api.h>
#include <cpuinfo.h>
#include <log.h>

/* Convert chipset name represented by cpuinfo_powepc_chipset structure to a string representation */
void cpuinfo_powerpc_chipset_to_string(
	const struct cpuinfo_powerpc_chipset chipset[restrict static 1],
	char name[restrict static CPUINFO_PACKAGE_NAME_MAX])
{
	/* TODO(rcardoso): hardcoded.  */
	const char* vendor_string = "IBM";
	const char* series_string = "Power8"; // ????

	snprintf(name, CPUINFO_PACKAGE_NAME_MAX, "%s %s", vendor_string, series_string);
}
