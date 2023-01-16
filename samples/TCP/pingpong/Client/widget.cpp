#include "widget.h"
#include "ui_widget.h"
#include <QMessageBox>
#include <qdatetime.h>
#include <qtimer.h>

using namespace std::placeholders;

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
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
    m_client->onConnectFailed = std::bind(&Widget::whenConnectFailed, this, _1, _2);
    m_client->onDisconnected = std::bind(&Widget::whenDisconnected, this, _1, _2);
    m_client->onBufferReceived = std::bind(&Widget::whenBufferReceived, this, _1, _2);
}

Widget::~Widget()
{
    m_client->syncDisconnect();
    delete ui;
}

void Widget::whenConnected(EasyIO::TCP::IConnection *)
{
    emit textNeedPrint(ui->txtedtMsg, "已连接");

    m_count.bytesRead = 0;
    m_count.countRead = 0;
    m_count.t = QDateTime::currentDateTime();

    m_client->enableKeepalive();
    m_client->setSendBufferSize(128 * 1024);
    m_client->setReceiveBufferSize(128 * 1024);
    m_client->setLinger(1, 0);

    m_client->recv(EasyIO::ByteBuffer());

    EasyIO::ByteBuffer data;
    data.ensureWritable(4096);
    data.moveWriterIndex(4096);
    m_client->send(data);
}

void Widget::whenConnectFailed(EasyIO::TCP::IConnection *con, const std::string& reason)
{
    emit textNeedPrint(ui->txtedtMsg, QString("连接失败: ") + reason.c_str());
}

void Widget::whenDisconnected(EasyIO::TCP::IConnection *, const std::string& reason)
{
    float bytesReadPer, countReadPer, cost;
    cost = m_count.t.msecsTo(QDateTime::currentDateTime()) / 1000.0;
    bytesReadPer = m_count.bytesRead / cost;
    countReadPer = m_count.countRead / cost;

    emit textNeedPrint(ui->txtedtMsg, QString::asprintf(
                "已断开\n"
                "共接收 %lld 次 %lld 字节\n"
                "平均每秒接收 %f 次 %f 字节",
                m_count.countRead.load(), m_count.bytesRead.load(),
                countReadPer, bytesReadPer));
}

void Widget::whenBufferReceived(EasyIO::TCP::IConnection *con, EasyIO::ByteBuffer data)
{
    m_count.countRead++;
    m_count.bytesRead += data.numReadableBytes();

    con->send(data);
    con->recv(EasyIO::ByteBuffer());
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

    m_client->connect(host.toStdString(), port);
}

void Widget::disconnect()
{
    m_client->disconnect();
}

void Widget::printText(QTextEdit *control, QString str)
{
    control->append(QDateTime::currentDateTime().toString());
    control->append(str.append("\n"));
}
