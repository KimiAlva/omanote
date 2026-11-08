#include <QApplication>
#include <QCommandLineParser>
#include <QTranslator>
#include <QSettings>
#include <QLocale>
#include <QDir>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("note");
    app.setApplicationDisplayName("Note");
    app.setApplicationVersion("0.2.0");
    app.setDesktopFileName("note");
    app.setOrganizationName("omarchy");

    QSettings s("omarchy", "note");
    QString lang = s.value("language", QLocale::system().name()).toString();

    // Qt base translations for standard buttons/dialogs
    QTranslator qtTranslator;
    QString qtLang = lang.split('_').first();
    if (qtTranslator.load("qtbase_" + qtLang, "/usr/share/qt6/translations"))
        app.installTranslator(&qtTranslator);

    QTranslator translator;
    QStringList paths = {
        app.applicationDirPath(),
        "/usr/share/note/translations",
        "/home/varo/Work/note/build"
    };
    for (const QString &p : paths) {
        if (translator.load("note_" + lang, p) ||
            translator.load("note_" + lang.split('_').first(), p)) {
            app.installTranslator(&translator);
            break;
        }
    }

    QCommandLineParser parser;
    parser.setApplicationDescription("Note - a lightweight text editor");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "File to open", "[file]");
    parser.process(app);

    MainWindow w;
    w.show();

    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) w.openFile(args.first());

    return app.exec();
}