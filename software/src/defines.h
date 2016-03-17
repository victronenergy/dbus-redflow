#ifndef DEFINES_H
#define DEFINES_H

#include <QByteArray>
#include <QtGlobal>

inline quint8 msb(quint16 d)
{
	return d >> 8;
}

inline quint8 lsb(quint16 d)
{
	return d & 0xFF;
}

inline quint16 toUInt16(quint8 msb, quint8 lsb)
{
	return (msb << 8) | lsb;
}

inline quint16 toUInt16(const QByteArray &a, int offset)
{
	return toUInt16(static_cast<quint8>(a[offset]), static_cast<quint8>(a[offset + 1]));
}

#endif // DEFINES_H
