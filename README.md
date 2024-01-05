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

cost的计算方式


2023/12/18
尝试先复现executor



磁盘管理
    vfd
    为什么要有vfd？
    因为操作系统可能对打开对文件描述符对数量有限制。
    思考：什么时候真正对关闭文件描述符？
    打开文件失败的时候么？
    进程退出的时候 注册需要关闭所有文件描述符

    1. 预留了10个文件描述符给dlopen使用（RESERVE_FOR_LD）
    2. PostgreSQL至少需要10个文件描述符（FD_MINFREE）

    VfdCache数组 索引为0上的元素是不使用的。
    free_vfd不会将fd从lru环上删除，而是更新它的fdstate以及next_free状态。
    lru_delete才会真正的close对应的文件描述符

    allocate_vfd会进行扩容2倍，旧的VfdCache会拷贝到新的内存空间。
    索引为0的元素的next_free指向SizeVfdCache。
    需要注意什么时候会更新next_free
    为什么lru_delete的时候不更新next_free。

    用法就是 打开文件file_open不主动进行关闭
    让fd模块直接接管 最后close all vfds来关闭所有的文件。
    考虑：
    如果一直进行file_open那么达到一定的上限，least recent used文件会从lru环上删除
    如果dirty的话 会进行fsync 然后关闭。这个时候如果进行file_read被删除的文件描述符
    会怎么样？
    源代码中会先进行file_access。
    在file_access中会判断如果文件没有打开，那么通过lru_insert来重新打开文件。

    问题1
    fileClose操作
    将所有虚拟文件描述符从LRU环上删除，然后进行关闭。但是这个地方没有free vfd的操作。
    也就是说之后这些虚拟文件描述符就不可用了。

    md
    为什么要有md？
    因为操作系统可能对文件大小有限制（2GB？）。通过md模块将文件分成一个个segment。
    
    md_init
    创建了MdxCtx内存管理上下文
    初始化了100个MdfdVec，每一个MdfdVec都是空闲的（free）。

    md_create(Relation relation)
    打开表文件 返回虚拟的文件描述符
    
    md_unlink(RelFileNode)
    删除对应文件 如果有多个segments的话 同时删除
    文件名.segno

    md_fd_get_seg(Relation, int)
    根据block的id来找到对应的segment
    同时会更新md_chain字段