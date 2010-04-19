/*
 * Copyright (C) the Wine project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * Sun LGPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL or LGPL is available it will apply instead, Sun elects to use only
 * the Lesser General Public License version 2.1 (LGPLv2) at this time for any software where
 * a choice of LGPL license versions is made available with the language indicating
 * that LGPLv2 or any later version may be used, or where a choice of which version
 * of the LGPL is applied is otherwise unspecified.
 */

#ifndef __DDK_USBDLIB_H__
#define __DDK_USBDLIB_H__

typedef struct _USBD_INTERFACE_LIST_ENTRY {
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    PUSBD_INTERFACE_INFORMATION Interface;
} USBD_INTERFACE_LIST_ENTRY;
typedef struct _USBD_INTERFACE_LIST_ENTRY *PUSBD_INTERFACE_LIST_ENTRY;

PURB WINAPI USBD_CreateConfigurationRequest(PUSB_CONFIGURATION_DESCRIPTOR,PUSHORT);
PURB WINAPI USBD_CreateConfigurationRequestEx(PUSB_CONFIGURATION_DESCRIPTOR,PUSBD_INTERFACE_LIST_ENTRY);
ULONG WINAPI USBD_GetInterfaceLength(PUSB_INTERFACE_DESCRIPTOR,PUCHAR);
VOID WINAPI USBD_GetUSBDIVersion(PUSBD_VERSION_INFORMATION);
PUSB_COMMON_DESCRIPTOR WINAPI USBD_ParseDescriptors(PVOID,ULONG,PVOID,LONG);
PUSB_INTERFACE_DESCRIPTOR WINAPI USBD_ParseConfigurationDescriptorEx(PUSB_CONFIGURATION_DESCRIPTOR,PVOID,LONG,LONG,LONG,LONG,LONG);

#endif
