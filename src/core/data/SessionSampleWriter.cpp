#include "SessionSampleWriter.h"

#include <QDataStream>

namespace {
constexpr quint32 kSampleMagic = 0x47475331;
constexpr quint16 kSampleVersion = 1;
}

SessionSampleWriter::SessionSampleWriter() = default;

SessionSampleWriter::~SessionSampleWriter()
{
    close();
}

bool SessionSampleWriter::start(const QString& filePath, QString* error)
{
    close();
    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = uiText("Failed to create the sample file.", "无法创建采样文件");
        }
        return false;
    }

    m_stream = new QDataStream(&m_file);
    m_stream->setByteOrder(QDataStream::LittleEndian);
    (*m_stream) << kSampleMagic << kSampleVersion;
    if (m_stream->status() != QDataStream::Ok) {
        if (error) {
            *error = uiText("Failed to initialize the sample file.", "无法初始化采样文件");
        }
        close();
        return false;
    }

    m_sampleCount = 0;
    return true;
}

bool SessionSampleWriter::append(const MouseSample& sample, QString* error)
{
    if (!m_stream) {
        if (error) {
            *error = uiText("The sample file is not open.", "采样文件未打开");
        }
        return false;
    }

    (*m_stream) << sample.timestampUs
                << static_cast<qint32>(sample.position.x())
                << static_cast<qint32>(sample.position.y())
                << static_cast<qint32>(sample.delta.x())
                << static_cast<qint32>(sample.delta.y())
                << static_cast<qint32>(sample.wheelDelta)
                << sample.buttonFlags
                << sample.eventType;
    if (m_stream->status() != QDataStream::Ok) {
        if (error) {
            *error = uiText("Failed to write the sample data.", "写入采样数据失败");
        }
        return false;
    }

    ++m_sampleCount;
    return true;
}

void SessionSampleWriter::close()
{
    delete m_stream;
    m_stream = nullptr;
    if (m_file.isOpen()) {
        m_file.close();
    }
}

bool SessionSampleWriter::isOpen() const
{
    return m_stream != nullptr;
}

qint64 SessionSampleWriter::sampleCount() const
{
    return m_sampleCount;
}
