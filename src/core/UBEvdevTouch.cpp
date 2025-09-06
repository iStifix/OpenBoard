#include "UBEvdevTouch.h"

#include <QGuiApplication>
#include <QScreen>
#include <QMutexLocker>

#ifdef Q_OS_LINUX
#  ifdef HAVE_LIBEVDEV
#    include <libevdev/libevdev.h>
#    include <fcntl.h>
#    include <unistd.h>
#    include <errno.h>
#  endif
#endif

UBEvdevTouch* UBEvdevTouch::instance()
{
    static UBEvdevTouch inst;
    return &inst;
}

UBEvdevTouch::UBEvdevTouch()
{
    initDevice();
#ifdef Q_OS_LINUX
#  ifdef HAVE_LIBEVDEV
    if (m_fd >= 0 && m_dev)
        start();
#  endif
#endif
}

UBEvdevTouch::~UBEvdevTouch()
{
    requestInterruption();
    wait(100);

#ifdef Q_OS_LINUX
#  ifdef HAVE_LIBEVDEV
    if (m_dev) libevdev_free(m_dev);
    if (m_fd >= 0) ::close(m_fd);
#  endif
#endif
}

void UBEvdevTouch::initDevice()
{
#ifdef Q_OS_LINUX
#  ifdef HAVE_LIBEVDEV
    for (int i = 0; i < 64; ++i) {
        const QByteArray path = QByteArray("/dev/input/event") + QByteArray::number(i);
        int fd = ::open(path.constData(), O_RDONLY | O_NONBLOCK);
        if (fd < 0)
            continue;

        libevdev* dev = nullptr;
        if (libevdev_new_from_fd(fd, &dev) < 0) {
            ::close(fd);
            continue;
        }

        if (libevdev_has_event_code(dev, EV_ABS, ABS_MT_SLOT) &&
            libevdev_has_event_code(dev, EV_ABS, ABS_MT_POSITION_X) &&
            libevdev_has_event_code(dev, EV_ABS, ABS_MT_POSITION_Y) &&
            libevdev_has_event_code(dev, EV_ABS, ABS_MT_TOUCH_MAJOR))
        {
            m_fd = fd;
            m_dev = dev;
            break;
        }

        libevdev_free(dev);
        ::close(fd);
    }

    if (m_fd >= 0 && m_dev) {
        if (const input_absinfo* ax = libevdev_get_abs_info(m_dev, ABS_MT_POSITION_X)) {
            m_ax.min = ax->minimum; m_ax.max = ax->maximum;
        }
        if (const input_absinfo* ay = libevdev_get_abs_info(m_dev, ABS_MT_POSITION_Y)) {
            m_ay.min = ay->minimum; m_ay.max = ay->maximum;
        }

        int slotCount = 0;
        if (const input_absinfo* as = libevdev_get_abs_info(m_dev, ABS_MT_SLOT))
            slotCount = as->maximum + 1;
        if (slotCount <= 0) slotCount = 16;

        m_slots.resize(slotCount);
        for (auto& s : m_slots) {
            s.tracking = -1;
            s.x = s.y = s.major = 0;
        }
    }
#  endif
#endif
}

void UBEvdevTouch::run()
{
#ifdef Q_OS_LINUX
#  ifdef HAVE_LIBEVDEV
    if (m_fd < 0 || !m_dev)
        return;

    input_event ev;
    while (!isInterruptionRequested()) {
        int rc = libevdev_next_event(m_dev, LIBEVDEV_READ_FLAG_BLOCKING, &ev);
        if (rc == LIBEVDEV_READ_STATUS_SYNC) {
            while (rc == LIBEVDEV_READ_STATUS_SYNC)
                rc = libevdev_next_event(m_dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
            continue;
        }
        if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
            if (ev.type == EV_ABS) {
                QMutexLocker lock(&m_mutex);
                if (ev.code == ABS_MT_SLOT) {
                    m_currentSlot = ev.value;
                    if (m_currentSlot < 0 || m_currentSlot >= m_slots.size())
                        m_currentSlot = 0;
                } else if (m_currentSlot >= 0 && m_currentSlot < m_slots.size()) {
                    SlotInfo& slot = m_slots[m_currentSlot];
                    switch (ev.code) {
                        case ABS_MT_POSITION_X:  slot.x = ev.value; break;
                        case ABS_MT_POSITION_Y:  slot.y = ev.value; break;
                        case ABS_MT_TOUCH_MAJOR: slot.major = ev.value; break;
                        case ABS_MT_TRACKING_ID: slot.tracking = ev.value; break; // -1 = up
                        default: break;
                    }
                }
            }
        } else if (rc == -EAGAIN) {
            msleep(1);
        } else {
            msleep(5);
        }
    }
#  endif
#endif
}

int UBEvdevTouch::majorNearGlobal(const QPoint& globalPos, int radiusPx)
{
#ifdef Q_OS_LINUX
#  ifdef HAVE_LIBEVDEV
    if (m_fd < 0 || !m_dev || m_ax.span() <= 0 || m_ay.span() <= 0)
        return 0;

    QScreen* scr = QGuiApplication::screenAt(globalPos);
    if (!scr) scr = QGuiApplication::primaryScreen();
    if (!scr) return 0;

    const QRect sg = scr->geometry();

    const double sx = double(m_ax.span()) / qMax(1, sg.width());
    const double sy = double(m_ay.span()) / qMax(1, sg.height());

    const double devX = m_ax.min + (globalPos.x() - sg.x()) * sx;
    const double devY = m_ay.min + (globalPos.y() - sg.y()) * sy;

    const double rX = radiusPx * sx;
    const double rY = radiusPx * sy;

    QMutexLocker lock(&m_mutex);

    int bestMajorDev = 0;
    for (const SlotInfo& s : m_slots) {
        if (s.tracking < 0) continue;
        const double dx = s.x - devX;
        const double dy = s.y - devY;
        if ((dx*dx)/(rX*rX + 1e-9) + (dy*dy)/(rY*rY + 1e-9) <= 1.0) {
            if (s.major > bestMajorDev)
                bestMajorDev = s.major;
        }
    }

    if (bestMajorDev <= 0) return 0;
    const double pxPerDevX = double(sg.width()) / qMax(1, m_ax.span());
    const int majorPx = int(bestMajorDev * pxPerDevX + 0.5);
    return majorPx;
#  else
    Q_UNUSED(globalPos)
    Q_UNUSED(radiusPx)
    return 0;
#  endif
#else
    Q_UNUSED(globalPos)
    Q_UNUSED(radiusPx)
    return 0;
#endif
}

