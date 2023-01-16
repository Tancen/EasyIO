#include "widget.h"
#include "ui_widget.h"
#include <QMessageBox>
#include <QDateTime>
#include "../../../test_config.h"

using namespace std::placeholders;

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    QObject::connect(ui->btnOpen, SIGNAL(clicked(bool)), this, SLOT(open()));
    QObject::connect(ui->btnClose, SIGNAL(clicked(bool)), this, SLOT(close()));
    QObject::connect(this, SIGNAL(textNeedPrint(QTextEdit*,QString)),
            this, SLOT(printText(QTextEdit*,QString)));

    m_server = EasyIO::UDP::Server::create();
    if (!m_server.get())
    {
        QMessageBox::critical(NULL, "初始化失败", "初始化 client 失败");
        exit(1);
        return;
    }

    m_server->onBufferReceived = std::bind(&Widget::whenBufferReceived, this, _1, _2, _3, _4);
}

Widget::~Widget()
{
    m_server.reset();
    delete ui;
}

void Widget::whenBufferReceived(EasyIO::UDP::ISocket* socket, const std::string& ip, unsigned short port, EasyIO::ByteBuffer data)
{
    std::string str;
    str.append("已接收 [")
        .append(ip).append(":").append(QString::number(port).toStdString()).append("] :");

    str.append(data.readableBytes(), data.numReadableBytes());

    socket->send(ip, port, data);
    socket->recv(EasyIO::ByteBuffer());

    emit textNeedPrint(ui->txtedtMsg, str.c_str());
}

void Widget::open()
{
    unsigned short port;
    if (ui->ledtPort->text().isEmpty())
    {
        QMessageBox::information(NULL, "提示", "端口不能为空");
        return;
    }

    bool isOk;
    port = ui->ledtPort->text().toShort(&isOk);
    if (!isOk || !port)
    {
        QMessageBox::information(NULL, "提示", "端口不是有效的数字");
        return;
    }

    if (!m_server->open(port))
    {
        ui->txtedtMsg->append("打开失败");
    }
    else
    {
        ui->txtedtMsg->append("打开成功");

       m_server->recv(EasyIO::ByteBuffer());
    }
}

void Widget::close()
{
    m_server->close();
    emit textNeedPrint(ui->txtedtMsg, "已关闭");
}

void Widget::printText(QTextEdit *control, QString str)
{
    control->append(QDateTime::currentDateTime().toString());
    control->append(str.append("\n"));
}
