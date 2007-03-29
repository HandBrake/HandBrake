#ifndef QHANDBRAKE_H
#define QHANDBRAKE_H

#include <QtGui>
#include "hb.h"

class QHandBrake : public QObject {
    Q_OBJECT

public:
    QHandBrake(QObject *parent = 0);
    ~QHandBrake();

    QStandardItemModel *titleListModel();

signals:
    void scanProgress(int cur, int total);

public slots:
    void startScan(const QString sDevice);
    void encode();

private:
    hb_handle_t *hbHandle;
    hb_title_t *hbTitle;

    QStandardItemModel *titleModel;
};

#endif // QHANDBRAKE_H
