#ifndef SWFTOHTML5_H
#define SWFTOHTML5_H

#include <QDialog>
#include <zlib.h>
class QString;
class QStringList;
class QLabel;
class QVBoxLayout;
class QPushButton;
class QJsonObject;
class QListWidget;
class QThread;

#define GZIP_WINDOWS_BIT 15 + 16
#define GZIP_CHUNK_SIZE 32 * 1024

class SwfToHtml5 : public QDialog
{
    Q_OBJECT

public:
    explicit SwfToHtml5(QWidget *parent = 0);
    void parseJSON(QJsonObject);
    bool gzipDecompress(QByteArray input, QByteArray &output);

    ~SwfToHtml5();

public slots:
    void loadFile();
    void process();

private:
    QStringList m_pathToFiles;
    QString m_nameOfFile;

    QLabel *m_pStatusLbl;
    QLabel *m_pPathToFileLbl;
    QVBoxLayout *m_pMainLayout;

    QPushButton *m_pLoadFileBtn;
    QPushButton *m_pCloseBtn;
    QPushButton *m_pBeginConvertBtn;

    QListWidget *m_filesLstWgt;
    QThread *m_pThread;
};

#endif // SWFTOHTML5_H
