/** @file
 *
 * VirtualBox COM class implementation extra definitions
 *
 * This file pulls in generated entities that may be rather big but rarely
 * changed. Separating them from VirtualBoxImpl.cpp should speed up
 * compilation a bit.
 */

/*
 * Copyright (C) 2006-2007 innotek GmbH
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation,
 * in version 2 as it comes in the "COPYING" file of the VirtualBox OSE
 * distribution. VirtualBox OSE is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "VirtualBoxImpl.h"

#include "VirtualBoxXMLUtil.h"

/* embedded XML Schema documents for validating XML settings files */
#include "xml_VirtualBox_settings_xsd.h"
#include "xml_VirtualBox_settings_common_xsd.h"

/* embedded settings converter template for updating settings files */
#include "xml_SettingsConverter_xsl.h"

/**
 * Resolves external entities while parting and validating XML settings files.
 *
 * @param aURI  URI of the external entity.
 * @param aID   ID of the external entity (may be NULL).
 *
 * @return      Input stream created using @c new or NULL to indicate
 *              a wrong URI/ID pair.
 */
settings::Input *
VirtualBox::SettingsTreeHelper::resolveEntity (const char *aURI, const char *aID)
{
    if (strcmp (aURI, VBOX_XML_SCHEMA_COMMON) == 0)
    {
        return new settings::
            MemoryBuf ((const char *) g_ab_xml_VirtualBox_settings_common_xsd,
                       g_cb_xml_VirtualBox_settings_common_xsd, aURI);
    }

    if (strcmp (aURI, VBOX_XML_SCHEMA) == 0)
    {
        return new settings::
            MemoryBuf ((const char *) g_ab_xml_VirtualBox_settings_xsd,
                       g_cb_xml_VirtualBox_settings_xsd, aURI);
    }

    if (strcmp (aURI, VBOX_XML_SETTINGS_CONVERTER) == 0)
    {
        return new settings::
            MemoryBuf ((const char *) g_ab_xml_SettingsConverter_xsl,
                       g_cb_xml_SettingsConverter_xsl, aURI);
    }

    AssertMsgFailed (("Unexpected entity: '%s' - knows: '%s' and '%s'\n", aURI,
                      VBOX_XML_SCHEMA_COMMON, VBOX_XML_SCHEMA));
    return NULL;
}

/**
 * Returns @true if the given tree needs to be converted using the XSLT
 * template identified by #templateUri(), or @false if no conversion is
 * required.
 *
 * The implementation normally checks for the "version" value of the
 * root key to determine if the conversion is necessary. The
 * implementation must return a string representing the old version
 * (before conversion) in the @c aOldVersion argument -- this string is
 * used by XmlTreeBackend::oldVersion() and must be non-NULL to indicate
 * that the conversion has been performed on the tree. The returned
 * string must be allocated using RTStrDup or such.
 *
 * @param aRoot                 Root settings key.
 * @param aOldVersionString     Old version string (allocated by
 *                              RTStrDup or such).
 */
bool VirtualBox::SettingsTreeHelper::
needsConversion (const settings::Key &aRoot, char *&aOldVersion) const
{
    if (strcmp (aRoot.name(), "VirtualBox") == 0)
    {
        const char *version = aRoot.stringValue ("version");
        const char *dash = strchr (version, '-');
        if (dash != NULL && strcmp (dash + 1, VBOX_XML_PLATFORM) == 0)
        {
            if (strcmp (version, VBOX_XML_VERSION_FULL) != 0)
            {
                /* version mismatch */
                aOldVersion = RTStrDup (version);
                return true;
            }
        }
    }

    /* either the tree is invalid, or it's the other platform, or it's the same
     * version */
    return false;
}

/**
 * Returns the URI of the XSLT template to perform the conversion.
 * This template will be applied to the tree if #needsConversion()
 * returns @c true for this tree.
 */
const char *VirtualBox::SettingsTreeHelper::templateUri() const
{
    return VBOX_XML_SETTINGS_CONVERTER;
}

