#include"utility/p_allocator.h"
#include<iostream>
#include<cstring>
using namespace std;

/* utility.h
typedef unsigned char Byte;
typedef uint64_t  Key;    // key(8 byte)
typedef uint64_t  Value;  // value(8 byte)

const string DATA_DIR =  ""; // TODO

typedef struct t_PPointer
{
    uint64_t fileId;
    uint64_t offset;

    bool operator==(const t_PPointer p) const;
} PPointer;

 * my path: const string DATA_DIR = "../../../Pmem/"
 * in utility.h
 */

// the file that store the information of allocator
const string P_ALLOCATOR_CATALOG_NAME = "p_allocator_catalog";
// a list storing the free leaves
const string P_ALLOCATOR_FREE_LIST    = "free_list";

PAllocator* PAllocator::pAllocator = new PAllocator();

PAllocator* PAllocator::getAllocator() {
    if (PAllocator::pAllocator == NULL) {
        PAllocator::pAllocator = new PAllocator();
    }
    return PAllocator::pAllocator;
}

/* data storing structure of allocator
   In the catalog file, the data structure is listed below
   | maxFileId(8 bytes) | freeNum = m | treeStartLeaf(the PPointer) |
   In freeList file:
   | freeList{(fId, offset)1,...(fId, offset)m} |
*/
PAllocator::PAllocator() {
    string allocatorCatalogPath = DATA_DIR + P_ALLOCATOR_CATALOG_NAME;
    string freeListPath         = DATA_DIR + P_ALLOCATOR_FREE_LIST;
    ifstream allocatorCatalog(allocatorCatalogPath, ios::in|ios::binary);
    ifstream freeListFile(freeListPath, ios::in|ios::binary);
    // judge if the catalog exists
    if (allocatorCatalog.is_open() && freeListFile.is_open()) {
        // exist
        // TODO
        // 初始化maxFileId, freeNum, startLeaf, freeList
        uint64_t value[5];
        allocatorCatalog.read((char*)(&value), 4 * sizeof(uint64_t));
        maxFileId = value[0] + 1, freeNum = value[1];
        startLeaf.fileId = value[2];
        startLeaf.offset = value[3];

        for(uint64_t i = 0; i < freeNum; ++i){
            PPointer temp;
            freeListFile.read((char *)(&temp), sizeof(PPointer));
            freeList.push_back(temp);
        }
    } else {
        // not exist, create catalog and free_list file, then open.
        // TODO
        // 判断哪个文件不存在, 然后创建该文件
        if(!allocatorCatalog.is_open()){
            ofstream temp(allocatorCatalogPath, ios::out);

            uint64_t value = 1;
            temp.write((char *)&(value), sizeof(uint64_t));
            value = 0;
            temp.write((char *)&(value), sizeof(uint64_t));
            value = 1;
            temp.write((char *)&(value), sizeof(uint64_t));
            value = 0;
            temp.write((char *)&(value), sizeof(uint64_t));

            temp.close();
            allocatorCatalog.open(allocatorCatalogPath, ios::in | ios::binary);
            if(!allocatorCatalog.is_open()) //check
                cout << "open allocatorCatalog failed!" << endl;
        }
        if(!freeListFile.is_open()){
            maxFileId = 1, freeNum = 0;
            ofstream temp(freeListPath, ios::out);

            uint64_t value = 1;
            temp.write((char *)&(value), sizeof(uint64_t));
            value = 0;
            temp.write((char *)&(value), sizeof(uint64_t));

            temp.close();
            freeListFile.open(freeListPath, ios::in | ios::binary);
            if(!freeListFile.is_open()) //check
                cout << "open freeListFile failed!" << endl;
        }
        maxFileId = 1, freeNum = 0;

    }
    this->initFilePmemAddr();
}

PAllocator::~PAllocator() {
    // TODO
    // 更新catalog file与free list file
    persistCatalog();
    string freeListPath = DATA_DIR + P_ALLOCATOR_FREE_LIST;
    ofstream mfile(freeListPath, ios::out|ios::binary);
    for(int i = 0; i < freeNum; ++i){
        PPointer temp = freeList[i];
        mfile.write((char *)(&temp), sizeof(PPointer));
    }
    mfile.close();
    pAllocator = nullptr;
}

// memory map all leaves to pmem address, storing them in the fId2PmAddr
void PAllocator::initFilePmemAddr() {
    // TODO
    // 把现有的leaf group file映射到虚拟内存然后存入fId2PmAddr
    size_t mapped_len;
    int is_pmem;
    char *pmem_addr;
    for(int i = 1; i < maxFileId; ++i){
        string name = DATA_DIR + std::to_string(i);
        pmem_addr = (char *)pmem_map_file(name.c_str(), LEAF_GROUP_HEAD + LEAF_GROUP_AMOUNT * calLeafSize(), PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem);
        if(pmem_addr == NULL){
            cout << "[error]: pmem_map_file " << i << " failed" << std::endl;
            continue;
        }
        fId2PmAddr.insert(pair<uint64_t, char*>(i, pmem_addr));
    }
}

// get the pmem address of the target PPointer from the map fId2PmAddr
char* PAllocator::getLeafPmemAddr(PPointer p) {
    // TODO
    // 得到map中存储的PPointer对应的fileId然后加上偏移量就是叶子的虚拟地址
    if(p.fileId == ILLEGAL_FILE_ID || p.fileId >= maxFileId)
        return NULL;
    return fId2PmAddr[p.fileId] + p.offset;
}

// get and use a leaf for the fptree leaf allocation
// return 
bool PAllocator::getLeaf(PPointer &p, char* &pmem_addr) {
    // TODO
    // 得到freeList中的最后一个叶子并修改叶子对应文件中的数据.
    if(freeList.empty())
        if(!newLeafGroup())
            return false;
    p = freeList.back();
    pmem_addr = getLeafPmemAddr(p);
    freeList.pop_back();
    --freeNum;
    string name = DATA_DIR + to_string(p.fileId); 
    fstream mfile(name, ios::in | ios::out | ios::binary);
    uint64_t usedNum;

    mfile.read((char *)&(usedNum), sizeof(uint64_t));
    ++usedNum;
    mfile.seekp(0, ios::beg);
    mfile.write((char *)&(usedNum), sizeof(uint64_t));

    uint8_t mbyte = 1;
    //开头usedNum 8个字节加上对应的是第几个叶子即可得到当前的叶子bitmap地址
    mfile.seekp(sizeof(uint64_t) + (p.offset - LEAF_GROUP_HEAD)/calLeafSize(), ios::beg);
    mfile.write((char *)&(mbyte), sizeof(uint8_t));
    mfile.close();
    return true;
}

bool PAllocator::ifLeafUsed(PPointer p) {
    // TODO
    // 读取文件的bitmap
    string name = DATA_DIR + to_string(p.fileId);
    ifstream mfile(name, ios::in | ios::binary);
    Byte data = 0;
    mfile.seekg(sizeof(uint64_t) + (p.offset - LEAF_GROUP_HEAD)/calLeafSize(), ios::beg);
    mfile.read((char *)&(data), sizeof(uint8_t));
    return (data == 1)? true: false;
}

bool PAllocator::ifLeafFree(PPointer p) {
    // TODO
    if(ifLeafUsed(p))
        return false;
    return true;
}

// judge whether the leaf with specific PPointer exists. 
bool PAllocator::ifLeafExist(PPointer p) {
    // TODO
    // 检查PPointer对应的文件是否存在, 如果存在则说明此叶子存在
    string name = DATA_DIR + to_string(p.fileId);
    ifstream mfile(name, ios::in | ios::binary);
    if(!mfile.is_open())
        return false;
    mfile.close();
    return true;
}

// free and reuse a leaf
bool PAllocator::freeLeaf(PPointer p) {
    // TODO
    // 在leaf group中的bitmap置0, 并修改freeList
    string name = DATA_DIR + to_string(p.fileId);
    fstream mfile(name, ios::in | ios::out | ios::binary);
    if(!mfile.is_open())
        return false;
    uint64_t moffset = (p.offset - LEAF_GROUP_HEAD) / calLeafSize();
    mfile.seekp(sizeof(uint64_t) + moffset, ios::beg);
    Byte data;
    data = 0;
    mfile.write((char*)&data, sizeof(data));

    uint64_t usedNum;
    mfile.seekg(0, ios::beg);
    mfile.read((char *)&usedNum, sizeof(usedNum));
    --usedNum;
    mfile.seekp(0, ios::beg);
    mfile.write((char *)&usedNum, sizeof(uint64_t));
    mfile.close();
    freeList.push_back(p);
    ++freeNum;
    return true;
}

bool PAllocator::persistCatalog() {
    // TODO
    // 持久化catalog文件
    string name = DATA_DIR + P_ALLOCATOR_CATALOG_NAME;
    ofstream mfile(name, ios::out | ios::binary);
    if(!mfile.is_open())
        return false;
    mfile.write((char *)(&maxFileId), sizeof(uint64_t));
    mfile.write((char *)(&freeNum), sizeof(uint64_t));
    mfile.write((char *)(&startLeaf), sizeof(PPointer));
    return true;
}

/*
  Leaf group structure: (uncompressed)
  | usedNum(8b) | bitmap(n * byte) | leaf1 |...| leafn |
*/
// create a new leafgroup, one file per leafgroup
bool PAllocator::newLeafGroup() {
    // TODO
    // maxFileId代表应该创建的文件名称
    // 一个leaf的度为56
    string name = DATA_DIR + std::to_string(maxFileId);
    ofstream mfile(name, ios::out | ios::binary);
    if(!mfile.is_open())
        return false;
    uint64_t usedNum = 0;
    uint8_t bit[LEAF_GROUP_HEAD * (1 + calLeafSize())];
    memset(bit, 0, sizeof(bit));
    mfile.write((char *)&usedNum, sizeof(uint64_t));
    mfile.write((char *)bit, sizeof(bit));
    for(int i = 0; i < 16; ++i){
        PPointer temp;
        temp.fileId = maxFileId, temp.offset = LEAF_GROUP_HEAD + i * calLeafSize();
        freeList.push_back(temp);
    }
    ++maxFileId;
    freeNum += 16;
    return true;
}