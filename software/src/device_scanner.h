#ifndef DEVICE_SCANNER_H
#define DEVICE_SCANNER_H

#include <QList>
#include <QObject>

class ModbusRtu;
class QTimer;

/// Finds Redflow batteries via modbus RTU
class DeviceScanner : public QObject
{
	Q_OBJECT
public:
	DeviceScanner(ModbusRtu *modbus, QObject *parent);

	int scanInterval() const;

	void setScanInterval(int i);

signals:
	void deviceFound(int address);

private slots:
	void onReadCompleted(int function, quint8 slaveAddress, const QList<quint16> &values);

	void onWriteCompleted(int function, quint8 slaveAddress, quint16 address, quint16 value);

	void onErrorReceived(int errorType, quint8 slaveAddress, int exception);

	void onTimer();

private:
	quint8 getNextCandidateAddress() const;

	quint8 getNextScanAddress(quint8 address);

	void addNewDevice(quint8 address);

	void scanAddress(quint8 address);

	ModbusRtu *mModbus;
	int mScanInterval;
	quint8 mProbedAddress;
	quint8 mNewDeviceAddress;
	quint8 mAutoScanAddress;
	quint8 mMaxAddress;
};

#endif // DEVICE_SCANNER_H
