#include <QTest>
#include <QProcess>

#include <QTemporaryDir>
#include <QTemporaryFile>

class FileOpsTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void cleanupTestCase();

	void testControlFile();
	void testLocalFileOps();

private:
	QFile m_controlFile;
	QTemporaryDir m_mountDir;
};

void FileOpsTest::initTestCase()
{
	QString programpath = QFINDTESTDATA("kio-fuse");

	QProcess kiofuseProcess;
	kiofuseProcess.setProgram(programpath);
	kiofuseProcess.setArguments({m_mountDir.path()});

	kiofuseProcess.start();

	// kio-fuse without "-f" daemonizes only after mounting succeeded
	QVERIFY(kiofuseProcess.waitForFinished());
	QVERIFY(kiofuseProcess.exitStatus() == QProcess::NormalExit);
	QVERIFY(kiofuseProcess.exitCode() == 0);

	m_controlFile.setFileName(m_mountDir.filePath(QStringLiteral("_control")));

	QVERIFY(m_controlFile.open(QIODevice::WriteOnly | QIODevice::Unbuffered));
}

void FileOpsTest::cleanupTestCase()
{
	// Make sure that the mountpoint is not busy
	m_controlFile.close();

	QProcess fusermountProcess;
	fusermountProcess.start(QStringLiteral("fusermount3"), {QStringLiteral("-u"), m_mountDir.path()});

	// If any of this fails, we can't do anything anyway
	fusermountProcess.waitForFinished();
	m_mountDir.remove();
}

void FileOpsTest::testControlFile()
{
	QVERIFY(m_controlFile.exists());
	QVERIFY(m_controlFile.isWritable());

	QString cmd = QStringLiteral("MOUNT invalid URL");
	QCOMPARE(m_controlFile.write(cmd.toUtf8()), -1);
}

void FileOpsTest::testLocalFileOps()
{
	QTemporaryFile localFile;
	QVERIFY(localFile.open());

	QCOMPARE(localFile.write("teststring"), 10);
	QVERIFY(localFile.flush());

	QString cmd = QStringLiteral("MOUNT file://%1").arg(localFile.fileName());
	QCOMPARE(m_controlFile.write(cmd.toUtf8()), cmd.length());

	QFile mirroredFile(QStringLiteral("%1/file/%2").arg(m_mountDir.path()).arg(localFile.fileName()));
	QVERIFY(mirroredFile.exists());
	QCOMPARE(mirroredFile.size(), localFile.size());
}

QTEST_GUILESS_MAIN(FileOpsTest)

#include "fileopstest.moc"