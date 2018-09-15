#ifndef INSTALLITEMWIDGET_H
#define INSTALLITEMWIDGET_H

#include <QWidget>

namespace Ui {
class InstallItemWidget;
}

class InstallItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit InstallItemWidget(QWidget *parent = nullptr);
    ~InstallItemWidget();

    void setApplicationUpdateInfo(QString name, QPixmap icon, QString newVersion = "");
    void setPackageUpdateInfo(QString name, QString oldVersion, QString newVersion);

signals:
    void showPackageUpdateList();

private:
    Ui::InstallItemWidget *ui;
};

#endif // INSTALLITEMWIDGET_H
