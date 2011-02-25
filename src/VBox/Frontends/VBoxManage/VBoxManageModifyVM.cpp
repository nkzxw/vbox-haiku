/* $Id$ */
/** @file
 * VBoxManage - Implementation of modifyvm command.
 */

/*
 * Copyright (C) 2006-2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#ifndef VBOX_ONLY_DOCS
#include <VBox/com/com.h>
#include <VBox/com/array.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>
#include <VBox/com/EventQueue.h>

#include <VBox/com/VirtualBox.h>
#endif /* !VBOX_ONLY_DOCS */

#include <iprt/cidr.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/getopt.h>
#include <VBox/log.h>

#include "VBoxManage.h"

#ifndef VBOX_ONLY_DOCS
using namespace com;


/** @todo refine this after HDD changes; MSC 8.0/64 has trouble with handleModifyVM.  */
#if defined(_MSC_VER)
# pragma optimize("g", off)
#endif

enum
{
    MODIFYVM_NAME = 1000,
    MODIFYVM_OSTYPE,
    MODIFYVM_MEMORY,
    MODIFYVM_PAGEFUSION,
    MODIFYVM_VRAM,
    MODIFYVM_FIRMWARE,
    MODIFYVM_ACPI,
    MODIFYVM_IOAPIC,
    MODIFYVM_PAE,
    MODIFYVM_SYNTHCPU,
    MODIFYVM_HWVIRTEX,
    MODIFYVM_HWVIRTEXEXCLUSIVE,
    MODIFYVM_NESTEDPAGING,
    MODIFYVM_LARGEPAGES,
    MODIFYVM_VTXVPID,
    MODIFYVM_CPUS,
    MODIFYVM_CPUHOTPLUG,
    MODIFYVM_PLUGCPU,
    MODIFYVM_UNPLUGCPU,
    MODIFYVM_SETCPUID,
    MODIFYVM_DELCPUID,
    MODIFYVM_DELALLCPUID,
    MODIFYVM_MONITORCOUNT,
    MODIFYVM_ACCELERATE3D,
#ifdef VBOX_WITH_VIDEOHWACCEL
    MODIFYVM_ACCELERATE2DVIDEO,
#endif
    MODIFYVM_BIOSLOGOFADEIN,
    MODIFYVM_BIOSLOGOFADEOUT,
    MODIFYVM_BIOSLOGODISPLAYTIME,
    MODIFYVM_BIOSLOGOIMAGEPATH,
    MODIFYVM_BIOSBOOTMENU,
    MODIFYVM_BIOSSYSTEMTIMEOFFSET,
    MODIFYVM_BIOSPXEDEBUG,
    MODIFYVM_BOOT,
    MODIFYVM_HDA,                // deprecated
    MODIFYVM_HDB,                // deprecated
    MODIFYVM_HDD,                // deprecated
    MODIFYVM_IDECONTROLLER,      // deprecated
    MODIFYVM_SATAIDEEMULATION,   // deprecated
    MODIFYVM_SATAPORTCOUNT,      // deprecated
    MODIFYVM_SATAPORT,           // deprecated
    MODIFYVM_SATA,               // deprecated
    MODIFYVM_SCSIPORT,           // deprecated
    MODIFYVM_SCSITYPE,           // deprecated
    MODIFYVM_SCSI,               // deprecated
    MODIFYVM_DVDPASSTHROUGH,     // deprecated
    MODIFYVM_DVD,                // deprecated
    MODIFYVM_FLOPPY,             // deprecated
    MODIFYVM_NICTRACEFILE,
    MODIFYVM_NICTRACE,
    MODIFYVM_NICTYPE,
    MODIFYVM_NICSPEED,
    MODIFYVM_NICBOOTPRIO,
    MODIFYVM_NICPROMISC,
    MODIFYVM_NIC,
    MODIFYVM_CABLECONNECTED,
    MODIFYVM_BRIDGEADAPTER,
    MODIFYVM_HOSTONLYADAPTER,
    MODIFYVM_INTNET,
    MODIFYVM_NATNET,
#ifdef VBOX_WITH_VDE
    MODIFYVM_VDENET,
#endif
    MODIFYVM_NATBINDIP,
    MODIFYVM_NATSETTINGS,
    MODIFYVM_NATPF,
    MODIFYVM_NATALIASMODE,
    MODIFYVM_NATTFTPPREFIX,
    MODIFYVM_NATTFTPFILE,
    MODIFYVM_NATTFTPSERVER,
    MODIFYVM_NATDNSPASSDOMAIN,
    MODIFYVM_NATDNSPROXY,
    MODIFYVM_NATDNSHOSTRESOLVER,
    MODIFYVM_MACADDRESS,
    MODIFYVM_HIDPTR,
    MODIFYVM_HIDKBD,
    MODIFYVM_UARTMODE,
    MODIFYVM_UART,
    MODIFYVM_GUESTMEMORYBALLOON,
    MODIFYVM_AUDIOCONTROLLER,
    MODIFYVM_AUDIO,
    MODIFYVM_CLIPBOARD,
    MODIFYVM_VRDPPORT,                /* VRDE: deprecated */
    MODIFYVM_VRDPADDRESS,             /* VRDE: deprecated */
    MODIFYVM_VRDPAUTHTYPE,            /* VRDE: deprecated */
    MODIFYVM_VRDPMULTICON,            /* VRDE: deprecated */
    MODIFYVM_VRDPREUSECON,            /* VRDE: deprecated */
    MODIFYVM_VRDPVIDEOCHANNEL,        /* VRDE: deprecated */
    MODIFYVM_VRDPVIDEOCHANNELQUALITY, /* VRDE: deprecated */
    MODIFYVM_VRDP,                    /* VRDE: deprecated */
    MODIFYVM_VRDEPROPERTY,
    MODIFYVM_VRDEPORT,
    MODIFYVM_VRDEADDRESS,
    MODIFYVM_VRDEAUTHTYPE,
    MODIFYVM_VRDEAUTHLIBRARY,
    MODIFYVM_VRDEMULTICON,
    MODIFYVM_VRDEREUSECON,
    MODIFYVM_VRDEVIDEOCHANNEL,
    MODIFYVM_VRDEVIDEOCHANNELQUALITY,
    MODIFYVM_VRDE_EXTPACK,
    MODIFYVM_VRDE,
    MODIFYVM_RTCUSEUTC,
    MODIFYVM_USBEHCI,
    MODIFYVM_USB,
    MODIFYVM_SNAPSHOTFOLDER,
    MODIFYVM_TELEPORTER_ENABLED,
    MODIFYVM_TELEPORTER_PORT,
    MODIFYVM_TELEPORTER_ADDRESS,
    MODIFYVM_TELEPORTER_PASSWORD,
    MODIFYVM_HARDWARE_UUID,
    MODIFYVM_HPET,
    MODIFYVM_IOCACHE,
    MODIFYVM_IOCACHESIZE,
    MODIFYVM_FAULT_TOLERANCE,
    MODIFYVM_FAULT_TOLERANCE_ADDRESS,
    MODIFYVM_FAULT_TOLERANCE_PORT,
    MODIFYVM_FAULT_TOLERANCE_PASSWORD,
    MODIFYVM_FAULT_TOLERANCE_SYNC_INTERVAL,
    MODIFYVM_CPU_EXECTUION_CAP,
    MODIFYVM_CHIPSET
};

static const RTGETOPTDEF g_aModifyVMOptions[] =
{
    { "--name",                     MODIFYVM_NAME,                      RTGETOPT_REQ_STRING },
    { "--ostype",                   MODIFYVM_OSTYPE,                    RTGETOPT_REQ_STRING },
    { "--memory",                   MODIFYVM_MEMORY,                    RTGETOPT_REQ_UINT32 },
    { "--pagefusion",               MODIFYVM_PAGEFUSION,                RTGETOPT_REQ_BOOL_ONOFF },
    { "--vram",                     MODIFYVM_VRAM,                      RTGETOPT_REQ_UINT32 },
    { "--firmware",                 MODIFYVM_FIRMWARE,                  RTGETOPT_REQ_STRING },
    { "--acpi",                     MODIFYVM_ACPI,                      RTGETOPT_REQ_BOOL_ONOFF },
    { "--ioapic",                   MODIFYVM_IOAPIC,                    RTGETOPT_REQ_BOOL_ONOFF },
    { "--pae",                      MODIFYVM_PAE,                       RTGETOPT_REQ_BOOL_ONOFF },
    { "--synthcpu",                 MODIFYVM_SYNTHCPU,                  RTGETOPT_REQ_BOOL_ONOFF },
    { "--hwvirtex",                 MODIFYVM_HWVIRTEX,                  RTGETOPT_REQ_BOOL_ONOFF },
    { "--hwvirtexexcl",             MODIFYVM_HWVIRTEXEXCLUSIVE,         RTGETOPT_REQ_BOOL_ONOFF },
    { "--nestedpaging",             MODIFYVM_NESTEDPAGING,              RTGETOPT_REQ_BOOL_ONOFF },
    { "--largepages",               MODIFYVM_LARGEPAGES,                RTGETOPT_REQ_BOOL_ONOFF },
    { "--vtxvpid",                  MODIFYVM_VTXVPID,                   RTGETOPT_REQ_BOOL_ONOFF },
    { "--cpuidset",                 MODIFYVM_SETCPUID,                  RTGETOPT_REQ_UINT32 | RTGETOPT_FLAG_HEX},
    { "--cpuidremove",              MODIFYVM_DELCPUID,                  RTGETOPT_REQ_UINT32 | RTGETOPT_FLAG_HEX},
    { "--cpuidremoveall",           MODIFYVM_DELALLCPUID,               RTGETOPT_REQ_NOTHING},
    { "--cpus",                     MODIFYVM_CPUS,                      RTGETOPT_REQ_UINT32 },
    { "--cpuhotplug",               MODIFYVM_CPUHOTPLUG,                RTGETOPT_REQ_BOOL_ONOFF },
    { "--plugcpu",                  MODIFYVM_PLUGCPU,                   RTGETOPT_REQ_UINT32 },
    { "--unplugcpu",                MODIFYVM_UNPLUGCPU,                 RTGETOPT_REQ_UINT32 },
    { "--cpuexecutioncap",          MODIFYVM_CPU_EXECTUION_CAP,         RTGETOPT_REQ_UINT32 },
    { "--rtcuseutc",                MODIFYVM_RTCUSEUTC,                 RTGETOPT_REQ_BOOL_ONOFF },
    { "--monitorcount",             MODIFYVM_MONITORCOUNT,              RTGETOPT_REQ_UINT32 },
    { "--accelerate3d",             MODIFYVM_ACCELERATE3D,              RTGETOPT_REQ_BOOL_ONOFF },
#ifdef VBOX_WITH_VIDEOHWACCEL
    { "--accelerate2dvideo",        MODIFYVM_ACCELERATE2DVIDEO,         RTGETOPT_REQ_BOOL_ONOFF },
#endif
    { "--bioslogofadein",           MODIFYVM_BIOSLOGOFADEIN,            RTGETOPT_REQ_BOOL_ONOFF },
    { "--bioslogofadeout",          MODIFYVM_BIOSLOGOFADEOUT,           RTGETOPT_REQ_BOOL_ONOFF },
    { "--bioslogodisplaytime",      MODIFYVM_BIOSLOGODISPLAYTIME,       RTGETOPT_REQ_UINT32 },
    { "--bioslogoimagepath",        MODIFYVM_BIOSLOGOIMAGEPATH,         RTGETOPT_REQ_STRING },
    { "--biosbootmenu",             MODIFYVM_BIOSBOOTMENU,              RTGETOPT_REQ_STRING },
    { "--biossystemtimeoffset",     MODIFYVM_BIOSSYSTEMTIMEOFFSET,      RTGETOPT_REQ_INT64 },
    { "--biospxedebug",             MODIFYVM_BIOSPXEDEBUG,              RTGETOPT_REQ_BOOL_ONOFF },
    { "--boot",                     MODIFYVM_BOOT,                      RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--hda",                      MODIFYVM_HDA,                       RTGETOPT_REQ_STRING },
    { "--hdb",                      MODIFYVM_HDB,                       RTGETOPT_REQ_STRING },
    { "--hdd",                      MODIFYVM_HDD,                       RTGETOPT_REQ_STRING },
    { "--idecontroller",            MODIFYVM_IDECONTROLLER,             RTGETOPT_REQ_STRING },
    { "--sataideemulation",         MODIFYVM_SATAIDEEMULATION,          RTGETOPT_REQ_UINT32 | RTGETOPT_FLAG_INDEX },
    { "--sataportcount",            MODIFYVM_SATAPORTCOUNT,             RTGETOPT_REQ_UINT32 },
    { "--sataport",                 MODIFYVM_SATAPORT,                  RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--sata",                     MODIFYVM_SATA,                      RTGETOPT_REQ_STRING },
    { "--scsiport",                 MODIFYVM_SCSIPORT,                  RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--scsitype",                 MODIFYVM_SCSITYPE,                  RTGETOPT_REQ_STRING },
    { "--scsi",                     MODIFYVM_SCSI,                      RTGETOPT_REQ_STRING },
    { "--dvdpassthrough",           MODIFYVM_DVDPASSTHROUGH,            RTGETOPT_REQ_STRING },
    { "--dvd",                      MODIFYVM_DVD,                       RTGETOPT_REQ_STRING },
    { "--floppy",                   MODIFYVM_FLOPPY,                    RTGETOPT_REQ_STRING },
    { "--nictracefile",             MODIFYVM_NICTRACEFILE,              RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--nictrace",                 MODIFYVM_NICTRACE,                  RTGETOPT_REQ_BOOL_ONOFF | RTGETOPT_FLAG_INDEX },
    { "--nictype",                  MODIFYVM_NICTYPE,                   RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--nicspeed",                 MODIFYVM_NICSPEED,                  RTGETOPT_REQ_UINT32 | RTGETOPT_FLAG_INDEX },
    { "--nicbootprio",              MODIFYVM_NICBOOTPRIO,               RTGETOPT_REQ_UINT32 | RTGETOPT_FLAG_INDEX },
    { "--nicpromisc",               MODIFYVM_NICPROMISC,                RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--nic",                      MODIFYVM_NIC,                       RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--cableconnected",           MODIFYVM_CABLECONNECTED,            RTGETOPT_REQ_BOOL_ONOFF | RTGETOPT_FLAG_INDEX },
    { "--bridgeadapter",            MODIFYVM_BRIDGEADAPTER,             RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--hostonlyadapter",          MODIFYVM_HOSTONLYADAPTER,           RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--intnet",                   MODIFYVM_INTNET,                    RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--natnet",                   MODIFYVM_NATNET,                    RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
#ifdef VBOX_WITH_VDE
    { "--vdenet",                   MODIFYVM_VDENET,                    RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
#endif
    { "--natbindip",                MODIFYVM_NATBINDIP,                 RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--natsettings",              MODIFYVM_NATSETTINGS,               RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--natpf",                    MODIFYVM_NATPF,                     RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--nataliasmode",             MODIFYVM_NATALIASMODE,              RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--nattftpprefix",            MODIFYVM_NATTFTPPREFIX,             RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--nattftpfile",              MODIFYVM_NATTFTPFILE,               RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--nattftpserver",            MODIFYVM_NATTFTPSERVER,             RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--natdnspassdomain",         MODIFYVM_NATDNSPASSDOMAIN,          RTGETOPT_REQ_BOOL_ONOFF | RTGETOPT_FLAG_INDEX },
    { "--natdnsproxy",              MODIFYVM_NATDNSPROXY,               RTGETOPT_REQ_BOOL_ONOFF | RTGETOPT_FLAG_INDEX },
    { "--natdnshostresolver",       MODIFYVM_NATDNSHOSTRESOLVER,        RTGETOPT_REQ_BOOL_ONOFF | RTGETOPT_FLAG_INDEX },
    { "--macaddress",               MODIFYVM_MACADDRESS,                RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--mouse",                    MODIFYVM_HIDPTR,                    RTGETOPT_REQ_STRING },
    { "--keyboard",                 MODIFYVM_HIDKBD,                    RTGETOPT_REQ_STRING },
    { "--uartmode",                 MODIFYVM_UARTMODE,                  RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--uart",                     MODIFYVM_UART,                      RTGETOPT_REQ_STRING | RTGETOPT_FLAG_INDEX },
    { "--guestmemoryballoon",       MODIFYVM_GUESTMEMORYBALLOON,        RTGETOPT_REQ_UINT32 },
    { "--audiocontroller",          MODIFYVM_AUDIOCONTROLLER,           RTGETOPT_REQ_STRING },
    { "--audio",                    MODIFYVM_AUDIO,                     RTGETOPT_REQ_STRING },
    { "--clipboard",                MODIFYVM_CLIPBOARD,                 RTGETOPT_REQ_STRING },
    { "--vrdpport",                 MODIFYVM_VRDPPORT,                  RTGETOPT_REQ_STRING },     /* deprecated */
    { "--vrdpaddress",              MODIFYVM_VRDPADDRESS,               RTGETOPT_REQ_STRING },     /* deprecated */
    { "--vrdpauthtype",             MODIFYVM_VRDPAUTHTYPE,              RTGETOPT_REQ_STRING },     /* deprecated */
    { "--vrdpmulticon",             MODIFYVM_VRDPMULTICON,              RTGETOPT_REQ_BOOL_ONOFF }, /* deprecated */
    { "--vrdpreusecon",             MODIFYVM_VRDPREUSECON,              RTGETOPT_REQ_BOOL_ONOFF }, /* deprecated */
    { "--vrdpvideochannel",         MODIFYVM_VRDPVIDEOCHANNEL,          RTGETOPT_REQ_BOOL_ONOFF }, /* deprecated */
    { "--vrdpvideochannelquality",  MODIFYVM_VRDPVIDEOCHANNELQUALITY,   RTGETOPT_REQ_STRING },     /* deprecated */
    { "--vrdp",                     MODIFYVM_VRDP,                      RTGETOPT_REQ_BOOL_ONOFF }, /* deprecated */
    { "--vrdeproperty",             MODIFYVM_VRDEPROPERTY,              RTGETOPT_REQ_STRING },
    { "--vrdeport",                 MODIFYVM_VRDEPORT,                  RTGETOPT_REQ_STRING },
    { "--vrdeaddress",              MODIFYVM_VRDEADDRESS,               RTGETOPT_REQ_STRING },
    { "--vrdeauthtype",             MODIFYVM_VRDEAUTHTYPE,              RTGETOPT_REQ_STRING },
    { "--vrdeauthlibrary",          MODIFYVM_VRDEAUTHLIBRARY,           RTGETOPT_REQ_STRING },
    { "--vrdemulticon",             MODIFYVM_VRDEMULTICON,              RTGETOPT_REQ_BOOL_ONOFF },
    { "--vrdereusecon",             MODIFYVM_VRDEREUSECON,              RTGETOPT_REQ_BOOL_ONOFF },
    { "--vrdevideochannel",         MODIFYVM_VRDEVIDEOCHANNEL,          RTGETOPT_REQ_BOOL_ONOFF },
    { "--vrdevideochannelquality",  MODIFYVM_VRDEVIDEOCHANNELQUALITY,   RTGETOPT_REQ_STRING },
    { "--vrdeextpack",              MODIFYVM_VRDE_EXTPACK,              RTGETOPT_REQ_STRING },
    { "--vrde",                     MODIFYVM_VRDE,                      RTGETOPT_REQ_BOOL_ONOFF },
    { "--usbehci",                  MODIFYVM_USBEHCI,                   RTGETOPT_REQ_BOOL_ONOFF },
    { "--usb",                      MODIFYVM_USB,                       RTGETOPT_REQ_BOOL_ONOFF },
    { "--snapshotfolder",           MODIFYVM_SNAPSHOTFOLDER,            RTGETOPT_REQ_STRING },
    { "--teleporter",               MODIFYVM_TELEPORTER_ENABLED,        RTGETOPT_REQ_BOOL_ONOFF },
    { "--teleporterenabled",        MODIFYVM_TELEPORTER_ENABLED,        RTGETOPT_REQ_BOOL_ONOFF }, /* deprecated */
    { "--teleporterport",           MODIFYVM_TELEPORTER_PORT,           RTGETOPT_REQ_UINT32 },
    { "--teleporteraddress",        MODIFYVM_TELEPORTER_ADDRESS,        RTGETOPT_REQ_STRING },
    { "--teleporterpassword",       MODIFYVM_TELEPORTER_PASSWORD,       RTGETOPT_REQ_STRING },
    { "--hardwareuuid",             MODIFYVM_HARDWARE_UUID,             RTGETOPT_REQ_STRING },
    { "--hpet",                     MODIFYVM_HPET,                      RTGETOPT_REQ_BOOL_ONOFF },
    { "--iocache",                  MODIFYVM_IOCACHE,                   RTGETOPT_REQ_BOOL_ONOFF },
    { "--iocachesize",              MODIFYVM_IOCACHESIZE,               RTGETOPT_REQ_UINT32 },
    { "--faulttolerance",           MODIFYVM_FAULT_TOLERANCE,           RTGETOPT_REQ_STRING },
    { "--faulttoleranceaddress",    MODIFYVM_FAULT_TOLERANCE_ADDRESS,   RTGETOPT_REQ_STRING },
    { "--faulttoleranceport",       MODIFYVM_FAULT_TOLERANCE_PORT,      RTGETOPT_REQ_UINT32 },
    { "--faulttolerancepassword",   MODIFYVM_FAULT_TOLERANCE_PASSWORD,  RTGETOPT_REQ_STRING },
    { "--faulttolerancesyncinterval", MODIFYVM_FAULT_TOLERANCE_SYNC_INTERVAL, RTGETOPT_REQ_UINT32 },
    { "--chipset",                  MODIFYVM_CHIPSET,                   RTGETOPT_REQ_STRING },
};

static void vrdeWarningDeprecatedOption(const char *pszOption)
{
    RTStrmPrintf(g_pStdErr, "Warning: '--vrdp%s' is deprecated. Use '--vrde%s'.\n", pszOption, pszOption);
}


int handleModifyVM(HandlerArg *a)
{
    int c;
    HRESULT rc;
    Bstr name;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetOptState;
    ComPtr <IMachine> machine;
    ComPtr <IBIOSSettings> biosSettings;

    /* VM ID + at least one parameter. Parameter arguments are checked
     * individually. */
    if (a->argc < 2)
        return errorSyntax(USAGE_MODIFYVM, "Not enough parameters");

    ULONG SerialPortCount = 0;
    {
        ComPtr <ISystemProperties> info;
        CHECK_ERROR_RET(a->virtualBox, COMGETTER(SystemProperties)(info.asOutParam()), 1);
        CHECK_ERROR_RET(info, COMGETTER(SerialPortCount)(&SerialPortCount), 1);
    }

    /* try to find the given machine */
    CHECK_ERROR_RET(a->virtualBox, FindMachine(Bstr(a->argv[0]).raw(),
                                               machine.asOutParam()), 1);


    /* Get the number of network adapters */
    ULONG NetworkAdapterCount = getMaxNics(a->virtualBox, machine);

    /* open a session for the VM */
    CHECK_ERROR_RET(machine, LockMachine(a->session, LockType_Write), 1);

    /* get the mutable session machine */
    a->session->COMGETTER(Machine)(machine.asOutParam());
    machine->COMGETTER(BIOSSettings)(biosSettings.asOutParam());

    RTGetOptInit(&GetOptState, a->argc, a->argv, g_aModifyVMOptions,
                 RT_ELEMENTS(g_aModifyVMOptions), 1, RTGETOPTINIT_FLAGS_NO_STD_OPTS);

    while (   SUCCEEDED (rc)
           && (c = RTGetOpt(&GetOptState, &ValueUnion)))
    {
        switch (c)
        {
            case MODIFYVM_NAME:
            {
                CHECK_ERROR(machine, COMSETTER(Name)(Bstr(ValueUnion.psz).raw()));
                break;
            }
            case MODIFYVM_OSTYPE:
            {
                ComPtr<IGuestOSType> guestOSType;
                CHECK_ERROR(a->virtualBox, GetGuestOSType(Bstr(ValueUnion.psz).raw(),
                                                          guestOSType.asOutParam()));
                if (SUCCEEDED(rc) && guestOSType)
                {
                    CHECK_ERROR(machine, COMSETTER(OSTypeId)(Bstr(ValueUnion.psz).raw()));
                }
                else
                {
                    errorArgument("Invalid guest OS type '%s'", Utf8Str(ValueUnion.psz).c_str());
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_MEMORY:
            {
                CHECK_ERROR(machine, COMSETTER(MemorySize)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_PAGEFUSION:
            {
                CHECK_ERROR(machine, COMSETTER(PageFusionEnabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_VRAM:
            {
                CHECK_ERROR(machine, COMSETTER(VRAMSize)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_FIRMWARE:
            {
                if (!strcmp(ValueUnion.psz, "efi"))
                {
                    CHECK_ERROR(machine, COMSETTER(FirmwareType)(FirmwareType_EFI));
                }
                else if (!strcmp(ValueUnion.psz, "efi32"))
                {
                    CHECK_ERROR(machine, COMSETTER(FirmwareType)(FirmwareType_EFI32));
                }
                else if (!strcmp(ValueUnion.psz, "efi64"))
                {
                    CHECK_ERROR(machine, COMSETTER(FirmwareType)(FirmwareType_EFI64));
                }
                else if (!strcmp(ValueUnion.psz, "efidual"))
                {
                    CHECK_ERROR(machine, COMSETTER(FirmwareType)(FirmwareType_EFIDUAL));
                }
                else if (!strcmp(ValueUnion.psz, "bios"))
                {
                    CHECK_ERROR(machine, COMSETTER(FirmwareType)(FirmwareType_BIOS));
                }
                else
                {
                    errorArgument("Invalid --firmware argument '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_ACPI:
            {
                CHECK_ERROR(biosSettings, COMSETTER(ACPIEnabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_IOAPIC:
            {
                CHECK_ERROR(biosSettings, COMSETTER(IOAPICEnabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_PAE:
            {
                CHECK_ERROR(machine, SetCPUProperty(CPUPropertyType_PAE, ValueUnion.f));
                break;
            }

            case MODIFYVM_SYNTHCPU:
            {
                CHECK_ERROR(machine, SetCPUProperty(CPUPropertyType_Synthetic, ValueUnion.f));
                break;
            }

            case MODIFYVM_HWVIRTEX:
            {
                CHECK_ERROR(machine, SetHWVirtExProperty(HWVirtExPropertyType_Enabled, ValueUnion.f));
                break;
            }

            case MODIFYVM_HWVIRTEXEXCLUSIVE:
            {
                CHECK_ERROR(machine, SetHWVirtExProperty(HWVirtExPropertyType_Exclusive, ValueUnion.f));
                break;
            }

            case MODIFYVM_SETCPUID:
            {
                uint32_t id = ValueUnion.u32;
                uint32_t aValue[4];

                for (unsigned i = 0 ; i < 4 ; i++)
                {
                    int vrc = RTGetOptFetchValue(&GetOptState, &ValueUnion, RTGETOPT_REQ_UINT32 | RTGETOPT_FLAG_HEX);
                    if (RT_FAILURE(vrc))
                        return errorSyntax(USAGE_MODIFYVM,
                                           "Missing or Invalid argument to '%s'",
                                           GetOptState.pDef->pszLong);
                    aValue[i] = ValueUnion.u32;
                }
                CHECK_ERROR(machine, SetCPUIDLeaf(id, aValue[0], aValue[1], aValue[2], aValue[3]));
                break;
            }

            case MODIFYVM_DELCPUID:
            {
                CHECK_ERROR(machine, RemoveCPUIDLeaf(ValueUnion.u32));
                break;
            }

            case MODIFYVM_DELALLCPUID:
            {
                CHECK_ERROR(machine, RemoveAllCPUIDLeaves());
                break;
            }

            case MODIFYVM_NESTEDPAGING:
            {
                CHECK_ERROR(machine, SetHWVirtExProperty(HWVirtExPropertyType_NestedPaging, ValueUnion.f));
                break;
            }

            case MODIFYVM_LARGEPAGES:
            {
                CHECK_ERROR(machine, SetHWVirtExProperty(HWVirtExPropertyType_LargePages, ValueUnion.f));
                break;
            }

            case MODIFYVM_VTXVPID:
            {
                CHECK_ERROR(machine, SetHWVirtExProperty(HWVirtExPropertyType_VPID, ValueUnion.f));
                break;
            }

            case MODIFYVM_CPUS:
            {
                CHECK_ERROR(machine, COMSETTER(CPUCount)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_RTCUSEUTC:
            {
                CHECK_ERROR(machine, COMSETTER(RTCUseUTC)(ValueUnion.f));
                break;
            }

            case MODIFYVM_CPUHOTPLUG:
            {
                CHECK_ERROR(machine, COMSETTER(CPUHotPlugEnabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_PLUGCPU:
            {
                CHECK_ERROR(machine, HotPlugCPU(ValueUnion.u32));
                break;
            }

            case MODIFYVM_UNPLUGCPU:
            {
                CHECK_ERROR(machine, HotUnplugCPU(ValueUnion.u32));
                break;
            }

            case MODIFYVM_CPU_EXECTUION_CAP:
            {
                CHECK_ERROR(machine, COMSETTER(CPUExecutionCap)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_MONITORCOUNT:
            {
                CHECK_ERROR(machine, COMSETTER(MonitorCount)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_ACCELERATE3D:
            {
                CHECK_ERROR(machine, COMSETTER(Accelerate3DEnabled)(ValueUnion.f));
                break;
            }

#ifdef VBOX_WITH_VIDEOHWACCEL
            case MODIFYVM_ACCELERATE2DVIDEO:
            {
                CHECK_ERROR(machine, COMSETTER(Accelerate2DVideoEnabled)(ValueUnion.f));
                break;
            }
#endif

            case MODIFYVM_BIOSLOGOFADEIN:
            {
                CHECK_ERROR(biosSettings, COMSETTER(LogoFadeIn)(ValueUnion.f));
                break;
            }

            case MODIFYVM_BIOSLOGOFADEOUT:
            {
                CHECK_ERROR(biosSettings, COMSETTER(LogoFadeOut)(ValueUnion.f));
                break;
            }

            case MODIFYVM_BIOSLOGODISPLAYTIME:
            {
                CHECK_ERROR(biosSettings, COMSETTER(LogoDisplayTime)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_BIOSLOGOIMAGEPATH:
            {
                CHECK_ERROR(biosSettings, COMSETTER(LogoImagePath)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_BIOSBOOTMENU:
            {
                if (!strcmp(ValueUnion.psz, "disabled"))
                {
                    CHECK_ERROR(biosSettings, COMSETTER(BootMenuMode)(BIOSBootMenuMode_Disabled));
                }
                else if (!strcmp(ValueUnion.psz, "menuonly"))
                {
                    CHECK_ERROR(biosSettings, COMSETTER(BootMenuMode)(BIOSBootMenuMode_MenuOnly));
                }
                else if (!strcmp(ValueUnion.psz, "messageandmenu"))
                {
                    CHECK_ERROR(biosSettings, COMSETTER(BootMenuMode)(BIOSBootMenuMode_MessageAndMenu));
                }
                else
                {
                    errorArgument("Invalid --biosbootmenu argument '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_BIOSSYSTEMTIMEOFFSET:
            {
                CHECK_ERROR(biosSettings, COMSETTER(TimeOffset)(ValueUnion.i64));
                break;
            }

            case MODIFYVM_BIOSPXEDEBUG:
            {
                CHECK_ERROR(biosSettings, COMSETTER(PXEDebugEnabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_BOOT:
            {
                if (!strcmp(ValueUnion.psz, "none"))
                {
                    CHECK_ERROR(machine, SetBootOrder(GetOptState.uIndex, DeviceType_Null));
                }
                else if (!strcmp(ValueUnion.psz, "floppy"))
                {
                    CHECK_ERROR(machine, SetBootOrder(GetOptState.uIndex, DeviceType_Floppy));
                }
                else if (!strcmp(ValueUnion.psz, "dvd"))
                {
                    CHECK_ERROR(machine, SetBootOrder(GetOptState.uIndex, DeviceType_DVD));
                }
                else if (!strcmp(ValueUnion.psz, "disk"))
                {
                    CHECK_ERROR(machine, SetBootOrder(GetOptState.uIndex, DeviceType_HardDisk));
                }
                else if (!strcmp(ValueUnion.psz, "net"))
                {
                    CHECK_ERROR(machine, SetBootOrder(GetOptState.uIndex, DeviceType_Network));
                }
                else
                    return errorArgument("Invalid boot device '%s'", ValueUnion.psz);
                break;
            }

            case MODIFYVM_HDA: // deprecated
            case MODIFYVM_HDB: // deprecated
            case MODIFYVM_HDD: // deprecated
            case MODIFYVM_SATAPORT: // deprecated
            {
                uint32_t u1 = 0, u2 = 0;
                Bstr bstrController = L"IDE Controller";

                switch (c)
                {
                    case MODIFYVM_HDA: // deprecated
                        u1 = 0;
                    break;

                    case MODIFYVM_HDB: // deprecated
                        u1 = 0;
                        u2 = 1;
                    break;

                    case MODIFYVM_HDD: // deprecated
                        u1 = 1;
                        u2 = 1;
                    break;

                    case MODIFYVM_SATAPORT: // deprecated
                        u1 = GetOptState.uIndex;
                        bstrController = L"SATA";
                    break;
                }

                if (!strcmp(ValueUnion.psz, "none"))
                {
                    machine->DetachDevice(bstrController.raw(), u1, u2);
                }
                else
                {
                    ComPtr<IMedium> hardDisk;
                    rc = findOrOpenMedium(a, ValueUnion.psz, DeviceType_HardDisk,
                                          hardDisk, NULL);
                    if (FAILED(rc))
                        break;
                    if (hardDisk)
                    {
                        CHECK_ERROR(machine, AttachDevice(bstrController.raw(),
                                                          u1, u2,
                                                          DeviceType_HardDisk,
                                                          hardDisk));
                    }
                    else
                        rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_IDECONTROLLER: // deprecated
            {
                ComPtr<IStorageController> storageController;
                CHECK_ERROR(machine, GetStorageControllerByName(Bstr("IDE Controller").raw(),
                                                                 storageController.asOutParam()));

                if (!RTStrICmp(ValueUnion.psz, "PIIX3"))
                {
                    CHECK_ERROR(storageController, COMSETTER(ControllerType)(StorageControllerType_PIIX3));
                }
                else if (!RTStrICmp(ValueUnion.psz, "PIIX4"))
                {
                    CHECK_ERROR(storageController, COMSETTER(ControllerType)(StorageControllerType_PIIX4));
                }
                else if (!RTStrICmp(ValueUnion.psz, "ICH6"))
                {
                    CHECK_ERROR(storageController, COMSETTER(ControllerType)(StorageControllerType_ICH6));
                }
                else
                {
                    errorArgument("Invalid --idecontroller argument '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_SATAIDEEMULATION: // deprecated
            {
                ComPtr<IStorageController> SataCtl;
                CHECK_ERROR(machine, GetStorageControllerByName(Bstr("SATA").raw(),
                                                                SataCtl.asOutParam()));

                if (SUCCEEDED(rc))
                    CHECK_ERROR(SataCtl, SetIDEEmulationPort(GetOptState.uIndex, ValueUnion.u32));
                break;
            }

            case MODIFYVM_SATAPORTCOUNT: // deprecated
            {
                ComPtr<IStorageController> SataCtl;
                CHECK_ERROR(machine, GetStorageControllerByName(Bstr("SATA").raw(),
                                                                SataCtl.asOutParam()));

                if (SUCCEEDED(rc) && ValueUnion.u32 > 0)
                    CHECK_ERROR(SataCtl, COMSETTER(PortCount)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_SATA: // deprecated
            {
                if (!strcmp(ValueUnion.psz, "on") || !strcmp(ValueUnion.psz, "enable"))
                {
                    ComPtr<IStorageController> ctl;
                    CHECK_ERROR(machine, AddStorageController(Bstr("SATA").raw(),
                                                              StorageBus_SATA,
                                                              ctl.asOutParam()));
                    CHECK_ERROR(ctl, COMSETTER(ControllerType)(StorageControllerType_IntelAhci));
                }
                else if (!strcmp(ValueUnion.psz, "off") || !strcmp(ValueUnion.psz, "disable"))
                    CHECK_ERROR(machine, RemoveStorageController(Bstr("SATA").raw()));
                else
                    return errorArgument("Invalid --usb argument '%s'", ValueUnion.psz);
                break;
            }

            case MODIFYVM_SCSIPORT: // deprecated
            {
                if (!strcmp(ValueUnion.psz, "none"))
                {
                    rc = machine->DetachDevice(Bstr("LsiLogic").raw(),
                                               GetOptState.uIndex, 0);
                    if (FAILED(rc))
                        CHECK_ERROR(machine, DetachDevice(Bstr("BusLogic").raw(),
                                                          GetOptState.uIndex, 0));
                }
                else
                {
                    ComPtr<IMedium> hardDisk;
                    rc = findOrOpenMedium(a, ValueUnion.psz, DeviceType_HardDisk,
                                          hardDisk, NULL);
                    if (FAILED(rc))
                        break;
                    if (hardDisk)
                    {
                        rc = machine->AttachDevice(Bstr("LsiLogic").raw(),
                                                   GetOptState.uIndex, 0,
                                                   DeviceType_HardDisk,
                                                   hardDisk);
                        if (FAILED(rc))
                            CHECK_ERROR(machine,
                                        AttachDevice(Bstr("BusLogic").raw(),
                                                     GetOptState.uIndex, 0,
                                                     DeviceType_HardDisk,
                                                     hardDisk));
                    }
                    else
                        rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_SCSITYPE: // deprecated
            {
                ComPtr<IStorageController> ctl;

                if (!RTStrICmp(ValueUnion.psz, "LsiLogic"))
                {
                    rc = machine->RemoveStorageController(Bstr("BusLogic").raw());
                    if (FAILED(rc))
                        CHECK_ERROR(machine, RemoveStorageController(Bstr("LsiLogic").raw()));

                    CHECK_ERROR(machine,
                                 AddStorageController(Bstr("LsiLogic").raw(),
                                                      StorageBus_SCSI,
                                                      ctl.asOutParam()));

                    if (SUCCEEDED(rc))
                        CHECK_ERROR(ctl, COMSETTER(ControllerType)(StorageControllerType_LsiLogic));
                }
                else if (!RTStrICmp(ValueUnion.psz, "BusLogic"))
                {
                    rc = machine->RemoveStorageController(Bstr("LsiLogic").raw());
                    if (FAILED(rc))
                        CHECK_ERROR(machine, RemoveStorageController(Bstr("BusLogic").raw()));

                    CHECK_ERROR(machine,
                                 AddStorageController(Bstr("BusLogic").raw(),
                                                      StorageBus_SCSI,
                                                      ctl.asOutParam()));

                    if (SUCCEEDED(rc))
                        CHECK_ERROR(ctl, COMSETTER(ControllerType)(StorageControllerType_BusLogic));
                }
                else
                    return errorArgument("Invalid --scsitype argument '%s'", ValueUnion.psz);
                break;
            }

            case MODIFYVM_SCSI: // deprecated
            {
                if (!strcmp(ValueUnion.psz, "on") || !strcmp(ValueUnion.psz, "enable"))
                {
                    ComPtr<IStorageController> ctl;

                    CHECK_ERROR(machine, AddStorageController(Bstr("BusLogic").raw(),
                                                              StorageBus_SCSI,
                                                              ctl.asOutParam()));
                    if (SUCCEEDED(rc))
                        CHECK_ERROR(ctl, COMSETTER(ControllerType)(StorageControllerType_BusLogic));
                }
                else if (!strcmp(ValueUnion.psz, "off") || !strcmp(ValueUnion.psz, "disable"))
                {
                    rc = machine->RemoveStorageController(Bstr("BusLogic").raw());
                    if (FAILED(rc))
                        CHECK_ERROR(machine, RemoveStorageController(Bstr("LsiLogic").raw()));
                }
                break;
            }

            case MODIFYVM_DVDPASSTHROUGH: // deprecated
            {
                CHECK_ERROR(machine, PassthroughDevice(Bstr("IDE Controller").raw(),
                                                       1, 0,
                                                       !strcmp(ValueUnion.psz, "on")));
                break;
            }

            case MODIFYVM_DVD: // deprecated
            {
                ComPtr<IMedium> dvdMedium;

                /* unmount? */
                if (!strcmp(ValueUnion.psz, "none"))
                {
                    /* nothing to do, NULL object will cause unmount */
                }
                /* host drive? */
                else if (!strncmp(ValueUnion.psz, "host:", 5))
                {
                    ComPtr<IHost> host;
                    CHECK_ERROR(a->virtualBox, COMGETTER(Host)(host.asOutParam()));
                    rc = host->FindHostDVDDrive(Bstr(ValueUnion.psz + 5).raw(),
                                                dvdMedium.asOutParam());
                    if (!dvdMedium)
                    {
                        /* 2nd try: try with the real name, important on Linux+libhal */
                        char szPathReal[RTPATH_MAX];
                        if (RT_FAILURE(RTPathReal(ValueUnion.psz + 5, szPathReal, sizeof(szPathReal))))
                        {
                            errorArgument("Invalid host DVD drive name \"%s\"", ValueUnion.psz + 5);
                            rc = E_FAIL;
                            break;
                        }
                        rc = host->FindHostDVDDrive(Bstr(szPathReal).raw(),
                                                    dvdMedium.asOutParam());
                        if (!dvdMedium)
                        {
                            errorArgument("Invalid host DVD drive name \"%s\"", ValueUnion.psz + 5);
                            rc = E_FAIL;
                            break;
                        }
                    }
                }
                else
                {
                    rc = findOrOpenMedium(a, ValueUnion.psz, DeviceType_DVD,
                                          dvdMedium, NULL);
                    if (FAILED(rc))
                        break;
                    if (!dvdMedium)
                    {
                        rc = E_FAIL;
                        break;
                    }
                }

                CHECK_ERROR(machine, MountMedium(Bstr("IDE Controller").raw(),
                                                 1, 0,
                                                 dvdMedium,
                                                 FALSE /* aForce */));
                break;
            }

            case MODIFYVM_FLOPPY: // deprecated
            {
                ComPtr<IMedium> floppyMedium;
                ComPtr<IMediumAttachment> floppyAttachment;
                machine->GetMediumAttachment(Bstr("Floppy Controller").raw(),
                                             0, 0, floppyAttachment.asOutParam());

                /* disable? */
                if (!strcmp(ValueUnion.psz, "disabled"))
                {
                    /* disable the controller */
                    if (floppyAttachment)
                        CHECK_ERROR(machine, DetachDevice(Bstr("Floppy Controller").raw(),
                                                          0, 0));
                }
                else
                {
                    /* enable the controller */
                    if (!floppyAttachment)
                        CHECK_ERROR(machine, AttachDevice(Bstr("Floppy Controller").raw(),
                                                          0, 0,
                                                          DeviceType_Floppy, NULL));

                    /* unmount? */
                    if (    !strcmp(ValueUnion.psz, "none")
                        ||  !strcmp(ValueUnion.psz, "empty"))   // deprecated
                    {
                        /* nothing to do, NULL object will cause unmount */
                    }
                    /* host drive? */
                    else if (!strncmp(ValueUnion.psz, "host:", 5))
                    {
                        ComPtr<IHost> host;
                        CHECK_ERROR(a->virtualBox, COMGETTER(Host)(host.asOutParam()));
                        rc = host->FindHostFloppyDrive(Bstr(ValueUnion.psz + 5).raw(),
                                                       floppyMedium.asOutParam());
                        if (!floppyMedium)
                        {
                            errorArgument("Invalid host floppy drive name \"%s\"", ValueUnion.psz + 5);
                            rc = E_FAIL;
                            break;
                        }
                    }
                    else
                    {
                        rc = findOrOpenMedium(a, ValueUnion.psz, DeviceType_Floppy,
                                              floppyMedium, NULL);
                        if (FAILED(rc))
                            break;
                        if (!floppyMedium)
                        {
                            rc = E_FAIL;
                            break;
                        }
                    }
                    CHECK_ERROR(machine, MountMedium(Bstr("Floppy Controller").raw(),
                                                     0, 0,
                                                     floppyMedium,
                                                     FALSE /* aForce */));
                }
                break;
            }

            case MODIFYVM_NICTRACEFILE:
            {
                ComPtr<INetworkAdapter> nic;

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMSETTER(TraceFile)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_NICTRACE:
            {
                ComPtr<INetworkAdapter> nic;

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMSETTER(TraceEnabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_NICTYPE:
            {
                ComPtr<INetworkAdapter> nic;

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                if (!strcmp(ValueUnion.psz, "Am79C970A"))
                {
                    CHECK_ERROR(nic, COMSETTER(AdapterType)(NetworkAdapterType_Am79C970A));
                }
                else if (!strcmp(ValueUnion.psz, "Am79C973"))
                {
                    CHECK_ERROR(nic, COMSETTER(AdapterType)(NetworkAdapterType_Am79C973));
                }
#ifdef VBOX_WITH_E1000
                else if (!strcmp(ValueUnion.psz, "82540EM"))
                {
                    CHECK_ERROR(nic, COMSETTER(AdapterType)(NetworkAdapterType_I82540EM));
                }
                else if (!strcmp(ValueUnion.psz, "82543GC"))
                {
                    CHECK_ERROR(nic, COMSETTER(AdapterType)(NetworkAdapterType_I82543GC));
                }
                else if (!strcmp(ValueUnion.psz, "82545EM"))
                {
                    CHECK_ERROR(nic, COMSETTER(AdapterType)(NetworkAdapterType_I82545EM));
                }
#endif
#ifdef VBOX_WITH_VIRTIO
                else if (!strcmp(ValueUnion.psz, "virtio"))
                {
                    CHECK_ERROR(nic, COMSETTER(AdapterType)(NetworkAdapterType_Virtio));
                }
#endif /* VBOX_WITH_VIRTIO */
                else
                {
                    errorArgument("Invalid NIC type '%s' specified for NIC %u", ValueUnion.psz, GetOptState.uIndex);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_NICSPEED:
            {
                ComPtr<INetworkAdapter> nic;

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMSETTER(LineSpeed)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_NICBOOTPRIO:
            {
                ComPtr<INetworkAdapter> nic;

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                /* Somewhat arbitrary limitation - we can pass a list of up to 4 PCI devices
                 * to the PXE ROM, hence only boot priorities 1-4 are allowed (in addition to
                 * 0 for the default lowest priority).
                 */
                if (ValueUnion.u32 > 4)
                {
                    errorArgument("Invalid boot priority '%u' specfied for NIC %u", ValueUnion.u32, GetOptState.uIndex);
                    rc = E_FAIL;
                }
                else
                {
                    CHECK_ERROR(nic, COMSETTER(BootPriority)(ValueUnion.u32));
                }
                break;
            }

            case MODIFYVM_NICPROMISC:
            {
                NetworkAdapterPromiscModePolicy_T enmPromiscModePolicy;
                if (!strcmp(ValueUnion.psz, "deny"))
                    enmPromiscModePolicy = NetworkAdapterPromiscModePolicy_Deny;
                else if (   !strcmp(ValueUnion.psz, "allow-vms")
                         || !strcmp(ValueUnion.psz, "allow-network"))
                    enmPromiscModePolicy = NetworkAdapterPromiscModePolicy_AllowNetwork;
                else if (!strcmp(ValueUnion.psz, "allow-all"))
                    enmPromiscModePolicy = NetworkAdapterPromiscModePolicy_AllowAll;
                else
                {
                    errorArgument("Unknown promiscuous mode policy '%s'", ValueUnion.psz);
                    rc = E_INVALIDARG;
                    break;
                }

                ComPtr<INetworkAdapter> nic;
                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMSETTER(PromiscModePolicy)(enmPromiscModePolicy));
                break;
            }

            case MODIFYVM_NIC:
            {
                ComPtr<INetworkAdapter> nic;

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                if (!strcmp(ValueUnion.psz, "none"))
                {
                    CHECK_ERROR(nic, COMSETTER(Enabled)(FALSE));
                }
                else if (!strcmp(ValueUnion.psz, "null"))
                {
                    CHECK_ERROR(nic, COMSETTER(Enabled)(TRUE));
                    CHECK_ERROR(nic, Detach());
                }
                else if (!strcmp(ValueUnion.psz, "nat"))
                {
                    CHECK_ERROR(nic, COMSETTER(Enabled)(TRUE));
                    CHECK_ERROR(nic, AttachToNAT());
                }
                else if (  !strcmp(ValueUnion.psz, "bridged")
                        || !strcmp(ValueUnion.psz, "hostif")) /* backward compatibility */
                {
                    CHECK_ERROR(nic, COMSETTER(Enabled)(TRUE));
                    CHECK_ERROR(nic, AttachToBridgedInterface());
                }
                else if (!strcmp(ValueUnion.psz, "intnet"))
                {
                    CHECK_ERROR(nic, COMSETTER(Enabled)(TRUE));
                    CHECK_ERROR(nic, AttachToInternalNetwork());
                }
#if defined(VBOX_WITH_NETFLT)
                else if (!strcmp(ValueUnion.psz, "hostonly"))
                {

                    CHECK_ERROR(nic, COMSETTER(Enabled)(TRUE));
                    CHECK_ERROR(nic, AttachToHostOnlyInterface());
                }
#endif
#ifdef VBOX_WITH_VDE
                else if (!strcmp(ValueUnion.psz, "vde"))
                {

                    CHECK_ERROR(nic, COMSETTER(Enabled)(TRUE));
                    CHECK_ERROR(nic, AttachToVDE());
                }
#endif
                else
                {
                    errorArgument("Invalid type '%s' specfied for NIC %u", ValueUnion.psz, GetOptState.uIndex);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_CABLECONNECTED:
            {
                ComPtr<INetworkAdapter> nic;

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMSETTER(CableConnected)(ValueUnion.f));
                break;
            }

            case MODIFYVM_BRIDGEADAPTER:
            case MODIFYVM_HOSTONLYADAPTER:
            {
                ComPtr<INetworkAdapter> nic;

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                /** @todo NULL string deprecated */
                /* remove it? */
                if (!strcmp(ValueUnion.psz, "none"))
                {
                    CHECK_ERROR(nic, COMSETTER(HostInterface)(NULL));
                }
                else
                {
                    CHECK_ERROR(nic, COMSETTER(HostInterface)(Bstr(ValueUnion.psz).raw()));
                }
                break;
            }

            case MODIFYVM_INTNET:
            {
                ComPtr<INetworkAdapter> nic;

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                /** @todo NULL string deprecated */
                /* remove it? */
                if (!strcmp(ValueUnion.psz, "none"))
                {
                    CHECK_ERROR(nic, COMSETTER(InternalNetwork)(NULL));
                }
                else
                {
                    CHECK_ERROR(nic, COMSETTER(InternalNetwork)(Bstr(ValueUnion.psz).raw()));
                }
                break;
            }

#ifdef VBOX_WITH_VDE
            case MODIFYVM_VDENET:
            {
                ComPtr<INetworkAdapter> nic;

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                /** @todo NULL string deprecated */
                /* remove it? */
                if (!strcmp(ValueUnion.psz, "default"))
                {
                    CHECK_ERROR(nic, COMSETTER(VDENetwork)(NULL));
                }
                else
                {
                    CHECK_ERROR(nic, COMSETTER(VDENetwork)(Bstr(ValueUnion.psz).raw()));
                }
                break;
            }
#endif
            case MODIFYVM_NATNET:
            {
                ComPtr<INetworkAdapter> nic;
                ComPtr<INATEngine> driver;

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMGETTER(NatDriver)(driver.asOutParam()));

                const char *psz = ValueUnion.psz;
                if (!strcmp("default", psz))
                    psz = "";

                CHECK_ERROR(driver, COMSETTER(Network)(Bstr(psz).raw()));
                break;
            }

            case MODIFYVM_NATBINDIP:
            {
                ComPtr<INetworkAdapter> nic;
                ComPtr<INATEngine> driver;

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMGETTER(NatDriver)(driver.asOutParam()));
                CHECK_ERROR(driver, COMSETTER(HostIP)(Bstr(ValueUnion.psz).raw()));
                break;
            }

#define ITERATE_TO_NEXT_TERM(ch)                                           \
    do {                                                                   \
        while (*ch != ',')                                                 \
        {                                                                  \
            if (*ch == 0)                                                  \
            {                                                              \
                return errorSyntax(USAGE_MODIFYVM,                         \
                                   "Missing or Invalid argument to '%s'",  \
                                    GetOptState.pDef->pszLong);            \
            }                                                              \
            ch++;                                                          \
        }                                                                  \
        *ch = '\0';                                                        \
        ch++;                                                              \
    } while(0)

            case MODIFYVM_NATSETTINGS:
            {
                ComPtr<INetworkAdapter> nic;
                ComPtr<INATEngine> driver;
                char *strMtu;
                char *strSockSnd;
                char *strSockRcv;
                char *strTcpSnd;
                char *strTcpRcv;
                char *strRaw = RTStrDup(ValueUnion.psz);
                char *ch = strRaw;
                strMtu = RTStrStrip(ch);
                ITERATE_TO_NEXT_TERM(ch);
                strSockSnd = RTStrStrip(ch);
                ITERATE_TO_NEXT_TERM(ch);
                strSockRcv = RTStrStrip(ch);
                ITERATE_TO_NEXT_TERM(ch);
                strTcpSnd = RTStrStrip(ch);
                ITERATE_TO_NEXT_TERM(ch);
                strTcpRcv = RTStrStrip(ch);

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMGETTER(NatDriver)(driver.asOutParam()));
                CHECK_ERROR(driver, SetNetworkSettings(RTStrToUInt32(strMtu), RTStrToUInt32(strSockSnd), RTStrToUInt32(strSockRcv),
                                    RTStrToUInt32(strTcpSnd), RTStrToUInt32(strTcpRcv)));
                break;
            }


            case MODIFYVM_NATPF:
            {
                ComPtr<INetworkAdapter> nic;
                ComPtr<INATEngine> driver;

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMGETTER(NatDriver)(driver.asOutParam()));
                /* format name:proto:hostip:hostport:guestip:guestport*/
                if (RTStrCmp(ValueUnion.psz, "delete") != 0)
                {
                    char *strName;
                    char *strProto;
                    char *strHostIp;
                    char *strHostPort;
                    char *strGuestIp;
                    char *strGuestPort;
                    char *strRaw = RTStrDup(ValueUnion.psz);
                    char *ch = strRaw;
                    strName = RTStrStrip(ch);
                    ITERATE_TO_NEXT_TERM(ch);
                    strProto = RTStrStrip(ch);
                    ITERATE_TO_NEXT_TERM(ch);
                    strHostIp = RTStrStrip(ch);
                    ITERATE_TO_NEXT_TERM(ch);
                    strHostPort = RTStrStrip(ch);
                    ITERATE_TO_NEXT_TERM(ch);
                    strGuestIp = RTStrStrip(ch);
                    ITERATE_TO_NEXT_TERM(ch);
                    strGuestPort = RTStrStrip(ch);
                    NATProtocol_T proto;
                    if (RTStrICmp(strProto, "udp") == 0)
                        proto = NATProtocol_UDP;
                    else if (RTStrICmp(strProto, "tcp") == 0)
                        proto = NATProtocol_TCP;
                    else
                    {
                        errorArgument("Invalid proto '%s' specfied for NIC %u", ValueUnion.psz, GetOptState.uIndex);
                        rc = E_FAIL;
                        break;
                    }
                    CHECK_ERROR(driver, AddRedirect(Bstr(strName).raw(), proto,
                                        Bstr(strHostIp).raw(),
                                        RTStrToUInt16(strHostPort),
                                        Bstr(strGuestIp).raw(),
                                        RTStrToUInt16(strGuestPort)));
                }
                else
                {
                    /* delete NAT Rule operation */
                    int vrc;
                    vrc = RTGetOptFetchValue(&GetOptState, &ValueUnion, RTGETOPT_REQ_STRING);
                    if (RT_FAILURE(vrc))
                        return errorSyntax(USAGE_MODIFYVM, "Not enough parameters");
                    CHECK_ERROR(driver, RemoveRedirect(Bstr(ValueUnion.psz).raw()));
                }
                break;
            }
            #undef ITERATE_TO_NEXT_TERM
            case MODIFYVM_NATALIASMODE:
            {
                ComPtr<INetworkAdapter> nic;
                ComPtr<INATEngine> driver;
                uint32_t aliasMode = 0;

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMGETTER(NatDriver)(driver.asOutParam()));
                if (RTStrCmp(ValueUnion.psz,"default") == 0)
                {
                    aliasMode = 0;
                }
                else
                {
                    char *token = (char *)ValueUnion.psz;
                    while(token)
                    {
                        if (RTStrNCmp(token, "log", 3) == 0)
                            aliasMode |= 0x1;
                        else if (RTStrNCmp(token, "proxyonly", 9) == 0)
                            aliasMode |= 0x2;
                        else if (RTStrNCmp(token, "sameports", 9) == 0)
                            aliasMode |= 0x4;
                        token = RTStrStr(token, ",");
                        if (token == NULL)
                            break;
                        token++;
                    }
                }
                CHECK_ERROR(driver, COMSETTER(AliasMode)(aliasMode));
                break;
            }

            case MODIFYVM_NATTFTPPREFIX:
            {
                ComPtr<INetworkAdapter> nic;
                ComPtr<INATEngine> driver;

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMGETTER(NatDriver)(driver.asOutParam()));
                CHECK_ERROR(driver, COMSETTER(TftpPrefix)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_NATTFTPFILE:
            {
                ComPtr<INetworkAdapter> nic;
                ComPtr<INATEngine> driver;

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMGETTER(NatDriver)(driver.asOutParam()));
                CHECK_ERROR(driver, COMSETTER(TftpBootFile)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_NATTFTPSERVER:
            {
                ComPtr<INetworkAdapter> nic;
                ComPtr<INATEngine> driver;

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMGETTER(NatDriver)(driver.asOutParam()));
                CHECK_ERROR(driver, COMSETTER(TftpNextServer)(Bstr(ValueUnion.psz).raw()));
                break;
            }
            case MODIFYVM_NATDNSPASSDOMAIN:
            {
                ComPtr<INetworkAdapter> nic;
                ComPtr<INATEngine> driver;

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMGETTER(NatDriver)(driver.asOutParam()));
                CHECK_ERROR(driver, COMSETTER(DnsPassDomain)(ValueUnion.f));
                break;
            }

            case MODIFYVM_NATDNSPROXY:
            {
                ComPtr<INetworkAdapter> nic;
                ComPtr<INATEngine> driver;

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMGETTER(NatDriver)(driver.asOutParam()));
                CHECK_ERROR(driver, COMSETTER(DnsProxy)(ValueUnion.f));
                break;
            }

            case MODIFYVM_NATDNSHOSTRESOLVER:
            {
                ComPtr<INetworkAdapter> nic;
                ComPtr<INATEngine> driver;

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                CHECK_ERROR(nic, COMGETTER(NatDriver)(driver.asOutParam()));
                CHECK_ERROR(driver, COMSETTER(DnsUseHostResolver)(ValueUnion.f));
                break;
            }
            case MODIFYVM_MACADDRESS:
            {
                ComPtr<INetworkAdapter> nic;

                CHECK_ERROR_BREAK(machine, GetNetworkAdapter(GetOptState.uIndex - 1, nic.asOutParam()));
                ASSERT(nic);

                /* generate one? */
                if (!strcmp(ValueUnion.psz, "auto"))
                {
                    CHECK_ERROR(nic, COMSETTER(MACAddress)(NULL));
                }
                else
                {
                    CHECK_ERROR(nic, COMSETTER(MACAddress)(Bstr(ValueUnion.psz).raw()));
                }
                break;
            }

            case MODIFYVM_HIDPTR:
            {
                bool fEnableUsb = false;
                if (!strcmp(ValueUnion.psz, "ps2"))
                {
                    CHECK_ERROR(machine, COMSETTER(PointingHidType)(PointingHidType_PS2Mouse));
                }
                else if (!strcmp(ValueUnion.psz, "usb"))
                {
                    CHECK_ERROR(machine, COMSETTER(PointingHidType)(PointingHidType_USBMouse));
                    if (SUCCEEDED(rc))
                        fEnableUsb = true;
                }
                else if (!strcmp(ValueUnion.psz, "usbtablet"))
                {
                    CHECK_ERROR(machine, COMSETTER(PointingHidType)(PointingHidType_USBTablet));
                    if (SUCCEEDED(rc))
                        fEnableUsb = true;
                }
                else
                {
                    errorArgument("Invalid type '%s' specfied for pointing device", ValueUnion.psz);
                    rc = E_FAIL;
                }
                if (fEnableUsb)
                {
                    /* Make sure the OHCI controller is enabled. */
                    ComPtr<IUSBController> UsbCtl;
                    rc = machine->COMGETTER(USBController)(UsbCtl.asOutParam());
                    if (SUCCEEDED(rc))
                    {
                        BOOL fEnabled;
                        rc = UsbCtl->COMGETTER(Enabled)(&fEnabled);
                        if (FAILED(rc))
                            fEnabled = false;
                        if (!fEnabled)
                            CHECK_ERROR(UsbCtl, COMSETTER(Enabled)(true));
                    }
                }
                break;
            }

            case MODIFYVM_HIDKBD:
            {
                bool fEnableUsb = false;
                if (!strcmp(ValueUnion.psz, "ps2"))
                {
                    CHECK_ERROR(machine, COMSETTER(KeyboardHidType)(KeyboardHidType_PS2Keyboard));
                }
                else if (!strcmp(ValueUnion.psz, "usb"))
                {
                    CHECK_ERROR(machine, COMSETTER(KeyboardHidType)(KeyboardHidType_USBKeyboard));
                    if (SUCCEEDED(rc))
                        fEnableUsb = true;
                }
                else
                {
                    errorArgument("Invalid type '%s' specfied for keyboard", ValueUnion.psz);
                    rc = E_FAIL;
                }
                if (fEnableUsb)
                {
                    /* Make sure the OHCI controller is enabled. */
                    ComPtr<IUSBController> UsbCtl;
                    rc = machine->COMGETTER(USBController)(UsbCtl.asOutParam());
                    if (SUCCEEDED(rc))
                    {
                        BOOL fEnabled;
                        rc = UsbCtl->COMGETTER(Enabled)(&fEnabled);
                        if (FAILED(rc))
                            fEnabled = false;
                        if (!fEnabled)
                            CHECK_ERROR(UsbCtl, COMSETTER(Enabled)(true));
                    }
                }
                break;
            }

            case MODIFYVM_UARTMODE:
            {
                ComPtr<ISerialPort> uart;
                char *pszIRQ = NULL;

                CHECK_ERROR_BREAK(machine, GetSerialPort(GetOptState.uIndex - 1, uart.asOutParam()));
                ASSERT(uart);

                if (!strcmp(ValueUnion.psz, "disconnected"))
                {
                    CHECK_ERROR(uart, COMSETTER(HostMode)(PortMode_Disconnected));
                }
                else if (   !strcmp(ValueUnion.psz, "server")
                         || !strcmp(ValueUnion.psz, "client")
                         || !strcmp(ValueUnion.psz, "file"))
                {
                    const char *pszMode = ValueUnion.psz;

                    int vrc = RTGetOptFetchValue(&GetOptState, &ValueUnion, RTGETOPT_REQ_STRING);
                    if (RT_FAILURE(vrc))
                        return errorSyntax(USAGE_MODIFYVM,
                                           "Missing or Invalid argument to '%s'",
                                           GetOptState.pDef->pszLong);

                    CHECK_ERROR(uart, COMSETTER(Path)(Bstr(ValueUnion.psz).raw()));

                    if (!strcmp(pszMode, "server"))
                    {
                        CHECK_ERROR(uart, COMSETTER(HostMode)(PortMode_HostPipe));
                        CHECK_ERROR(uart, COMSETTER(Server)(TRUE));
                    }
                    else if (!strcmp(pszMode, "client"))
                    {
                        CHECK_ERROR(uart, COMSETTER(HostMode)(PortMode_HostPipe));
                        CHECK_ERROR(uart, COMSETTER(Server)(FALSE));
                    }
                    else if (!strcmp(pszMode, "file"))
                    {
                        CHECK_ERROR(uart, COMSETTER(HostMode)(PortMode_RawFile));
                    }
                }
                else
                {
                    CHECK_ERROR(uart, COMSETTER(Path)(Bstr(ValueUnion.psz).raw()));
                    CHECK_ERROR(uart, COMSETTER(HostMode)(PortMode_HostDevice));
                }
                break;
            }

            case MODIFYVM_UART:
            {
                ComPtr<ISerialPort> uart;

                CHECK_ERROR_BREAK(machine, GetSerialPort(GetOptState.uIndex - 1, uart.asOutParam()));
                ASSERT(uart);

                if (!strcmp(ValueUnion.psz, "off") || !strcmp(ValueUnion.psz, "disable"))
                    CHECK_ERROR(uart, COMSETTER(Enabled)(FALSE));
                else
                {
                    const char *pszIOBase = ValueUnion.psz;
                    uint32_t uVal = 0;

                    int vrc = RTGetOptFetchValue(&GetOptState, &ValueUnion, RTGETOPT_REQ_UINT32) != MODIFYVM_UART;
                    if (RT_FAILURE(vrc))
                        return errorSyntax(USAGE_MODIFYVM,
                                           "Missing or Invalid argument to '%s'",
                                           GetOptState.pDef->pszLong);

                    CHECK_ERROR(uart, COMSETTER(IRQ)(ValueUnion.u32));

                    vrc = RTStrToUInt32Ex(pszIOBase, NULL, 0, &uVal);
                    if (vrc != VINF_SUCCESS || uVal == 0)
                        return errorArgument("Error parsing UART I/O base '%s'", pszIOBase);
                    CHECK_ERROR(uart, COMSETTER(IOBase)(uVal));

                    CHECK_ERROR(uart, COMSETTER(Enabled)(TRUE));
                }
                break;
            }

            case MODIFYVM_GUESTMEMORYBALLOON:
            {
                CHECK_ERROR(machine, COMSETTER(MemoryBalloonSize)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_AUDIOCONTROLLER:
            {
                ComPtr<IAudioAdapter> audioAdapter;
                machine->COMGETTER(AudioAdapter)(audioAdapter.asOutParam());
                ASSERT(audioAdapter);

                if (!strcmp(ValueUnion.psz, "sb16"))
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioController)(AudioControllerType_SB16));
                else if (!strcmp(ValueUnion.psz, "ac97"))
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioController)(AudioControllerType_AC97));
                else if (!strcmp(ValueUnion.psz, "hda"))
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioController)(AudioControllerType_HDA));
                else
                {
                    errorArgument("Invalid --audiocontroller argument '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_AUDIO:
            {
                ComPtr<IAudioAdapter> audioAdapter;
                machine->COMGETTER(AudioAdapter)(audioAdapter.asOutParam());
                ASSERT(audioAdapter);

                /* disable? */
                if (!strcmp(ValueUnion.psz, "none"))
                {
                    CHECK_ERROR(audioAdapter, COMSETTER(Enabled)(false));
                }
                else if (!strcmp(ValueUnion.psz, "null"))
                {
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioDriver)(AudioDriverType_Null));
                    CHECK_ERROR(audioAdapter, COMSETTER(Enabled)(true));
                }
#ifdef RT_OS_WINDOWS
#ifdef VBOX_WITH_WINMM
                else if (!strcmp(ValueUnion.psz, "winmm"))
                {
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioDriver)(AudioDriverType_WinMM));
                    CHECK_ERROR(audioAdapter, COMSETTER(Enabled)(true));
                }
#endif
                else if (!strcmp(ValueUnion.psz, "dsound"))
                {
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioDriver)(AudioDriverType_DirectSound));
                    CHECK_ERROR(audioAdapter, COMSETTER(Enabled)(true));
                }
#endif /* RT_OS_WINDOWS */
#ifdef RT_OS_LINUX
# ifdef VBOX_WITH_ALSA
                else if (!strcmp(ValueUnion.psz, "alsa"))
                {
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioDriver)(AudioDriverType_ALSA));
                    CHECK_ERROR(audioAdapter, COMSETTER(Enabled)(true));
                }
# endif
# ifdef VBOX_WITH_PULSE
                else if (!strcmp(ValueUnion.psz, "pulse"))
                {
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioDriver)(AudioDriverType_Pulse));
                    CHECK_ERROR(audioAdapter, COMSETTER(Enabled)(true));
                }
# endif
#endif /* !RT_OS_LINUX */
#ifdef RT_OS_SOLARIS
                else if (!strcmp(ValueUnion.psz, "solaudio"))
                {
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioDriver)(AudioDriverType_SolAudio));
                    CHECK_ERROR(audioAdapter, COMSETTER(Enabled)(true));
                }
#endif /* !RT_OS_SOLARIS */
#ifdef RT_OS_FREEBSD
                else if (!strcmp(ValueUnion.psz, "oss"))
                {
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioDriver)(AudioDriverType_OSS));
                    CHECK_ERROR(audioAdapter, COMSETTER(Enabled)(true));
                }
# ifdef VBOX_WITH_PULSE
                else if (!strcmp(ValueUnion.psz, "pulse"))
                {
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioDriver)(AudioDriverType_Pulse));
                    CHECK_ERROR(audioAdapter, COMSETTER(Enabled)(true));
                }
# endif
#endif /* !RT_OS_FREEBSD */
#ifdef RT_OS_DARWIN
                else if (!strcmp(ValueUnion.psz, "coreaudio"))
                {
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioDriver)(AudioDriverType_CoreAudio));
                    CHECK_ERROR(audioAdapter, COMSETTER(Enabled)(true));
                }

#endif /* !RT_OS_DARWIN */
# if defined(RT_OS_FREEBSD) || defined(RT_OS_LINUX) || defined(VBOX_WITH_SOLARIS_OSS)
                else if (!strcmp(ValueUnion.psz, "oss"))
                {
                    CHECK_ERROR(audioAdapter, COMSETTER(AudioDriver)(AudioDriverType_OSS));
                    CHECK_ERROR(audioAdapter, COMSETTER(Enabled)(true));
                }
# endif
                else
                {
                    errorArgument("Invalid --audio argument '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_CLIPBOARD:
            {
                if (!strcmp(ValueUnion.psz, "disabled"))
                {
                    CHECK_ERROR(machine, COMSETTER(ClipboardMode)(ClipboardMode_Disabled));
                }
                else if (!strcmp(ValueUnion.psz, "hosttoguest"))
                {
                    CHECK_ERROR(machine, COMSETTER(ClipboardMode)(ClipboardMode_HostToGuest));
                }
                else if (!strcmp(ValueUnion.psz, "guesttohost"))
                {
                    CHECK_ERROR(machine, COMSETTER(ClipboardMode)(ClipboardMode_GuestToHost));
                }
                else if (!strcmp(ValueUnion.psz, "bidirectional"))
                {
                    CHECK_ERROR(machine, COMSETTER(ClipboardMode)(ClipboardMode_Bidirectional));
                }
                else
                {
                    errorArgument("Invalid --clipboard argument '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_VRDE_EXTPACK:
            {
                ComPtr<IVRDEServer> vrdeServer;
                machine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                if (vrdeServer)
                {
                    if (strcmp(ValueUnion.psz, "default") != 0)
                    {
                        Bstr bstr(ValueUnion.psz);
                        CHECK_ERROR(vrdeServer, COMSETTER(VRDEExtPack)(bstr.raw()));
                    }
                    else
                        CHECK_ERROR(vrdeServer, COMSETTER(VRDEExtPack)(NULL));
                }
                break;
            }

            case MODIFYVM_VRDEPROPERTY:
            {
                ComPtr<IVRDEServer> vrdeServer;
                machine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                if (vrdeServer)
                {
                    /* Parse 'name=value' */
                    char *pszProperty = RTStrDup(ValueUnion.psz);
                    if (pszProperty)
                    {
                        char *pDelimiter = strchr(pszProperty, '=');
                        if (pDelimiter)
                        {
                            *pDelimiter = '\0';

                            Bstr bstrName = pszProperty;
                            Bstr bstrValue = &pDelimiter[1];
                            CHECK_ERROR(vrdeServer, SetVRDEProperty(bstrName.raw(), bstrValue.raw()));
                        }
                        else
                        {
                            RTStrFree(pszProperty);

                            errorArgument("Invalid --vrdeproperty argument '%s'", ValueUnion.psz);
                            rc = E_FAIL;
                            break;
                        }
                        RTStrFree(pszProperty);
                    }
                    else
                    {
                        RTStrmPrintf(g_pStdErr, "Error: Failed to allocate memory for VRDE property '%s'\n", ValueUnion.psz);
                        rc = E_FAIL;
                    }
                }
                break;
            }

            case MODIFYVM_VRDPPORT:
                vrdeWarningDeprecatedOption("port");

            case MODIFYVM_VRDEPORT:
            {
                ComPtr<IVRDEServer> vrdeServer;
                machine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                if (!strcmp(ValueUnion.psz, "default"))
                    CHECK_ERROR(vrdeServer, SetVRDEProperty(Bstr("TCP/Ports").raw(), Bstr("0").raw()));
                else
                    CHECK_ERROR(vrdeServer, SetVRDEProperty(Bstr("TCP/Ports").raw(), Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_VRDPADDRESS:
                vrdeWarningDeprecatedOption("address");

            case MODIFYVM_VRDEADDRESS:
            {
                ComPtr<IVRDEServer> vrdeServer;
                machine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                CHECK_ERROR(vrdeServer, SetVRDEProperty(Bstr("TCP/Address").raw(), Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_VRDPAUTHTYPE:
                vrdeWarningDeprecatedOption("authtype");
            case MODIFYVM_VRDEAUTHTYPE:
            {
                ComPtr<IVRDEServer> vrdeServer;
                machine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                if (!strcmp(ValueUnion.psz, "null"))
                {
                    CHECK_ERROR(vrdeServer, COMSETTER(AuthType)(AuthType_Null));
                }
                else if (!strcmp(ValueUnion.psz, "external"))
                {
                    CHECK_ERROR(vrdeServer, COMSETTER(AuthType)(AuthType_External));
                }
                else if (!strcmp(ValueUnion.psz, "guest"))
                {
                    CHECK_ERROR(vrdeServer, COMSETTER(AuthType)(AuthType_Guest));
                }
                else
                {
                    errorArgument("Invalid --vrdeauthtype argument '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_VRDEAUTHLIBRARY:
            {
                ComPtr<IVRDEServer> vrdeServer;
                machine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                if (vrdeServer)
                {
                    if (strcmp(ValueUnion.psz, "default") != 0)
                    {
                        Bstr bstr(ValueUnion.psz);
                        CHECK_ERROR(vrdeServer, COMSETTER(AuthLibrary)(bstr.raw()));
                    }
                    else
                        CHECK_ERROR(vrdeServer, COMSETTER(AuthLibrary)(NULL));
                }
                break;
            }

            case MODIFYVM_VRDPMULTICON:
                vrdeWarningDeprecatedOption("multicon");
            case MODIFYVM_VRDEMULTICON:
            {
                ComPtr<IVRDEServer> vrdeServer;
                machine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                CHECK_ERROR(vrdeServer, COMSETTER(AllowMultiConnection)(ValueUnion.f));
                break;
            }

            case MODIFYVM_VRDPREUSECON:
                vrdeWarningDeprecatedOption("reusecon");
            case MODIFYVM_VRDEREUSECON:
            {
                ComPtr<IVRDEServer> vrdeServer;
                machine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                CHECK_ERROR(vrdeServer, COMSETTER(ReuseSingleConnection)(ValueUnion.f));
                break;
            }

            case MODIFYVM_VRDPVIDEOCHANNEL:
                vrdeWarningDeprecatedOption("videochannel");
            case MODIFYVM_VRDEVIDEOCHANNEL:
            {
                ComPtr<IVRDEServer> vrdeServer;
                machine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                CHECK_ERROR(vrdeServer, SetVRDEProperty(Bstr("VideoChannel/Enabled").raw(),
                                                        ValueUnion.f? Bstr("true").raw():  Bstr("false").raw()));
                break;
            }

            case MODIFYVM_VRDPVIDEOCHANNELQUALITY:
                vrdeWarningDeprecatedOption("videochannelquality");
            case MODIFYVM_VRDEVIDEOCHANNELQUALITY:
            {
                ComPtr<IVRDEServer> vrdeServer;
                machine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                CHECK_ERROR(vrdeServer, SetVRDEProperty(Bstr("VideoChannel/Quality").raw(),
                                                        Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_VRDP:
                vrdeWarningDeprecatedOption("");
            case MODIFYVM_VRDE:
            {
                ComPtr<IVRDEServer> vrdeServer;
                machine->COMGETTER(VRDEServer)(vrdeServer.asOutParam());
                ASSERT(vrdeServer);

                CHECK_ERROR(vrdeServer, COMSETTER(Enabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_USBEHCI:
            {
                ComPtr<IUSBController> UsbCtl;
                CHECK_ERROR(machine, COMGETTER(USBController)(UsbCtl.asOutParam()));
                if (SUCCEEDED(rc))
                    CHECK_ERROR(UsbCtl, COMSETTER(EnabledEhci)(ValueUnion.f));
                break;
            }

            case MODIFYVM_USB:
            {
                ComPtr<IUSBController> UsbCtl;
                CHECK_ERROR(machine, COMGETTER(USBController)(UsbCtl.asOutParam()));
                if (SUCCEEDED(rc))
                    CHECK_ERROR(UsbCtl, COMSETTER(Enabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_SNAPSHOTFOLDER:
            {
                if (!strcmp(ValueUnion.psz, "default"))
                    CHECK_ERROR(machine, COMSETTER(SnapshotFolder)(NULL));
                else
                    CHECK_ERROR(machine, COMSETTER(SnapshotFolder)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_TELEPORTER_ENABLED:
            {
                CHECK_ERROR(machine, COMSETTER(TeleporterEnabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_TELEPORTER_PORT:
            {
                CHECK_ERROR(machine, COMSETTER(TeleporterPort)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_TELEPORTER_ADDRESS:
            {
                CHECK_ERROR(machine, COMSETTER(TeleporterAddress)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_TELEPORTER_PASSWORD:
            {
                CHECK_ERROR(machine, COMSETTER(TeleporterPassword)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_FAULT_TOLERANCE:
            {
                if (!strcmp(ValueUnion.psz, "master"))
                {
                    CHECK_ERROR(machine, COMSETTER(FaultToleranceState(FaultToleranceState_Master)));
                }
                else
                if (!strcmp(ValueUnion.psz, "standby"))
                {
                    CHECK_ERROR(machine, COMSETTER(FaultToleranceState(FaultToleranceState_Standby)));
                }
                else
                {
                    errorArgument("Invalid --faulttolerance argument '%s'", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            case MODIFYVM_FAULT_TOLERANCE_ADDRESS:
            {
                CHECK_ERROR(machine, COMSETTER(FaultToleranceAddress)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_FAULT_TOLERANCE_PORT:
            {
                CHECK_ERROR(machine, COMSETTER(FaultTolerancePort)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_FAULT_TOLERANCE_PASSWORD:
            {
                CHECK_ERROR(machine, COMSETTER(FaultTolerancePassword)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_FAULT_TOLERANCE_SYNC_INTERVAL:
            {
                CHECK_ERROR(machine, COMSETTER(FaultToleranceSyncInterval)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_HARDWARE_UUID:
            {
                CHECK_ERROR(machine, COMSETTER(HardwareUUID)(Bstr(ValueUnion.psz).raw()));
                break;
            }

            case MODIFYVM_HPET:
            {
                CHECK_ERROR(machine, COMSETTER(HpetEnabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_IOCACHE:
            {
                CHECK_ERROR(machine, COMSETTER(IoCacheEnabled)(ValueUnion.f));
                break;
            }

            case MODIFYVM_IOCACHESIZE:
            {
                CHECK_ERROR(machine, COMSETTER(IoCacheSize)(ValueUnion.u32));
                break;
            }

            case MODIFYVM_CHIPSET:
            {
                if (!strcmp(ValueUnion.psz, "piix3"))
                {
                    CHECK_ERROR(machine, COMSETTER(ChipsetType)(ChipsetType_PIIX3));
                }
                else if (!strcmp(ValueUnion.psz, "ich9"))
                {
                    CHECK_ERROR(machine, COMSETTER(ChipsetType)(ChipsetType_ICH9));
                    BOOL fIoApic = FALSE;
                    CHECK_ERROR(biosSettings, COMGETTER(IOAPICEnabled)(&fIoApic));
                    if (!fIoApic)
                    {
                        RTStrmPrintf(g_pStdErr, "*** I/O APIC must be enabled for ICH9, enabling. ***\n");
                        CHECK_ERROR(biosSettings, COMSETTER(IOAPICEnabled)(TRUE));
                    }
                }
                else
                {
                    errorArgument("Invalid --chipset argument '%s' (valid: piix3,ich9)", ValueUnion.psz);
                    rc = E_FAIL;
                }
                break;
            }

            default:
            {
                errorGetOpt(USAGE_MODIFYVM, c, &ValueUnion);
                rc = E_FAIL;
                break;
            }
        }
    }

    /* commit changes */
    if (SUCCEEDED(rc))
        CHECK_ERROR(machine, SaveSettings());

    /* it's important to always close sessions */
    a->session->UnlockMachine();

    return SUCCEEDED(rc) ? 0 : 1;
}

#endif /* !VBOX_ONLY_DOCS */
