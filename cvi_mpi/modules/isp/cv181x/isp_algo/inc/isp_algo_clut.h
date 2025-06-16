/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_clut.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_CLUT_H_
#define _ISP_ALGO_CLUT_H_

#include "cvi_comm_isp.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


struct clut_param_in {
	ISP_U16_PTR ClutR;
	ISP_U16_PTR ClutG;
	ISP_U16_PTR ClutB;
	ISP_VOID_PTR hsl_attr;
};

struct clut_param_out {
	CVI_BOOL isUpdated;
	CVI_U16 ClutR[ISP_CLUT_LUT_LENGTH];
	CVI_U16 ClutG[ISP_CLUT_LUT_LENGTH];
	CVI_U16 ClutB[ISP_CLUT_LUT_LENGTH];
};

CVI_S32 isp_algo_clut_main(CVI_U32 ViPipe, struct clut_param_in *clut_param_in, struct clut_param_out *clut_param_out);
CVI_S32 isp_algo_clut_init(CVI_U32 ViPipe);
CVI_S32 isp_algo_clut_uninit(CVI_U32 ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_CLUT_H_
