/*****************************************************************************
 * qt4.cpp : QT4 interface
 ****************************************************************************
 * Copyright © 2006-2008 the VideoLAN team
 * $Id$
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Jean-Baptiste Kempf <jb@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QDate>

#include "qt4.hpp"
#include "dialogs_provider.hpp"
#include "input_manager.hpp"
#include "main_interface.hpp"
#include "dialogs/help.hpp" /* update */

#ifdef HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#endif

#include "../../../share/vlc32x32.xpm"
#include "../../../share/vlc32x32-christmas.xpm"

/*****************************************************************************
 * Local prototypes.
 *****************************************************************************/
static int  Open         ( vlc_object_t * );
static void Close        ( vlc_object_t * );
static int  OpenDialogs  ( vlc_object_t * );
static void Run          ( intf_thread_t * );
static void Init         ( intf_thread_t * );
static void ShowDialog   ( intf_thread_t *, int, int, intf_dialog_args_t * );

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
#define ADVANCED_PREFS_TEXT N_( "Show advanced prefs over simple ones" )
#define ADVANCED_PREFS_LONGTEXT N_( "Show advanced preferences and not simple "\
                                    "preferences when opening the preferences "\
                                    "dialog." )

#define SYSTRAY_TEXT N_( "Systray icon" )
#define SYSTRAY_LONGTEXT N_( "Show an icon in the systray " \
                             "allowing you to control VLC media player " \
                             "for basic actions" )

#define MINIMIZED_TEXT N_( "Start VLC with only a systray icon" )
#define MINIMIZED_LONGTEXT N_( "When you launch VLC with that option, " \
                               "VLC will start with just an icon in" \
                               "your taskbar" )

#define TITLE_TEXT N_( "Show playing item name in window title" )
#define TITLE_LONGTEXT N_( "Show the name of the song or video in the " \
                           "controler window title" )

#define FILEDIALOG_PATH_TEXT N_( "Path to use in openfile dialog" )

#define NOTIFICATION_TEXT N_( "Show notification popup on track change" )
#define NOTIFICATION_LONGTEXT N_( \
    "Show a notification popup with the artist and track name when " \
    "the current playlist item changes, when VLC is minimized or hidden." )

#define ADVANCED_OPTIONS_TEXT N_( "Advanced options" )
#define ADVANCED_OPTIONS_LONGTEXT N_( "Show all the advanced options " \
                                      "in the dialogs" )

#define OPACITY_TEXT N_( "Windows opacity between 0.1 and 1." )
#define OPACITY_LONGTEXT N_( "Sets the windows opacity between 0.1 and 1 " \
                             "for main interface, playlist and extended panel." \
                             " This option only works with Windows and " \
                             "X11 with composite extensions." )

#define ERROR_TEXT N_( "Show unimportant error and warnings dialogs" )

#define UPDATER_TEXT N_( "Activate the updates availability notification" )
#define UPDATER_LONGTEXT N_( "Activate the automatic notification of new " \
                            "versions of the software. It runs once a week." )
#define UPDATER_DAYS_TEXT N_("Number of days between two update checks")

#define COMPLETEVOL_TEXT N_( "Allow the volume to be set to 400%" )
#define COMPLETEVOL_LONGTEXT N_( "Allow the volume to have range from 0% to " \
                                 "400%, instead of 0% to 200%. This option " \
                                 "can distort the audio, since it uses " \
                                 "software amplification." )

#define SAVEVOL_TEXT N_( "Automatically save the volume on exit" )

#define BLING_TEXT N_( "Use non native buttons and volume slider" )

#define PRIVACY_TEXT N_( "Ask for network policy at start" )


#define VIEWDETAIL_TEXT N_( "Show the opening dialog view in detail mode" )

#define QT_MODE_TEXT N_( "Selection of the starting mode and look " )
#define QT_MODE_LONGTEXT N_( "Start VLC with:\n" \
                             " - normal mode\n"  \
                             " - a zone always present to show information" \
                                  "as lyrics, album arts...\n" \
                             " - minimal mode with limited controls" )

#define QT_NORMAL_MODE_TEXT N_( "Classic look" )
#define QT_ALWAYS_VIDEO_MODE_TEXT N_( "Complete look with information area" )
#define QT_MINIMAL_MODE_TEXT N_( "Minimal look with no menus" )

static int i_mode_list[] =
    { QT_NORMAL_MODE, QT_ALWAYS_VIDEO_MODE, QT_MINIMAL_MODE };
static const char *psz_mode_list_text[] =
    { QT_NORMAL_MODE_TEXT, QT_ALWAYS_VIDEO_MODE_TEXT, QT_MINIMAL_MODE_TEXT };

vlc_module_begin();
    set_shortname( (char *)"Qt" );
    set_description( (char*)_("Qt interface") );
    set_category( CAT_INTERFACE ) ;
    set_subcategory( SUBCAT_INTERFACE_MAIN );
    set_capability( "interface", 151 );
    set_callbacks( Open, Close );

    add_shortcut("qt");

    add_submodule();
        set_description( "Dialogs provider" );
        set_capability( "dialogs provider", 51 );

        add_integer( "qt-display-mode", QT_NORMAL_MODE, NULL,
                     QT_MODE_TEXT, QT_MODE_LONGTEXT, VLC_FALSE );
            change_integer_list( i_mode_list, psz_mode_list_text, 0 );

        add_bool( "qt-notification", VLC_TRUE, NULL, NOTIFICATION_TEXT,
                  NOTIFICATION_LONGTEXT, VLC_FALSE );

        add_float_with_range( "qt-opacity", 1., 0.1, 1., NULL, OPACITY_TEXT,
                  OPACITY_LONGTEXT, VLC_FALSE );
        add_bool( "qt-blingbling", VLC_TRUE, NULL, BLING_TEXT,
                  BLING_TEXT, VLC_FALSE );

        add_bool( "qt-system-tray", VLC_TRUE, NULL, SYSTRAY_TEXT,
                SYSTRAY_LONGTEXT, VLC_FALSE);
        add_bool( "qt-start-minimized", VLC_FALSE, NULL, MINIMIZED_TEXT,
                MINIMIZED_LONGTEXT, VLC_TRUE);
        add_bool( "qt-name-in-title", VLC_TRUE, NULL, TITLE_TEXT,
                  TITLE_LONGTEXT, VLC_FALSE );

        add_bool( "qt-volume-complete", VLC_FALSE, NULL, COMPLETEVOL_TEXT,
                COMPLETEVOL_LONGTEXT, VLC_TRUE );
        add_bool( "qt-autosave-volume", VLC_FALSE, NULL, SAVEVOL_TEXT,
                SAVEVOL_TEXT, VLC_TRUE );
        add_string( "qt-filedialog-path", NULL, NULL, FILEDIALOG_PATH_TEXT,
                FILEDIALOG_PATH_TEXT, VLC_TRUE );
            change_autosave();
            change_internal();

        add_bool( "qt-adv-options", VLC_FALSE, NULL, ADVANCED_OPTIONS_TEXT,
                  ADVANCED_OPTIONS_LONGTEXT, VLC_TRUE );
        add_bool( "qt-advanced-pref", VLC_FALSE, NULL, ADVANCED_PREFS_TEXT,
                ADVANCED_PREFS_LONGTEXT, VLC_FALSE );
        add_bool( "qt-error-dialogs", VLC_TRUE, NULL, ERROR_TEXT,
                ERROR_TEXT, VLC_FALSE );
#ifdef UPDATE_CHECK
        add_bool( "qt-updates-notif", VLC_TRUE, NULL, UPDATER_TEXT,
                UPDATER_LONGTEXT, VLC_FALSE );
        add_integer( "qt-updates-days", 14, NULL, UPDATER_DAYS_TEXT,
                UPDATER_DAYS_TEXT, VLC_FALSE );
#endif

        add_bool( "qt-open-detail", VLC_FALSE, NULL, VIEWDETAIL_TEXT,
                VIEWDETAIL_TEXT, VLC_FALSE );

        add_bool( "qt-privacy-ask", VLC_TRUE, NULL, PRIVACY_TEXT, PRIVACY_TEXT,
                VLC_FALSE );

        set_callbacks( OpenDialogs, Close );
vlc_module_end();

/*****************************************************************************
 * Module callbacks
 *****************************************************************************/
static int Open( vlc_object_t *p_this )
{
    intf_thread_t *p_intf = (intf_thread_t *)p_this;

#if defined Q_WS_X11 && defined HAVE_X11_XLIB_H
    /* Thanks for libqt4 calling exit() in QApplication::QApplication()
     * instead of returning an error, we have to check the X11 display */
    Display *p_display = XOpenDisplay( NULL );
    if( !p_display )
    {
        msg_Err( p_intf, "Could not connect to X server" );
        return VLC_EGENERIC;
    }
    XCloseDisplay( p_display );
#endif

    p_intf->p_sys = (intf_sys_t *)malloc( sizeof( intf_sys_t ) );
    if( !p_intf->p_sys )
    {
        msg_Err( p_intf, "Out of memory" );
        return VLC_ENOMEM;
    }
    memset( p_intf->p_sys, 0, sizeof( intf_sys_t ) );

    p_intf->pf_run = Run;

    p_intf->p_sys->p_playlist = pl_Yield( p_intf );
    p_intf->p_sys->p_sub = msg_Subscribe( p_intf, MSG_QUEUE_NORMAL );

    /* We support play on start */
    p_intf->b_play = VLC_TRUE;

    return VLC_SUCCESS;
}

static int OpenDialogs( vlc_object_t *p_this )
{
    intf_thread_t *p_intf = (intf_thread_t *)p_this;
    int val = Open( p_this );
    if( val )
        return val;

    p_intf->pf_show_dialog = ShowDialog;
    return VLC_SUCCESS;
}

static void Close( vlc_object_t *p_this )
{
    intf_thread_t *p_intf = (intf_thread_t *)p_this;

    vlc_mutex_lock( &p_intf->object_lock );
    p_intf->b_dead = VLC_TRUE;
    vlc_mutex_unlock( &p_intf->object_lock );

    if( p_intf->p_sys->b_isDialogProvider )
    {
        DialogEvent *event = new DialogEvent( INTF_DIALOG_EXIT, 0, NULL );
        QApplication::postEvent( THEDP, static_cast<QEvent*>(event) );

        vlc_thread_join( p_intf );
    }

    vlc_object_release( p_intf->p_sys->p_playlist );
    msg_Unsubscribe( p_intf, p_intf->p_sys->p_sub );
    free( p_intf->p_sys );
}


/*****************************************************************************
 * Initialize the interface or the dialogs provider
 *****************************************************************************/
static void Run( intf_thread_t *p_intf )
{
    if( p_intf->pf_show_dialog )
    {
        if( vlc_thread_create( p_intf, "Qt dialogs", Init, 0, VLC_TRUE ) )
            msg_Err( p_intf, "failed to create Qt dialogs thread" );
    }
    else
        Init( p_intf );
}

static void Init( intf_thread_t *p_intf )
{
    char dummy[] = "";
    char *argv[] = { dummy };
    int argc = 1;

    Q_INIT_RESOURCE( vlc );

#if !defined(WIN32) && !defined(__APPLE__)
    /* KLUDGE:
     * disables icon theme use because that makes Cleanlooks style bug
     * because it asks gconf for some settings that timeout because of threads
     * see commits 21610 21622 21654 for reference */

    /* If you don't have a gconftool-2 binary, you should comment this line */
    QApplication::setDesktopSettingsAware( false );
#endif

    /* Start the QApplication here */
    QApplication *app = new QApplication( argc, argv , true );
    if( QDate::currentDate().dayOfYear() >= 354 )
        app->setWindowIcon( QIcon( QPixmap(vlc_christmas_xpm) ) );
    else
        app->setWindowIcon( QIcon( QPixmap(vlc_xpm) ) );
    p_intf->p_sys->p_app = app;

    // Initialize timers and the Dialog Provider
    DialogsProvider::getInstance( p_intf );

    // Create the normal interface
    if( !p_intf->pf_show_dialog )
    {
        MainInterface *p_mi = new MainInterface( p_intf );
        p_intf->p_sys->p_mi = p_mi;
        /* We don't show it because it is done in the MainInterface constructor
        p_mi->show(); */
        p_intf->p_sys->b_isDialogProvider = false;
    }
    else
    {
        vlc_thread_ready( p_intf );
        p_intf->p_sys->b_isDialogProvider = true;
    }


#ifdef ENABLE_NLS
    // Translation - get locale
    QLocale ql = QLocale::system();
    // Translations for qt's own dialogs
    QTranslator qtTranslator( 0 );
    // Let's find the right path for the translation file
#if !defined( WIN32 )
    QString path =  QString( QT4LOCALEDIR );
#else
    QString path = QString( QString(config_GetDataDir()) + DIR_SEP +
                            "locale" + DIR_SEP );
#endif
    // files depending on locale
    bool b_loaded = qtTranslator.load( path + "qt_" + ql.name());
    if (!b_loaded)
        msg_Dbg( p_intf, "Error while initializing qt-specific localization" );
    app->installTranslator( &qtTranslator );
#endif  //ENABLE_NLS

    /* Start playing if needed */
    if( !p_intf->pf_show_dialog && p_intf->b_play )
    {
        playlist_Control( THEPL, PLAYLIST_AUTOPLAY, VLC_FALSE );
    }

    /* Explain to the core how to show a dialog :D */
    p_intf->pf_show_dialog = ShowDialog;

    /* Last settings */
    app->setQuitOnLastWindowClosed( false );

    /*        retrieve last known path used in file browsing */
    char *psz_path = config_GetPsz( p_intf, "qt-filedialog-path" );
    p_intf->p_sys->psz_filepath = EMPTY_STR( psz_path ) ? psz_path
                           : p_intf->p_libvlc->psz_homedir;

#ifdef UPDATE_CHECK
    if( config_GetInt( p_intf, "qt-updates-notif" ) )
    {
        int interval = config_GetInt( p_intf, "qt-updates-days" );
        QSettings settings( "vlc", "vlc-qt-interface" );
        if( QDate::currentDate() >
                settings.value( "updatedate" ).toDate().addDays( interval ) )
        {
            msg_Dbg( p_intf, "Someone said I need to check updates" );
            /* The constructor of the update Dialog will do the 1st request */
            UpdateDialog::getInstance( p_intf );
            settings.setValue( "updatedate", QDate::currentDate() );
        }
    }
#endif

    /* Launch */
    app->exec();

    /* And quit */

    /* Destroy first the main interface because it is connected to some slots
       in the MainInputManager */
    if( p_intf->p_sys->p_mi ) delete p_intf->p_sys->p_mi;

    /* Destroy then other windows, because some are connected to some slots
       in the MainInputManager */
    DialogsProvider::killInstance();

    /* Destroy the MainInputManager */
    MainInputManager::killInstance();

    delete app;

    config_PutPsz( p_intf, "qt-filedialog-path", p_intf->p_sys->psz_filepath );
    free( psz_path );
}

/*****************************************************************************
 * Callback to show a dialog
 *****************************************************************************/
static void ShowDialog( intf_thread_t *p_intf, int i_dialog_event, int i_arg,
                        intf_dialog_args_t *p_arg )
{
    DialogEvent *event = new DialogEvent( i_dialog_event, i_arg, p_arg );
    QApplication::postEvent( THEDP, static_cast<QEvent*>(event) );
}

/*****************************************************************************
 * PopupMenuCB: callback to show the popupmenu.
 *  We don't show the menu directly here because we don't want the
 *  caller to block for a too long time.
 *****************************************************************************/
static int PopupMenuCB( vlc_object_t *p_this, const char *psz_variable,
                        vlc_value_t old_val, vlc_value_t new_val, void *param )
{
    intf_thread_t *p_intf = (intf_thread_t *)param;
    ShowDialog( p_intf, INTF_DIALOG_POPUPMENU, new_val.b_bool, 0 );
    return VLC_SUCCESS;
}
