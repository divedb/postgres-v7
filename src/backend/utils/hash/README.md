Extendible Hash

1. Hash tables can be designed to have a fixed capacity or to grow semalessly with the data stored
in it.
- Static hashing: The capacity of the table is fixed at creation. If the data outgrows the capacity
a rehash is necessary.
- Dynamic Hashing: The capacity of the table begins small to accommodate initial data but grows (or
shrinks) as new data is added (or deleted).


1. Linear Hashing
    同时使用2个哈希函数来解决动态调整大小的问题
    当哈希数组需要扩张大小时，从前向后进行，当前正在扩张的 bucket 下标记为 p。对于在 p 之前的位置，使用hi+1，对于 p 及其之后的位置仍然使用 hi。这样就可以非常平滑的，每次操作只扩张一个 bucket，而不需要把所有的元素都 rehash。 不过这样做有一个缺点，就是有可能有一个位置上比较靠后的 bucket 一直比较拥挤，经过很多次插入后，才能对这个 bucket 进行扩张以缓解性能下降。对于这一问题，Spiral Storage 的方法处理的比较好。

2. Spiral Storage
    Spiral Storage 总是将负载更多的放在哈希表靠前的位置上，而非均匀地将负载分配到整个哈希表中。这样尽管是像 Linear Hashing 一样，总是从哈希表的头部开始进行 bucket 的分裂，也不会有不及时处理非常满的 bucket 的问题。
    Spiral Storage 的思路是这样的。哈希表的负载从前向后逐渐降低；扩展大小时，需要将表头的 bucket 中的元素分配到多个新 bucket 中并添加到哈希表的末尾，并且依然保持负载从前向后逐渐下降的性质。假设每去掉表头的一个 bucket 就添加 d 个新 bucket，称 d 为哈希表的增长因子。考虑到哈希表是非线性增加大小的，应该采用一个非线性增长的哈希函数族，将 U 映射到 R。易发现指数函数满足这样的性质。

3. Extendible Hashing
    将 bucket 和 bucket 的索引分别存放，使用 bucket 对应key的前缀对其进行索引，这样在扩展哈希表的大小时，就无需复制所有对象调整索引部分即可。


https://www.jianshu.com/p/8d9c22d26e34
https://www.youtube.com/watch?v=SiAHYSBkFTY