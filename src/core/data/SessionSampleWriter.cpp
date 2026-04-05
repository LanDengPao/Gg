#include "SessionSampleWriter.h"

#include <QCoreApplication>
#include <QDataStream>

namespace {
constexpr quint32 kSampleMagic = 0x47475331;
constexpr quint16 kSampleVersion = 1;
// 写入端和读取端共享同一份文件头定义，确保样本文件格式可校验。
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
            *error = QCoreApplication::translate("SessionSampleWriter", "Failed to create the sample file.");
        }
        return false;
    }

    m_stream = new QDataStream(&m_file);
    m_stream->setByteOrder(QDataStream::LittleEndian);
    // 先写入文件头，后续读取时可快速识别格式版本。
    (*m_stream) << kSampleMagic << kSampleVersion;
    if (m_stream->status() != QDataStream::Ok) {
        if (error) {
            *error = QCoreApplication::translate("SessionSampleWriter", "Failed to initialize the sample file.");
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
            *error = QCoreApplication::translate("SessionSampleWriter", "The sample file is not open.");
        }
        return false;
    }

    // 字段顺序必须与 WorkspaceRepository::loadSamples 中的读取顺序保持一致。
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
            *error = QCoreApplication::translate("SessionSampleWriter", "Failed to write the sample data.");
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
