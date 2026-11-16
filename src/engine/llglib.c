/* Low-level graphics: video/GX setup, frame-buffer swapping and screen queries. */
#include <dolphin/gx/GXFifo.h>
#include <dolphin/gx/GXFrameBuffer.h>
#include <dolphin/gx/GXManage.h>
#include <dolphin/gx/GXPixel.h>
#include <dolphin/gx/GXTransform.h>
#include <dolphin/os/OSCache.h>
#include <dolphin/vi/vifuncs.h>
#include "engine/llglib.h"
#include "game/fn_801D2B7C.h"

extern GlibConfig_t lbl_803E91F8;
extern GlibState_t lbl_803E9224;
extern unsigned char lbl_803EBC68;
extern int lbl_803EBC6C;
extern void (*lbl_803EBC74)(void);
extern void (*lbl_803EBC78)(void);
extern unsigned char lbl_803EBC87;
extern unsigned char lbl_803EBC88;
extern int lbl_803ECF00;
extern unsigned char lbl_803ECF04;
extern unsigned char lbl_803ECF05;
extern unsigned char lbl_803ECF0C;
extern int lbl_803ED634;
extern int lbl_803ED638;
extern int lbl_803ED63C;
extern GXRenderModeObj lbl_802F7A4C;
extern GXRenderModeObj lbl_802F7A88;

void *fn_801C1F94();
void *fn_801C1FBC();
void fn_801F7B04(void);
void GXSetProjection(float (*pMtx)[4], int type);
void GlibRenderBegin(GlibState_t *pMode);
void TMRenderBeginFrame(void);
void TMRenderEndFrame(void);
extern unsigned char lbl_80383DA0[];

int GlibVsync(int a);
void GlibClearZBuffer(void);
void GlibSetCopyFilter(GXBool enable);
static void _GlibScissor(void);
static void _GlibSetupScissor(GlibConfig_t *pConfig);

static int _GlibLogBase2(unsigned int value)
{
    int bits = 0;
    unsigned int v;

    for (v = value >> 1; v; v >>= 1) {
        bits++;
    }
    if (value & (value - 1)) {
        bits++;
    }
    return bits;
}

static void _GlibSnapFrameBuffer(void)
{
}

void GlibSetVsyncCallback(void (*callback)(void))
{
    lbl_803EBC74 = callback;
}

void GlibSetVsyncCallbackAfter(void (*callback)(void))
{
    lbl_803EBC78 = callback;
}

void GlibRemoveVsyncCallback(void)
{
    lbl_803EBC74 = 0;
}

void GlibSetVsyncFlipEnable(int enable)
{
    lbl_803EBC6C = enable;
}

static void _GlibLLDrawDone(void)
{
    lbl_803ED638 = -lbl_803ED634;
}

static void _Glib_VIRetraceCallback(u32 retraceCount)
{
    fn_801F7B04();
    if (lbl_803EBC74) {
        lbl_803EBC74();
    }
    if (lbl_803EBC87) {
        GXDisableBreakPt();
    }
    if (lbl_803EBC78) {
        lbl_803EBC78();
    }
}

void GlibEnableVIRetraceCallback(int enable)
{
    if (enable) {
        VISetPostRetraceCallback(_Glib_VIRetraceCallback);
    } else {
        VISetPostRetraceCallback(0);
    }
}

int GlibLLInit(GlibConfig_t *pConfig)
{
    GlibConfig_t *pCfg;
    GlibState_t *pMode;
    GXRenderModeObj *pSrcMode = 0;
    void *pFifo;

    lbl_803E91F8 = *pConfig;
    pCfg = &lbl_803E91F8;
    pMode = &lbl_803E9224;
    lbl_803ECF05 = 0;
    switch (pCfg->mTvMode) {
    case 0:
        pSrcMode = &GXNtsc240Ds;
        break;
    case 1:
        pSrcMode = &GXNtsc240DsAa;
        break;
    case 2:
        pSrcMode = &GXNtsc240Int;
        break;
    case 3:
        pSrcMode = &GXNtsc240IntAa;
        break;
    case 4:
        pSrcMode = &GXNtsc480IntDf;
        break;
    case 5:
        pSrcMode = &GXNtsc480Int;
        break;
    case 6:
        pSrcMode = &GXNtsc480IntAa;
        break;
    case 7:
        pSrcMode = &lbl_802F7A4C;
        lbl_803ECF05 = 1;
        break;
    case 8:
        pSrcMode = &lbl_802F7A88;
        break;
    }
    fn_801C1F94(pMode, 0, 0x3C);
    if (pCfg->mTvMode == 7) {
        GXAdjustForOverscan(pSrcMode, &pMode->mMode, 0, 16);
    } else if (pCfg->mTvMode == 8) {
        GXAdjustForOverscan(pSrcMode, &pMode->mMode, 0, 16);
    } else {
        GXAdjustForOverscan(pSrcMode, &pMode->mMode, 0, 16);
    }
    lbl_803ECF00 = ((pMode->mMode.fbWidth + 15) & 0xFFF0) * pMode->mMode.xfbHeight * 2;
    pMode->mpXfb[1] = pMode->mpXfb[0] = fn_801D2B7C(lbl_803ECF00, 0, 0);
    pMode->mFrame = 0;
    lbl_803ECF04 = 0;
    VIConfigure(&pMode->mMode);
    VISetNextFrameBuffer(pMode->mpXfb[pMode->mFrame]);
    pMode->mLog2Width = _GlibLogBase2(pCfg->mWidth);
    pMode->mLog2Height = _GlibLogBase2(pCfg->mHeight);
    pFifo = fn_801D2B7C(pCfg->mFifoSize, 0, 0);
    DCInvalidateRange(pFifo, pCfg->mFifoSize);
    pMode->mpFifoBuffer = pFifo;
    pMode->mpFifo = GXInit(pFifo, pCfg->mFifoSize);
    pMode->mFifoSize = pCfg->mFifoSize;
    GXSetCopyClear(pCfg->mClearColor, 0xFFFFFF);
    GXSetViewport(0.0f, 0.0f, (unsigned int)pMode->mMode.fbWidth, (unsigned int)pMode->mMode.xfbHeight, 0.0f, 1.0f);
    GXSetScissor(0, 0, pMode->mMode.fbWidth, pMode->mMode.efbHeight);
    GXSetDispCopySrc(0, 0, pMode->mMode.fbWidth, pMode->mMode.efbHeight);
    GXSetDispCopyDst(pMode->mMode.fbWidth, pMode->mMode.xfbHeight);
    GXSetDispCopyYScale((float)(unsigned int)pMode->mMode.xfbHeight / (float)(unsigned int)pMode->mMode.efbHeight);
    GlibSetCopyFilter(1);
    if (pMode->mMode.aa) {
        GXSetPixelFmt(2, 0);
    } else {
        GXSetPixelFmt(0, 0);
    }
    VISetBlack(1);
    VIFlush();
    GXCopyDisp(pMode->mpXfb[pMode->mFrame], 1);
    VIFlush();
    GXCopyDisp(pMode->mpXfb[pMode->mFrame], 1);
    VIFlush();
    GlibVsync(0);
    if (pMode->mMode.viTVmode & 1) {
        VIWaitForRetrace();
    }
    VISetBlack(0);
    GXSetDrawDoneCallback(_GlibLLDrawDone);
    GlibEnableVIRetraceCallback(1);
    _GlibSetupScissor(pCfg);
    lbl_803EBC68 = 1;
    return 0;
}

int GlibLLShutdown(void)
{
    return 0;
}

void GlibSetUpRender(void)
{
    void *a;
    void *b;

    if (lbl_803ECF0C) {
        GXFlush();
        GXGetFifoPtrs(GXGetCPUFifo(), &a, &b);
        GXEnableBreakPt(b);
    }
    GlibRenderBegin(&lbl_803E9224);
    TMRenderBeginFrame();
}

static inline void _GlibWaitDrawDone(int count)
{
    GXWaitDrawDone();
    lbl_803ED63C = count;
}

void GlibFinishRender(void)
{
    if (!lbl_803EBC68) {
        GlibClearZBuffer();
    }
    TMRenderEndFrame();
    GXSetDrawDone();
    GXFlush();
    if (lbl_803ECF0C) {
        GXDisableBreakPt();
        _GlibWaitDrawDone(0);
    }
}

int GlibVsync(int a)
{
    VIWaitForRetrace();
    return 0;
}

static void _GlibSetBreakpoint(void)
{
    void *a;
    void *b;

    GXFlush();
    GXGetFifoPtrs(GXGetCPUFifo(), &a, &b);
    GXEnableBreakPt(b);
}

void GlibSwapBuffers(void)
{
    if (lbl_803ECF04) {
        VISetNextFrameBuffer(lbl_803E9224.mpXfb[lbl_803E9224.mFrame]);
        VIFlush();
        VIWaitForRetrace();
        lbl_803E9224.mFrame ^= 1;
        _GlibScissor();
    } else {
        GXDrawDone();
        _GlibSnapFrameBuffer();
        VISetNextFrameBuffer(lbl_803E9224.mpXfb[0]);
        VIFlush();
        if (lbl_803EBC88) {
            _GlibSetBreakpoint();
            lbl_803EBC87 = 1;
        } else {
            VIWaitForRetrace();
        }
        lbl_803ED634 = 0;
        GXSetColorUpdate(1);
        GXSetZMode(1, 3, 1);
        GXCopyDisp(lbl_803E9224.mpXfb[0], lbl_803EBC68);
        GXPixModeSync();
        lbl_803E9224.mFrame = 0;
    }
}

void GlibSetVIInt(unsigned char enable)
{
    lbl_803EBC88 = enable;
}

void GlibSyncFrame(void)
{
    GXDrawDone();
}

unsigned char GlibGetFrame(void)
{
    return lbl_803E9224.mFrame;
}

int GlibGetZDepthBits(void)
{
    return 0;
}

void *GlibGetFrameBufferAddr(int frame)
{
    return lbl_803E9224.mpXfb[frame];
}

void GlibClearZBuffer(void)
{
    GlibState_t *pMode = &lbl_803E9224;

    GXSetColorUpdate(0);
    GXSetZMode(0, 0, 1);
    GXSetDispCopyDst(0, 0);
    GXCopyDisp(lbl_80383DA0, 1);
    GXSetColorUpdate(1);
    GXSetZMode(1, 3, 1);
    GXSetDispCopyDst(pMode->mMode.fbWidth, pMode->mMode.xfbHeight);
}

void GlibSetZBufferEnable(GXBool enable)
{
    GXSetZMode(enable, 3, 1);
}

int GlibGetScreenWidth(void)
{
    return lbl_803E91F8.mWidth;
}

int GlibGetScreenHeight(void)
{
    return lbl_803E91F8.mHeight;
}

void GlibSetProjection(float (*pMatrix)[4])
{
    GXSetProjection(pMatrix, 0);
}

void GlibSetCopyFilter(GXBool enable)
{
    GXSetCopyFilter(lbl_803E9224.mMode.aa, lbl_803E9224.mMode.sample_pattern, enable, lbl_803E9224.mMode.vfilter);
}

void GlibSetClear(unsigned char clear)
{
    lbl_803EBC68 = clear;
}

void GlibSwitchToSingle(void)
{
    GlibState_t *pMode = &lbl_803E9224;

    VISetNextFrameBuffer(pMode->mpXfb[0]);
    VIFlush();
    VIWaitForRetrace();
    fn_801D2BD0(pMode->mpXfb[1]);
    lbl_803ECF04 = 0;
    pMode->mFrame = 0;
    pMode->mpXfb[1] = pMode->mpXfb[0];
}

void GlibSwitchToDouble(int skipSwap)
{
    if (!skipSwap) {
        GlibSwapBuffers();
        GlibSwapBuffers();
    }
    lbl_803E9224.mpXfb[1] = fn_801D2B7C(lbl_803ECF00, 0, 0);
    fn_801C1FBC(lbl_803E9224.mpXfb[1], lbl_803E9224.mpXfb[0], lbl_803ECF00);
    lbl_803ECF04 = 1;
}

int GlibGetLetterBoxMode(void)
{
    return 0xFFFF;
}

int GlibGetWidescreenMode(void)
{
    return 0xFFFF;
}

void GlibToggleProgressive(int progressive)
{
    if (lbl_803ECF05 != progressive) {
        lbl_803ECF05 = progressive;
        VISetBlack(1);
        if (progressive) {
            GXAdjustForOverscan(&lbl_802F7A4C, &lbl_803E9224.mMode, 0, 16);
        } else {
            GXAdjustForOverscan(&GXNtsc480IntDf, &lbl_803E9224.mMode, 0, 16);
        }
        VIConfigure(&lbl_803E9224.mMode);
        VIFlush();
        VIWaitForRetrace();
        VIWaitForRetrace();
        VIWaitForRetrace();
        VISetBlack(0);
        VIFlush();
        VIWaitForRetrace();
    }
}

static void _GlibScissor(void)
{
}

static void _GlibSetupScissor(GlibConfig_t *pConfig)
{
    GXSetScissor(0, 0, pConfig->mWidth - 1, pConfig->mHeight - 1);
}

void GlibSetScissor(int left, int width, int top, int height)
{
    GXSetScissor(left, top, width, height);
}

int GlibGetScreenLetterBoxHeight(void)
{
    return 0x150;
}

int GlibGetScreenLetterBoxWidth(void)
{
    return 0x280;
}

unsigned short GlibGetXFBHeight(void)
{
    return lbl_803E9224.mMode.xfbHeight;
}
