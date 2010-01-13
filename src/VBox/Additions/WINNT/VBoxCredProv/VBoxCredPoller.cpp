/* $Id$ */
/** @file
 * VBoxCredPoller - Thread for retrieving user credentials.
 */

/*
 * Copyright (C) 2009 Sun Microsystems, Inc.
 *
 * Sun Microsystems, Inc. confidential
 * All rights reserved
 */


#include <windows.h>

#include <VBox/VBoxGuest.h>
#include <VBox/VBoxGuestLib.h>
#include <VBox/VMMDev.h>
#include <iprt/string.h>

#include "dll.h"
#include "VBoxCredProv.h"
#include "VBoxCredential.h"
#include "VBoxCredPoller.h"


VBoxCredPoller::VBoxCredPoller()
{
    m_hThreadPoller = NIL_RTTHREAD;
    m_pProv = NULL;

    m_pszUser = NULL;
    m_pszPw = NULL;
    m_pszDomain = NULL;
}


VBoxCredPoller::~VBoxCredPoller()
{
    Shutdown();
}


bool VBoxCredPoller::Initialize(VBoxCredProv *pProvider)
{
    Log(("VBoxCredPoller::Initialize\n"));

    if (m_pProv != NULL)
        m_pProv->Release();
    m_pProv = pProvider;
    Assert(m_pProv);
    m_pProv->AddRef();

    /* don't create more than one of them */
    if (m_hThreadPoller != NIL_RTTHREAD)
    {
        Log(("VBoxCredPoller::Initialize: Thread already running, returning!\n"));
        return false;
    }

    int rc = RTCritSectInit(&m_csCredUpate);
    if (RT_FAILURE(rc))
        Log(("VBoxCredPoller: Could not init critical section! rc = %Rrc\n", rc));

    /* create the poller thread */
    rc = RTThreadCreate(&m_hThreadPoller, VBoxCredPoller::threadPoller, this, 0, RTTHREADTYPE_INFREQUENT_POLLER,
                        RTTHREADFLAGS_WAITABLE, "creds");
    if (RT_FAILURE(rc))
    {
        Log(("VBoxCredPoller::Initialize: Failed to create thread, rc = %Rrc\n", rc));
        return false;
    }
    return true;
}


bool VBoxCredPoller::Shutdown(void)
{
    Log(("VBoxCredPoller::Shutdown\n"));

    if (m_hThreadPoller == NIL_RTTHREAD)
    {
        Log(("VBoxCredPoller::Shutdown: Either thread or exit sem is NULL!\n"));
        return false;
    }

    /* Post termination event semaphore */
    int rc = RTThreadUserSignal(m_hThreadPoller);
    if (RT_SUCCESS(rc))
    {
        Log(("VBoxCredPoller::Shutdown: Waiting for thread to terminate\n"));
        /* wait until the thread has terminated */
        rc = RTThreadWait(m_hThreadPoller, RT_INDEFINITE_WAIT, NULL);
        Log(("VBoxCredPoller::Shutdown: Thread has (probably) terminated (rc = %Rrc)\n", rc));
    }
    else
    {
        /* failed to signal the thread - very unlikely - so no point in waiting long. */
        Log(("VBoxCredPoller::Shutdown: Failed to signal semaphore, rc = %Rrc\n", rc));
        rc = RTThreadWait(m_hThreadPoller, 100, NULL);
        Log(("VBoxCredPoller::Shutdown: Thread has terminated? wait rc = %Rrc\n", rc));
    }

    if (m_pProv != NULL)
    {
        m_pProv->Release();
        m_pProv = NULL;
    }

    credentialsReset();
    RTCritSectDelete(&m_csCredUpate);

    m_hThreadPoller = NIL_RTTHREAD;
    return true;
}


int VBoxCredPoller::credentialsRetrieve(void)
{
    credentialsReset();

    /* get credentials */
    RTCritSectEnter(&m_csCredUpate);
    int rc = VbglR3CredentialsRetrieve(&m_pszUser, &m_pszPw, &m_pszDomain);
    if (RT_SUCCESS(rc))
    {
        Log(("VBoxCredPoller::credentialsRetrieve: Credentials retrieved (user=%s, pw=%s, domain=%s)\n",
             m_pszUser ? m_pszUser : "<empty>",
             m_pszPw ? m_pszPw : "<empty>",
             m_pszDomain ? m_pszDomain : "<empty>"));

        /* allocated but empty? delete and re-fill with default value in block below. */
        if (strlen(m_pszDomain) == 0)
        {
            RTStrFree(m_pszDomain);
            m_pszDomain = NULL;
        }

        /* if we don't have a domain specified, fill in a dot (".") specifying the
         * local computer. */
        if (m_pszDomain == NULL)
        {
            rc = RTStrAPrintf(&m_pszDomain, ".");
            if (RT_FAILURE(rc))
                Log(("VBoxCredPoller::credentialsRetrieve: Could not set default domain name, rc = %Rrc", rc));
            else
                Log(("VBoxCredPoller::credentialsRetrieve: No domain name given, set default value to: %s\n", m_pszDomain));
        }
    }

    /* if all went fine, notify parent */
    if (RT_SUCCESS(rc))
    {
        Assert(m_pProv);
        m_pProv->OnCredentialsProvided(m_pszUser,
                                       m_pszPw,
                                       m_pszDomain);
    }
    RTCritSectLeave(&m_csCredUpate);
    return rc;
}


bool VBoxCredPoller::QueryCredentials(VBoxCredential *pCred)
{
    Assert (pCred);
    RTCritSectEnter(&m_csCredUpate);
    pCred->Update(m_pszUser, m_pszPw, m_pszDomain);
    RTCritSectLeave(&m_csCredUpate);

    bool bRet = (   (m_pszUser && strlen(m_pszUser))
                 || (m_pszPw && strlen(m_pszPw))
                 || (m_pszDomain && strlen(m_pszDomain)));
    credentialsReset();
    return bRet;
}


void VBoxCredPoller::credentialsReset(void)
{
    RTCritSectEnter(&m_csCredUpate);

    if (m_pszUser)
        SecureZeroMemory(m_pszUser, strlen(m_pszUser) * sizeof(char));
    if (m_pszPw)
        SecureZeroMemory(m_pszPw, strlen(m_pszPw) * sizeof(char));
    if (m_pszDomain)
        SecureZeroMemory(m_pszDomain, strlen(m_pszDomain) * sizeof(char));

    RTStrFree(m_pszUser);
    m_pszUser = NULL;
    RTStrFree(m_pszPw);
    m_pszPw = NULL;
    RTStrFree(m_pszDomain);
    m_pszDomain = NULL;

    RTCritSectLeave(&m_csCredUpate);
}


DECLCALLBACK(int) VBoxCredPoller::threadPoller(RTTHREAD ThreadSelf, void *pvUser)
{
    Log(("VBoxCredPoller::threadPoller\n"));

    VBoxCredPoller *pThis = (VBoxCredPoller *)pvUser;

    do
    {
        int rc;
        if (VbglR3CredentialsAreAvailable())
        {
            Log(("VBoxCredPoller::threadPoller: Credentials available.\n"));
            Assert(pThis);
            rc = pThis->credentialsRetrieve();
        }

        /* wait a bit */
        if (RTThreadUserWait(ThreadSelf, 500) == VINF_SUCCESS)
        {
            Log(("VBoxCredPoller::threadPoller: Terminating\n"));
            /* we were asked to terminate, do that instantly! */
            return 0;
        }
    }
    while (1);

    return 0;
}

