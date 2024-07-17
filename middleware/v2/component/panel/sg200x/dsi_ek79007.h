#ifndef _MIPI_TX_PARAM_EK79007_H_
#define _MIPI_TX_PARAM_EK79007_H_

#include <linux/vo_mipi_tx.h>
#include <linux/cvi_comm_mipi_tx.h>

struct combo_dev_cfg_s dev_cfg_ek79007_600x1024 = {
	.devno = 0,
	.lane_id = {MIPI_TX_LANE_0, MIPI_TX_LANE_1, MIPI_TX_LANE_CLK, MIPI_TX_LANE_2, MIPI_TX_LANE_3},
	.lane_pn_swap = {false, false, false, false, false},
	.output_mode = OUTPUT_MODE_DSI_VIDEO,
	.video_mode = BURST_MODE,
	.output_format = OUT_FORMAT_RGB_24_BIT,
	.sync_info = {
		.vid_hsa_pixels = 10,
		.vid_hbp_pixels = 160,
		.vid_hfp_pixels = 160,
		.vid_hline_pixels = 1024,
		.vid_vsa_lines = 1,
		.vid_vbp_lines = 23,
		.vid_vfp_lines = 12,
		.vid_active_lines = 600,
		.vid_vsa_pos_polarity = false,
		.vid_hsa_pos_polarity = true,
	},
	.pixel_clk = 51669,
};

const struct hs_settle_s hs_timing_cfg_ek79007_600x1024 = { .prepare = 6, .zero = 32, .trail = 1 };

static CVI_U8 data_ek79007_0[] = { 0x80, 0xac };
static CVI_U8 data_ek79007_1[] = { 0x81, 0xb8 };
static CVI_U8 data_ek79007_2[] = { 0x82, 0x09 };
static CVI_U8 data_ek79007_3[] = { 0x83, 0x78 };
static CVI_U8 data_ek79007_4[] = { 0x84, 0x7f };
static CVI_U8 data_ek79007_5[] = { 0x85, 0xbb };
static CVI_U8 data_ek79007_6[] = { 0x86, 0x70 };


const struct dsc_instr dsi_init_cmds_ek79007_600x1024[] = {
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_ek79007_0 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_ek79007_1 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_ek79007_2 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_ek79007_3 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_ek79007_4 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_ek79007_5 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_ek79007_6 },
};

#else
#error "MIPI_TX_PARAM multi-delcaration!!"
#endif // _MIPI_TX_PARAM_EK79007_H_