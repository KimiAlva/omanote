#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>
#include "theme.h"
#include "editor.h"
#include "findreplace.h"

class QMenu;
class QTimer;
class QFileSystemWatcher;
class QDockWidget;
class QTreeView;
class QFileSystemModel;
class MinimapWidget;

class MinimapWidget : public QWidget {
    Q_OBJECT
public:
    explicit MinimapWidget(QWidget *parent = nullptr);
    void setEditor(Editor *ed);
protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
private slots:
    void refresh();
private:
    Editor *m_editor = nullptr;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    void openFile(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newFile();
    void open();
    void save();
    void saveAs();
    void closeTab(int index);
    void currentChanged(int index);
    void applyTheme();
    void showFindReplace();
    void zoomIn();
    void zoomOut();
    void toggleWrap();
    void about();

private:
    Editor *currentEditor() const;
    bool maybeSave(Editor *ed);
    bool saveFile(Editor *ed, const QString &path);
    void addTab(Editor *ed, const QString &title);
    void updateTitle();
    void updateStatus();
    QString detectLanguage(const QString &path) const;
    void setEditorLanguage(Editor *ed, const QString &path);
    void applySettingsToEditor(Editor *ed);
    void loadSettings();
    void saveSettings();
    void updateRecentFilesMenu();
    void addToRecentFiles(const QString &path);
    void showPreferences();
    void printDocument();
    void ensureTrailingNewline(Editor *ed);

    QTabWidget *m_tabs;
    Theme *m_theme;
    FindReplaceDialog *m_findDlg = nullptr;
    QLabel *m_status;
    bool m_wrap = false;
    int m_wrapMode = 0; // 0 none,1 word,2 char,3 word-char
    int m_fontSize = 12;

    // Xed-like settings
    bool m_lineNumbers = true;
    bool m_highlightCurrentLine = true;
    bool m_bracketMatching = false;
    bool m_rightMargin = false;
    int m_rightMarginColumn = 120;
    bool m_drawWhitespace = false;
    bool m_autoBracketCompletion = true;
    bool m_trimTrailingSpaces = false;
    bool m_syntaxHighlighting = true;
    bool m_autoIndent = false;
    bool m_insertSpaces = true;
    int m_tabSize = 4;
    bool m_restoreCursorPosition = true;
    bool m_ensureTrailingNewline = true;
    bool m_autosave = false;
    int m_autosaveInterval = 10;
    bool m_createBackup = false;
    int m_smartHomeEnd = 2; // 0 disabled,1 before,2 after,3 always
    bool m_closeLastTab = false;
    bool m_searchHighlighting = true;
    QString m_language;
    QStringList m_recentFiles;
    QTimer *m_autosaveTimer = nullptr;
    QMenu *m_recentMenu = nullptr;

    QDockWidget *m_sideDock = nullptr;
    QDockWidget *m_bottomDock = nullptr;
    QDockWidget *m_minimapDock = nullptr;
    MinimapWidget *m_minimap = nullptr;
    QTreeView *m_fileTree = nullptr;
    QFileSystemModel *m_fileModel = nullptr;
    QLabel *m_docInfo = nullptr;
    QFileSystemWatcher *m_fileWatcher = nullptr;
    QString m_watchedFile;
};
