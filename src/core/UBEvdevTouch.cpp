#include "UBEvdevTouch.h"

#if defined(Q_OS_LINUX) && defined(HAVE_LIBEVDEV)
#include <libevdev/libevdev.h>
#include <fcntl.h>
#include <unistd.h>
#include <QGuiApplication>
#include <QScreen>
#include <QtMath>

UBEvdevTouch* UBEvdevTouch::instance()
{
    static UBEvdevTouch inst;
    return &inst;
}

UBEvdevTouch::UBEvdevTouch()
    : m_fd(-1)
    , m_dev(nullptr)
    , m_currentSlot(0)
    , m_scaleX(1.0)
    , m_scaleY(1.0)
{
    initDevice();
    if (m_fd >= 0)
        start();
}

UBEvdevTouch::~UBEvdevTouch()
{
    requestInterruption();
    wait(100);
    if (m_dev)
        libevdev_free(m_dev);
    if (m_fd >= 0)
        ::close(m_fd);
}

void UBEvdevTouch::initDevice()
{
    for (int i = 0; i < 32; ++i) {
        QString path = QString("/dev/input/event%1").arg(i);
        int fd = ::open(path.toUtf8().constData(), O_RDONLY | O_NONBLOCK);
        if (fd < 0)
            continue;
        libevdev* dev = nullptr;
        if (libevdev_new_from_fd(fd, &dev) < 0) {
            ::close(fd);
            continue;
        }
        if (libevdev_has_event_code(dev, EV_ABS, ABS_MT_SLOT) &&
            libevdev_has_event_code(dev, EV_ABS, ABS_MT_POSITION_X) &&
            libevdev_has_event_code(dev, EV_ABS, ABS_MT_TOUCH_MAJOR)) {
            m_fd = fd;
            m_dev = dev;
            break;
        }
        libevdev_free(dev);
        ::close(fd);
    }
    if (m_fd >= 0) {
        const struct input_absinfo* ax = libevdev_get_abs_info(m_dev, ABS_MT_POSITION_X);
        const struct input_absinfo* ay = libevdev_get_abs_info(m_dev, ABS_MT_POSITION_Y);
        QScreen* screen = QGuiApplication::primaryScreen();
        if (ax && ay && screen) {
            m_scaleX = screen->geometry().width() / double(ax->maximum - ax->minimum);
            m_scaleY = screen->geometry().height() / double(ay->maximum - ay->minimum);
        }
    }
}

void UBEvdevTouch::run()
{
    if (m_fd < 0)
        return;
    input_event ev;
    while (!isInterruptionRequested()) {
        int rc = libevdev_next_event(m_dev, LIBEVDEV_READ_FLAG_BLOCKING, &ev);
        if (rc != 0)
            continue;
        QMutexLocker lock(&m_mutex);
        if (ev.type == EV_ABS) {
            if (ev.code == ABS_MT_SLOT) {
                m_currentSlot = ev.value;
            } else {
                SlotInfo &slot = m_slots[m_currentSlot];
                switch (ev.code) {
                case ABS_MT_POSITION_X:
                    slot.x = ev.value;
                    break;
                case ABS_MT_POSITION_Y:
                    slot.y = ev.value;
                    break;
                case ABS_MT_TOUCH_MAJOR:
                    slot.major = ev.value;
                    break;
                case ABS_MT_TRACKING_ID:
                    slot.tracking = ev.value;
                    break;
                default:
                    break;
                }
            }
        }
    }
}

int UBEvdevTouch::majorNear(const QPointF& pos, int radius)
{
    if (m_fd < 0)
        return 0;
    QMutexLocker lock(&m_mutex);
    double tx = pos.x() / m_scaleX;
    double ty = pos.y() / m_scaleY;
    double r = radius / m_scaleX;
    double r2 = r * r;
    int best = 0;
    for (const SlotInfo &slot : m_slots) {
        if (slot.tracking < 0)
            continue;
        double dx = slot.x - tx;
        double dy = slot.y - ty;
        if (dx*dx + dy*dy <= r2) {
            int maj = slot.major * m_scaleX;
            if (maj > best)
                best = maj;
        }
    }
    return best;
}

#endif // Q_OS_LINUX && HAVE_LIBEVDEV
