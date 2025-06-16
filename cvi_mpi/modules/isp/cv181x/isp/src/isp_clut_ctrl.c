/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_clut_ctrl.c
 * Description:
 *
 */

#include "cvi_sys.h"

#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"
#include "isp_ioctl.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_interpolate.h"

#include "isp_clut_ctrl.h"
#include "isp_mgr_buf.h"

#define CLUT_CHANNEL_SIZE				(17)		// HW dependent
#define CLUT_PARTIAL_UPDATE_SIZE		(119)
#define CLUT_PARTIAL_UPDATE_TIMES		(42)		// 4913/CLUT_PARTIAL_UPDATE_SIZE
#define CLUT_OFFLINE_FULL_UPDATE_SYMBOL	(0xFF)

const struct isp_module_ctrl clut_mod = {
	.init = isp_clut_ctrl_init,
	.uninit = isp_clut_ctrl_uninit,
	.suspend = isp_clut_ctrl_suspend,
	.resume = isp_clut_ctrl_resume,
	.ctrl = isp_clut_ctrl_ctrl
};

static CVI_S32 isp_clut_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_clut_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_clut_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_clut_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 isp_clut_ctrl_check_clut_attr_valid(const ISP_CLUT_ATTR_S *pstCLUTAttr);
static CVI_S32 isp_clut_ctrl_check_clut_hsl_attr_valid(const ISP_CLUT_HSL_ATTR_S *pstClutHslAttr);
static CVI_S32 table_update_partial(VI_PIPE ViPipe);

static CVI_S32 set_clut_proc_info(VI_PIPE ViPipe);

static struct isp_clut_ctrl_runtime  *_get_clut_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_clut_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_clut_ctrl_runtime *runtime = _get_clut_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	isp_algo_clut_init(ViPipe);

	runtime->bUpdateFullMode = CVI_FALSE;
	runtime->updateFailCnt = 0;
	runtime->u8LastSyncIdx = 0;

	runtime->preprocess_updated = CVI_TRUE;
	runtime->is_updating = CVI_FALSE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;
	runtime->clut_param_out.isUpdated = CVI_FALSE;
	runtime->isLutNeedUpdate = CVI_FALSE;

	return ret;
}

CVI_S32 isp_clut_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	isp_algo_clut_uninit(ViPipe);

	return ret;
}

CVI_S32 isp_clut_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_clut_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_clut_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_clut_ctrl_runtime *runtime = _get_clut_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_clut_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassClut;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassClut = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_clut_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_clut_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_clut_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_clut_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_clut_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_clut_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_clut_ctrl_runtime *runtime = _get_clut_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_CLUT_ATTR_S *clut_attr;
	const ISP_CLUT_HSL_ATTR_S *clut_hsl_attr = NULL;

	isp_clut_ctrl_get_clut_attr(ViPipe, &clut_attr);
	isp_clut_ctrl_get_clut_hsl_attr(ViPipe, &clut_hsl_attr);

	CVI_BOOL is_preprocess_update = runtime->preprocess_updated;

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	if (clut_attr->Enable && !clut_hsl_attr->Enable)
		runtime->isLutNeedUpdate = CVI_TRUE;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!clut_attr->Enable || runtime->is_module_bypass || !clut_hsl_attr->Enable)
		return ret;

	// ParamIn
	runtime->clut_param_in.ClutR = ISP_PTR_CAST_PTR(clut_attr->ClutR);
	runtime->clut_param_in.ClutG = ISP_PTR_CAST_PTR(clut_attr->ClutG);
	runtime->clut_param_in.ClutB = ISP_PTR_CAST_PTR(clut_attr->ClutB);
	runtime->clut_param_in.hsl_attr = ISP_PTR_CAST_PTR(clut_hsl_attr);

	runtime->process_updated = CVI_TRUE;
	UNUSED(algoResult);

	return ret;
}

static CVI_S32 isp_clut_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_clut_ctrl_runtime *runtime = _get_clut_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_clut_main(ViPipe,
		(struct clut_param_in *)&runtime->clut_param_in,
		(struct clut_param_out *)&runtime->clut_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_clut_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_clut_ctrl_runtime *runtime = _get_clut_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = 0;

	struct cvi_vip_isp_clut_config *clut_cfg =
		(struct cvi_vip_isp_clut_config *)&(post_addr->tun_cfg[tun_idx].clut_cfg);

	const ISP_CLUT_ATTR_S *clut_attr = NULL;
	const ISP_CLUT_HSL_ATTR_S *clut_hsl_attr = NULL;
	const CVI_BOOL bIsMultiCam = IS_MULTI_CAM();

	isp_clut_ctrl_get_clut_attr(ViPipe, &clut_attr);
	isp_clut_ctrl_get_clut_hsl_attr(ViPipe, &clut_hsl_attr);

	CVI_BOOL is_postprocess_update = (runtime->postprocess_updated
									|| bIsMultiCam
									|| runtime->is_updating
									|| runtime->clut_param_out.isUpdated
									|| runtime->isLutNeedUpdate);

	if (bIsMultiCam) {
		runtime->bUpdateFullMode = CVI_TRUE;
	}

	CVI_BOOL enable = clut_attr->Enable && !runtime->is_module_bypass;

	if (is_postprocess_update == CVI_TRUE) {
		if (runtime->bUpdateFullMode) {
			clut_cfg->update = 1;
			clut_cfg->is_update_partial = 0;
			clut_cfg->enable = enable;
			if (runtime->clut_param_out.isUpdated) {
				if (clut_cfg->enable && clut_hsl_attr->Enable) {
					runtime->isLutNeedUpdate = CVI_FALSE;
					memcpy(clut_cfg->r_lut, runtime->clut_param_out.ClutR, sizeof(uint16_t) * ISP_CLUT_LUT_LENGTH);
					memcpy(clut_cfg->g_lut, runtime->clut_param_out.ClutG, sizeof(uint16_t) * ISP_CLUT_LUT_LENGTH);
					memcpy(clut_cfg->b_lut, runtime->clut_param_out.ClutB, sizeof(uint16_t) * ISP_CLUT_LUT_LENGTH);
				} else {
					runtime->clut_param_out.isUpdated = CVI_FALSE;
				}
			} else if (runtime->isLutNeedUpdate) {
				if (clut_cfg->enable) {
					memcpy(clut_cfg->r_lut, clut_attr->ClutR, sizeof(uint16_t) * ISP_CLUT_LUT_LENGTH);
					memcpy(clut_cfg->g_lut, clut_attr->ClutG, sizeof(uint16_t) * ISP_CLUT_LUT_LENGTH);
					memcpy(clut_cfg->b_lut, clut_attr->ClutB, sizeof(uint16_t) * ISP_CLUT_LUT_LENGTH);
				} else {
					runtime->isLutNeedUpdate = CVI_FALSE;
				}
			}
		} else {
			if (runtime->is_updating) {
				if (!enable) {
					runtime->is_updating = CVI_FALSE;
				}
			} else {
				if (runtime->clut_param_out.isUpdated) {
					if (enable && clut_hsl_attr->Enable) {
						runtime->is_updating = CVI_TRUE;
						runtime->isLutNeedUpdate = CVI_FALSE;
						runtime->lutR = runtime->clut_param_out.ClutR;
						runtime->lutG = runtime->clut_param_out.ClutG;
						runtime->lutB = runtime->clut_param_out.ClutB;
					}
					runtime->clut_param_out.isUpdated = CVI_FALSE;
				} else if (runtime->isLutNeedUpdate) {
					if (enable && !clut_hsl_attr->Enable) {
						runtime->is_updating = CVI_TRUE;
						runtime->lutR = clut_attr->ClutR;
						runtime->lutG = clut_attr->ClutG;
						runtime->lutB = clut_attr->ClutB;
					}
					runtime->isLutNeedUpdate = CVI_FALSE;
				}
			}

			if (runtime->is_updating) {
				table_update_partial(ViPipe);
			} else {
				if (!enable) {
					clut_cfg->update = 1;
					clut_cfg->is_update_partial = 0;
					clut_cfg->enable = 0;
				}
			}
		}
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 table_update_partial(VI_PIPE ViPipe)
{
	struct isp_clut_ctrl_runtime *runtime = _get_clut_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = 0;
	struct cvi_vip_isp_clut_config *clut_cfg =
		(struct cvi_vip_isp_clut_config *)&(post_addr->tun_cfg[tun_idx].clut_cfg);

	CVI_U32 u32CurSyncIdx;

	G_EXT_CTRLS_VALUE(VI_IOCTL_GET_CLUT_TBL_IDX, tun_idx, &u32CurSyncIdx);
	if ((u32CurSyncIdx != runtime->u8LastSyncIdx) && (runtime->u8LastSyncIdx != 0)) {
		if (runtime->updateFailCnt++ > 50) {
			runtime->updateFailCnt = 0;
			ISP_LOG_ERR("ViPipe=%d,clut table partial update fail reset index.\n", ViPipe);
		}
	} else {
		runtime->updateFailCnt = 0;
		if (runtime->u8LastSyncIdx == CLUT_PARTIAL_UPDATE_TIMES) {
			clut_cfg->update = 0;
			clut_cfg->is_update_partial = 0;
			runtime->u8LastSyncIdx = 0;
			runtime->is_updating = CVI_FALSE;
		} else {
			CVI_U32 u32DataCount = 0;
			CVI_U32 u32LutIdx;
			CVI_U32 b_idx, g_idx, r_idx;
			CVI_U32 b_value, g_value, r_value;

			u32LutIdx = runtime->u8LastSyncIdx * CLUT_PARTIAL_UPDATE_SIZE;

			for (CVI_U32 idx = 0; ((idx < CLUT_PARTIAL_UPDATE_SIZE)
				&& (u32LutIdx < ISP_CLUT_LUT_LENGTH));
				++idx, ++u32LutIdx) {

				b_idx = u32LutIdx / (CLUT_CHANNEL_SIZE * CLUT_CHANNEL_SIZE);
				g_idx = (u32LutIdx / CLUT_CHANNEL_SIZE) % CLUT_CHANNEL_SIZE;
				r_idx = u32LutIdx % CLUT_CHANNEL_SIZE;

				b_value = runtime->lutB[u32LutIdx];
				g_value = runtime->lutG[u32LutIdx];
				r_value = runtime->lutR[u32LutIdx];

				// 0: addr, 1: value
				clut_cfg->lut[idx][0] = (b_idx << 16) + (g_idx << 8) + r_idx;
				clut_cfg->lut[idx][1] = ((r_value & 0x3FF) << 20)
					+ ((g_value & 0x3FF) << 10) + (b_value & 0x3FF);
				u32DataCount++;
			}
			runtime->u8LastSyncIdx++;
			clut_cfg->tbl_idx = runtime->u8LastSyncIdx;
			clut_cfg->update_length = u32DataCount;
			clut_cfg->update = 1;
			clut_cfg->is_update_partial = 1;
			if (clut_cfg->tbl_idx == CLUT_PARTIAL_UPDATE_TIMES) {
				clut_cfg->enable = 1;
			}
		}
	}

	return CVI_SUCCESS;
}

static CVI_S32 set_clut_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef ENABLE_ISP_PROC_DEBUG
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		const ISP_CLUT_ATTR_S *clut_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		isp_clut_ctrl_get_clut_attr(ViPipe, &clut_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->CLutEnable = clut_attr->Enable;
	}
#else
	UNUSED(ViPipe);
#endif
	return ret;
}

static struct isp_clut_ctrl_runtime  *_get_clut_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_clut_shared_buffer *shared_buf = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CLUT, (CVI_VOID *) &shared_buf);

	return &shared_buf->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_clut_ctrl_check_clut_attr_valid(const ISP_CLUT_ATTR_S *pstCLUTAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstCLUTAttr, Enable, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstCLUTAttr, UpdateInterval, 0, 0xff);
	CHECK_VALID_ARRAY_1D(pstCLUTAttr, ClutR, ISP_CLUT_LUT_LENGTH, 0x0, 0x3ff);
	CHECK_VALID_ARRAY_1D(pstCLUTAttr, ClutG, ISP_CLUT_LUT_LENGTH, 0x0, 0x3ff);
	CHECK_VALID_ARRAY_1D(pstCLUTAttr, ClutB, ISP_CLUT_LUT_LENGTH, 0x0, 0x3ff);

	return ret;
}

static CVI_S32 isp_clut_ctrl_check_clut_hsl_attr_valid(const ISP_CLUT_HSL_ATTR_S *pstClutHslAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstClutHslAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_ARRAY_1D(pstClutHslAttr, HByH, ISP_CLUT_HUE_LENGTH, -0x1e, 0x1e);
	CHECK_VALID_ARRAY_1D(pstClutHslAttr, SByH, ISP_CLUT_HUE_LENGTH, 0x0, 0x64);
	CHECK_VALID_ARRAY_1D(pstClutHslAttr, LByH, ISP_CLUT_HUE_LENGTH, 0x0, 0x64);
	CHECK_VALID_ARRAY_1D(pstClutHslAttr, SByS, ISP_CLUT_SAT_LENGTH, 0x0, 0x64);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_clut_ctrl_get_clut_attr(VI_PIPE ViPipe, const ISP_CLUT_ATTR_S **pstCLUTAttr)
{
	if (pstCLUTAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_clut_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CLUT, (CVI_VOID *) &shared_buffer);
	*pstCLUTAttr = &shared_buffer->stCLUTAttr;

	return ret;
}

CVI_S32 isp_clut_ctrl_set_clut_attr(VI_PIPE ViPipe, const ISP_CLUT_ATTR_S *pstCLUTAttr)
{
	if (pstCLUTAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_clut_ctrl_runtime *runtime = _get_clut_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_clut_ctrl_check_clut_attr_valid(pstCLUTAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_CLUT_ATTR_S *p = CVI_NULL;

	isp_clut_ctrl_get_clut_attr(ViPipe, &p);
	memcpy((CVI_VOID *) p, pstCLUTAttr, sizeof(*pstCLUTAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return CVI_SUCCESS;
}

CVI_S32 isp_clut_ctrl_get_clut_hsl_attr(VI_PIPE ViPipe,
	const ISP_CLUT_HSL_ATTR_S **pstClutHslAttr)
{
	if (pstClutHslAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_clut_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CLUT, (CVI_VOID *) &shared_buffer);
	*pstClutHslAttr = &shared_buffer->stClutHslAttr;

	return ret;
}

CVI_S32 isp_clut_ctrl_set_clut_hsl_attr(VI_PIPE ViPipe,
	const ISP_CLUT_HSL_ATTR_S *pstClutHslAttr)
{
	if (pstClutHslAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_clut_ctrl_runtime *runtime = _get_clut_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_clut_ctrl_check_clut_hsl_attr_valid(pstClutHslAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_CLUT_HSL_ATTR_S *p = CVI_NULL;

	isp_clut_ctrl_get_clut_hsl_attr(ViPipe, &p);
	memcpy((CVI_VOID *) p, pstClutHslAttr, sizeof(*pstClutHslAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return CVI_SUCCESS;
}

