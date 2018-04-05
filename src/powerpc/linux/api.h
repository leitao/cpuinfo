#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <cpuinfo.h>
#include <powerpc/api.h>
#include <linux/api.h>

/* No hard limit in the kernel, maximum length observed on non-rogue kernels is 64 */
#define CPUINFO_HARDWARE_VALUE_MAX 64

struct cpuinfo_powerpc_linux_proc_cpuinfo_cache {
	uint32_t i_size;
	uint32_t i_assoc;
	uint32_t i_line_length;
	uint32_t i_sets;
	uint32_t d_size;
	uint32_t d_assoc;
	uint32_t d_line_length;
	uint32_t d_sets;
};

struct cpuinfo_powerpc_linux_processor {
	uint32_t architecture_version;
	uint32_t architecture_flags;
	struct cpuinfo_powerpc_linux_proc_cpuinfo_cache proc_cpuinfo_cache;
	uint64_t features;

	/**
	 * Main PVR Register value.
	 */
	uint32_t pvr;
	enum cpuinfo_vendor vendor;
	enum cpuinfo_uarch uarch;
	/**
	 * ID of the physical package which includes this logical processor.
	 * The value is parsed from /sys/devices/system/cpu/cpu<N>/topology/physical_package_id
	 */
	uint32_t package_id;
	/**
	 * Minimum processor ID on the package which includes this logical processor.
	 * This value can serve as an ID for the cluster of logical processors: it is the
	 * same for all logical processors on the same package.
	 */
	uint32_t package_leader_id;
	/**
	 * Number of logical processors in the package.
	 */
	uint32_t package_processor_count;
	/**
	 * Maximum frequency, in kHZ.
	 * The value is parsed from /sys/devices/system/cpu/cpu<N>/cpufreq/cpuinfo_max_freq
	 * If failed to read or parse the file, the value is 0.
	 */
	uint32_t max_frequency;
	/**
	 * Minimum frequency, in kHZ.
	 * The value is parsed from /sys/devices/system/cpu/cpu<N>/cpufreq/cpuinfo_min_freq
	 * If failed to read or parse the file, the value is 0.
	 */
	uint32_t min_frequency;
	/** Linux processor ID */
	uint32_t system_processor_id;
	uint32_t flags;

	bool disabled;
};

uint64_t cpuinfo_arm_linux_hwcap_from_getauxval(void);

void cpuinfo_ppc64_linux_decode_isa_from_proc_cpuinfo(
		uint64_t features,
		struct cpuinfo_powerpc_isa isa[restrict static 1]);

bool cpuinfo_powerpc_linux_parse_proc_cpuinfo(
        char hardware[restrict static CPUINFO_HARDWARE_VALUE_MAX],
        uint32_t max_processors_count,
        struct cpuinfo_powerpc_linux_processor processors[restrict static max_processors_count]);

void cpuinfo_powerpc_linux_count_cluster_processors(
		uint32_t max_processors,
		struct cpuinfo_powerpc_linux_processor processors[restrict static max_processors]);

#define CPUINFO_POWERPC_LINUX_VALID_PROCESSOR    UINT32_C(0x00200000)
