#include <QtWidgets>
#include <QtNetwork>
#include <QDebug>
#include <QJsonParseError>

#include "client.h"

Client::Client(QWidget *parent)
    : QDialog(parent)
    , hostCombo(new QComboBox)
    , portLineEdit(new QLineEdit)
    , getJsonButton(new QPushButton(tr("Get Json data")))
    , tcpSocket(new QTcpSocket(this))
    , textEdit(new QTextEdit(this))
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    hostCombo->setEditable(true);
    // find out name of this machine
    QString name = QHostInfo::localHostName();
    if (!name.isEmpty()) {
        hostCombo->addItem(name);
        QString domain = QHostInfo::localDomainName();
        if (!domain.isEmpty())
            hostCombo->addItem(name + QChar('.') + domain);
    }
    if (name != QLatin1String("localhost"))
        hostCombo->addItem(QString("localhost"));
    // find out IP addresses of this machine
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // add non-localhost addresses
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (!ipAddressesList.at(i).isLoopback())
            hostCombo->addItem(ipAddressesList.at(i).toString());
    }
    // add localhost addresses
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i).isLoopback())
            hostCombo->addItem(ipAddressesList.at(i).toString());
    }

    portLineEdit->setValidator(new QIntValidator(1, 65535, this));

    auto hostLabel = new QLabel(tr("&Server name:"));
    hostLabel->setBuddy(hostCombo);
    auto portLabel = new QLabel(tr("S&erver port:"));
    portLabel->setBuddy(portLineEdit);



    statusLabel = new QLabel(tr("This examples requires that you run the "
                                "Json Server example as well."));

    getJsonButton->setDefault(true);
    getJsonButton->setEnabled(false);

    auto quitButton = new QPushButton(tr("Quit"));

    auto buttonBox = new QDialogButtonBox;
    buttonBox->addButton(getJsonButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

//! [1]
//! [1]

    connect(hostCombo, &QComboBox::editTextChanged,
            this, &Client::enableGetPackageButton);
    connect(portLineEdit, &QLineEdit::textChanged,
            this, &Client::enableGetPackageButton);
    connect(getJsonButton, &QAbstractButton::clicked,
            this, &Client::requestNewPackage);
    connect(quitButton, &QAbstractButton::clicked, this, &QWidget::close);
    connect(tcpSocket, &QIODevice::readyRead, this, &Client::readPackage);
    connect(tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &Client::displayError);

    QGridLayout *mainLayout = nullptr;
    if (QGuiApplication::styleHints()->showIsFullScreen() || QGuiApplication::styleHints()->showIsMaximized()) {
        auto outerVerticalLayout = new QVBoxLayout(this);
        outerVerticalLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
        auto outerHorizontalLayout = new QHBoxLayout;
        outerHorizontalLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Ignored));
        auto groupBox = new QGroupBox(QGuiApplication::applicationDisplayName());
        mainLayout = new QGridLayout(groupBox);
        outerHorizontalLayout->addWidget(groupBox);
        outerHorizontalLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Ignored));
        outerVerticalLayout->addLayout(outerHorizontalLayout);
        outerVerticalLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
    } else {
        mainLayout = new QGridLayout(this);
    }
    mainLayout->addWidget(hostLabel, 0, 0);
    mainLayout->addWidget(hostCombo, 0, 1);
    mainLayout->addWidget(portLabel, 1, 0);
    mainLayout->addWidget(portLineEdit, 1, 1);
    mainLayout->addWidget(statusLabel, 2, 0, 1, 2);
    mainLayout->addWidget(textEdit, 3, 0, 3, 2);
    mainLayout->addWidget(buttonBox, 6, 0, 1, 2);

    setWindowTitle(QGuiApplication::applicationDisplayName());
    portLineEdit->setFocus();

    QNetworkConfigurationManager manager;
    if (manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired) {
        // Get saved network configuration
        QSettings settings(QSettings::UserScope, QLatin1String("QtProject"));
        settings.beginGroup(QLatin1String("QtNetwork"));
        const QString id = settings.value(QLatin1String("DefaultNetworkConfiguration")).toString();
        settings.endGroup();

        // If the saved network configuration is not currently discovered use the system default
        QNetworkConfiguration config = manager.configurationFromIdentifier(id);
        if ((config.state() & QNetworkConfiguration::Discovered) !=
            QNetworkConfiguration::Discovered) {
            config = manager.defaultConfiguration();
        }

        networkSession = new QNetworkSession(config, this);
        connect(networkSession, &QNetworkSession::opened, this, &Client::sessionOpened);

        getJsonButton->setEnabled(false);
        statusLabel->setText(tr("Opening network session."));
        networkSession->open();
    }
}

void Client::requestNewPackage()
{
    getJsonButton->setEnabled(false);
    tcpSocket->abort();
    tcpSocket->connectToHost(hostCombo->currentText(),
                             portLineEdit->text().toInt());
    textEdit->clear();
}

void Client::readPackage()
{
    if(m_exptected_json_size == 0)
    {
        if(!extract_content_size())
        {
            return; //wait to finish
        }
    }
    m_received_data.append(tcpSocket->readAll());
    if(m_exptected_json_size > 0 && m_received_data.size()>m_exptected_json_size )
    {
        if(parseJson())
        {
            qDebug()<<"Show Content" ;
            getJsonButton->setEnabled(true);
        }else{
            qDebug()<<"Continue Reading";
        }
    }
}

void Client::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this, tr("Json Client"),
                                 tr("The host was not found. Please check the "
                                    "host name and port settings."));
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this, tr("Json Client"),
                                 tr("The connection was refused by the peer. "
                                    "Make sure the fortune server is running, "
                                    "and check that the host name and port "
                                    "settings are correct."));
        break;
    default:
        QMessageBox::information(this, tr("Json Client"),
                                 tr("The following error occurred: %1.")
                                 .arg(tcpSocket->errorString()));
    }

    getJsonButton->setEnabled(true);
}
//! [13]

void Client::enableGetPackageButton()
{
    getJsonButton->setEnabled((!networkSession || networkSession->isOpen()) &&
                                 !hostCombo->currentText().isEmpty() &&
                                 !portLineEdit->text().isEmpty());

}

void Client::sessionOpened()
{
    // Save the used configuration
    QNetworkConfiguration config = networkSession->configuration();
    QString id;
    if (config.type() == QNetworkConfiguration::UserChoice)
        id = networkSession->sessionProperty(QLatin1String("UserChoiceConfiguration")).toString();
    else
        id = config.identifier();

    QSettings settings(QSettings::UserScope, QLatin1String("QtProject"));
    settings.beginGroup(QLatin1String("QtNetwork"));
    settings.setValue(QLatin1String("DefaultNetworkConfiguration"), id);
    settings.endGroup();

    statusLabel->setText(tr("This examples requires that you run the "
                            "Json Server example as well."));

    enableGetPackageButton();
}

bool Client::extract_content_size()
{
    quint64 asize = tcpSocket->bytesAvailable();
    if(asize > treshold)
    {
       m_received_data.append(tcpSocket->readAll());
       QDataStream in;
       QBuffer in_buffer;
       in_buffer.setBuffer(&m_received_data);
       in_buffer.open(QIODevice::ReadOnly);
       in.setDevice(&in_buffer);
       in.setVersion(QDataStream::Qt_5_10);
       quint64 size = 0;
       in>>size;
       if(size>0)
       {
           m_exptected_json_size = size;
           in_buffer.close();
           return true;
       }
       in_buffer.close();
       return false;

    }
    return false;
}

bool Client::parseJson()
{
    int eof;
    QByteArray json_data;
    QDataStream in;
    m_buffer.setBuffer(&m_received_data);
    if(!m_buffer.open(QIODevice::ReadOnly))
        return false;
    in.setDevice(&m_buffer);
    in.setVersion(QDataStream::Qt_5_10);
    in.startTransaction();
    quint64 json_size;
    in>>json_size>>json_data>>eof;
    if(!in.commitTransaction())
    {
        m_buffer.close();
        return false;
    }
    m_buffer.close();
    if(eof != end_of_file)
    {
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(json_data, &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                if (jsonDoc.isObject())
                    textEdit->clear();
                    textEdit->setText(jsonDoc.toJson());
                    m_received_data.clear();
                    return true;
            } else {
                return false;
            }
    return true;
}

