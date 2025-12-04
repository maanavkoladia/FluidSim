#include "../inc/Renderer.h"
#include "../../config.h"

#ifdef OFF_SCREEN_RENDERING
#    include "Renderer_OffScreen.h"
#else
#    include "Renderer_Display.h"
#endif

// extern void TransForm_RawFrameYeild(Render_Frame_t* pFrame);
// extern void TransForm_ColorFrameYeild(Render_Frame_Colors_t* pFrame);

// -----------------------------------------------------------------------------
// Frame / Init / Window Management
// -----------------------------------------------------------------------------
//

#if defined(__APPLE__)
render_err_t Render_Init(void) {
    LOG("Render Starting Up");
    AtomicFlag_Clear(&killFlag);
    // create the sim snap fifo
    LF_Fifo_Init(&frameInFifo, FRAME_IN_FIFO_SIZE);
    Task_Renderer(NULL);
    // LOG("Renderer Init Success");
    return RENDER_SUCCESS;
}

#else
render_err_t Render_Init(void) {

#    ifdef OFF_SCREEN_RENDERING
    OffScreenRender_Init();
#    else
    DisplayService_Init();
#    endif
    return RENDER_SUCCESS;
}

#endif

render_err_t Render_Dtr(void) {

#ifdef OFF_SCREEN_RENDERING
    OffScreenRender_Dtr();
#else
    DisplayService_Dtr();
#endif
    return RENDER_SUCCESS;
}
