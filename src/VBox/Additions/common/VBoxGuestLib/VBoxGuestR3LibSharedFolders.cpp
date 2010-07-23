/* $Id$ */
/** @file
 * VBoxGuestR3Lib - Ring-3 Support Library for VirtualBox guest additions, shared folders.
 */

/*
 * Copyright (C) 2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <iprt/string.h>
#include <iprt/mem.h>
#include <iprt/assert.h>
#include <iprt/cpp/autores.h>
#include <iprt/stdarg.h>
#include <VBox/log.h>
#include <VBox/shflsvc.h> /** @todo File should be moved to VBox/HostServices/SharedFolderSvc.h */

#include "VBGLR3Internal.h"


/**
 * Connects to the shared folder service.
 *
 * @returns VBox status code
 * @param   pu32ClientId    Where to put the client id on success. The client id
 *                          must be passed to all the other calls to the service.
 */
VBGLR3DECL(int) VbglR3SharedFolderConnect(uint32_t *pu32ClientId)
{
    VBoxGuestHGCMConnectInfo Info;
    Info.result = VERR_WRONG_ORDER;
    Info.Loc.type = VMMDevHGCMLoc_LocalHost_Existing;
    RT_ZERO(Info.Loc.u);
    strcpy(Info.Loc.u.host.achName, "VBoxSharedFolders");
    Info.u32ClientID = UINT32_MAX;  /* try make valgrid shut up. */

    int rc = vbglR3DoIOCtl(VBOXGUEST_IOCTL_HGCM_CONNECT, &Info, sizeof(Info));
    if (RT_SUCCESS(rc))
    {
        rc = Info.result;
        if (RT_SUCCESS(rc))
            *pu32ClientId = Info.u32ClientID;
    }
    return rc;
}


/**
 * Disconnect from the shared folder service.
 *
 * @returns VBox status code.
 * @param   u32ClientId     The client id returned by VbglR3InfoSvcConnect().
 */
VBGLR3DECL(int) VbglR3SharedFolderDisconnect(uint32_t u32ClientId)
{
    VBoxGuestHGCMDisconnectInfo Info;
    Info.result = VERR_WRONG_ORDER;
    Info.u32ClientID = u32ClientId;

    int rc = vbglR3DoIOCtl(VBOXGUEST_IOCTL_HGCM_DISCONNECT, &Info, sizeof(Info));
    if (RT_SUCCESS(rc))
        rc = Info.result;
    return rc;
}


/**
 * Get the list of available shared folders.
 *
 * @returns VBox status code.
 * @param   u32ClientId     The client id returned by VbglR3InvsSvcConnect().
 * @param   fAutoMountOnly  Flag whether only auto-mounted shared folders
 *                          should be reported.
 * @param   paMappings      Pointer to a preallocated array which will retrieve the mapping info.
 * @param   cbMappings      Size (in bytes) of the provided array.
 * @param   pcMappings      On input, the size of @a paMappings gives as an
 *                          item count.  On output, the number of mappings
 *                          returned in @a paMappings.
 *
 * @todo    r=bird: cbMappings and @a *pcMappings overlap.  The better API
 *          would be to change cbMappings to cMappings (the entries are fixed
 *          sized) and move the move the input aspect of @a *pcMappings to it.
 *
 *          However, it would be better if this function would do the array
 *          allocation.  This way you could deal with too-much-data conditions
 *          here (or hide the max-number-of-shared-folders-per-vm-define).
 *          Then paMappings would become ppaMappings and cbMappings could be
 *          removed altogether. *pcMappings would only be output.  A
 *          corresponding VbglR3SharedFolderFreeMappings would be required for
 *          a 100% clean API (this is an (/going to be) offical API for C/C++
 *          programs).
 */
VBGLR3DECL(int) VbglR3SharedFolderGetMappings(uint32_t u32ClientId, bool fAutoMountOnly,
                                              PVBGLR3SHAREDFOLDERMAPPING paMappings, uint32_t cbMappings,
                                              uint32_t *pcMappings)
{
    AssertPtr(pcMappings);

    VBoxSFQueryMappings Msg;

    Msg.callInfo.result = VERR_WRONG_ORDER;
    Msg.callInfo.u32ClientID = u32ClientId;
    Msg.callInfo.u32Function = SHFL_FN_QUERY_MAPPINGS;
    Msg.callInfo.cParms = 3;

    /* Set the mapping flags. */
    uint32_t u32Flags = 0; /** @todo SHFL_MF_UTF8 is not implemented yet. */
    if (fAutoMountOnly) /* We only want the mappings which get auto-mounted. */
        u32Flags |= SHFL_MF_AUTOMOUNT;
    VbglHGCMParmUInt32Set(&Msg.flags, u32Flags);

    /* Init the rest of the message. */
    VbglHGCMParmUInt32Set(&Msg.numberOfMappings, *pcMappings);
    VbglHGCMParmPtrSet(&Msg.mappings, &paMappings[0], cbMappings);

    int rc = vbglR3DoIOCtl(VBOXGUEST_IOCTL_HGCM_CALL(sizeof(Msg)), &Msg, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        VbglHGCMParmUInt32Get(&Msg.numberOfMappings, pcMappings);
        rc = Msg.callInfo.result;
    }
    return rc;
}


/**
 * Get the real name of a shared folder.
 *
 * @returns VBox status code.
 * @param   u32ClientId     The client id returned by VbglR3InvsSvcConnect().
 * @param   u32Root         Root ID of shared folder to get the name for.
 * @param   ppszName        Where to return the name string.  This shall be
 *                          freed by calling RTStrFree.
 */
VBGLR3DECL(int) VbglR3SharedFolderGetName(uint32_t u32ClientId, uint32_t u32Root, char **ppszName)
{
    AssertPtr(ppszName);

    VBoxSFQueryMapName Msg;

    Msg.callInfo.result = VERR_WRONG_ORDER;
    Msg.callInfo.u32ClientID = u32ClientId;
    Msg.callInfo.u32Function = SHFL_FN_QUERY_MAP_NAME;
    Msg.callInfo.cParms = 2;

    int         rc;
    uint32_t    cbString = sizeof(SHFLSTRING) + SHFL_MAX_LEN;
    PSHFLSTRING pString = (PSHFLSTRING)RTMemAlloc(cbString);
    if (pString)
    {
        RT_ZERO(*pString);
        ShflStringInitBuffer(pString, SHFL_MAX_LEN);

        VbglHGCMParmUInt32Set(&Msg.root, u32Root);
        VbglHGCMParmPtrSet(&Msg.name, pString, cbString);

        rc = vbglR3DoIOCtl(VBOXGUEST_IOCTL_HGCM_CALL(sizeof(Msg)), &Msg, sizeof(Msg));
        if (RT_SUCCESS(rc))
        {
            *ppszName = NULL;
            rc = RTUtf16ToUtf8(&pString->String.ucs2[0], ppszName);
            if (RT_SUCCESS(rc))
                rc = Msg.callInfo.result; /** @todo r=bird: Shouldn't you check this *before* doing the conversion? */
        }
        RTMemFree(pString);
    }
    else
        rc = VERR_INVALID_PARAMETER;
    return rc;
}
