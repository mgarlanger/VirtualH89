/// \file H89.cpp
///
/// \date   Mar 8, 2009
/// \author Mark Garlanger
///

#include "H89.h"
#include "H89-roms.h"
#include "ROM.h"
#include "SystemMemory8K.h"
#include "MemoryLayout.h"
#include "MemoryDecoder.h"
#include "z80.h"
#include "AddressBus.h"
#include "InterruptController.h"
#include "H37InterruptController.h"
#include "MMS316IntrCtrlr.h"
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
#include "mms77320.h"
#include "ParallelLink.h"
#include "h-17-1.h"
#include "h-17-4.h"
#include "H47Drive.h"
#include "HardSectoredDisk.h"
// #include "SoftSectoredDisk.h"
// #include "EightInchDisk.h"
#include "CPNetDevice.h"
#include "Console.h"
#include "logger.h"
#include "propertyutil.h"

/// \cond
#include <vector>
/// \endcond

using namespace std;

H89::H89(): Computer()
{
    pthread_mutex_init(&h89_mutex, nullptr);
}

void
H89::buildSystem(Console* console, PropertyUtil::PropertyMapT props)
{
    this->console = console;

    string s; // for general property queries.

    // TODO: use properties to configure rest of hardware.
    // Possibly set defaults and write/create file.

    interruptController = nullptr;
    ab                  = nullptr;


    cpu                 = new Z80(this, cpuClockRate_c, clockInterruptPerSecond_c);
    h89io               = new H89_IO;
    cpu->setIOBus(h89io);


    s = props["sw501"];

    if (!s.empty())
    {
        gpp = new GeneralPurposePort(this, s);
    }
    else
    {
        gpp = new GeneralPurposePort(this);
    }
    h89io->addDevice(gpp);

    ROM*  monitorROM = nullptr;
    s = props["monitor_rom"];

    if (!s.empty())
    {
        monitorROM = ROM::getROM(s.c_str(), 0);
    }

    if (monitorROM == nullptr)
    {
        monitorROM = new ROM(4096);
        monitorROM->setBaseAddress(0);
        monitorROM->initialize(&MTR90_2_ROM[0], 4096);
    }

    ROM*                       h17ROM = new ROM(2048);
    h17ROM->setBaseAddress(6 * 1024);
    h17ROM->initialize(&H17_ROM[0], 2048);

    vector<string>             memslots        = {"slot_p501", "slot_p502", "slot_p503"};
    bool                       memorySpecified = false;
    bool                       haveMMS77318    = false;
    bool                       ORG_Zero        = true;
    MemoryLayout::MemorySize_t memSize         = MemoryLayout::Mem_48k;

    s = props["ORG0"];

    // Allow config file to turn off ORG-0, but default to true;
    if (s.compare("False") == 0)
    {
        ORG_Zero        = false;
        memorySpecified = true;
    }
    s = props["CPU_Memory"];

    if (s.compare("16K") == 0)
    {
        memSize         = MemoryLayout::Mem_16k;
        memorySpecified = true;
    }
    else if (s.compare("32K") == 0)
    {
        memSize         = MemoryLayout::Mem_32k;
        memorySpecified = true;
    }
    else if (s.compare("48K") == 0)
    {
        memSize         = MemoryLayout::Mem_48k;
        memorySpecified = true;
    }
    else if (s.compare("64K") == 0)
    {
        memSize         = MemoryLayout::Mem_64k;
        memorySpecified = true;
        // \todo error if ORG_Zero
    }

    // only P503 can contain the 16K/128K add-on board.
    //
    for (int x = 0; x < memslots.size(); ++x)
    {
        if (memslots[x] == "slot_p503")
        {
            s = props[memslots[x]];
            if (s.compare("MMS77311") == 0 || s.compare("WH-88-16") == 0)
            {
                memSize = MemoryLayout::Mem_64k;
            }
            else if (s.compare("MMS77318") == 0)
            {
                haveMMS77318 = true;
            }
            else if (s.compare("Empty") == 0)
            {
                // do nothing.
            }
            else
            {
                // default to 64k if config didn't specify memory size
                if (!memorySpecified)
                {
                    memSize = MemoryLayout::Mem_64k;
                }
            }
        }
    }

    // default to no speedup.
    unsigned long speedup = 0;
    s = props["z80_speedup_option"];
    if (!s.empty())
    {
        speedup = strtoul(s.c_str(), nullptr, 10);
        if (speedup > 40)
        {
            debugss(ssH89, ERROR, "Illegal CPU speedup factor %d, disabling\n", speedup);
            speedup = 0;
        }
        else if (haveMMS77318)
        {
            debugss(ssH89, ERROR, "CPU speedup incompatible with MMS77318, disabling\n");
            speedup = 0;
        }
    }
    if ((speedup > 1) && (!haveMMS77318))
    {
        cpu->setSpeedup((unsigned) speedup);
        cpu->enableFast();
    }

    sysMem = make_shared<SystemMemory8K>();
    sysMem->installROM(monitorROM);
    sysMem->installROM(h17ROM);
    sysMem->enableRAM(0x1400, 1024);

    // H17
    H17*     h17 = nullptr;

    // Z37
    Z_89_37* h37 = nullptr;

    // Z47
    z47If       = nullptr;
    z47Cntrl    = nullptr;
    z47Link     = nullptr;
    driveUnitE0 = nullptr;
    driveUnitE1 = nullptr;
    eight0      = nullptr;
    eight1      = nullptr;

    MMS77316*    m316 = nullptr;
    MMS77320*    m320 = nullptr;
    CPNetDevice* cpn  = CPNetDevice::install_CPNetDevice(props);

    if (cpn != nullptr)
    {
        h89io->addDevice(cpn);
    }

    // TODO: not all slots are identical, handle restrictions...
    // Could have a Slot object with more details...
    vector<string> devslots  = {"slot_p504", "slot_p505", "slot_p506"};
    bool           dev_slots = false;
    bool           ser_slots = true;

    for (int x = 0; x < devslots.size(); ++x)
    {
        s = props[devslots[x]];

        if (s.compare("MMS77316") == 0)
        {
            interruptController = new MMS316IntrCtrlr(cpu);
            m316                = MMS77316::install_MMS77316(interruptController,
                                                             props,
                                                             devslots[x]);
            h89io->addDiskDevice(m316);
            dev_slots = true;
        }
        else if (s.compare("MMS77320") == 0)
        {
            // Also includes (auxiliary) serial ports... TODO
            m320 = MMS77320::install_MMS77320(props, devslots[x]);
            h89io->addDiskDevice(m320);
        }
        else if (s.compare("H17") == 0)
        {
            // TODO - should only support slot p506
            h17       = H17::install_H17(H17_BaseAddress_c, props, devslots[x]);
            h89io->addDiskDevice(h17);

            dev_slots = true;

        }
        else if (s.compare("H37") == 0)
        {
            // TODO make sure only one of H37 or MMS77316 is configured.
            interruptController = new H37InterruptController(cpu);

            // TODO - should only be supported on p504 (and maybe p505 - check schematics)
            h37                 = Z_89_37::install_H37(this,
                                                       H37_BaseAddress_c,
                                                       interruptController,
                                                       props,
                                                       devslots[x]);
            h89io->addDiskDevice(h37);

            dev_slots = true;
        }
        else if (s.compare("H47") == 0)
        {
            // Not functional

            // select port based on slot - p504 -> 0170 p506 -> 0174
            z47If    = new Z47Interface(devslots[x].compare("slot_p506") == 0 ?
                                        Z47_BaseAddress_2_c : Z47_BaseAddress_1_c);

            z47Cntrl = new Z47Controller();
            z47Link  = new ParallelLink();

            z47If->connectDriveLink(z47Link);
            z47Cntrl->connectHostLink(z47Link);

            driveUnitE0 = new H47Drive;
            driveUnitE1 = new H47Drive;
            s           = props["z47_disk1"];

            if (s.empty())
            {
                s = "diskA.eightdisk";
            }

            // eight0 = new EightInchDisk(s.c_str(), EightInchDisk::dif_8RAW);
            s = props["z47_disk2"];

            if (s.empty())
            {
                s = "diskB.eightdisk";
            }

            // eight1 = new EightInchDisk(s.c_str(), EightInchDisk::dif_8RAW);

        }
        else if (s.compare("H_88_3") == 0)
        {
            // 3-port serial board
            // \todo make sure it's on P504 or P505.
            lpPort    = new INS8250(this, Serial_LpPort_c);
            modemPort = new INS8250(this, Serial_ModemPort_c);
            auxPort   = new INS8250(this, Serial_AuxPort_c);

            h89io->addDevice(lpPort);
            h89io->addDevice(auxPort);
            h89io->addDevice(modemPort);

            ser_slots = true;
        }
    }

    // if no interrupt created with the soft-sectored controllers, create the default one
    if (interruptController == nullptr)
    {
        interruptController = new InterruptController(cpu);
    }

    ab    = new AddressBus(interruptController);
    cpu->setAddressBus(ab);
    timer = new H89Timer(this, cpu);

    h89io->addDevice(new NMIPort(cpu, NMI_BaseAddress_1_c, NMI_NumPorts_1_c));
    h89io->addDevice(new NMIPort(cpu, NMI_BaseAddress_2_c, NMI_NumPorts_2_c));

    string memDecoderType;
    if (haveMMS77318)
    {
        memDecoderType = "MMS77318";
    }
    else if (ORG_Zero)
    {
        memDecoderType = "H89";
    }
    else
    {
        memDecoderType = "H88";
    }

    ab->installMemory(MemoryDecoder::createMemoryDecoder(memDecoderType,
                                                         sysMem,
                                                         memSize));

    if (!dev_slots)
    {
        h17 = new H17(H17_BaseAddress_c);
        // create the floppy drives for the hard-sectored controller.
        shared_ptr<DiskDrive> driveUnitH0 = make_shared<H_17_1>();
        shared_ptr<DiskDrive> driveUnitH1 = make_shared<H_17_1>();
        shared_ptr<DiskDrive> driveUnitH2 = make_shared<H_17_1>();

        s = props["h17_disk1"];

        if (s.empty())
        {
            s = "diskA.tmpdisk";
        }

        shared_ptr<HardSectoredDisk> hard0 = make_shared<HardSectoredDisk>(s.c_str());
        s = props["h17_disk2"];

        if (s.empty())
        {
            s = "diskB.tmpdisk";
        }

        shared_ptr<HardSectoredDisk> hard1 = make_shared<HardSectoredDisk>(s.c_str());
        s = props["h17_disk3"];

        if (s.empty())
        {
            s = "diskC.tmpdisk";
        }

        shared_ptr<HardSectoredDisk> hard2 = make_shared<HardSectoredDisk>(s.c_str());

        h89io->addDiskDevice(h17);

        // Connect all the floppy drives for the hard-sectored controller.
        h17->connectDrive(0, driveUnitH0);
        h17->connectDrive(1, driveUnitH1);
        h17->connectDrive(2, driveUnitH2);

        // Insert all the the disks into the drives.
        driveUnitH0->insertDisk(hard0);
        driveUnitH1->insertDisk(hard1);
        driveUnitH2->insertDisk(hard2);

    }

    // Serial Ports.
    consolePort = new INS8250(this, Serial_Console_c, Serial_Console_Interrupt_c);
    consolePort->attachDevice(console);

    h89io->addDevice(consolePort);

    if (!ser_slots)
    {
        lpPort    = new INS8250(this, Serial_LpPort_c);
        modemPort = new INS8250(this, Serial_ModemPort_c);
        auxPort   = new INS8250(this, Serial_AuxPort_c);

        h89io->addDevice(lpPort);
        h89io->addDevice(auxPort);
        h89io->addDevice(modemPort);
    }

}


H89::~H89()
{
    // Acquire system mutex before starting to tear-down
    // everything.  This avoids a race where the CPU might be using
    // objects that are being destroyed.
    //
    systemMutexAcquire();

    // eject all the disk files

    for (DiskController* dev : h89io->getDiskDevices())
    {
        if (dev != nullptr)
        {
            dev->~DiskController();
        }
    }

}

void
H89::reset()
{
    cpu->reset();
    ab->reset();
    console->reset();
    h89io->reset();
    timer->reset();
}

void
H89::init()
{
    console->init();

    if (z47Cntrl != nullptr)
    {
        z47Cntrl->loadDisk();
    }
}

void
H89::writeProtectH17RAM()
{
    sysMem->writeProtect(0x1400, 1024);
}

void
H89::writeEnableH17RAM()
{
    sysMem->writeEnable(0x1400, 1024);
}

void
H89::raiseNMI(void)
{
    cpu->raiseNMI();
}

void
H89::systemMutexAcquire()
{
    pthread_mutex_lock(&h89_mutex);
}

void
H89::systemMutexRelease()
{
    pthread_mutex_unlock(&h89_mutex);
}

void
H89::raiseINT(int level)
{
    debugss(ssH89, VERBOSE, "level - %d\n", level);
    interruptController->raiseInterrupt(level);
}

void
H89::lowerINT(int level)
{
    debugss(ssH89, VERBOSE, "level - %d\n", level);
    interruptController->lowerInterrupt(level);
}

void
H89::continueCPU(void)
{
    cpu->continueRunning();
}

void
H89::waitCPU(void)
{
    cpu->waitState();
}

H89_IO&
H89::getIO()
{
    return (*h89io);
}

BYTE
H89::run()
{
    cpu->reset();
    timer->start();

    return (cpu->execute());
}

AddressBus&
H89::getAddressBus()
{
    return (*ab);
}

CPU&
H89::getCPU()
{
    return (*cpu);
}

string
H89::dumpDebug()
{
    string ret = gpp->dumpDebug();
    // Note: INT should be part of H89 (or InterruptController), not CPU...
    // And... the signals are not actually latched on motherboard, only in
    // respective I/O adapters. But for convenience they should be latched
    // somewhere, or else we need to ask every device whether it has an interrupt.
    ret += PropertyUtil::sprintf("INT=%02x\n", 0);
    return ret;
}

// DEPRECATED
void
H89::keypress(BYTE ch)
{
}

void
H89::display()
{
}
