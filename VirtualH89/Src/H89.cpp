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
#include "AddressBus.h"
#include "InterruptController.h"
#include "h89-timer.h"
#include "h89-io.h"
#include "NMIPort.h"
#include "GeneralPurposePort.h"
#include "INS8250.h"
#include "h17.h"
#include "h37.h"
#include "Z47Interface.h"
#include "Z47Controller.h"
#include "mms77316.h"
#include "RawFloppyImage.h"
#include "ParallelLink.h"
#include "h-17-1.h"
#include "h-17-4.h"
#include "H47Drive.h"
#include "HardSectoredDisk.h"
#include "SoftSectoredDisk.h"
#include "EightInchDisk.h"
#include "Console.h"
#include "logger.h"
#include "propertyutil.h"

#include <stdlib.h>
#include <vector>

H89::H89(): Computer()
{
}

void H89::buildSystem(Console *console)
{
    PropertyUtil::PropertyMapT props;
    // TODO: allow specification of config file via cmdline args.
    std::string cfg;
    char *env = getenv("V89_CONFIG");

    this->console = console;

    if (env)
    {
        // If file-not-found, we still might create it later...
        cfg = env;

        try
        {
            PropertyUtil::read(cfg.c_str(), props);
        }

        catch (std::exception &e) {}
    }
    else
    {
        cfg = getenv("HOME");
        cfg += "/.v89rc";

        try
        {
            PropertyUtil::read(cfg.c_str(), props);
        }

        catch (std::exception &e) {}
    }

    std::string s; // for general property queries.

    // TODO: use properties to configure rest of hardware.
    // Possibly set defaults and write/create file.

    cpu   = new Z80(cpuClockRate_c, clockInterruptPerSecond_c);
    interruptController = new InterruptController(cpu);
    ab    = new AddressBus(interruptController);
    cpu->setAddressBus(ab);
    cpu->setSpeed(false);
    timer       = new H89Timer(cpu, interruptController);
    h19 = new H19;

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
    s = props["z47_disk1"];

    if (s.empty())
    {
        s = "diskA.eightdisk";
    }

    eight0      = new EightInchDisk(s.c_str(), EightInchDisk::dif_8RAW);
    s = props["z47_disk2"];

    if (s.empty())
    {
        s = "diskB.eightdisk";
    }

    eight1      = new EightInchDisk(s.c_str(), EightInchDisk::dif_8RAW);
#else
    z47If       = nullptr;
    z47Cntrl    = nullptr;
    z47Link     = nullptr;

    driveUnitE0 = nullptr;
    driveUnitE1 = nullptr;
    eight0      = nullptr;
    eight1      = nullptr;
#endif

    MMS77316 *m316 = NULL;
    // TODO: not all slots are identical, handle restrictions...
    // Could have a Slot object with more details...
    std::vector<std::string> devslots = { "slot_p504", "slot_p505", "slot_p506" };

    for (int x = 0; x < devslots.size(); ++x)
    {
        s = props[devslots[x]];

        if (s.compare("MMS77316") == 0)
        {
            m316 = MMS77316::install_MMS77316(props, devslots[x]);
        }

        else if (s.compare("H17") == 0)
        {
        }
        else if (s.compare("H37") == 0)
        {
        }
        else if (s.compare("H47") == 0)
        {
        }
    }

    h89io       = new H89_IO;
    nmi1        = new NMIPort(NMI_BaseAddress_1_c, NMI_NumPorts_1_c);
    nmi2        = new NMIPort(NMI_BaseAddress_2_c, NMI_NumPorts_2_c);
    s = props["sw501"];

    if (!s.empty())
    {
        gpp = new GeneralPurposePort(s);
    }

    else
    {
        gpp = new GeneralPurposePort();
    }

    // Serial Ports.
    consolePort = new INS8250(Serial_Console_c, Serial_Console_Interrupt_c);
    lpPort      = new INS8250(Serial_LpPort_c);
    modemPort   = new INS8250(Serial_ModemPort_c);
    auxPort     = new INS8250(Serial_AuxPort_c);

    s = props["z17_disk1"];

    if (s.empty())
    {
        s = "diskA.tmpdisk";
    }

    hard0       = new HardSectoredDisk(s.c_str());
    s = props["z17_disk2"];

    if (s.empty())
    {
        s = "diskB.tmpdisk";
    }

    hard1       = new HardSectoredDisk(s.c_str());
    s = props["z17_disk3"];

    if (s.empty())
    {
        s = "diskC.tmpdisk";
    }

    hard2       = new HardSectoredDisk(s.c_str());

#if Z37
    s = props["z37_disk1"];

    if (s.empty())
    {
        s = "diskA.softdisk";
    }

    soft0 = new SoftSectoredDisk(s.c_str(), SoftSectoredDisk::dif_RAW);
    s = props["z37_disk2"];

    if (s.empty())
    {
        s = "diskB.softdisk";
    }

    soft1 = new SoftSectoredDisk(s.c_str(), SoftSectoredDisk::dif_RAW);
    s = props["z37_disk3"];

    if (s.empty())
    {
        s = "diskC.softdisk";
    }

    soft2 = new SoftSectoredDisk(s.c_str(), SoftSectoredDisk::dif_RAW);
    soft3 = 0;
//    s = props["z37_disk4"];
//    if (s.empty()) s = "diskD.softdisk";
//    soft3 = 0; new SoftSectoredDisk(s.c_str(), SoftSectoredDisk::dif_RAW);

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

    monitorROM = NULL;
    s = props["monitor_rom"];

    if (!s.empty())
    {
        monitorROM = ROM::getROM(s.c_str(), 0);
    }

    if (monitorROM == NULL)
    {
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
    h17ROM->setBaseAddress(6 * 1024);
    h17ROM->initialize(&H17_ROM[0], 2048);

    h17RAM     = new RAM(1024);
    h17RAM->setBaseAddress(5 * 1024);
    /// TODO determine whether the default h17RAM is write-protected or write-enabled at boot.

    CPM8k      = new RAM(8 * 1024);
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

    consolePort->attachDevice(console);

    h89io->addDevice(gpp);
    h89io->addDevice(nmi1);
    h89io->addDevice(nmi2);
    h89io->addDevice(consolePort);
    h89io->addDevice(lpPort);
    h89io->addDevice(auxPort);
    h89io->addDevice(modemPort);
    h89io->addDiskDevice(h17);
#if Z37
    h89io->addDiskDevice(h37);
#else
    h89io->addDiskDevice(z47If);
#endif

    if (m316 != NULL)
    {
        h89io->addDiskDevice(m316);
    }

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
    std::vector<IODevice *> dsks = h89io->getDiskDevices();

    for (int x = 0; x < dsks.size(); ++x)
    {
        IODevice *dev = dsks[x];

        if (dev != NULL)
        {
            dev->~IODevice();
        }
    }

//  driveUnitH0->ejectDisk("saveA.tmpdisk");
//  driveUnitH1->ejectDisk("saveB.tmpdisk");
//  driveUnitH2->ejectDisk("saveC.tmpdisk");

//    driveUnitS0->ejectDisk("saveS0.tmpdisk");
//    driveUnitS1->ejectDisk("saveS1.tmpdisk");
//    driveUnitS2->ejectDisk("saveS2.tmpdisk");
//    driveUnitS3->ejectDisk("saveS3.tmpdisk");
}

void H89::reset()
{
    cpu->reset();
    console->reset(); // TODO: does H89 reset really also reset H19?
    /// \todo reset everything, like the H17, H37, etc.
}

void H89::init()
{
    console->init();
    z47Cntrl->loadDisk();
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

H89Timer& H89::getTimer()
{
    return (*timer);
}

void H89::raiseNMI(void)
{
    cpu->raiseNMI();
}

void H89::registerInter(Z80::intrCheck *func, void *data)
{
    cpu->registerInter(func, data);
}

void H89::unregisterInter(Z80::intrCheck *func)
{
    cpu->unregisterInter(func);
}

void H89::raiseINT(int level)
{
    debugss(ssH89, VERBOSE, "%s: level - %d\n", __FUNCTION__, level);
    interruptController->raiseInterrupt(level);
}

void H89::lowerINT(int level)
{
    debugss(ssH89, VERBOSE, "%s: level - %d\n", __FUNCTION__, level);
    interruptController->lowerInterrupt(level);
}

void H89::continueCPU(void)
{
    cpu->continueRunning();
}

void H89::waitCPU(void)
{
    cpu->waitState();
}

H89_IO& H89::getIO()
{
    return (*h89io);
}

BYTE H89::run()
{
    cpu->reset();

    return (cpu->execute());
}

void H89::clearMemory(BYTE data)
{
    ab->clearMemory(data);
}

AddressBus& H89::getAddressBus()
{
    return (*ab);
}

// DEPRECATED
void H89::keypress(BYTE ch) {}
void H89::display() {}
