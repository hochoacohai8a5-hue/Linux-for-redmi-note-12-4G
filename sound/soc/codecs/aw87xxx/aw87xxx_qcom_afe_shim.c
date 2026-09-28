// SPDX-License-Identifier: GPL-2.0-only
/*
 * Minimal QCOM AFE shims for the Awinic aw87xxx DSP path.
 *
 * Adapted from downstream/audio/dsp/q6afe.c (afe_get_topology,
 * aw_send_afe_cal_apr). The full downstream implementations need the
 * downstream AFE core (this_afe topology tracking), ION (msm_audio_ion_alloc),
 * rtac mapping (afe_map_rtac_block) and q6common pack helpers, none of which
 * exist upstream.
 *
 * These are only ever used for the DSP vmax/real-time path, which is
 * inactive: aw_dsp_isEnable() reports false (see aw_dsp.c, no DSP halo
 * upstream), so the monitor runs in no-DSP mode and never calls them.
 * The shims exist solely to satisfy the linker with honest semantics:
 * topology unknown (DSP not ready) and APR messaging unsupported.
 */

#include <linux/errno.h>
#include <linux/types.h>

int afe_get_topology(int port_id)
{
	/* unknown: no AFE topology tracking upstream, DSP stays off */
	return 0;
}

int aw_send_afe_cal_apr(uint32_t param_id, void *buf, int cmd_size,
			bool write)
{
	return -EOPNOTSUPP;
}
