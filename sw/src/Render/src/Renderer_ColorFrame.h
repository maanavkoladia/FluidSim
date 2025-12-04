#pragma once

#include "../inc/Renderer.h"

render_err_t Render_ColorFramesProcessing_Init(void);

render_err_t Render_Send_Frame_Colors(Render_Frame_Colors_t* pFrameIn);

render_err_t Render_ColorFrame_Process(void);

render_err_t Render_ColorFramesProcessing_Dtr(void);
