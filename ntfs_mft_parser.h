#pragma once


#include <stdint.h>
#include <windows.h>

#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <list>
#include <algorithm>

#pragma pack(push,1)
struct BootSector {
    uint8_t     jump[3];
    char        name[8];
    uint16_t    bytesPerSector;
    uint8_t     sectorsPerCluster;
    uint16_t    reservedSectors;
    uint8_t     unused0[3];
    uint16_t    unused1;
    uint8_t     media;
    uint16_t    unused2;
    uint16_t    sectorsPerTrack;
    uint16_t    headsPerCylinder;
    uint32_t    hiddenSectors;
    uint32_t    unused3;
    uint32_t    unused4;
    uint64_t    totalSectors;
    uint64_t    mftStart;
    uint64_t    mftMirrorStart;
    uint32_t    clustersPerFileRecord;
    uint32_t    clustersPerIndexBlock;
    uint64_t    serialNumber;
    uint32_t    checksum;
    uint8_t     bootloader[426];
    uint16_t    bootSignature;
};


struct FileRecordHeader {
    uint32_t    magic;
    uint16_t    updateSequenceOffset;
    uint16_t    updateSequenceSize;
    uint64_t    logSequence;
    uint16_t    sequenceNumber;
    uint16_t    hardLinkCount;
    uint16_t    firstAttributeOffset;
    uint16_t    inUse : 1;
    uint16_t    isDirectory : 1;
    uint32_t    usedSize;
    uint32_t    allocatedSize;
    uint64_t    fileReference;
    uint16_t    nextAttributeID;
    uint16_t    unused;
    uint32_t    recordNumber;
};

struct AttributeHeader {
    uint32_t    attributeType;
    uint32_t    length;
    uint8_t     nonResident;
    uint8_t     nameLength;
    uint16_t    nameOffset;
    uint16_t    flags;
    uint16_t    attributeID;
};

struct ResidentAttributeHeader : AttributeHeader {
    uint32_t    attributeLength;
    uint16_t    attributeOffset;
    uint8_t     indexed;
    uint8_t     unused;
};

struct FileNameAttributeHeader : ResidentAttributeHeader {
    uint64_t    parentRecordNumber : 48;
    uint64_t    sequenceNumber : 16;
    uint64_t    creationTime;
    uint64_t    modificationTime;
    uint64_t    metadataModificationTime;
    uint64_t    readTime;
    uint64_t    allocatedSize;
    uint64_t    realSize;
    uint32_t    flags;
    uint32_t    repase;
    uint8_t     fileNameLength;
    uint8_t     namespaceType;
    wchar_t     fileName[1];
};

struct NonResidentAttributeHeader : AttributeHeader {
    uint64_t    firstCluster;
    uint64_t    lastCluster;
    uint16_t    dataRunsOffset;
    uint16_t    compressionUnit;
    uint32_t    unused;
    uint64_t    attributeAllocated;
    uint64_t    attributeSize;
    uint64_t    streamDataSize;
};

struct RunHeader {
    uint8_t     lengthFieldBytes : 4;
    uint8_t     offsetFieldBytes : 4;
};
#pragma pack(pop)


using namespace std;

class Ntfs_c {

public:
    bool init(std::wstring deicePath);

    struct FileRecord {
        void addName(std::wstring name) { names.push_back(name); }
        struct FileRecordHeader file_rec_header;
        struct FileNameAttributeHeader file_name_attr_header;
        std::vector<std::wstring> names;
    };


    struct FileRange {
        uint64_t start;
        uint64_t length;
        uint64_t recordNumber;
    };

    void printRanges();
    void printRangesByFileNumber(uint64_t recordNumber);

    void printRecordByNumber(uint64_t recordNumber);
    uint64_t printRecordByFileName(std::wstring name);

    uint64_t printRecordByFileReferenceNumber(uint64_t  ref_num);


    std::wstring getPathByFileReferenceNumber(uint64_t ref_num, bool absolute) {
        auto recNum = _getRecordByFileReferenceNumber(ref_num);
        if (0 == recNum) {
			return L"";
		}
        return printRecordName(recNum, absolute);
    }


    std::wstring printRecordName(uint64_t recNum, bool absolute = false);
    void printFileNames(bool absolute = false);
    void findRanges(uint64_t start, uint64_t len);
    void getResidentDataRun(ResidentAttributeHeader* dataAttribute);
    vector<pair<uint64_t, uint64_t>>  getNonResidentDataRuns(NonResidentAttributeHeader* dataAttribute);


private:
    HANDLE _drive;
    BootSector _bootSector;
    uint64_t _bytesPerCluster;


    static const int MFT_FILE_SIZE{ 1024 };
    uint8_t mftFile[MFT_FILE_SIZE];

    static const int MFT_FILES_PER_BUFFER{ 65536 };
    uint8_t mftBuffer[MFT_FILES_PER_BUFFER * MFT_FILE_SIZE];


    void Read(void* buffer, uint64_t from, uint64_t count) {
        DWORD bytesAccessed;

        LONG high = from >> 32;
        SetFilePointer(_drive, from & 0xFFFFFFFF, &high, FILE_BEGIN);
        ReadFile(_drive, buffer, (DWORD)count, &bytesAccessed, NULL);
        assert(bytesAccessed == count);
    }

    uint64_t _createFileReferenceNumber(uint64_t mftRecordNumber, uint16_t sequenceNumber);
    uint64_t _createFileReferenceNumber(FileRecord& rec) {
        return _createFileReferenceNumber(rec.file_rec_header.recordNumber, rec.file_rec_header.sequenceNumber);
    }
    uint64_t _getRecordByFileReferenceNumber(uint64_t  ref_num);

    std::map<uint64_t, struct FileRecord> _file_records;
    std::map<uint64_t, struct FileRecord*> _file_references;


    std::list<FileRange> _ranges;
};
