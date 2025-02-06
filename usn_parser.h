#pragma once

#include <string>
#include <fstream>

#include "ntfs_mft_parser.h"

class UsnJournalParser
{
public :
	UsnJournalParser();
	virtual ~UsnJournalParser();

	//use "vssadmin List Shadows" to get VSS shadow copy volume names ..or open any live NTFS volume 
	//"\\\\?\\GLOBALROOT\\Device\\HarddiskVolumeShadowCopy3"
	bool init(std::wstring vol_path);

	bool QueryUsnJournal(USN_JOURNAL_DATA_V2& data);
	void ReadUsnJournalRecords(USN_JOURNAL_DATA_V2& JournalData, UINT64 usn_start, UINT64 usn_end, UINT64 usn_count, std::string format);

	bool setOutputFile(std::string path) { 
		_outputStream.open(path);
		if (!_outputStream.is_open()) {
			
			std::cerr << "Failed to open output file" << std::endl;
			return false;
		}

		return true;
	}

private:
	bool _PrintUsnRecord(PUSN_RECORD& record, std::wstring path, std::string format);
	bool _PrintCSVHeader();

	Ntfs_c* _mft;
	HANDLE _hVolume;

    std::wofstream _outputStream;
};