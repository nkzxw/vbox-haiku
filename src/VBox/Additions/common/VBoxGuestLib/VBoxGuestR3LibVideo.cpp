/* $Id$ */
/** @file
 * VBoxGuestR3Lib - Ring-3 Support Library for VirtualBox guest additions, Video.
 */

/*
 * Copyright (C) 2007 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <iprt/string.h>
#include <iprt/mem.h>
#include <iprt/assert.h>
#include <VBox/log.h>

#include "VBGLR3Internal.h"


/**
 * Enable or disable video acceleration.
 *
 * @returns VBox status code.
 *
 * @param   fEnable       Pass zero to disable, any other value to enable.
 */
VBGLR3DECL(int) VbglR3VideoAccelEnable(bool fEnable)
{
    VMMDevVideoAccelEnable Req;
    vmmdevInitRequest(&Req.header, VMMDevReq_VideoAccelEnable);
    Req.u32Enable = fEnable;
    Req.cbRingBuffer = VBVA_RING_BUFFER_SIZE;
    Req.fu32Status = 0;
    return vbglR3GRPerform(&Req.header);
}


/**
 * Flush the video buffer.
 *
 * @returns VBox status code.
 */
VBGLR3DECL(int) VbglR3VideoAccelFlush(void)
{
    VMMDevVideoAccelFlush Req;
    vmmdevInitRequest(&Req.header, VMMDevReq_VideoAccelFlush);
    return vbglR3GRPerform(&Req.header);
}


/**
 * Send mouse pointer shape information to the host.
 *
 * @returns VBox status code.
 *
 * @param   fFlags      Mouse pointer flags.
 * @param   xHot        X coordinate of hot spot.
 * @param   yHot        Y coordinate of hot spot.
 * @param   cx          Pointer width.
 * @param   cy          Pointer height.
 * @param   pvImg       Pointer to the image data (can be NULL).
 * @param   cbImg       Size of the image data pointed to by pvImg.
 */
VBGLR3DECL(int) VbglR3SetPointerShape(uint32_t fFlags, uint32_t xHot, uint32_t yHot, uint32_t cx, uint32_t cy, const void *pvImg, size_t cbImg)
{
    VMMDevReqMousePointer *pReq;
    int rc = vbglR3GRAlloc((VMMDevRequestHeader **)&pReq, RT_OFFSETOF(VMMDevReqMousePointer, pointerData) + cbImg, VMMDevReq_SetPointerShape);
    if (RT_SUCCESS(rc))
    {
        pReq->fFlags = fFlags;
        pReq->xHot = xHot;
        pReq->yHot = yHot;
        pReq->width = cx;
        pReq->height = cy;
        if (pvImg)
            memcpy(pReq->pointerData, pvImg, cbImg);

        rc = vbglR3GRPerform(&pReq->header);
        vbglR3GRFree(&pReq->header);
        if (RT_SUCCESS(rc))
            rc = pReq->header.rc;
    }
    return rc;
}


/**
 * Send mouse pointer shape information to the host.
 * This version of the function accepts a request for clients that
 * already allocate and manipulate the request structure directly.
 *
 * @returns VBox status code.
 *
 * @param   pReq        Pointer to the VMMDevReqMousePointer structure.
 */
VBGLR3DECL(int) VbglR3SetPointerShapeReq(VMMDevReqMousePointer *pReq)
{
    int rc = vbglR3GRPerform(&pReq->header);
    if (RT_SUCCESS(rc))
        rc = pReq->header.rc;
    return rc;
}


/**
 * Query the last display change request.
 *
 * @returns iprt status value
 * @param   pcx         Where to store the horizontal pixel resolution (0 = do not change).
 * @param   pcy         Where to store the vertical pixel resolution (0 = do not change).
 * @param   pcBits      Where to store the bits per pixel (0 = do not change).
 * @param   iDisplay    Where to store the display number the request was for - 0 for the
 *                      primary display, 1 for the first secondary, etc.
 */
VBGLR3DECL(int) VbglR3GetLastDisplayChangeRequest(uint32_t *pcx, uint32_t *pcy, uint32_t *pcBits, uint32_t *piDisplay)
{
    VMMDevDisplayChangeRequest2 Req = { { 0 } };

#ifndef VBOX_VBGLR3_XFREE86
    AssertPtrReturn(pcx, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pcy, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pcBits, VERR_INVALID_PARAMETER);
    AssertPtrReturn(piDisplay, VERR_INVALID_PARAMETER);
#endif
vmmdevInitRequest(&Req.header, VMMDevReq_GetDisplayChangeRequest2);
    int rc = vbglR3GRPerform(&Req.header);
    if (RT_SUCCESS(rc))
        rc = Req.header.rc;
    if (RT_SUCCESS(rc))
    {
        *pcx = Req.xres;
        *pcy = Req.yres;
        *pcBits = Req.bpp;
        *piDisplay = Req.display;
    }
    return rc;
}

/**
 * Wait for a display change request event from the host.  These events must have been
 * activated previously using VbglR3CtlFilterMask.
 *
 * @returns IPRT status value
 * @param   pcx       On success, where to return the requested display width.
 *                    0 means no change.
 * @param   pcy       On success, where to return the requested display height.
 *                    0 means no change.
 * @param   pcBits    On success, where to return the requested bits per pixel.
 *                    0 means no change.
 * @param   piDisplay On success, where to return the index of the display to be changed.
 */
VBGLR3DECL(int) VbglR3DisplayChangeWaitEvent(uint32_t *pcx, uint32_t *pcy, uint32_t *pcBits, uint32_t *piDisplay)
{
    VBoxGuestWaitEventInfo waitEvent;
    int rc;

#ifndef VBOX_VBGLR3_XFREE86
    AssertPtrReturn(pcx, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pcy, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pcBits, VERR_INVALID_PARAMETER);
    AssertPtrReturn(piDisplay, VERR_INVALID_PARAMETER);
#endif
    waitEvent.u32TimeoutIn = RT_INDEFINITE_WAIT;
    waitEvent.u32EventMaskIn = VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST;
    waitEvent.u32Result = VBOXGUEST_WAITEVENT_ERROR;
    waitEvent.u32EventFlagsOut = 0;
    rc = vbglR3DoIOCtl(VBOXGUEST_IOCTL_WAITEVENT, &waitEvent, sizeof(waitEvent));
    if (RT_SUCCESS(rc))
    {
        /* did we get the right event? */
        if (waitEvent.u32EventFlagsOut & VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST)
        {
            VMMDevDisplayChangeRequest2 Req = { { 0 } };
            vmmdevInitRequest(&Req.header, VMMDevReq_GetDisplayChangeRequest2);
            Req.eventAck = VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST;
            int rc = vbglR3GRPerform(&Req.header);
            if (RT_SUCCESS(rc))
                rc = Req.header.rc;
            if (RT_SUCCESS(rc))
            {
                *pcx = Req.xres;
                *pcy = Req.yres;
                *pcBits = Req.bpp;
                *piDisplay = Req.display;
            }
        }
        else
            rc = VERR_TRY_AGAIN;
    }
    return rc;
}

/**
 * Query the host as to whether it likes a specific video mode.
 *
 * @returns the result of the query
 * @param   cx     the width of the mode being queried
 * @param   cy     the height of the mode being queried
 * @param   cBits  the bpp of the mode being queried
 */
VBGLR3DECL(bool) VbglR3HostLikesVideoMode(uint32_t cx, uint32_t cy, uint32_t cBits)
{
    bool fRc = false;
    int rc;
    VMMDevVideoModeSupportedRequest req;

    vmmdevInitRequest(&req.header, VMMDevReq_VideoModeSupported);
    req.width      = cx;
    req.height     = cy;
    req.bpp        = cBits;
    req.fSupported = false;
    rc = vbglR3GRPerform(&req.header);
    if (RT_SUCCESS(rc) && RT_SUCCESS(req.header.rc))
        fRc = req.fSupported;
    else
        LogRelFunc(("error querying video mode supported status from VMMDev."
                    "rc = %Vrc, VMMDev rc = %Vrc\n", rc, req.header.rc));
    return fRc;
}


/**
 * Report the maximum resolution that we currently support to the host.
 *
 * @returns iprt status value.
 * @param   cx      The maximum horizontal resolution.
 * @param   cy      The maximum vertical resolution.
 */
VBGLR3DECL(int) VbglR3ReportMaxGuestResolution(uint32_t cx, uint32_t cy)
{
    int rc = VERR_UNRESOLVED_ERROR;
    VMMDevReqGuestResolution req;

    vmmdevInitRequest(&req.header, VMMDevReq_SetMaxGuestResolution);
    req.u32MaxWidth     = cx;
    req.u32MaxHeight    = cy;
    rc = vbglR3GRPerform(&req.header);
    if (!RT_SUCCESS(rc) || !RT_SUCCESS(req.header.rc))
        LogRelFunc(("error reporting maximum supported resolution to VMMDev. "
                    "rc = %Vrc, VMMDev rc = %Vrc\n", rc, req.header.rc));
    if (RT_SUCCESS(rc))
        rc = req.header.rc;
    return rc;
}
