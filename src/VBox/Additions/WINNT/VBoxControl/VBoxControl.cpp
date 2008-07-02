/** @file
 *
 * VBoxControl - Guest Additions Utility
 *
 * Copyright (C) 2006-2007 Sun Microsystems, Inc.
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

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>

#include <VBox/VBoxGuest.h>
#include <VBox/version.h>
#include <VBox/HostServices/VBoxInfoSvc.h>

void printHelp()
{
    printf("VBoxControl   getversion\n"
           "\n"
           "VBoxControl   getvideoacceleration\n"
           "\n"
           "VBoxControl   setvideoacceleration <on|off>\n"
           "\n"
           "VBoxControl   listcustommodes\n"
           "\n"
           "VBoxControl   addcustommode <width> <height> <bpp>\n"
           "\n"
           "VBoxControl   removecustommode <width> <height> <bpp>\n"
           "\n"
           "VBoxControl   setvideomode <width> <height> <bpp> <screen>\n"
           "\n"
           "VBoxControl   getguestproperty <key>\n"
           "\n"
           "VBoxControl   setguestproperty <key> [<value>] (no value to delete)\n");
}

void printVersion()
{
    printf("%d.%d.%dr%d\n", VBOX_VERSION_MAJOR, VBOX_VERSION_MINOR, VBOX_VERSION_BUILD, VBOX_SVN_REV);
}

#if defined(DEBUG) || defined(LOG_ENABLED)
#define dprintf(a) do { int err = GetLastError (); printf a; SetLastError (err); } while (0)
#else
#define dprintf(a) do {} while (0)
#endif /* DEBUG */

LONG (WINAPI * gpfnChangeDisplaySettingsEx)(LPCTSTR lpszDeviceName, LPDEVMODE lpDevMode, HWND hwnd, DWORD dwflags, LPVOID lParam);

static unsigned nextAdjacentRectXP (RECTL *paRects, unsigned nRects, unsigned iRect)
{
    unsigned i;
    for (i = 0; i < nRects; i++)
    {
        if (paRects[iRect].right == paRects[i].left)
        {
            return i;
        }
    }
    return ~0;
}

static unsigned nextAdjacentRectXN (RECTL *paRects, unsigned nRects, unsigned iRect)
{
    unsigned i;
    for (i = 0; i < nRects; i++)
    {
        if (paRects[iRect].left == paRects[i].right)
        {
            return i;
        }
    }
    return ~0;
}

static unsigned nextAdjacentRectYP (RECTL *paRects, unsigned nRects, unsigned iRect)
{
    unsigned i;
    for (i = 0; i < nRects; i++)
    {
        if (paRects[iRect].bottom == paRects[i].top)
        {
            return i;
        }
    }
    return ~0;
}

unsigned nextAdjacentRectYN (RECTL *paRects, unsigned nRects, unsigned iRect)
{
    unsigned i;
    for (i = 0; i < nRects; i++)
    {
        if (paRects[iRect].top == paRects[i].bottom)
        {
            return i;
        }
    }
    return ~0;
}

void resizeRect(RECTL *paRects, unsigned nRects, unsigned iPrimary, unsigned iResized, int NewWidth, int NewHeight)
{
    RECTL *paNewRects = (RECTL *)alloca (sizeof (RECTL) * nRects);
    memcpy (paNewRects, paRects, sizeof (RECTL) * nRects);
    paNewRects[iResized].right += NewWidth - (paNewRects[iResized].right - paNewRects[iResized].left);
    paNewRects[iResized].bottom += NewHeight - (paNewRects[iResized].bottom - paNewRects[iResized].top);
    
    /* Verify all pairs of originally adjacent rectangles for all 4 directions. 
     * If the pair has a "good" delta (that is the first rectangle intersects the second)
     * at a direction and the second rectangle is not primary one (which can not be moved),
     * move the second rectangle to make it adjacent to the first one.
     */
    
    /* X positive. */
    unsigned iRect;
    for (iRect = 0; iRect < nRects; iRect++)
    {
        /* Find the next adjacent original rect in x positive direction. */
        unsigned iNextRect = nextAdjacentRectXP (paRects, nRects, iRect);
        dprintf(("next %d -> %d\n", iRect, iNextRect));
        
        if (iNextRect == ~0 || iNextRect == iPrimary)
        {
            continue;
        }
        
        /* Check whether there is an X intesection between these adjacent rects in the new rectangles
         * and fix the intersection if delta is "good".
         */
        int delta = paNewRects[iRect].right - paNewRects[iNextRect].left;
        
        if (delta > 0)
        {
            dprintf(("XP intersection right %d left %d, diff %d\n",
                     paNewRects[iRect].right, paNewRects[iNextRect].left,
                     delta));
            
            paNewRects[iNextRect].left += delta;
            paNewRects[iNextRect].right += delta;
        }
    }
    
    /* X negative. */
    for (iRect = 0; iRect < nRects; iRect++)
    {
        /* Find the next adjacent original rect in x negative direction. */
        unsigned iNextRect = nextAdjacentRectXN (paRects, nRects, iRect);
        dprintf(("next %d -> %d\n", iRect, iNextRect));
        
        if (iNextRect == ~0 || iNextRect == iPrimary)
        {
            continue;
        }
        
        /* Check whether there is an X intesection between these adjacent rects in the new rectangles
         * and fix the intersection if delta is "good".
         */
        int delta = paNewRects[iRect].left - paNewRects[iNextRect].right;
        
        if (delta < 0)
        {
            dprintf(("XN intersection left %d right %d, diff %d\n",
                     paNewRects[iRect].left, paNewRects[iNextRect].right,
                     delta));
            
            paNewRects[iNextRect].left += delta;
            paNewRects[iNextRect].right += delta;
        }
    }
    
    /* Y positive (in the computer sence, top->down). */
    for (iRect = 0; iRect < nRects; iRect++)
    {
        /* Find the next adjacent original rect in y positive direction. */
        unsigned iNextRect = nextAdjacentRectYP (paRects, nRects, iRect);
        dprintf(("next %d -> %d\n", iRect, iNextRect));
        
        if (iNextRect == ~0 || iNextRect == iPrimary)
        {
            continue;
        }
        
        /* Check whether there is an Y intesection between these adjacent rects in the new rectangles
         * and fix the intersection if delta is "good".
         */
        int delta = paNewRects[iRect].bottom - paNewRects[iNextRect].top;
        
        if (delta > 0)
        {
            dprintf(("YP intersection bottom %d top %d, diff %d\n",
                     paNewRects[iRect].bottom, paNewRects[iNextRect].top,
                     delta));
            
            paNewRects[iNextRect].top += delta;
            paNewRects[iNextRect].bottom += delta;
        }
    }
    
    /* Y negative (in the computer sence, down->top). */
    for (iRect = 0; iRect < nRects; iRect++)
    {
        /* Find the next adjacent original rect in x negative direction. */
        unsigned iNextRect = nextAdjacentRectYN (paRects, nRects, iRect);
        dprintf(("next %d -> %d\n", iRect, iNextRect));
        
        if (iNextRect == ~0 || iNextRect == iPrimary)
        {
            continue;
        }
        
        /* Check whether there is an Y intesection between these adjacent rects in the new rectangles
         * and fix the intersection if delta is "good".
         */
        int delta = paNewRects[iRect].top - paNewRects[iNextRect].bottom;
        
        if (delta < 0)
        {
            dprintf(("YN intersection top %d bottom %d, diff %d\n",
                     paNewRects[iRect].top, paNewRects[iNextRect].bottom,
                     delta));
            
            paNewRects[iNextRect].top += delta;
            paNewRects[iNextRect].bottom += delta;
        }
    }
    
    memcpy (paRects, paNewRects, sizeof (RECTL) * nRects);
    return;
}

/* Returns TRUE to try again. */
static BOOL ResizeDisplayDevice(ULONG Id, DWORD Width, DWORD Height, DWORD BitsPerPixel)
{
    BOOL fModeReset = (Width == 0 && Height == 0 && BitsPerPixel == 0);
    
    DISPLAY_DEVICE DisplayDevice;

    ZeroMemory(&DisplayDevice, sizeof(DisplayDevice));
    DisplayDevice.cb = sizeof(DisplayDevice);
    
    /* Find out how many display devices the system has */
    DWORD NumDevices = 0;
    DWORD i = 0;
    while (EnumDisplayDevices (NULL, i, &DisplayDevice, 0))
    { 
        dprintf(("[%d] %s\n", i, DisplayDevice.DeviceName));

        if (DisplayDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
        {
            dprintf(("Found primary device. err %d\n", GetLastError ()));
            NumDevices++;
        }
        else if (!(DisplayDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER))
        {
            
            dprintf(("Found secondary device. err %d\n", GetLastError ()));
            NumDevices++;
        }
        
        ZeroMemory(&DisplayDevice, sizeof(DisplayDevice));
        DisplayDevice.cb = sizeof(DisplayDevice);
        i++;
    }
    
    dprintf(("Found total %d devices. err %d\n", NumDevices, GetLastError ()));
    
    if (NumDevices == 0 || Id >= NumDevices)
    {
        dprintf(("Requested identifier %d is invalid. err %d\n", Id, GetLastError ()));
        return FALSE;
    }
    
    DISPLAY_DEVICE *paDisplayDevices = (DISPLAY_DEVICE *)alloca (sizeof (DISPLAY_DEVICE) * NumDevices);
    DEVMODE *paDeviceModes = (DEVMODE *)alloca (sizeof (DEVMODE) * NumDevices);
    RECTL *paRects = (RECTL *)alloca (sizeof (RECTL) * NumDevices);
    
    /* Fetch information about current devices and modes. */
    DWORD DevNum = 0;
    DWORD DevPrimaryNum = 0;
    
    ZeroMemory(&DisplayDevice, sizeof(DISPLAY_DEVICE));
    DisplayDevice.cb = sizeof(DISPLAY_DEVICE);
    
    i = 0;
    while (EnumDisplayDevices (NULL, i, &DisplayDevice, 0))
    { 
        dprintf(("[%d(%d)] %s\n", i, DevNum, DisplayDevice.DeviceName));
        
        BOOL bFetchDevice = FALSE;

        if (DisplayDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
        {
            dprintf(("Found primary device. err %d\n", GetLastError ()));
            DevPrimaryNum = DevNum;
            bFetchDevice = TRUE;
        }
        else if (!(DisplayDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER))
        {
            
            dprintf(("Found secondary device. err %d\n", GetLastError ()));
            bFetchDevice = TRUE;
        }
        
        if (bFetchDevice)
        {
            if (DevNum >= NumDevices)
            {
                dprintf(("%d >= %d\n", NumDevices, DevNum));
                return FALSE;
            }
        
            paDisplayDevices[DevNum] = DisplayDevice;
            
            ZeroMemory(&paDeviceModes[DevNum], sizeof(DEVMODE));
            paDeviceModes[DevNum].dmSize = sizeof(DEVMODE);
            if (!EnumDisplaySettings((LPSTR)DisplayDevice.DeviceName,
                 ENUM_REGISTRY_SETTINGS, &paDeviceModes[DevNum]))
            {
                dprintf(("EnumDisplaySettings err %d\n", GetLastError ()));
                return FALSE;
            }
            
            dprintf(("%dx%d at %d,%d\n",
                    paDeviceModes[DevNum].dmPelsWidth,
                    paDeviceModes[DevNum].dmPelsHeight,
                    paDeviceModes[DevNum].dmPosition.x,
                    paDeviceModes[DevNum].dmPosition.y));
                    
            paRects[DevNum].left   = paDeviceModes[DevNum].dmPosition.x;
            paRects[DevNum].top    = paDeviceModes[DevNum].dmPosition.y;
            paRects[DevNum].right  = paDeviceModes[DevNum].dmPosition.x + paDeviceModes[DevNum].dmPelsWidth;
            paRects[DevNum].bottom = paDeviceModes[DevNum].dmPosition.y + paDeviceModes[DevNum].dmPelsHeight;
            DevNum++;
        }
        
        ZeroMemory(&DisplayDevice, sizeof(DISPLAY_DEVICE));
        DisplayDevice.cb = sizeof(DISPLAY_DEVICE);
        i++;
    }
    
    if (Width == 0)
    {
        Width = paRects[Id].right - paRects[Id].left;
    }

    if (Height == 0)
    {
        Height = paRects[Id].bottom - paRects[Id].top;
    }

    /* Check whether a mode reset or a change is requested. */
    if (   !fModeReset
        && paRects[Id].right - paRects[Id].left == Width
        && paRects[Id].bottom - paRects[Id].top == Height
        && paDeviceModes[Id].dmBitsPerPel == BitsPerPixel)
    {
        dprintf(("VBoxDisplayThread : already at desired resolution.\n"));
        return FALSE;
    }

    resizeRect(paRects, NumDevices, DevPrimaryNum, Id, Width, Height);
#ifdef dprintf
    for (i = 0; i < NumDevices; i++)
    {
        dprintf(("[%d]: %d,%d %dx%d\n",
                i, paRects[i].left, paRects[i].top,
                paRects[i].right - paRects[i].left,
                paRects[i].bottom - paRects[i].top));
    }
#endif /* dprintf */
    
    /* Without this, Windows will not ask the miniport for its
     * mode table but uses an internal cache instead.
     */
    DEVMODE tempDevMode;
    ZeroMemory (&tempDevMode, sizeof (tempDevMode));
    tempDevMode.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(NULL, 0xffffff, &tempDevMode);

    /* Assign the new rectangles to displays. */
    for (i = 0; i < NumDevices; i++)
    {
        paDeviceModes[i].dmPosition.x = paRects[i].left;
        paDeviceModes[i].dmPosition.y = paRects[i].top;
        paDeviceModes[i].dmPelsWidth  = paRects[i].right - paRects[i].left;
        paDeviceModes[i].dmPelsHeight = paRects[i].bottom - paRects[i].top;
        
        paDeviceModes[i].dmFields = DM_POSITION | DM_PELSHEIGHT | DM_PELSWIDTH;
        
        if (   i == Id
            && BitsPerPixel != 0)
        {
            paDeviceModes[i].dmFields |= DM_BITSPERPEL;
            paDeviceModes[i].dmBitsPerPel = BitsPerPixel;
        }
        dprintf(("calling pfnChangeDisplaySettingsEx %x\n", gpfnChangeDisplaySettingsEx));     
        gpfnChangeDisplaySettingsEx((LPSTR)paDisplayDevices[i].DeviceName, 
                 &paDeviceModes[i], NULL, CDS_NORESET | CDS_UPDATEREGISTRY, NULL); 
        dprintf(("ChangeDisplaySettings position err %d\n", GetLastError ()));
    }
    
    /* A second call to ChangeDisplaySettings updates the monitor. */
    LONG status = ChangeDisplaySettings(NULL, 0); 
    dprintf(("ChangeDisplaySettings update status %d\n", status));
    if (status == DISP_CHANGE_SUCCESSFUL || status == DISP_CHANGE_BADMODE)
    {
        /* Successfully set new video mode or our driver can not set the requested mode. Stop trying. */
        return FALSE;
    }

    /* Retry the request. */
    return TRUE;
}

void handleSetVideoMode(int argc, char *argv[])
{
    if (argc != 3 && argc != 4)
    {
        printf("Error: not enough parameters!\n");
        return;
    }

    DWORD xres = atoi(argv[0]);
    DWORD yres = atoi(argv[1]);
    DWORD bpp  = atoi(argv[2]);
    DWORD scr  = 0;

    if (argc == 4)
    {
        scr = atoi(argv[3]);
    }

    HMODULE hUser = GetModuleHandle("USER32");

    if (hUser)
    {
        *(uintptr_t *)&gpfnChangeDisplaySettingsEx = (uintptr_t)GetProcAddress(hUser, "ChangeDisplaySettingsExA");
        dprintf(("VBoxService: pChangeDisplaySettingsEx = %p\n", gpfnChangeDisplaySettingsEx));
        
        if (gpfnChangeDisplaySettingsEx)
        {
            /* The screen index is 0 based in the ResizeDisplayDevice call. */
            scr = scr > 0? scr - 1: 0;
            
            /* Horizontal resolution must be a multiple of 8, round down. */
            xres &= ~0x7;

            ResizeDisplayDevice(scr, xres, yres, bpp);
        }
    }
}

HKEY getVideoKey(bool writable)
{
    HKEY hkeyDeviceMap = 0;
    HKEY hkeyVideo = 0;
    LONG status;

    status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\VIDEO", 0, KEY_READ, &hkeyDeviceMap);
    if ((status != ERROR_SUCCESS) || !hkeyDeviceMap)
    {
        printf("Error opening video device map registry key!\n");
        return 0;
    }
    char szVideoLocation[256];
    DWORD dwKeyType;
    szVideoLocation[0] = 0;
    DWORD len = sizeof(szVideoLocation);
    status = RegQueryValueExA(hkeyDeviceMap, "\\Device\\Video0", NULL, &dwKeyType, (LPBYTE)szVideoLocation, &len);
    /*
     * This value will start with a weird value: \REGISTRY\Machine
     * Make sure this is true.
     */
    if (   (status == ERROR_SUCCESS)
        && (dwKeyType == REG_SZ)
        && (_strnicmp(szVideoLocation, "\\REGISTRY\\Machine", 17) == 0))
    {
        /* open that branch */
        status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, &szVideoLocation[18], 0, KEY_READ | (writable ? KEY_WRITE : 0), &hkeyVideo);
    }
    else
    {
        printf("Error opening registry key '%s'\n", &szVideoLocation[18]);
    }
    RegCloseKey(hkeyDeviceMap);
    return hkeyVideo;
}

void handleGetVideoAcceleration(int argc, char *argv[])
{
    ULONG status;
    HKEY hkeyVideo = getVideoKey(false);

    if (hkeyVideo)
    {
        /* query the actual value */
        DWORD fAcceleration = 1;
        DWORD len = sizeof(fAcceleration);
        DWORD dwKeyType;
        status = RegQueryValueExA(hkeyVideo, "EnableVideoAccel", NULL, &dwKeyType, (LPBYTE)&fAcceleration, &len);
        if (status != ERROR_SUCCESS)
            printf("Video acceleration: default\n");
        else
            printf("Video acceleration: %s\n", fAcceleration ? "on" : "off");
        RegCloseKey(hkeyVideo);
    }
}

void handleSetVideoAcceleration(int argc, char *argv[])
{
    ULONG status;
    HKEY hkeyVideo;

    /* must have exactly one argument: the new offset */
    if (   (argc != 1)
        || (   strcmp(argv[0], "on")
            && strcmp(argv[0], "off")))
    {
        printf("Error: invalid video acceleration status!\n");
        return;
    }

    hkeyVideo = getVideoKey(true);

    if (hkeyVideo)
    {
        int fAccel = 0;
        if (!strcmp(argv[0], "on"))
            fAccel = 1;
        /* set a new value */
        status = RegSetValueExA(hkeyVideo, "EnableVideoAccel", 0, REG_DWORD, (LPBYTE)&fAccel, sizeof(fAccel));
        if (status != ERROR_SUCCESS)
        {
            printf("Error %d writing video acceleration status!\n", status);
        }
        RegCloseKey(hkeyVideo);
    }
}

#define MAX_CUSTOM_MODES 128

/* the table of custom modes */
struct
{
    DWORD xres;
    DWORD yres;
    DWORD bpp;
} customModes[MAX_CUSTOM_MODES] = {0};

void getCustomModes(HKEY hkeyVideo)
{
    ULONG status;
    int curMode = 0;

    /* null out the table */
    memset(customModes, 0, sizeof(customModes));

    do
    {
        char valueName[20];
        DWORD xres, yres, bpp = 0;
        DWORD dwType;
        DWORD dwLen = sizeof(DWORD);

        sprintf(valueName, "CustomMode%dWidth", curMode);
        status = RegQueryValueExA(hkeyVideo, valueName, NULL, &dwType, (LPBYTE)&xres, &dwLen);
        if (status != ERROR_SUCCESS)
            break;
        sprintf(valueName, "CustomMode%dHeight", curMode);
        status = RegQueryValueExA(hkeyVideo, valueName, NULL, &dwType, (LPBYTE)&yres, &dwLen);
        if (status != ERROR_SUCCESS)
            break;
        sprintf(valueName, "CustomMode%dBPP", curMode);
        status = RegQueryValueExA(hkeyVideo, valueName, NULL, &dwType, (LPBYTE)&bpp, &dwLen);
        if (status != ERROR_SUCCESS)
            break;

        /* check if the mode is OK */
        if (   (xres > (1 << 16))
            && (yres > (1 << 16))
            && (   (bpp != 16)
                || (bpp != 24)
                || (bpp != 32)))
            break;

        /* add mode to table */
        customModes[curMode].xres = xres;
        customModes[curMode].yres = yres;
        customModes[curMode].bpp  = bpp;

        ++curMode;

        if (curMode >= MAX_CUSTOM_MODES)
            break;
    } while(1);
}

void writeCustomModes(HKEY hkeyVideo)
{
    ULONG status;
    int tableIndex = 0;
    int modeIndex = 0;

    /* first remove all values */
    for (int i = 0; i < MAX_CUSTOM_MODES; i++)
    {
        char valueName[20];
        sprintf(valueName, "CustomMode%dWidth", i);
        RegDeleteValueA(hkeyVideo, valueName);
        sprintf(valueName, "CustomMode%dHeight", i);
        RegDeleteValueA(hkeyVideo, valueName);
        sprintf(valueName, "CustomMode%dBPP", i);
        RegDeleteValueA(hkeyVideo, valueName);
    }

    do
    {
        if (tableIndex >= MAX_CUSTOM_MODES)
            break;

        /* is the table entry present? */
        if (   (!customModes[tableIndex].xres)
            || (!customModes[tableIndex].yres)
            || (!customModes[tableIndex].bpp))
        {
            tableIndex++;
            continue;
        }

        printf("writing mode %d (%dx%dx%d)\n", modeIndex, customModes[tableIndex].xres, customModes[tableIndex].yres, customModes[tableIndex].bpp);
        char valueName[20];
        sprintf(valueName, "CustomMode%dWidth", modeIndex);
        status = RegSetValueExA(hkeyVideo, valueName, 0, REG_DWORD, (LPBYTE)&customModes[tableIndex].xres,
                                sizeof(customModes[tableIndex].xres));
        sprintf(valueName, "CustomMode%dHeight", modeIndex);
        RegSetValueExA(hkeyVideo, valueName, 0, REG_DWORD, (LPBYTE)&customModes[tableIndex].yres,
                       sizeof(customModes[tableIndex].yres));
        sprintf(valueName, "CustomMode%dBPP", modeIndex);
        RegSetValueExA(hkeyVideo, valueName, 0, REG_DWORD, (LPBYTE)&customModes[tableIndex].bpp,
                       sizeof(customModes[tableIndex].bpp));

        modeIndex++;
        tableIndex++;

    } while(1);

}

void handleListCustomModes(int argc, char *argv[])
{
    if (argc != 0)
    {
        printf("Error: too many parameters!");
        return;
    }

    HKEY hkeyVideo = getVideoKey(false);

    if (hkeyVideo)
    {
        getCustomModes(hkeyVideo);
        for (int i = 0; i < (sizeof(customModes) / sizeof(customModes[0])); i++)
        {
            if (   !customModes[i].xres
                || !customModes[i].yres
                || !customModes[i].bpp)
                continue;

            printf("Mode: %d x %d x %d\n",
                   customModes[i].xres, customModes[i].yres, customModes[i].bpp);
        }
        RegCloseKey(hkeyVideo);
    }
}

void handleAddCustomMode(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Error: not enough parameters!\n");
        return;
    }

    DWORD xres = atoi(argv[0]);
    DWORD yres = atoi(argv[1]);
    DWORD bpp  = atoi(argv[2]);

    /** @todo better check including xres mod 8 = 0! */
    if (   (xres > (1 << 16))
        && (yres > (1 << 16))
        && (   (bpp != 16)
            || (bpp != 24)
            || (bpp != 32)))
    {
        printf("Error: invalid mode specified!\n");
        return;
    }

    HKEY hkeyVideo = getVideoKey(true);

    if (hkeyVideo)
    {
        int i;
        int fModeExists = 0;
        getCustomModes(hkeyVideo);
        for (i = 0; i < MAX_CUSTOM_MODES; i++)
        {
            /* mode exists? */
            if (   customModes[i].xres == xres
                && customModes[i].yres == yres
                && customModes[i].bpp  == bpp
               )
            {
                fModeExists = 1;
            }
        }
        if (!fModeExists)
        {
            for (i = 0; i < MAX_CUSTOM_MODES; i++)
            {
                /* item free? */
                if (!customModes[i].xres)
                {
                    customModes[i].xres = xres;
                    customModes[i].yres = yres;
                    customModes[i].bpp  = bpp;
                    break;
                }
            }
            writeCustomModes(hkeyVideo);
        }
        RegCloseKey(hkeyVideo);
    }
}

void handleRemoveCustomMode(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Error: not enough parameters!\n");
        return;
    }

    DWORD xres = atoi(argv[0]);
    DWORD yres = atoi(argv[1]);
    DWORD bpp  = atoi(argv[2]);

    HKEY hkeyVideo = getVideoKey(true);

    if (hkeyVideo)
    {
        getCustomModes(hkeyVideo);
        for (int i = 0; i < MAX_CUSTOM_MODES; i++)
        {
            /* correct item? */
            if (   (customModes[i].xres == xres)
                && (customModes[i].yres == yres)
                && (customModes[i].bpp  == bpp))
            {
printf("found mode at index %d\n", i);
                memset(&customModes[i], 0, sizeof(customModes[i]));
                break;
            }
        }
        writeCustomModes(hkeyVideo);
        RegCloseKey(hkeyVideo);
    }
}


#ifdef VBOX_WITH_INFO_SVC
/**
 * Open the VirtualBox guest device.
 * @returns IPRT status value
 * @param   hDevice  where to store the handle to the open device
 */
static int openGuestDevice(HANDLE *hDevice)
{
    if (!VALID_PTR(hDevice))
        return VERR_INVALID_POINTER;
    *hDevice = CreateFile(VBOXGUEST_DEVICE_NAME,
                          GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL);
    return (*hDevice != INVALID_HANDLE_VALUE) ? VINF_SUCCESS : VERR_OPEN_FAILED;
}


/**
 * Connect to an HGCM service.
 * @returns IPRT status code
 * @param   hDevice      handle to the VBox device
 * @param   pszService   the name of the service to connect to
 * @param   pu32ClientID where to store the connection handle
 */
static int hgcmConnect(HANDLE hDevice, char *pszService, uint32_t *pu32ClientID)
{
    if (!VALID_PTR(pszService) || !VALID_PTR(pu32ClientID))
        return VERR_INVALID_POINTER;
    VBoxGuestHGCMConnectInfo info;
    int rc = VINF_SUCCESS;

    memset (&info, 0, sizeof (info));
    if (strlen(pszService) + 1 > sizeof(info.Loc.u.host.achName))
        return false;
    strcpy (info.Loc.u.host.achName, pszService);
    info.Loc.type = VMMDevHGCMLoc_LocalHost_Existing;
    DWORD cbReturned;
    if (DeviceIoControl (hDevice,
                         IOCTL_VBOXGUEST_HGCM_CONNECT,
                         &info, sizeof (info),
                         &info, sizeof (info),
                         &cbReturned,
                         NULL))
        rc = info.result;
    else
        rc = VERR_FILE_IO_ERROR;
    if (RT_SUCCESS(rc))
        *pu32ClientID = info.u32ClientID;
    return rc;
}


/** Set a 32bit unsigned integer parameter to an HGCM request */
static void VbglHGCMParmUInt32Set(HGCMFunctionParameter *pParm, uint32_t u32)
{
    pParm->type = VMMDevHGCMParmType_32bit;
    pParm->u.value64 = 0; /* init unused bits to 0 */
    pParm->u.value32 = u32;
}


/** Get a 32bit unsigned integer returned from an HGCM request */
static int VbglHGCMParmUInt32Get(HGCMFunctionParameter *pParm, uint32_t *pu32)
{
    if (pParm->type == VMMDevHGCMParmType_32bit)
    {
        *pu32 = pParm->u.value32;
        return VINF_SUCCESS;
    }
    return VERR_INVALID_PARAMETER;
}


/** Set a pointer parameter to an HGCM request */
static void VbglHGCMParmPtrSet(HGCMFunctionParameter *pParm, void *pv, uint32_t cb)
{
    pParm->type                    = VMMDevHGCMParmType_LinAddr;
    pParm->u.Pointer.size          = cb;
    pParm->u.Pointer.u.linearAddr  = (uintptr_t)pv;
}


/** Make an HGCM call */
static int hgcmCall(HANDLE hDevice, VBoxGuestHGCMCallInfo *pMsg, size_t cbMsg)
{
    DWORD cbReturned;
    int rc = VERR_NOT_SUPPORTED;

    if (DeviceIoControl (hDevice,
                         IOCTL_VBOXGUEST_HGCM_CALL,
                         pMsg, cbMsg,
                         pMsg, cbMsg,
                         &cbReturned,
                         NULL))
        rc = VINF_SUCCESS;
    return rc;
}


/**
 * Retrieve a property from the host/guest configuration registry
 * @returns  IPRT status code
 * @param hDevice handle to the VBox device
 * @param   u32ClientID     The client id returned by VbglR3ClipboardConnect().
 * @param   pszKey          The registry key to save to.
 * @param   pszValue        Where to store the value retrieved.
 * @param   cbValue         The size of the buffer pszValue points to.
 * @param   pcbActual       Where to store the required buffer size on
 *                          overflow or the value size on success.  A value
 *                          of zero means that the property does not exist.
 *                          Optional.
 */
static int hgcmInfoSvcGetProp(HANDLE hDevice, uint32_t u32ClientID,
                              char *pszKey, char *pszValue,
                              uint32_t cbValue, uint32_t *pcbActual)
{
    using namespace svcInfo;

    if (!VALID_PTR(pszValue))
        return VERR_INVALID_POINTER;
    GetConfigKey Msg;

    Msg.hdr.result = (uint32_t)VERR_WRONG_ORDER;  /** @todo drop the cast when the result type has been fixed! */
    Msg.hdr.u32ClientID = u32ClientID;
    Msg.hdr.u32Function = GET_CONFIG_KEY;
    Msg.hdr.cParms = 3;
    VbglHGCMParmPtrSet(&Msg.key, pszKey, strlen(pszKey) + 1);
    VbglHGCMParmPtrSet(&Msg.value, pszValue, cbValue);
    VbglHGCMParmUInt32Set(&Msg.size, 0);
    int rc = hgcmCall(hDevice, &Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
        rc = Msg.hdr.result;
    uint32_t cbActual;
    if (RT_SUCCESS(rc) || (VERR_BUFFER_OVERFLOW == rc))
    {
        int rc2 = VbglHGCMParmUInt32Get(&Msg.size, &cbActual);
        if (RT_SUCCESS(rc2))
        {
            if (pcbActual != NULL)
                *pcbActual = cbActual;
        }
        else
            rc = rc2;
    }
    return rc;
}


/**
 * Store a property from the host/guest configuration registry
 * @returns  IPRT status code
 * @param hDevice handle to the VBox device
 * @param   u32ClientID     The client id returned by VbglR3ClipboardConnect().
 * @param   pszKey          The registry key to save to.
 * @param   pszValue        The value to store.  If this is NULL then the key
 *                          will be removed.
 */
static int hgcmInfoSvcSetProp(HANDLE hDevice, uint32_t u32ClientID,
                              char *pszKey, char *pszValue)
{
    using namespace svcInfo;

    if (!VALID_PTR(pszKey))
        return VERR_INVALID_POINTER;
    if (!VALID_PTR(pszValue) && (pszValue != NULL));
    int rc;

    if (pszValue != NULL)
    {
        SetConfigKey Msg;

        Msg.hdr.result = (uint32_t)VERR_WRONG_ORDER;  /** @todo drop the cast when the result type has been fixed! */
        Msg.hdr.u32ClientID = u32ClientID;
        Msg.hdr.u32Function = SET_CONFIG_KEY;
        Msg.hdr.cParms = 2;
        VbglHGCMParmPtrSet(&Msg.key, pszKey, strlen(pszKey) + 1);
        VbglHGCMParmPtrSet(&Msg.value, pszValue, strlen(pszValue) + 1);
        rc = hgcmCall(hDevice, &Msg.hdr, sizeof(Msg));
        if (RT_SUCCESS(rc))
            rc = Msg.hdr.result;
    }
    else
    {
        DelConfigKey Msg;

        Msg.hdr.result = (uint32_t)VERR_WRONG_ORDER;  /** @todo drop the cast when the result type has been fixed! */
        Msg.hdr.u32ClientID = u32ClientID;
        Msg.hdr.u32Function = DEL_CONFIG_KEY;
        Msg.hdr.cParms = 1;
        VbglHGCMParmPtrSet(&Msg.key, pszKey, strlen(pszKey) + 1);
        rc = hgcmCall(hDevice, &Msg.hdr, sizeof(Msg));
        if (RT_SUCCESS(rc))
            rc = Msg.hdr.result;
    }
    return rc;
}


/** Disconnects from an HGCM service. */
static void hgcmDisconnect(HANDLE hDevice, uint32_t u32ClientID)
{
    if (u32ClientID == 0)
        return;

    VBoxGuestHGCMDisconnectInfo info;
    memset (&info, 0, sizeof (info));
    info.u32ClientID = u32ClientID;

    DWORD cbReturned;
    DeviceIoControl (hDevice,
                     IOCTL_VBOXGUEST_HGCM_DISCONNECT,
                     &info, sizeof (info),
                     &info, sizeof (info),
                     &cbReturned,
                     NULL);
}


/**
 * Retrieves a value from the host/guest configuration registry.
 * This is accessed through the "VBoxSharedInfoSvc" HGCM service.
 *
 * @returns IPRT status value
 * @param   key  (string) the key which the value is stored under.
 */
static int handleGetGuestProperty(int argc, char *argv[])
{
    if (argc != 1)
    {
        printHelp();
        return 1;
    }
    char szValue[svcInfo::KEY_MAX_VALUE_LEN];
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    uint32_t u32ClientID = 0;
    int rc = openGuestDevice(&hDevice);
    if (!RT_SUCCESS(rc))
        printf("Failed to open the VirtualBox device, RT error %d\n", rc);
    if (RT_SUCCESS(rc))
    {
        rc = hgcmConnect(hDevice, "VBoxSharedInfoSvc", &u32ClientID);
        if (!RT_SUCCESS(rc))
            printf("Failed to connect to the host/guest registry service, RT error %d\n", rc);
    }
    if (RT_SUCCESS(rc))
    {
        rc = hgcmInfoSvcGetProp(hDevice, u32ClientID, argv[0], szValue,
                                sizeof(szValue), NULL);
        if (!RT_SUCCESS(rc) && (rc != VERR_NOT_FOUND))
            printf("Failed to retrieve the property value, RT error %d\n", rc);
    }
    if (RT_SUCCESS(rc) || (VERR_NOT_FOUND == rc))
    {
        if (RT_SUCCESS(rc))
            printf("Value: %s\n", szValue);
        else
            printf("No value set!\n");
    }
    if (u32ClientID != 0)
        hgcmDisconnect(hDevice, u32ClientID);
    if (hDevice != INVALID_HANDLE_VALUE)
        CloseHandle(hDevice);
    return rc;
}


/**
 * Writes a value to the host/guest configuration registry.
 * This is accessed through the "VBoxSharedInfoSvc" HGCM service.
 *
 * @returns IPRT status value
 * @param   key   (string) the key which the value is stored under.
 * @param   value (string) the value to write.  If empty, the key will be
 *                removed.
 */
static int handleSetGuestProperty(int argc, char *argv[])
{
    if (argc != 1 && argc != 2)
    {
        printHelp();
        return 1;
    }
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    char *pszValue = NULL;
    if (2 == argc)
        pszValue = argv[1];
    uint32_t u32ClientID = 0;
    int rc = openGuestDevice(&hDevice);
    if (!RT_SUCCESS(rc))
        printf("Failed to open the VirtualBox device, RT error %d\n", rc);
    if (RT_SUCCESS(rc))
    {
        rc = hgcmConnect(hDevice, "VBoxSharedInfoSvc", &u32ClientID);
        if (!RT_SUCCESS(rc))
            printf("Failed to connect to the host/guest registry service, RT error %d\n", rc);
    }
    if (RT_SUCCESS(rc))
    {
        rc = hgcmInfoSvcSetProp(hDevice, u32ClientID, argv[0], pszValue);
        if (!RT_SUCCESS(rc))
            printf("Failed to store the property value, RT error %d\n", rc);
    }
    if (u32ClientID != 0)
        hgcmDisconnect(hDevice, u32ClientID);
    if (hDevice != INVALID_HANDLE_VALUE)
        CloseHandle(hDevice);
    return rc;
}
#endif  /* VBOX_WITH_INFO_SVC */


/**
 * Main function
 */
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printHelp();
        return 1;
    }

    /* todo: add better / stable command line handling here! */

    /* determine which command */
    if ((stricmp(argv[1], "getversion") == 0) ||
        (stricmp(argv[1], "-v") == 0) ||
        (stricmp(argv[1], "--version") == 0) ||
        (stricmp(argv[1], "-version") == 0))
    {
        printVersion();
    }
    else if (stricmp(argv[1], "getvideoacceleration") == 0)
    {
        handleGetVideoAcceleration(argc - 2, &argv[2]);
    }
    else if (stricmp(argv[1], "setvideoacceleration") == 0)
    {
        handleSetVideoAcceleration(argc - 2, &argv[2]);
    }
    else if (stricmp(argv[1], "listcustommodes") == 0)
    {
        handleListCustomModes(argc - 2, &argv[2]);
    }
    else if (stricmp(argv[1], "addcustommode") == 0)
    {
        handleAddCustomMode(argc - 2, &argv[2]);
    }
    else if (stricmp(argv[1], "removecustommode") == 0)
    {
        handleRemoveCustomMode(argc - 2, &argv[2]);
    }
    else if (stricmp(argv[1], "setvideomode") == 0)
    {
        handleSetVideoMode(argc - 2, &argv[2]);
    }
#ifdef VBOX_WITH_INFO_SVC
    else if (stricmp(argv[1], "getguestproperty") == 0)
    {
        int rc = handleGetGuestProperty(argc - 2, &argv[2]);
        return RT_SUCCESS(rc) ? 0 : 1;
    }
    else if (stricmp(argv[1], "setguestproperty") == 0)
    {
        handleSetGuestProperty(argc - 2, &argv[2]);
    }
#endif  /* VBOX_WITH_INFO_SVC */
    else
    {
        printHelp();
        return 1;
    }

    return 0;
}
