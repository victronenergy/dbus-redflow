#include <QsLog.h>
#include <QTimer>
#include "device_scanner.h"
#include "modbus_rtu.h"

static const int DefaultAddress1 = 1;
static const int DefaultAddress2 = 99;

DeviceScanner::DeviceScanner(ModbusRtu *modbus, QObject *parent):
	QObject(parent),
	mModbus(modbus),
	mScanInterval(0),
	mProbedAddress(0),
	mNewDeviceAddress(0),
	mAutoScanAddress(DefaultAddress1 + 1),
	mMaxAddress(1)
{
	Q_ASSERT(modbus != 0);
	Q_ASSERT(settings != 0);
	scanAddress(DefaultAddress1);
	connect(mModbus, SIGNAL(readCompleted(int, quint8, const QList<quint16> &)),
			this, SLOT(onReadCompleted(int, quint8, QList<quint16>)));
	connect(mModbus, SIGNAL(writeCompleted(int, quint8, quint16, quint16)),
			this, SLOT(onWriteCompleted(int, quint8, quint16, quint16)));
	connect(mModbus, SIGNAL(errorReceived(int, quint8, int)),
			this, SLOT(onErrorReceived(int, quint8, int)));
}

int DeviceScanner::scanInterval() const
{
	return mScanInterval;
}

void DeviceScanner::setScanInterval(int i)
{
	mScanInterval = i;
}

void DeviceScanner::onReadCompleted(int function, quint8 slaveAddress,
									const QList<quint16> &values)
{
	Q_UNUSED(function);
	Q_UNUSED(values);
	if (slaveAddress != mProbedAddress)
		return;
	// We have a successful read. There are several options here:
	// * We found a new device mProbedAddress with default address (1 or 99)
	//   * Create a new candidate address (max address + 1) and try to read from
	//     it. On timeout, we know the address is not in use, so we can set
	//     it as new address for the device we just found.
	// * We found an existing device while checking a candidate
	//   * Create a new candidate and try again
	// * We found an existing device during a random device scan
	//   * Use straight away.
	if (mProbedAddress == DefaultAddress1 || mProbedAddress == DefaultAddress2) {
		mNewDeviceAddress = mProbedAddress;
		scanAddress(getNextCandidateAddress());
	} else if (mNewDeviceAddress > 0) {
		addNewDevice(mProbedAddress);
		scanAddress(getNextCandidateAddress());
	} else {
		addNewDevice(mProbedAddress);
		scanAddress(getNextScanAddress(mAutoScanAddress));
	}
}

void DeviceScanner::onWriteCompleted(int function, quint8 slaveAddress,
									 quint16 address, quint16 value)
{
	Q_UNUSED(function)
	Q_UNUSED(value)
	if (slaveAddress != mNewDeviceAddress || address != 0x9030)
		return;
	// We get here if mCandidate address has been written to the device.
	// Let's find out if the operation was successful.
	mMaxAddress = qMax(mMaxAddress, mProbedAddress);
	mNewDeviceAddress = 0;
	addNewDevice(mProbedAddress);
}

void DeviceScanner::onErrorReceived(int errorType, quint8 slaveAddress,
									int exception)
{
	Q_UNUSED(exception);
	if (slaveAddress != mProbedAddress)
		return;
	/// @todo EV This may also be a write error.
	if (errorType == ModbusRtu::Timeout) {
		// No device found. Options:
		// * No device found at candidate address. We can use this address for
		//   new device.
		// * Random device scan failed to find a device, try next address
		if (mNewDeviceAddress > 0) {
			QLOG_INFO() << "Change modbus address from" << mNewDeviceAddress
						<< "to" << mProbedAddress;
			mModbus->writeRegister(ModbusRtu::WriteSingleRegister,
								   mNewDeviceAddress, 0x9030, mProbedAddress);
		} else {
			scanAddress(getNextScanAddress(mProbedAddress));
		}
	} else {
		scanAddress(mProbedAddress);
	}
}

void DeviceScanner::onTimer()
{
	mModbus->readRegisters(ModbusRtu::ReadHoldingRegisters, mProbedAddress, 0x9010, 1);
}

int DeviceScanner::getNextCandidateAddress() const
{
	for (int a = mMaxAddress + 1;; ++a) {
		if (a != DefaultAddress1 && a != DefaultAddress2) {
			return a;
		}
	}
}

int DeviceScanner::getNextScanAddress(int address)
{
	if (address == DefaultAddress1)
		return DefaultAddress2;
	if (address == DefaultAddress2)
		return mAutoScanAddress;
	for (int a = address + 1;; ++a) {
		if (a > 254)
			a = DefaultAddress1 + 1;
		if (a == DefaultAddress2)
			a = DefaultAddress2 + 1;
		mAutoScanAddress = a;
		if (a % 10 == 0)
			a = DefaultAddress1;
		return a;
	}
}

void DeviceScanner::addNewDevice(int address)
{
	QLOG_INFO() << "New device found by scanner:" << address;
	if (address == DefaultAddress1 || address == DefaultAddress2) {
		QLOG_ERROR() << "Default modbus address reported as new address" << address;
		return;
	}
	mMaxAddress = qMax(mMaxAddress, address);
	emit deviceFound(address);
}

void DeviceScanner::scanAddress(int address)
{
	QLOG_TRACE() << "Polling modbus address" << address;
	mProbedAddress = address;
	if (mScanInterval < 250) {
		mModbus->readRegisters(ModbusRtu::ReadHoldingRegisters, address, 0x9010, 1);
	} else {
		QTimer::singleShot(mScanInterval - 250, this, SLOT(onTimer()));
	}
}
