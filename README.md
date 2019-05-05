# 系统说明书
----
## 系统的基本说明
### 系统简介
这是一个键值对数据库系统，能够实现基本的插入，查询，更新，删除操作。并且与Google的levelDB进行性能对比.
### 系统文件结构说明
```
project
├── gtest
├── include
│   ├── fptree
│   │   └── fptree.h
│   └── utility
│       ├── clhash.h
│       ├── p_allocator.h
│       └── utility.h: 注意修改头文件中的DATA_DIR路径到自己的pmem内存盘上面 
├── README.md
├── src
│   ├── bin
│   ├── clhash.c
│   ├── fptree.cpp: (TODO)
│   ├── lycsb.cpp: LevelDB的测试代码(TODO)(finished)
│   ├── main.cpp
│   ├── makefile
│   ├── p_allocator.cpp: NVM内存分配器文件(TODO)(finished)
│   ├── utility.cpp
│   └── ycsb.cpp: (TODO)
├── test
│   ├── bin
│   ├── fptree_test.cpp
│   ├── makefile
│   └── utility_test.cpp
└── workloads
```
### 编译
- 编译`src`目录下的文件
```
cd src
make all
```
- 编译`test`目录下的文件
```
cd test
mkdir bin
make all
```
## 说明
#### p_allocator.cpp

- Dependence: p_allocator.h, **utility.h(存放一个叶子节点对应的leafGroup文件的fileID与叶子在其中的偏移量, 声明NVM的路径并通过calLeafSize()得到叶子大小)**
- Function: 操作粒度为leafGroup, 为叶子节点在NVM中开辟空间
- 已通过所有的gtest
- Details:

1. **构造函数PAllocator()**: 如果catalog文件与freeList存在则初始化maxFIleId, freeNum, startLeaf, freeList变量, 如果不存在则创建相应的文件并向其中写入应有的值
2. **析构函数~PAllocator()**: 为了模仿NVM的环境, 需要调用persistCatalog()更新catalog文件, 然后更新freeList文件.
3. **initFilePmemAddr()**: 通过PMDK初始化各个File对应的虚拟内存地址, 把现有的leaf group file映射到虚拟内存地址中然后保存在fId2PmAddr变量中.
4. **getLeafPmemAddr()**: 得到一个叶子对应的NVM中虚拟内存地址, 通过fId2PmAddr得到此叶子对应的leaf group的虚拟地址, 然后加上偏移量offset即是此叶子在NVM中对应的虚拟内存地址.
5. **getLeaf()**: 申请一个叶子空间(需要先检测freeList中是否有足够的空闲叶子空间, 不够的话分配新的leaf group), 根据gtest文件, 我们把freeList中最后一个free leaf空间分配给此叶子.
6. **ifLeafUsed()**: 根据leaf group中的bitmap判断此叶子是否使用.
7. **ifLeafFree()**: return ifLeafUsed()? false: true;
8. **ifLeafExist()**: 判断此叶子对应的leaf group文件是否存在, 不存在的话说明此叶子不存在
9. **freeLeaf()**: 释放叶子在leaf group文件中的空间, 置bitmap为0, 并更新usedNum
10. **persistCatalog()**: 持久化catalog 文件, 也就是更新catalog 文件
11. **newLeafGroup()**: 分配一个新的leaf group文件, 写入0



## 任务规划
- 在5.4晚上之前完成系统的编译框架，levelDB的测试，p_allocator的实现和测试，编写gtest的测试文件，这次作业完成后我们商讨一下剩下代码的具体分配，将其中的插入，更新，删除操作并行处理，各个组员之间并行完成。
- 计划对于未来三周的任务以插入，更新，删除三个操作，为单位均匀分配到每个组员，独立负责。