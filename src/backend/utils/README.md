1. MemoryContext depends elog and elog depends ipc since it needs execute some callbacks when exits.
And ipc replies on MemoryContext, a cycle exists.