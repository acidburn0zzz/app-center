#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "applicationitemwidget.h"
#include "installitemwidget.h"
#include <QTextStream>
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

    ui->transactionProgressBar->hide();
    ui->transactionStatusLabel->hide();

    //Settings
    if (!(settings.contains("Distribution"))) {
        settings.setValue("Distribution", "NorcuxOS");
    }
    if (!(settings.contains("MetadataRepo"))) {
        settings.setValue("MetadataRepo", "https://codeload.github.com/NorcuxOS/app-center-metadata/tar.gz/master");
    }

    distribution = settings.value("Distribution").toString();
    metadataRepo = settings.value("MetadataRepo").toString();

    //Set icons
    ui->softwareCenterLogo->setPixmap( QIcon::fromTheme("muondiscover").pixmap(64, 64) );
    ui->updateManagerLogo_1->setPixmap( QIcon::fromTheme("system-software-update").pixmap(64, 64) );
    ui->updateManagerLogo_2->setPixmap( QIcon::fromTheme("system-software-update").pixmap(64, 64) );

    //Connect category buttons
    QList<QPushButton *> categoryButtons = ui->categoriesFrame->findChildren<QPushButton *>();
    foreach(QPushButton *categoryButton, categoryButtons)
    {
        connect(categoryButton, &QPushButton::released, this, &MainWindow::categoryPushButtonReleased);
    }

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
        QProcess *downloadMetadataCacheProcess = new QProcess();
        downloadMetadataCacheProcess->setWorkingDirectory(cacheFolder);
        downloadMetadataCacheProcess->start("curl", QStringList() << metadataRepo << "--output" << distribution + "-app-center-metadata.tar.gz");
        connect(downloadMetadataCacheProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus){
            if (downloadMetadataCacheProcess->exitCode() == 0) {
                QProcess *unpackMetadataCache = new QProcess();
                unpackMetadataCache->setWorkingDirectory(cacheFolder);
                unpackMetadataCache->start("tar", QStringList() << "-xf" << distribution + "-app-center-metadata.tar.gz" << "--one-top-level=" + distribution + "-app-center-metadata" << "--strip" << "1");
                connect(unpackMetadataCache, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus){
                    if (unpackMetadataCache->exitCode() == 0) {
                        ui->installUpdatesListWidget_2->hide();
                        ui->transactionStatusLabel->hide();
                        ui->transactionProgressBar->hide();
                        if (settings.contains("FirstRunDone")) {
                            if (settings.value("FirstRunDone") == true) {
                                initialUpdate = false;
                                ui->pages->setCurrentIndex(0);
                                lastPage.append(0);
                            } else {
                                ui->loadingLabel->setText("Downloading data, this may take some time...");
                                getUpdates();
                            }
                            settings.setValue("FirstRunDone", "true");
                        } else {
                            ui->loadingLabel->setText("Downloading data, this may take some time...");
                            getUpdates();
                            settings.setValue("FirstRunDone", "true");
                        }
                    }
                });
            } else {
                //Workaround for wrong MessageBox position
                QProcess *sleepProcess = new QProcess();
                sleepProcess->start("sleep", QStringList() << "0.5");
                connect(sleepProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus){
                    QByteArray error = downloadMetadataCacheProcess->readAllStandardError();
                    if (error.contains("Could not resolve host")) {
                        QMessageBox::critical(this, "Error - App Center", "No internet connection!");
                    } else {
                        QMessageBox::critical(this, "Error - App Center", "An unknown error occoured!");
                    }
                    exit(1);
                });
            }
        });
    }
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
                  QStringList applicationPackages;

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
                          QString applicationPackagesKeyword = "PACKAGES=";

                          //Application Name
                          int applicationNamePos = line.indexOf(applicationNameKeyword);
                          if (applicationNamePos >= 0)
                          {
                              applicationName = line.mid(applicationNamePos + applicationNameKeyword.length());
                          }

                          //Application Developer
                          int applicationDeveloperPos = line.indexOf(applicationDeveloperKeyword);
                          if (applicationDeveloperPos >= 0)
                          {
                              applicationDeveloper = line.mid(applicationDeveloperPos + applicationDeveloperKeyword.length());
                          }

                          //Application URL
                          int applicationURLPos = line.indexOf(applicationURLKeyword);
                          if (applicationURLPos >= 0)
                          {
                              applicationURL = line.mid(applicationURLPos + applicationURLKeyword.length());
                          }

                          //Application Theme Icon
                          int applicationThemeIconPos = line.indexOf(applicationThemeIconKeyword);
                          if (applicationThemeIconPos >= 0)
                          {
                              applicationThemeIcon = line.mid(applicationThemeIconPos + applicationThemeIconKeyword.length());
                              if (applicationThemeIcon == "null" || !(QIcon::hasThemeIcon(applicationThemeIcon))) {
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
                              applicationSummary = line.mid(applicationSummaryPos + applicationSummaryKeyword.length());
                          }

                          //Application Description
                          int applicationDescriptionPos = line.indexOf(applicationDescriptionKeyword);
                          if (applicationDescriptionPos >= 0)
                          {
                              applicationDescription = line.mid(applicationDescriptionPos + applicationDescriptionKeyword.length());
                          }

                          //Application Packages
                          int applicationPackagesPos = line.indexOf(applicationPackagesKeyword);
                          if (applicationPackagesPos >= 0)
                          {
                              applicationPackages = line.mid(applicationPackagesPos + applicationPackagesKeyword.length()).split(" ");
                          }
                      }
                  }

                  applicationItemWidget->setApplicationInfo(application, applicationName, applicationDeveloper, applicationURL, applicationIconPixmap, applicationSummary, applicationDescription, applicationPackages[0]);

                  //Check if application is already installed
                  QProcess *packageManagerSearchProcess;
                  QString packageManagerSearchProcessProgram = "dpkg";
                  QStringList packageManagerSearchProcessArguments;
                  packageManagerSearchProcessArguments << "--get-selections" << applicationPackages[0];
                  packageManagerSearchProcess = new QProcess();
                  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                  env.insert("LC_ALL", "C");
                  packageManagerSearchProcess->setProcessEnvironment(env);

                  packageManagerSearchProcess->start(packageManagerSearchProcessProgram, packageManagerSearchProcessArguments);

                  connect(packageManagerSearchProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus){
                      QByteArray output = packageManagerSearchProcess->readAllStandardOutput();
                      if (output.contains("install")) {
                          applicationItemWidget->setInstalled(true);
                      } else if (output.contains("dpkg: no packages found matching") || output.isEmpty()) {
                          applicationItemWidget->setInstalled(false);
                      }
                  });

                  //Remove application
                  connect( applicationItemWidget, &ApplicationItemWidget::removeApplication, [=]{
                      QProcess *packageManagerRemoveProcess;
                      QString packageManagerRemoveProcessProgram = "pkexec";
                      QStringList packageManagerRemoveProcessArguments;
                      packageManagerRemoveProcessArguments << "apt-get" << "remove" << "--yes" << applicationPackages;
                      packageManagerRemoveProcess = new QProcess();
                      QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                      env.insert("LC_ALL", "C");
                      packageManagerRemoveProcess->setProcessEnvironment(env);

                      packageManagerRemoveProcess->start(packageManagerRemoveProcessProgram, packageManagerRemoveProcessArguments);

                      ui->transactionStatusLabel->setText("Loading...");
                      ui->transactionStatusLabel->show();
                      ui->transactionProgressBar->setMaximum(0);
                      ui->transactionProgressBar->show();
                      ui->categoryTitleLabel->setEnabled(false);
                      ui->applicationListWidget->setEnabled(false);
                      ui->backPushButton_1->setEnabled(false);

                      connect(packageManagerRemoveProcess, &QProcess::readyReadStandardOutput, [=]{
                          QByteArray output = packageManagerRemoveProcess->readAllStandardOutput();
                          QTextStream(stdout) << output;
                          if (output.contains("(Reading database ...")) {
                              ui->transactionStatusLabel->setText("Removing...");
                          }
                      });
                      connect(packageManagerRemoveProcess, &QProcess::readyReadStandardError, [=]{
                          QByteArray error = packageManagerRemoveProcess->readAllStandardError();
                          QTextStream(stderr) << error;
                      });

                      connect(packageManagerRemoveProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus){
                          if (exitCode == 0) {
                              QTextStream(stdout) << "Done.\n";
                              applicationItemWidget->setInstalled(false);
                          } else if (exitCode == 1) {
                              QMessageBox::critical(this, "Error - App Center", "An unknown error occoured!");
                          } else if (exitCode == 127) {
                              QMessageBox::critical(this, "Error - App Center", "You are not authorized!");
                          }

                          ui->transactionStatusLabel->hide();
                          ui->transactionProgressBar->hide();
                          ui->categoryTitleLabel->setEnabled(true);
                          ui->applicationListWidget->setEnabled(true);
                          ui->backPushButton_1->setEnabled(true);
                      });

                  });

                  //Install application
                  connect( applicationItemWidget, &ApplicationItemWidget::installApplication, [=]{
                      QProcess *packageManagerInstallProcess;
                      QString packageManagerInstallProcessProgram = "pkexec";
                      QStringList packageManagerInstallProcessArguments;
                      packageManagerInstallProcessArguments << "apt-get" << "install" << "--yes" << applicationPackages;
                      packageManagerInstallProcess = new QProcess();
                      QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                      env.insert("LC_ALL", "C");
                      packageManagerInstallProcess->setProcessEnvironment(env);

                      packageManagerInstallProcess->start(packageManagerInstallProcessProgram, packageManagerInstallProcessArguments);

                      ui->transactionStatusLabel->setText("Loading...");
                      ui->transactionStatusLabel->show();
                      ui->transactionProgressBar->setMaximum(0);
                      ui->transactionProgressBar->show();
                      ui->categoryTitleLabel->setEnabled(false);
                      ui->applicationListWidget->setEnabled(false);
                      ui->backPushButton_1->setEnabled(false);

                      connect(packageManagerInstallProcess, &QProcess::readyReadStandardOutput, [=]{
                          QByteArray output = packageManagerInstallProcess->readAllStandardOutput();
                          QTextStream(stdout) << output;
                          if (output.contains("Get:")) {
                              ui->transactionStatusLabel->setText("Downloading...");
                          } else if (output.contains("(Reading database ...")) {
                              ui->transactionStatusLabel->setText("Installing...");
                          }
                      });
                      connect(packageManagerInstallProcess, &QProcess::readyReadStandardError, [=]{
                          QByteArray error = packageManagerInstallProcess->readAllStandardError();
                          QTextStream(stderr) << error;
                      });

                      connect(packageManagerInstallProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus){
                          if (exitCode == 0) {
                              QTextStream(stdout) << "Done.\n";
                              applicationItemWidget->setInstalled(true);
                          } else if (exitCode == 1) {
                              QMessageBox::critical(this, "Error - App Center", "An unknown error occoured!");
                          } else if (exitCode == 127) {
                              QMessageBox::critical(this, "Error - App Center", "You are not authorized!");
                          }

                          ui->transactionStatusLabel->hide();
                          ui->transactionProgressBar->hide();
                          ui->categoryTitleLabel->setEnabled(true);
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
            updatesAvailable = false;

            QString OSUpdate = queryUpdatesProcessOutput.split("\n").first();

            if (OSUpdate != "===== No OS updates are available. =====") {
                updatesAvailable = true;
            }

            QStringList updateList = queryUpdatesProcessOutput.split("\n");
            updateList.removeFirst();
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
                    foreach (QString update, updateList) {
                        QString package = update.split(" ").at(0);
                        package = package.split("/").first();
                        QString oldVersion = update.split(" ").at(1);
                        QString newVersion = update.split(" ").at(5);
                        newVersion.chop(1);

                        //If package update is not ignored
                        if (!(update.contains("[ignored]"))) {
                            //Add Package Update Item
                            QListWidgetItem *installListWidgetItem = new QListWidgetItem(ui->updateManagerMoreInfoListWidget);
                            ui->updateManagerMoreInfoListWidget->addItem( installListWidgetItem);
                            InstallItemWidget *installItemWidget = new InstallItemWidget;
                            installItemWidget->setPackageUpdateInfo(package, oldVersion, newVersion);
                            installListWidgetItem->setSizeHint (installItemWidget->sizeHint());
                            ui->updateManagerMoreInfoListWidget->setItemWidget (installListWidgetItem, installItemWidget);

                            updatesAvailable = true;
                        }
                    }
                    ui->pages->setCurrentIndex(4);
                    lastPage.append(4);

                    ui->updateManagerMoreInfoListWidget->setFocus();
                });

                ui->installNowPushButton_2->setEnabled(true);
                ui->installUpdatesListWidget_2->setFocus();
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

    connect(updateProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus){
        if (exitCode == 0) {
            if (initialUpdate) {
                initialUpdate = false;
                ui->pages->setCurrentIndex(0);
                lastPage.append(0);
            } else {
                MainWindow::queryUpdates();
            }
        } else {
            QByteArray error = updateProcess->readAllStandardError();
            QTextStream(stderr) << error;
            if (error.contains("Error executing command as another user: Not authorized")) {
                QMessageBox::critical(this, "Error - App Center", "You are not authorized!");
            } else if (error.contains("===== No internet connection! =====")) {
                QMessageBox::critical(this, "Error - App Center", "No internet connection!");
            } else {
                QMessageBox::critical(this, "Error - App Center", "An unknown error occoured!");
            }

            if (initialUpdate) {
                exit(1);
            }
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

//On back button press (update manager more info page)
void MainWindow::on_backPushButton_5_released()
{
    ui->pages->setCurrentIndex(3);
    lastPage.removeLast();

    ui->installUpdatesListWidget_2->setFocus();
}

//On install button press (update manager page)
void MainWindow::on_installNowPushButton_2_released()
{
    QProcess *packageManagerInstallProcess;
    QString packageManagerInstallProcessProgram = "pkexec";
    QStringList packageManagerInstallProcessArguments;
    packageManagerInstallProcessArguments << "apt-get" << "upgrade" << "--yes";
    packageManagerInstallProcess = new QProcess();
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LC_ALL", "C");
    packageManagerInstallProcess->setProcessEnvironment(env);

    packageManagerInstallProcess->start(packageManagerInstallProcessProgram, packageManagerInstallProcessArguments);

    ui->transactionStatusLabel->setText("Loading...");
    ui->transactionStatusLabel->show();
    ui->transactionProgressBar->setMaximum(0);
    ui->transactionProgressBar->show();
    ui->installTitleLabel_2->setEnabled(false);
    ui->backPushButton_3->setEnabled(false);
    ui->installNowPushButton_2->setEnabled(false);
    ui->installUpdatesListWidget_2->setEnabled(false);

    connect(packageManagerInstallProcess, &QProcess::readyReadStandardOutput, [=]{
        QByteArray output = packageManagerInstallProcess->readAllStandardOutput();
        QTextStream(stdout) << output;
        if (output.contains("Get:")) {
            ui->transactionStatusLabel->setText("Downloading updates...");
        }
        if (output.contains("(Reading database ...")) {
            ui->transactionStatusLabel->setText("Installing updates...");
        }
    });
    connect(packageManagerInstallProcess, &QProcess::readyReadStandardError, [=]{
        QByteArray error = packageManagerInstallProcess->readAllStandardError();
        QTextStream(stderr) << error;
    });

    connect(packageManagerInstallProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus){
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

        ui->transactionStatusLabel->hide();
        ui->transactionProgressBar->hide();
        ui->installTitleLabel_2->setEnabled(true);
        ui->backPushButton_3->setEnabled(true);
        ui->installUpdatesListWidget_2->setEnabled(true);
    });
}
