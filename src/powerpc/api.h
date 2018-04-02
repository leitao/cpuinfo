#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <cpuinfo.h>

enum cpuinfo_powerpc_chipset_vendor {
	cpuinfo_powerpc_chipset_vendor_unknown = 0,
	cpuinfo_powerpc_chipset_vendor_ibm
};

/* TODO(rcardoso): not sure what series means here.*/
/*
enum cpuinfo_powerpc_chipset_series {
    cpuinfo_powerpc_chipset_series_unknown = 0,
    cpuinfo_powerpc_chipset_series_power8
};
*/

struct cpuinfo_powerpc_chipset {
	enum cpuinfo_powerpc_chipset_vendor vendor;
};

void cpuinfo_powerpc_decode_vendor_uarch(
	uint32_t pvr,
	enum cpuinfo_vendor vendor[restrict static 1],
	enum cpuinfo_uarch uarch[restrict static 1]);

void cpuinfo_powerpc_chipset_to_string(
    const struct cpuinfo_powerpc_chipset chipset[restrict static 1],
    char name[restrict static CPUINFO_PACKAGE_NAME_MAX]);
