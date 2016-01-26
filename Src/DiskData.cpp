///
/// \file DiskData.cpp
///
///
/// \date Sep 11, 2012
/// \author Mark Garlanger
///

#include "DiskData.h"

#include <fstream>
#include <iostream>
#include <string>

#include "logger.h"

using namespace std;

DiskData::DiskData(char *name): fileRead_m(false),
		                        valid_m(false),
		                        data_m(0),
                                dataSize_m(0),
                                writeProtectedFlag_m(false)
{
	ifstream inFile;

	// read and store entire file
	inFile.open(name, ios::in | ios::binary | ios::ate);

	if (inFile.is_open())
	{
		fileRead_m = true;
		dataSize_m = inFile.tellg();
	    data_m     = new BYTE [dataSize_m];
		inFile.seekg (0, ios::beg);
		inFile.read((char *) data_m, dataSize_m);
		inFile.close();
	}
}

#if 0
DiskData::DiskData(): fileRead_m(false),
		              valid_m(true),
                      data_m(0),
                      dataSize_m(0),
                      writeProtectedFlag_m(false)
{
	createBlank();
}
#endif

DiskData::~DiskData()
{

}

bool DiskData::parse()
{
	unsigned int pos;

	if (!fileRead_m)
	{
		debugss(ssFloppyDisk, ERROR, "%s: File could not be read\n", __FUNCTION__);
		return false;
	}
	if (dataSize_m < 20) /// \todo determine the minimum size.
	{
		debugss(ssFloppyDisk, ERROR, "%s: Invalid file size: %d\n", __FUNCTION__,
		                      dataSize_m);
		return false;
	}
	if ((data_m[0] !='H') || (data_m[2] != '7') || (data_m[3] == 'D'))
	{
		debugss(ssFloppyDisk, ERROR, "%s: Invalid file signature %c%c%c%c\n",
		                      __FUNCTION__, data_m[0], data_m[1], data_m[2], data_m[3]);
		return false;
	}
	switch (data_m[1])
	{
	case '1':
		debugss(ssFloppyDisk,INFO, "Reading an H17D file.\n");
		break;
	case '3':
	case '4':
	case '6':
		debugss(ssFloppyDisk, ERROR, "Disk format H%c7D not currently supported\n",
		                      data_m[1]);
		return false;
	default:
		debugss(ssFloppyDisk, ERROR, "Invalid file signature %c%c%c%c\n", data_m[0],
		                      data_m[1], data_m[2], data_m[3]);
		return false;
	}

	majorVersion_m = data_m[4];
	minorVersion_m = data_m[5];
	pointVersion_m = data_m[6];

	debugss(ssFloppyDisk, INFO, "Parsing version: %d.%d.%d\n", majorVersion_m,
	                                                           minorVersion_m,
			                                                   pointVersion_m);
	pos = 7;

	// per spec, blocks must be in order. This is type "0", must be first.
	if (data_m[pos] == diskFormatType_c)
	{
		pos++;
		readDiskFormat(pos);
	}
	else
	{
		// disk Format Type is missing - not an error, assume standard
	    // 40 track/single-sided raw format.

	}

	// this is type "1" must be next.
	if (data_m[pos] == flagsType_c)
	{
		pos++;
		if (!readFlags(pos))
		{
			return false;
		}
	}
	else
	{
		// no flag block, assume default.
	}

	while ((pos < dataSize_m) && (data_m[pos] <= dataBlockType_c))
	{
		if (data_m[pos++] == dataBlockType_c)
		{
			if (!readDataBlock(pos))
			{
				return false;
			}
		}
		else
		{
			if (!readUnknownBlock(pos))
			{
				// block was mandatory... abort out.
				return false;
			}
		}
	}

	while (pos < dataSize_m)
	{
		if (!readUnknownBlock(pos))
		{
			// block was mandatory... abort out.
			return false;
		}
	}

	return true;
}

unsigned int DiskData::readLength(unsigned int &pos)
{
	unsigned int tmp = 0;

	tmp  = data_m[pos++] << 24;
	tmp |= data_m[pos++] << 16;
	tmp |= data_m[pos++] << 8;
	tmp |= data_m[pos++];

	return tmp;
}

bool DiskData::readDiskFormat(unsigned int &pos)
{
	BYTE flags = data_m[pos++];

	if ((flags & mandatoryFlag_c) == mandatoryFlag_c)
	{
		debugss(ssFloppyDisk, WARNING, "%s Missing Mandatory Flag\n", __FUNCTION__);
	}

	unsigned int blockLength = readLength(pos);
	if (blockLength != diskFormatSize_c)
	{
		debugss(ssFloppyDisk, ERROR, "%s: Invalid size block: %d expected: %d\n", __FUNCTION__,
				                          blockLength, diskFormatSize_c);
		return false;
	}
	// For the current version,

	return true;
}
bool DiskData::readFlags(unsigned int &pos)
{
	BYTE flags = data_m[pos++];
	if ((flags & mandatoryFlag_c) == mandatoryFlag_c)
	{
		debugss(ssFloppyDisk, WARNING, "%s Missing Mandatory Flag\n", __FUNCTION__);
	}

    unsigned int blockLength = readLength(pos);
	if (blockLength < flagsSize_c)
	{
		debugss(ssFloppyDisk, ERROR, "%s: Block too short: %d\n", __FUNCTION__, blockLength);
		return false;
	}

	// First is the write protection
	writeProtectedFlag_m = ((data_m[pos++] & boolFlagMask_c) == boolFlagMask_c);

	// Second is the Distribution Flag
	distributionDiskFlag_m = (data_m[pos++] & valueFlagMask_c);

	// Third is the source of the track data
	imagingTypeFlag_m = (data_m[pos++] & valueFlagMask_c);

	// now we must go through the rest of the flags, if any
	blockLength -= flagsSize_c;

	while (blockLength--)
	{
		// go through any other flags and check for any mandatory flags, if so, fail
		if ((data_m[pos++] & mandatoryFlag_c) == mandatoryFlag_c)
		{
			return false;
		}
	}

	// end of block, processing passed.
	return true;
}

bool DiskData::readDataBlock(unsigned int &pos)
{
	BYTE flags = data_m[pos++];

	if ((flags & mandatoryFlag_c) == mandatoryFlag_c)
	{
		debugss(ssFloppyDisk, WARNING, "%s Missing Mandatory Flag\n", __FUNCTION__);
	}
	unsigned int blockLength = readLength(pos);
	if (blockLength < diskFormatSize_c)
	{
		debugss(ssFloppyDisk, ERROR, "%s Block too short: %d\n", __FUNCTION__, blockLength);
		return false;
	}

	return true;
}

bool DiskData::readHoleBlock(unsigned int &pos)
{
	BYTE flags = data_m[pos++];

	if ((flags & mandatoryFlag_c) == mandatoryFlag_c)
	{
		debugss(ssFloppyDisk, WARNING, "%s Missing Mandatory Flag\n", __FUNCTION__);
	}
	unsigned int blockLength = readLength(pos);
	/// \todo implement hole block.

    pos += blockLength;

    // since we don't actually read it yet, we must return failure if we encounter one.
    return false;
	//return true;
}

bool DiskData::readUnknownBlock(unsigned int &pos)
{
	BYTE flags = data_m[pos++];
	debugss(ssFloppyDisk, WARNING, "Unknown block type: 0x%02x\n", data_m[pos-1]);
	if (flags & mandatoryFlag_c)
	{
		debugss(ssFloppyDisk, ERROR, "Mandatory Block(0x%2x) - Fatal Error.\n", data_m[pos-2]);
		// Encountered a mandatory block we don't know how to process - abort.
		return false;
	}
	// Not mandatory, so skip it.
	unsigned int blockLength = readLength(pos);
	pos += blockLength;

	return true;
}

bool DiskData::checkWriteProtect()
{
	return writeProtectedFlag_m;
}

bool DiskData::getType(DiskTypes &type)
{
	type = fiveHardSectored_c;

	return true;
}
