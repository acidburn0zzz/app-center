#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QString distribution;
    QString metadataRepo;
    QString cacheFolder;
    bool localtestMetadata = false;
    QList<int> lastPage;
    QStringList installPackageUpdateList;
    bool updatesAvailable = false;

    void categoryPushButtonReleased();
    void queryUpdates();
    void getUpdates();


private slots:
    void on_searchForUpdatesPushButton_released();
    void on_backPushButton_1_released();
    void on_backPushButton_2_released();
    void on_backPushButton_3_released();
    void on_backPushButton_4_released();
    void on_installNowPushButton_2_released();

    void on_backPushButton_5_released();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
