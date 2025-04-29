#include <QApplication>
#include <QCommandLineParser>
#include <QSettings>
#include "gui/mainwindow.h"
#include "x86emulator/emulation.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("x86Emulator");
    app.setApplicationVersion("0.1.0");
    
    // Set up command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("x86Emulator - i386 PC emulator");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // Add config option
    QCommandLineOption configOption(QStringList() << "c" << "config",
                                   "Load configuration file",
                                   "file");
    parser.addOption(configOption);
    
    // Add CPU backend option
    QCommandLineOption cpuOption(QStringList() << "cpu",
                                "Select CPU backend (box86 or mame)",
                                "backend");
    parser.addOption(cpuOption);
    
    // Add machine type option
    QCommandLineOption machineOption(QStringList() << "machine",
                                   "Select machine type (generic, sis630)",
                                   "type");
    parser.addOption(machineOption);
    
    // Process command line arguments
    parser.process(app);
    
    // Create emulation core
    auto emulation = x86emu::Emulation::Create();
    
    // Set CPU backend if specified
    if (parser.isSet(cpuOption)) {
        QString cpuBackend = parser.value(cpuOption);
        if (cpuBackend.toLower() == "box86") {
            emulation->SetCPUBackend(x86emu::CPUBackend::BOX86);
        } else if (cpuBackend.toLower() == "mame") {
            emulation->SetCPUBackend(x86emu::CPUBackend::MAME);
        } 
        // Remove WinUAE option
        // else if (cpuBackend.toLower() == "winuae") {
        //     emulation->SetCPUBackend(x86emu::CPUBackend::WINUAE);
        // } 
        else {
            qWarning() << "Unknown CPU backend:" << cpuBackend;
            qWarning() << "Using default CPU backend";
        }
    }
    
    // Set machine type if specified
    if (parser.isSet(machineOption)) {
        QString machineType = parser.value(machineOption);
        if (machineType.toLower() == "sis630") {
            emulation->SetMachineType(x86emu::MachineType::SIS_630_SYSTEM);
        } else if (machineType.toLower() == "generic") {
            emulation->SetMachineType(x86emu::MachineType::GENERIC_PENTIUM);
        } else {
            qWarning() << "Unknown machine type:" << machineType;
            qWarning() << "Using default machine type";
        }
    }
    
    // Create and show the main window
    x86emu::MainWindow mainWindow(std::move(emulation));
    mainWindow.show();
    
    // If a config file was specified, load it
    if (parser.isSet(configOption)) {
        QString configFile = parser.value(configOption);
        mainWindow.loadConfiguration(configFile);
    }
    
    return app.exec();
}