#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <cpuinfo.h>
#include <powerpc/linux/api.h>
#include <powerpc/api.h>
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

static void cluster_siblings_parser(
	uint32_t processor, uint32_t siblings_start, uint32_t siblings_end,
	struct cpuinfo_powerpc_linux_processor* processors)
{
	processors[processor].flags |=  CPUINFO_LINUX_FLAG_PACKAGE_CLUSTER;
	uint32_t package_leader_id = processors[processor].package_leader_id;

	for (uint32_t sibling = siblings_start; sibling < siblings_end; sibling++) {
		if (!bitmask_all(processors[sibling].flags, CPUINFO_LINUX_MASK_USABLE)) {
			cpuinfo_log_warning("invalid processor %"PRIu32" reported as a sibling for processor %"PRIu32,
					sibling, processor);
			continue;
		}

		const uint32_t sibling_package_leader_id = processors[sibling].package_leader_id;
		if (sibling_package_leader_id < package_leader_id) {
			package_leader_id = sibling_package_leader_id;
		}
		processors[sibling].package_leader_id = package_leader_id;
		processors[sibling].flags |= CPUINFO_LINUX_FLAG_PACKAGE_CLUSTER;
	}

	processors[processor].package_leader_id = package_leader_id;
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
	struct cpuinfo_cache* l3 = NULL;
	struct cpuinfo_cache* l4 = NULL;
	int smt;

	const uint32_t max_processors_count = cpuinfo_linux_get_max_processors_count();
	cpuinfo_log_debug("system maximum processors count: %"PRIu32, max_processors_count);

	const uint32_t max_possible_processors_count = 1 +
		cpuinfo_linux_get_max_possible_processor(max_processors_count);
	cpuinfo_log_debug("maximum possible processors count: %"PRIu32, max_possible_processors_count);

	const uint32_t max_present_processors_count = 1 +
		cpuinfo_linux_get_max_present_processor(max_processors_count);
	cpuinfo_log_debug("maximum present processors count: %"PRIu32, max_present_processors_count);

	const uint32_t powerpc_linux_processors_count = cpuinfo_linux_get_max_processors_count();
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

	uint32_t usable_processors = 0;
	for (uint32_t i = 0; i < powerpc_linux_processors_count; i++) {
		powerpc_linux_processors[i].system_processor_id = i;
		if (bitmask_all(powerpc_linux_processors[i].flags, CPUINFO_LINUX_MASK_USABLE)) {
			usable_processors += 1;
			if (!(powerpc_linux_processors[i].flags & CPUINFO_POWERPC_LINUX_VALID_PROCESSOR)) {
				cpuinfo_log_info("processor %"PRIu32" is not listed in /proc/cpuinfo", i);
			}
			cpuinfo_log_debug("parsed processor %"PRIu32" PVR 0x%08"PRIx32,
					i, powerpc_linux_processors[i].pvr);
		} else {
			/* Processor reported in /proc/cpuinfo, but not in possible and/or present lists: log and ignore */
			if (!(powerpc_linux_processors[i].flags & CPUINFO_POWERPC_LINUX_VALID_PROCESSOR)) {
				cpuinfo_log_warning("invalid processor %"PRIu32" reported in /proc/cpuinfo", i);
			}
		}
	}

	/* Where's this used ?? */
	const uint32_t isa_features = cpuinfo_powerpc_linux_hwcap_from_getauxval();
	cpuinfo_ppc64_linux_decode_isa_from_proc_cpuinfo(isa_features, &cpuinfo_isa);

	/* Detect min/max frequency and package id*/
	for (uint32_t i = 0; i < powerpc_linux_processors_count; i++) {
		const uint32_t max_frequency = cpuinfo_linux_get_processor_max_frequency(i);
		if (max_frequency != 0) {
			powerpc_linux_processors[i].max_frequency = max_frequency;
			powerpc_linux_processors[i].flags |= CPUINFO_LINUX_FLAG_MAX_FREQUENCY;
		}

		const uint32_t min_frequency = cpuinfo_linux_get_processor_min_frequency(i);
		if (min_frequency != 0) {
			powerpc_linux_processors[i].min_frequency = min_frequency;
			powerpc_linux_processors[i].flags |= CPUINFO_LINUX_FLAG_MIN_FREQUENCY;
		}
		if (cpuinfo_linux_get_processor_package_id(i, &powerpc_linux_processors[i].package_id)) {
			powerpc_linux_processors[i].flags |= CPUINFO_LINUX_FLAG_PACKAGE_ID;
		}
	}

	/* Initialize topology group IDs */
	for (uint32_t i = 0; i < powerpc_linux_processors_count; i++) {
		powerpc_linux_processors[i].package_leader_id = i;
	}

	for (uint32_t i = 0; i < powerpc_linux_processors_count; i++) {
		if (!bitmask_all(powerpc_linux_processors[i].flags, CPUINFO_LINUX_MASK_USABLE)) {
			continue;
		}
		if (powerpc_linux_processors[i].flags, CPUINFO_LINUX_FLAG_PACKAGE_ID) {
			cpuinfo_linux_detect_core_siblings(
					powerpc_linux_processors_count, i,
					(cpuinfo_siblings_callback) cluster_siblings_parser,
					powerpc_linux_processors);
		}
	}

	/* Propagate all cluster IDs */
	uint32_t clustered_processors = 0;
	for (uint32_t i = 0; i < powerpc_linux_processors_count; i++) {
		if (bitmask_all(powerpc_linux_processors[i].flags, CPUINFO_LINUX_MASK_USABLE | CPUINFO_LINUX_FLAG_PACKAGE_CLUSTER)) {
			clustered_processors += 1;
		}
		const uint32_t package_leader_id = powerpc_linux_processors[i].package_leader_id;
		if (package_leader_id < i) {
			powerpc_linux_processors[i].package_leader_id = powerpc_linux_processors[package_leader_id].package_leader_id;
		}
		cpuinfo_log_debug("processor %"PRIu32" clustered with processor %"PRIu32" as inferred from system siblings lists",
				i, powerpc_linux_processors[i].package_leader_id);
	}

	for (uint32_t i = 0; i < powerpc_linux_processors_count; i++) {
		if (bitmask_all(powerpc_linux_processors[i].flags, CPUINFO_LINUX_MASK_USABLE)) {
			const uint32_t cluster_leader = powerpc_linux_processors[i].package_leader_id;
			if (cluster_leader == i) {
				cpuinfo_powerpc_decode_vendor_uarch(
				powerpc_linux_processors[i].pvr,
				&powerpc_linux_processors[i].vendor,
				&powerpc_linux_processors[i].uarch);
			} else {
				powerpc_linux_processors[i].flags |= (powerpc_linux_processors[cluster_leader].flags & CPUINFO_LINUX_FLAG_MAX_FREQUENCY);
				powerpc_linux_processors[i].pvr = powerpc_linux_processors[cluster_leader].pvr;
				powerpc_linux_processors[i].vendor = powerpc_linux_processors[cluster_leader].vendor;
				powerpc_linux_processors[i].uarch = powerpc_linux_processors[cluster_leader].uarch;
				powerpc_linux_processors[i].max_frequency = powerpc_linux_processors[cluster_leader].max_frequency;
			}
		}
	}

	/* TODO: */
	if (clustered_processors != usable_processors) {
		/*
		 * Topology information about some or all logical processors may be unavailable, for the following reasons:
		 * - Linux kernel is too old, or configured without support for topology information in sysfs.
		 * - Core is offline, and Linux kernel is configured to not report topology for offline cores.
		 */
	}

	cpuinfo_powerpc_linux_count_cluster_processors(powerpc_linux_processors_count, powerpc_linux_processors);

	const uint32_t cluster_count =  cpuinfo_powerpc_linux_detect_cluster(
		powerpc_linux_processors_count, usable_processors, powerpc_linux_processors); /* TODO */

	const struct cpuinfo_powerpc_chipset chipset = {
		/* TODO(rcardoso): hardcoded. */
		.vendor = cpuinfo_powerpc_chipset_vendor_ibm,
	};

	cpuinfo_powerpc_chipset_to_string(&chipset, package.name);
	package.processor_count = usable_processors;
	package.core_count = usable_processors;
	package.cluster_count = cluster_count;

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

	clusters = calloc(cluster_count, sizeof(struct cpuinfo_cluster));
	if (clusters == NULL) {
		cpuinfo_log_error("failed to allocate %zu bytes for descriptions of %"PRIu32" core clusters",
			cluster_count * sizeof(struct cpuinfo_cluster), cluster_count);
			goto cleanup;
	}

	/* TBD */
	uint32_t l1i_count = usable_processors ;
	if (l1i_count != 0) {
		l1i = calloc(l1i_count, sizeof(struct cpuinfo_cache));
		if (l1i == NULL) {
			cpuinfo_log_error("failed to allocate %zu bytes for descriptions of %"PRIu32" L1I caches",
			l1i_count * sizeof(struct cpuinfo_cache), l1i_count);
			goto cleanup;
		}
	}

	/* TBD */
	uint32_t l1d_count = usable_processors;
	if (l1d_count != 0) {
		l1d = calloc(l1d_count, sizeof(struct cpuinfo_cache));
		if (l1d == NULL) {
			cpuinfo_log_error("failed to allocate %zu bytes for descriptions of %"PRIu32" L1D caches",
				l1d_count * sizeof(struct cpuinfo_cache), l1d_count);
				goto cleanup;
		}
	}

	/* TBD */
	uint32_t l2_count = usable_processors;
	if (l2_count != 0) {
		l2 = calloc(l2_count, sizeof(struct cpuinfo_cache));
		if (l2 == NULL) {
			cpuinfo_log_error("failed to allocate %zu bytes for descriptions of %"PRIu32" L2 caches",
				l2_count * sizeof(struct cpuinfo_cache), l2_count);
				goto cleanup;
		}
	}

	/* TBD */
	uint32_t l3_count = usable_processors;
	if (l3_count != 0) {
		l3 = calloc(l3_count, sizeof(struct cpuinfo_cache));
		if (l3 == NULL) {
			cpuinfo_log_error("failed to allocate %zu bytes for descriptions of %"PRIu32" L3 caches",
				l3_count * sizeof(struct cpuinfo_cache), l3_count);
				goto cleanup;
		}
	}

	/* TBD */
	linux_cpu_to_core_map = calloc(powerpc_linux_processors_count, sizeof(struct cpuinfo_core*));
	if (linux_cpu_to_core_map == NULL) {
		cpuinfo_log_error("failed to allocate %zu bytes for mapping entries of %"PRIu32" cores",
		powerpc_linux_processors_count * sizeof(struct cpuinfo_core*),
		powerpc_linux_processors_count);
		goto cleanup;
	}

	/* TBD */
	linux_cpu_to_processor_map = calloc(powerpc_linux_processors_count, sizeof(struct cpuinfo_processor*));
	if (linux_cpu_to_processor_map == NULL) {
		cpuinfo_log_error("failed to allocate %zu bytes for %"PRIu32" logical processor mapping entries",
			powerpc_linux_processors_count * sizeof(struct cpuinfo_processor*), powerpc_linux_processors_count);
		goto cleanup;
	}

	uint32_t cluster_id = UINT32_MAX;
	for (uint32_t i = 0; i < usable_processors; i++) {
		if (powerpc_linux_processors[i].package_leader_id == powerpc_linux_processors[i].system_processor_id) {
			cluster_id++;
		}
		processors[i].smt_id = i;
		processors[i].core = cores + i;
		processors[i].cluster = clusters + cluster_id;
		processors[i].package = &package;
		processors[i].linux_id = (int) powerpc_linux_processors[i].system_processor_id;
		processors[i].cache.l1i = l1i + i;
		processors[i].cache.l1d = l1d + i;
		processors[i].cache.l2 = l2 + i ;
		processors[i].cache.l3 = l3 + i ;
		cores[i].processor_start = i;
		//TODO: Hard wiring SMT=8 for now
		smt = 8;
		/* TODO(rcardoso): unused. */
		linux_cpu_to_processor_map[powerpc_linux_processors[i].system_processor_id] = &processors[i];
		cores[i].processor_count = smt;
		cores[i].core_id = i;
		cores[i].cluster = clusters + cluster_id;
		cores[i].package = &package;
		cores[i].vendor = powerpc_linux_processors[i].vendor;
		cores[i].uarch = powerpc_linux_processors[i].uarch;
		linux_cpu_to_core_map[powerpc_linux_processors[i].system_processor_id] = &cores[i];
		// Disable all by default
		cores[i].disabled = powerpc_linux_processors[i].disabled;
		/* Populate the cache information */
		(processors[i].smt_id, &l1i[i], &l1d[i], &l2[i], &l3[i]);
		l1i[i].processor_start = l1d[i].processor_start = 0;
		l1i[i].processor_count = l1d[i].processor_count = i;
		l2[i].processor_start = l3[i].processor_start = 0;
		l2[i].processor_count = l3[i].processor_count = i;
		l1d[i].partitions = l1i[i].partitions  = 1;

		if (powerpc_linux_processors[i].package_leader_id == powerpc_linux_processors[i].system_processor_id) {

			clusters[cluster_id] = (struct cpuinfo_cluster) {
				.processor_start = i,
				.processor_count = powerpc_linux_processors[i].package_processor_count,
				.core_start = i,
				.core_count = powerpc_linux_processors[i].package_processor_count,
				.cluster_id = cluster_id,
				.package = &package,
				.vendor = powerpc_linux_processors[i].vendor,
				.uarch = powerpc_linux_processors[i].uarch,
			};
		}
	}

	/* Commit */
	cpuinfo_linux_cpu_to_processor_map = linux_cpu_to_processor_map;
	cpuinfo_linux_cpu_to_core_map = linux_cpu_to_core_map;
	cpuinfo_processors = processors;
	cpuinfo_cores = cores;
	cpuinfo_packages = &package;
	cpuinfo_clusters = clusters;
	cpuinfo_cache[cpuinfo_cache_level_1i] = l1i;
	cpuinfo_cache[cpuinfo_cache_level_1d] = l1d;
	cpuinfo_cache[cpuinfo_cache_level_2]  = l2;
	cpuinfo_cache[cpuinfo_cache_level_3]  = l3;

	cpuinfo_processors_count = usable_processors;
	cpuinfo_cores_count = usable_processors;
	//cpuinfo_clusters_count = cluster_count;
	cpuinfo_packages_count = 1;
	cpuinfo_cache_count[cpuinfo_cache_level_1i] = usable_processors;
	cpuinfo_cache_count[cpuinfo_cache_level_1d] = usable_processors;
	cpuinfo_cache_count[cpuinfo_cache_level_2]  = usable_processors;
	cpuinfo_cache_count[cpuinfo_cache_level_3]  = usable_processors;

	linux_cpu_to_processor_map = NULL;
	linux_cpu_to_core_map = NULL;
	processors = NULL;
	cores = NULL;
	clusters = NULL;
	l1i = l1d = l2 = l3 = NULL;

cleanup:
	free(powerpc_linux_processors);
	free(linux_cpu_to_processor_map);
	free(linux_cpu_to_core_map);
	free(processors);
	free(cores);
	free(l1i);
	free(l1d);
	free(l2);
	free(l3);
}
