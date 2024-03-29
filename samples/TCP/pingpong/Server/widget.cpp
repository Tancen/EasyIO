#include "widget.h"
#include "ui_widget.h"
#include <QMessageBox>
#include <QTime>
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

    m_server = EasyIO::TCP::Server::create();
    if (!m_server.get())
    {
        QMessageBox::critical(NULL, "初始化失败", "初始化 server 失败");
        exit(1);
        return;
    }

    m_server->onConnected = std::bind(&Widget::whenConnected, this, _1);
    m_server->onDisconnected = std::bind(&Widget::whenDisconnected, this, _1, _2);
    m_server->onBufferReceived = std::bind(&Widget::whenBufferReceived, this, _1, _2);
}

Widget::~Widget()
{
    m_server.reset();;
    delete ui;
}

void Widget::whenConnected(EasyIO::TCP::IConnection *con)
{
    QString str;

    str.append(con->peerIP().c_str())
        .append(":")
        .append(QString::number(con->peerPort()))
        .append(" 已连接");

    emit textNeedPrint(ui->txtedtMsg, str);

    con->enableKeepalive();
    con->setSendBufferSize(128 * 1024);
    con->setReceiveBufferSize(128 * 1024);
    con->setLinger(1, 0);

    Count *pCount = new Count();
    pCount->bytesRead = 0;
    pCount->countRead = 0;
    pCount->t = QDateTime::currentDateTime();
    con->bindUserdata(pCount);

    con->recv(EasyIO::ByteBuffer());
}

void Widget::whenDisconnected(EasyIO::TCP::IConnection *con, const std::string& reason)
{
    Count *pCount = (Count *)(con->userdata());

    float bytesReadPer, countReadPer, cost;
    cost = pCount->t.msecsTo(QDateTime::currentDateTime()) / 1000.0;
    bytesReadPer = pCount->bytesRead / cost;
    countReadPer = pCount->countRead / cost;


    emit textNeedPrint(ui->txtedtMsg, QString::asprintf(
                "%s:%u 已断开\n"
                "共接收 %lld 次 %lld 字节\n"
                "平均每秒接收 %f 次 %f 字节",
                con->peerIP().c_str(), con->peerPort(),
                pCount->countRead.load(), pCount->bytesRead.load(),
                countReadPer, bytesReadPer));

    delete pCount;
}

void Widget::whenBufferReceived(EasyIO::TCP::IConnection *con, EasyIO::ByteBuffer data)
{
    Count *pCount = (Count *)(con->userdata());
    pCount->countRead++;
    pCount->bytesRead += data.numReadableBytes();

    con->send(data);
    con->recv(EasyIO::ByteBuffer());
}

void Widget::open()
{
    bool isOk;
    unsigned short port;


    port = ui->ledtPort->text().toShort(&isOk);
    if(!isOk || !port)
    {
        QMessageBox::information(NULL, "提示", "端口不是有效的数字");
        return;
    }

    if (!m_server->open(port))
    {
        ui->txtedtMsg->append("开启失败");
    }
    else
    {
        ui->txtedtMsg->append("已开启");
    }
}

void Widget::close()
{
    m_server->close();
    ui->txtedtMsg->append("已关闭");
}

void Widget::printText(QTextEdit *control, QString str)
{
    control->append(QDateTime::currentDateTime().toString());
    control->append(str.append("\n"));
}
