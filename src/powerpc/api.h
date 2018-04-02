#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <cpuinfo.h>

void cpuinfo_powerpc_decode_vendor_uarch(
	uint32_t pvr,
	enum cpuinfo_vendor vendor[restrict static 1],
	enum cpuinfo_uarch uarch[restrict static 1]);
