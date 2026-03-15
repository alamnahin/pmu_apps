#ifndef QPMU_APP_MAIN_PAGE_INTERFACE_H
#define QPMU_APP_MAIN_PAGE_INTERFACE_H

#include <QWidget>
#include <QIcon>
#include <QList>

struct SidePanelItem
{
    QWidget *widget;
    QString text;
    QIcon icon = QIcon();
};

class MainPageInterface
{
public:
    virtual QVector<SidePanelItem> sidePanelItems() const { return {}; }
};

Q_DECLARE_INTERFACE(MainPageInterface, "com.qpmu.MainPageInterface/1.0")

#endif // QPMU_APP_MAIN_PAGE_INTERFACE_H