/* $Id$ */
/** @file
 * VBox Debugger GUI - Console.
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
#define LOG_GROUP LOG_GROUP_DBGG
#include "VBoxDbgConsole.h"

#include <QLabel>
#include <QApplication>
#include <QFont>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QAction>
#include <QContextMenuEvent>

#include <VBox/dbg.h>
#include <VBox/vmm/cfgm.h>
#include <VBox/err.h>

#include <iprt/thread.h>
#include <iprt/tcp.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/alloc.h>
#include <iprt/string.h>




/*
 *
 *          V B o x D b g C o n s o l e O u t p u t
 *          V B o x D b g C o n s o l e O u t p u t
 *          V B o x D b g C o n s o l e O u t p u t
 *
 *
 */


VBoxDbgConsoleOutput::VBoxDbgConsoleOutput(QWidget *pParent/* = NULL*/, const char *pszName/* = NULL*/)
    : QTextEdit(pParent), m_uCurLine(0), m_uCurPos(0), m_hGUIThread(RTThreadNativeSelf())
{
    setReadOnly(true);
    setUndoRedoEnabled(false);
    setOverwriteMode(false);
    setPlainText("");
    setTextInteractionFlags(Qt::TextBrowserInteraction);
    setAutoFormatting(QTextEdit::AutoAll);
    setTabChangesFocus(true);
    setAcceptRichText(false);

#ifdef Q_WS_MAC
    QFont Font("Monaco", 10, QFont::Normal, FALSE);
    Font.setStyleStrategy(QFont::NoAntialias);
#else
    QFont Font = font();
    Font.setStyleHint(QFont::TypeWriter);
    Font.setFamily("Courier [Monotype]");
#endif
    setFont(Font);

    /* green on black */
    QPalette Pal(palette());
    Pal.setColor(QPalette::All, QPalette::Base, QColor(Qt::black));
    setPalette(Pal);
    setTextColor(QColor(qRgb(0, 0xe0, 0)));
    NOREF(pszName);
}


VBoxDbgConsoleOutput::~VBoxDbgConsoleOutput()
{
    Assert(m_hGUIThread == RTThreadNativeSelf());
}


void
VBoxDbgConsoleOutput::appendText(const QString &rStr)
{
    Assert(m_hGUIThread == RTThreadNativeSelf());

    if (rStr.isEmpty() || rStr.isNull() || !rStr.length())
        return;

    /*
     * Insert all in one go and make sure it's visible.
     */
    QTextCursor Cursor = textCursor();
    if (!Cursor.atEnd())
        moveCursor(QTextCursor::End); /* make sure we append the text */
    Cursor.insertText(rStr);
    ensureCursorVisible();
}




/*
 *
 *      V B o x D b g C o n s o l e I n p u t
 *      V B o x D b g C o n s o l e I n p u t
 *      V B o x D b g C o n s o l e I n p u t
 *
 *
 */


VBoxDbgConsoleInput::VBoxDbgConsoleInput(QWidget *pParent/* = NULL*/, const char *pszName/* = NULL*/)
    : QComboBox(pParent), m_iBlankItem(0), m_hGUIThread(RTThreadNativeSelf())
{
    insertItem(m_iBlankItem, "");
    setEditable(true);
    setInsertPolicy(NoInsert);
    setAutoCompletion(false);
    setMaxCount(50);
    const QLineEdit *pEdit = lineEdit();
    if (pEdit)
        connect(pEdit, SIGNAL(returnPressed()), this, SLOT(returnPressed()));

    NOREF(pszName);
}


VBoxDbgConsoleInput::~VBoxDbgConsoleInput()
{
    Assert(m_hGUIThread == RTThreadNativeSelf());
}


void
VBoxDbgConsoleInput::setLineEdit(QLineEdit *pEdit)
{
    Assert(m_hGUIThread == RTThreadNativeSelf());
    QComboBox::setLineEdit(pEdit);
    if (lineEdit() == pEdit && pEdit)
        connect(pEdit, SIGNAL(returnPressed()), this, SLOT(returnPressed()));
}


void
VBoxDbgConsoleInput::returnPressed()
{
    Assert(m_hGUIThread == RTThreadNativeSelf());
    /* deal with the current command. */
    QString Str = currentText();
    emit commandSubmitted(Str);

    /* update the history and clear the entry field */
    QString PrevStr = m_iBlankItem > 0 ? itemText(m_iBlankItem - 1) : "";
    if (PrevStr != Str)
    {
        setItemText(m_iBlankItem, Str);
        if (    m_iBlankItem > 0
            &&  m_iBlankItem >= maxCount() - 1)
            removeItem(m_iBlankItem - maxCount() - 1);
        insertItem(++m_iBlankItem, "");
    }

    clearEditText();
    setCurrentIndex(m_iBlankItem);
}






/*
 *
 *      V B o x D b g C o n s o l e
 *      V B o x D b g C o n s o l e
 *      V B o x D b g C o n s o l e
 *
 *
 */


VBoxDbgConsole::VBoxDbgConsole(VBoxDbgGui *a_pDbgGui, QWidget *a_pParent/* = NULL*/)
    : VBoxDbgBaseWindow(a_pDbgGui, a_pParent), m_pOutput(NULL), m_pInput(NULL), m_fInputRestoreFocus(false),
    m_pszInputBuf(NULL), m_cbInputBuf(0), m_cbInputBufAlloc(0),
    m_pszOutputBuf(NULL), m_cbOutputBuf(0), m_cbOutputBufAlloc(0),
    m_pTimer(NULL), m_fUpdatePending(false), m_Thread(NIL_RTTHREAD), m_EventSem(NIL_RTSEMEVENT),
    m_fTerminate(false), m_fThreadTerminated(false)
{
    setWindowTitle("VBoxDbg - Console");

    /*
     * Create the output text box.
     */
    m_pOutput = new VBoxDbgConsoleOutput(this);

    /* try figure a suitable size */
    QLabel *pLabel = new QLabel(      "11111111111111111111111111111111111111111111111111111111111111111111111111111112222222222", this);
    pLabel->setFont(m_pOutput->font());
    QSize Size = pLabel->sizeHint();
    delete pLabel;
    Size.setWidth((int)(Size.width() * 1.10));
    Size.setHeight(Size.width() / 2);
    resize(Size);

    /*
     * Create the input combo box (with a label).
     */
    QHBoxLayout *pLayout = new QHBoxLayout();
    //pLayout->setSizeConstraint(QLayout::SetMaximumSize);

    pLabel = new QLabel(" Command ");
    pLayout->addWidget(pLabel);
    pLabel->setMaximumSize(pLabel->sizeHint());
    pLabel->setAlignment(Qt::AlignCenter);

    m_pInput = new VBoxDbgConsoleInput(NULL);
    pLayout->addWidget(m_pInput);
    m_pInput->setDuplicatesEnabled(false);
    connect(m_pInput, SIGNAL(commandSubmitted(const QString &)), this, SLOT(commandSubmitted(const QString &)));

# if 0//def Q_WS_MAC
    pLabel = new QLabel("  ");
    pLayout->addWidget(pLabel);
    pLabel->setMaximumSize(20, m_pInput->sizeHint().height() + 6);
    pLabel->setMinimumSize(20, m_pInput->sizeHint().height() + 6);
# endif

    QWidget *pHBox = new QWidget(this);
    pHBox->setLayout(pLayout);

    m_pInput->setEnabled(false);    /* (we'll get a ready notification) */


    /*
     * Vertical layout box on the whole widget.
     */
    QVBoxLayout *pVLayout = new QVBoxLayout();
    pVLayout->setContentsMargins(0, 0, 0, 0);
    pVLayout->setSpacing(5);
    pVLayout->addWidget(m_pOutput);
    pVLayout->addWidget(pHBox);
    setLayout(pVLayout);

    /*
     * The tab order is from input to output, not the other way around as it is by default.
     */
    setTabOrder(m_pInput, m_pOutput);

    /*
     * Setup the timer.
     */
    m_pTimer = new QTimer(this);
    connect(m_pTimer, SIGNAL(timeout()), SLOT(updateOutput()));

    /*
     * Init the backend structure.
     */
    m_Back.Core.pfnInput   = backInput;
    m_Back.Core.pfnRead    = backRead;
    m_Back.Core.pfnWrite   = backWrite;
    m_Back.Core.pfnSetReady = backSetReady;
    m_Back.pSelf = this;

    /*
     * Create the critical section, the event semaphore and the debug console thread.
     */
    int rc = RTCritSectInit(&m_Lock);
    AssertRC(rc);

    rc = RTSemEventCreate(&m_EventSem);
    AssertRC(rc);

    rc = RTThreadCreate(&m_Thread, backThread, this, 0, RTTHREADTYPE_DEBUGGER, RTTHREADFLAGS_WAITABLE, "VBoxDbgC");
    AssertRC(rc);
    if (RT_FAILURE(rc))
        m_Thread = NIL_RTTHREAD;

    /*
     * Shortcuts.
     */
    m_pFocusToInput = new QAction("", this);
    m_pFocusToInput->setShortcut(QKeySequence("Ctrl+L"));
    addAction(m_pFocusToInput);
    connect(m_pFocusToInput, SIGNAL(triggered(bool)), this, SLOT(actFocusToInput()));

    m_pFocusToOutput = new QAction("", this);
    m_pFocusToOutput->setShortcut(QKeySequence("Ctrl+O"));
    addAction(m_pFocusToOutput);
    connect(m_pFocusToOutput, SIGNAL(triggered(bool)), this, SLOT(actFocusToOutput()));
}


VBoxDbgConsole::~VBoxDbgConsole()
{
    Assert(isGUIThread());

    /*
     * Wait for the thread.
     */
    ASMAtomicWriteBool(&m_fTerminate, true);
    RTSemEventSignal(m_EventSem);
    if (m_Thread != NIL_RTTHREAD)
    {
        int rc = RTThreadWait(m_Thread, 15000, NULL);
        AssertRC(rc);
        m_Thread = NIL_RTTHREAD;
    }

    /*
     * Free resources.
     */
    delete m_pTimer;
    m_pTimer = NULL;
    RTCritSectDelete(&m_Lock);
    RTSemEventDestroy(m_EventSem);
    m_EventSem = 0;
    m_pOutput = NULL;
    m_pInput = NULL;
    if (m_pszInputBuf)
    {
        RTMemFree(m_pszInputBuf);
        m_pszInputBuf = NULL;
    }
    m_cbInputBuf = 0;
    m_cbInputBufAlloc = 0;

    delete m_pFocusToInput;
    m_pFocusToInput = NULL;
    delete m_pFocusToOutput;
    m_pFocusToOutput = NULL;
}


void
VBoxDbgConsole::commandSubmitted(const QString &rCommand)
{
    Assert(isGUIThread());

    lock();
    RTSemEventSignal(m_EventSem);

    QByteArray Utf8Array = rCommand.toUtf8();
    const char *psz = Utf8Array.constData();
    size_t cb = strlen(psz);

    /*
     * Make sure we've got space for the input.
     */
    if (cb + m_cbInputBuf >= m_cbInputBufAlloc)
    {
        size_t cbNew = RT_ALIGN_Z(cb + m_cbInputBufAlloc + 1, 128);
        void *pv = RTMemRealloc(m_pszInputBuf, cbNew);
        if (!pv)
        {
            unlock();
            return;
        }
        m_pszInputBuf = (char *)pv;
        m_cbInputBufAlloc = cbNew;
    }

    /*
     * Add the input and output it.
     */
    memcpy(m_pszInputBuf + m_cbInputBuf, psz, cb);
    m_cbInputBuf += cb;
    m_pszInputBuf[m_cbInputBuf++] = '\n';

    m_pOutput->appendText(rCommand + "\n");
    m_pOutput->ensureCursorVisible();

    m_fInputRestoreFocus = m_pInput->hasFocus();    /* dirty focus hack */
    m_pInput->setEnabled(false);

    Log(("VBoxDbgConsole::commandSubmitted: %s (input-enabled=%RTbool)\n", psz, m_pInput->isEnabled()));
    unlock();
}


void
VBoxDbgConsole::updateOutput()
{
    Assert(isGUIThread());

    lock();
    m_fUpdatePending = false;
    if (m_cbOutputBuf)
    {
        m_pOutput->appendText(QString::fromUtf8((const char *)m_pszOutputBuf, (int)m_cbOutputBuf));
        m_cbOutputBuf = 0;
    }
    unlock();
}


/**
 * Lock the object.
 */
void
VBoxDbgConsole::lock()
{
    RTCritSectEnter(&m_Lock);
}


/**
 * Unlocks the object.
 */
void
VBoxDbgConsole::unlock()
{
    RTCritSectLeave(&m_Lock);
}



/**
 * Checks if there is input.
 *
 * @returns true if there is input ready.
 * @returns false if there not input ready.
 * @param   pBack       Pointer to VBoxDbgConsole::m_Back.
 * @param   cMillies    Number of milliseconds to wait on input data.
 */
/*static*/ DECLCALLBACK(bool)
VBoxDbgConsole::backInput(PDBGCBACK pBack, uint32_t cMillies)
{
    VBoxDbgConsole *pThis = VBOXDBGCONSOLE_FROM_DBGCBACK(pBack);
    pThis->lock();

    bool fRc = true;
    if (!pThis->m_cbInputBuf)
    {
        /*
         * Wait outside the lock for the requested time, then check again.
         */
        pThis->unlock();
        RTSemEventWait(pThis->m_EventSem, cMillies);
        pThis->lock();
        fRc = pThis->m_cbInputBuf
           || ASMAtomicUoReadBool(&pThis->m_fTerminate);
    }

    pThis->unlock();
    return fRc;
}


/**
 * Read input.
 *
 * @returns VBox status code.
 * @param   pBack       Pointer to VBoxDbgConsole::m_Back.
 * @param   pvBuf       Where to put the bytes we read.
 * @param   cbBuf       Maximum nymber of bytes to read.
 * @param   pcbRead     Where to store the number of bytes actually read.
 *                      If NULL the entire buffer must be filled for a
 *                      successful return.
 */
/*static*/ DECLCALLBACK(int)
VBoxDbgConsole::backRead(PDBGCBACK pBack, void *pvBuf, size_t cbBuf, size_t *pcbRead)
{
    VBoxDbgConsole *pThis = VBOXDBGCONSOLE_FROM_DBGCBACK(pBack);
    Assert(pcbRead); /** @todo implement this bit */
    if (pcbRead)
        *pcbRead = 0;

    pThis->lock();
    int rc = VINF_SUCCESS;
    if (!ASMAtomicUoReadBool(&pThis->m_fTerminate))
    {
        if (pThis->m_cbInputBuf)
        {
            const char *psz = pThis->m_pszInputBuf;
            size_t cbRead = RT_MIN(pThis->m_cbInputBuf, cbBuf);
            memcpy(pvBuf, psz, cbRead);
            psz += cbRead;
            pThis->m_cbInputBuf -= cbRead;
            if (*psz)
                memmove(pThis->m_pszInputBuf, psz, pThis->m_cbInputBuf);
            pThis->m_pszInputBuf[pThis->m_cbInputBuf] = '\0';
            *pcbRead = cbRead;
        }
    }
    else
        rc = VERR_GENERAL_FAILURE;
    pThis->unlock();
    return rc;
}


/**
 * Write (output).
 *
 * @returns VBox status code.
 * @param   pBack       Pointer to VBoxDbgConsole::m_Back.
 * @param   pvBuf       What to write.
 * @param   cbBuf       Number of bytes to write.
 * @param   pcbWritten  Where to store the number of bytes actually written.
 *                      If NULL the entire buffer must be successfully written.
 */
/*static*/ DECLCALLBACK(int)
VBoxDbgConsole::backWrite(PDBGCBACK pBack, const void *pvBuf, size_t cbBuf, size_t *pcbWritten)
{
    VBoxDbgConsole *pThis = VBOXDBGCONSOLE_FROM_DBGCBACK(pBack);
    int rc = VINF_SUCCESS;

    pThis->lock();
    if (cbBuf + pThis->m_cbOutputBuf >= pThis->m_cbOutputBufAlloc)
    {
        size_t cbNew = RT_ALIGN_Z(cbBuf + pThis->m_cbOutputBufAlloc + 1, 1024);
        void *pv = RTMemRealloc(pThis->m_pszOutputBuf, cbNew);
        if (!pv)
        {
            pThis->unlock();
            if (pcbWritten)
                *pcbWritten = 0;
            return VERR_NO_MEMORY;
        }
        pThis->m_pszOutputBuf = (char *)pv;
        pThis->m_cbOutputBufAlloc = cbNew;
    }

    /*
     * Add the output.
     */
    memcpy(pThis->m_pszOutputBuf + pThis->m_cbOutputBuf, pvBuf, cbBuf);
    pThis->m_cbOutputBuf += cbBuf;
    pThis->m_pszOutputBuf[pThis->m_cbOutputBuf] = '\0';
    if (pcbWritten)
        *pcbWritten = cbBuf;

    if (ASMAtomicUoReadBool(&pThis->m_fTerminate))
        rc = VERR_GENERAL_FAILURE;

    /*
     * Tell the GUI thread to draw this text.
     * We cannot do it from here without frequent crashes.
     */
    if (!pThis->m_fUpdatePending)
        QApplication::postEvent(pThis, new VBoxDbgConsoleEvent(VBoxDbgConsoleEvent::kUpdate));

    pThis->unlock();

    return rc;
}


/*static*/ DECLCALLBACK(void)
VBoxDbgConsole::backSetReady(PDBGCBACK pBack, bool fReady)
{
    VBoxDbgConsole *pThis = VBOXDBGCONSOLE_FROM_DBGCBACK(pBack);
    if (fReady)
        QApplication::postEvent(pThis, new VBoxDbgConsoleEvent(VBoxDbgConsoleEvent::kInputEnable));
}


/**
 * The Debugger Console Thread
 *
 * @returns VBox status code (ignored).
 * @param   Thread      The thread handle.
 * @param   pvUser      Pointer to the VBoxDbgConsole object.s
 */
/*static*/ DECLCALLBACK(int)
VBoxDbgConsole::backThread(RTTHREAD Thread, void *pvUser)
{
    VBoxDbgConsole *pThis = (VBoxDbgConsole *)pvUser;
    LogFlow(("backThread: Thread=%p pvUser=%p\n", (void *)Thread, pvUser));

    NOREF(Thread);

    /*
     * Create and execute the console.
     */
    int rc = pThis->dbgcCreate(&pThis->m_Back.Core, 0);

    ASMAtomicUoWriteBool(&pThis->m_fThreadTerminated, true);
    if (!ASMAtomicUoReadBool(&pThis->m_fTerminate))
        QApplication::postEvent(pThis, new VBoxDbgConsoleEvent(rc == VINF_SUCCESS
                                                               ? VBoxDbgConsoleEvent::kTerminatedUser
                                                               : VBoxDbgConsoleEvent::kTerminatedOther));
    LogFlow(("backThread: returns %Rrc (m_fTerminate=%RTbool)\n", rc, ASMAtomicUoReadBool(&pThis->m_fTerminate)));
    return rc;
}


bool
VBoxDbgConsole::event(QEvent *pGenEvent)
{
    Assert(isGUIThread());
    if (pGenEvent->type() == (QEvent::Type)VBoxDbgConsoleEvent::kEventNumber)
    {
        VBoxDbgConsoleEvent *pEvent = (VBoxDbgConsoleEvent *)pGenEvent;

        switch (pEvent->command())
        {
            /* make update pending. */
            case VBoxDbgConsoleEvent::kUpdate:
                lock();
                if (!m_fUpdatePending)
                {
                    m_fUpdatePending = true;
                    m_pTimer->setSingleShot(true);
                    m_pTimer->start(10);
                }
                unlock();
                break;

            /* Re-enable the input field and restore focus. */
            case VBoxDbgConsoleEvent::kInputEnable:
                Log(("VBoxDbgConsole: kInputEnable (input-enabled=%RTbool)\n", m_pInput->isEnabled()));
                m_pInput->setEnabled(true);
                if (    m_fInputRestoreFocus
                    &&  !m_pInput->hasFocus())
                    m_pInput->setFocus(); /* this is a hack. */
                m_fInputRestoreFocus = false;
                break;

            /* The thread terminated by user command (exit, quit, bye). */
            case VBoxDbgConsoleEvent::kTerminatedUser:
                Log(("VBoxDbgConsole: kTerminatedUser (input-enabled=%RTbool)\n", m_pInput->isEnabled()));
                m_pInput->setEnabled(false);
                close();
                break;

            /* The thread terminated for some unknown reason., disable input */
            case VBoxDbgConsoleEvent::kTerminatedOther:
                Log(("VBoxDbgConsole: kTerminatedOther (input-enabled=%RTbool)\n", m_pInput->isEnabled()));
                m_pInput->setEnabled(false);
                break;

            /* paranoia */
            default:
                AssertMsgFailed(("command=%d\n", pEvent->command()));
                break;
        }
        return true;
    }

    return VBoxDbgBaseWindow::event(pGenEvent);
}


void
VBoxDbgConsole::closeEvent(QCloseEvent *a_pCloseEvt)
{
    if (m_fThreadTerminated)
    {
        a_pCloseEvt->accept();
        delete this;
    }
}


void
VBoxDbgConsole::actFocusToInput()
{
    if (!m_pInput->hasFocus())
        m_pInput->setFocus(Qt::ShortcutFocusReason);
}


void
VBoxDbgConsole::actFocusToOutput()
{
    if (!m_pOutput->hasFocus())
        m_pOutput->setFocus(Qt::ShortcutFocusReason);
}

