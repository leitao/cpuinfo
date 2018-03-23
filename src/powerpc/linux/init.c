#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <cpuinfo.h>
#include <powerpc/linux/api.h>
#include <gpu/api.h>
#include <linux/api.h>
#include <api.h>
#include <log.h>


struct cpuinfo_powerpc_isa cpuinfo_isa = { 0 };

static struct cpuinfo_package package = { { 0 } };


static inline uint32_t min(uint32_t a, uint32_t b) {
	return a < b ? a : b;
}

static inline int cmp(uint32_t a, uint32_t b) {
	return (a > b) - (a < b);
}

static inline bool bitmask_all(uint32_t bitfield, uint32_t mask) {
        return (bitfield & mask) == mask;
}


void cpuinfo_powerpc_linux_init(void) {
	struct cpuinfo_powerpc_linux_processor* powerpc_linux_processors = NULL;
	struct cpuinfo_processor* processors = NULL;
	struct cpuinfo_core* cores = NULL;
	struct cpuinfo_cluster* clusters = NULL;
	const struct cpuinfo_processor** linux_cpu_to_processor_map = NULL;
	const struct cpuinfo_core** linux_cpu_to_core_map = NULL;
	struct cpuinfo_cache* l1i = NULL;
	struct cpuinfo_cache* l1d = NULL;
	struct cpuinfo_cache* l2 = NULL;
	uint32_t usable_processors = 0;


	const uint32_t max_processors_count = cpuinfo_linux_get_max_processors_count();
	cpuinfo_log_debug("system maximum processors count: %"PRIu32, max_processors_count);

	const uint32_t max_possible_processors_count = 1 +
		cpuinfo_linux_get_max_possible_processor(max_processors_count);
	cpuinfo_log_debug("maximum possible processors count: %"PRIu32, max_possible_processors_count);
	const uint32_t max_present_processors_count = 1 +
		cpuinfo_linux_get_max_present_processor(max_processors_count);
	cpuinfo_log_debug("maximum present processors count: %"PRIu32, max_present_processors_count);

	const uint32_t powerpc_linux_processors_count = 8;

	powerpc_linux_processors = calloc(powerpc_linux_processors_count, sizeof(struct cpuinfo_powerpc_linux_processor));
	if (powerpc_linux_processors == NULL) {
		cpuinfo_log_error(
			"failed to allocate %zu bytes for descriptions of %"PRIu32" POWERPC logical processors",
			powerpc_linux_processors_count * sizeof(struct cpuinfo_powerpc_linux_processor),
			powerpc_linux_processors_count);
		return;
	}

	cpuinfo_linux_detect_possible_processors(
		powerpc_linux_processors_count, &powerpc_linux_processors->flags,
		sizeof(struct cpuinfo_powerpc_linux_processor),
		CPUINFO_LINUX_FLAG_POSSIBLE);

	cpuinfo_linux_detect_present_processors(
		powerpc_linux_processors_count, &powerpc_linux_processors->flags,
		sizeof(struct cpuinfo_powerpc_linux_processor),
		CPUINFO_LINUX_FLAG_PRESENT);

	char proc_cpuinfo_hardware[CPUINFO_HARDWARE_VALUE_MAX] = { 0 };

	if (!cpuinfo_powerpc_linux_parse_proc_cpuinfo(
			proc_cpuinfo_hardware,
			powerpc_linux_processors_count,
			powerpc_linux_processors)) {
		cpuinfo_log_error("failed to parse processor information from /proc/cpuinfo");
		return;
	}

	for (uint32_t i = 0; i < powerpc_linux_processors_count; i++) {
		if (bitmask_all(powerpc_linux_processors[i].flags, CPUINFO_LINUX_MASK_USABLE)) {
			cpuinfo_log_debug("parsed processor %"PRIu32" MIDR 0x%08"PRIx32,
				i, powerpc_linux_processors[i].midr);
		}
	}


	/* Initialize topology group IDs */
	for (uint32_t i = 0; i < powerpc_linux_processors_count; i++) {
		powerpc_linux_processors[i].package_leader_id = i;
	}

        for (uint32_t i = 0; i < powerpc_linux_processors_count; i++) {
                powerpc_linux_processors[i].system_processor_id = i;
                if (bitmask_all(powerpc_linux_processors[i].flags, CPUINFO_LINUX_MASK_USABLE)) {
                        usable_processors += 1;
		}
	}


	processors = calloc(usable_processors, sizeof(struct cpuinfo_processor));
	if (processors == NULL) {
		cpuinfo_log_error("failed to allocate %zu bytes for descriptions of %"PRIu32" logical processors",
			sizeof(struct cpuinfo_processor), usable_processors);
		goto cleanup;
	}

	cores = calloc(usable_processors, sizeof(struct cpuinfo_core));
	if (cores == NULL) {
		cpuinfo_log_error("failed to allocate %zu bytes for descriptions of %"PRIu32" cores",
			sizeof(struct cpuinfo_core), usable_processors);
		goto cleanup;
	}

	linux_cpu_to_processor_map = calloc(powerpc_linux_processors_count, sizeof(struct cpuinfo_processor*));
	if (linux_cpu_to_processor_map == NULL) {
		cpuinfo_log_error("failed to allocate %zu bytes for %"PRIu32" logical processor mapping entries",
			powerpc_linux_processors_count * sizeof(struct cpuinfo_processor*), powerpc_linux_processors_count);
		goto cleanup;
	}
	/* Commit */
	cpuinfo_linux_cpu_to_processor_map = linux_cpu_to_processor_map;
	cpuinfo_linux_cpu_to_core_map = linux_cpu_to_core_map;
	cpuinfo_processors = processors;
	cpuinfo_cores = cores;
	//cpuinfo_clusters = clusters;
	//cpuinfo_packages = &package;
	//cpuinfo_cache[cpuinfo_cache_level_1i] = l1i;
	//cpuinfo_cache[cpuinfo_cache_level_1d] = l1d;
	//cpuinfo_cache[cpuinfo_cache_level_2]  = l2;

	//cpuinfo_processors_count = usable_processors;
	//cpuinfo_cores_count = usable_processors;
	//cpuinfo_clusters_count = cluster_count;
	//cpuinfo_packages_count = 1;
	//cpuinfo_cache_count[cpuinfo_cache_level_1i] = usable_processors;
	//cpuinfo_cache_count[cpuinfo_cache_level_1d] = usable_processors;
	//cpuinfo_cache_count[cpuinfo_cache_level_2]  = l2_count;

	//linux_cpu_to_processor_map = NULL;
	//linux_cpu_to_core_map = NULL;
	//processors = NULL;
	//cores = NULL;
	//clusters = NULL;
	//l1i = l1d = l2 = NULL;

cleanup:
	free(powerpc_linux_processors);
	free(linux_cpu_to_processor_map);
	free(linux_cpu_to_core_map);
	free(processors);
	free(cores);
}
