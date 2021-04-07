#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTextEdit>
#include "EasyIO.h"

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

signals:
    void textNeedPrint(QTextEdit* control, QString str);

private:
    void whenConnected(EasyIO::TCP::IConnection*);
    void whenConnectFailed(EasyIO::TCP::IConnection*);
    void whenDisconnected(EasyIO::TCP::IConnection*);
    void whenBufferSent(EasyIO::TCP::IConnection*, EasyIO::AutoBuffer data);
    void whenBufferReceived(EasyIO::TCP::IConnection*, EasyIO::AutoBuffer data);

private slots:
    void connect();
    void disconnect();
    void send();

    void printText(QTextEdit* control, QString str);

private:
    Ui::Widget *ui;
    EasyIO::TCP::IClientPtr m_client;
    EasyIO::AutoBuffer m_recvBuf;
};

#endif // WIDGET_H
