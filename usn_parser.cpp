#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

#include <Windows.h>
#include <WinIoCtl.h>
#include <stdio.h>

#include <iostream>
#include <locale>
#include <string>
#include <codecvt>

#include "usn_parser.h"


inline std::wstring getReason(int usn_reason) {
	std::wstring reason = L"";
	if (USN_REASON_DATA_OVERWRITE && usn_reason)
		reason += L"USN_REASON_DATA_OVERWRITE|";
	if (USN_REASON_DATA_EXTEND & usn_reason)
		reason += L"USN_REASON_DATA_EXTEND|";
	if (USN_REASON_DATA_TRUNCATION & usn_reason)
		reason += L" USN_REASON_DATA_TRUNCATION|";
	if (USN_REASON_NAMED_DATA_OVERWRITE & usn_reason)
		reason += L"USN_REASON_NAMED_DATA_OVERWRITE|";
	if (USN_REASON_NAMED_DATA_EXTEND & usn_reason)
		reason += L"USN_REASON_NAMED_DATA_EXTEND|";
	if (USN_REASON_NAMED_DATA_TRUNCATION & usn_reason)
		reason += L"USN_REASON_NAMED_DATA_TRUNCATION|";
	if (USN_REASON_FILE_CREATE & usn_reason)
		reason += L"USN_REASON_FILE_CREATE|";
	if (USN_REASON_FILE_DELETE & usn_reason)
		reason += L"USN_REASON_FILE_DELETE|";
	if (USN_REASON_EA_CHANGE & usn_reason)
		reason += L"USN_REASON_EA_CHANGE|";
	if (USN_REASON_SECURITY_CHANGE & usn_reason)
		reason += L"USN_REASON_SECURITY_CHANGE|";
	if (USN_REASON_RENAME_OLD_NAME & usn_reason)
		reason += L"USN_REASON_RENAME_OLD_NAME|";
	if (USN_REASON_RENAME_NEW_NAME & usn_reason)
		reason += L"USN_REASON_RENAME_NEW_NAME|";
	if (USN_REASON_INDEXABLE_CHANGE & usn_reason)
		reason += L"USN_REASON_INDEXABLE_CHANGE|";
	if (USN_REASON_BASIC_INFO_CHANGE & usn_reason)
		reason += L"USN_REASON_BASIC_INFO_CHANGE|";
	if (USN_REASON_HARD_LINK_CHANGE & usn_reason)
		reason += L"USN_REASON_HARD_LINK_CHANGE|";
	if (USN_REASON_COMPRESSION_CHANGE & usn_reason)
		reason += L"USN_REASON_COMPRESSION_CHANGE|";
	if (USN_REASON_ENCRYPTION_CHANGE & usn_reason)
		reason += L"USN_REASON_ENCRYPTION_CHANGE|";
	if (USN_REASON_OBJECT_ID_CHANGE & usn_reason)
		reason += L"USN_REASON_OBJECT_ID_CHANGE|";
	if (USN_REASON_REPARSE_POINT_CHANGE & usn_reason)
		reason += L"USN_REASON_REPARSE_POINT_CHANGE|";
	if (USN_REASON_STREAM_CHANGE & usn_reason)
		reason += L"USN_REASON_STREAM_CHANGE|";
	if (USN_REASON_CLOSE & usn_reason)
		reason += L"USN_REASON_CLOSE|";

	return reason;
}



#define NTFS_VOLUME_NAME L"\\\\.\\C:" // Replace C: with your volume letter if needed




bool UsnJournalParser::QueryUsnJournal( USN_JOURNAL_DATA_V2 &data) {
    DWORD bytesReturned = 0;
    USN_JOURNAL_DATA_V2 journalData = { 0 };

    // Query for the journal information
    if (DeviceIoControl(_hVolume, FSCTL_QUERY_USN_JOURNAL, NULL, 0,
        &journalData, sizeof(journalData), &bytesReturned, NULL)) {
        /*
        std::wcout << L"USN Journal ID: " << journalData.UsnJournalID << std::endl;
        std::wcout << L"First Usn: " << journalData.FirstUsn << std::endl;
        std::wcout << L"Next Usn: " << journalData.NextUsn << std::endl;
        std::wcout << L"Max Size: " << journalData.MaximumSize << std::endl;
        std::wcout << L"Allocation Delta: " << journalData.AllocationDelta << std::endl;
        */
        data = journalData;
    }
    else {
        std::wcerr << L"Failed to query USN Journal data. Error: " << GetLastError() << std::endl;
        return false;
    }

    return true;

}


#if (NTDDI_VERSION >= NTDDI_WIN8)
typedef READ_USN_JOURNAL_DATA_V1 READ_USN_JOURNAL_DATA, * PREAD_USN_JOURNAL_DATA;
#else
typedef READ_USN_JOURNAL_DATA_V0 READ_USN_JOURNAL_DATA, * PREAD_USN_JOURNAL_DATA;
#endif

bool UsnJournalParser::_PrintCSVHeader() {

    //print into file
    if (_outputStream.is_open()) {
        _outputStream << "USN,Path,Reason,FileReferenceNumber,ParentFileReferenceNumber,FileAttributes" << std::endl;
        return true;
    }

    //print on screen
    std::wcout << L"USN,Path,Reason,FileReferenceNumber,ParentFileReferenceNumber,FileAttributes" << std::endl;
	return true;

}

bool UsnJournalParser::_PrintUsnRecord(PUSN_RECORD &record, std::wstring path, std::string format="") {
	// Print the USN Record

    if (format == "csv"){
        //print into file
        if (_outputStream.is_open()) {
            _outputStream <<record->Usn << L"," << path << L"," << getReason(record->Reason) << L"," << record->FileReferenceNumber << L"," << record->ParentFileReferenceNumber << L",0x" << std::hex << record->FileAttributes << std::dec << std::endl;
            return true;
        }
        //print on screen
		std::wcout << record->Usn << L"," << path << L"," << getReason(record->Reason) << L"," << record->FileReferenceNumber << L"," << record->ParentFileReferenceNumber << L",0x" << std::hex<< record->FileAttributes<<std::dec << std::endl;
		return true;
	}

    std::wcout<< L"------------------------" << std::endl;
    std::wcout << L"USN: " << record->Usn << std::endl;
    std::wcout << L"Path: " << path << std::endl;
    std::wcout << L"Reason: " << getReason(record->Reason) << std::endl;
    std::wcout << L"FileReferenceNumber: " << record->FileReferenceNumber << std::endl;
	std::wcout << L"ParentFileReferenceNumber: " << record->ParentFileReferenceNumber << std::endl;

    std::wcout << L"FileAttributes: 0x" << std::hex  << record->FileAttributes << std::dec << std::endl;


    return true;

}

// Function to read and display USN Journal records
void UsnJournalParser::ReadUsnJournalRecords(USN_JOURNAL_DATA_V2 & JournalData,UINT64 usn_start=0, UINT64 usn_end=0, UINT64 usn_count=0, std::string format="") {
    DWORD bytesReturned = 0;
    ULONG bufferSize = 8192; // Size of the buffer to read
    PVOID buffer = malloc(bufferSize);

    if (!buffer) {
        std::wcerr << L"Memory allocation failed!" << std::endl;
        return;
    }

    // Prepare the input structure for reading the journal

    READ_USN_JOURNAL_DATA ReadData = {0};
    ReadData.UsnJournalID = JournalData.UsnJournalID;
    ReadData.StartUsn = JournalData.FirstUsn;
    if (usn_start != 0) {
		ReadData.StartUsn = usn_start;
	}

    ReadData.ReasonMask = 0xFFFFFFFF;
    ReadData.ReturnOnlyOnClose = FALSE;
    ReadData.Timeout = 0;
    ReadData.BytesToWaitFor = 0;
    ReadData.MinMajorVersion = 2;
    ReadData.MaxMajorVersion = 2;

    size_t rec_num = 0;
    USN last_usn = 0;


    if (format == "csv")
        _PrintCSVHeader();

    while (true) {
        // Call DeviceIoControl to read the USN Journal records
        if (!DeviceIoControl(_hVolume, FSCTL_READ_USN_JOURNAL, &ReadData,
            sizeof(ReadData), 
            buffer, bufferSize, &bytesReturned, NULL)) 
        {
                DWORD error = GetLastError();
                if (error == ERROR_HANDLE_EOF) {
                    break;
                }
                else {
                    std::wcerr << L"Failed to read USN Journal data. Error: " << error << std::endl;
                    break;
                }
        }

        ReadData.StartUsn = *(USN*)buffer;
        bytesReturned -= sizeof(USN);

        // Process the records in the buffer
        PUSN_RECORD record = (PUSN_RECORD)((PBYTE)buffer + sizeof(USN));


        if (bytesReturned <=0){
			break;
		}
        
        while (bytesReturned > 0) {
            rec_num+= 1;
            if (usn_count > 0 && rec_num > usn_count) 
                break;
            if (usn_end > 0 && record->Usn > (USN)usn_end) 
				break;


            last_usn = record->Usn;

            bytesReturned -= record->RecordLength;


            auto parent_path  = _mft->getPathByFileReferenceNumber(record->ParentFileReferenceNumber, true);
            auto path = record->FileNameLength > 0 ? std::wstring(record->FileName, record->FileNameLength / 2) : L"";
            if (!parent_path.empty())
				path = parent_path + L"\\" + path;

             /*
            if (path.empty()) {
                path = std::wstring(record->FileName, record->FileNameLength / 2);
                auto parent_path = _mft->getPathByFileReferenceNumber(record->ParentFileReferenceNumber, true);

                if (!parent_path.empty())
                    path = parent_path + L"\\" + path;

            }
            */


            //std::wcout <<L"Path:" << path <<  L"     Event: " << getReason(record->Reason) << std::endl;

            _PrintUsnRecord(record, path, format);

            //std::wcout.flush(); // Ensure output is written immediately


            record = (PUSN_RECORD)(((PBYTE)record) + record->RecordLength);


            //ReadData.StartUsn = record->Usn; // Update the FirstUsn for the next iteration
        } // end buffer while
    }


    std::wcout << L"Records number: " << rec_num << std::endl;
    std::wcout << L"Last USN : " << last_usn << std::flush << std::endl;


    // Clean up memory
    free(buffer);
}


UsnJournalParser::UsnJournalParser() {
    this->_mft = new Ntfs_c();
    this->_hVolume= 0;
}

UsnJournalParser::~UsnJournalParser() {
	delete this->_mft;
    
    if (this->_hVolume != 0) {
		CloseHandle(this->_hVolume);
	}

    if (this->_outputStream.is_open()) {
        this->_outputStream.close();
    }
}

bool UsnJournalParser::init(std::wstring vol_path) {

    if (!this->_mft->init(vol_path)) {
        std::cerr << "Error opening drive!" << std::endl;
        return false;
    }

    _hVolume = CreateFile(L"\\\\.\\C:", GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if (_hVolume == INVALID_HANDLE_VALUE) {
        std::wcerr << L"Failed to open volume. Error: " << GetLastError() << std::endl;
        return false;
    }



    return true;
}

int usage(){

    std::string help_msg= "\n\
Usage: usn_parser [[<cmd> [[--volume <path>][ --start <usn_start>][--end <usn_end>][--reason <usn_reason>][--regexp <regexp> ][--format <txt|csv|json>][--ofile <path>]]]\n\
where cmd is one of the following: \n\
    help: Display this help message. \n\
    stats: Get usn journal stats \n\
        E.g. usn_parser stats --volume \"\\\\.\\C:\" \n\
    usn_last: Get the last usn number\n\
    usn_read: Read usn journal records\n\
		E.g. usn_parser usn_read --volume \"\\\\.\\C:\" [--start <usn_start> ][[--end <usn_end>]|[--count <count>]]  \n\
";

	std::cout << help_msg << std::endl;
	return 1;
}

int get_switch_value(int argc, char* argv[], std::string switch_name, std::string& value) {
	for (int i = 0; i < argc; i++) {
		if (argv[i] == switch_name) {
			if (i + 1 < argc) {
				value = argv[i + 1];
				return 0;
			}
			else {
				std::cerr << "Missing value for switch " << switch_name << std::endl;
				return 1;
			}
		}
	}
	return 1;
}


int main(int argc, char** argv) {


    std::ios_base::sync_with_stdio(false);
    std::locale utf8(std::locale(), new std::codecvt_utf8_utf16<wchar_t>);
    std::wcout.imbue(utf8);


    if (argc < 2) {
        return usage();
    }

    std::string command = argv[1];
    if (command == "help") {
        return usage();
    }

    UsnJournalParser usn_parser;
    std::string volume = "";
    get_switch_value(argc, argv, "--volume", volume);

    std::string start = "";
    get_switch_value(argc, argv, "--start", start);
    UINT64 usn_start = 0;
    if (!start.empty()) {
		usn_start = std::stoull(start);
	}


    std::string end = "";
    get_switch_value(argc, argv, "--end", end);
    UINT64 usn_end = 0;
    if (!end.empty()) {
        usn_end = std::stoull(end);
     }

    std::string count = "";
    get_switch_value(argc, argv, "--count", count);
    UINT64 usn_count = 0;
    if (!count.empty()) {
		usn_count = std::stoull(count);
	}

    std::string reason = "";
    get_switch_value(argc, argv, "--reason", reason);

    std::string regexp = "";
    get_switch_value(argc, argv, "--regexp", regexp);

    std::string format = "";
    get_switch_value(argc, argv, "--format", format);

    std::string ofile = "";
    get_switch_value(argc, argv, "--ofile", ofile);
    if (!ofile.empty()) {
		if (!usn_parser.setOutputFile(ofile)) {
			std::cerr << "Failed to open output file" << std::endl;
			return 1;
		}
	}



    if (volume.empty()) 
        return usage();
    
    wstring vol = std::wstring(volume.begin(), volume.end());
    if (!usn_parser.init(vol)) {
        std::wcerr << L"initializing usn parser for " << vol << std::endl;
        return 1;
    }

    USN_JOURNAL_DATA_V2 journalData = { 0 };
    // Query the USN Journal and get the journal data
    if (!usn_parser.QueryUsnJournal(journalData)) {
        std::wcerr << L"Failed to read USN journal data " << std::endl;
        return 1;
    }

    if (command == "stats") {
        //Print the USN Journal data and exit
        std::wcout << L"UsnJournalID : " << journalData.UsnJournalID << std::endl;
        std::wcout << L"FirstUsn: " << journalData.FirstUsn << std::endl;
        std::wcout << L"NextUsn: " << journalData.NextUsn << std::endl;
        std::wcout << L"MaxUsn: " << journalData.MaxUsn << std::endl;
        std::wcout << L"LowestValidUsn: " << journalData.LowestValidUsn << std::endl;

        std::wcout << L"MaximumSize : " << journalData.MaximumSize << std::endl;
        std::wcout << L"AllocationDelta: " << journalData.AllocationDelta << std::endl;
        std::wcout << L"Flags: " << std::hex  << journalData.Flags << std::dec << std::endl;
        std::wcout << L"MaxSupportedMajorVersion : " << journalData.MaxSupportedMajorVersion << std::endl;
        std::wcout << L"MinSupportedMajorVersion: " << journalData.MinSupportedMajorVersion << std::endl;
        std::wcout << L"RangeTrackChunkSize: " << journalData.RangeTrackChunkSize<< std::endl;
        std::wcout << L"RangeTrackFileSizeThreshold: " << journalData.RangeTrackFileSizeThreshold<< std::endl;
        

        return 0;
    }

    if (command == "usn_read") {        
        usn_parser.ReadUsnJournalRecords(journalData,usn_start,usn_end,usn_count, format);  
        return 0;
    }

    return usage();
}
