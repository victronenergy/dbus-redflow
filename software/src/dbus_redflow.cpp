#include <QsLog.h>
#include <QTimer>
#include "battery_controller_bridge.h"
#include "battery_controller_settings.h"
#include "battery_controller_updater.h"
#include "battery_controller.h"
#include "battery_summary.h"
#include "battery_summary_bridge.h"
#include "bms_service.h"
#include "dbus_redflow.h"
#include "device_scanner.h"

DBusRedflow::DBusRedflow(const QString &portName, QObject *parent):
	QObject(parent),
	mDeviceScanner(0),
	mModbus(new ModbusRtu(portName, 19200, this)),
	mPortName(portName),
	mSummary(0),
	mBmsService(0)
{
	qRegisterMetaType<ConnectionState>();
	qRegisterMetaType<QList<quint16> >();

	connect(mModbus, SIGNAL(serialEvent(const char *)),
			this, SLOT(onSerialEvent(const char *)));

//	// Permanently monitor modbus address 1 to 10 and 99. If you want to try the
//	// device scanner, uncomment the code below.
//	addUpdater(99);
//	for (int i=1; i<=10; ++i)
//		addUpdater(i);

	mDeviceScanner = new DeviceScanner(mModbus, this);
	connect(mDeviceScanner, SIGNAL(deviceFound(int)), this, SLOT(onDeviceFound(int)));

	QTimer::singleShot(7500, this, SLOT(onScanTimeout()));
}

void DBusRedflow::onDeviceFound(int address)
{
	foreach (BatteryController *c, mBatteryControllers) {
		if (c->DeviceAddress() == address)
			return;
	}
	BatteryController *m = new BatteryController(mPortName, address, this);
	new BatteryControllerUpdater(m, mModbus, m);
	mBatteryControllers.append(m);
	connect(m, SIGNAL(connectionStateChanged()),
			this, SLOT(onConnectionStateChanged()));
	mDeviceScanner->setScanInterval(2000);
}

void DBusRedflow::onConnectionStateChanged()
{
	BatteryController *m = static_cast<BatteryController *>(sender());
	switch (m->connectionState()) {
	case Disconnected:
		onConnectionLost(m);
		break;
	case Searched:
		break;
	case Detected:
		onDeviceFound(m);
		break;
	case Connected:
		onDeviceInitialized(m);
		break;
	}
}

void DBusRedflow::onScanTimeout()
{
	if (mBatteryControllers.isEmpty()) {
		QLOG_ERROR() << "No batteries found during scan. Application will shut down.";
		exit(1);
	}
}

void DBusRedflow::onDeviceFound(BatteryController *battery)
{
	QLOG_INFO() << "Device found:" << battery->serial()
				<< '@' << battery->portName();
}

void DBusRedflow::onDeviceInitialized(BatteryController *battery)
{
	new BatteryControllerBridge(battery, battery);
	if (mSummary == 0) {
		mSummary = new BatterySummary(this);
		// Make sure we add the battery before registration. The addBattery
		// function will update the values within the summary, so we avoid
		// registering a service without valid values.
		mSummary->addBattery(battery);
		BatterySummaryBridge *bridge = new BatterySummaryBridge(mSummary, mSummary);
		bridge->registerService();
	} else {
		mSummary->addBattery(battery);
	}
	/// @todo EV: Disabled for now, because BMS like support is still is
	/// design.
//	if (mBmsService == 0) {
//		mBmsService = new BmsService(this);
//		mBmsService->addBattery(m);
//		mBmsService->registerService();
//	} else {
//		mBmsService->addBattery(m);
//	}
}

void DBusRedflow::onConnectionLost(BatteryController *battery)
{
	// This will remove the ZBM battery from the D-Bus
	delete battery->findChild<BatteryControllerBridge *>();
	foreach (BatteryController *c, mBatteryControllers) {
		if (c->connectionState() != Disconnected)
			return;
	}
	QLOG_ERROR() << "No more batteries connected. Application will shut down.";
	exit(1);
}

void DBusRedflow::onSerialEvent(const char *description)
{
	QLOG_ERROR() << "Serial event:" << description
				 << "Application will shut down.";
	exit(1);
}

void DBusRedflow::addUpdater(int deviceAddress)
{
	BatteryController *m = new BatteryController(mPortName, deviceAddress, this);
	new BatteryControllerUpdater(m, mModbus, m);
	mBatteryControllers.append(m);
	connect(m, SIGNAL(connectionStateChanged()),
			this, SLOT(onConnectionStateChanged()));
}
