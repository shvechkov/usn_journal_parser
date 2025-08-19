/*
* The below link/code was used/ammended by me when studing NTFS MFT internals 
* Cudos to the author of the article! 
* https://handmade.network/forums/articles/t/7002-tutorial_parsing_the_mft
*
    This is free and unencumbered software released into the public domain.
    Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
    software, either in source code form or as a compiled binary, for any purpose,
    commercial or non-commercial, and by any means.
    In jurisdictions that recognize copyright laws, the author or authors of this
    software dedicate any and all copyright interest in the software to the public
    domain. We make this dedication for the benefit of the public at large and to
    the detriment of our heirs and successors. We intend this dedication to be an
    overt act of relinquishment in perpetuity of all present and future rights to
    this software under copyright law.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
    ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <windows.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <list>
#include <algorithm>

#include <cassert>

#include "ntfs_mft_parser.h"

using namespace std;



void Ntfs_c::printRanges() {
    for (auto& range : _ranges) {
        std::cout << "Record Number: " << range.recordNumber << " start: " << range.start << " length: " << range.length << std::endl;
    }
}


// Custom comparator function
bool compare(const Ntfs_c::FileRange& a, const Ntfs_c::FileRange& b) {
    return a.start < b.start; // Sort in descending order
}

void Ntfs_c::printRangesByFileNumber(uint64_t recordNumber) {
    cout << "Ranges for record number: " << recordNumber << endl;
    for (auto& range : _ranges) {
        if (range.recordNumber == recordNumber) {
            std::cout << "\tstart: " << range.start << " length: " << range.length << std::endl;
        }
    }
}




// Function to create a 64-bit FileReferenceNumber
/*
What is the USN FileReferenceNumber?
In the NTFS file system, the FileReferenceNumber is a unique identifier assigned to each file and directory on a volume. It consists of two parts:

MFT Entry Number – Identifies the record within the Master File Table (MFT).
Sequence Number – Ensures integrity by tracking how many times a record has been reused.
This identifier remains unique as long as the MFT entry is not reused.

Structure of FileReferenceNumber
The FileReferenceNumber is a 64-bit value:

Lower 48 bits → MFT Entry Number
Upper 16 bits → Sequence Number
Where is it Stored in MFT?
The Master File Table (MFT) maintains a structured record for each file and directory on an NTFS volume. The FileReferenceNumber is stored within the MFT record of each file.

MFT Entry Header contains:
The MFT Record Number (FileReferenceNumber)
A Sequence Number
$STANDARD_INFORMATION or $FILE_NAME attributes may reference this number.
How is it Used?
It helps track files uniquely, even if they are renamed or moved.
Used by the USN Journal (Update Sequence Number Journal) to log changes to files.
The NTFS Indexing System uses it to quickly locate files.
*/
uint64_t Ntfs_c::_createFileReferenceNumber(uint64_t mftRecordNumber, uint16_t sequenceNumber) {
    // Ensure the MFT Record Number fits within 48 bits
    mftRecordNumber &= 0x0000FFFFFFFFFFFF;

    // Shift the sequence number to the upper 16 bits and combine with MFT record number
    uint64_t fileReferenceNumber = (static_cast<uint64_t>(sequenceNumber) << 48) | mftRecordNumber;

    return fileReferenceNumber;
}

void Ntfs_c::printRecordByNumber(uint64_t recordNumber) {
    auto rec = _file_records[recordNumber];


    std::cout << "Record Number: " << rec.file_rec_header.recordNumber << std::endl;
    for (auto& name : rec.names) {
        std::wcout << "\t" << name << std::endl;
    }


    std::cout << "fileReferenceNumber: " << _createFileReferenceNumber(rec.file_rec_header.recordNumber, rec.file_rec_header.sequenceNumber) << std::endl;
    std::cout << "isDirectory: " << rec.file_rec_header.isDirectory << std::endl;
    std::cout << "usedSize: " << rec.file_rec_header.usedSize << std::endl;
    std::cout << "allocatedSize: " << rec.file_rec_header.allocatedSize << std::endl;
    std::cout << "fileReference: " << rec.file_rec_header.fileReference << std::endl;
    std::cout << "nextAttributeID: " << rec.file_rec_header.nextAttributeID << std::endl;
    std::cout << "recordNumber: " << rec.file_rec_header.recordNumber << std::endl;
    std::cout << "firstAttributeOffset: " << rec.file_rec_header.firstAttributeOffset << std::endl;
    std::cout << "inUse: " << rec.file_rec_header.inUse << std::endl;
    std::cout << "isDirectory: " << rec.file_rec_header.isDirectory << std::endl;
    std::cout << "hardLinkCount: " << rec.file_rec_header.hardLinkCount << std::endl;
    std::cout << "sequenceNumber: " << rec.file_rec_header.sequenceNumber << std::endl;
    std::cout << "logSequence: " << rec.file_rec_header.logSequence << std::endl;
    std::cout << "updateSequenceSize: " << rec.file_rec_header.updateSequenceSize << std::endl;
    std::cout << "updateSequenceOffset: " << rec.file_rec_header.updateSequenceOffset << std::endl;

    std::cout << "File Name Attribute: " << std::endl;
    std::cout << "parentRecordNumber: " << rec.file_name_attr_header.parentRecordNumber << std::endl;
    std::cout << "namespaceType: " << rec.file_name_attr_header.namespaceType << std::endl;
    std::cout << "fileNameLength: " << rec.file_name_attr_header.fileNameLength << std::endl;
    std::cout << "flags: " << rec.file_name_attr_header.flags << std::endl;

    std::wcout << "fileName: " << std::wstring(rec.file_name_attr_header.fileName, rec.file_name_attr_header.fileName + rec.file_name_attr_header.fileNameLength) << std::endl;


}

uint64_t Ntfs_c::_getRecordByFileReferenceNumber(uint64_t  ref_num) {
    auto it = _file_references.find(ref_num);
    if (it == _file_references.end()) {
        //std::cout << "File not found." << std::endl;
        return 0;
    }
    return it->second->file_rec_header.recordNumber;
}


uint64_t Ntfs_c::printRecordByFileReferenceNumber(uint64_t  ref_num) {
    auto it = _file_references.find(ref_num);
    if (it == _file_references.end()) {
        std::cout << "File not found." << std::endl;
        return 0;
    }

    printRecordByNumber(it->second->file_rec_header.recordNumber);


    wcout << "Abs file path: " << getPathByFileReferenceNumber(ref_num, true) << endl;

    return it->second->file_rec_header.recordNumber;
}

uint64_t Ntfs_c::printRecordByFileName(std::wstring name) {
    for (auto rec : _file_records) {
        for (auto& n : rec.second.names) {
            if (n == name) {
                printRecordByNumber(rec.first);
                return rec.first;
            }
        }
    }
    std::cout << "File not found." << std::endl;

    return 0;
}


std::wstring Ntfs_c::printRecordName(uint64_t recNum, bool absolute) {

    auto it = _file_records.find(recNum);
    if (it == _file_records.end())
        return L"";

    auto cur_rec = it->second;

    if (!cur_rec.names.size())
        return L"";

    std::wstring path = cur_rec.names[0];
    while (absolute && cur_rec.file_rec_header.recordNumber != cur_rec.file_name_attr_header.parentRecordNumber) {
        auto parent_rec = _file_records[cur_rec.file_name_attr_header.parentRecordNumber];
        
        if (parent_rec.names.size() == 0)
			break;

        path = parent_rec.names[0] + L"\\" + path;
        cur_rec = _file_records[parent_rec.file_rec_header.recordNumber];
    }
    //std::cout << "\t" << path << std::endl;
    return path;

}


void Ntfs_c::printFileNames(bool absolute) {
    for (auto rec : _file_records) {
        std::cout << "Record Number: " << rec.first << std::endl;
        for (auto& name : rec.second.names) {
            wstring path = name;
            if (absolute) {
                //print the full path - we need to resolve all parent directories                
                auto cur_rec = rec.second;
                while (cur_rec.file_rec_header.recordNumber != cur_rec.file_name_attr_header.parentRecordNumber) {
                    auto parent_rec = _file_records[cur_rec.file_name_attr_header.parentRecordNumber];
                    if (parent_rec.names.size() == 0)
                        break;
                    path = parent_rec.names[0] + L"\\" + path;
                    cur_rec = _file_records[parent_rec.file_rec_header.recordNumber];
                }
                std::wcout << "\t" << path << std::endl;
            }
        }
    }
}



void Ntfs_c::findRanges(uint64_t start, uint64_t len) {
    //assume that input list of ranges already sorted by the range start value (by FileRange->start filed)
    // using binary search , find range with largest FileRange->start  which is less or equal to an input start value . Print start and length for that range

    auto it = std::lower_bound(_ranges.begin(), _ranges.end(), start,
        [](const FileRange& range, uint64_t value)
        {
            return range.start < value;
        });

    if (it == _ranges.end()) {
        std::cout << "No ranges found." << std::endl;
        return;
    }

    //std::cout << "Range found: start: " << it->start << " length: " << it->length << std::endl;
    it--;
    //std::cout << "range: start: " << it->start << " length: " << it->length << std::endl;

    int found_files = 0;

    while (it++ != _ranges.end()) {
        if (it->start > start + len) {
            break;
        }
        found_files++;


        FileRange r = *it;
        FileRecord f = _file_records[r.recordNumber];

        std::wcout << L"\trange: start: " << it->start << L" length: " << it->length << L"  name: " << printRecordName(r.recordNumber, true) << std::endl;
    }

    std::cout << "Found " << found_files << " file(s)." << std::endl;
    return;

}




vector<pair<uint64_t, uint64_t>>  Ntfs_c::getNonResidentDataRuns(NonResidentAttributeHeader* dataAttribute) {

    vector<pair<uint64_t, uint64_t>> ret;

    // Get the data run header
    RunHeader* dataRun = (RunHeader*)((uint8_t*)dataAttribute + dataAttribute->dataRunsOffset);

    uint64_t  offset_prev = 0;

    // Iterate through the data runs
    while (((uint8_t*)dataRun - (uint8_t*)dataAttribute) < dataAttribute->length && dataRun->lengthFieldBytes) {
        // Initialize the length and offset
        uint64_t length = 0, offset = 0;



        // Calculate the length and offset from the data run
        for (int i = 0; i < dataRun->lengthFieldBytes; i++) {
            length |= (uint64_t)(((uint8_t*)dataRun)[1 + i]) << (i * 8);
        }

        for (int i = 0; i < dataRun->offsetFieldBytes; i++) {
            offset |= (uint64_t)(((uint8_t*)dataRun)[1 + dataRun->lengthFieldBytes + i]) << (i * 8);
        }

        // Handle negative offset
        if (offset & ((uint64_t)1 << (dataRun->offsetFieldBytes * 8 - 1))) {
            int64_t neg = static_cast<int64_t>(offset);
            neg |= static_cast<int64_t>(0xFFFFFFFFFFFFFFFF) << (dataRun->offsetFieldBytes * 8);
            offset = offset_prev + neg;
        }
        else {
            offset = offset_prev + offset;
        }


        // std::cout << "\tDataRun;  len:" << length << " offset: " << offset << std::endl;

         //offset counts from the start of the previous cluster 
         //i.e. starting cluster of the previous data run + offset = starting cluster of the current data run
         //cluster size is 4K  - i.e.  uint64_t bytesPerCluster = bootSector.bytesPerSector * bootSector.sectorsPerCluster;


        ret.push_back(make_pair(offset, length));
        offset_prev = offset;

        // Move to the next data run
        dataRun = (RunHeader*)((uint8_t*)dataRun + 1 + dataRun->lengthFieldBytes + dataRun->offsetFieldBytes);

    }

    return ret;

}

void Ntfs_c::getResidentDataRun(ResidentAttributeHeader* dataAttribute) {
    //  std::cout << "Resident data run" << std::endl;    
}




// Function to apply update sequence fixup for a FileRecordHeader
// This is used to restore the original bytes in the update sequence area of the MFT record
// w/o this code the MFT record may not be correctly parsed, as the update sequence area contains fixups for the data runs
// and as a result the previous code w/o this fucntoin could not read filenames longer than 75 symbols


/*
The issue with not being able to read or find file names longer than 75 characters , without the update 
sequence fixup code, stems from how NTFS handles data integrity in MFT records. Specifically, NTFS uses an update sequence mechanism
to protect against data corruption during disk writes, which overwrites certain bytes in each MFT record. Without reversing this 
process (via fixup), the parser reads corrupted data, particularly affecting attributes like $FILE_NAME when they span or are near
specific sector boundaries. This led to the observed truncation of file names at 75 characters and the appearance of corrupted 
characters (e.g., ↨ caused by invalid UTF-16 code units like 07 00 or 09 00). Below, I’ll explain the details of why this happens 
and how the fixup code resolves it, focusing on the NTFS update sequence mechanism and its impact on file name parsing.
*/



void applyUpdateSequenceFixup(FileRecordHeader* fileRecord) {
    if (fileRecord->magic == 0x454C4946) { // Check for "FILE" magic
        uint16_t* updateSequence = (uint16_t*)((uint8_t*)fileRecord + fileRecord->updateSequenceOffset);
        uint16_t sequenceNumber = updateSequence[0];
        if (fileRecord->updateSequenceSize == 3) { // Expect 3 words: sequence number + 2 fixups
            uint16_t* fixupPos1 = (uint16_t*)((uint8_t*)fileRecord + 510); // Last 2 bytes of first sector
            if (*fixupPos1 == sequenceNumber) {
                *fixupPos1 = updateSequence[1]; // Restore original bytes
            }
            uint16_t* fixupPos2 = (uint16_t*)((uint8_t*)fileRecord + 1022); // Last 2 bytes of second sector
            if (*fixupPos2 == sequenceNumber) {
                *fixupPos2 = updateSequence[2]; // Restore original bytes
            }
        }
    }
}

bool Ntfs_c::init(std::wstring deicePath) {
    _drive = CreateFileW(deicePath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    Read(&_bootSector, 0, 512);
    _bytesPerCluster = _bootSector.bytesPerSector * _bootSector.sectorsPerCluster;

    // Read the MFT file
    Read(&mftFile, _bootSector.mftStart * _bytesPerCluster, MFT_FILE_SIZE);

    // Get the file record header
    FileRecordHeader* fileRecord = (FileRecordHeader*)mftFile;


    applyUpdateSequenceFixup(fileRecord);

    // Get the first attribute header
    AttributeHeader* attribute = (AttributeHeader*)(mftFile + fileRecord->firstAttributeOffset);

    // Initialize the data attribute pointer
    NonResidentAttributeHeader* dataAttribute = nullptr;

    // Initialize the approximate record count
    uint64_t approximateRecordCount = 0;

    // Check the magic number of the file record header
    assert(fileRecord->magic == 0x454C4946);

    // Iterate through the attributes
    while (true) {
        // Check the attribute type
        if (attribute->attributeType == 0x80) {
            // Found the data attribute
            dataAttribute = (NonResidentAttributeHeader*)attribute;
        }
        else if (attribute->attributeType == 0xB0) {
            // Found the approximate record count attribute
            approximateRecordCount = ((NonResidentAttributeHeader*)attribute)->attributeSize * 8;
        }
        else if (attribute->attributeType == 0xFFFFFFFF) {
            // Reached the end of the attributes
            break;
        }

        // Move to the next attribute
        attribute = (AttributeHeader*)((uint8_t*)attribute + attribute->length);
    }

    // Check if the data attribute is found
    assert(dataAttribute);

    // Get the data run header
    RunHeader* dataRun = (RunHeader*)((uint8_t*)dataAttribute + dataAttribute->dataRunsOffset);

    // Initialize the cluster number and records processed
    uint64_t clusterNumber = 0, recordsProcessed = 0;

    // Iterate through the data runs
    while (((uint8_t*)dataRun - (uint8_t*)dataAttribute) < dataAttribute->length && dataRun->lengthFieldBytes) {
        // Initialize the length and offset
        uint64_t length = 0, offset = 0;

        // Calculate the length and offset from the data run
        for (int i = 0; i < dataRun->lengthFieldBytes; i++) {
            length |= (uint64_t)(((uint8_t*)dataRun)[1 + i]) << (i * 8);
        }

        for (int i = 0; i < dataRun->offsetFieldBytes; i++) {
            offset |= (uint64_t)(((uint8_t*)dataRun)[1 + dataRun->lengthFieldBytes + i]) << (i * 8);
        }

        // Handle negative offset
        if (offset & ((uint64_t)1 << (dataRun->offsetFieldBytes * 8 - 1))) {
            for (int i = dataRun->offsetFieldBytes; i < 8; i++) {
                offset |= (uint64_t)0xFF << (i * 8);
            }
        }

        // Update the cluster number
        clusterNumber += offset;

        // Move to the next data run
        dataRun = (RunHeader*)((uint8_t*)dataRun + 1 + dataRun->lengthFieldBytes + dataRun->offsetFieldBytes);

        // Calculate the number of files remaining and the position in the block
        uint64_t filesRemaining = length * _bytesPerCluster / MFT_FILE_SIZE;
        uint64_t positionInBlock = 0;

        // Iterate through the files
        while (filesRemaining) {
            // Print the progress
            fprintf(stderr, "\rReading MFT: %d%% ", (int)(recordsProcessed * 100 / approximateRecordCount));

            // Calculate the number of files to load
            uint64_t filesToLoad = MFT_FILES_PER_BUFFER;
            if (filesRemaining < MFT_FILES_PER_BUFFER) filesToLoad = filesRemaining;

            // Read the MFT buffer
            Read(&mftBuffer, clusterNumber * _bytesPerCluster + positionInBlock, filesToLoad * MFT_FILE_SIZE);
            positionInBlock += filesToLoad * MFT_FILE_SIZE;
            filesRemaining -= filesToLoad;

            // Process each file record in the buffer
            for (int i = 0; i < filesToLoad; i++) {
                // Get the file record
                FileRecordHeader* fileRecord = (FileRecordHeader*)(mftBuffer + MFT_FILE_SIZE * i);

				applyUpdateSequenceFixup(fileRecord);

                recordsProcessed++;

                // Skip unused file records
                if (!fileRecord->inUse) continue;

                // Get the first attribute of the file record
                AttributeHeader* attribute = (AttributeHeader*)((uint8_t*)fileRecord + fileRecord->firstAttributeOffset);

                // Check the magic number of the file record
                assert(fileRecord->magic == 0x454C4946);

                FileRecord fr;
                fr.file_rec_header = *fileRecord;
                _file_records[fileRecord->recordNumber] = fr;

                _file_references[_createFileReferenceNumber(fr)] = &_file_records[fileRecord->recordNumber];

                vector<pair<uint64_t, uint64_t>> fileDataRuns;

                if (fileRecord->unused)
                    continue;

                // Iterate through the attributes of the file record
                while ((uint8_t*)attribute - (uint8_t*)fileRecord < MFT_FILE_SIZE) {
                    // Check the attribute type
                    if (attribute->attributeType == 0x80) {
                        // Found the data attribute
                        if (attribute->nonResident) {
                            dataAttribute = (NonResidentAttributeHeader*)attribute;
                            fileDataRuns = getNonResidentDataRuns(dataAttribute);

                            // Saving file's data runs into a global list of ranges
                            for (auto r : fileDataRuns) {
                                struct FileRange fr;
                                fr.start = r.first;
                                fr.length = r.second;
                                fr.recordNumber = fileRecord->recordNumber;
                                _ranges.push_back(fr);
                            }
                        }
                        else {
                            ResidentAttributeHeader* dataAttribute = (ResidentAttributeHeader*)attribute;

                            auto mft_block_offset = clusterNumber * _bytesPerCluster + positionInBlock;
                            auto buff_offset = (uint8_t*)dataAttribute - (uint8_t*)mftBuffer;
                            struct FileRange fr;
                            fr.start = mft_block_offset + buff_offset;
                            fr.length = dataAttribute->attributeLength;
                            fr.recordNumber = fileRecord->recordNumber;
                            _ranges.push_back(fr);
                        }
                    }
                    else if (attribute->attributeType == 0x30) {
                        // Found the file name attribute
                        FileNameAttributeHeader* fileNameAttribute = (FileNameAttributeHeader*)attribute;

                        // Check the namespace type and non-resident flag
                        if (fileNameAttribute->namespaceType != 2 && !fileNameAttribute->nonResident) {
                            std::wstring fname(fileNameAttribute->fileName, fileNameAttribute->fileName + fileNameAttribute->fileNameLength);
                            _file_records[fileRecord->recordNumber].addName(fname);
                            _file_records[fileRecord->recordNumber].file_name_attr_header = *fileNameAttribute;

                            //std::wstring targetName = L"000000000000000000000000000000000000000000000000000000000000000123456A.pdf";
                            //if (fname == targetName || 144466 == fileRecord->recordNumber) {
                            //    // Debug output for specific record or target file
                            //    std::cout << "Record " << fileRecord->recordNumber << ": attributeLength=" << attribute->length
                            //        << ", fileNameLength=" << (int)fileNameAttribute->fileNameLength
                            //        << ", namespaceType=" << (int)fileNameAttribute->namespaceType << std::endl;

                            //    // Check if file name fits within attribute and MFT record
                            //    size_t nameBytes = fileNameAttribute->fileNameLength * sizeof(wchar_t);
                            //    if ((uint8_t*)fileNameAttribute->fileName + nameBytes > (uint8_t*)fileRecord + MFT_FILE_SIZE ||
                            //        (uint8_t*)fileNameAttribute->fileName + nameBytes > (uint8_t*)attribute + attribute->length) {
                            //        std::cerr << "Error: File name exceeds boundaries for record " << fileRecord->recordNumber << std::endl;
                            //    }
                            //    else {
                            //        std::wstring fname(fileNameAttribute->fileName, fileNameAttribute->fileName + fileNameAttribute->fileNameLength);
                            //        std::wcout << L"File name: " << fname << L", length: " << fname.length() << std::endl;
                            //        // Hexdump file name data
                            //        std::cout << "Raw file name bytes: ";
                            //        for (size_t i = 0; i < nameBytes; i++) {
                            //            printf("%02x ", ((uint8_t*)fileNameAttribute->fileName)[i]);
                            //        }
                            //        std::cout << std::endl;
                            //        _file_records[fileRecord->recordNumber].addName(fname);
                            //        _file_records[fileRecord->recordNumber].file_name_attr_header = *fileNameAttribute;
                            //    }

                            //    // Use std::wcout for wide string output
                            //    std::wcout << fname << L" is my file - check it " << std::endl;
                            //    int64_t ref_num = _createFileReferenceNumber(fr);
                            //    printRecordByFileReferenceNumber(ref_num);
                            //}
                        }
                        else if (fileNameAttribute->namespaceType != 2 && fileNameAttribute->nonResident) {
                            assert(0); // $FILE_NAME is always resident
                        }
                    }
                    else if (attribute->attributeType == 0xFFFFFFFF) {
                        // Reached the end of the file record
                        break;
                    }

                    // Move to the next attribute
                    attribute = (AttributeHeader*)((uint8_t*)attribute + attribute->length);
                }
            }
        }
    }

    fprintf(stderr, "\rReading MFT: Done");

    // Print the number of files found
    fprintf(stderr, "\nFound %lld files.\n", _file_records.size());

    // Sort the list using the custom comparator (for searching files/ranges affected during incremental backup)
    _ranges.sort(compare);

    return true;
}

std::string Ntfs_c::ChangeType2Str(Ntfs_c::change_type_n change) {
    if (change == Ntfs_c::NEW) {
        return "NEW";
    }
    else if (change == Ntfs_c::MODIFIED) {
        return "MODIFIED";
    }
    else if (change == Ntfs_c::RENAMED) {
        return "RENAMED";
    }
    else if (change == Ntfs_c::DELETED) {
        return "DELETED";
    }
    else if (change == Ntfs_c::NO_CHANGE) {
        return "NO_CHANGE";
    }
    else {
        return "UNKNOWN";
    }
}

Ntfs_c::change_type_n Ntfs_c::detectRecordChange(Ntfs_c& prev_mft, Ntfs_c::FileRecord& prev_rec, Ntfs_c& cur_mft, Ntfs_c::FileRecord& cur_rec) {

    // Check if the record was renamed
    auto prev_name = prev_mft.printRecordName(prev_rec.file_rec_header.recordNumber, true);
    auto cur_name = cur_mft.printRecordName(cur_rec.file_rec_header.recordNumber, true);

    if (prev_rec.file_rec_header.fileReference != cur_rec.file_rec_header.fileReference) {
        assert(prev_rec.file_rec_header.fileReference != cur_rec.file_rec_header.fileReference); //this should never happen - if it does, we have a bug in the code
        return NO_CHANGE;
    }

    if (prev_rec.file_name_attr_header.parentRecordNumber != cur_rec.file_name_attr_header.parentRecordNumber || prev_name != cur_name) {
        return RENAMED;
    }

    // Check if the record was modified
    if (prev_rec.file_name_attr_header.modificationTime != cur_rec.file_name_attr_header.modificationTime ||
        prev_rec.file_rec_header.usedSize != cur_rec.file_rec_header.usedSize ||
        prev_rec.file_name_attr_header.metadataModificationTime != cur_rec.file_name_attr_header.metadataModificationTime ||
        prev_rec.file_name_attr_header.allocatedSize != cur_rec.file_name_attr_header.allocatedSize ||
        prev_rec.file_name_attr_header.realSize != cur_rec.file_name_attr_header.realSize ||
        prev_rec.file_rec_header.allocatedSize != cur_rec.file_rec_header.allocatedSize ||
        prev_rec.file_rec_header.logSequence != cur_rec.file_rec_header.logSequence ||
        prev_rec.file_name_attr_header.flags != cur_rec.file_name_attr_header.flags) {
        return MODIFIED;
    }



    // If no changes detected, return NO_CHANGE
    return NO_CHANGE;


}

//find a record in the other MFT which matches the current record by FRN (FRN is a unique identifier for a file in NTFS and includes record number and sequence number)
std::pair<const uint64_t, Ntfs_c::FileRecord*>* Ntfs_c::FindRecord(std::pair<uint64_t, Ntfs_c::FileRecord*> rec, Ntfs_c& other) {

    assert(rec.second->file_rec_header.fileReference != rec.first);

    //find rec by FRN in other 
    auto it = other._file_references.find(rec.first);
    if (it == other._file_references.end()) {
        return nullptr; // record not found in the other MFT
    }

    return &(*it);
}




std::map<uint64_t, Ntfs_c::change_type_n > Ntfs_c::diff(Ntfs_c& other) {

    std::map<uint64_t, Ntfs_c::change_type_n > diff_map;

    //first find which files are missing in the other MFT - we search by FRN (FileReferenceNumber) which is a unique identifier for a file in NTFS and includes record number and sequence number
    for (auto& rec : _file_references) {
        //below searches both by file record which can be reassigned and by fileReferenceNumber
        auto p = FindRecord(rec, other);
        if (!p)
            diff_map[rec.first] = DELETED; // file is missing in the other MFT
    }

    // now find files which are present in other MFT but missing in current MFT
    for (auto& rec : other._file_references) {
        //below searches both by file record which can be reassigned and by fileReferenceNumber
        auto p = FindRecord(rec, *this);
        if (!p)
            diff_map[rec.first] = NEW; // file is missing in the current MFT
        else if (rec.first <= 11) { //skip system files - TBD check where this number comes from
            continue; //skip system files
        }
        else {
            //check if the file record was reassigned
            auto other_rec = other._file_records[rec.second->file_rec_header.recordNumber]; //get the file record from the other MFT
            auto cur_rec = _file_records[p->second->file_rec_header.recordNumber]; //get the file record from the current MFT
            auto change = detectRecordChange(other, other_rec, *this, cur_rec);
            if (change != NO_CHANGE) {
                diff_map[rec.first] = change; //record reassigned
            }
        }

    }
    return diff_map;

}




/*
int main(int argc, char** argv) {
    // Open the drive for reading


    Ntfs_c* vol = new Ntfs_c();

    //use "vssadmin List Shadows" to get VSS shadow copy volume names ..or open any live NTFS volume 
    //"\\\\?\\GLOBALROOT\\Device\\HarddiskVolumeShadowCopy3"

    if (!vol->init("\\\\.\\C:")) {
        std::cerr << "Error opening drive!" << std::endl;
        return 1;
    }






    while (true) {
        std::string cmd;
        std::cout << "mftinfo> ";
        std::getline(std::cin, cmd);
        if (cmd == "exit") {
            break;
        }
        else if (cmd == "ranges") {
            vol->printRanges();
        }
        else if (cmd == "names") {
            vol->printFileNames();
        }
        else if (cmd == "names_full") {
            vol->printFileNames(true);
        }
        else if (cmd == "help") {
            std::cout << "Commands: exit, ranges, names, names_full,file, ref" << std::endl;
        }
        else if (cmd == "ref") {
            // Get FileReferenceNumber
            std::cout << "Enter FileReferenceNumber: ";

            std::string inputStr;
            std::cin >> inputStr;
            int64_t ref_num = stringToInt64(inputStr);


            auto rec_num = vol->printRecordByFileReferenceNumber(ref_num);
        }
        else if (cmd == "file") {
            // Get the file name
            std::string name;
            std::cout << "Enter the file name: ";
            std::getline(std::cin, name);
            auto rec_num = vol->printRecordByFileName(name);
            if (rec_num)
                vol->printRangesByFileNumber(rec_num);
        }
        else if (cmd == "search") {
            // Get the file name
            uint64_t start;
            std::string startStr;
            std::cout << "starting cluster:";
            std::getline(std::cin, startStr);
            start = std::stoull(startStr);

            uint64_t len;
            std::string lenStr;
            std::cout << "length:";
            std::getline(std::cin, lenStr);
            len = std::stoull(lenStr);

            vol->findRanges(start, len);


        }
        else if (cmd == "exit") {
            break;
        }

        else {
            std::cout << "Unknown command." << std::endl;
        }
    }





    return 0;
}

*/

