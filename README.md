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

先把bootstrap运行起来
call stack
    BootstrapMain


tuple