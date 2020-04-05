#ifndef CLIENT_H
#define CLIENT_H

#include <QDataStream>
#include <QDialog>
#include <QTcpSocket>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QTcpSocket>
#include <QNetworkSession>
#include <QBuffer>

class Client : public QDialog
{
    Q_OBJECT

public:
    explicit Client(QWidget *parent = nullptr);

private slots:
    void requestNewPackage();
    void readPackage();
    void displayError(QAbstractSocket::SocketError socketError);
    void enableGetPackageButton();
    void sessionOpened();
    bool extract_content_size();
    bool parseJson();

private:
    QComboBox *hostCombo = nullptr;
    QLineEdit *portLineEdit = nullptr;
    QLabel *statusLabel = nullptr;
    QPushButton *getJsonButton = nullptr;
    QTextEdit * textEdit = nullptr;

    QTcpSocket *tcpSocket = nullptr;
    QDataStream in;
    QByteArray m_data;
    QByteArray m_received_data;
    QBuffer m_buffer;
    QString currentPackage;
    quint64 m_exptected_json_size = 0;

    QNetworkSession *networkSession = nullptr;
    const quint64 treshold = 8;
    const qint32 end_of_file = 4444;
};

#endif
