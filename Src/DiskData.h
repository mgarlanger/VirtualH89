///
/// \file DiskData.h
///
///
/// \date Sep 11, 2012
/// \author Mark Garlanger
///

#ifndef DISKDATA_H_
#define DISKDATA_H_

#include "config.h"
#include "h89Types.h"


class DiskData
{
  public:
	enum DiskTypes
	{
		none_c = 0,
		fiveHardSectored_c = 1,
		fiveSoftSectored_c = 2,
		eightSoftSectored_c = 3,
		hardDisk_c = 4
	};

	DiskData(char *name);
	DiskData(DiskTypes type); // what about size, tracks, sides, density, etc...
	virtual ~DiskData();

	virtual bool getType(DiskTypes &type);
	virtual bool parse();

	virtual bool checkWriteProtect();

  protected:
	bool fileRead_m;
	bool valid_m;
	DiskTypes type;

	BYTE *data_m;
	unsigned int dataSize_m;
	BYTE majorVersion_m;
	BYTE minorVersion_m;
	BYTE pointVersion_m;

	BYTE distributionDiskFlag_m;
	bool writeProtectedFlag_m;
	BYTE imagingTypeFlag_m;

	static const BYTE mandatoryFlag_c  = 0x80;

	static const BYTE valueFlagMask_c  = 0x7f;
	static const BYTE boolFlagMask_c   = 0x01;

	static const BYTE diskFormatType_c = 0x00;
	static const BYTE diskFormatSize_c = 8;

	static const BYTE flagsType_c      = 0x01;
	static const BYTE flagsSize_c      = 3;

	static const BYTE dataBlockType_c  = 0x10;

	unsigned int readLength(unsigned int &pos);
	virtual bool readDiskFormat(unsigned int &pos);
	virtual bool readFlags(unsigned int &pos);
	virtual bool readDataBlock(unsigned int &pos);
	virtual bool readHoleBlock(unsigned int &pos);   /// \todo make this pure virtual ? why?
	virtual bool readUnknownBlock(unsigned int &pos);
};



#endif // DISKDATA_H_
