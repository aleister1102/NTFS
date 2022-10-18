#include "MFT.h"

EntryHeader EH;
StandardAttributeHeader SAH;
FileNameAttribute FNA;

uint16_t *currentFileName = nullptr;
vector<int> directoryStack = {5};
vector<EntryInfos> directoryEntries;

void getEntry(LPCWSTR drive, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, BYTE entry[1024])
{
    int retCode = 0;
    DWORD bytesRead;
    HANDLE device = NULL;

    device = CreateFile(drive,                              // Drive to open
                        GENERIC_READ,                       // Access mode
                        FILE_SHARE_READ | FILE_SHARE_WRITE, // Share Mode
                        NULL,                               // Security Descriptor
                        OPEN_EXISTING,                      // How to create
                        0,                                  // File attributes
                        NULL);                              // Handle to template

    if (device == INVALID_HANDLE_VALUE) // Open Error
    {
        printf("CreateFile: %u\n", GetLastError());
        return;
    }

    SetFilePointer(device, lDistanceToMove, lpDistanceToMoveHigh, FILE_BEGIN); // Set a Point to Read

    if (!ReadFile(device, entry, 1024, &bytesRead, NULL))
    {
        printf("ReadFile: %u\n", GetLastError());
    }
    else
    {
        // printf("Success!\n");
    }
}

void getNthEntryAndWriteToFile(BYTE entry[1024], int entryOffset = 0)
{
    LARGE_INTEGER li;

    uint64_t startSector = START_CLUSTER * SECTOR_PER_CLUSTER * SECTOR_SIZE;
    uint64_t bypassSector = entryOffset * 2 * SECTOR_SIZE;
    uint64_t readSector = startSector + bypassSector;
    li.QuadPart = readSector;

    getEntry(CURRENT_DRIVE, li.LowPart, &li.HighPart, entry);
    writeEntryToFile(entry);
}

void writeEntryToFile(BYTE entry[1024])
{
    FILE *fp = fopen(ENTRY_FILENAME, "wb");
    fwrite(entry, 1, 1024, fp);
    fclose(fp);
}

void getCurrDir(unsigned int currDirID = 5)
{
    cout << "-----------------------------------------------------------------------------" << endl;
    cout << "Type\tStatus\t\tID\tName" << endl;

    // Khi nào thì MFT kết thúc?
    for (int i = 0; i < 100; i++)
    {
        BYTE entry[1024];
        tuple<int, int> flags;
        unsigned int parentID;

        getNthEntryAndWriteToFile(entry, i);
        readEntry(flags, parentID);

        if (parentID == currDirID && currentFileName)
        {
            // Bỏ qua các entry không phải là file hoặc thư mục
            if (!EH.ID && i > 0)
                continue;

            saveEntryInfos(parentID, flags);
            printEntry(flags);
        }
        parentID = -1;
    }

    cout << "-----------------------------------------------------------------------------" << endl;
}

void readEntry(tuple<int, int> &flags, unsigned int &parentID)
{
    int currentOffset = STANDARD_INFORMATION_OFFSET;

    readEntryHeader();
    flags = readEntryFlags(EH.flags);
    readStandardInformation(currentOffset);
    readFileNameAttribute(currentOffset);
    parentID = readParentID(FNA.parentID);
}

void printEntry(tuple<int, int> tp)
{
    // Loại và trạng thái của entry
    string isDir = get<0>(tp) ? "dir" : "file";
    cout << isDir << "\t";
    string isUsed = get<1>(tp) ? "being used" : "";
    cout << isUsed << "\t";

    // ID của entry
    cout << EH.ID << "\t";

    // Tên của entry
    printFileName(FNA.fileNameLength);
    releaseFileNameBuffer();
    cout << endl;
}

void saveEntryInfos(int parentID, tuple<int, int> flags)
{
    EntryInfos EI;

    // Chuyển tên của entry thành string
    char *buffer = new char[FNA.fileNameLength + 1];
    for (int i = 0; i < FNA.fileNameLength; i++)
        buffer[i] = (char)currentFileName[i];
    buffer[FNA.fileNameLength] = '\0';

    EI.entryName = buffer;
    EI.ID = EH.ID;
    EI.parentID = parentID;
    EI.isDir = get<0>(flags);
    EI.isUsed = get<1>(flags);

    directoryEntries.push_back(EI);
}

void readEntryHeader()
{
    FILE *fp = fopen(ENTRY_FILENAME, "rb");
    fread(&EH, sizeof(EH), 1, fp);
    fclose(fp);
}

tuple<int, int> readEntryFlags(uint16_t flags)
{
    // Bit 0: trạng thái sử dụng
    int isUsed = flags & 1;
    // Bit 1: loại entry (tập tin hoặc thư mục)
    flags = flags >> 1;
    int isDir = flags & 1;

    return make_tuple(isDir, isUsed);
}

void readStandardInformation(int &currentOffset)
{
    FILE *fp = fopen(ENTRY_FILENAME, "rb");
    readStandardAttributeHeader(fp, currentOffset);
    fclose(fp);
}

void readFileNameAttribute(int &currentOffset)
{
    FILE *fp = fopen(ENTRY_FILENAME, "rb");
    readStandardAttributeHeader(fp, currentOffset);

    if (SAH.attributeType != 48)
        return;

    fread(&FNA, sizeof(FNA), 1, fp);
    readFileName(fp, FNA.fileNameLength);
    fclose(fp);
}

void readStandardAttributeHeader(FILE *fp, int &currentOffset)
{
    fseek(fp, currentOffset, SEEK_SET);
    fread(&SAH, sizeof(SAH), 1, fp);
    currentOffset += SAH.totalLength;
}

uint32_t readParentID(char parentID[6])
{
    uint32_t buffer;
    memcpy(&buffer, parentID, 6);
    return buffer;
}

void readFileName(FILE *fp, int fileNameLength)
{
    currentFileName = new uint16_t[100];

    for (int i = 0; i < fileNameLength; i++)
        fread(&currentFileName[i], 2, 1, fp);
}

void printFileName(int fileNameLength)
{
    for (int i = 0; i < fileNameLength; i++)
        cout << (char)currentFileName[i];
}

void releaseFileNameBuffer()
{
    delete[] currentFileName;
    currentFileName = nullptr;
}

void readDataAttribute(int currOffset)
{
    FILE *fp = fopen("MFT.bin", "rb");
    fseek(fp, currOffset, SEEK_SET);

    DataAttributeHeader DH;
    fread(&DH, sizeof(DH), 1, fp);
    fclose(fp);

    currOffset += DH.totalLength;
}

void menu()
{
    int running = true;

    do
    {
        wcout << CURRENT_DRIVE[4] << "\\";
        printCurrDir();

        string command;
        getline(cin, command);
        vector<string> args = split(command);
        running = handleCommands(args);
    } while (running);
}

void printCurrDir()
{
    for (int i = 0; i < directoryStack.size(); i++)
    {
        int dirID = directoryStack.at(i);
        for (int i = 0; i < directoryEntries.size(); i++)
        {
            if (directoryEntries.at(i).ID == dirID)
            {
                if (directoryEntries.at(i).entryName != ".")
                {
                    cout << directoryEntries.at(i).entryName << "\\";
                    break;
                }
            }
        }
    }

    cout << " > ";
}

vector<string> split(const string &s, char delim)
{
    vector<string> result;
    stringstream ss(s);
    string item;

    while (getline(ss, item, delim))
    {
        result.push_back(item);
    }

    return result;
}

int handleCommands(vector<string> args)
{
    if (args[0] == "cls")
        system("cls");
    else if (args[0] == "dir")
        getCurrDir(directoryStack.back());
    else if (args[0] == "cd")
    {
        if (args.size() < 2)
            cout << "Missing input" << endl;
        else if (args[1] == "..")
        {
            if (directoryStack.back() != 5)
                directoryStack.pop_back();
            return 1;
        }
        else
        {
            int entryID;
            if (validateInputDirectory(args[1], directoryStack.back(), entryID))
                directoryStack.push_back(entryID);
        }
    }
    else if (args[0] == "cat")
    {
        if (args.size() < 2)
            cout << "Missing input" << endl;
        else
        {
            string input = args[1];
            readFileContent(input);
        }
    }
    else if (args[0] == "exit")
        return 0;
    else
        cout << "Can not regconize '" << args[0] << "' command" << endl;

    return 1;
}

bool validateInputDirectory(string input, int parentID, int &ID)
{
    EntryInfos entryInfos = findEntryInfos(input, directoryStack.back());
    bool valid = true;

    if (entryInfos.ID < 0)
    {
        cout << "Can not find " << input << endl;
        valid = false;
    }
    else if (!entryInfos.isDir)
    {
        cout << input << " is not directory" << endl;
        valid = false;
    }
    else
        ID = entryInfos.ID;

    return valid;
}

EntryInfos findEntryInfos(string dirName, int parentID)
{
    for (auto entry : directoryEntries)
    {
        if (entry.parentID == parentID && entry.entryName == dirName)
            return entry;
    }

    return EntryInfos();
}

void readFileContent(string input)
{
    EntryInfos entryInfos = findEntryInfos(input, directoryStack.back());

    if (entryInfos.ID < 0)
    {
        cout << "Can not find " << input << endl;
        return;
    }

    BYTE entry[1024];
    FILE *fp = fopen(ENTRY_FILENAME, "rb");
    int currentOffset = STANDARD_INFORMATION_OFFSET;

    getNthEntryAndWriteToFile(entry, entryInfos.ID);
    readEntryHeader();

    do
    {
        if (SAH.attributeType == 0x80)
            printFileContent(fp, currentOffset);

        readStandardAttributeHeader(fp, currentOffset);

    } while (SAH.attributeType != 0xFFFFFFFF);
}

void printFileContent(FILE *fp, int currentOffset)
{
    int dataOffset = currentOffset - SAH.totalLength + sizeof(StandardAttributeHeader);
    fseek(fp, dataOffset, SEEK_SET);

    char buffer;
    for (int i = 0; i < SAH.attrDataLength; i++)
    {
        fread(&buffer, 1, 1, fp);
        cout << buffer;
    }

    cout << endl;
}

int main(int argc, char **argv)
{
    getCurrDir();
    menu();

    return 0;
}