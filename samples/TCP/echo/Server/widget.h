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
    void whenConnected(EasyIO::TCP::IConnection *con);
    void whenDisconnected(EasyIO::TCP::IConnection *con, const std::string& reason);
    void whenBufferReceived(EasyIO::TCP::IConnection*, EasyIO::ByteBuffer data);

private slots:
    void open();
    void close();

    void printText(QTextEdit* control, QString str);

private:
    Ui::Widget *ui;
    EasyIO::TCP::IServerPtr m_server;
};

#endif // WIDGET_H
