/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: teaisp_pq_ctrl.h
 * Description:
 *
 */

#ifndef _TEAISP_PQ_CTRL_H_
#define _TEAISP_PQ_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct snow_tuning_attr {
	// AE attribute: EVBias * 2, range: 0 - 65535
	CVI_U16 ae_u16EVBias;
};

struct fog_tuning_attr {
	// dehaze
	CVI_BOOL dehaze_enable;
};

struct backlight_tuning_attr {
	// drc
	CVI_U8 SdrTargetYGain[ISP_AUTO_LV_NUM];
	CVI_U8 SdrDEAdaptTargetGain[ISP_AUTO_LV_NUM];
	CVI_U32 TargetYScale[ISP_AUTO_LV_NUM];
	CVI_U8 DEAdaptTargetGain[ISP_AUTO_LV_NUM];
};

struct grass_tuning_attr {
	CVI_BOOL clut_hsl_enable;
	// clut: SByH: +30%; LByH: +10% (8 - 14 bit)
	CVI_U16 LByH[ISP_CLUT_HUE_LENGTH];
	CVI_U16 SByH[ISP_CLUT_HUE_LENGTH];
};

struct  common_tuning_attr {
	CVI_U8 resv;
};

struct teaisp_pq_ctrl_runtime {
	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;

	CVI_BOOL is_module_update_scene;
	TEAISP_PQ_SCENE_INFO last_scene_info;
	TEAISP_PQ_SCENE_INFO cur_scene_info;
	TEAISP_PQ_SCENE_INFO module_set_scene_info;
	TEAISP_PQ_SCENE_INFO module_algo_scene_info;
	CVI_U64 continue_scene_cnt[TEAISP_SCENE_NUM];
	CVI_FLOAT module_detect_sence_cfd[TEAISP_SCENE_NUM];

	ISP_ALGO_RESULT_S *algo_result;

	struct snow_tuning_attr snow_tun_attr[2];
	struct fog_tuning_attr fog_tun_attr[2];
	struct backlight_tuning_attr backlight_tun_attr[2];
	struct grass_tuning_attr grass_tun_attr[2];
	struct common_tuning_attr common_tun_attr[2];
};

CVI_S32 teaisp_pq_ctrl_init(VI_PIPE ViPipe);
CVI_S32 teaisp_pq_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 teaisp_pq_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 teaisp_pq_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 teaisp_pq_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 teaisp_pq_ctrl_get_pq_attr(VI_PIPE ViPipe, const TEAISP_PQ_ATTR_S **ppstTEAISPPQAttr);
CVI_S32 teaisp_pq_ctrl_set_pq_attr(VI_PIPE ViPipe, const TEAISP_PQ_ATTR_S *pstTEAISPPQAttr);

CVI_S32 teaisp_pq_ctrl_set_pq_scene(VI_PIPE ViPipe, const TEAISP_PQ_SCENE_INFO *pstTEAISPPQSceneInfo);
CVI_S32 teaisp_pq_ctrl_get_pq_scene(VI_PIPE ViPipe, TEAISP_PQ_SCENE_INFO *pstTEAISPPQSceneInfo);
CVI_S32 teaisp_pq_ctrl_get_pq_detect_scene(VI_PIPE ViPipe, TEAISP_PQ_SCENE_INFO *pstTEAISPPQSceneInfo);

extern const struct isp_module_ctrl teaisp_pq_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _TEAISP_PQ_CTRL_H_

