#ifndef APPLICATIONITEMWIDGET_H
#define APPLICATIONITEMWIDGET_H

#include <QWidget>

namespace Ui {
class ApplicationItemWidget;
}

class ApplicationItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ApplicationItemWidget(QWidget *parent = nullptr);
    ~ApplicationItemWidget();

    QString applicationID;
    QString applicationName;
    QString applicationDeveloper;
    QString applicationURL;
    QPixmap applicationIcon;
    QString applicationSummary;
    QString applicationDescription;
    QString applicationPackage;

    bool applicationInstalled;

    void setInstalled(bool installed);
    void setApplicationInfo(QString application, QString name, QString developer, QString URL, QPixmap icon, QString summary, QString description, QString package);

signals:
    void removeApplication();
    void installApplication();

private:
    Ui::ApplicationItemWidget *ui;
};

#endif // APPLICATIONITEMWIDGET_H
