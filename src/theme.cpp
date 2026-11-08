#include "theme.h"
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QDir>
#include <cctype>
#include <QStandardPaths>

static QString colorsPath() {
    QString home = QDir::homePath();
    return home + "/.local/state/omarchy/current/theme/colors.toml";
}

Theme::Theme(QObject *parent) : QObject(parent) {
    m_colorsPath = colorsPath();
    // defaults
    m_colors["accent"] = QColor("#FFD60A");
    m_colors["background"] = QColor("#1e1e2e");
    m_colors["foreground"] = QColor("#cdd6f4");
    m_colors["selection"] = QColor("#353543");
    m_colors["muted"] = QColor("#45475a");
    m_accent = m_colors["accent"];
    m_background = m_colors["background"];
    m_foreground = m_colors["foreground"];
    m_selection = m_colors["selection"];
    m_muted = m_colors["muted"];
    readColors();
    readRadius();
    armWatcher();
    m_poll.setInterval(2000);
    connect(&m_poll, &QTimer::timeout, this, [this]() { readColors(); readRadius(); });
    m_poll.start();
}

void Theme::armWatcher() {
    m_watcher.removePaths(m_watcher.files());
    m_watcher.removePaths(m_watcher.directories());
    QFileInfo fi(m_colorsPath);
    if (fi.exists()) {
        m_watcher.addPath(fi.absoluteFilePath());
        m_watcher.addPath(fi.absolutePath());
    }
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &) {
        // Theme switch replaces the file/symlink; re-arm after a short delay.
        QTimer::singleShot(100, this, [this]() { readColors(); readRadius(); armWatcher(); });
    }, Qt::UniqueConnection);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString &) {
        QTimer::singleShot(100, this, [this]() { readColors(); readRadius(); armWatcher(); });
    }, Qt::UniqueConnection);
}

void Theme::refresh() { readColors(); readRadius(); }

void Theme::readColors() {
    QFile f(m_colorsPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // fallback already set
        return;
    }
    QTextStream in(&f);
    bool dirty = false;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;
        int eq = line.indexOf('=');
        if (eq < 0) continue;
        QString k = line.left(eq).trimmed();
        QString v = line.mid(eq + 1).trimmed();
        v.remove('"');
        QColor c(v);
        if (!c.isValid()) continue;
        if (!m_colors.contains(k) || m_colors[k] != c) {
            m_colors[k] = c;
            dirty = true;
        }
    }
    // sync convenience members
    m_accent = m_colors.value("accent", m_accent);
    m_background = m_colors.value("background", m_background);
    m_foreground = m_colors.value("foreground", m_foreground);
    m_selection = m_colors.value("selection", m_selection);
    m_muted = m_colors.value("muted", m_muted);
    if (dirty) emit changed();
}

QColor Theme::color(const QString &name) const {
    return m_colors.value(name);
}

void Theme::readRadius() {
    QProcess p;
    p.start("hyprctl", {"getoption", "decoration:rounding", "-j"});
    if (!p.waitForFinished(2000)) return;
    QByteArray out = p.readAllStandardOutput();
    // parse "int": N
    int idx = out.indexOf("\"int\"");
    if (idx < 0) idx = out.indexOf("int");
    // find number after colon
    int colon = out.indexOf(':', idx);
    if (colon < 0) return;
    int i = colon + 1;
    while (i < out.size() && (out[i] == ' ')) i++;
    int start = i;
    while (i < out.size() && std::isdigit(static_cast<unsigned char>(out[i]))) i++;
    if (i == start) return;
    int val = out.mid(start, i - start).toInt();
    if (val != m_radius) { m_radius = val; emit changed(); }
}
