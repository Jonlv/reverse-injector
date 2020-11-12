# reverse-injector

Inject a process into your context. You can call functions that do not reference absolute virtual addresses. Reverse injector copies a target processes pml4e's into another specified 
processes pml4. Current the project copies the targets pml4e's into the current process (reverse-injector.exe), but you can configure the project to inject a process into any process
you want.

# How?

```
VDM ---> PTM ---> reverse-injector
```

The project uses PTM which uses VDM. Althought VDM is utilized in this project you do not need to have VDM's vulnerable driver loaded into the kernel when creating an `injector_ctx`. 
As you can see in the demo I have modified VDM to change how it reads and writes to physical memory. I make VDM use a mem_ctx object to read/write physical memory. (mem_ctx is PTM). 
PTM manages its own set of paging tables from usermode and does not need a vulnerable driver after it has been initalized. 