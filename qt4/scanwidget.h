#ifndef CHOOSEDVD_H
#define CHOOSEDVD_H

#include <QtGui>

#include "ui_scanwidget.h"

class ClickedField : public QObject
{
    Q_OBJECT
public:
    ClickedField(QRadioButton *b, QObject *parent = 0);
protected:
    bool eventFilter(QObject *, QEvent *);
    QRadioButton *rb;
};

class ScanWidget : public QWidget, private Ui::ScanWidget
{
    Q_OBJECT
public:
    ScanWidget(QWidget *parent = 0);
    QMap<QString, QString> *volumeList();

signals:
    void scanDVD(const QString path);
    void scanningDone();

public slots:
    void goScan();
    void updateVolumeList(QString);
    void setProgress(int value, int maximum);
    void setFolder();

private:
    QMap<QString, QString> *devices;
};

#endif // CHOOSEDRIVE_H
