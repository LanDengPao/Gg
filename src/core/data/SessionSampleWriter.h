#pragma once

#include "AppTypes.h"

#include <QFile>

class QDataStream;

class SessionSampleWriter
{
public:
    SessionSampleWriter();
    ~SessionSampleWriter();

    bool start(const QString& filePath, QString* error = nullptr);
    bool append(const MouseSample& sample, QString* error = nullptr);
    void close();

    bool isOpen() const;
    qint64 sampleCount() const;

private:
    QFile m_file;
    QDataStream* m_stream = nullptr;
    qint64 m_sampleCount = 0;
};
