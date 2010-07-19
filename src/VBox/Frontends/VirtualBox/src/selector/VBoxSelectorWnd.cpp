/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * VBoxSelectorWnd class implementation
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include "precomp.h"
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */
#include "QISplitter.h"
#include "UIBar.h"
#include "UIDownloaderUserManual.h"
#include "UIExportApplianceWzd.h"
#include "UIIconPool.h"
#include "UIImportApplianceWzd.h"
#include "UINewVMWzd.h"
#include "UIVMDesktop.h"
#include "UIVMListView.h"
#include "UIVirtualBoxEventHandler.h"
#include "VBoxGlobal.h"
#include "VBoxMediaManagerDlg.h"
#include "VBoxProblemReporter.h"
#include "VBoxSelectorWnd.h"
#include "VBoxSettingsDialogSpecific.h"
#include "VBoxToolBar.h"
#include "VBoxVMLogViewer.h"

/* Global includes */
#include <QDesktopWidget>
#include <QMenuBar>
#include <QResizeEvent>

#include <iprt/buildconfig.h>
#include <VBox/version.h>
#ifdef Q_WS_X11
# include <iprt/env.h>
#endif
#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

// VBoxSelectorWnd class
////////////////////////////////////////////////////////////////////////////////

/** \class VBoxSelectorWnd
 *
 *  The VBoxSelectorWnd class is a VM selector window, one of two main VBox
 *  GUI windows.
 *
 *  This window appears when the user starts the VirtualBox executable.
 *  It allows to view the list of configured VMs, their settings
 *  and the current state, create, reconfigure, delete and start VMs.
 */

/**
 *  Constructs the VM selector window.
 *
 *  @param aSelf pointer to a variable where to store |this| right after
 *               this object's constructor is called (necessary to avoid
 *               recursion in VBoxGlobal::selectorWnd())
 */
VBoxSelectorWnd::
VBoxSelectorWnd (VBoxSelectorWnd **aSelf, QWidget* aParent,
                 Qt::WindowFlags aFlags /* = Qt::Window */)
    : QIWithRetranslateUI2<QMainWindow> (aParent, aFlags)
    , mDoneInaccessibleWarningOnce (false)
{
    VBoxGlobalSettings settings = vboxGlobal().settings();

    if (aSelf)
        *aSelf = this;

    statusBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(statusBar(), SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(showViewContextMenu(const QPoint&)));

#if defined (Q_WS_MAC) && (QT_VERSION < 0x040402)
    qApp->installEventFilter (this);
#endif /* defined (Q_WS_MAC) && (QT_VERSION < 0x040402) */

#if !(defined (Q_WS_WIN) || defined (Q_WS_MAC))
    /* The application icon. On Win32, it's built-in to the executable. On Mac
     * OS X the icon referenced in info.plist is used. */
    setWindowIcon (QIcon (":/VirtualBox_48px.png"));
#endif

    /* actions */

    mFileMediaMgrAction = new QAction (this);
    mFileMediaMgrAction->setIcon(UIIconPool::iconSet(":/diskimage_16px.png"));

    mFileApplianceImportAction = new QAction (this);
    mFileApplianceImportAction->setIcon(UIIconPool::iconSet(":/import_16px.png"));

    mFileApplianceExportAction = new QAction (this);
    mFileApplianceExportAction->setIcon(UIIconPool::iconSet(":/export_16px.png"));

    mFileSettingsAction = new QAction(this);
    mFileSettingsAction->setMenuRole (QAction::PreferencesRole);
    mFileSettingsAction->setIcon(UIIconPool::iconSet(":/global_settings_16px.png"));
    mFileExitAction = new QAction (this);
    mFileExitAction->setMenuRole (QAction::QuitRole);
    mFileExitAction->setIcon(UIIconPool::iconSet(":/exit_16px.png"));

    mVmNewAction = new QAction (this);
    mVmNewAction->setIcon(UIIconPool::iconSetFull(
        QSize (32, 32), QSize (16, 16),
        ":/vm_new_32px.png", ":/new_16px.png"));
    mVmConfigAction = new QAction (this);
    mVmConfigAction->setIcon(UIIconPool::iconSetFull(
        QSize (32, 32), QSize (16, 16),
        ":/vm_settings_32px.png", ":/settings_16px.png",
        ":/vm_settings_disabled_32px.png", ":/settings_dis_16px.png"));
    mVmDeleteAction = new QAction (this);
    mVmDeleteAction->setIcon(UIIconPool::iconSetFull(
        QSize (32, 32), QSize (16, 16),
        ":/vm_delete_32px.png", ":/delete_16px.png",
        ":/vm_delete_disabled_32px.png", ":/delete_dis_16px.png"));
    mVmStartAction = new QAction (this);
    mVmStartAction->setIcon(UIIconPool::iconSetFull(
        QSize (32, 32), QSize (16, 16),
        ":/vm_start_32px.png", ":/start_16px.png",
        ":/vm_start_disabled_32px.png", ":/start_dis_16px.png"));
    mVmDiscardAction = new QAction (this);
    mVmDiscardAction->setIcon(UIIconPool::iconSetFull(
        QSize (32, 32), QSize (16, 16),
        ":/vm_discard_32px.png", ":/discard_16px.png",
        ":/vm_discard_disabled_32px.png", ":/discard_dis_16px.png"));
    mVmPauseAction = new QAction (this);
    mVmPauseAction->setCheckable (true);
    mVmPauseAction->setIcon(UIIconPool::iconSetFull(
        QSize (32, 32), QSize (16, 16),
        ":/vm_pause_32px.png", ":/pause_16px.png",
        ":/vm_pause_disabled_32px.png", ":/pause_disabled_16px.png"));
    mVmRefreshAction = new QAction (this);
    mVmRefreshAction->setIcon(UIIconPool::iconSetFull(
        QSize (32, 32), QSize (16, 16),
        ":/refresh_32px.png", ":/refresh_16px.png",
        ":/refresh_disabled_32px.png", ":/refresh_disabled_16px.png"));
    mVmShowLogsAction = new QAction (this);
    mVmShowLogsAction->setIcon(UIIconPool::iconSetFull(
        QSize (32, 32), QSize (16, 16),
        ":/vm_show_logs_32px.png", ":/show_logs_16px.png",
        ":/vm_show_logs_disabled_32px.png", ":/show_logs_disabled_16px.png"));

    mHelpActions.setup (this);

    /* VM list toolbar */
    mVMToolBar = new VBoxToolBar(this);
    mVMToolBar->setContextMenuPolicy(Qt::CustomContextMenu);
#ifndef Q_WS_MAC
    connect(mVMToolBar, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(showViewContextMenu(const QPoint&)));
#else /* !Q_WS_MAC */
    /* A simple connect doesn't work on the Mac, also we want receive right
     * click notifications on the title bar. So register our own handler. */
    ::darwinRegisterForUnifiedToolbarContextMenuEvents(this);
#endif /* Q_WS_MAC */

    /* VM list view */
    mVMListView = new UIVMListView();
    mVMModel = new UIVMItemModel(mVMListView);
    mVMListView->setModel(mVMModel);
    mVMListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    /* Make non-possible to activate list elements by single click,
     * this hack should disable the current possibility to do it if present */
    if (mVMListView->style()->styleHint (QStyle::SH_ItemView_ActivateItemOnSingleClick, 0, mVMListView))
        mVMListView->setStyleSheet ("activate-on-singleclick : 0");

    m_pSplitter = new QISplitter(this);
    m_pSplitter->setHandleType(QISplitter::Native);

#define BIG_TOOLBAR
#if MAC_LEOPARD_STYLE
    /* Enable unified toolbars on Mac OS X. Available on Qt >= 4.3 */
    addToolBar (mVMToolBar);
    mVMToolBar->setMacToolbar();
    /* Central widget @ horizontal layout */
    setCentralWidget(m_pSplitter);
    m_pSplitter->addWidget (mVMListView);
#else /* MAC_LEOPARD_STYLE */
//    mVMToolBar->setContentsMargins(5, 5, 0, 0);
//    addToolBar (mVMToolBar);
//    m_pSplitter->addWidget (mVMListView);
    QWidget *pLeftWidget = new QWidget(this);
    QVBoxLayout *pLeftVLayout = new QVBoxLayout(pLeftWidget);
    pLeftVLayout->setContentsMargins(0, 0, 0, 0);
    pLeftVLayout->setSpacing(0);
# ifdef BIG_TOOLBAR
    m_pBar = new UIBar(this);
    m_pBar->setContentWidget(mVMToolBar);
    pLeftVLayout->addWidget(m_pBar);
    pLeftVLayout->addWidget(m_pSplitter);
    setCentralWidget(pLeftWidget);
    m_pSplitter->addWidget(mVMListView);
# else /* BIG_TOOLBAR */
    pLeftVLayout->addWidget(mVMToolBar);
    pLeftVLayout->addWidget(mVMListView);
    setCentralWidget(m_pSplitter);
    m_pSplitter->addWidget(pLeftWidget);
# endif /* BIG_TOOLBAR */
#endif /* MAC_LEOPARD_STYLE */

    /* add actions to the toolbar */

    mVMToolBar->setIconSize (QSize (32, 32));
    mVMToolBar->setToolButtonStyle (Qt::ToolButtonTextUnderIcon);
//    mVMToolBar->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Preferred);

    mVMToolBar->addAction (mVmNewAction);
    mVMToolBar->addAction (mVmConfigAction);
#if 0 /* delete action is really rare */
    mVMToolBar->addAction (mVmDeleteAction);
#endif
    mVMToolBar->addAction (mVmStartAction);
    mVMToolBar->addAction (mVmDiscardAction);

    /* VM tab widget containing details and snapshots tabs */
    QWidget *w = new QWidget();
    QVBoxLayout *pVBox = new QVBoxLayout(w);
    pVBox->setContentsMargins(0, 0, 0, 0);

    m_pVMDesktop = new UIVMDesktop(mVMToolBar, mVmRefreshAction, this);
    pVBox->addWidget(m_pVMDesktop);

    m_pSplitter->addWidget (w);

    /* Set the initial distribution. The right site is bigger. */
    m_pSplitter->setStretchFactor(0, 2);
    m_pSplitter->setStretchFactor(1, 3);

    /* Configure menubar */
    menuBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(menuBar(), SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(showViewContextMenu(const QPoint&)));

    /* add actions to menubar */
    mFileMenu = menuBar()->addMenu (QString::null);
    mFileMenu->addAction (mFileMediaMgrAction);
    mFileMenu->addAction (mFileApplianceImportAction);
    mFileMenu->addAction (mFileApplianceExportAction);
#ifndef Q_WS_MAC
    mFileMenu->addSeparator();
#endif /* Q_WS_MAC */
    mFileMenu->addAction (mFileSettingsAction);
#ifndef Q_WS_MAC
    mFileMenu->addSeparator();
#endif /* Q_WS_MAC */
    mFileMenu->addAction (mFileExitAction);

    mVMMenu = menuBar()->addMenu (QString::null);
    mVMMenu->addAction (mVmNewAction);
    mVMMenu->addAction (mVmConfigAction);
    mVMMenu->addAction (mVmDeleteAction);
    mVMMenu->addSeparator();
    mVMMenu->addAction (mVmStartAction);
    mVMMenu->addAction (mVmDiscardAction);
    mVMMenu->addAction (mVmPauseAction);
    mVMMenu->addSeparator();
    mVMMenu->addAction (mVmRefreshAction);
    mVMMenu->addAction (mVmShowLogsAction);

    mVMCtxtMenu = new QMenu (this);
    mVMCtxtMenu->addAction (mVmConfigAction);
    mVMCtxtMenu->addAction (mVmDeleteAction);
    mVMCtxtMenu->addSeparator();
    mVMCtxtMenu->addAction (mVmStartAction);
    mVMCtxtMenu->addAction (mVmDiscardAction);
    mVMCtxtMenu->addAction (mVmPauseAction);
    mVMCtxtMenu->addSeparator();
    mVMCtxtMenu->addAction (mVmRefreshAction);
    mVMCtxtMenu->addAction (mVmShowLogsAction);

    /* Make sure every status bar hint from the context menu is cleared when
     * the menu is closed. */
    connect (mVMCtxtMenu, SIGNAL(aboutToHide()),
             statusBar(), SLOT(clearMessage()));

    mHelpMenu = menuBar()->addMenu (QString::null);
    mHelpActions.addTo (mHelpMenu);

#ifdef VBOX_GUI_WITH_SYSTRAY
    mTrayIcon = new VBoxTrayIcon (this, mVMModel);
    Assert (mTrayIcon);
    connect (mTrayIcon, SIGNAL (activated (QSystemTrayIcon::ActivationReason)),
             this, SLOT (trayIconActivated (QSystemTrayIcon::ActivationReason)));
#endif

    retranslateUi();

    CVirtualBox vbox = vboxGlobal().virtualBox();
    /* Restore the position of the window */
    {
        QString winPos = vbox.GetExtraData (VBoxDefs::GUI_LastWindowPosition);

        bool ok = false, max = false;
        int x = 0, y = 0, w = 0, h = 0;
        x = winPos.section (',', 0, 0).toInt (&ok);
        if (ok)
            y = winPos.section (',', 1, 1).toInt (&ok);
        if (ok)
            w = winPos.section (',', 2, 2).toInt (&ok);
        if (ok)
            h = winPos.section (',', 3, 3).toInt (&ok);
        if (ok)
            max = winPos.section (',', 4, 4) == VBoxDefs::GUI_LastWindowState_Max;

        QRect ar = ok ? QApplication::desktop()->availableGeometry (QPoint (x, y)) :
                        QApplication::desktop()->availableGeometry (this);

        if (ok /* previous parameters were read correctly */
            && (y > 0) && (y < ar.bottom()) /* check vertical bounds */
            && (x + w > ar.left()) && (x < ar.right()) /* & horizontal bounds */)
        {
            mNormalGeo.moveTo (x, y);
            mNormalGeo.setSize (QSize (w, h).expandedTo (minimumSizeHint())
                                            .boundedTo (ar.size()));
            setGeometry (mNormalGeo);
            if (max) /* maximize if needed */
                showMaximized();
        }
        else
        {
            mNormalGeo.setSize (QSize (770, 550).expandedTo (minimumSizeHint())
                                                .boundedTo (ar.size()));
            mNormalGeo.moveCenter (ar.center());
            setGeometry (mNormalGeo);
        }
    }

    /* Update the list */
    refreshVMList();

    /* Reset to the first item */
    mVMListView->selectItemByRow (0);

    /* restore the position of vm selector */
    {
        QString prevVMId = vbox.GetExtraData (VBoxDefs::GUI_LastVMSelected);

        mVMListView->selectItemById (prevVMId);
    }

    /* Read the splitter handle position */
    {
        QList<int> sizes = vbox.GetExtraDataIntList(VBoxDefs::GUI_SplitterSizes);
        if (sizes.size() == 2)
            m_pSplitter->setSizes(sizes);
    }

    /* Restore toolbar and statusbar visibility */
    {
        QString strToolbar = vbox.GetExtraData(VBoxDefs::GUI_Toolbar);
        QString strStatusbar = vbox.GetExtraData(VBoxDefs::GUI_Statusbar);

#ifdef Q_WS_MAC
        mVMToolBar->setVisible(strToolbar.isEmpty() || strToolbar == "true");
#else /* Q_WS_MAC */
        m_pBar->setVisible(strToolbar.isEmpty() || strToolbar == "true");
#endif /* !Q_WS_MAC */
        statusBar()->setVisible(strStatusbar.isEmpty() || strStatusbar == "true");
    }

    /* refresh the details et all (necessary for the case when the stored
     * selection is still the first list item) */
    vmListViewCurrentChanged();

    /* signals and slots connections */
    connect (mFileMediaMgrAction, SIGNAL (triggered()), this, SLOT (fileMediaMgr()));
    connect (mFileApplianceImportAction, SIGNAL (triggered()), this, SLOT (fileImportAppliance()));
    connect (mFileApplianceExportAction, SIGNAL (triggered()), this, SLOT (fileExportAppliance()));
    connect (mFileSettingsAction, SIGNAL (triggered()), this, SLOT (fileSettings()));
    connect (mFileExitAction, SIGNAL (triggered()), this, SLOT (fileExit()));
    connect (mVmNewAction, SIGNAL (triggered()), this, SLOT (vmNew()));

    connect (mVmConfigAction, SIGNAL (triggered()), this, SLOT (vmSettings()));
    connect (mVmDeleteAction, SIGNAL (triggered()), this, SLOT (vmDelete()));
    connect (mVmStartAction, SIGNAL (triggered()), this, SLOT (vmStart()));
    connect (mVmDiscardAction, SIGNAL (triggered()), this, SLOT (vmDiscard()));
    connect (mVmPauseAction, SIGNAL (toggled (bool)), this, SLOT (vmPause (bool)));
    connect (mVmRefreshAction, SIGNAL (triggered()), this, SLOT (vmRefresh()));
    connect (mVmShowLogsAction, SIGNAL (triggered()), this, SLOT (vmShowLogs()));

    connect (mVMListView, SIGNAL (currentChanged()),
             this, SLOT (vmListViewCurrentChanged()));
    connect (mVMListView, SIGNAL (activated()),
             this, SLOT (vmStart()));
    connect (mVMListView, SIGNAL (customContextMenuRequested (const QPoint &)),
             this, SLOT (showContextMenu (const QPoint &)));

    connect (m_pVMDesktop, SIGNAL(linkClicked(const QString &)),
             this, SLOT(vmSettings(const QString &)));

    /* listen to media enumeration signals */
    connect (&vboxGlobal(), SIGNAL (mediumEnumStarted()),
             this, SLOT (mediumEnumStarted()));
    connect (&vboxGlobal(), SIGNAL (mediumEnumFinished (const VBoxMediaList &)),
             this, SLOT (mediumEnumFinished (const VBoxMediaList &)));

    /* connect VirtualBox events */
    connect (gVBoxEvents, SIGNAL(sigMachineStateChange(QString, KMachineState)),
             this, SLOT(machineStateChanged(QString, KMachineState)));
    connect (gVBoxEvents, SIGNAL(sigMachineDataChange(QString)),
             this, SLOT(machineDataChanged(QString)));
    connect (gVBoxEvents, SIGNAL(sigMachineRegistered(QString, bool)),
             this, SLOT(machineRegistered(QString, bool)));
    connect (gVBoxEvents, SIGNAL(sigSessionStateChange(QString, KSessionState)),
             this, SLOT(sessionStateChanged(QString, KSessionState)));
    connect (gVBoxEvents, SIGNAL(sigSnapshotChange(QString, QString)),
             this, SLOT(snapshotChanged(QString, QString)));
#ifdef VBOX_GUI_WITH_SYSTRAY
    connect (gEDataEvents, SIGNAL(sigMainWindowCountChange(int)),
             this, SLOT(mainWindowCountChanged(int)));
    connect (gEDataEvents, SIGNAL(sigCanShowTrayIcon(bool)),
             this, SLOT(trayIconCanShow(bool)));
    connect (gEDataEvents, SIGNAL(sigTrayIconChange(bool)),
             this, SLOT(trayIconChanged(bool)));
    connect (&vboxGlobal(), SIGNAL(sigTrayIconShow(bool)),
             this, SLOT(trayIconShow(bool)));
#endif

    /* Listen to potential downloaders signals: */
    connect(&vboxProblem(), SIGNAL(sigDownloaderUserManualCreated()), this, SLOT(sltDownloaderUserManualEmbed()));

    /* bring the VM list to the focus */
    mVMListView->setFocus();
}

VBoxSelectorWnd::~VBoxSelectorWnd()
{
    CVirtualBox vbox = vboxGlobal().virtualBox();

    /* Destroy our event handlers */
    UIVirtualBoxEventHandler::destroy();

    /* Save the position of the window */
    {
        int y = mNormalGeo.y();
        QString winPos = QString ("%1,%2,%3,%4")
            .arg (mNormalGeo.x()).arg (y)
            .arg (mNormalGeo.width()).arg (mNormalGeo.height());
#ifdef Q_WS_MAC
        ::darwinUnregisterForUnifiedToolbarContextMenuEvents(this);
        if (::darwinIsWindowMaximized(this))
#else /* Q_WS_MAC */
        if (isMaximized())
#endif /* !Q_WS_MAC */
            winPos += QString (",%1").arg (VBoxDefs::GUI_LastWindowState_Max);

        vbox.SetExtraData (VBoxDefs::GUI_LastWindowPosition, winPos);
    }

    /* Save VM selector position */
    {
        UIVMItem *item = mVMListView->selectedItem();
        QString curVMId = item ?
            QString (item->id()) :
            QString::null;
        vbox.SetExtraData (VBoxDefs::GUI_LastVMSelected, curVMId);
    }

    /* Save the splitter handle position */
    {
        vbox.SetExtraDataIntList(VBoxDefs::GUI_SplitterSizes, m_pSplitter->sizes());
    }

#ifdef VBOX_GUI_WITH_SYSTRAY
    /* Delete systray menu object */
    delete mTrayIcon;
    mTrayIcon = NULL;
#endif

    /* Delete the items from our model */
    mVMModel->clear();
}

//
// Public slots
/////////////////////////////////////////////////////////////////////////////

void VBoxSelectorWnd::fileMediaMgr()
{
    VBoxMediaManagerDlg::showModeless (this);
}

void VBoxSelectorWnd::fileImportAppliance()
{
    UIImportApplianceWzd wzd (this);

    wzd.exec();
}

void VBoxSelectorWnd::fileExportAppliance()
{
    QString name;

    UIVMItem *item = mVMListView->selectedItem();
    if (item)
        name = item->name();

    UIExportApplianceWzd wzd (this, name);

    wzd.exec();
}

void VBoxSelectorWnd::fileSettings()
{
    VBoxGlobalSettings settings = vboxGlobal().settings();
    CSystemProperties props = vboxGlobal().virtualBox().GetSystemProperties();

    VBoxSettingsDialog *dlg = new VBoxGLSettingsDlg (this);
    dlg->getFrom();

    if (dlg->exec() == QDialog::Accepted)
        dlg->putBackTo();

    delete dlg;
}

void VBoxSelectorWnd::fileExit()
{
    /* We have to check if there are any open windows beside this mainwindow
     * (e.g. VDM) and if so close them. Note that the default behavior is
     * different to Qt3 where a *mainWidget* exists & if this going to close
     * all other windows are closed automatically. We do the same below. */
    foreach (QWidget *widget, QApplication::topLevelWidgets())
    {
        if (widget->isVisible() &&
            widget != this)
            widget->close();
    }
    /* We close this widget last. */
    close();
}

void VBoxSelectorWnd::vmNew()
{
    UINewVMWzd wzd (this);
    if (wzd.exec() == QDialog::Accepted)
    {
        CMachine m = wzd.machine();

        /* wait until the list is updated by OnMachineRegistered() */
        QModelIndex index;
        while (!index.isValid())
        {
            qApp->processEvents();
            index = mVMModel->indexById (m.GetId());
        }
        mVMListView->setCurrentIndex (index);
    }
}

/**
 *  Opens the VM settings dialog.
 */
void VBoxSelectorWnd::vmSettings (const QString &aCategory /* = QString::null */,
                                  const QString &aControl /* = QString::null */,
                                  const QString &aUuid /* = QString::null */)
{
    if (!aCategory.isEmpty() && aCategory [0] != '#')
    {
        /* Assume it's a href from the Details HTML */
        vboxGlobal().openURL (aCategory);
        return;
    }
    QString strCategory = aCategory;
    QString strControl = aControl;
    /* Maybe the control is coded into the URL by %% */
    if (aControl == QString::null)
    {
        QStringList parts = aCategory.split("%%");
        if (parts.size() == 2)
        {
            strCategory = parts.at(0);
            strControl = parts.at(1);
        }
    }

    UIVMItem *item = aUuid.isNull() ? mVMListView->selectedItem() :
                       mVMModel->itemById (aUuid);

    AssertMsgReturnVoid (item, ("Item must be always selected here"));

    // open a direct session to modify VM settings
    QString id = item->id();
    CSession session = vboxGlobal().openSession (id);
    if (session.isNull())
        return;

    CMachine m = session.GetMachine();
    AssertMsgReturn (!m.isNull(), ("Machine must not be null"), (void) 0);

    VBoxSettingsDialog *dlg = new VBoxVMSettingsDlg (this, m, strCategory, strControl);
    dlg->getFrom();

    if (dlg->exec() == QDialog::Accepted)
    {
        QString oldName = m.GetName();
        dlg->putBackTo();

        m.SaveSettings();
        if (m.isOk())
        {
            if (oldName.compare (m.GetName()))
                mVMModel->sort();
        }
        else
            vboxProblem().cannotSaveMachineSettings (m);

        /* To check use the result in future
         * vboxProblem().cannotApplyMachineSettings (m, res); */
    }

    delete dlg;

    mVMListView->setFocus();

    session.Close();
}

void VBoxSelectorWnd::vmDelete (const QString &aUuid /* = QString::null */)
{
    UIVMItem *item = aUuid.isNull() ? mVMListView->selectedItem() :
                       mVMModel->itemById (aUuid);

    AssertMsgReturnVoid (item, ("Item must be always selected here"));

    if (vboxProblem().confirmMachineDeletion (item->machine()))
    {
        CVirtualBox vbox = vboxGlobal().virtualBox();
        QString id = item->id();
        bool ok = false;
        if (item->accessible())
        {
            /* Open a direct session to modify VM settings */
            CSession session = vboxGlobal().openSession (id);
            if (session.isNull())
                return;
            CMachine machine = session.GetMachine();
            /* Detach all attached Hard Disks */
            CMediumAttachmentVector vec = machine.GetMediumAttachments();
            for (int i = 0; i < vec.size(); ++ i)
            {
                CMediumAttachment hda = vec [i];
                const QString ctlName = hda.GetController();

                machine.DetachDevice(ctlName, hda.GetPort(), hda.GetDevice());
                if (!machine.isOk())
                {
                    CStorageController ctl = machine.GetStorageControllerByName(ctlName);
                    vboxProblem().cannotDetachDevice (this, machine, VBoxDefs::MediumType_HardDisk,
                        vboxGlobal().getMedium (CMedium (hda.GetMedium())).location(),
                        ctl.GetBus(), hda.GetPort(), hda.GetDevice());
                }
            }
            /* Commit changes */
            machine.SaveSettings();
            if (!machine.isOk())
                vboxProblem().cannotSaveMachineSettings (machine);
            else
                ok = true;
            session.Close();
        }
        else
            ok = true;

        if (ok)
        {
            CMachine machine = item->machine();
            vbox.UnregisterMachine (id);
            if (vbox.isOk() && item->accessible())
            {
                /* delete machine settings */
                machine.DeleteSettings();
                /* remove the item shortly: cmachine it refers to is no longer valid! */
                int row = mVMModel->rowById (item->id());
                mVMModel->removeItem (item);
                delete item;
                mVMListView->ensureSomeRowSelected (row);
            }
            if (!vbox.isOk() || !machine.isOk())
                vboxProblem().cannotDeleteMachine (vbox, machine);
        }
    }
}

void VBoxSelectorWnd::vmStart (const QString &aUuid /* = QString::null */)
{
    QUuid uuid (aUuid);
    UIVMItem *item = uuid.isNull() ? mVMListView->selectedItem() :
                       mVMModel->itemById (aUuid);

    AssertMsgReturnVoid (item, ("Item must be always selected here"));

    /* Are we called from the mVMListView's activated() signal? */
    if (uuid.isNull())
    {
        /* We always get here when mVMListView emits the activated() signal,
         * so we must explicitly check if the action is enabled or not. */
        if (!mVmStartAction->isEnabled())
            return;
    }

    AssertMsg (!vboxGlobal().isVMConsoleProcess(),
               ("Must NOT be a VM console process"));

    /* just switch to the VM window if it already exists */
    if (item->canSwitchTo())
    {
        item->switchTo();
        return;
    }

    AssertMsg (   item->state() == KMachineState_PoweredOff
               || item->state() == KMachineState_Saved
               || item->state() == KMachineState_Teleported
               || item->state() == KMachineState_Aborted
               , ("Machine must be PoweredOff/Saved/Aborted (%d)", item->state()));

    QString id = item->id();
    CVirtualBox vbox = vboxGlobal().virtualBox();
    CSession session;

    session.createInstance (CLSID_Session);
    if (session.isNull())
    {
        vboxProblem().cannotOpenSession (session);
        return;
    }

#if defined (Q_OS_WIN32)
    /* allow the started VM process to make itself the foreground window */
    AllowSetForegroundWindow (ASFW_ANY);
#endif

    QString env;
#if defined (Q_WS_X11)
    /* make sure the VM process will start on the same display as the Selector */
    const char *display = RTEnvGet ("DISPLAY");
    if (display)
        env.append(QString("DISPLAY=%1\n").arg(display));
    const char *xauth = RTEnvGet ("XAUTHORITY");
    if (xauth)
        env.append(QString("XAUTHORITY=%1\n").arg(xauth));
#endif

    CProgress progress = vbox.OpenRemoteSession (session, id, "GUI/Qt", env);
    if (!vbox.isOk())
    {
        vboxProblem().cannotOpenSession (vbox, item->machine());
        return;
    }

    /* Hide the "VM spawning" progress dialog */
    /* I hope 1 minute will be enough to spawn any running VM silently, isn't it? */
    int iSpawningDuration = 60000;
    vboxProblem().showModalProgressDialog(progress, item->name(), this, iSpawningDuration);
    if (progress.GetResultCode() != 0)
        vboxProblem().cannotOpenSession(vbox, item->machine(), progress);

    session.Close();
}

void VBoxSelectorWnd::vmDiscard (const QString &aUuid /* = QString::null */)
{
    UIVMItem *item = aUuid.isNull() ? mVMListView->selectedItem() :
                       mVMModel->itemById (aUuid);

    AssertMsgReturnVoid (item, ("Item must be always selected here"));

    if (!vboxProblem().confirmDiscardSavedState (item->machine()))
        return;

    /* open a session to modify VM settings */
    QString id = item->id();
    CSession session;
    CVirtualBox vbox = vboxGlobal().virtualBox();
    session.createInstance (CLSID_Session);
    if (session.isNull())
    {
        vboxProblem().cannotOpenSession (session);
        return;
    }
    vbox.OpenSession (session, id);
    if (!vbox.isOk())
    {
        vboxProblem().cannotOpenSession (vbox, item->machine());
        return;
    }

    CConsole console = session.GetConsole();
    console.ForgetSavedState(true);
    if (!console.isOk())
        vboxProblem().cannotDiscardSavedState (console);

    session.Close();
}

void VBoxSelectorWnd::vmPause (bool aPause, const QString &aUuid /* = QString::null */)
{
    UIVMItem *item = aUuid.isNull() ? mVMListView->selectedItem() :
                       mVMModel->itemById (aUuid);

    AssertMsgReturnVoid (item, ("Item must be always selected here"));

    CSession session = vboxGlobal().openExistingSession (item->id());
    if (session.isNull())
        return;

    CConsole console = session.GetConsole();
    if (console.isNull())
        return;

    if (aPause)
        console.Pause();
    else
        console.Resume();

    bool ok = console.isOk();
    if (!ok)
    {
        if (aPause)
            vboxProblem().cannotPauseMachine (console);
        else
            vboxProblem().cannotResumeMachine (console);
    }

    session.Close();
}

void VBoxSelectorWnd::vmRefresh (const QString &aUuid /* = QString::null */)
{
    UIVMItem *item = aUuid.isNull() ? mVMListView->selectedItem() :
                       mVMModel->itemById (aUuid);

    AssertMsgReturnVoid (item, ("Item must be always selected here"));

    refreshVMItem (item->id(),
                   true /* aDetails */,
                   true /* aSnapshot */,
                   true /* aDescription */);
}

void VBoxSelectorWnd::vmShowLogs (const QString &aUuid /* = QString::null */)
{
    UIVMItem *item = aUuid.isNull() ? mVMListView->selectedItem() :
                       mVMModel->itemById (aUuid);

    AssertMsgReturnVoid (item, ("Item must be always selected here"));

    CMachine machine = item->machine();
    VBoxVMLogViewer::createLogViewer (this, machine);
}

void VBoxSelectorWnd::refreshVMList()
{
    CVirtualBox vbox = vboxGlobal().virtualBox();
    CMachineVector vec = vbox.GetMachines();
    for (CMachineVector::ConstIterator m = vec.begin();
         m != vec.end(); ++ m)
        mVMModel->addItem (new UIVMItem (*m));
    mVMModel->sort();

    vmListViewCurrentChanged();

#ifdef VBOX_GUI_WITH_SYSTRAY
    if (vboxGlobal().isTrayMenu())
        mTrayIcon->refresh();
#endif
}

void VBoxSelectorWnd::refreshVMItem (const QString &aID, bool aDetails,
                                                       bool aSnapshots,
                                                       bool aDescription)
{
    UIVMItem *item = mVMModel->itemById (aID);
    if (item)
    {
        mVMModel->refreshItem (item);
        if (item && item->id() == aID)
            vmListViewCurrentChanged (aDetails, aSnapshots, aDescription);
    }
}

void VBoxSelectorWnd::showContextMenu (const QPoint &aPoint)
{
    /* Send a context menu request */
    const QModelIndex &index = mVMListView->indexAt (aPoint);
    if (index.isValid())
        if (mVMListView->model()->data (index,
            UIVMItemModel::UIVMItemPtrRole).value <UIVMItem*>())
                mVMCtxtMenu->exec (mVMListView->mapToGlobal (aPoint));
}

#ifdef VBOX_GUI_WITH_SYSTRAY

void VBoxSelectorWnd::trayIconActivated (QSystemTrayIcon::ActivationReason aReason)
{
    switch (aReason)
    {
        case QSystemTrayIcon::Context:

            mTrayIcon->refresh();
            break;

        case QSystemTrayIcon::Trigger:
            break;

        case QSystemTrayIcon::DoubleClick:

            vboxGlobal().trayIconShowSelector();
            break;

        case QSystemTrayIcon::MiddleClick:
            break;

        default:
            break;
    }
}

void VBoxSelectorWnd::showWindow()
{
    showNormal();
    raise();
    activateWindow();
}

#endif // VBOX_GUI_WITH_SYSTRAY

// Protected members
/////////////////////////////////////////////////////////////////////////////

bool VBoxSelectorWnd::event (QEvent *e)
{
    switch (e->type())
    {
        /* By handling every Resize and Move we keep track of the normal
         * (non-minimized and non-maximized) window geometry. Shame on Qt
         * that it doesn't provide this geometry in its public APIs. */

        case QEvent::Resize:
        {
            QResizeEvent *re = (QResizeEvent *) e;
            if ((windowState() & (Qt::WindowMaximized | Qt::WindowMinimized |
                                  Qt::WindowFullScreen)) == 0)
                mNormalGeo.setSize (re->size());
            break;
        }
        case QEvent::Move:
        {
            if ((windowState() & (Qt::WindowMaximized | Qt::WindowMinimized |
                                  Qt::WindowFullScreen)) == 0)
                mNormalGeo.moveTo (geometry().x(), geometry().y());
            break;
        }
        case QEvent::WindowDeactivate:
        {
            /* Make sure every status bar hint is cleared when the window lost
             * focus. */
            statusBar()->clearMessage();
            break;
        }
#ifdef Q_WS_MAC
        case QEvent::ContextMenu:
        {
            /* This is the unified context menu event. Lets show the context
             * menu. */
            QContextMenuEvent *pCE = static_cast<QContextMenuEvent*>(e);
            showViewContextMenu(pCE->globalPos());
            /* Accept it to interrupt the chain. */
            pCE->accept();
            return false;
            break;
        }
        case QEvent::ToolBarChange:
        {
            CVirtualBox vbox = vboxGlobal().virtualBox();
            /* We have to invert the isVisible check one time, cause this event
             * is sent *before* the real toggle is done. Really intuitive
             * Trolls. */
            vbox.SetExtraData(VBoxDefs::GUI_Toolbar, !::darwinIsToolbarVisible(mVMToolBar) ? "true" : "false");
            break;
        }
#endif /* Q_WS_MAC */
        default:
            break;
    }

    return QMainWindow::event (e);
}

void VBoxSelectorWnd::closeEvent (QCloseEvent *aEvent)
{
#ifdef VBOX_GUI_WITH_SYSTRAY
    /* Needed for breaking out of the while() loop in main(). */
    if (vboxGlobal().isTrayMenu())
        vboxGlobal().setTrayMenu (false);
#endif

    emit closing();
    QMainWindow::closeEvent (aEvent);
}

#if defined (Q_WS_MAC) && (QT_VERSION < 0x040402)
bool VBoxSelectorWnd::eventFilter (QObject *aObject, QEvent *aEvent)
{
    if (!isActiveWindow())
        return QIWithRetranslateUI2<QMainWindow>::eventFilter (aObject, aEvent);

    if (qobject_cast<QWidget*> (aObject) &&
        qobject_cast<QWidget*> (aObject)->window() != this)
        return QIWithRetranslateUI2<QMainWindow>::eventFilter (aObject, aEvent);

    switch (aEvent->type())
    {
        case QEvent::KeyPress:
            {
                /* Bug in Qt below 4.4.2. The key events are send to the current
                 * window even if a menu is shown & has the focus. See
                 * http://trolltech.com/developer/task-tracker/index_html?method=entry&id=214681. */
                if (::darwinIsMenuOpen())
                    return true;
            }
        default:
            break;
    }
    return QIWithRetranslateUI2<QMainWindow>::eventFilter (aObject, aEvent);
}
#endif /* defined (Q_WS_MAC) && (QT_VERSION < 0x040402) */

/**
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void VBoxSelectorWnd::retranslateUi()
{
#ifdef VBOX_OSE
    QString title (tr ("VirtualBox OSE"));
#else
    QString title (VBOX_PRODUCT);
#endif

#ifdef VBOX_BLEEDING_EDGE
    title += QString(" EXPERIMENTAL build ")
          +  QString(RTBldCfgVersion())
          +  QString(" r")
          +  QString(RTBldCfgRevisionStr())
          +  QString(" - "VBOX_BLEEDING_EDGE);
#endif

    setWindowTitle (title);

    /* ensure the details and screenshot view are updated */
    vmListViewCurrentChanged();

    mFileMediaMgrAction->setText (tr ("&Virtual Media Manager..."));
    mFileMediaMgrAction->setShortcut (QKeySequence ("Ctrl+D"));
    mFileMediaMgrAction->setStatusTip (tr ("Display the Virtual Media Manager dialog"));

    mFileApplianceImportAction->setText (tr ("&Import Appliance..."));
    mFileApplianceImportAction->setShortcut (QKeySequence ("Ctrl+I"));
    mFileApplianceImportAction->setStatusTip (tr ("Import an appliance into VirtualBox"));

    mFileApplianceExportAction->setText (tr ("&Export Appliance..."));
    mFileApplianceExportAction->setShortcut (QKeySequence ("Ctrl+E"));
    mFileApplianceExportAction->setStatusTip (tr ("Export one or more VirtualBox virtual machines as an appliance"));

#ifdef Q_WS_MAC
    /*
     * Macification: Getting the right menu as application preference menu item.
     *
     * QMenuBar::isCommand() in qmenubar_mac.cpp doesn't recognize "Setting"(s)
     * unless it's in the first position. So, we use the Mac term here to make
     * sure we get picked instead of the VM settings.
     *
     * Now, since both QMenuBar and we translate these strings, it's going to
     * be really interesting to see how this plays on non-english systems...
     */
    mFileSettingsAction->setText (tr ("&Preferences...", "global settings"));
#else
    /*
     * ...and on other platforms we use "Preferences" as well. The #ifdef is
     * left because of the possible localization problems on Mac we first need
     * to figure out.
     */
    mFileSettingsAction->setText (tr ("&Preferences...", "global settings"));
#endif
    mFileSettingsAction->setShortcut (QKeySequence ("Ctrl+G"));
    mFileSettingsAction->setStatusTip (tr ("Display the global settings dialog"));

    mFileExitAction->setText (tr ("E&xit"));
    mFileExitAction->setShortcut (QKeySequence ("Ctrl+Q"));
    mFileExitAction->setStatusTip (tr ("Close application"));

    mVmNewAction->setText (tr ("&New..."));
    mVmNewAction->setShortcut (QKeySequence ("Ctrl+N"));
    mVmNewAction->setStatusTip (tr ("Create a new virtual machine"));
    mVmNewAction->setToolTip (mVmNewAction->text().remove ('&').remove ('.') +
        QString (" (%1)").arg (mVmNewAction->shortcut().toString()));

    mVmConfigAction->setText (tr ("&Settings..."));
    mVmConfigAction->setShortcut (QKeySequence ("Ctrl+S"));
    mVmConfigAction->setStatusTip (tr ("Configure the selected virtual machine"));
    mVmConfigAction->setToolTip (mVmConfigAction->text().remove ('&').remove ('.') +
        QString (" (%1)").arg (mVmConfigAction->shortcut().toString()));

    mVmDeleteAction->setText (tr ("&Delete"));
    mVmDeleteAction->setShortcut (QKeySequence ("Ctrl+R"));
    mVmDeleteAction->setStatusTip (tr ("Delete the selected virtual machine"));

    /* Note: mVmStartAction text is set up in vmListViewCurrentChanged() */

    mVmDiscardAction->setText (tr ("D&iscard"));
    mVmDiscardAction->setStatusTip (
        tr ("Discard the saved state of the selected virtual machine"));

    mVmPauseAction->setText (tr ("&Pause"));
    mVmPauseAction->setStatusTip (
        tr ("Suspend the execution of the virtual machine"));

    mVmRefreshAction->setText (tr ("Re&fresh"));
    mVmRefreshAction->setStatusTip (
        tr ("Refresh the accessibility state of the selected virtual machine"));

    mVmShowLogsAction->setText (tr ("Show &Log..."));
    mVmShowLogsAction->setIconText (tr ("Log", "icon text"));
    mVmShowLogsAction->setShortcut (QKeySequence ("Ctrl+L"));
    mVmShowLogsAction->setStatusTip (
        tr ("Show the log files of the selected virtual machine"));

    mHelpActions.retranslateUi();

#ifdef Q_WS_MAC
    mFileMenu->setTitle (tr("&File", "Mac OS X version"));
#else /* Q_WS_MAC */
    mFileMenu->setTitle (tr("&File", "Non Mac OS X version"));
#endif /* !Q_WS_MAC */
    mVMMenu->setTitle (tr ("&Machine"));
    mHelpMenu->setTitle (tr ("&Help"));

#ifdef VBOX_GUI_WITH_SYSTRAY
    if (vboxGlobal().isTrayMenu())
    {
        mTrayIcon->retranslateUi();
        mTrayIcon->refresh();
    }
#endif

#ifdef QT_MAC_USE_COCOA
    /* There is a bug in Qt Cocoa which result in showing a "more arrow" when
       the necessary size of the toolbar is increased. Also for some languages
       the with doesn't match if the text increase. So manually adjust the size
       after changing the text. */
    mVMToolBar->updateLayout();
#endif /* QT_MAC_USE_COCOA */
}


// Private members
/////////////////////////////////////////////////////////////////////////////

//
// Private slots
/////////////////////////////////////////////////////////////////////////////

void VBoxSelectorWnd::vmListViewCurrentChanged (bool aRefreshDetails,
                                                bool aRefreshSnapshots,
                                                bool aRefreshDescription)
{
    UIVMItem *item = mVMListView->selectedItem();

    if (item && item->accessible())
    {
        CMachine m = item->machine();

        KMachineState state = item->state();
        bool running = item->sessionState() != KSessionState_Closed;
        bool modifyEnabled = !running && state != KMachineState_Saved;

        if (aRefreshDetails)
            m_pVMDesktop->updateDetails(item, m);
        if (aRefreshSnapshots)
            m_pVMDesktop->updateSnapshots(item, m);
        if (aRefreshDescription)
            m_pVMDesktop->updateDescription(item, m);

        /* enable/disable modify actions */
        mVmConfigAction->setEnabled (modifyEnabled);
        mVmDeleteAction->setEnabled (modifyEnabled);
        mVmDiscardAction->setEnabled (state == KMachineState_Saved && !running);
        mVmPauseAction->setEnabled (   state == KMachineState_Running
                                    || state == KMachineState_Teleporting
                                    || state == KMachineState_LiveSnapshotting
                                    || state == KMachineState_Paused
                                    || state == KMachineState_TeleportingPausedVM /** @todo Live Migration: does this make sense? */
                                   );

        /* change the Start button text accordingly */
        if (   state == KMachineState_PoweredOff
            || state == KMachineState_Saved
            || state == KMachineState_Teleported
            || state == KMachineState_Aborted
           )
        {
            mVmStartAction->setText (tr ("S&tart"));
#ifdef QT_MAC_USE_COCOA
            /* There is a bug in Qt Cocoa which result in showing a "more arrow" when
               the necessary size of the toolbar is increased. Also for some languages
               the with doesn't match if the text increase. So manually adjust the size
               after changing the text. */
            mVMToolBar->updateLayout();
#endif /* QT_MAC_USE_COCOA */
            mVmStartAction->setStatusTip (
                tr ("Start the selected virtual machine"));

            mVmStartAction->setEnabled (!running);
        }
        else
        {
            mVmStartAction->setText (tr ("S&how"));
#ifdef QT_MAC_USE_COCOA
            /* There is a bug in Qt Cocoa which result in showing a "more arrow" when
               the necessary size of the toolbar is increased. Also for some languages
               the with doesn't match if the text increase. So manually adjust the size
               after changing the text. */
            mVMToolBar->updateLayout();
#endif /* QT_MAC_USE_COCOA */
            mVmStartAction->setStatusTip (
                tr ("Switch to the window of the selected virtual machine"));

            mVmStartAction->setEnabled (item->canSwitchTo());
        }

        /* change the Pause/Resume button text accordingly */
        if (   state == KMachineState_Paused
            || state == KMachineState_TeleportingPausedVM /*?*/
           )
        {
            mVmPauseAction->setText (tr ("R&esume"));
            mVmPauseAction->setShortcut (QKeySequence ("Ctrl+P"));
            mVmPauseAction->setStatusTip (
                tr ("Resume the execution of the virtual machine"));
            mVmPauseAction->blockSignals (true);
            mVmPauseAction->setChecked (true);
            mVmPauseAction->blockSignals (false);
        }
        else
        {
            mVmPauseAction->setText (tr ("&Pause"));
            mVmPauseAction->setShortcut (QKeySequence ("Ctrl+P"));
            mVmPauseAction->setStatusTip (
                tr ("Suspend the execution of the virtual machine"));
            mVmPauseAction->blockSignals (true);
            mVmPauseAction->setChecked (false);
            mVmPauseAction->blockSignals (false);
        }

        /* disable Refresh for accessible machines */
        mVmRefreshAction->setEnabled (false);

        /* enable the show log item for the selected vm */
        mVmShowLogsAction->setEnabled (true);
    }
    else
    {
        /* Note that the machine becomes inaccessible (or if the last VM gets
         * deleted), we have to update all fields, ignoring input
         * arguments. */

        if (item)
        {
            /* the VM is inaccessible */
            m_pVMDesktop->updateDetailsErrorText(
                VBoxProblemReporter::formatErrorInfo(item->accessError()));
            mVmRefreshAction->setEnabled(true);
        }
        else
        {
            /* default HTML support in Qt is terrible so just try to get
             * something really simple */
            m_pVMDesktop->updateDetailsText(
                tr("<h3>"
                   "Welcome to VirtualBox!</h3>"
                   "<p>The left part of this window is  "
                   "a list of all virtual machines on your computer. "
                   "The list is empty now because you haven't created any virtual "
                   "machines yet."
                   "<img src=:/welcome.png align=right/></p>"
                   "<p>In order to create a new virtual machine, press the "
                   "<b>New</b> button in the main tool bar located "
                   "at the top of the window.</p>"
                   "<p>You can press the <b>%1</b> key to get instant help, "
                   "or visit "
                   "<a href=http://www.virtualbox.org>www.virtualbox.org</a> "
                   "for the latest information and news.</p>").arg (QKeySequence (QKeySequence::HelpContents).toString (QKeySequence::NativeText)));
            mVmRefreshAction->setEnabled(false);
        }

        /* empty and disable other tabs */
        m_pVMDesktop->updateSnapshots(0, CMachine());
        m_pVMDesktop->updateDescription(0, CMachine());

        /* disable modify actions */
        mVmConfigAction->setEnabled (false);
        mVmDeleteAction->setEnabled (item != NULL);
        mVmDiscardAction->setEnabled (false);
        mVmPauseAction->setEnabled (false);

        /* change the Start button text accordingly */
        mVmStartAction->setText (tr ("S&tart"));
        mVmStartAction->setStatusTip (
            tr ("Start the selected virtual machine"));
        mVmStartAction->setEnabled (false);

        /* disable the show log item for the selected vm */
        mVmShowLogsAction->setEnabled (false);
    }
}

void VBoxSelectorWnd::mediumEnumStarted()
{
    /* refresh the current details to pick up hard disk sizes */
    vmListViewCurrentChanged (true /* aRefreshDetails */);
}

void VBoxSelectorWnd::mediumEnumFinished (const VBoxMediaList &list)
{
    /* refresh the current details to pick up hard disk sizes */
    vmListViewCurrentChanged (true /* aRefreshDetails */);

    /* we warn about inaccessible media only once (after media emumeration
     * started from main() at startup), to avoid annoying the user */
    if (   mDoneInaccessibleWarningOnce
#ifdef VBOX_GUI_WITH_SYSTRAY
        || vboxGlobal().isTrayMenu()
#endif
       )
        return;

    mDoneInaccessibleWarningOnce = true;

    do
    {
        /* ignore the signal if a modal widget is currently active (we won't be
         * able to properly show the modeless VDI manager window in this case) */
        if (QApplication::activeModalWidget())
            break;

        /* ignore the signal if a VBoxMediaManagerDlg window is active */
        if (qApp->activeWindow() &&
            !strcmp (qApp->activeWindow()->metaObject()->className(), "VBoxMediaManagerDlg"))
            break;

        /* look for at least one inaccessible media */
        VBoxMediaList::const_iterator it;
        for (it = list.begin(); it != list.end(); ++ it)
            if ((*it).state() == KMediumState_Inaccessible)
                break;

        if (it != list.end() && vboxProblem().remindAboutInaccessibleMedia())
        {
            /* Show the VDM dialog but don't refresh once more after a
             * just-finished refresh */
            VBoxMediaManagerDlg::showModeless (this, false /* aRefresh */);
        }
    }
    while (0);
}

void VBoxSelectorWnd::machineStateChanged(QString strId, KMachineState /* state */)
{
#ifdef VBOX_GUI_WITH_SYSTRAY
    if (vboxGlobal().isTrayMenu())
    {
        /* Check if there are some machines alive - else quit, since
         * we're not needed as a systray menu anymore. */
        if (vboxGlobal().mainWindowCount() == 0)
        {
            fileExit();
            return;
        }
    }
#endif

    refreshVMItem (strId,
                   false /* aDetails */,
                   false /* aSnapshots */,
                   false /* aDescription */);

    /* simulate a state change signal */
    m_pVMDesktop->updateDescriptionState();
}

void VBoxSelectorWnd::machineDataChanged(QString strId)
{
    refreshVMItem (strId,
                   true  /* aDetails */,
                   false /* aSnapshots */,
                   true  /* aDescription */);
}

void VBoxSelectorWnd::machineRegistered(QString strId, bool fRegistered)
{
    if (fRegistered)
    {
        CVirtualBox vbox = vboxGlobal().virtualBox();
        CMachine m = vbox.GetMachine (strId);
        if (!m.isNull())
        {
            mVMModel->addItem (new UIVMItem (m));
            mVMModel->sort();
            /* Make sure the description, ... pages are properly updated.
             * Actualy we haven't call the next method, but unfortunately Qt
             * seems buggy if the new item is on the same position as the
             * previous one. So go on the safe side and call this by our self. */
            vmListViewCurrentChanged();
        }
        /* m.isNull() is ok (theoretically, the machine could have been
         * already deregistered by some other client at this point) */
    }
    else
    {
        UIVMItem *item = mVMModel->itemById (strId);
        if (item)
        {
            int row = mVMModel->rowById (item->id());
            mVMModel->removeItem (item);
            delete item;
            mVMListView->ensureSomeRowSelected (row);
        }

        /* item = 0 is ok (if we originated this event then the item
         * has been already removed) */
    }
}

void VBoxSelectorWnd::sessionStateChanged(QString strId, KSessionState /* state */)
{
    refreshVMItem (strId,
                   true  /* aDetails */,
                   false /* aSnapshots */,
                   false /* aDescription */);

    /* simulate a state change signal */
    m_pVMDesktop->updateDescriptionState();
}

void VBoxSelectorWnd::snapshotChanged(QString strId, QString /* strSnapshotId */)
{
    refreshVMItem (strId,
                   false /* aDetails */,
                   true  /* aSnapshot */,
                   false /* aDescription */);
}

#ifdef VBOX_GUI_WITH_SYSTRAY

void VBoxSelectorWnd::mainWindowCountChanged(int count)
{
    if (vboxGlobal().isTrayMenu() && count <= 1)
        fileExit();
}

void VBoxSelectorWnd::trayIconCanShow(bool fEnabled)
{
    emit trayIconChanged(VBoxChangeTrayIconEvent (vboxGlobal().settings().trayIconEnabled()));
}

void VBoxSelectorWnd::trayIconShow(bool fEnabled)
{
    if (vboxGlobal().isTrayMenu() && mTrayIcon)
        mTrayIcon->trayIconShow(fEnabled);
}

void VBoxSelectorWnd::trayIconChanged(bool fEnabled)
{
    /* Not used yet. */
}

VBoxTrayIcon::VBoxTrayIcon (VBoxSelectorWnd* aParent, UIVMItemModel* aVMModel)
{
    mParent = aParent;
    mVMModel = aVMModel;

    mShowSelectorAction = new QAction (this);
    Assert (mShowSelectorAction);
    mShowSelectorAction->setIcon(UIIconPool::iconSet(":/VirtualBox_16px.png"));

    mHideSystrayMenuAction = new QAction (this);
    Assert (mHideSystrayMenuAction);
    mHideSystrayMenuAction->setIcon(UIIconPool::iconSet(":/exit_16px.png"));

    /* reuse parent action data */

    mVmConfigAction = new QAction (this);
    Assert (mVmConfigAction);
    mVmConfigAction->setIcon (mParent->vmConfigAction()->icon());

    mVmDeleteAction = new QAction (this);
    Assert (mVmDeleteAction);
    mVmDeleteAction->setIcon (mParent->vmDeleteAction()->icon());

    mVmStartAction = new QAction (this);
    Assert (mVmStartAction);
    mVmStartAction->setIcon (mParent->vmStartAction()->icon());

    mVmDiscardAction = new QAction (this);
    Assert (mVmDiscardAction);
    mVmDiscardAction->setIcon (mParent->vmDiscardAction()->icon());

    mVmPauseAction = new QAction (this);
    Assert (mVmPauseAction);
    mVmPauseAction->setCheckable (true);
    mVmPauseAction->setIcon (mParent->vmPauseAction()->icon());

    mVmRefreshAction = new QAction (this);
    Assert (mVmRefreshAction);
    mVmRefreshAction->setIcon (mParent->vmRefreshAction()->icon());

    mVmShowLogsAction = new QAction (this);
    Assert (mVmConfigAction);
    mVmShowLogsAction->setIcon (mParent->vmShowLogsAction()->icon());

    mTrayIconMenu = new QMenu (aParent);
    Assert (mTrayIconMenu);

    setIcon (QIcon (":/VirtualBox_16px.png"));
    setContextMenu (mTrayIconMenu);

    connect (mShowSelectorAction, SIGNAL (triggered()), mParent, SLOT (showWindow()));
    connect (mHideSystrayMenuAction, SIGNAL (triggered()), this, SLOT (trayIconShow()));
}

VBoxTrayIcon::~VBoxTrayIcon ()
{
    /* Erase dialog handle in config file. */
    if (mActive)
    {
        vboxGlobal().virtualBox().SetExtraData (VBoxDefs::GUI_TrayIconWinID,
                                                QString::null);
        hide();
    }
}

void VBoxTrayIcon::retranslateUi ()
{
    if (!mActive)
        return;

    mShowSelectorAction->setText (tr ("Show Selector Window"));
    mShowSelectorAction->setStatusTip (tr (
        "Show the selector window assigned to this menu"));

    mHideSystrayMenuAction->setText (tr ("Hide Tray Icon"));
    mHideSystrayMenuAction->setStatusTip (tr (
        "Remove this icon from the system tray"));

    /* reuse parent action data */

    mVmConfigAction->setText (mParent->vmConfigAction()->text());
    mVmConfigAction->setStatusTip (mParent->vmConfigAction()->statusTip());

    mVmDeleteAction->setText (mParent->vmDeleteAction()->text());
    mVmDeleteAction->setStatusTip (mParent->vmDeleteAction()->statusTip());

    mVmPauseAction->setText (mParent->vmPauseAction()->text());
    mVmPauseAction->setStatusTip (mParent->vmPauseAction()->statusTip());

    mVmDiscardAction->setText (mParent->vmDiscardAction()->text());
    mVmDiscardAction->setStatusTip (mParent->vmDiscardAction()->statusTip());

    mVmShowLogsAction->setText (mParent->vmShowLogsAction()->text());
    mVmShowLogsAction->setStatusTip (mParent->vmShowLogsAction()->statusTip());
}

void VBoxTrayIcon::showSubMenu ()
{
    if (!mActive)
        return;

    UIVMItem* pItem = NULL;
    QMenu *pMenu = NULL;
    QVariant vID;

    if ((pMenu = qobject_cast<QMenu*>(sender())))
    {
        vID = pMenu->menuAction()->data();
        if (vID.canConvert<QString>() && mVMModel)
            pItem = mVMModel->itemById (qvariant_cast<QString>(vID));
    }

    mVmConfigAction->setData (vID);
    mVmDeleteAction->setData (vID);
    mVmDiscardAction->setData (vID);
    mVmStartAction->setData (vID);
    mVmPauseAction->setData (vID);
    mVmShowLogsAction->setData (vID);

    if (pItem && pItem->accessible())
    {
        /* look at vmListViewCurrentChanged() */
        CMachine m = pItem->machine();
        KMachineState s = pItem->state();
        bool running = pItem->sessionState() != KSessionState_Closed;
        bool modifyEnabled = !running && s != KMachineState_Saved;

        /* Settings */
        mVmConfigAction->setEnabled (modifyEnabled);

        /* Delete */
        mVmDeleteAction->setEnabled (modifyEnabled);

        /* Discard */
        mVmDiscardAction->setEnabled (s == KMachineState_Saved && !running);

        /* Change the Start button text accordingly */
        if (   s == KMachineState_PoweredOff
            || s == KMachineState_Saved
            || s == KMachineState_Teleported
            || s == KMachineState_Aborted
           )
        {
            mVmStartAction->setText (UIVMListView::tr ("S&tart"));
            mVmStartAction->setStatusTip (
                  UIVMListView::tr ("Start the selected virtual machine"));
            mVmStartAction->setEnabled (!running);
        }
        else
        {
            mVmStartAction->setText (UIVMListView::tr ("S&how"));
            mVmStartAction->setStatusTip (
                  UIVMListView::tr ("Switch to the window of the selected virtual machine"));
            mVmStartAction->setEnabled (pItem->canSwitchTo());
        }

        /* Change the Pause/Resume button text accordingly */
        mVmPauseAction->setEnabled (   s == KMachineState_Running
                                    || s == KMachineState_Teleporting
                                    || s == KMachineState_LiveSnapshotting
                                    || s == KMachineState_Paused
                                    || s == KMachineState_TeleportingPausedVM
                                   );

        if (   s == KMachineState_Paused
            || s == KMachineState_TeleportingPausedVM /*?*/
           )
        {
            mVmPauseAction->setText (UIVMListView::tr ("R&esume"));
            mVmPauseAction->setStatusTip (
                  UIVMListView::tr ("Resume the execution of the virtual machine"));
            mVmPauseAction->blockSignals (true);
            mVmPauseAction->setChecked (true);
            mVmPauseAction->blockSignals (false);
        }
        else
        {
            mVmPauseAction->setText (UIVMListView::tr ("&Pause"));
            mVmPauseAction->setStatusTip (
                  UIVMListView::tr ("Suspend the execution of the virtual machine"));
            mVmPauseAction->blockSignals (true);
            mVmPauseAction->setChecked (false);
            mVmPauseAction->blockSignals (false);
        }

        mVmShowLogsAction->setEnabled (true);

        /* Disconnect old slot which maybe was connected from another selected sub menu. */
        disconnect (mVmConfigAction, SIGNAL (triggered()), this, SLOT (vmSettings()));
        disconnect (mVmDeleteAction, SIGNAL (triggered()), this, SLOT (vmDelete()));
        disconnect (mVmDiscardAction, SIGNAL (triggered()), this, SLOT (vmDiscard()));
        disconnect (mVmStartAction, SIGNAL (triggered()), this, SLOT (vmStart()));
        disconnect (mVmPauseAction, SIGNAL (toggled (bool)), this, SLOT (vmPause (bool)));
        disconnect (mVmShowLogsAction, SIGNAL (triggered()), this, SLOT (vmShowLogs()));

        /* Connect new sub menu with slots. */
        connect (mVmConfigAction, SIGNAL (triggered()), this, SLOT (vmSettings()));
        connect (mVmDeleteAction, SIGNAL (triggered()), this, SLOT (vmDelete()));
        connect (mVmDiscardAction, SIGNAL (triggered()), this, SLOT (vmDiscard()));
        connect (mVmStartAction, SIGNAL (triggered()), this, SLOT (vmStart()));
        connect (mVmPauseAction, SIGNAL (toggled (bool)), this, SLOT (vmPause (bool)));
        connect (mVmShowLogsAction, SIGNAL (triggered()), this, SLOT (vmShowLogs()));
    }
    else    /* Item is not accessible. */
    {
        mVmConfigAction->setEnabled (false);
        mVmDeleteAction->setEnabled (pItem != NULL);
        mVmDiscardAction->setEnabled (false);
        mVmPauseAction->setEnabled (false);

        /* Set the Start button text accordingly. */
        mVmStartAction->setText (UIVMListView::tr ("S&tart"));
        mVmStartAction->setStatusTip (
              UIVMListView::tr ("Start the selected virtual machine"));
        mVmStartAction->setEnabled (false);

        /* Disable the show log item for the selected vm. */
        mVmShowLogsAction->setEnabled (false);
    }

    /* Build sub menu entries (add rest of sub menu entries later here). */
    pMenu->addAction (mVmStartAction);
    pMenu->addAction (mVmPauseAction);
}

void VBoxTrayIcon::hideSubMenu ()
{
    if (!mActive)
        return;

    UIVMItem* pItem = NULL;
    QVariant vID;

    if (QMenu *pMenu = qobject_cast<QMenu*>(sender()))
    {
        vID = pMenu->menuAction()->data();
        if (vID.canConvert<QString>() && mVMModel)
            pItem = mVMModel->itemById (qvariant_cast<QString>(vID));
    }

    /* Nothing to do here yet. */

    Assert (pItem);
}

void VBoxTrayIcon::refresh ()
{
    if (!mActive)
        return;

    AssertReturnVoid (mVMModel);
    AssertReturnVoid (mTrayIconMenu);

    mTrayIconMenu->clear();

    UIVMItem* pItem = NULL;
    QMenu* pCurMenu = mTrayIconMenu;
    QMenu* pSubMenu = NULL;

    int iCurItemCount = 0;

    mTrayIconMenu->addAction (mShowSelectorAction);
    mTrayIconMenu->setDefaultAction (mShowSelectorAction);

    if (mVMModel->rowCount() > 0)
        mTrayIconMenu->addSeparator();

    for (int i = 0; i < mVMModel->rowCount(); i++, iCurItemCount++)
    {
        pItem = mVMModel->itemByRow(i);
        Assert(pItem);

        if (iCurItemCount > 10) /* 10 machines per sub menu. */
        {
            pSubMenu = new QMenu (tr ("&Other Machines...", "tray menu"));
            Assert (pSubMenu);
            pCurMenu->addMenu (pSubMenu);
            pCurMenu = pSubMenu;
            iCurItemCount = 0;
        }

        pSubMenu = new QMenu (QString ("&%1. %2")
                              .arg ((iCurItemCount + 1) % 100).arg (pItem->name()));
        Assert (pSubMenu);
        pSubMenu->setIcon (pItem->sessionStateIcon());

        QAction *pAction = NULL;
        QVariant vID;
        vID.setValue (pItem->id());

        pSubMenu->menuAction()->setData (vID);
        connect (pSubMenu, SIGNAL (aboutToShow()), this, SLOT (showSubMenu()));
        connect (pSubMenu, SIGNAL (aboutToHide()), this, SLOT (hideSubMenu()));
        pCurMenu->addMenu (pSubMenu);
    }

    if (mVMModel->rowCount() > 0)
        mTrayIconMenu->addSeparator();

    mTrayIconMenu->addAction (mHideSystrayMenuAction);

    /* We're done constructing the menu, show it */
    setVisible (true);
}

UIVMItem* VBoxTrayIcon::GetItem (QObject* aObject)
{
    UIVMItem* pItem = NULL;
    if (QAction *pAction = qobject_cast<QAction*>(sender()))
    {
        QVariant v = pAction->data();
        if (v.canConvert<QString>() && mVMModel)
            pItem = mVMModel->itemById (qvariant_cast<QString>(v));
    }

    Assert (pItem);
    return pItem;
}

void VBoxTrayIcon::trayIconShow (bool aShow)
{
    if (!vboxGlobal().isTrayMenu())
        return;

    mActive = aShow;
    if (mActive)
    {
        refresh();
        retranslateUi();
    }
    setVisible (mActive);

    if (!mActive)
        mParent->fileExit();
}

void VBoxTrayIcon::vmSettings()
{
    UIVMItem* pItem = GetItem (sender());
    mParent->vmSettings (NULL, NULL, pItem->id());
}

void VBoxTrayIcon::vmDelete()
{
    UIVMItem* pItem = GetItem (sender());
    mParent->vmDelete (pItem->id());
}

void VBoxTrayIcon::vmStart()
{
    UIVMItem* pItem = GetItem (sender());
    mParent->vmStart (pItem->id());
}

void VBoxTrayIcon::vmDiscard()
{
    UIVMItem* pItem = GetItem (sender());
    mParent->vmDiscard (pItem->id());
}

void VBoxTrayIcon::vmPause(bool aPause)
{
    UIVMItem* pItem = GetItem (sender());
    mParent->vmPause (aPause, pItem->id());
}

void VBoxTrayIcon::vmRefresh()
{
    UIVMItem* pItem = GetItem (sender());
    mParent->vmRefresh (pItem->id());
}

void VBoxTrayIcon::vmShowLogs()
{
    UIVMItem* pItem = GetItem (sender());
    mParent->vmShowLogs (pItem->id());
}

#endif // VBOX_GUI_WITH_SYSTRAY

void VBoxSelectorWnd::sltDownloaderUserManualEmbed()
{
    /* If there is User Manual downloader created => show the process bar: */
    if (UIDownloaderUserManual *pDl = UIDownloaderUserManual::current())
        statusBar()->addWidget(pDl->processWidget(this), 0);
}

void VBoxSelectorWnd::showViewContextMenu(const QPoint &pos)
{
    CVirtualBox vbox = vboxGlobal().virtualBox();
    QString strToolbar = vbox.GetExtraData(VBoxDefs::GUI_Toolbar);
    QString strStatusbar = vbox.GetExtraData(VBoxDefs::GUI_Statusbar);
    bool fToolbar = strToolbar.isEmpty() || strToolbar == "true";
    bool fStatusbar = strStatusbar.isEmpty() || strStatusbar == "true";

    QList<QAction*> actions;
    QAction *pShowToolBar = new QAction(tr("Show Toolbar"), 0);
    pShowToolBar->setCheckable(true);
    pShowToolBar->setChecked(fToolbar);
    actions << pShowToolBar;
    QAction *pShowStatusBar = new QAction(tr("Show Statusbar"), 0);
    pShowStatusBar->setCheckable(true);
    pShowStatusBar->setChecked(fStatusbar);
    actions << pShowStatusBar;

    QPoint gpos = pos;
    QWidget *pSender = static_cast<QWidget*>(sender());
    if (pSender)
        gpos = pSender->mapToGlobal(pos);
    QAction *pResult = QMenu::exec(actions, gpos);
    if (pResult == pShowToolBar)
    {
        if (pResult->isChecked())
        {
#ifdef Q_WS_MAC
            mVMToolBar->show();
#else /* Q_WS_MAC */
            m_pBar->show();
#endif /* !Q_WS_MAC */
            vbox.SetExtraData(VBoxDefs::GUI_Toolbar, "true");
        }
        else
        {
#ifdef Q_WS_MAC
            mVMToolBar->hide();
#else /* Q_WS_MAC */
            m_pBar->hide();
#endif /* !Q_WS_MAC */
            vbox.SetExtraData(VBoxDefs::GUI_Toolbar, "false");
        }
    }else if (pResult == pShowStatusBar)
    {
        if (pResult->isChecked())
        {
            statusBar()->show();
            vbox.SetExtraData(VBoxDefs::GUI_Statusbar, "true");
        }
        else
        {
            statusBar()->hide();
            vbox.SetExtraData(VBoxDefs::GUI_Statusbar, "false");
        }
    }
}
