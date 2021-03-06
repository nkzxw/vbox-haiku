/* $Id$ */

/** @file
 * VBox OpenGL windows ICD driver functions
 */

/*
 * Copyright (C) 2006-2008 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "cr_error.h"
#include "icd_drv.h"
#include "cr_gl.h"
#include "stub.h"
#include "cr_mem.h"

#include <windows.h>

//TODO: consider
/* We can modify chronium dispatch table functions order to match the one required by ICD,
 * but it'd render us incompatible with other chromium SPUs and require more changes.
 * In current state, we can use unmodified binary chromium SPUs. Question is do we need it?
*/

#define GL_FUNC(func) cr_gl##func

static ICDTABLE icdTable = { 336, {
#define ICD_ENTRY(func) (PROC)GL_FUNC(func),
#include "VBoxICDList.h"
#undef ICD_ENTRY
} };

static GLuint desiredVisual = CR_RGB_BIT;

/**
 * Compute a mask of CR_*_BIT flags which reflects the attributes of
 * the pixel format of the given hdc.
 */
static GLuint ComputeVisBits( HDC hdc )
{
    PIXELFORMATDESCRIPTOR pfd; 
    int iPixelFormat;
    GLuint b = 0;

    iPixelFormat = GetPixelFormat( hdc );

    DescribePixelFormat( hdc, iPixelFormat, sizeof(pfd), &pfd );

    if (pfd.cDepthBits > 0)
        b |= CR_DEPTH_BIT;
    if (pfd.cAccumBits > 0)
        b |= CR_ACCUM_BIT;
    if (pfd.cColorBits > 8)
        b |= CR_RGB_BIT;
    if (pfd.cStencilBits > 0)
        b |= CR_STENCIL_BIT;
    if (pfd.cAlphaBits > 0)
        b |= CR_ALPHA_BIT;
    if (pfd.dwFlags & PFD_DOUBLEBUFFER)
        b |= CR_DOUBLE_BIT;
    if (pfd.dwFlags & PFD_STEREO)
        b |= CR_STEREO_BIT;

    return b;
}

void APIENTRY DrvReleaseContext(HGLRC hglrc)
{
    /*crDebug( "DrvReleaseContext(0x%x) called", hglrc );*/
    stubMakeCurrent( NULL, NULL );
}

BOOL APIENTRY DrvValidateVersion(DWORD version)
{
    if (stubInit()) {
        crDebug("DrvValidateVersion %x -> TRUE\n", version);
        return TRUE;
    }

    crDebug("DrvValidateVersion %x -> FALSE, going to use system default opengl32.dll\n", version); 
    return FALSE;
}

//we're not going to change icdTable at runtime, so callback is unused
PICDTABLE APIENTRY DrvSetContext(HDC hdc, HGLRC hglrc, void *callback)
{
    ContextInfo *context;
    WindowInfo *window;
    BOOL ret;

    /*crDebug( "DrvSetContext called(0x%x, 0x%x)", hdc, hglrc );*/
    (void) (callback);

    crHashtableLock(stub.windowTable);
    crHashtableLock(stub.contextTable);

    context = (ContextInfo *) crHashtableSearch(stub.contextTable, (unsigned long) hglrc);
    window = stubGetWindowInfo(hdc);

    ret = stubMakeCurrent(window, context);

    crHashtableUnlock(stub.contextTable);
    crHashtableUnlock(stub.windowTable);

    return ret ? &icdTable:NULL;
}

BOOL APIENTRY DrvSetPixelFormat(HDC hdc, int iPixelFormat)
{
    crDebug( "DrvSetPixelFormat(0x%x, %i) called.", hdc, iPixelFormat );

    if ( (iPixelFormat<1) || (iPixelFormat>2) ) {
        crError( "wglSetPixelFormat: iPixelFormat=%d?", iPixelFormat );
    }

    return 1;
}

HGLRC APIENTRY DrvCreateContext(HDC hdc)
{
    char dpyName[MAX_DPY_NAME];
    ContextInfo *context;

    crDebug( "DrvCreateContext(0x%x) called.", hdc);

    stubInit();

    CRASSERT(stub.contextTable);

    sprintf(dpyName, "%d", hdc);
    if (stub.haveNativeOpenGL)
        desiredVisual |= ComputeVisBits( hdc );

    context = stubNewContext(dpyName, desiredVisual, UNDECIDED, 0);
    if (!context)
        return 0;

    return (HGLRC) context->id;
}

HGLRC APIENTRY DrvCreateLayerContext(HDC hdc, int iLayerPlane)
{
    crDebug( "DrvCreateLayerContext(0x%x, %i) called.", hdc, iLayerPlane);
    //We don't support more than 1 layers.
    if (iLayerPlane == 0) {
        return DrvCreateContext(hdc);
    } else {
        crError( "DrvCreateLayerContext (%x,%x): unsupported", hdc, iLayerPlane);
        return NULL;
    }
    
}

BOOL APIENTRY DrvDescribeLayerPlane(HDC hdc,int iPixelFormat,
                                    int iLayerPlane, UINT nBytes,
                                    LPLAYERPLANEDESCRIPTOR plpd)
{
    crWarning( "DrvDescribeLayerPlane: unimplemented" );
    CRASSERT(false);
    return 0;
}

int APIENTRY DrvGetLayerPaletteEntries(HDC hdc, int iLayerPlane,
                                       int iStart, int cEntries,
                                       COLORREF *pcr)
{
    crWarning( "DrvGetLayerPaletteEntries: unsupported" );
    CRASSERT(false);
    return 0;
}

int APIENTRY DrvDescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR pfd)
{
    if ( !pfd ) {
        return 2;
    }

    if ( nBytes != sizeof(*pfd) ) {
        crWarning( "DrvDescribePixelFormat: nBytes=%u?", nBytes );
        return 2;
    }

    if (iPixelFormat==1)
    {
        crMemZero(pfd, sizeof(*pfd));

        pfd->nSize           = sizeof(*pfd);
        pfd->nVersion        = 1;
        pfd->dwFlags         = (PFD_DRAW_TO_WINDOW |
                                PFD_SUPPORT_OPENGL |
                                PFD_DOUBLEBUFFER);
        pfd->iPixelType      = PFD_TYPE_RGBA;
        pfd->cColorBits      = 32;
        pfd->cRedBits        = 8;
        pfd->cRedShift       = 24;
        pfd->cGreenBits      = 8;
        pfd->cGreenShift     = 16;
        pfd->cBlueBits       = 8;
        pfd->cBlueShift      = 8;
        pfd->cAlphaBits      = 8;
        pfd->cAlphaShift     = 0;
        pfd->cAccumBits      = 0;
        pfd->cAccumRedBits   = 0;
        pfd->cAccumGreenBits = 0;
        pfd->cAccumBlueBits  = 0;
        pfd->cAccumAlphaBits = 0;
        pfd->cDepthBits      = 32;
        pfd->cStencilBits    = 8;
        pfd->cAuxBuffers     = 0;
        pfd->iLayerType      = PFD_MAIN_PLANE;
        pfd->bReserved       = 0;
        pfd->dwLayerMask     = 0;
        pfd->dwVisibleMask   = 0;
        pfd->dwDamageMask    = 0;
    }
    else
    {
        crMemZero(pfd, sizeof(*pfd));
        pfd->nVersion        = 1;
        pfd->dwFlags         = (PFD_DRAW_TO_WINDOW|
                                PFD_SUPPORT_OPENGL);

        pfd->iPixelType      = PFD_TYPE_RGBA;
        pfd->cColorBits      = 32;
        pfd->cRedBits        = 8;
        pfd->cRedShift       = 16;
        pfd->cGreenBits      = 8;
        pfd->cGreenShift     = 8;
        pfd->cBlueBits       = 8;
        pfd->cBlueShift      = 0;
        pfd->cAlphaBits      = 0;
        pfd->cAlphaShift     = 0;
        pfd->cAccumBits      = 64;
        pfd->cAccumRedBits   = 16;
        pfd->cAccumGreenBits = 16;
        pfd->cAccumBlueBits  = 16;
        pfd->cAccumAlphaBits = 0;
        pfd->cDepthBits      = 16;
        pfd->cStencilBits    = 8;
        pfd->cAuxBuffers     = 0;
        pfd->iLayerType      = PFD_MAIN_PLANE;
        pfd->bReserved       = 0;
        pfd->dwLayerMask     = 0;
        pfd->dwVisibleMask   = 0;
        pfd->dwDamageMask    = 0;
    }

    /* the max PFD index */
    return 2;
}

BOOL APIENTRY DrvDeleteContext(HGLRC hglrc)
{
    /*crDebug( "DrvDeleteContext(0x%x) called", hglrc );*/
    stubDestroyContext( (unsigned long) hglrc );
    return 1;
}

BOOL APIENTRY DrvCopyContext(HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask)
{
    crWarning( "DrvCopyContext: unsupported" );
    return 0;
}

BOOL APIENTRY DrvShareLists(HGLRC hglrc1, HGLRC hglrc2)
{
    crWarning( "DrvShareLists: unsupported" );
    return 1;
}

int APIENTRY DrvSetLayerPaletteEntries(HDC hdc, int iLayerPlane,
                                       int iStart, int cEntries,
                                       CONST COLORREF *pcr)
{
    crWarning( "DrvSetLayerPaletteEntries: unsupported" );
    return 0;
}


BOOL APIENTRY DrvRealizeLayerPalette(HDC hdc, int iLayerPlane, BOOL bRealize)
{
    crWarning( "DrvRealizeLayerPalette: unsupported" );
    return 0;
}

BOOL APIENTRY DrvSwapLayerBuffers(HDC hdc, UINT fuPlanes)
{
    if (fuPlanes == 1)
    {
        return DrvSwapBuffers(hdc);
    }
    else
    {
        crWarning( "DrvSwapLayerBuffers: unsupported" );
        CRASSERT(false);
        return 0;
    }
}

BOOL APIENTRY DrvSwapBuffers(HDC hdc)
{
    WindowInfo *window;
    /*crDebug( "DrvSwapBuffers(0x%x) called", hdc );*/
    window = stubGetWindowInfo(hdc);    
    stubSwapBuffers( window, 0 );
    return 1;
}
