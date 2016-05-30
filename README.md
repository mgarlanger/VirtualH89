# VirtualH89

Heathkit H89 Emulator in C++

## Description

The Heathkit H89, was an 8-bit Z80-based system sold in kit-form during 1979-1985 by Heath Company. An identical system was sold assembled, by Zenith Data System, as the Z-89 and Z-90. It supported upto 64k of RAM, and offered several different options for storage, including cassette, hard-sectored 5.25" floppy disks, soft-sectored 5.25" floppy disks, 8" floppy disks, and a 10M Winchester HardDrive. Currently, the emulator supports hard-sectored 5.25" floppy drives/disks, the MMS 77316 Soft-sectored Controller, and the MMS 77320 SASI controller. . 
It is targeted to Unix-based systems, including Mac OS X and Linux.

Currently using GLUT for the UI, but plan to change to a more modern cross-platfrom library.

Additional details are available on my site: [Heathkit H89 Emulator](http://heathkit.garlanger.com/emulator/)



## Getting Started

An xcode project is provided for the Mac OS X platform. 


## Configuration file

By default, the emulator will look for a configuration file specified in the env variable V89_CONFIG. 
Without a config file, the emulator will default to a basic H89 with 64k (ORG-0 support), and H17 controller
and 3 H-17-1 hard-sectored disk drives.


## Example config file

```
sw401 = 10001100     # switch sw401 as defined on the Terminal Board
sw402 = 00000101     # switch sw402 as defined on the Terminal Board
sw501 = 00100000     # switch sw501 as defined on the Terminal Board

monitor_rom = /Users/mgarlanger/h89Data/ROMs/Heath/2732_444-142_MTR90A.bin # location of ROM image

ORG0 = True    # Whether or not ORG-0 support provide - set to False if ORG-0 is not supported, defaults to True
slot_p503 = MMS77318  # type of memory add-on card - Options MMS77318 - MMS's 128k board
                      # WH-88-16 - Heath's 16k board, Empty
CPU_Memory = 48K  # Amount of memory on CPU board, Options: 16K, 32K, 48K (64K - although there is not enough
                  # sockets for 64k, some systems have memory chips piggy-back on the top row.


# Heath's H37 soft-sector controller 
slot_p504 = H37   # type of card in the 3rd slot from the right, typically soft-sectored controller cards
                  # Options: H37, MMS77316 - MMS's soft-sectored controller.

h37_drive1 = FDD_5_25_DS_DT
h37_drive2 = FDD_5_25_DS_DT
h37_drive3 = FDD_5_25_DS_DT
h37_drive4 = FDD_5_25_DS_DT
h37_disk1 = /Users/mgarlanger/h89Data/Disks/h37/MMS_CPM_Plus_Disk1.IMD rw
h37_disk2 = /Users/mgarlanger/h89Data/Disks/h37/MMS_CPM_Plus_Disk2.IMD rw
h37_disk3 = /Users/mgarlanger/h89Data/Disks/h37/MMS_CPM_Plus_Disk3.IMD rw
h37_disk4 = /Users/mgarlanger/h89Data/Disks/h37/MMS_CPM_Plus_Disk4.IMD rw

# another option MMS77316 soft-sectored controller
#slot_p504 = MMS77316
#mms77316_drive1 = FDD_8_DS
#mms77316_drive2 = FDD_8_DS
#mms77316_drive3 = FDD_8_DS
#mms77316_drive4 = FDD_8_DS
#mms77316_drive5 = FDD_5_25_DS_DT
#mms77316_drive6 = FDD_5_25_DS_DT
#mms77316_drive7 = FDD_5_25_DS_DT
#mms77316_drive8 = FDD_5_25_DS_DT
#mms77316_disk1 = /Users/mgarlanger/h89Data/Disks/mmscpm3ds8.logdisk rw
#mms77316_disk2 = /Users/mgarlanger/h89Data/Disks/mmscpm3ds8.logdisk rw
#mms77316_disk3 = /Users/mgarlanger/h89Data/Disks/mmscpm3ds8.logdisk rw
#mms77316_disk4 = /Users/mgarlanger/h89Data/Disks/mmscpm3ds8.logdisk rw
#mms77316_disk5 = /Users/mgarlanger/h89Data/Disks/mms-cpm-distro2.mmsdisk rw
#mms77316_disk6 = /Users/mgarlanger/h89Data/Disks/mms-cpm-distro3.mmsdisk rw
#mms77316_disk7 = /Users/mgarlanger/h89Data/Disks/invaders.mmsdisk rw
#mms77316_disk8 = /Users/mgarlanger/h89Data/Disks/blank5ddds.mmsdisk rw

# 3 port serial
slot_p505 = H_88_3

# hard-sectored controller
slot_p506 = H17

h17_drive1 = H17_1
h17_drive2 = H17_1
h17_drive3 = H17_1

h17_disk1 = /Users/mgarlanger/h89Data/Disks/h37/HDOS_2-0_Hos_5_Update.h17raw
h17_disk2 = /Users/mgarlanger/h89Data/Disks/diskB.tmpdisk
h17_disk3 = /Users/mgarlanger/h89Data/Disks/diskC.tmpdisk
```


