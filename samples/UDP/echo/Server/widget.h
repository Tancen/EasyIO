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
    void whenBufferReceived(EasyIO::UDP::ISocket*, const std::string& ip, unsigned short port, EasyIO::AutoBuffer data);

private slots:
    void open();
    void close();

    void printText(QTextEdit* control, QString str);

private:
    Ui::Widget *ui;
    EasyIO::UDP::IServerPtr m_server;
    EasyIO::AutoBuffer m_recvBuf;
};

#endif // WIDGET_H
