#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include <linux/api.h>
#include <powerpc/linux/api.h>
#include <log.h>

static inline bool bitmask_all(uint32_t bitfield, uint32_t mask) {
	return (bitfield & mask) == mask;
}

/*
 * Counts the number of logical processors in each core cluster.
 *
*/
void cpuinfo_powerpc_linux_count_cluster_processors(
	uint32_t max_processors,
	struct cpuinfo_powerpc_linux_processor processors[restrict static max_processors])
{
	/* First pass: accumulate the number of processors at the group leader's package_processor_count */
	for (uint32_t i = 0; i< max_processors; i++) {
		if (bitmask_all(processors[i].flags, CPUINFO_LINUX_MASK_USABLE)) {
			const uint32_t package_leader_id = processors[i].package_leader_id;
			processors[package_leader_id].package_processor_count += 1;
		}
	}

	/* Second pass: copy the package_processor_count from the group leader processor */
	for (uint32_t i = 0; i< max_processors; i++) {
		if (bitmask_all(processors[i].flags, CPUINFO_LINUX_MASK_USABLE)) {
			const uint32_t package_leader_id = processors[i].package_leader_id;
			processors[i].package_processor_count = processors[package_leader_id].package_processor_count;
		}
	}
}
