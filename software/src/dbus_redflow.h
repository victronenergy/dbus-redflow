#ifndef DBUS_REDFLOW_H
#define DBUS_REDFLOW_H

#include <QObject>
#include <QList>

class BatteryController;
class BatteryControllerUpdater;
class BatterySummary;
class BmsService;
class DeviceScanner;
class ModbusRtu;
class Settings;

/*!
 * Main object which ties everything together.
 * This class will coordinate the search for new AC sensors, and start
 * acquisition when found. The class `AcSensorUpdater` is responsible for
 * communication with the sensors. Sensor data is stored in `AcSensor`.
 *
 * The class will also make sure that the Hub-4 control loop is started when
 * apropriate. The control loop itself is implemented in `ControlLoop`.
 *
 * Finally, this class will detect the presence of a multi within the setup,
 * which we need for the Hub-4 control loop.
 */
class DBusRedflow : public QObject
{
	Q_OBJECT
public:
	DBusRedflow(const QString &portName, QObject *parent = 0);

signals:
	void connectionLost();

private slots:
	void onDeviceFound(int address);

	void onDeviceFound(BatteryController *battery);

	void onDeviceInitialized(BatteryController *c);

	void onConnectionLost(BatteryController *battery);

	void onConnectionStateChanged();

	void onScanTimeout();

	void onSerialEvent(const char *description);

private:
	void addUpdater(int deviceAddress);

	DeviceScanner *mDeviceScanner;
	ModbusRtu *mModbus;
	QString mPortName;
	QList<BatteryController *> mBatteryControllers;
	BatterySummary *mSummary;
	BmsService *mBmsService;
};

#endif // DBUS_REDFLOW_H
