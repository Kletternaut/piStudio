// ResourceBroker.cpp
// MCIM Phase 0, Schritt 0.3

#include "ResourceBroker.h"
#include <QDir>
#include <QMutexLocker>
#include <QDebug>

ResourceBroker::ResourceBroker(QObject *parent)
    : QObject(parent)
{
    // Standard V4L2 Subdevices
    m_v4l2Devices[0] = "/dev/v4l-subdev0";
    m_v4l2Devices[1] = "/dev/v4l-subdev1";
}

int ResourceBroker::allocatePort(int cameraIndex, PortRole role) const
{
    return basePort(role) + cameraIndex * PORT_OFFSET;
}

bool ResourceBroker::tryAcquireHailo(int cameraIndex)
{
    QMutexLocker lock(&m_mutex);
    if (m_hailoOwner == -1 || m_hailoOwner == cameraIndex) {
        m_hailoOwner = cameraIndex;
        qDebug() << "ResourceBroker: Hailo8L acquired by Camera" << cameraIndex;
        emit hailoOwnerChanged(cameraIndex);
        return true;
    }
    qDebug() << "ResourceBroker: Hailo8L already owned by Camera" << m_hailoOwner
             << "– Camera" << cameraIndex << "denied";
    return false;
}

void ResourceBroker::releaseHailo(int cameraIndex)
{
    QMutexLocker lock(&m_mutex);
    if (m_hailoOwner == cameraIndex) {
        m_hailoOwner = -1;
        qDebug() << "ResourceBroker: Hailo8L released by Camera" << cameraIndex;
        emit hailoOwnerChanged(-1);
    }
}

int ResourceBroker::hailoOwner() const
{
    QMutexLocker lock(&m_mutex);
    return m_hailoOwner;
}

QString ResourceBroker::outputPath(int cameraIndex) const
{
    if (m_outputBasePath.isEmpty()) {
        return QString("cam%1").arg(cameraIndex);
    }
    return QDir(m_outputBasePath).filePath(QString("cam%1").arg(cameraIndex));
}

void ResourceBroker::setOutputBasePath(const QString &basePath)
{
    m_outputBasePath = basePath;
}

QString ResourceBroker::v4l2SubDevice(int cameraIndex) const
{
    return m_v4l2Devices.value(cameraIndex,
        QString("/dev/v4l-subdev%1").arg(cameraIndex));
}

void ResourceBroker::setV4l2SubDevice(int cameraIndex, const QString &device)
{
    m_v4l2Devices[cameraIndex] = device;
}

int ResourceBroker::basePort(PortRole role)
{
    switch (role) {
        case PortRole::GStreamerRTSP:  return 8554;
        case PortRole::StreamViewer:   return 8555;
        case PortRole::UdpInference:   return 8567;
        case PortRole::TcpOutput:      return 8888;
    }
    return 8554;
}