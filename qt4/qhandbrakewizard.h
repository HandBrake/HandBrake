#ifndef QHANDBRAKEWIZARD_H
#define QHANDBRAKEWIZARD_H

#include <QtGui>

typedef class QHandBrake;
typedef class FaderWidget;
typedef class ScanWidget;
typedef class EncodeWidget;

class QHandBrakeWizard : public QStackedWidget
{
    Q_OBJECT
public:
    QHandBrakeWizard(QStackedWidget *parent = 0);

public slots:
    void activateScanPage();
    void activateEncodePage();

    void fadeInWidget(QWidget *);

private:
    QPointer<QHandBrake> qhb;
    QPointer<FaderWidget> faderWidget;
    QPointer<ScanWidget> scanWidget;
    QPointer<EncodeWidget> encodeWidget;
};

class Magic : public QObject
{
    Q_OBJECT
public:
    Magic(QWidget *w, QHandBrakeWizard *q, QObject *p = 0);

protected:
    bool eventFilter(QObject *o, QEvent *e);
    QWidget *m;

    friend class QHandBrakeWizard;
    QHandBrakeWizard *qm;
};

#endif // QHANDBRAKEWIZARD_H
