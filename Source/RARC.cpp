#include "RARC.h"

RARC::RARC(const std::string &rFileName) {
    BinaryReader* reader = new BinaryReader(rFileName, EndianSelect::Big);
    read(*reader);
}

void RARC::read(BinaryReader &rReader) {
    if (rReader.readString(0x4) != "RARC") {
        printf("Invalid identifier! Expected: RARC");
        return;
    }

    rReader.seek(0xC, std::ios::beg);
    mFileDataOffset = rReader.readU32(); + 0x20;
    rReader.seek(0x20, std::ios::beg);
    mDirNodeCount = rReader.readU32();
    mDirNodeOffset = rReader.readU32() + 0x20;
    rReader.skip(0x4);
    mFileEntriesOffset = rReader.readU32() + 0x20;
    rReader.skip(0x4);
    mStringTableOffset = rReader.readU32() + 0x20;

    DirNode* rootDir = new DirNode();
    rootDir->mEntryId = 0;
    rootDir->mParentId = 0xFFFFFFFF;

    rReader.seek(mDirNodeOffset + 0x6, std::ios::beg);
    u32 rootDirOffset = rReader.readU16();
    rReader.seek(mStringTableOffset + rootDirOffset, std::ios::beg);
    rootDir->mName = rReader.readNullTerminatedString();
    rootDir->mFullName = "/" + rootDir->mName;
    mDirNodes.push_back(rootDir);

    for (s32 i = 0; i < mDirNodeCount; i++) {
        DirNode* dir = mDirNodes[i];
        rReader.seek(mDirNodeOffset + (i * 0x10) + 10, std::ios::beg);
        u16 entryCount = rReader.readU16();
        u32 firstEntry = rReader.readU32();

        for (u32 y = 0; y < entryCount; y++) {
            u32 entryOffset = mFileEntriesOffset + ((y + firstEntry) * 0x14);   
            rReader.seek(entryOffset, std::ios::beg);

            u16 id = rReader.readU16();
            rReader.skip(0x4);
            u16 nameOffset = rReader.readU16();
            u16 dataOffset = rReader.readU32();
            u16 dataSize = rReader.readU32();

            rReader.seek(mStringTableOffset + nameOffset, std::ios::beg);
            std::string name = rReader.readNullTerminatedString();

            if (name == "." || name == "..")
                continue;

            std::string fullName = dir->mFullName + "/" + name;

            if (id == 0xFFFF) {
                DirNode* dir = new DirNode();
                dir->mEntryOffset = entryOffset;
                dir->mEntryId = dataOffset;
                dir->mParentId = i;
                dir->mNameOffset = nameOffset;
                dir->mName = name;
                dir->mFullName = fullName;

                mDirNodes.push_back(dir);
            }
            else {
                FileNode* file = new FileNode();
                file->mEntryOffset = entryOffset;
                file->mEntryId = id;
                file->mParentDirId = i;
                file->mNameOffset = nameOffset;
                file->mDataOffset = dataOffset;
                file->mDataSize = dataSize;
                file->mName = name;
                file->mFullName = fullName;     

                mFileNodes.push_back(file);
            }
        }

        rReader.~BinaryReader();
    }
}