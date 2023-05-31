#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QFileDialog>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QTimer>
#include <QMessageBox>
#include <QTextDocument>
#include <QTextBlock>
#include <QMenu>
#include <QDesktopServices>
#include <QToolBar>
#include <stdlib.h>
#ifdef Q_OS_WIN
#include "windows.h"
#include "winnt.h"
#include <win/dirent/dirent.h>
#else
    #include <dirent.h>
#endif
#include <QDebug>
#include "appvars.h"
#include "codeeditor.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QDir appDir;

private slots:
    void on_btnGo_clicked();
    void on_btnDir_clicked();
    void checkUpdates();
    void on_btnAbout_clicked();
    void on_btnSave_clicked();
    void on_listResults_cellDoubleClicked(int row, int column);
    void listResultsContextMenu(const QPoint& pos);

private:
    Ui::MainWindow *ui;
    int fileCount;
    int matchCount;
    bool stop;
    void createFileList(QString mask, QString path, bool recurse);
    void scanFile(QString filename);
    void addFind(int line, QString filename, QString content, QString matchedword);
    void LoadHistory();
    void SaveHistory();
    void ResetTable();
    void SetControlsEnabled(bool b);
    QLabel *labelUpdate;
};

#endif // MAINWINDOW_H
