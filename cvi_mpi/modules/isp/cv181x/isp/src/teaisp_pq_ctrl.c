/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: teaisp_pq_ctrl.c
 * Description:
 *
 */

#include "isp_3a.h"
#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"
#include "isp_tun_buf_ctrl.h"
#include "teaisp_pq_ctrl.h"
#include "isp_mgr_buf.h"
#include "cvi_ae.h"
#include "isp_algo_teaisp_pq.h"
#include <math.h>

#define AR_EQ(arr0, arr1, arr2, size) \
({ \
	bool ret = true; \
	for (int i = 0; i < (size); i++) { \
		if ((arr1)[i] != (arr2)[i]) { \
			arr0[i] = arr2[i]; \
			ret = false; \
		} \
	} \
	ret; \
})

#define VAL_EQ(val0, val1, val2) \
({ \
	bool ret = true; \
	if ((val1) != (val2)) { \
		val0 = val2; \
		ret = false; \
	} \
	ret; \
})

#define AR_MUL(arr1, arr0, const_val, size, UPB) \
{ \
	for (int i = 0; i < (size); i++) { \
		(arr1)[i] = (arr0)[i] * (const_val); \
		if ((arr1)[i] > UPB) { \
			(arr1)[i] = UPB; \
		} \
	} \
}

#define PR_AR(arr, size) \
{ \
	ISP_LOG_DEBUG("%s:\n", #arr); \
	for (int i = 0; i < (size); i++) \
		ISP_LOG_DEBUG("%d, ", (arr)[i]); \
	ISP_LOG_DEBUG("\n"); \
}

const struct isp_module_ctrl teaisp_pq_mod = {
	.init = teaisp_pq_ctrl_init,
	.uninit = teaisp_pq_ctrl_uninit,
	.suspend = teaisp_pq_ctrl_suspend,
	.resume = teaisp_pq_ctrl_resume,
	.ctrl = teaisp_pq_ctrl_ctrl
};

typedef void (*ENTER_LEAVE_FUNC)(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime);

static CVI_S32 teaisp_pq_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 teaisp_pq_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 teaisp_pq_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 teaisp_pq_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 set_teaisp_pq_proc_info(VI_PIPE ViPipe);
static CVI_S32 teaisp_pq_ctrl_check_attr_valid(const TEAISP_PQ_ATTR_S *pstTEAISPPQAttr);

static struct teaisp_pq_ctrl_runtime  *_get_teaisp_pq_ctrl_runtime(VI_PIPE ViPipe);

// scene contorl func
static CVI_S32 teaisp_pq_postprocess_snow(VI_PIPE ViPipe);
static CVI_S32 teaisp_pq_postprocess_fog(VI_PIPE ViPipe);
static CVI_S32 teaisp_pq_postprocess_backlight(VI_PIPE ViPipe);
static CVI_S32 teaisp_pq_postprocess_grass(VI_PIPE ViPipe);
static CVI_S32 teaisp_pq_postprocess_common(VI_PIPE ViPipe);

static void enter_scene_snow(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime);
static void enter_scene_fog(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime);
static void enter_scene_backlight(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime);
static void enter_scene_grass(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime);
static void enter_scene_common(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime);

static void leave_scene_snow(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime);
static void leave_scene_fog(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime);
static void leave_scene_backlight(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime);
static void leave_scene_grass(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime);
static void leave_scene_common(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime);

static void scene_snow_tuning_algo(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime);
static void scene_fog_tuning_algo(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime);
static void scene_grass_tuning_algo(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime);
static void scene_backlight_tuning_algo(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime);
static void scene_backlight_tuning_algo_auto(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime);
//static void scene_common_tuning_algo(struct teaisp_pq_ctrl_runtime *runtime);

static ENTER_LEAVE_FUNC enter_scene_func[TEAISP_SCENE_NUM] = {
	enter_scene_snow,
	enter_scene_fog,
	enter_scene_backlight,
	enter_scene_grass,
	enter_scene_common
};

static ENTER_LEAVE_FUNC leave_scene_func[TEAISP_SCENE_NUM] = {
	leave_scene_snow,
	leave_scene_fog,
	leave_scene_backlight,
	leave_scene_grass,
	leave_scene_common
};

typedef void (*SCENE_BACKLIGHT_TUNING_ALGO)(VI_PIPE, struct teaisp_pq_ctrl_runtime *);

static SCENE_BACKLIGHT_TUNING_ALGO scene_backlight_tuning_algo_ls[] = {
	scene_backlight_tuning_algo,
	scene_backlight_tuning_algo_auto
};

CVI_S32 teaisp_pq_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_pq_ctrl_runtime *runtime = _get_teaisp_pq_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;
	runtime->is_module_update_scene = CVI_FALSE;
	runtime->last_scene_info.scene =
	runtime->module_set_scene_info.scene =
	runtime->module_algo_scene_info.scene =
	runtime->cur_scene_info.scene = SCENE_COMMON;
	runtime->algo_result = NULL;

	for (int i = 0; i < TEAISP_SCENE_NUM; ++i) {
		runtime->continue_scene_cnt[i] = 0;
		runtime->module_detect_sence_cfd[i] = 0.0f;
	}

	return ret;
}

CVI_S32 teaisp_pq_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 teaisp_pq_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 teaisp_pq_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 teaisp_pq_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_pq_ctrl_runtime *runtime = _get_teaisp_pq_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		teaisp_pq_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		// TODO
		break;
	case MOD_CMD_GET_MODCTRL:
		// TODO
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 teaisp_pq_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_U32 frame_id = algoResult->u32FrameIdx;

	ISP_LOG_INFO("---- post eof, frame id: %d, module: %s, ", frame_id, "TEAISP.PQ");

	teaisp_pq_ctrl_preprocess(ViPipe, algoResult);
	teaisp_pq_ctrl_process(ViPipe);
	teaisp_pq_ctrl_postprocess(ViPipe);

	set_teaisp_pq_proc_info(ViPipe);

	return ret;
}

static CVI_S32 teaisp_pq_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_pq_ctrl_runtime *runtime = _get_teaisp_pq_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL)
		return CVI_FAILURE;

	const TEAISP_PQ_ATTR_S *pteaisppq_attr = NULL;

	teaisp_pq_ctrl_get_pq_attr(ViPipe, &pteaisppq_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(pteaisppq_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;
	runtime->algo_result = algoResult;

	// No need to update parameters if disable. Because its meaningless
	if (!pteaisppq_attr->Enable || runtime->is_module_bypass)
		return ret;

	// ParamIn
	// ParamOut

	runtime->process_updated = CVI_TRUE;

	return ret;
}

static CVI_S32 teaisp_pq_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	const TEAISP_PQ_ATTR_S *pteaisppq_attr = NULL;
	struct teaisp_pq_ctrl_runtime *runtime = _get_teaisp_pq_ctrl_runtime(ViPipe);

	teaisp_pq_ctrl_get_pq_attr(ViPipe, &pteaisppq_attr);

	if (runtime == CVI_NULL)
		return CVI_FAILURE;

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	if (runtime->is_module_update_scene) {
		runtime->is_module_update_scene = CVI_FALSE;
		int current_detect_scene = runtime->module_set_scene_info.scene;
		unsigned int max_cnt = runtime->continue_scene_cnt[current_detect_scene] + 1;

		for (int i = 0; i < TEAISP_SCENE_NUM; ++i) {
			if (i == current_detect_scene) {
				runtime->continue_scene_cnt[i]++;
				runtime->module_detect_sence_cfd[i] += log(max_cnt) / log(2.0);
			} else {
				if (runtime->module_detect_sence_cfd[i] > 0.0f) {
					runtime->module_detect_sence_cfd[i] -= pow(2, max_cnt);
					if (runtime->module_detect_sence_cfd[i] < 0.0f) {
						runtime->module_detect_sence_cfd[i] = 0.0f;
						runtime->continue_scene_cnt[i] = 0;
					} else {
						if (runtime->continue_scene_cnt[i] > 0) {
							runtime->continue_scene_cnt[i] -= 1;
						}
					}
				} else {
					runtime->continue_scene_cnt[i] = 0;
				}
			}
		}

		if (runtime->continue_scene_cnt[current_detect_scene] >= pteaisppq_attr->SmoothThr) {
			memcpy(&runtime->module_algo_scene_info, &runtime->module_set_scene_info, sizeof(TEAISP_PQ_SCENE_INFO));
		}
	}

	if (pteaisppq_attr->TuningMode != 0) {
		// use TuningMode to set the scene: 1~5: snow, fog, backlight, grass, common
		memcpy(&runtime->last_scene_info, &runtime->cur_scene_info, sizeof(TEAISP_PQ_SCENE_INFO));
		runtime->cur_scene_info.scene = (pteaisppq_attr->TuningMode - 1) % TEAISP_SCENE_NUM;
		runtime->cur_scene_info.scene_score = 100;
	} else {
		// use the module detect algo result
		if (!pteaisppq_attr->SceneBypass[runtime->module_algo_scene_info.scene] ||
				runtime->module_algo_scene_info.scene_score >= pteaisppq_attr->SceneConfThres[runtime->module_set_scene_info.scene]) {
			memcpy(&runtime->last_scene_info, &runtime->cur_scene_info, sizeof(TEAISP_PQ_SCENE_INFO));
			memcpy(&runtime->cur_scene_info, &runtime->module_algo_scene_info, sizeof(TEAISP_PQ_SCENE_INFO));
		}
	}

	return ret;
}

static CVI_S32 teaisp_pq_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_pq_ctrl_runtime *runtime = _get_teaisp_pq_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL)
		return CVI_FAILURE;

	const TEAISP_PQ_ATTR_S *pteaisppq_attr = NULL;

	teaisp_pq_ctrl_get_pq_attr(ViPipe, &pteaisppq_attr);

	if (!pteaisppq_attr->Enable) {
		runtime->cur_scene_info.scene = SCENE_COMMON;
	}

	CVI_BOOL is_postprocess_update = runtime->postprocess_updated;

	if (is_postprocess_update == CVI_FALSE)
		return ret;

	switch (runtime->cur_scene_info.scene) {
	case SCENE_SNOW:
		teaisp_pq_postprocess_snow(ViPipe);
		break;
	case SCENE_FOG:
		teaisp_pq_postprocess_fog(ViPipe);
		break;
	case SCENE_BACKLIGHT:
		teaisp_pq_postprocess_backlight(ViPipe);
		break;
	case SCENE_GRASS:
		teaisp_pq_postprocess_grass(ViPipe);
		break;
	case SCENE_COMMON:
	default:
		teaisp_pq_postprocess_common(ViPipe);
		break;
	}

	return ret;
}

static CVI_S32 set_teaisp_pq_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

static struct teaisp_pq_ctrl_runtime  *_get_teaisp_pq_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct teaisp_pq_shared_buffer *shared_buf = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_TEAISP_PQ, (CVI_VOID *) &shared_buf);

	return &shared_buf->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 teaisp_pq_ctrl_check_attr_valid(const TEAISP_PQ_ATTR_S *pstTEAISPPQAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(pstTEAISPPQAttr);

	return ret;
}

static CVI_S32 teaisp_pq_postprocess_snow(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("process SCENE: SNOW ----\n");

	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_pq_ctrl_runtime *runtime = _get_teaisp_pq_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL)
		return CVI_FAILURE;

	if (runtime->cur_scene_info.scene != runtime->last_scene_info.scene)
		leave_scene_func[runtime->last_scene_info.scene](ViPipe, runtime);

	enter_scene_func[runtime->cur_scene_info.scene](ViPipe, runtime);

	return ret;
}

static CVI_S32 teaisp_pq_postprocess_fog(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("process SCENE: FOG ----\n");

	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_pq_ctrl_runtime *runtime = _get_teaisp_pq_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL)
		return CVI_FAILURE;

	if (runtime->cur_scene_info.scene != runtime->last_scene_info.scene)
		leave_scene_func[runtime->last_scene_info.scene](ViPipe, runtime);

	enter_scene_func[runtime->cur_scene_info.scene](ViPipe, runtime);

	return ret;
}

static CVI_S32 teaisp_pq_postprocess_backlight(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("process SCENE: BACKLIGHT ----\n");

	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_pq_ctrl_runtime *runtime = _get_teaisp_pq_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL)
		return CVI_FAILURE;

	if (runtime->cur_scene_info.scene != runtime->last_scene_info.scene)
		leave_scene_func[runtime->last_scene_info.scene](ViPipe, runtime);

	enter_scene_func[runtime->cur_scene_info.scene](ViPipe, runtime);

	return ret;
}

static CVI_S32 teaisp_pq_postprocess_grass(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("process SCENE: GRASS ----\n");

	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_pq_ctrl_runtime *runtime = _get_teaisp_pq_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL)
		return CVI_FAILURE;

	if (runtime->cur_scene_info.scene != runtime->last_scene_info.scene)
		leave_scene_func[runtime->last_scene_info.scene](ViPipe, runtime);

	enter_scene_func[runtime->cur_scene_info.scene](ViPipe, runtime);

	return ret;
}

static CVI_S32 teaisp_pq_postprocess_common(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("process SCENE: COMMON ----\n");

	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_pq_ctrl_runtime *runtime = _get_teaisp_pq_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL)
		return CVI_FAILURE;

	if (runtime->cur_scene_info.scene != runtime->last_scene_info.scene)
		leave_scene_func[runtime->last_scene_info.scene](ViPipe, runtime);

	enter_scene_func[runtime->cur_scene_info.scene](ViPipe, runtime);

	return ret;
}

static void enter_scene_snow(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime)
{
	ISP_LOG_DEBUG("--enter snow ---\n");

	ISP_EXPOSURE_ATTR_S exposure_attr;
	struct snow_tuning_attr *p0 = &(runtime->snow_tun_attr[0]);
	struct snow_tuning_attr *p1 = &(runtime->snow_tun_attr[1]);

	CVI_ISP_GetExposureAttr(ViPipe, &exposure_attr);

	ISP_LOG_DEBUG("cur secne: %d, last scene: %d\n", runtime->cur_scene_info.scene, runtime->last_scene_info.scene);
	if (runtime->cur_scene_info.scene == runtime->last_scene_info.scene) {
		ISP_LOG_DEBUG("scene not change!\n");
		// judge: [1] compare exposure_attr; update: exposure_attr -> [0]
		bool is_pqtool_update_attr = !(
			VAL_EQ(p0->ae_u16EVBias, p1->ae_u16EVBias, exposure_attr.stAuto.u16EVBias)
		);

		ISP_LOG_DEBUG("is_pqtool_udate_attr: %d\n", is_pqtool_update_attr);

		if (!is_pqtool_update_attr)
			return;

	} else {
		ISP_LOG_DEBUG("scene change\n");
		// update: exposure_attr -> [0], [1]
		p1->ae_u16EVBias = p0->ae_u16EVBias = exposure_attr.stAuto.u16EVBias;
	}

	// update: [0] -> [1]
	scene_snow_tuning_algo(ViPipe, runtime);

	// set exposure attribute
	exposure_attr.stAuto.u16EVBias = p1->ae_u16EVBias;

	CVI_ISP_SetExposureAttr(ViPipe, &exposure_attr);
}

static void enter_scene_fog(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime)
{
	ISP_LOG_DEBUG("--enter fog ---\n");

	const ISP_DEHAZE_ATTR_S *p_dehaze_attr_s = NULL;
	struct fog_tuning_attr *p0 = &(runtime->fog_tun_attr[0]);
	struct fog_tuning_attr *p1 = &(runtime->fog_tun_attr[1]);

	isp_dehaze_ctrl_get_dehaze_attr(ViPipe, &p_dehaze_attr_s);

	ISP_LOG_DEBUG("cur secne: %d, last scene: %d\n", runtime->cur_scene_info.scene, runtime->last_scene_info.scene);
	if (runtime->cur_scene_info.scene == runtime->last_scene_info.scene) {
		ISP_LOG_DEBUG("scene not change!\n");
		// judge: [1] compare p_dehaze_attr_s; update: p_dehaze_attr_s -> [0]
		bool is_pqtool_update_attr = !(
			VAL_EQ(p0->dehaze_enable, p1->dehaze_enable, p_dehaze_attr_s->Enable)
		);

		ISP_LOG_DEBUG("is_pqtool_udate_attr: %d\n", is_pqtool_update_attr);

		if (!is_pqtool_update_attr)
			return;

	} else {
		ISP_LOG_DEBUG("scene change\n");
		// update: p_dehaze_attr_s -> [0], [1]
		p1->dehaze_enable = p0->dehaze_enable = p_dehaze_attr_s->Enable;
	}

	// update: [0] -> [1]
	scene_fog_tuning_algo(ViPipe, runtime);

	// set dehaze attribute
	ISP_DEHAZE_ATTR_S dehaze_attr_set;

	memcpy(&dehaze_attr_set, p_dehaze_attr_s, sizeof(ISP_DEHAZE_ATTR_S));
	dehaze_attr_set.Enable = p1->dehaze_enable;

	isp_dehaze_ctrl_set_dehaze_attr(ViPipe, &dehaze_attr_set);
}

static void enter_scene_backlight(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime)
{
	ISP_LOG_DEBUG("--enter backlight ---\n");

	const ISP_DRC_ATTR_S *p_drc_attr = NULL;
	struct backlight_tuning_attr *p0 = &(runtime->backlight_tun_attr[0]);
	struct backlight_tuning_attr *p1 = &(runtime->backlight_tun_attr[1]);

	isp_drc_ctrl_get_drc_attr(ViPipe, &p_drc_attr);

	ISP_LOG_DEBUG("cur secne: %d, last scene: %d\n", runtime->cur_scene_info.scene, runtime->last_scene_info.scene);
	if (runtime->cur_scene_info.scene == runtime->last_scene_info.scene) {
		ISP_LOG_DEBUG("scene not change!\n");
		// judge: [1] compare p_drc_attr; update: drc -> [0]
		bool is_pqtool_update_attr = !(
			AR_EQ(p0->SdrTargetYGain, p1->SdrTargetYGain, p_drc_attr->stAuto.SdrTargetYGain, ISP_AUTO_LV_NUM)
			&& AR_EQ(p0->SdrDEAdaptTargetGain, p1->SdrDEAdaptTargetGain, p_drc_attr->stAuto.SdrDEAdaptTargetGain, ISP_AUTO_LV_NUM)
			&& AR_EQ(p0->TargetYScale, p1->TargetYScale, p_drc_attr->stAuto.TargetYScale, ISP_AUTO_LV_NUM)
			&& AR_EQ(p0->DEAdaptTargetGain, p1->DEAdaptTargetGain, p_drc_attr->stAuto.DEAdaptTargetGain, ISP_AUTO_LV_NUM)
		);

		ISP_LOG_DEBUG("is_pqtool_udate_attr: %d\n", is_pqtool_update_attr);

		if (!is_pqtool_update_attr)
			return;

	} else {
		ISP_LOG_DEBUG("scene change\n");
		// update: p_drc_attr -> [0], [1]
		memcpy(p0->SdrTargetYGain, p_drc_attr->stAuto.SdrTargetYGain, ISP_AUTO_LV_NUM * sizeof(CVI_U8));
		memcpy(p0->SdrDEAdaptTargetGain, p_drc_attr->stAuto.SdrDEAdaptTargetGain, ISP_AUTO_LV_NUM * sizeof(CVI_U8));
		memcpy(p0->TargetYScale, p_drc_attr->stAuto.TargetYScale, ISP_AUTO_LV_NUM * sizeof(CVI_U32));
		memcpy(p0->DEAdaptTargetGain, p_drc_attr->stAuto.DEAdaptTargetGain, ISP_AUTO_LV_NUM * sizeof(CVI_U8));

		memcpy(p1, p0, sizeof(struct backlight_tuning_attr));
	}

	// update: [0] -> [1]
	scene_backlight_tuning_algo_ls[1](ViPipe, runtime);

	// set drc attribute
	ISP_DRC_ATTR_S drc_attr_set;

	memcpy(&drc_attr_set, p_drc_attr, sizeof(ISP_DRC_ATTR_S));

	memcpy(drc_attr_set.stAuto.SdrTargetYGain, p1->SdrTargetYGain, ISP_AUTO_LV_NUM * sizeof(CVI_U8));
	memcpy(drc_attr_set.stAuto.SdrDEAdaptTargetGain, p1->SdrDEAdaptTargetGain, ISP_AUTO_LV_NUM * sizeof(CVI_U8));
	memcpy(drc_attr_set.stAuto.TargetYScale, p1->TargetYScale, ISP_AUTO_LV_NUM * sizeof(CVI_U32));
	memcpy(drc_attr_set.stAuto.DEAdaptTargetGain, p1->DEAdaptTargetGain, ISP_AUTO_LV_NUM * sizeof(CVI_U8));

	isp_drc_ctrl_set_drc_attr(ViPipe, &drc_attr_set);
}

static void enter_scene_grass(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime)
{
	ISP_LOG_DEBUG("--enter grass ---\n");

	const ISP_CLUT_HSL_ATTR_S *p_clut_hsl_attr = NULL;
	struct grass_tuning_attr *p0 = &(runtime->grass_tun_attr[0]);
	struct grass_tuning_attr *p1 = &(runtime->grass_tun_attr[1]);

	isp_clut_ctrl_get_clut_hsl_attr(ViPipe, &p_clut_hsl_attr);

	ISP_LOG_DEBUG("cur secne: %d, last scene: %d\n", runtime->cur_scene_info.scene, runtime->last_scene_info.scene);
	if (runtime->cur_scene_info.scene == runtime->last_scene_info.scene) {
		ISP_LOG_DEBUG("scene not change!\n");
		// judge: [1] compare p_clut_hsl_attr; update: p_clut_hsl_attr -> [0]
		bool is_pqtool_update_attr = !(
			AR_EQ(p0->LByH, p1->LByH, p_clut_hsl_attr->LByH, ISP_CLUT_HUE_LENGTH)
			&& AR_EQ(p0->SByH, p1->SByH, p_clut_hsl_attr->SByH, ISP_CLUT_HUE_LENGTH)
			&& VAL_EQ(p0->clut_hsl_enable, p1->clut_hsl_enable, p_clut_hsl_attr->Enable)
		);

		ISP_LOG_DEBUG("is_pqtool_udate_attr: %d\n", is_pqtool_update_attr);

		if (!is_pqtool_update_attr)
			return;

	} else {
		ISP_LOG_DEBUG("scene change\n");
		// update: p_clut_hsl_attr -> [0], [1]
		memcpy(p0->LByH, p_clut_hsl_attr->LByH, ISP_CLUT_HUE_LENGTH * sizeof(CVI_U16));
		memcpy(p0->SByH, p_clut_hsl_attr->SByH, ISP_CLUT_HUE_LENGTH * sizeof(CVI_U16));

		p1->clut_hsl_enable = p0->clut_hsl_enable = p_clut_hsl_attr->Enable;

		memcpy(p1, p0, sizeof(struct grass_tuning_attr));
	}

	// update: [0] -> [1]
	scene_grass_tuning_algo(ViPipe, runtime);

	// set clut attribute
	ISP_CLUT_HSL_ATTR_S clut_hsl_attr_set;

	memcpy(&clut_hsl_attr_set, p_clut_hsl_attr, sizeof(ISP_CLUT_HSL_ATTR_S));

	memcpy(clut_hsl_attr_set.LByH, p1->LByH, ISP_CLUT_HUE_LENGTH * sizeof(CVI_U16));
	memcpy(clut_hsl_attr_set.SByH, p1->SByH, ISP_CLUT_HUE_LENGTH * sizeof(CVI_U16));

	clut_hsl_attr_set.Enable = p1->clut_hsl_enable;

	isp_clut_ctrl_set_clut_hsl_attr(ViPipe, &clut_hsl_attr_set);
}

static void enter_scene_common(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime)
{
	ISP_LOG_DEBUG("--enter common ---\n");
	UNUSED(ViPipe);
	UNUSED(runtime);
}

static void leave_scene_snow(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime)
{
	ISP_LOG_DEBUG("--leave snow ---\n");

	ISP_EXPOSURE_ATTR_S exposure_attr_set;
	struct snow_tuning_attr *p0 = &runtime->snow_tun_attr[0];
	struct snow_tuning_attr *p1 = &runtime->snow_tun_attr[0];

	CVI_ISP_GetExposureAttr(ViPipe, &exposure_attr_set);

	// [0] -> [1]
	memcpy(p1, p0, sizeof(struct snow_tuning_attr));

	// [0] -> exposure_attr
	exposure_attr_set.stAuto.u16EVBias = p0->ae_u16EVBias;

	CVI_ISP_SetExposureAttr(ViPipe, &exposure_attr_set);
}

static void leave_scene_fog(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime)
{
	ISP_LOG_DEBUG("--leave fog ---\n");

	const ISP_DEHAZE_ATTR_S *p_dehaze_attr = NULL;
	ISP_DEHAZE_ATTR_S dehaze_attr_set;
	struct fog_tuning_attr *p0 = &runtime->fog_tun_attr[0];
	struct fog_tuning_attr *p1 = &runtime->fog_tun_attr[0];

	isp_dehaze_ctrl_get_dehaze_attr(ViPipe, &p_dehaze_attr);

	// [0] -> [1], dehaze_attr_set
	memcpy(&dehaze_attr_set, p_dehaze_attr, sizeof(ISP_DEHAZE_ATTR_S));
	dehaze_attr_set.Enable = p1->dehaze_enable = p0->dehaze_enable;

	isp_dehaze_ctrl_set_dehaze_attr(ViPipe, &dehaze_attr_set);
}

static void leave_scene_backlight(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime)
{
	ISP_LOG_DEBUG("--leave backlight ---\n");

	const ISP_DRC_ATTR_S *p_drc_attr = NULL;
	ISP_DRC_ATTR_S drc_attr_set;
	struct backlight_tuning_attr *p0 = &runtime->backlight_tun_attr[0];
	struct backlight_tuning_attr *p1 = &runtime->backlight_tun_attr[0];

	isp_drc_ctrl_get_drc_attr(ViPipe, &p_drc_attr);

	// [0] -> [1]
	memcpy(p1, p0, sizeof(struct backlight_tuning_attr));

	// [0] -> drc_attr
	memcpy(&drc_attr_set, p_drc_attr, sizeof(ISP_DRC_ATTR_S));

	memcpy(drc_attr_set.stAuto.SdrTargetYGain, p0->SdrTargetYGain, ISP_AUTO_LV_NUM * sizeof(CVI_U8));
	memcpy(drc_attr_set.stAuto.SdrDEAdaptTargetGain, p0->SdrDEAdaptTargetGain, ISP_AUTO_LV_NUM * sizeof(CVI_U8));
	memcpy(drc_attr_set.stAuto.TargetYScale, p0->TargetYScale, ISP_AUTO_LV_NUM * sizeof(CVI_U32));
	memcpy(drc_attr_set.stAuto.DEAdaptTargetGain, p0->DEAdaptTargetGain, ISP_AUTO_LV_NUM * sizeof(CVI_U8));

	isp_drc_ctrl_set_drc_attr(ViPipe, &drc_attr_set);
}

static void leave_scene_grass(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime)
{
	ISP_LOG_DEBUG("--leave grass ---\n");

	const ISP_CLUT_HSL_ATTR_S *p_clut_hsl_attr = NULL;
	ISP_CLUT_HSL_ATTR_S clut_hsl_attr_set;
	struct grass_tuning_attr *p0 = &runtime->grass_tun_attr[0];
	struct grass_tuning_attr *p1 = &runtime->grass_tun_attr[0];

	isp_clut_ctrl_get_clut_hsl_attr(ViPipe, &p_clut_hsl_attr);

	// [0] -> [1]
	memcpy(p1, p0, sizeof(struct grass_tuning_attr));

	// [0] -> clut_hsl_attr
	memcpy(&clut_hsl_attr_set, p_clut_hsl_attr, sizeof(ISP_CLUT_HSL_ATTR_S));

	memcpy(clut_hsl_attr_set.LByH, p0->LByH, ISP_CLUT_HUE_LENGTH);
	memcpy(clut_hsl_attr_set.SByH, p0->SByH, ISP_CLUT_HUE_LENGTH);

	clut_hsl_attr_set.Enable = p0->clut_hsl_enable;

	isp_clut_ctrl_set_clut_hsl_attr(ViPipe, &clut_hsl_attr_set);
}

static void leave_scene_common(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime)
{
	ISP_LOG_DEBUG("--leave common ---\n");
	UNUSED(ViPipe);
	UNUSED(runtime);
}

static void scene_backlight_tuning_algo(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime)
{
	struct backlight_tuning_attr *p0 = &(runtime->backlight_tun_attr[0]);
	struct backlight_tuning_attr *p1 = &(runtime->backlight_tun_attr[1]);
	ISP_ALGO_RESULT_S *p_algo_ret = runtime->algo_result;

	CVI_BOOL is_wdr_mode = (p_algo_ret->enFSWDRMode == WDR_MODE_NONE) ? 0 : 1;
	CVI_U32 wdr_exp_ratio = p_algo_ret->au32ExpRatio[0];
	CVI_U32 *p_hist256_val;
	ISP_AE_STATISTICS_COMPAT_S *p_ae_sts = NULL;

	isp_sts_ctrl_get_ae_sts(ViPipe, &p_ae_sts);

	if (p_ae_sts == NULL) {
		return;
	}

	p_hist256_val = p_ae_sts->aeStat1[0].au32HistogramMemArray[0];

	if (is_wdr_mode) {
		// exp ratio -> drc: auto.targetyscale, auto.deadaptargetgain
		if (wdr_exp_ratio <= 1000) {
			AR_MUL(p1->TargetYScale, p0->TargetYScale, 1.2f, ISP_AUTO_LV_NUM, 0x800);
			AR_MUL(p1->DEAdaptTargetGain, p0->DEAdaptTargetGain, 1.2f, ISP_AUTO_LV_NUM, 0x60);
		} else if (wdr_exp_ratio > 1000 && wdr_exp_ratio <= 1500) {
			AR_MUL(p1->TargetYScale, p0->TargetYScale, 1.4f, ISP_AUTO_LV_NUM, 0x800);
			AR_MUL(p1->DEAdaptTargetGain, p0->DEAdaptTargetGain, 1.4f, ISP_AUTO_LV_NUM, 0x60);
		} else {
			AR_MUL(p1->TargetYScale, p0->TargetYScale, 1.6f, ISP_AUTO_LV_NUM, 0x800);
			AR_MUL(p1->DEAdaptTargetGain, p0->DEAdaptTargetGain, 1.6f, ISP_AUTO_LV_NUM, 0x60);
		}

		ISP_LOG_DEBUG("wdr exp ratio's value: %d\n", wdr_exp_ratio);
	} else {
		// count AE Hist256 value -> drc: auto.sdrtargetygain, sdrdeadapttargetgain
		CVI_U32 hist_pixel_cnt = 0;
		CVI_U32 hist_ge255_pixel_cnt = p_hist256_val[HIST_NUM - 1];
		float hist_ge_255_ratio = 0.0f;

		for (int i = 0; i < HIST_NUM; ++i) {
			hist_pixel_cnt += p_hist256_val[i];
		}

		hist_ge_255_ratio = (float)hist_ge255_pixel_cnt / hist_pixel_cnt;

		if (hist_ge_255_ratio <= 0.4f) {
			AR_MUL(p1->SdrTargetYGain, p0->SdrTargetYGain, 1.2f, ISP_AUTO_LV_NUM, 0x80);
			AR_MUL(p1->SdrDEAdaptTargetGain, p0->SdrDEAdaptTargetGain, 1.2f, ISP_AUTO_LV_NUM, 0x40);
		} else if (hist_ge_255_ratio > 0.4f && hist_ge_255_ratio <= 0.7f) {
			AR_MUL(p1->SdrTargetYGain, p0->SdrTargetYGain, 1.4f, ISP_AUTO_LV_NUM, 0x80);
			AR_MUL(p1->SdrDEAdaptTargetGain, p0->SdrDEAdaptTargetGain, 1.4f, ISP_AUTO_LV_NUM, 0x40);

		} else {
			AR_MUL(p1->SdrTargetYGain, p0->SdrTargetYGain, 1.6f, ISP_AUTO_LV_NUM, 0x80);
			AR_MUL(p1->SdrDEAdaptTargetGain, p0->SdrDEAdaptTargetGain, 1.6f, ISP_AUTO_LV_NUM, 0x40);
		}
		ISP_LOG_DEBUG("hist ge 255 ratio: %f\n", hist_ge_255_ratio);
	}
}

static void scene_backlight_tuning_algo_auto(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime)
{
	struct backlight_tuning_attr *p0 = &(runtime->backlight_tun_attr[0]);
	struct backlight_tuning_attr *p1 = &(runtime->backlight_tun_attr[1]);
	ISP_ALGO_RESULT_S *p_algo_ret = runtime->algo_result;


	CVI_BOOL is_wdr_mode = (p_algo_ret->enFSWDRMode == WDR_MODE_NONE) ? 0 : 1;
	CVI_U32 wdr_exp_ratio = p_algo_ret->au32ExpRatio[0];

	if (is_wdr_mode) {
		// exp ratio -> drc: auto.targetyscale, auto.deadaptargetgain
		if (wdr_exp_ratio <= 1000) {
			AR_MUL(p1->TargetYScale, p0->TargetYScale, 1.2f, ISP_AUTO_LV_NUM, 0x800);
			AR_MUL(p1->DEAdaptTargetGain, p0->DEAdaptTargetGain, 1.2f, ISP_AUTO_LV_NUM, 0x60);
		} else if (wdr_exp_ratio > 1000 && wdr_exp_ratio <= 1500) {
			AR_MUL(p1->TargetYScale, p0->TargetYScale, 1.4f, ISP_AUTO_LV_NUM, 0x800);
			AR_MUL(p1->DEAdaptTargetGain, p0->DEAdaptTargetGain, 1.4f, ISP_AUTO_LV_NUM, 0x60);
		} else {
			AR_MUL(p1->TargetYScale, p0->TargetYScale, 1.6f, ISP_AUTO_LV_NUM, 0x800);
			AR_MUL(p1->DEAdaptTargetGain, p0->DEAdaptTargetGain, 1.6f, ISP_AUTO_LV_NUM, 0x60);
		}

		ISP_LOG_DEBUG("wdr exp ratio's value: %d\n", wdr_exp_ratio);
	} else {
		// count AE Hist256 value -> drc: auto.sdrtargetygain, sdrdeadapttargetgain
		float avg_ratio = 1.0;
		int dark_th = 64;
		float dark_compensate_ratio = 50.0f;
		int luma_th_left = 128;
		int luma_th_right = 192;
		float luma_compensate_ratio = 0.0;
		CVI_U32 *p_hist256_val;

		ISP_AE_STATISTICS_COMPAT_S *p_ae_sts = NULL;

		isp_sts_ctrl_get_ae_sts(ViPipe, &p_ae_sts);

		if (p_ae_sts == NULL) {
			return;
		}

		p_hist256_val = p_ae_sts->aeStat1[0].au32HistogramMemArray[0];

		float drc_level = auto_drc(p_hist256_val, avg_ratio, dark_th,
				dark_compensate_ratio, luma_th_left, luma_th_right, luma_compensate_ratio);

		drc_level = drc_level / 255;

		if (drc_level <= 0.3f) {
			AR_MUL(p1->SdrTargetYGain, p0->SdrTargetYGain, 1.2f, ISP_AUTO_LV_NUM, 0x80);
			AR_MUL(p1->SdrDEAdaptTargetGain, p0->SdrDEAdaptTargetGain, 1.2f, ISP_AUTO_LV_NUM, 0x40);
		} else if (drc_level > 0.3f && drc_level <= 0.7f) {
			AR_MUL(p1->SdrTargetYGain, p0->SdrTargetYGain, 1.4f, ISP_AUTO_LV_NUM, 0x80);
			AR_MUL(p1->SdrDEAdaptTargetGain, p0->SdrDEAdaptTargetGain, 1.4f, ISP_AUTO_LV_NUM, 0x40);

		} else {
			AR_MUL(p1->SdrTargetYGain, p0->SdrTargetYGain, 1.6f, ISP_AUTO_LV_NUM, 0x80);
			AR_MUL(p1->SdrDEAdaptTargetGain, p0->SdrDEAdaptTargetGain, 1.6f, ISP_AUTO_LV_NUM, 0x40);
		}
		ISP_LOG_DEBUG("hist ge 255 ratio: %f\n", hist_ge_255_ratio);
	}
}

static void scene_snow_tuning_algo(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime)
{
	UNUSED(ViPipe);

	struct snow_tuning_attr *p0 = &(runtime->snow_tun_attr[0]);
	struct snow_tuning_attr *p1 = &(runtime->snow_tun_attr[1]);

	if (p0->ae_u16EVBias * 2 > 0xffff) {
		p1->ae_u16EVBias = 0xffff;
	} else {
		p1->ae_u16EVBias = p0->ae_u16EVBias * 2;
	}
}

static void scene_fog_tuning_algo(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime)
{
	UNUSED(ViPipe);

	struct fog_tuning_attr *p1 = &(runtime->fog_tun_attr[1]);

	p1->dehaze_enable = 1;
}

static void scene_grass_tuning_algo(VI_PIPE ViPipe, struct teaisp_pq_ctrl_runtime *runtime)
{
	UNUSED(ViPipe);

	struct grass_tuning_attr *p0 = &(runtime->grass_tun_attr[0]);
	struct grass_tuning_attr *p1 = &(runtime->grass_tun_attr[1]);
	// clut (8 - 14)
	int clut_start_id = 7;
	int clut_end_id = 13;

	p1->clut_hsl_enable = 1;

	for (int i = clut_start_id; i <= clut_end_id; ++i) {
		if (p0->SByH[i] + 30 > 0x64) {
			p1->SByH[i] = 0x64;
		} else {
			p1->SByH[i] = p0->SByH[i] + 30;
		}

		if (p0->LByH[i] + 10 > 0x64) {
			p1->LByH[i] = 0x64;
		} else {
			p1->LByH[i] = p0->LByH[i] + 10;
		}
	}
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 teaisp_pq_ctrl_get_pq_attr(VI_PIPE ViPipe, const TEAISP_PQ_ATTR_S **pstTEAISPPQAttr)
{
	if (pstTEAISPPQAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_pq_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_TEAISP_PQ, (CVI_VOID *) &shared_buffer);
	*pstTEAISPPQAttr = &shared_buffer->stTEAISPPQAttr;

	return ret;
}

CVI_S32 teaisp_pq_ctrl_set_pq_attr(VI_PIPE ViPipe, const TEAISP_PQ_ATTR_S *pstTEAISPPQAttr)
{
	if (pstTEAISPPQAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_pq_ctrl_runtime *runtime = _get_teaisp_pq_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = teaisp_pq_ctrl_check_attr_valid(pstTEAISPPQAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const TEAISP_PQ_ATTR_S *p = CVI_NULL;

	teaisp_pq_ctrl_get_pq_attr(ViPipe, &p);
	memcpy((CVI_VOID *) p, pstTEAISPPQAttr, sizeof(*pstTEAISPPQAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

CVI_S32 teaisp_pq_ctrl_get_pq_scene(VI_PIPE ViPipe, TEAISP_PQ_SCENE_INFO *pstTEAISPPQSceneInfo)
{
	if (pstTEAISPPQSceneInfo == CVI_NULL)
		return CVI_FAILURE;

	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_pq_ctrl_runtime *runtime = _get_teaisp_pq_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL)
		return CVI_FAILURE;

	memcpy(pstTEAISPPQSceneInfo, &runtime->cur_scene_info, sizeof(TEAISP_PQ_SCENE_INFO));

	return ret;
}

CVI_S32 teaisp_pq_ctrl_set_pq_scene(VI_PIPE ViPipe, const TEAISP_PQ_SCENE_INFO *pstTEAISPPQSceneInfo)
{
	if (pstTEAISPPQSceneInfo == CVI_NULL)
		return CVI_FAILURE;

	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_pq_ctrl_runtime *runtime = _get_teaisp_pq_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL)
		return CVI_FAILURE;

	memcpy(&runtime->module_set_scene_info, pstTEAISPPQSceneInfo, sizeof(TEAISP_PQ_SCENE_INFO));
	runtime->is_module_update_scene = CVI_TRUE;

	return ret;
}

CVI_S32 teaisp_pq_ctrl_get_pq_detect_scene(VI_PIPE ViPipe, TEAISP_PQ_SCENE_INFO *pstTEAISPPQSceneInfo)
{
	if (pstTEAISPPQSceneInfo == CVI_NULL)
		return CVI_FAILURE;

	CVI_S32 ret = CVI_SUCCESS;
	struct teaisp_pq_ctrl_runtime *runtime = _get_teaisp_pq_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL)
		return CVI_FAILURE;

	memcpy(pstTEAISPPQSceneInfo, &runtime->module_algo_scene_info, sizeof(TEAISP_PQ_SCENE_INFO));

	return ret;
}
