#include "installitemwidget.h"
#include "ui_installitemwidget.h"
#include <QFontDatabase>

InstallItemWidget::InstallItemWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::InstallItemWidget)
{
    ui->setupUi(this);
}

InstallItemWidget::~InstallItemWidget()
{
    delete ui;
}

void InstallItemWidget::setApplicationUpdateInfo(QString name, QPixmap icon, QString newVersion) {
    ui->checkBox->hide();
    ui->applicationNameLabel->setText(name);
    if (newVersion == "") {
        ui->applicationNameLabel->setAlignment(Qt::AlignVCenter);
        ui->applicationVersionLabel->hide();
    } else {
        ui->applicationVersionLabel->setText(newVersion);
    }
    ui->applicationIconLabel->setPixmap(icon);
    setObjectName("installApplicationUpdateListWidgetItem_" + name);

    connect(ui->moreInfoPushButton, &QPushButton::released, [=]{
        emit showPackageUpdateList();
    });
}

void InstallItemWidget::setPackageUpdateInfo(QString name, QString oldVersion, QString newVersion) {
    ui->checkBox->hide();
    QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->applicationNameLabel->setFont(fixedFont);
    ui->applicationNameLabel->setText(name + " [" + oldVersion + " -> " + newVersion + "]");
    ui->applicationIconLabel->hide();
    ui->applicationNameLabel->setAlignment(Qt::AlignVCenter);
    ui->applicationVersionLabel->hide();
    ui->moreInfoPushButton->hide();
    setObjectName("installPackageUpdateListWidgetItem_" + name);
}
