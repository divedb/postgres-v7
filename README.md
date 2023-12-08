Learn the internals of PostgreSQL.
Adaptation from v7.0.

2023/11/05
prs2lock -> pg_list

spinlock有多个 LockManager, ShmemLock都有锁
lock模块到底是如何工作的

共享内存
    1、固定大小的 postgres初始化之后就不能更改
    2、Hashtable可以动态增加或者删除
    3、Queue
    共享内存存储哪些数据？

    ShmemIndex作用是什么

    ipc模块
    创造几个共享内存

    s_lock通过tas指令实现一个简单的lock

trace
    openlog


实现计划
    ipc   信号量和共享内存
    ipci  是对ipc的一些初始化
    shmem hash和ipc 
    lock


tuple

MemoryContext
    通过MemoryContextInit初始化，底层是通过AllocSet来实现
    TopMemoryContext是通过malloc分配内存的
    其余的context是依赖于parent context的

    context的内存分配策略是
    1. 先看下需要分配的内存大小 如果大于max chunk size 就直接分配一个block
    2. 否则 先看下free list中有没有合适的 如果有则直接从free list中分配
    3. 如果freelist中没有 那么从第一个block看看 是否有足够的内存 如果没有的话
       就把block中剩余的分配到freelist中
    4. 最终没有的 直接分配一个block 并放在链表的第一个block

AllocSet分配策略
    超过8K的，直接malloc分配

AllocSet释放策略
    >8K的，直接free 归还给操作系统
    <=8K的 放到freelist中


先把bootstrap运行起来
call stack
    BootstrapMain