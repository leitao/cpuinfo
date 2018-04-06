#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include <linux/api.h>
#include <powerpc/linux/api.h>
#include <log.h>

#define CLUSTERS_MAX 4 /* TODO: */

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

uint32_t cpuinfo_powerpc_linux_detect_cluster(
	uint32_t max_processors,
	uint32_t usable_processors,
	struct cpuinfo_powerpc_linux_processor processors[restrict static max_processors])
{
	uint32_t clusters_count = 0;
	uint32_t cluster_leaders[CLUSTERS_MAX];
	uint32_t last_processor_in_cpuinfo = max_processors;

	for (uint32_t i = 0; i < max_processors; i++) {
		if (bitmask_all(processors[i].flags, CPUINFO_LINUX_MASK_USABLE)) {
			if (processors[i].flags & CPUINFO_POWERPC_LINUX_VALID_PROCESSOR) {
				last_processor_in_cpuinfo = i;
			}
			const uint32_t group_leader = processors[i].package_leader_id;
			if (group_leader == i) {
				if (clusters_count < CLUSTERS_MAX) {
					cluster_leaders[clusters_count] = i;
				}
				clusters_count += 1;
			} else {
				/* TODO(rcardoso): Copy known bits of information to cluster leader */
			if ((processors[i].flags & ~processors[group_leader].flags) & CPUINFO_LINUX_FLAG_MAX_FREQUENCY) {
					processors[group_leader].max_frequency = processors[i].max_frequency;
					processors[group_leader].flags |= CPUINFO_LINUX_FLAG_MAX_FREQUENCY;
				}
			}
		}
	}
	cpuinfo_log_debug("detected %"PRIu32" core clusters", clusters_count);

	return clusters_count;
}
