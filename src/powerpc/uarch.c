#include <stdint.h>

#include <powerpc/api.h>
#include <log.h>

void cpuinfo_powerpc_decode_vendor_uarch(
    uint32_t pvr,
    enum cpuinfo_vendor vendor[restrict static 1],
    enum cpuinfo_uarch uarch[restrict static 1]) {

	*uarch = cpuinfo_uarch_power8;
	*vendor = cpuinfo_vendor_ibm;
}
