#include "applicationitemwidget.h"
#include "ui_applicationitemwidget.h"

ApplicationItemWidget::ApplicationItemWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ApplicationItemWidget)
{
    ui->setupUi(this);
}

ApplicationItemWidget::~ApplicationItemWidget()
{
    delete ui;
}

void ApplicationItemWidget::setInstalled(bool installed) {
    if (installed) {
        applicationInstalled = true;
        ui->installPushButton->setText("Remove");
        ui->installPushButton->setIcon(QIcon::fromTheme("edit-delete"));
    } else {
        applicationInstalled = false;
        ui->installPushButton->setText("Install");
        ui->installPushButton->setIcon(QIcon::fromTheme("download"));
    }
}

void ApplicationItemWidget::setApplicationInfo(QString application, QString name, QString developer, QString URL, QPixmap icon, QString summary, QString description, QString package)
{
    applicationID = application;
    applicationName = name;
    applicationDeveloper = developer;
    applicationURL = URL;
    applicationIcon = icon;
    applicationSummary = summary;
    applicationDescription = description;
    applicationPackage = package;

    ui->applicationNameLabel->setText(applicationName);
    ui->applicationIconLabel->setPixmap(applicationIcon);
    ui->applicationDescriptionLabel->setText(applicationSummary);
    ApplicationItemWidget::setObjectName("ApplicationItemWidget_" + applicationID);

    connect(ui->installPushButton, &QPushButton::released, [=]{
        if (applicationInstalled) {
            emit removeApplication();
        } else {
            emit installApplication();
        }
    });
}
