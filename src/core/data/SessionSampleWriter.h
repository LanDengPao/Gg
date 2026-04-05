#pragma once

#include "AppTypes.h"

#include <QFile>

class QDataStream;

// 将采集到的鼠标采样持续写入会话对应的二进制样本文件。
class SessionSampleWriter
{
public:
    SessionSampleWriter();
    ~SessionSampleWriter();

    // 打开新的样本文件并写入文件头。
    bool start(const QString& filePath, QString* error = nullptr);
    // 按仓储层约定的二进制格式追加一条采样记录。
    bool append(const MouseSample& sample, QString* error = nullptr);
    void close();

    bool isOpen() const;
    qint64 sampleCount() const;

private:
    QFile m_file;
    QDataStream* m_stream = nullptr;
    qint64 m_sampleCount = 0;
};
