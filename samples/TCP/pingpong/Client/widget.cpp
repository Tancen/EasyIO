#include "widget.h"
#include "ui_widget.h"
#include <QMessageBox>
#include <qdatetime.h>
#include <qtimer.h>

using namespace std::placeholders;

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    m_bufSend(TEST_DEFAULT_DATA_SIZ_TCP),
    m_bufRecv(TEST_DEFAULT_DATA_SIZ_TCP)
{
    ui->setupUi(this);

    QObject::connect(ui->btnConnect, SIGNAL(clicked(bool)), this, SLOT(connect()));
    QObject::connect(ui->btnDisconnect, SIGNAL(clicked(bool)), this, SLOT(disconnect()));
    QObject::connect(this, SIGNAL(textNeedPrint(QTextEdit*,QString)),
            this, SLOT(printText(QTextEdit*,QString)));

    m_client = EasyIO::TCP::Client::create();
    if (!m_client.get())
    {
        QMessageBox::critical(NULL, "初始化失败", "初始化 client 失败");
        exit(1);
        return;
    }

    m_client->onConnected = std::bind(&Widget::whenConnected, this, _1);
    m_client->onConnectFailed = std::bind(&Widget::whenConnectFailed, this, _1);
    m_client->onDisconnected = std::bind(&Widget::whenDisconnected, this, _1);
    m_client->onBufferSent = std::bind(&Widget::whenBufferSent, this, _1, _2);
    m_client->onBufferReceived = std::bind(&Widget::whenBufferReceived, this, _1, _2);
}

Widget::~Widget()
{
    m_client.reset();
    delete ui;
}

void Widget::whenConnected(EasyIO::TCP::IConnection *)
{
    emit textNeedPrint(ui->txtedtMsg, "已连接");

    m_count.bytesRead = 0;
    m_count.bytesWrite = 0;
    m_count.countRead = 0;
    m_count.countWrite = 0;
    m_count.t = QDateTime::currentDateTime();

    m_client->enableKeepalive();
    m_client->setSendBufferSize(128 * 1024);
    m_client->setReceiveBufferSize(128 * 1024);
    m_client->setLinger(1, 0);

    m_bufSend.resize();
    m_bufRecv.resize();
    if(!m_client->send(m_bufSend) || !m_client->recv(m_bufRecv))
    {
        m_client->disconnect();
    }
}

void Widget::whenConnectFailed(EasyIO::TCP::IConnection *con)
{
    emit textNeedPrint(ui->txtedtMsg, "连接失败，错误 " + QString::number(con->lastSystemError()) + "");
}

void Widget::whenDisconnected(EasyIO::TCP::IConnection *)
{
    float bytesReadPer, bytesWritePer, countReadPer, countWritePer, cost;
    cost = m_count.t.msecsTo(QDateTime::currentDateTime()) / 1000.0;
    bytesReadPer = m_count.bytesRead / cost;
    bytesWritePer = m_count.bytesWrite / cost;
    countReadPer = m_count.countRead / cost;
    countWritePer = m_count.countWrite / cost;

    emit textNeedPrint(ui->txtedtMsg, QString::asprintf(
                "已断开\n"
                "共发送 %lld 次 %lld 字节, 接收 %lld 次 %lld 字节\n"
                "平均每秒发送 %f 次 %f 字节, 接收 %f 次 %f 字节",
                m_count.countWrite.load(), m_count.bytesWrite.load(), m_count.countRead.load(), m_count.bytesRead.load(),
                countWritePer, bytesWritePer, countReadPer, bytesReadPer));
}

void Widget::whenBufferSent(EasyIO::TCP::IConnection *, EasyIO::AutoBuffer data)
{
    m_count.countWrite++;
    m_count.bytesWrite += data.size();
}

void Widget::whenBufferReceived(EasyIO::TCP::IConnection *, EasyIO::AutoBuffer data)
{
    QString str;
    str.append(":");

    if (data.size() != TEST_DEFAULT_DATA_SIZ_TCP)
    {
        str.append("接收到异常数据, 期望接收到 " +  QString::number(TEST_DEFAULT_DATA_SIZ_TCP)
                   + " 字节，实际字节 " + QString::number(data.size()));
        emit textNeedPrint(ui->txtedtMsg, str);
    }
    else
    {
        m_count.countRead++;
        m_count.bytesRead += data.size();

        m_bufSend.resize();
        m_bufRecv.resize();
        if(!m_client->send(m_bufSend) || !m_client->recv(m_bufRecv))
        {
            m_client->disconnect();
        }
    }
}

void Widget::connect()
{
    QString host;
    unsigned short port;

    if (ui->ledtHost->text().isEmpty())
    {
        QMessageBox::information(NULL, "提示", "地址不能为空");
        return;
    }

    host = ui->ledtHost->text();

    if (ui->ledtPort->text().isEmpty())
    {
        QMessageBox::information(NULL, "提示", "端口不能为空");
        return;
    }

    bool isOk;
    port = ui->ledtPort->text().toShort(&isOk);
    if(!isOk || !port)
    {
        QMessageBox::information(NULL, "提示", "端口不是有效的数字");
        return;
    }

    if (!m_client->connect(host.toStdString(), port))
    {
        ui->txtedtMsg->append("连接失败");
    }
}

void Widget::disconnect()
{
    if(!m_client->disconnect())
    {
        ui->txtedtMsg->append("断开连接失败");
    }
}

void Widget::printText(QTextEdit *control, QString str)
{
    control->append(QDateTime::currentDateTime().toString());
    control->append(str.append("\n"));
}
