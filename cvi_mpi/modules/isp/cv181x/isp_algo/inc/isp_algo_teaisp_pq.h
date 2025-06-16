/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_teaisp_pq.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_TEAISP_PQ_H_
#define _ISP_ALGO_TEAISP_PQ_H_

#include "cvi_comm_isp.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

float auto_drc(CVI_U32 *pHist, float avg_ratio, int dark_th, float dark_compensate_ratio,
				int luma_th_left, int luma_th_right, float luma_compensate_ratio);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_TEAISP_PQ_H_
