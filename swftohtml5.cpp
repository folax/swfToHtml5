#include "swftohtml5.h"
#include <QFileDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDebug>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QUrl>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QListWidget>
#include <QThread>
#include <QEventLoop>
#include <QDataStream>
#include <QStringList>
#include <QPalette>
#include <QStyleFactory>
#include <QApplication>

static int cnt = 0;

SwfToHtml5::SwfToHtml5(QWidget *parent) : QDialog(parent)
{
    this->setWindowIcon(QIcon(":/images/images/icon.png"));
    QApplication::setStyle(new newStyle);
    setWindowFlags(windowFlags()|Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
    resize(400, 300);
    m_pStatusLbl = new QLabel(tr("Select the adobe flash files (*.swf)"), this);
    m_pStatusLbl->setAlignment(Qt::AlignCenter);
    m_pPathToFileLbl = new QLabel(tr(""), this);

    m_pLoadFileBtn = new QPushButton(tr("Load files"), this);
    m_pCloseBtn = new QPushButton(tr("Close"), this);
    m_pBeginConvertBtn = new QPushButton(tr("Start convert"), this);
    m_pBeginConvertBtn->setEnabled(false);

    m_filesLstWgt = new QListWidget(this);

    m_pMainLayout = new QVBoxLayout(this);
    m_pMainLayout->addWidget(m_pStatusLbl);
    m_pMainLayout->addWidget(m_filesLstWgt);
    m_pMainLayout->addWidget(m_pPathToFileLbl);
    m_pMainLayout->addWidget(m_pLoadFileBtn);
    m_pMainLayout->addWidget(m_pBeginConvertBtn);
    m_pMainLayout->addWidget(m_pCloseBtn);

    this->setLayout(m_pMainLayout);
    connect(m_pLoadFileBtn, &QPushButton::clicked, this, &SwfToHtml5::loadFile);
    connect(m_pCloseBtn, &QPushButton::clicked, this, &SwfToHtml5::close);
    connect(m_pBeginConvertBtn, &QPushButton::clicked, this, &SwfToHtml5::process);
}

void SwfToHtml5::loadFile()
{
    m_pathToFiles = QFileDialog::getOpenFileNames(this,
                                                  QObject::tr("Open adobe flash file"),
                                                  QCoreApplication::applicationDirPath(),
                                                  QObject::tr("swf files (*.swf)"));
    if (m_pathToFiles.isEmpty())
        return;
    //remove dublicats
    m_pathToFiles.removeDuplicates();
    m_filesLstWgt->clear();
    m_filesLstWgt->addItems(m_pathToFiles);
    m_pBeginConvertBtn->setEnabled(true);
}

void SwfToHtml5::process()
{
    m_pLoadFileBtn->setEnabled(false);
    m_pLoadFileBtn->setText("Convertation started!");
    if (m_pathToFiles.size() < 1)
        return;

    for (int i = 0; i < m_pathToFiles.size(); ++i) {
        if (QFileInfo(m_pathToFiles.at(i)).isFile()) {
            QFile inputFile(m_pathToFiles.at(i));
            QByteArray bufBa;
            m_nameOfFile = m_pathToFiles.at(i);
            m_nameOfFile = m_nameOfFile.remove(".swf");
            m_nameOfFile += ".html";
            //read swf file
            if (!inputFile.open(QIODevice::ReadOnly)) {
                qDebug()<<"Could not open the input file\n"<<m_pathToFiles.at(i);
                return;
            } else {
                while(!inputFile.atEnd()) {
                    bufBa += inputFile.readLine();
                }
            }
            bufBa = bufBa.toBase64();
            bufBa = bufBa.replace('+', '-');
            bufBa = bufBa.replace('/', '_');

            QUrl apiUrl("https://www.googleapis.com/rpc?key=AIzaSyCC_WIu0oVvLtQGzv4-g7oaWNoc-u8JpEI");
            QByteArray req = "{ \"apiVersion\": \"v1\",\"method\": \"swiffy.convertToHtml\", \"params\": { \"client\": \"Swiffy Flash Extension\", \"input\": \"$\" } }";
            req = req.replace('$', bufBa);

            QNetworkRequest request(apiUrl);

            QEventLoop eventLoop;

            request.setHeader(QNetworkRequest::ContentTypeHeader,"www.googleapis.com");

            QNetworkAccessManager *manager = new QNetworkAccessManager(this);

            connect(manager, &QNetworkAccessManager::finished, &eventLoop, &QEventLoop::quit);

            QNetworkReply *reply = manager->post(request, req);
            eventLoop.exec();

            QString dataFromReply;
            if (reply->error() == QNetworkReply::NoError) {
                dataFromReply = QString::fromUtf8(reply->readAll());
            }
            QByteArray replyBa;
            replyBa.append(dataFromReply);

            QFile replyFile("json.txt");
            replyFile.open(QIODevice::WriteOnly);
            replyFile.write(replyBa);
            replyFile.close();

            QFile jsonFile("json.txt");
            if (jsonFile.open(QIODevice::ReadOnly)) {
                QJsonParseError parseError;
                QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonFile.readAll(), &parseError);
                if (parseError.error == QJsonParseError::NoError) {
                    if(jsonDoc.isObject())
                        parseJSON(jsonDoc.object());
                }
                else
                    qDebug() << "JSON parse error: " << parseError.errorString();
            }
            else
                qDebug() << "json.txt not open";
            reply->deleteLater();
        }
        ++cnt;
    }
    m_pLoadFileBtn->setText("Load files");
    m_pLoadFileBtn->setEnabled(true);
    m_filesLstWgt->addItem("Finished!");
}

void SwfToHtml5::parseJSON(QJsonObject jsonObject)
{
    m_operationResult.append(jsonObject["result"].toObject()["response"].toObject()["status"].toString());
    QByteArray jsonData;
    jsonData.append(jsonObject["result"].toObject()["response"].toObject()["output"].toString());
    jsonData = jsonData.replace("-", "+");
    jsonData = jsonData.replace("_", "/");
    jsonData = QByteArray::fromBase64(jsonData);

    //decompress
    QByteArray decGzipBa;
    gzipDecompress(jsonData, decGzipBa);
    QFile html5File(m_nameOfFile);
    html5File.open(QIODevice::WriteOnly);
    html5File.write(decGzipBa);
    html5File.close();
    m_nameOfFile.clear();
    qDebug() << "parseJSON " + m_nameOfFile + " finished.";

    QListWidgetItem *lstItem = m_filesLstWgt->item(cnt);
    QString data;
    data = lstItem->text();
    data += " " + m_operationResult;
    lstItem->setText(data);
    m_operationResult.clear();
}

//http://stackoverflow.com/questions/2690328/qt-quncompress-gzip-data/24949005#24949005
bool SwfToHtml5::gzipDecompress(QByteArray input, QByteArray &output)
{
    output.clear();

    if(input.length() > 0) {
        z_stream strm;
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = 0;
        strm.next_in = Z_NULL;

        // Initialize inflater
        int ret = inflateInit2(&strm, GZIP_WINDOWS_BIT);

        if (ret != Z_OK)
            return(false);

        // Extract pointer to input data
        char *input_data = input.data();
        int input_data_left = input.length();

        // Decompress data until available
        do {
            // Determine current chunk size
            int chunk_size = qMin(GZIP_CHUNK_SIZE, input_data_left);

            // Check for termination
            if(chunk_size <= 0)
                break;

            // Set inflater references
            strm.next_in = (unsigned char*)input_data;
            strm.avail_in = chunk_size;

            // Update interval variables
            input_data += chunk_size;
            input_data_left -= chunk_size;

            // Inflate chunk and cumulate output
            do {

                // Declare vars
                char out[GZIP_CHUNK_SIZE];

                // Set inflater references
                strm.next_out = (unsigned char*)out;
                strm.avail_out = GZIP_CHUNK_SIZE;

                // Try to inflate chunk
                ret = inflate(&strm, Z_NO_FLUSH);

                switch (ret) {
                case Z_NEED_DICT:
                    ret = Z_DATA_ERROR;
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                case Z_STREAM_ERROR:
                    // Clean-up
                    inflateEnd(&strm);

                    // Return
                    return(false);
                }

                // Determine decompressed size
                int have = (GZIP_CHUNK_SIZE - strm.avail_out);

                // Cumulate result
                if(have > 0)
                    output.append((char*)out, have);

            } while (strm.avail_out == 0);

        } while (ret != Z_STREAM_END);

        // Clean-up
        inflateEnd(&strm);

        // Return
        return (ret == Z_STREAM_END);
    }
    else
        return(true);
}

SwfToHtml5::~SwfToHtml5()
{

}


newStyle::newStyle() : QProxyStyle(QStyleFactory::create("Fusion"))
{

}

void newStyle::polish(QPalette &darkPalette)
{
    darkPalette.setColor(QPalette::Window, QColor(53,53,53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25,25,25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(0,102,0));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
}
