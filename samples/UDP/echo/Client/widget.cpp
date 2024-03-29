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

    QObject::connect(ui->btnSend, SIGNAL(clicked(bool)), this, SLOT(send()));
    QObject::connect(ui->btnOpen, SIGNAL(clicked(bool)), this, SLOT(open()));
    QObject::connect(ui->btnClose, SIGNAL(clicked(bool)), this, SLOT(close()));
    QObject::connect(this, SIGNAL(textNeedPrint(QTextEdit*,QString)),
            this, SLOT(printText(QTextEdit*,QString)));

    m_client = EasyIO::UDP::Client::create();
    if (!m_client.get())
    {
        QMessageBox::critical(NULL, "初始化失败", "初始化 client 失败");
        exit(1);
        return;
    }

    m_client->onBufferReceived = std::bind(&Widget::whenBufferReceived, this, _1, _2, _3, _4);
}

Widget::~Widget()
{
    m_client.reset();
    delete ui;
}

void Widget::whenBufferReceived(EasyIO::UDP::ISocket* socket, const std::string& ip, unsigned short port, EasyIO::ByteBuffer data)
{
    std::string str;
    str.append("已接收 [")
        .append(ip).append(":").append(QString::number(port).toStdString()).append("] :");

    str.append(data.readableBytes(), data.numReadableBytes());

    socket->recv(EasyIO::ByteBuffer());

    emit textNeedPrint(ui->txtedtMsg, str.c_str());
}

void Widget::open()
{
    if (!m_client->open())
    {
        ui->txtedtMsg->append("打开失败");
    }
    else
    {
        ui->txtedtMsg->append("已打开 " + QString::number(m_client->localPort()));
        m_client->recv(EasyIO::ByteBuffer());
    }
}

void Widget::close()
{
    m_client->close();
    emit textNeedPrint(ui->txtedtMsg, "已关闭");
}

void Widget::send()
{
    std::string host;
    unsigned short port;

    if (ui->ledtHost->text().isEmpty())
    {
        QMessageBox::information(NULL, "提示", "地址不能为空");
        return;
    }

    host = ui->ledtHost->text().toStdString();

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


    std::string text = ui->txtedtInput->toPlainText().toStdString();
    if (text.empty())
    {
        return;
    }

    EasyIO::ByteBuffer buf(text.c_str(), text.length() + 1);
    m_client->send(host, port, buf);

    QString str;
    str.append(" 已发送 [")
        .append(host.c_str()).append(":").append(QString::number(port)).append("] :")
        .append(text.c_str())
        .append("");

    emit textNeedPrint(ui->txtedtMsg, str);
}

void Widget::printText(QTextEdit *control, QString str)
{
    control->append(QDateTime::currentDateTime().toString());
    control->append(str.append("\n"));
}
