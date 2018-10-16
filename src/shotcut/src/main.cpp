/*
 * Copyright (c) 2011-2016 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
//#include <PythonQt.h>
//#include <gui/PythonQtScriptingConsole.h>


#include <QtWidgets>
#include <QtGlobal>
#include "mainwindow.h"
#include "settings.h"
#include <Logger.h>
#include <FileAppender.h>
#include <ConsoleAppender.h>
#include <QSysInfo>
#include <QProcess>
#include <QCommandLineParser>
#include <framework/mlt_log.h>
#include "securitybookmark/transport_security_bookmark.h"
#include "qmltypes/qmlutilities.h"
 #include <QQmlEngine>
#include <QProcess>
#include <registrationchecker.h>

#include "maincontroller.h"


#ifdef Q_OS_WIN
extern "C"
{
    // Inform the driver we could make use of the discrete gpu
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

static void mlt_log_handler(void *service, int mlt_level, const char *format, va_list args)
{
    if (mlt_level > mlt_log_get_level())
        return;

    enum Logger::LogLevel cuteLoggerLevel = Logger::Fatal;
    switch (mlt_level) {
    case MLT_LOG_DEBUG:
        cuteLoggerLevel = Logger::Trace;
        break;
    case MLT_LOG_ERROR:
    case MLT_LOG_FATAL:
    case MLT_LOG_PANIC:
        cuteLoggerLevel = Logger::Error;
        break;
    case MLT_LOG_INFO:
        cuteLoggerLevel = Logger::Info;
        break;
    case MLT_LOG_VERBOSE:
        cuteLoggerLevel = Logger::Debug;
        break;
    case MLT_LOG_WARNING:
        cuteLoggerLevel = Logger::Warning;
        break;
    }
    QString message;
    mlt_properties properties = service? MLT_SERVICE_PROPERTIES((mlt_service) service) : NULL;
    if (properties) {
        char *mlt_type = mlt_properties_get(properties, "mlt_type");
        char *service_name = mlt_properties_get(properties, "mlt_service");
        char *resource = mlt_properties_get(properties, "resource");
        if (!resource || resource[0] != '<' || resource[strlen(resource) - 1] != '>')
            mlt_type = mlt_properties_get(properties, "mlt_type" );
        if (service_name)
            message = QString("[%1 %2] ").arg(mlt_type).arg(service_name);
        else
            message = QString().sprintf("[%s %p] ", mlt_type, service);
        if (resource)
            message.append(QString("\"%1\" ").arg(resource));
        message.append(QString().vsprintf(format, args));
        message.replace('\n', "");
    } else {
        message = QString().vsprintf(format, args);
        message.replace('\n', "");
    }
    Logger::write(cuteLoggerLevel, __FILE__, __LINE__, "MLT", message);
}

class Application : public QApplication
{
public:
    MainWindow* mainWindow;
    QTranslator qtTranslator;
    QTranslator qtBaseTranslator;
    QTranslator shotcutTranslator;
    QString resourceArg;
    bool isFullScreen;

    Application(int &argc, char **argv)
        : QApplication(argc, argv)
    {
        QDir dir(applicationDirPath());
        dir.cd("lib");
        dir.cd("qt5");
        addLibraryPath(dir.absolutePath());


        setOrganizationName("etinysoft");
        setOrganizationDomain("etinysoft.com");
#if MOVIEMATOR_PRO
        setApplicationName("MovieMator Pro");
#else
        setApplicationName("MovieMator");
#endif
        setApplicationVersion(SHOTCUT_VERSION);
        setAttribute(Qt::AA_UseHighDpiPixmaps);
        setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
#if defined(Q_OS_MAC)
        setAttribute(Qt::AA_DontShowIconsInMenus);
#endif


        // Startup logging.
        dir = QStandardPaths::standardLocations(QStandardPaths::DataLocation).first();
        if (!dir.exists()) dir.mkpath(dir.path());


        const QString logFileName = dir.filePath("moviemator-log.txt");
        QFile::remove(logFileName);
        FileAppender* fileAppender = new FileAppender(logFileName);
        fileAppender->setFormat("[%-7l] <%c> %m\n");
        Logger::registerAppender(fileAppender);
#ifndef NDEBUG
        // Only log to console in dev debug builds.
        ConsoleAppender* consoleAppender = new ConsoleAppender();
        consoleAppender->setFormat(fileAppender->format());
        Logger::registerAppender(consoleAppender);

        mlt_log_set_level(MLT_LOG_VERBOSE);
//        mlt_log_set_level(MLT_LOG_QUIET);
#else
        mlt_log_set_level(MLT_LOG_VERBOSE);
#endif
        mlt_log_set_callback(mlt_log_handler);

        // Log some basic info.
        LOG_INFO() << "Starting MovieMator version" << SHOTCUT_VERSION;
#if defined (Q_OS_WIN)
        LOG_INFO() << "Windows version" << QSysInfo::windowsVersion();
#elif defined(Q_OS_MAC)
        LOG_INFO() << "OS X version" << QSysInfo::macVersion();
#else
        LOG_INFO() << "Linux version";
#endif
        LOG_INFO() << "number of logical cores =" << QThread::idealThreadCount();
        LOG_INFO() << "locale =" << QLocale();
        LOG_INFO() << "install dir =" <<  applicationDirPath();

#if defined(Q_OS_WIN)
        if (Settings.playerGPU()) {
            QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
        } else if (Settings.drawMethod() >= Qt::AA_UseDesktopOpenGL &&
                   Settings.drawMethod() <= Qt::AA_UseSoftwareOpenGL) {
            QCoreApplication::setAttribute(Qt::ApplicationAttribute(Settings.drawMethod()));
        }
#endif
        // Load translations
        QString locale = Settings.language();
        dir = applicationDirPath();
    #if defined(Q_OS_MAC)
        dir.cdUp();
        dir.cd("Resources");
        dir.cd("translations");
    #elif defined(Q_OS_WIN)
        dir.cd("share");
        dir.cd("translations");
    #else
        dir.cdUp();
        dir.cd("share");
        dir.cd("moviemator");
        dir.cd("translations");
    #endif
        if (locale.startsWith("pt_"))
            locale = "pt";
        if (qtTranslator.load("qt_" + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
            installTranslator(&qtTranslator);
        else if (qtTranslator.load("qt_" + locale, dir.absolutePath()))
            installTranslator(&qtTranslator);
        if (qtBaseTranslator.load("qtbase_" + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
            installTranslator(&qtBaseTranslator);
        else if (qtBaseTranslator.load("qtbase_" + locale, dir.absolutePath()))
            installTranslator(&qtBaseTranslator);
        if (shotcutTranslator.load("mm_" + Settings.language(), dir.absolutePath()))
            installTranslator(&shotcutTranslator);
        else
            qDebug()<<"failed to load translator file";

        qDebug()<<"mm_"<<Settings.language() << dir.absolutePath();

        QCommandLineParser parser;
        parser.addHelpOption();
        parser.addVersionOption();
        QCommandLineOption fullscreenOption("fullscreen",
            QCoreApplication::translate("main", "Fill the screen with the MovieMator window."));
        parser.addOption(fullscreenOption);
        QCommandLineOption gpuOption("gpu",
            QCoreApplication::translate("main", "Use GPU processing."));
        parser.addOption(gpuOption);
        parser.addPositionalArgument("resource",
            QCoreApplication::translate("main", "A file to open."));


        QCommandLineOption pythonFileOption(QStringList() << "pythonfile" << "python-file",
                                            QCoreApplication::translate("main", "Execute Python file"),
                                            QCoreApplication::translate("main", "python"));
        parser.addOption(pythonFileOption);

        parser.process(arguments());
        isFullScreen = parser.isSet(fullscreenOption);
        if (parser.isSet(gpuOption))
            Settings.setPlayerGPU(true);


        if (!parser.positionalArguments().isEmpty())
            resourceArg = parser.positionalArguments().first();

        QString pythonFile = parser.value(pythonFileOption);


    }

    ~Application()
    {
        delete mainWindow;
        LOG_DEBUG() << "exiting";
    }

protected:
    bool event(QEvent *event) {
        if (event->type() == QEvent::FileOpen) {
            QFileOpenEvent *openEvent = static_cast<QFileOpenEvent*>(event);
            resourceArg = openEvent->file();
            return true;
        }
        else return QApplication::event(event);
    }
};

int main(int argc, char **argv)
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    QCoreApplication::setAttribute(Qt::AA_X11InitThreads);
    QCoreApplication::addLibraryPath("./lib");
#endif

    setenv("QT_DEVICE_PIXEL_RATIO", "auto", 1);
    setenv("QT_SCALE_FACTOR", "2", 1);
    Application a(argc, argv);


    //copy qml files
    QDir dir = QStandardPaths::standardLocations(QStandardPaths::DataLocation).first();
    QmlUtilities::sharedEngine()->addImportPath(dir.path());
    QDir appDir(qApp->applicationDirPath());
    appDir.cdUp();
    appDir.cd("Resources");
    //QmlUtilities::sharedEngine()->addImportPath(appDir.path());

    QString dstArchivePath = dir.path().append("/resource.zip");
    QFile::remove(dstArchivePath);
   // QFile::copy(appDir.path().append("/resource"), dstArchivePath);
    QFile::copy(appDir.path().append("/resource.zip"), dstArchivePath);
    QStringList args;
    args << "-o";
    args << dstArchivePath;
    args << "-d";
    args << dir.path();
    QProcess unzip;

    QDir appDir1(qApp->applicationDirPath());
    QString unzip_path = appDir1.path().append("/unzip.exe");
    //unzip.start("/usr/bin/unzip", args);
    unzip.start(unzip_path, args);

    unzip.waitForFinished();

    appDir.cd("share");
    appDir.cd("mlt");
    setenv("MLT_DATA", appDir.path().toUtf8().constData(), 1);


    QSplashScreen splash(QPixmap(""));
    splash.showMessage(QCoreApplication::translate("main", "Loading plugins..."), Qt::AlignHCenter | Qt::AlignBottom);
    splash.show();

  //  resolve_security_bookmark();
    Registration.readAndCheckRegistrationInfo();

    a.setProperty("system-style", a.style()->objectName());

    QFile file(":/src/qss/mainstylesheet.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QString::fromLatin1(file.readAll());
    qApp->setStyleSheet(styleSheet);

    QFontDatabase::addApplicationFont(":/fonts/DigifaceWide.ttf");

    MainWindow::changeTheme("dark");//Settings.theme());

    a.mainWindow = &MAIN;
    a.mainWindow->show();
    a.mainWindow->move ((QApplication::desktop()->width() - a.mainWindow->width())/2,(QApplication::desktop()->height() - a.mainWindow->height())/2);
    a.mainWindow->setFullScreen(a.isFullScreen);


    splash.finish(a.mainWindow);


  //  QMessageBox::information(NULL, "Title", a.resourceArg, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    if (!a.resourceArg.isEmpty())
        a.mainWindow->open(a.resourceArg);
    else
        a.mainWindow->open(a.mainWindow->untitledFileName());


    a.mainWindow->createMultitrackModelIfNeeded();



#ifdef MOVIEMATOR_FREE
//    if (Settings.firstUse())
//    {
//        a.mainWindow->showUpgradeToProPromptDialog();
//    }

    QDateTime lastUse = Settings.lastUse();
    QDateTime currentTime = QDateTime::currentDateTime();
    if (lastUse.isNull())
    {
        a.mainWindow->showUpgradeToProPromptDialog();
    }
    else
    {

        int days = lastUse.daysTo(currentTime);
        if (days > 1)
        {
            a.mainWindow->showUpgradeToProPromptDialog();
        }
    }
    Settings.setLastUse(currentTime);
#endif


//    MAINCONTROLLER.initPythonQt();
//    PythonQtObjectPtr mainContext = PythonQt::self()->getMainModule();
//    PythonQtScriptingConsole console(NULL, mainContext);
//    console.show();

    int result = a.exec();

//    if (EXIT_RESTART == result) {
//        LOG_DEBUG() << "restarting app";
//        QProcess* restart = new QProcess;
//        restart->start(a.applicationFilePath(), QStringList());
//        restart->waitForReadyRead();
//        restart->waitForFinished(10000);
//        result = EXIT_SUCCESS;
//    }

    return result;
}