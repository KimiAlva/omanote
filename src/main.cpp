#include <QApplication>
#include <QCommandLineParser>
#include <QTranslator>
#include <QSettings>
#include <QLocale>
#include <QDir>
#include <QFile>
#include <QTimer>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("omanote");
    app.setApplicationDisplayName("Omanote");
    app.setApplicationVersion("0.3.0");
    app.setDesktopFileName("omanote");
    app.setOrganizationName("omarchy");

    // migrate old settings file
    QSettings oldSettings("omarchy", "note");
    QSettings newSettings("omarchy", "omanote");
    if (!newSettings.contains("geometry") && oldSettings.contains("geometry")) {
        for (const QString &key : oldSettings.allKeys())
            newSettings.setValue(key, oldSettings.value(key));
    }

    QString lang = newSettings.value("language", QLocale::system().name()).toString();

    // Qt base translations
    QTranslator qtTranslator;
    QString qtLang = lang.split('_').first();
    if (qtTranslator.load("qtbase_" + qtLang, "/usr/share/qt6/translations"))
        app.installTranslator(&qtTranslator);

    QTranslator translator;
    QStringList paths = {
        app.applicationDirPath(),
        "/usr/share/omanote/translations",
        "./build"
    };
    for (const QString &p : paths) {
        if (translator.load("omanote_" + lang, p) ||
            translator.load("omanote_" + lang.split('_').first(), p)) {
            app.installTranslator(&translator);
            break;
        }
    }

    QCommandLineParser parser;
    parser.setApplicationDescription("Omanote - a lightweight text editor for Omarchy");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "File to open", "[file]");
    parser.addOption(QCommandLineOption("screenshot", "Save a screenshot to <file> and exit", "file"));
    parser.process(app);

    MainWindow w;
    w.show();

    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) w.openFile(args.first());

    if (parser.isSet("screenshot")) {
        QTimer::singleShot(1500, &w, [&](){
            w.grab().save(parser.value("screenshot"));
            app.quit();
        });
    }

    return app.exec();
}