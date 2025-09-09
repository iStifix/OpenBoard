/*
 * Copyright (C) 2015-2022 Département de l'Instruction Publique (DIP-SEM)
 *
 * Copyright (C) 2013 Open Education Foundation
 *
 * Copyright (C) 2010-2013 Groupement d'Intérêt Public pour
 * l'Education Numérique en Afrique (GIP ENA)
 *
 * This file is part of OpenBoard.
 *
 * OpenBoard is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License,
 * with a specific linking exception for the OpenSSL project's
 * "OpenSSL" library (or with modified versions of it that use the
 * same license as the "OpenSSL" library).
 *
 * OpenBoard is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBoard. If not, see <http://www.gnu.org/licenses/>.
 */




#include <QtGui>
#include <QSysInfo>
#include <QSettings>
#include <QDir>
#include <QSurfaceFormat>
#include <QPixmapCache>

#include "frameworks/UBPlatformUtils.h"
#include "frameworks/UBFileSystemUtils.h"

#include "UBApplication.h"
#include "UBSettings.h"

/* Uncomment this for memory leaks detection */
/*
#if defined(WIN32) && defined(_DEBUG)
     #define _CRTDBG_MAP_ALLOC
     #include <stdlib.h>
     #include <crtdbg.h>
     #define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
     #define new DEBUG_NEW
#endif
*/

void ub_message_output(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    // We must temporarily remove the handler to avoid the infinite recursion of
    // ub_message_output -> qt_message_output -> ub_message_output -> qt_message_output ...
    QtMessageHandler previousHandler = qInstallMessageHandler(0);

#if defined(QT_NO_DEBUG)
    // Suppress qDebug output in release builds
    if (type != QtDebugMsg)
    {
        qt_message_output(type, context, msg);
    }

#else
    // Default output in debug builds
    qt_message_output(type, context, msg);
#endif

    if (UBApplication::app() && UBApplication::app()->isVerbose()) {
        QString logFileNamePath = UBSettings::userDataDirectory() + "/log/"+ qApp->applicationName() + ".log";
        QFile logFile(logFileNamePath);

        if (logFile.exists() && logFile.size() > 10000000)
            logFile.remove();

        if (logFile.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&logFile);
            out << QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
                << "      " << msg << "\n";
            logFile.close();
        }
    }

    qInstallMessageHandler(previousHandler);
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
    #if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 2))
        qputenv("QT_MEDIA_BACKEND", "ffmpeg");
    #endif
#endif

    // Uncomment next section to have memory leaks information
    // tracing in VC++ debug mode under Windows
/*
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
*/

    // QT_NO_GLIB=1 is set by default on Linux, and prevents media playback
    if (qEnvironmentVariableIsSet("QT_NO_GLIB"))
        qunsetenv("QT_NO_GLIB");

    Q_INIT_RESOURCE(OpenBoard);

    qInstallMessageHandler(ub_message_output);

    // HighDPI and GL defaults before app construction
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    // Keep crisp rendering under fractional scaling on Wayland/KDE
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    {
        const QByteArray glDefaults = qgetenv("OPENBOARD_GL_DEFAULTS");
        const bool setGlDefaults = !(glDefaults == "0" || glDefaults.compare("false", Qt::CaseInsensitive) == 0);
        if (setGlDefaults) {
            QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
            fmt.setRenderableType(QSurfaceFormat::OpenGL);
            fmt.setVersion(3, 3);
            fmt.setProfile(QSurfaceFormat::CoreProfile);
            bool ok = false;
            int swapInt = qEnvironmentVariableIntValue("OPENBOARD_GL_SWAP_INTERVAL", &ok);
            if (ok) fmt.setSwapInterval(swapInt);
            QSurfaceFormat::setDefaultFormat(fmt);
        }
    }
#endif

    // Mesa driver hint: enable GL threading to reduce CPU overhead (safe on AMD/Radeon/Mesa)
    if (!qEnvironmentVariableIsSet("MESA_GLTHREAD"))
        qputenv("MESA_GLTHREAD", QByteArray("true"));

    // Helper to append a Chromium flag once to QTWEBENGINE_CHROMIUM_FLAGS
    auto appendChromiumFlag = [](const QByteArray &flag) {
        QByteArray current = qgetenv("QTWEBENGINE_CHROMIUM_FLAGS");
        if (!current.contains(flag)) {
            if (!current.isEmpty())
                current.append(' ');
            current.append(flag);
            qputenv("QTWEBENGINE_CHROMIUM_FLAGS", current);
        }
    };

    // Disable the Chromium sandbox to improve compatibility on systems where it
    // is not available (e.g., running as root or certain ARM builds).
    appendChromiumFlag("--no-sandbox");
    appendChromiumFlag("--ozone-platform-hint=auto");
    // Keep ozone auto-detection; no hard preference set here

    // Optional safety switches for problematic GPU stacks.
    // You can use config (Perf/WebEngine) or env vars.
    bool forceSoftware = qEnvironmentVariableIntValue("OPENBOARD_FORCE_SOFTWARE") == 1;
    bool enableGpuOverride = qEnvironmentVariableIntValue("OPENBOARD_ENABLE_GPU") == 1;
#ifdef Q_OS_LINUX
    // Try to read user config before app construction
    {
        const QString userCfgPath = QDir::homePath() + "/.local/share/OpenBoard/OpenBoardUser.config";
        QSettings userCfg(userCfgPath, QSettings::IniFormat);
        QVariant vForce = userCfg.value("Perf/WebEngine/ForceSoftware", QVariant());
        if (vForce.isValid()) forceSoftware = vForce.toBool();
        QVariant vEnable = userCfg.value("Perf/WebEngine/EnableGPU", QVariant());
        if (vEnable.isValid()) enableGpuOverride = vEnable.toBool();
    }
#endif
    const QString arch = QSysInfo::currentCpuArchitecture();

    const bool isArmArch = arch.contains("arm", Qt::CaseInsensitive) || arch.contains("aarch64", Qt::CaseInsensitive);

    if (forceSoftware) {
        // Prefer Qt's software OpenGL implementation for stability.
        QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
        qputenv("QT_OPENGL", "software");
        // Also hint Chromium to avoid GPU usage in its process.
        appendChromiumFlag("--disable-gpu");
        appendChromiumFlag("--disable-gpu-compositing");
    } else if (isArmArch && !enableGpuOverride) {
        // On ARM/aarch64, some driver stacks (e.g. certain Mesa/AMDGPU combos)
        // can crash in the GPU process. Prefer disabling GPU for WebEngine.
        appendChromiumFlag("--disable-gpu");
        appendChromiumFlag("--disable-gpu-compositing");
    }

    // Share OpenGL contexts across threads to avoid GPU related crashes when
    // QtWebEngine creates its internal rendering contexts.
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    
    bool hasProcessFlag = false;

    for (int i = 1; i < argc; ++i)
    {
        QString arg = QString::fromLocal8Bit(argv[i]);

        if (arg == "--single-process" || arg == "--process-per-site")
         {
             hasProcessFlag = true;
             break;
         }
    }

    std::vector<const char*> argv_vector(argv, argv + argc);

    /*
    * https://stackoverflow.com/questions/43372267/how-to-append-a-value-to-the-array-of-command-line-arguments
    */
    if (!hasProcessFlag)
    {
        // add --process-per-site flag
        argv_vector.push_back("--process-per-site");
        argv_vector.push_back(nullptr);
        argv = const_cast<char**>(argv_vector.data());
        argc++;
    }

    UBApplication app("OpenBoard", argc, argv);

    // Increase pixmap cache to reduce re-rasterization on large boards
    {
        QVariant cfg = UBSettings::settings()->value("Perf/PixmapCacheMB", QVariant());
        int cacheMb = 256;
        if (cfg.isValid()) {
            cacheMb = cfg.toInt();
        } else {
            bool ok = false;
            int envMb = qEnvironmentVariableIntValue("OPENBOARD_PIXMAP_CACHE_MB", &ok);
            if (ok && envMb > 0) cacheMb = envMb;
        }
        QPixmapCache::setCacheLimit(cacheMb * 1024);
    }

    QStringList args = app.arguments();

    QString dumpPath = UBSettings::userDataDirectory() + "/log";
    QDir logDir(dumpPath);
    if (!logDir.exists())
        logDir.mkdir(dumpPath);

    QString fileToOpen;

    if (args.size() > 2) {
        // On Windows/Linux first argument is the file that has been double clicked.
        // On Mac OSX we use FileOpen QEvent to manage opening file in current instance. So we will never
        // have file to open as a parameter on OSX.

        // loop through arguments and skip flags (starting with '-')
        for (int i = 1; i < args.size(); ++i)
        {
            QFile f(args[i]);

            if (f.exists()) {
                fileToOpen = args[i];

                if (app.sendMessage(UBSettings::appPingMessage.toUtf8(), 20000)) {
                    app.sendMessage(fileToOpen.toUtf8(), 1000000);
                    return 0;
                }

                break;
            }
        }
    }

    int result = 0;
    if (app.isPrimary())
    {
        qDebug() << "file name argument" << fileToOpen;
        result = app.exec(fileToOpen);
    }

    app.cleanup();

    qDebug() << "application is quitting";

    return result;
}
