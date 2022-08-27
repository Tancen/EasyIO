#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTextEdit>
#include "EasyIO.h"
#include "../../../test_config.h"

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
    void whenConnectFailed(EasyIO::TCP::IConnection*, const std::string& reason);
    void whenDisconnected(EasyIO::TCP::IConnection*, const std::string& reason);
    void whenBufferReceived(EasyIO::TCP::IConnection*, EasyIO::ByteBuffer data);

private slots:
    void connect();
    void disconnect();

    void printText(QTextEdit* control, QString str);

private:
    Ui::Widget *ui;
    EasyIO::TCP::IClientPtr m_client;
    Count m_count;
};

#endif // WIDGET_H
