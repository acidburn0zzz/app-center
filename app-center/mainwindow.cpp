#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "applicationitemwidget.h"
#include "installitemwidget.h"
#include <QtDebug>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QMessageBox>

//On startup
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //Settings
    QSettings settings;
    if (!(settings.contains("Distribution"))) {
        settings.setValue("Distribution", "NorcuxOS");
    }
    if (!(settings.contains("MetadataRepo"))) {
        settings.setValue("MetadataRepo", "https://codeload.github.com/NorcuxOS/app-center-metadata/tar.gz/master");
    }

    distribution = settings.value("Distribution").toString();
    metadataRepo = settings.value("MetadataRepo").toString();

    //Set icons
    ui->updateManagerLogo_1->setPixmap( QIcon::fromTheme("system-software-update").pixmap(64, 64) );
    ui->updateManagerLogo_2->setPixmap( QIcon::fromTheme("system-software-update").pixmap(64, 64) );

    //Setup cache
    cacheFolder = QString( QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + "/" + QCoreApplication::applicationName() + "/");
    QDir cacheDir = QDir(cacheFolder);
    if ( !cacheDir.exists() ) {
        cacheDir.mkpath(".");
    }

    if (localtestMetadata) {
        QDir localtestMetadataDir = QDir(cacheFolder + distribution + "-app-center-localtest-metadata");
        if ( !localtestMetadataDir.exists() ) {
            localtestMetadataDir.mkpath(".");
        }
    } else {
        QProcess *downloadMetadataCache = new QProcess();
        downloadMetadataCache->setWorkingDirectory(cacheFolder);
        downloadMetadataCache->start("curl", QStringList() << metadataRepo << "--output" << distribution + "-appcenter-metadata.tar.gz");
        //TODO: Use a singal instead
        downloadMetadataCache->waitForFinished(-1);

        QProcess *unpackMetadataCache = new QProcess();
        unpackMetadataCache->setWorkingDirectory(cacheFolder);
        unpackMetadataCache->start("tar", QStringList() << "-xf" << distribution + "-appcenter-metadata.tar.gz" << "--one-top-level=" + distribution + "-app-center-metadata" << "--strip" << "1");
        //TODO: Use a singal instead
        unpackMetadataCache->waitForFinished(-1);
    }

    //Connect category buttons
    QList<QPushButton *> categoryButtons = ui->categoriesFrame->findChildren<QPushButton *>();
    foreach(QPushButton *categoryButton, categoryButtons)
    {
        connect(categoryButton, &QPushButton::released, this, &MainWindow::categoryPushButtonReleased);
    }

    ui->installUpdatesListWidget_2->hide();
}

MainWindow::~MainWindow()
{
    delete ui;
}

//On category button press
void MainWindow::categoryPushButtonReleased()
{
    QObject *senderObj = sender();
    QString senderObjName = senderObj->objectName();
    QString category = senderObjName.split("_").last();
    QString categoryName = senderObj->parent()->findChild<QLabel *>("categoryNameLabel_" + category)->text();

    ui->categoryTitleLabel->setText(categoryName);

    QFile categoryFile;
    if (localtestMetadata) {
        categoryFile.setFileName(cacheFolder + distribution + "-app-center-localtest-metadata/categories/category-" + category + ".txt");
    } else {
        categoryFile.setFileName(cacheFolder + distribution + "-app-center-metadata/categories/category-" + category + ".txt");
    }

    //Load application metadata
    if (categoryFile.open(QIODevice::ReadOnly))
    {
       QTextStream in(&categoryFile);
       while (!in.atEnd())
       {
          QString application = in.readLine();
          if (application != "") {

              QFile applicationMetadataFile;
              if (localtestMetadata) {
                  applicationMetadataFile.setFileName(cacheFolder + distribution + "-app-center-localtest-metadata/applications/" + application + "/metadata.conf");
              } else {
                  applicationMetadataFile.setFileName(cacheFolder + distribution + "-app-center-metadata/applications/" + application + "/metadata.conf");
              }

              if (applicationMetadataFile.open(QIODevice::ReadOnly))
              {
                  QListWidgetItem *applicationListWidgetItem = new QListWidgetItem(ui->applicationListWidget);
                  ui->applicationListWidget->addItem(applicationListWidgetItem);
                  ApplicationItemWidget *applicationItemWidget = new ApplicationItemWidget;
                  applicationListWidgetItem->setSizeHint (applicationItemWidget->sizeHint());
                  ui->applicationListWidget->setItemWidget(applicationListWidgetItem, applicationItemWidget);

                  QString applicationName;
                  QString applicationDeveloper;
                  QString applicationURL;
                  QString applicationThemeIcon;
                  QPixmap applicationIconPixmap;
                  QString applicationSummary;
                  QString applicationDescription;
                  QString applicationPackage;

                  QTextStream in(&applicationMetadataFile);
                  while (!in.atEnd())
                  {
                      QString line = in.readLine();
                      if (line != "") {
                          QString applicationNameKeyword = "NAME=";
                          QString applicationDeveloperKeyword = "DEVELOPER=";
                          QString applicationURLKeyword = "URL=";
                          QString applicationThemeIconKeyword = "THEME_ICON=";
                          QString applicationSummaryKeyword = "SUMMARY=";
                          QString applicationDescriptionKeyword = "DESCRIPTION=";
                          QString applicationPackageKeyword = "PACKAGE=";

                          //Application Name
                          int applicationNamePos = line.indexOf(applicationNameKeyword);
                          if (applicationNamePos >= 0)
                          {
                              applicationName = line.mid(applicationNamePos + applicationNameKeyword.length()).split("\"")[1];
                          }

                          //Application Developer
                          int applicationDeveloperPos = line.indexOf(applicationDeveloperKeyword);
                          if (applicationDeveloperPos >= 0)
                          {
                              applicationDeveloper = line.mid(applicationDeveloperPos + applicationDeveloperKeyword.length()).split("\"")[1];
                          }

                          //Application URL
                          int applicationURLPos = line.indexOf(applicationURLKeyword);
                          if (applicationURLPos >= 0)
                          {
                              applicationURL = line.mid(applicationURLPos + applicationURLKeyword.length()).split("\"")[1];
                          }

                          //Application Theme Icon
                          int applicationThemeIconPos = line.indexOf(applicationThemeIconKeyword);
                          if (applicationThemeIconPos >= 0)
                          {
                              applicationThemeIcon = line.mid(applicationThemeIconPos + applicationThemeIconKeyword.length()).split("\"")[1];
                              if (applicationThemeIcon == "null") {
                                  //Application Icon from Image
                                  if (localtestMetadata) {
                                      applicationIconPixmap = QPixmap( cacheFolder + distribution + "-app-center-localtest-metadata/applications/" + application + "/icon.png" );
                                  } else {
                                      applicationIconPixmap = QPixmap( cacheFolder + distribution + "-app-center-metadata/applications/" + application + "/icon.png" );
                                  }
                              } else {
                                  //Application Icon from Theme
                                  applicationIconPixmap = QIcon::fromTheme(applicationThemeIcon).pixmap(64, 64);
                              }
                          }

                          //Application Summary
                          int applicationSummaryPos = line.indexOf(applicationSummaryKeyword);
                          if (applicationSummaryPos >= 0)
                          {
                              applicationSummary = line.mid(applicationSummaryPos + applicationSummaryKeyword.length()).split("\"")[1];
                          }

                          //Application Description
                          int applicationDescriptionPos = line.indexOf(applicationDescriptionKeyword);
                          if (applicationDescriptionPos >= 0)
                          {
                              applicationDescription = line.mid(applicationDescriptionPos + applicationDescriptionKeyword.length()).split("\"")[1];
                          }

                          //Application Package
                          int applicationPackagePos = line.indexOf(applicationPackageKeyword);
                          if (applicationPackagePos >= 0)
                          {
                              applicationPackage = line.mid(applicationPackagePos + applicationPackageKeyword.length()).split("\"")[1];
                          }
                      }
                  }

                  //Check if application is already installed
                  QProcess *pacmanSearchProcess;
                  QString pacmanSearchProcessProgram = "pacman";
                  QStringList pacmanSearchProcessArguments;
                  pacmanSearchProcessArguments << "-Qs" << "^" + applicationPackage + "$";
                  pacmanSearchProcess = new QProcess();
                  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                  env.insert("LC_ALL", "C");
                  pacmanSearchProcess->setProcessEnvironment(env);

                  pacmanSearchProcess->start(pacmanSearchProcessProgram, pacmanSearchProcessArguments);

                  connect(pacmanSearchProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus){
                      if (exitCode == 0) {
                          applicationItemWidget->setInstalled(true);
                      } else if (exitCode == 1) {
                          applicationItemWidget->setInstalled(false);
                      }
                  });

                  pacmanSearchProcess->waitForFinished(-1);

                  applicationItemWidget->setApplicationInfo(application, applicationName, applicationDeveloper, applicationURL, applicationIconPixmap, applicationSummary, applicationDescription, applicationPackage);

                  //Remove application
                  connect( applicationItemWidget, &ApplicationItemWidget::removeApplication, [=]{
                      QProcess *pacmanRemoveProcess;
                      QString pacmanRemoveProcessProgram = "pkexec";
                      QStringList pacmanRemoveProcessArguments;
                      pacmanRemoveProcessArguments << "pacman" << "-Rs" << "--noconfirm" << applicationPackage;
                      pacmanRemoveProcess = new QProcess();
                      QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                      env.insert("LC_ALL", "C");
                      pacmanRemoveProcess->setProcessEnvironment(env);

                      pacmanRemoveProcess->start(pacmanRemoveProcessProgram, pacmanRemoveProcessArguments);

                      setCursor(Qt::WaitCursor);
                      ui->applicationListWidget->setEnabled(false);
                      ui->backPushButton_1->setEnabled(false);

                      connect(pacmanRemoveProcess, &QProcess::readyReadStandardOutput, [=]{
                          QTextStream(stdout) << pacmanRemoveProcess->readAllStandardOutput();
                      });
                      connect(pacmanRemoveProcess, &QProcess::readyReadStandardError, [=]{
                          QByteArray error = pacmanRemoveProcess->readAllStandardError();
                          QTextStream(stderr) << error;
                      });

                      connect(pacmanRemoveProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus){
                          if (exitCode == 0) {
                              QTextStream(stdout) << "Done.\n";
                              applicationItemWidget->setInstalled(false);
                          } else if (exitCode == 1) {
                              QMessageBox::critical(this, "Error - App Center", "An unknown error occoured!");
                          } else if (exitCode == 127) {
                              QMessageBox::critical(this, "Error - App Center", "You are not authorized!");
                          }

                          setCursor(Qt::ArrowCursor);
                          ui->applicationListWidget->setEnabled(true);
                          ui->backPushButton_1->setEnabled(true);
                      });

                  });

                  //Install application
                  connect( applicationItemWidget, &ApplicationItemWidget::installApplication, [=]{
                      QProcess *pacmanInstallProcess;
                      QString pacmanInstallProcessProgram = "pkexec";
                      QStringList pacmanInstallProcessArguments;
                      pacmanInstallProcessArguments << "pacman" << "-Sy" << "--noconfirm" << applicationPackage;
                      pacmanInstallProcess = new QProcess();
                      QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                      env.insert("LC_ALL", "C");
                      pacmanInstallProcess->setProcessEnvironment(env);

                      pacmanInstallProcess->start(pacmanInstallProcessProgram, pacmanInstallProcessArguments);

                      setCursor(Qt::WaitCursor);
                      ui->applicationListWidget->setEnabled(false);
                      ui->backPushButton_1->setEnabled(false);

                      connect(pacmanInstallProcess, &QProcess::readyReadStandardOutput, [=]{
                          QTextStream(stdout) << pacmanInstallProcess->readAllStandardOutput();
                      });
                      connect(pacmanInstallProcess, &QProcess::readyReadStandardError, [=]{
                          QByteArray error = pacmanInstallProcess->readAllStandardError();
                          QTextStream(stderr) << error;
                      });

                      connect(pacmanInstallProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus){
                          if (exitCode == 0) {
                              QTextStream(stdout) << "Done.\n";
                              applicationItemWidget->setInstalled(true);
                          } else if (exitCode == 1) {
                              QMessageBox::critical(this, "Error - App Center", "An unknown error occoured!");
                          } else if (exitCode == 127) {
                              QMessageBox::critical(this, "Error - App Center", "You are not authorized!");
                          }

                          setCursor(Qt::ArrowCursor);
                          ui->applicationListWidget->setEnabled(true);
                          ui->backPushButton_1->setEnabled(true);                      
                      });

                  });
                  applicationMetadataFile.close();
              }
          }
       }
       categoryFile.close();
    }

    ui->pages->setCurrentIndex(1);
    lastPage.append(1);

    ui->applicationListWidget->setFocus();
}

//Query updates
void MainWindow::queryUpdates()
{
    QProcess *queryUpdatesProcess;
    QString queryUpdatesProcessProgram = "app-center-cli";
    QStringList queryUpdatesProcessArguments;
    queryUpdatesProcessArguments << "query-updates";
    queryUpdatesProcess = new QProcess();
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LC_ALL", "C");
    queryUpdatesProcess->setProcessEnvironment(env);

    queryUpdatesProcess->start(queryUpdatesProcessProgram, queryUpdatesProcessArguments);

    connect(queryUpdatesProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus){
        QString queryUpdatesProcessOutput = queryUpdatesProcess->readAllStandardOutput();

        if (queryUpdatesProcessOutput != "") {
            QString OSUpdate = queryUpdatesProcessOutput.split("\n").first();

            if (OSUpdate != "===== No OS updates are available. =====") {
                updatesAvailable = true;
            }

            QStringList updateList = queryUpdatesProcessOutput.split("\n");
            updateList.removeFirst();
            //Remove empty string from update list
            updateList.removeLast();

            if (!(updateList.isEmpty())) {
                updatesAvailable = true;
            }

            ui->installUpdatesListWidget_2->setEnabled(true);
            ui->backPushButton_3->setEnabled(true);
            if (updatesAvailable) {
                //Add Operating System Update Item
                QListWidgetItem *installListWidgetItem = new QListWidgetItem(ui->installUpdatesListWidget_2);
                ui->installUpdatesListWidget_2->addItem(installListWidgetItem);
                InstallItemWidget *installItemWidget = new InstallItemWidget;
                if (OSUpdate == "===== No OS updates are available. =====") {
                    installItemWidget->setApplicationUpdateInfo("Operating System Updates", QIcon::fromTheme("applications-system").pixmap(64, 64));
                } else {
                    installItemWidget->setApplicationUpdateInfo("Operating System Updates", QIcon::fromTheme("applications-system").pixmap(64, 64), OSUpdate.remove("===== ").remove(" ====="));
                }
                installListWidgetItem->setSizeHint (installItemWidget->sizeHint());
                ui->installUpdatesListWidget_2->setItemWidget (installListWidgetItem, installItemWidget);

                connect( installItemWidget, &InstallItemWidget::showPackageUpdateList, [=]{
                    ui->installUpdatesListWidget_2->clear();
                    foreach (QString update, updateList) {
                        QString package = update.split(" ").at(0);
                        QString oldVersion = update.split(" ").at(1);
                        QString newVersion = update.split(" ").at(3);

                        //If package update is not ignored
                        if (!(update.contains("[ignored]"))) {
                            //Add Package Update Item
                            QListWidgetItem *installListWidgetItem = new QListWidgetItem(ui->installUpdatesListWidget_2);
                            ui->installUpdatesListWidget_2->addItem( installListWidgetItem);
                            InstallItemWidget *installItemWidget = new InstallItemWidget;
                            installItemWidget->setPackageUpdateInfo(package, oldVersion, newVersion);
                            installListWidgetItem->setSizeHint (installItemWidget->sizeHint());
                            ui->installUpdatesListWidget_2->setItemWidget (installListWidgetItem, installItemWidget);

                            updatesAvailable = true;
                        }
                    }
                });

                ui->installNowPushButton_2->setEnabled(true);
                ui->searchingForUpdatesFrame_2->hide();
                ui->installUpdatesListWidget_2->show();
            } else {
                ui->updateManagerLogo_2->setPixmap( QIcon::fromTheme("dialog-ok-apply").pixmap(64, 64) );
                ui->searchingForUpdatesLabel_2->setText("Your system is up-to-date");
                ui->installNowPushButton_2->setEnabled(false);
                ui->backPushButton_3->setEnabled(true);
                ui->searchingForUpdatesProgressBar_2->hide();
            }
        }
    });
}

//Get updates
void MainWindow::getUpdates()
{
    ui->backPushButton_3->setEnabled(false);
    ui->installNowPushButton_2->setEnabled(false);
    ui->installUpdatesListWidget_2->clear();

    ui->updateManagerLogo_2->setPixmap( QIcon::fromTheme("system-software-update").pixmap(64, 64) );
    ui->searchingForUpdatesLabel_2->setText("Searching for Updates...");
    ui->searchingForUpdatesProgressBar_2->show();

    ui->searchingForUpdatesFrame_2->show();
    ui->installUpdatesListWidget_2->hide();

    QProcess *updateProcess;
    QString updateProcessProgram = "pkexec";
    QStringList updateProcessArguments;
    updateProcessArguments << "app-center-cli" << "update";
    updateProcess = new QProcess();
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LC_ALL", "C");
    updateProcess->setProcessEnvironment(env);

    updateProcess->start(updateProcessProgram, updateProcessArguments);

    connect(updateProcess, &QProcess::readyReadStandardOutput, [=]{
        QTextStream(stdout) << updateProcess->readAllStandardOutput();
    });
    connect(updateProcess, &QProcess::readyReadStandardError, [=]{
        QByteArray error = updateProcess->readAllStandardError();
        QTextStream(stderr) << error;
    });

    connect(updateProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus){
        if (exitCode == 0) {
            MainWindow::queryUpdates();
        } else if (exitCode == 1) {
            QMessageBox::critical(this, "Error - App Center", "An unknown error occoured!");
            ui->pages->setCurrentIndex(0);
            lastPage.append(0);
        } else if (exitCode == 127) {
            QMessageBox::critical(this, "Error - App Center", "You are not authorized!");
            ui->pages->setCurrentIndex(0);
            lastPage.append(0);
        }
    });
}

//On "Search for Updates" button press
void MainWindow::on_searchForUpdatesPushButton_released()
{
    ui->pages->setCurrentIndex(3);
    lastPage.append(3);

    getUpdates();
}

//On back button press (application list page)
void MainWindow::on_backPushButton_1_released() {
    ui->applicationListWidget->clear();

    ui->pages->setCurrentIndex(0);
    lastPage.removeLast();
}

//On back button press (install page)
void MainWindow::on_backPushButton_2_released()
{
    ui->pages->setCurrentIndex(0);
    lastPage.removeLast();
}

//On back button press (update manager page)
void MainWindow::on_backPushButton_3_released()
{
    ui->pages->setCurrentIndex(0);
    lastPage.removeLast();
}

//On back button press (application info page)
void MainWindow::on_backPushButton_4_released()
{
    ui->pages->setCurrentIndex(0);
    lastPage.removeLast();
}

//On install button press (update manager page)
void MainWindow::on_installNowPushButton_2_released()
{
    QProcess *pacmanInstallProcess;
    QString pacmanInstallProcessProgram = "pkexec";
    QStringList pacmanInstallProcessArguments;
    pacmanInstallProcessArguments << "pacman" << "-Syu" << "--noconfirm";
    pacmanInstallProcess = new QProcess();
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LC_ALL", "C");
    pacmanInstallProcess->setProcessEnvironment(env);

    pacmanInstallProcess->start(pacmanInstallProcessProgram, pacmanInstallProcessArguments);

    setCursor(Qt::WaitCursor);
    ui->backPushButton_3->setEnabled(false);
    ui->installNowPushButton_2->setEnabled(false);
    ui->installUpdatesListWidget_2->setEnabled(false);

    connect(pacmanInstallProcess, &QProcess::readyReadStandardOutput, [=]{
        QTextStream(stdout) << pacmanInstallProcess->readAllStandardOutput();
    });
    connect(pacmanInstallProcess, &QProcess::readyReadStandardError, [=]{
        QByteArray error = pacmanInstallProcess->readAllStandardError();
        QTextStream(stderr) << error;
    });

    connect(pacmanInstallProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus){
        if (exitCode == 0) {
            //If finished installing package Updates
            installPackageUpdateList.clear();
            QTextStream(stdout) << "Done.\n";
            ui->updateManagerLogo_2->setPixmap( QIcon::fromTheme("dialog-ok-apply").pixmap(64, 64) );
            ui->searchingForUpdatesLabel_2->setText("Your system is up-to-date");
            ui->installNowPushButton_2->setEnabled(false);
            ui->searchingForUpdatesProgressBar_2->hide();
            ui->searchingForUpdatesFrame_2->show();
            ui->installUpdatesListWidget_2->hide();
        } else if (exitCode == 1) {
            QMessageBox::critical(this, "Error - App Center", "An unknown error occoured!");
        } else if (exitCode == 127) {
            QMessageBox::critical(this, "Error - App Center", "You are not authorized!");
        }

        setCursor(Qt::ArrowCursor);
        ui->backPushButton_3->setEnabled(true);
        ui->installUpdatesListWidget_2->setEnabled(true);
    });
}
