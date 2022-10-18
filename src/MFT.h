#define UNICODE
// Yêu cầu compiler cấp phát bộ nhớ đúng với kdl (không padding)
#pragma pack(1)

#include <cstdint>
#include <cwchar>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdio.h>
#include <string>
#include <tuple>
#include <vector>
#include <windows.h>
using namespace std;

const wchar_t *CURRENT_DRIVE = L"\\\\.\\U:";
const char *ENTRY_FILENAME = "entry.bin";
long long START_CLUSTER = 786432;
long SECTOR_PER_CLUSTER = 8;
long SECTOR_SIZE = 512;
int STANDARD_INFORMATION_OFFSET = 56;

struct EntryHeader
{
    uint32_t signature;
    uint16_t offsetToUpdateSeq;
    uint16_t updateSeqSize;
    uint64_t logFileSeqNum;
    uint16_t seqNum;
    uint16_t hardLinkCount;
    uint16_t offsetToFirstAttr;
    uint16_t flags;
    uint32_t realSizeOfFileRecord;
    uint32_t allocatedSizeOfFileRecord;
    uint64_t baseFileRecord;
    uint16_t nextAttrID;
    uint16_t unused;
    uint32_t ID;
};

struct StandardAttributeHeader
{
    uint32_t attributeType;
    uint32_t totalLength;
    uint8_t nonResidentFlag;
    uint8_t nameLength;
    uint16_t nameOffset;
    uint16_t flags;
    uint16_t attrID;
    uint32_t attrDataLength;
    uint16_t offsetToAttrData;
    uint8_t indexFlag;
    uint8_t padding;
};

struct FileNameAttribute
{
    char parentID[6];
    char parentSeqNum[2];
    uint64_t fileCreated;
    uint64_t fileModified;
    uint64_t MFTChanged;
    uint64_t lastAccess;
    uint64_t allocatedSize;
    uint64_t realSize;
    uint32_t fileAttributes;
    uint32_t reparse;
    uint8_t fileNameLength;
    uint8_t fileNameFormat;
};

struct DataAttributeHeader
{
    uint32_t attributeType;
    uint32_t totalLength;
    uint8_t nonResidentFlag;
    uint8_t nameLength;
    uint16_t nameOffset;
    uint16_t flags;
    uint16_t attrID;
    uint64_t firstVCN;
    uint64_t lastVCN;
    uint16_t dataRunsOffset;
    uint16_t compressionUnitSize;
    uint32_t padding;
    uint64_t allocatedSize;
    uint64_t realSize;
    uint64_t initializedSize;
};

struct EntryInfos
{
    string entryName;
    int ID = -1;
    int parentID;
    int isDir;
    int isUsed;
};

void writeEntryToFile(BYTE entry[1024]);
void readEntry(tuple<int, int> &flags, unsigned int &parentID);
void printEntry(tuple<int, int> tp);
void saveEntryInfos(int parentID, tuple<int, int> flags);
void readEntryHeader();
tuple<int, int> readEntryFlags(uint16_t flags);
void readStandardInformation(int &currentOffset);
void readFileNameAttribute(int &currentOffset);
unsigned int readParentID(char parentID[6]);
void readFileName(FILE *fp, int fileNameLength);
void printFileName(int fileNameLength);
vector<string> split(const string &s, char delim = ' ');
int handleCommands(vector<string> args);
void printCurrDir();
void readFileContent(string input);
void releaseFileNameBuffer();
bool validateInputDirectory(string input, int parentID, int &ID);
EntryInfos findDirectoryInfos(string dirName, int parentID);