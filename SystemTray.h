#pragma once

#include <QLabel>
#include <QSystemTrayIcon>

class QSystemTrayIcon;
class QMenu;

#define U_AVG                   5
#define P_AC                    0x3293
#define P_100                   0x3200
#define P_1                     0x2350
#define DEFAULT_TIMER_DELAY     2000

//#define ENABLE_WATCHDOG
class SystemTray : public QLabel {
Q_OBJECT
private:
    QSystemTrayIcon     *m_ptrayIcon;
    QMenu               *m_ptrayIconMenu;
    quint16             m_pU[U_AVG], m_nCurU, m_nMaxU, m_nState;
    qint16              m_nCurrentI;
    quint32             m_nTimerDelay;
    qint32              m_nTempUValue; // Temporary
    QPixmap             m_curIcon;
    int                 m_i2c_handle;
    QString             m_qsCurMsg;
    QString             m_qsUsedI2cDev0, m_qsUsedI2cDev1;
    quint8              m_u8Mode;
#ifdef ENABLE_WATCHDOG
    int 				m_fdWatchdog;							// Watchdog handle
#endif

protected:
    int i2cgetw( char *strDev, int address, int daddress, int *data );
    virtual void closeEvent(QCloseEvent*);
    void timerEvent(QTimerEvent *event);
    void SelDefI2cBus();

public:
    SystemTray(QWidget* pwgt = 0);

public slots:
    void slotShowHide();
    void slotShowMessage();
    void slotChangeIcon ();
    void onActivated(QSystemTrayIcon::ActivationReason reason);
};

