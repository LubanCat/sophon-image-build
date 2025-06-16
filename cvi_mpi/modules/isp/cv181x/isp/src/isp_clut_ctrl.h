/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_clut_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_CLUT_CTRL_H_
#define _ISP_CLUT_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_clut.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_clut_ctrl_runtime {
	struct clut_param_in clut_param_in;
	struct clut_param_out clut_param_out;

	CVI_BOOL bUpdateFullMode;
	CVI_U8 u8LastSyncIdx;
	CVI_U8 updateFailCnt;
	const CVI_U16 *lutR;
	const CVI_U16 *lutG;
	const CVI_U16 *lutB;

	CVI_BOOL preprocess_updated;
	CVI_BOOL isLutNeedUpdate;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_updating;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_clut_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_clut_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_clut_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_clut_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_clut_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_clut_ctrl_get_clut_attr(VI_PIPE ViPipe, const ISP_CLUT_ATTR_S **pstCLUTAttr);
CVI_S32 isp_clut_ctrl_set_clut_attr(VI_PIPE ViPipe, const ISP_CLUT_ATTR_S *pstCLUTAttr);
CVI_S32 isp_clut_ctrl_get_clut_hsl_attr(VI_PIPE ViPipe,
	const ISP_CLUT_HSL_ATTR_S **pstClutHslAttr);
CVI_S32 isp_clut_ctrl_set_clut_hsl_attr(VI_PIPE ViPipe,
	const ISP_CLUT_HSL_ATTR_S *pstClutHslAttr);

extern const struct isp_module_ctrl clut_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_CLUT_CTRL_H_
