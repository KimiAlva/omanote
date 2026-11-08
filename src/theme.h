#pragma once
#include <QObject>
#include <QColor>
#include <QMap>
#include <QFileSystemWatcher>
#include <QTimer>

// Follows the Omarchy accent color and Hyprland's window rounding live.
class Theme : public QObject {
    Q_OBJECT
    Q_PROPERTY(QColor accent READ accent NOTIFY changed)
    Q_PROPERTY(int radius READ radius NOTIFY changed)
    Q_PROPERTY(QColor background READ background NOTIFY changed)
    Q_PROPERTY(QColor foreground READ foreground NOTIFY changed)
    Q_PROPERTY(QColor selection READ selection NOTIFY changed)
    Q_PROPERTY(QColor muted READ muted NOTIFY changed)
public:
    explicit Theme(QObject *parent = nullptr);

    QColor accent() const { return m_accent; }
    int radius() const { return m_radius; }
    QColor background() const { return m_background; }
    QColor foreground() const { return m_foreground; }
    QColor selection() const { return m_selection; }
    QColor muted() const { return m_muted; }
    QColor color(const QString &name) const;

    void refresh();

signals:
    void changed();

private:
    void readColors();
    void readRadius();
    void armWatcher();

    QColor m_accent = QColor("#FFD60A");
    QColor m_background = QColor("#1e1e2e");
    QColor m_foreground = QColor("#cdd6f4");
    QColor m_selection = QColor("#353543");
    QColor m_muted = QColor("#45475a");
    int m_radius = 8;

    QFileSystemWatcher m_watcher;
    QTimer m_poll;
    QString m_colorsPath;
    QMap<QString,QColor> m_colors;
};
