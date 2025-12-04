#pragma once

#include "../inc/Renderer.h"

render_err_t Render_RawFrameProcessing_Init(void);

render_err_t Render_Send_Raw_Frame(Render_Frame_t* pFrameIn);

render_err_t Render_RawFrame_Process(void);

render_err_t Render_RawFramesProcessing_Dtr(void);
