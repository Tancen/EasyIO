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
    m_server->onDisconnected = std::bind(&Widget::whenDisconnected, this, _1);
    m_server->onBufferSent = std::bind(&Widget::whenBufferSent, this, _1, _2);
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
    pCount->bytesWrite = 0;
    pCount->countRead = 0;
    pCount->countWrite = 0;
    pCount->t = QDateTime::currentDateTime();
    con->bindUserdata(pCount);

    EasyIO::AutoBuffer buf(TEST_DEFAULT_DATA_SIZ_TCP);
    if(!con->recv(buf))
    {
        con->disconnect();
    }
}

void Widget::whenDisconnected(EasyIO::TCP::IConnection *con)
{
    Count *pCount = (Count *)(con->userdata());

    float bytesReadPer, bytesWritePer, countReadPer, countWritePer, cost;
    cost = pCount->t.msecsTo(QDateTime::currentDateTime()) / 1000.0;
    bytesReadPer = pCount->bytesRead / cost;
    bytesWritePer = pCount->bytesWrite / cost;
    countReadPer = pCount->countRead / cost;
    countWritePer = pCount->countWrite / cost;


    emit textNeedPrint(ui->txtedtMsg, QString::asprintf(
                "%s:%u 已断开\n"
                "共发送 %lld 次 %lld 字节, 接收 %lld 次 %lld 字节\n"
                "平均每秒发送 %f 次 %f 字节, 接收 %f 次 %f 字节",
                con->peerIP().c_str(), con->peerPort(),
                pCount->countWrite.load(), pCount->bytesWrite.load(), pCount->countRead.load(), pCount->bytesRead.load(),
                countWritePer, bytesWritePer, countReadPer, bytesReadPer));

    delete pCount;
}

void Widget::whenBufferSent(EasyIO::TCP::IConnection *con, EasyIO::AutoBuffer data)
{
    Count *pCount = (Count *)(con->userdata());
    pCount->countWrite++;
    pCount->bytesWrite += data.size();
}

void Widget::whenBufferReceived(EasyIO::TCP::IConnection *con, EasyIO::AutoBuffer data)
{
    QString str;
    Count *pCount = (Count *)(con->userdata());

    str.append(con->peerIP().c_str())
        .append(":")
        .append(QString::number(con->peerPort()))
        .append(":");

    if (data.size() != TEST_DEFAULT_DATA_SIZ_TCP)
    {
        str.append("接收到异常数据, 期望接收到 " +  QString::number(TEST_DEFAULT_DATA_SIZ_TCP)
                   + " 字节，实际字节 " + QString::number(data.size()));
        emit textNeedPrint(ui->txtedtMsg, str);
    }
    else
    {
        EasyIO::AutoBuffer buf(data.data(), data.size());

        pCount->countRead++;
        pCount->bytesRead += data.size();

        data.resize();
        buf.resize();
        if(!con->send(buf) || !con->recv(data))
        {
            con->disconnect();
        }
    }
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
