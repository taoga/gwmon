#include <QtWidgets>
#include <QDebug>
#include "SystemTray.h"
extern "C" {
    #include <linux/i2c-dev.h>  // need install libi2c-dev
    #include <i2c/smbus.h>
    #include "i2cbusses.h"
}
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "axsmbus.h"

#ifdef ENABLE_WATCHDOG
#include <linux/watchdog.h>
#endif

// ----------------------------------------------------------------------
SystemTray::SystemTray(QWidget* pwgt /*=0*/) 
    : QLabel("<H1>Application Window</H1>", pwgt)
{
    setWindowTitle("gwmon");
    // FIFO voltages
    memset( m_pU, 0, U_AVG * sizeof(quint16) );
    m_nCurU = 0;    // Current voltage index
    m_nMaxU = 0;    // Number of voltages
    m_nState = 0xFFFF;
    m_nTimerDelay = DEFAULT_TIMER_DELAY;
    m_nTempUValue = P_AC+100; // Temporary
    m_i2c_handle = -1;
    m_qsUsedI2cDev0 = "/dev/i2c-0";
    m_qsUsedI2cDev1 = "/dev/i2c-4";
    m_u8Mode = 0;       // 0 - battery manager; 1 - gatework gsc voltage; 2 - axiomtek sbc
    m_nCurrentI = 0;

    // Launch Watchdog /////////////////////////////////////////////////////////
    #ifdef ENABLE_WATCHDOG
    m_fdWatchdog = -1;
    m_fdWatchdog = open("/dev/watchdog", O_WRONLY);
    if ( m_fdWatchdog < 0 )
    {
        qDebug("Error: '/dev/watchdog' not opened.");
        m_fdWatchdog = open("/dev/watchdog1", O_WRONLY);
        if ( m_fdWatchdog < 0 )
        {
            qDebug("Error: '/dev/watchdog1' not opened.");
        }
    }
    if ( m_fdWatchdog >= 0 )
    {
        // Init watchdog
        int interval = 20;	// in seconds
        if ( ioctl( m_fdWatchdog, WDIOC_SETTIMEOUT, &interval) != 0)
        {
            qDebug("Error: '/dev/watchdog' not set timeout.\n");
        }
    }
    #endif

    QAction* pactShowMessage = new QAction("S&how Message", this);
    connect(pactShowMessage, SIGNAL(triggered()),
            this,            SLOT(slotShowMessage()) );

    QAction* pactQuit = new QAction("&Quit", this);
    connect(pactQuit, SIGNAL(triggered()), qApp, SLOT(quit()));

    m_ptrayIconMenu = 0;
    m_ptrayIconMenu = new QMenu(this);
    m_ptrayIconMenu->addAction(pactShowMessage);
    m_ptrayIconMenu->addAction(pactQuit);

    m_ptrayIcon = new QSystemTrayIcon(this);

    m_qsCurMsg = "No info";
    connect( m_ptrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
             this, SLOT( onActivated(QSystemTrayIcon::ActivationReason) ) );

    m_ptrayIcon->setContextMenu(m_ptrayIconMenu);

    if( m_u8Mode == 0)
    {
        SelDefI2cBus();
    }
    else
    if( m_u8Mode == 2)
    {
        AxI2cInit();
    }

    // Set the initial state
    slotChangeIcon();
    // Start the power supply state handler timer (once every 3 seconds)
    startTimer( m_nTimerDelay );

    m_ptrayIcon->show();
}

void SystemTray::SelDefI2cBus(void)
{
    struct i2c_adap *adapters;
    int             count, nData = 0;
    QString         qsPort;

    adapters = gather_i2c_busses();
    if (adapters == NULL) {
        qDebug( "Error: Out of memory!" );
        return;
    }

    for (count = 0; adapters[count].name; count++)
    {
        qDebug( "i2c-%d\t%-10s\t%-32s\t%s",
            adapters[count].nr, adapters[count].funcs,
            adapters[count].name, adapters[count].algo);

        qsPort = QString::asprintf( "/dev/i2c-%d", adapters[count].nr );

        if( i2cgetw((char*)(qsPort.toStdString().data()), 0x0b, 0x0d, &nData) >= 0 )
        {
            qDebug() << "Default port:" << qsPort;

            m_qsUsedI2cDev0 = qsPort;
            break;
        }
        else
            qDebug() << "Unsupport smbus";
    }

    free_adapters(adapters);
}

// ----------------------------------------------------------------------
/*virtual*/void SystemTray::closeEvent(QCloseEvent*)
{
    if (m_ptrayIcon->isVisible()) {
        hide();
    }
    if( m_i2c_handle != -1 )
        ::close( m_i2c_handle );

    if( m_ptrayIconMenu )
        delete m_ptrayIconMenu;
}

// ----------------------------------------------------------------------
void SystemTray::slotShowHide()
{
    setVisible(!isVisible());
}

// ----------------------------------------------------------------------
void SystemTray::slotShowMessage()
{
    m_ptrayIcon->showMessage("State",
                             m_qsCurMsg,
                             QSystemTrayIcon::Information,
                             3000 );
}

void SystemTray::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    if( reason == QSystemTrayIcon::Trigger )
    {
        slotShowMessage();
    }
}

// ----------------------------------------------------------------------
void SystemTray::slotChangeIcon()
{
    QString strPixmapName;
    quint32 nAvgU = 0;
    quint16 nU = 0, nState = 0;
    QString qsTemp;
    qint16  i16Temp = 0;
    bool    bI2CError = false;

    int data = 0;
    // Calculate average voltage, change battery image
    if( m_u8Mode == 1 ) // GATEWORKS
    {
        m_nTempUValue = 0;

        if( i2cgetw( (char*)(m_qsUsedI2cDev1.toStdString().data()), 0x29, 0x02, &data) >= 0 )
        {
            m_nTempUValue = data;
            m_pU[m_nCurU] = m_nTempUValue;
            m_nCurU++;
            if( m_nCurU >= U_AVG ) m_nCurU = 0;
            m_nMaxU++;
            if( m_nMaxU > U_AVG ) m_nMaxU = U_AVG;

            if( m_nMaxU > 0 )
            {
                for( nU = 0; nU < m_nMaxU; nU++ )
                    nAvgU += m_pU[nU];

                nAvgU /= m_nMaxU;

                if( nAvgU >= P_AC )
                {
                    strPixmapName = ":/images/b_ac.png";
                    qsTemp = "AC mode";
                    m_ptrayIcon->setToolTip( qsTemp );
                    m_qsCurMsg = qsTemp;
                    nState = 200;
                }
                else
                {
                    if( nAvgU < P_1 )
                        nState = 0;
                    else
                    if( nAvgU >= P_100 )
                        nState = 100;
                    else
                    {
                        nAvgU -= P_1;
                        nState = P_100 - P_1;

                        nState = (quint16)( ((double)nAvgU / (double)nState) * 100.0 );
                    }
                    if( nState > 100 ) nState = 100;

                    qsTemp = QString::asprintf("%d%%", nState );
                    m_ptrayIcon->setToolTip( qsTemp );
                    m_qsCurMsg = qsTemp;
                    // Execute an external command when the battery reaches a critical charge point
                    if( nState < 3 )
                    {
                        QProcess::startDetached( "shutdown -h now" );
                    }
                }
            }
        }
        else
            qDebug("Read from %s:0x29:0x02 - return error", m_qsUsedI2cDev1.toStdString().data() );
    }
    else
    if( m_u8Mode == 0 ) // SMBUS
    {
        // State, %
        if( i2cgetw( (char*)(m_qsUsedI2cDev0.toStdString().data()), 0x0b, 0x0d, &data) >= 0 )
        {
            if( data >= 0 && data <= 100 )
            {
                nState = (qint16)data;

                qsTemp = QString::asprintf("%d%%", nState );
                m_ptrayIcon->setToolTip( qsTemp );
                m_qsCurMsg = qsTemp;
            }
        }
        else
            bI2CError = true;
        // Current, (+ or -)mA
        if( i2cgetw( (char*)(m_qsUsedI2cDev0.toStdString().data()), 0x0b, 0x0a, &data) >= 0 )
        {
            i16Temp = (qint16)data;
        }
        else
            bI2CError = true;

        qDebug() << " nState:" << nState << " nU:" << i16Temp;
    }
    else
    if( m_u8Mode == 2 ) // Axiomtek SBC
    {
        qint16  i16Data= 0;
        quint8  u8Addr = 0x0b << 1; // 0 bit used for select read or write operation
        // State, %
        if( AxSmbBusReadByteWord( u8Addr, 0x0d, (UINT8*)&i16Data, AxEcI2cbusReadWord ) >= 0 )
        {
            if( i16Data >= 0 && data <= i16Data )
            {
                nState = i16Data;

                qsTemp = QString::asprintf("%d%%", nState );
                m_ptrayIcon->setToolTip( qsTemp );
                m_qsCurMsg = qsTemp;
            }
        }
        else
            bI2CError = true;
        // Current, (+ or -)mA
        if( AxSmbBusReadByteWord( u8Addr, 0x0a, (UINT8*)&i16Data, AxEcI2cbusReadWord ) >= 0 )
        {
            i16Temp = i16Data;
        }
        else
            bI2CError = true;
    }

    if( !bI2CError )
    {
        if( nState > 93  && nState <= 105 )
        {
            if( i16Temp < -50 ) // < -50 discharge
                strPixmapName = ":/images/b_100.png";
            else
                strPixmapName = ":/images/b_100ac.png";
            nState = 100;
        }
        else
        if( nState > 87  && nState <= 93 )
        {
            nState = 93;
            if( i16Temp < -50 ) // < -50 discharge
                strPixmapName = ":/images/b_93.png";
            else
                strPixmapName = ":/images/b_93ac.png";
        }
        else
        if( nState > 81  && nState <= 87 )
        {
            nState = 87;
            if( i16Temp < -50 ) // < -50 discharge
                strPixmapName = ":/images/b_87.png";
            else
                strPixmapName = ":/images/b_87ac.png";
        }
        else
        if( nState > 75  && nState <= 81 )
        {
            nState = 81;
            if( i16Temp < -50 ) // < -50 discharge
                strPixmapName = ":/images/b_81.png";
            else
                strPixmapName = ":/images/b_81ac.png";
        }
        else
        if( nState > 68  && nState <= 75 )
        {
            nState = 75;
            if( i16Temp < -50 ) // < -50 discharge
                strPixmapName = ":/images/b_75.png";
            else
                strPixmapName = ":/images/b_75ac.png";
        }
        else
        if( nState > 62  && nState <= 68 )
        {
            nState = 68;
            if( i16Temp < -50 ) // < -50 discharge
                strPixmapName = ":/images/b_68.png";
            else
                strPixmapName = ":/images/b_68ac.png";
        }
        else
        if( nState > 56  && nState <= 62 )
        {
            nState = 62;
            if( i16Temp < -50 ) // < -50 discharge
                strPixmapName = ":/images/b_62.png";
            else
                strPixmapName = ":/images/b_62ac.png";
        }
        else
        if( nState > 50  && nState <= 56 )
        {
            nState = 56;
            if( i16Temp < -50 ) // < -50 discharge
                strPixmapName = ":/images/b_56.png";
            else
                strPixmapName = ":/images/b_56ac.png";
        }
        else
        if( nState > 43  && nState <= 50 )
        {
            nState = 50;
            if( i16Temp < -50 ) // < -50 discharge
                strPixmapName = ":/images/b_50.png";
            else
                strPixmapName = ":/images/b_50ac.png";
        }
        else
        if( nState > 37  && nState <= 43 )
        {
            nState = 43;
            if( i16Temp < -50 ) // < -50 discharge
                strPixmapName = ":/images/b_43.png";
            else
                strPixmapName = ":/images/b_43ac.png";
        }
        else
        if( nState > 31  && nState <= 37 )
        {
            nState = 37;
            if( i16Temp < -50 ) // < -50 discharge
                strPixmapName = ":/images/b_37.png";
            else
                strPixmapName = ":/images/b_37ac.png";
        }
        else
        if( nState > 25  && nState <= 31 )
        {
            nState = 31;
            if( i16Temp < -50 ) // < -50 discharge
                strPixmapName = ":/images/b_31.png";
            else
                strPixmapName = ":/images/b_31ac.png";
        }
        else
        if( nState > 18  && nState <= 25 )
        {
            nState = 25;
            if( i16Temp < -50 ) // < -50 discharge
                strPixmapName = ":/images/b_25.png";
            else
                strPixmapName = ":/images/b_25ac.png";
        }
        else
        if( nState > 12  && nState <= 18 )
        {
            nState = 18;
            if( i16Temp < -50 ) // < -50 discharge
                strPixmapName = ":/images/b_18.png";
            else
                strPixmapName = ":/images/b_18ac.png";
        }
        else
        if( nState > 6  && nState <= 12 )
        {
            nState = 12;
            if( i16Temp < -50 ) // < -50 discharge
                strPixmapName = ":/images/b_12.png";
            else
                strPixmapName = ":/images/b_12ac.png";
        }
        else
        if( nState > 0  && nState <= 6 )
        {
            if( i16Temp < -50 ) // < -50 discharge
                strPixmapName = ":/images/b_6.png";
            else
                strPixmapName = ":/images/b_6ac.png";
            nState = 6;
        }
        else
        if( nState == 0 )
        {
            nState = 0;
            if( i16Temp < -50 ) // < -50 discharge
            {
                strPixmapName = ":/images/b_1.png";
                QProcess::startDetached( "shutdown -h now" );
            }
            else
                strPixmapName = ":/images/b_1ac.png";
        }

        if( m_nState != nState || (m_nCurrentI & 0xFFF0) != (i16Temp & 0xFFF0) )
        {
            m_ptrayIcon->setIcon(QPixmap(strPixmapName));
            m_nState = nState;

            m_nCurrentI = i16Temp;
        }
    } //end !bI2CError
}

void SystemTray::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event)
    #ifdef ENABLE_WATCHDOG
        if ( m_fdWatchdog != -1 )
            ioctl( m_fdWatchdog, WDIOC_KEEPALIVE, NULL);
    #endif
    // Update info
    slotChangeIcon();
}
int SystemTray::i2cgetw( char *strDev, int address, int daddress, int *data )
{
    int ret_val = -1;

    if( m_i2c_handle == -1 )
    {
        m_i2c_handle = open( strDev, O_RDWR);

        if( m_i2c_handle < 0 ) {
            qDebug( "Cannot get handle for %s", strDev );
            return -1;
        }
        ret_val = ioctl( m_i2c_handle, I2C_SLAVE_FORCE, address );
        if( ret_val < 0 )
        {
            qDebug( "Could not set I2C_SLAVE dev_addr:0x%X", address);
            ::close( m_i2c_handle );
            m_i2c_handle = -1;
            return -2;
        }
    }

    if( m_i2c_handle != -1 )
    {
        ret_val = i2c_smbus_read_word_data( m_i2c_handle, daddress );
    }

    if (ret_val < 0)
    {
        qDebug( "i2c_smbus_read_word_data return error. reg_addr:0x%X\n", daddress);
        ::close( m_i2c_handle );
        m_i2c_handle = -1;
        return -3;
    }
    else {
        *data = ret_val;
        return 0;
    }
}
