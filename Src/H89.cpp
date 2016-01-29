/// \file H89.cpp
///
/// \date   Mar 8, 2009
/// \author Mark Garlanger
///

#include "H89.h"
#include "H89-roms.h"
#include "ROM.h"
#include "RAM.h"
#include "z80.h"
#include "h19.h"
#include "AddressBus.h"
#include "h89-timer.h"
#include "h89-io.h"
#include "NMIPort.h"
#include "GeneralPurposePort.h"
#include "INS8250.h"
#include "h17.h"
#include "h37.h"
#include "Z47Interface.h"
#include "Z47Controller.h"
#include "ParallelLink.h"
#include "h-17-1.h"
#include "h-17-4.h"
#include "H47Drive.h"
#include "HardSectoredDisk.h"
#include "SoftSectoredDisk.h"
#include "EightInchDisk.h"
#include "logger.h"

#include <stdlib.h>

H89::H89()
{
    ab    = new AddressBus;
    cpu   = new Z80(cpuClockRate_c, clockInterruptPerSecond_c);
    h19   = new H19;
    h17   = new H17(H17_BaseAddress_c);

    // create the floppy drives for the hard-sectored controller.
    driveUnitH0 = new H_17_1;
    driveUnitH1 = new H_17_4;
    driveUnitH2 = new H_17_4;

#if Z37
    h37   = new Z_89_37(H37_BasePort_c);
    // create the floppy drives for the soft-sectored controller.
    driveUnitS0 = new H_17_1;
    driveUnitS1 = new H_17_1;
    driveUnitS2 = new H_17_1;
    driveUnitS3 = new H_17_1;
#else
    h37         = nullptr;
    driveUnitS0 = nullptr;
    driveUnitS1 = nullptr;
    driveUnitS2 = nullptr;
    driveUnitS3 = nullptr;
#endif

#if Z47
    z47If       = new Z47Interface(Z47_BasePort1_c);
    z47Cntrl    = new Z47Controller();
    z47Link     = new ParallelLink();

    z47If->connectDriveLink(z47Link);
    z47Cntrl->connectHostLink(z47Link);

    driveUnitE0 = new H47Drive;
    driveUnitE1 = new H47Drive;
    eight0      = new EightInchDisk("diskA.eightdisk", EightInchDisk::dif_8RAW);
    eight1      = new EightInchDisk("diskB.eightdisk", EightInchDisk::dif_8RAW);
#else
    z47If       = nullptr;
    z47Cntrl    = nullptr;
    z47Link     = nullptr;

    driveUnitE0 = nullptr;
    driveUnitE1 = nullptr;
    eight0      = nullptr;
    eight1      = nullptr;
#endif

    timer       = new H89Timer;
    h89io       = new H89_IO;
    nmi1        = new NMIPort(NMI_BaseAddress_1_c, NMI_NumPorts_1_c);
    nmi2        = new NMIPort(NMI_BaseAddress_2_c, NMI_NumPorts_2_c);
    gpp         = new GeneralPurposePort;

    // Serial Ports.
    consolePort = new INS8250(Serial_Console_c, Serial_Console_Interrupt_c);
    lpPort      = new INS8250(Serial_LpPort_c);
    modemPort   = new INS8250(Serial_ModemPort_c);
    auxPort     = new INS8250(Serial_AuxPort_c);

    hard0       = new HardSectoredDisk("diskA.tmpdisk");
    hard1       = new HardSectoredDisk("diskB.tmpdisk");
    hard2       = new HardSectoredDisk("diskC.tmpdisk");

#if Z37
    soft0 = new SoftSectoredDisk("diskA.softdisk", SoftSectoredDisk::dif_RAW);
    soft1 = new SoftSectoredDisk("diskB.softdisk", SoftSectoredDisk::dif_RAW);
    soft2 = new SoftSectoredDisk("diskC.softdisk", SoftSectoredDisk::dif_RAW);
    soft3 = 0;
//    soft3 = 0; new SoftSectoredDisk("diskD.softdisk", SoftSectoredDisk::dif_RAW);

//    soft0->dump();
//    soft1->dump();
//    soft2->dump();
//    soft3->dump();
#else
    soft0 = nullptr;
    soft1 = nullptr;
    soft2 = nullptr;
    soft3 = nullptr;
#endif

    timer->setCPU(cpu);

    monitorROM = NULL;
    // TODO: use a config/INI file to get this
    char *s = getenv("V89_ROM_IMAGE");
    if (s != NULL) {
        monitorROM = ROM::getROM(s, 0);
    }
    if (monitorROM == NULL) {
#if MTR90
        monitorROM = new ROM(4096);
        monitorROM->setBaseAddress(0);
        monitorROM->initialize(&MTR90_2_ROM[0], 4096);
#else
        monitorROM = new ROM(2048);
        monitorROM->setBaseAddress(0);
        monitorROM->initialize(&MTR89_ROM[0], 2048);
#endif
    }

    h17ROM     = new ROM(2048);
    h17ROM->setBaseAddress(6*1024);
    h17ROM->initialize(&H17_ROM[0], 2048);

    h17RAM     = new RAM(1024);
    h17RAM->setBaseAddress(5*1024);
    /// TODO determine whether the default h17RAM is write-protected or write-enabled at boot.

    CPM8k      = new RAM(8*1024);
    CPM8k->setBaseAddress(0);

    /// \todo - consolidate RAM into a single object
    // memory = new RAM(16 * 1024);
    // memory = new RAM(0xE000);
    // memory = new RAM(48 * 1024);
    memory = new RAM(56 * 1024);
    memory->setBaseAddress(8 * 1024);

    ab->installMemory(monitorROM);
    ab->installMemory(h17RAM);
    ab->installMemory(h17ROM);
    ab->installMemory(memory);

    consolePort->attachDevice(h19);

    cpu->setAddressBus(ab);
    cpu->setSpeed(false);

    h89io->addDevice(gpp);
    h89io->addDevice(nmi1);
    h89io->addDevice(nmi2);
    h89io->addDevice(consolePort);
    h89io->addDevice(lpPort);
    h89io->addDevice(auxPort);
    h89io->addDevice(modemPort);
    h89io->addDevice(h17);
#if Z37
    h89io->addDevice(h37);
#else
    h89io->addDevice(z47If);
#endif

    // Connect all the floppy drives for the hard-sectored controller.
    h17->connectDrive(0, driveUnitH0);
    h17->connectDrive(1, driveUnitH1);
    h17->connectDrive(2, driveUnitH2);

    // Insert all the the disks into the drives.
    driveUnitH0->insertDisk(hard0);
    driveUnitH1->insertDisk(hard1);
    driveUnitH2->insertDisk(hard2);

#if Z37
    // connect all the drives to the soft-sectored controller.
    h37->connectDrive(0, driveUnitS0);
    h37->connectDrive(1, driveUnitS1);
    h37->connectDrive(2, driveUnitS2);
    h37->connectDrive(3, driveUnitS3);
#endif

#if Z47
    //
    z47Cntrl->connectDrive(0, driveUnitE0);
    z47Cntrl->connectDrive(1, driveUnitE1);
#endif

#if Z37
    // Insert all the disks into the drives.
    driveUnitS0->insertDisk(soft0);
    driveUnitS1->insertDisk(soft1);
    driveUnitS2->insertDisk(soft2);
    driveUnitS3->insertDisk(soft3);
#endif

#if Z47
    driveUnitE0->insertDisk(eight0);
    driveUnitE1->insertDisk(eight1);
#endif
    // driveUnitH0.insertBlankDisk();
    // driveUnitH1.insertBlankDisk();
    // driveUnitH2.insertBlankDisk();

    // driveUnitS0.insertBlankDisk();
    // driveUnitS1.insertBlankDisk();
    // driveUnitS2.insertBlankDisk();
    // driveUnitS3.insertBlankDisk();
}

H89::~H89()
{
    // eject all the disk files

    driveUnitH0->ejectDisk("saveA.tmpdisk");
    driveUnitH1->ejectDisk("saveB.tmpdisk");
    driveUnitH2->ejectDisk("saveC.tmpdisk");

//    driveUnitS0->ejectDisk("saveS0.tmpdisk");
//    driveUnitS1->ejectDisk("saveS1.tmpdisk");
//    driveUnitS2->ejectDisk("saveS2.tmpdisk");
//    driveUnitS3->ejectDisk("saveS3.tmpdisk");
}

void H89::reset()
{
    cpu->reset();
    h19->reset();
    /// \todo reset everything, like the H17, H37, etc.
}

void H89::init()
{
    h19->init();
    z47Cntrl->loadDisk();
}

void H89::keypress(BYTE ch)
{
    h19->keypress(ch);
}

void H89::display()
{
    h19->display();
}

void H89::enableROM()
{
    ab->installMemory(monitorROM);
    ab->installMemory(h17RAM);
    ab->installMemory(h17ROM);
}

void H89::disableROM()
{
    ab->installMemory(CPM8k);
}

void H89::writeProtectH17RAM()
{
    h17RAM->writeProtect(true);
}

void H89::writeEnableH17RAM()
{
    h17RAM->writeProtect(false);
}

void H89::selectSideH17(BYTE side)
{
	h17->selectSide(side);
}

void H89::setSpeed(bool fast)
{
	cpu->setSpeed(fast);
}

bool H89::checkUpdated()
{
    return(h19->checkUpdated());
}

H89Timer &H89::getTimer()
{
    return(*timer);
}

void H89::raiseNMI(void)
{
    cpu->raiseNMI();
}

void H89::raiseINT(int level)
{
    cpu->raiseINT(level);
}

void H89::lowerINT(int level)
{
    cpu->lowerINT(level);
}

void H89::continueCPU(void)
{
    cpu->continueRunning();
}

H89_IO &H89::getIO()
{
    return(*h89io);
}

BYTE H89::run()
{
    cpu->reset();

    return(cpu->execute());
}

void H89::clearMemory(BYTE data)
{
    ab->clearMemory(data);
}

AddressBus &H89::getAddressBus()
{
    return(*ab);
}

