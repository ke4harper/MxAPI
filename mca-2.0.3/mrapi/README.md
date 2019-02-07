# MRAPI

Multicore Association Resource APIs

MRAPI is the Multicore Resource Management API that handles memory management, basic synchronization, resource registration, and resource partitioning. The reference implementation uses an operating system semaphore guarding a shared memory database to provide user mode synchronization and resource objects.  

Determining whether a concurrency approach satisfies the architectural non-functional requirements means running tests to verify the I/O performance and validate deterministic and reliable communication.  
## Portability Sensitivity
Adding a portability layer to the runtime stack may have a performance impact. Anecdotally, stack frames can cause overhead in preparing and returning function arguments. On the other hand, compilers today are very efficient at using registers to speed many of the common operations.  

![SingleCPU](img/Portability Sensitivity - Single CPU.png)

*Portability Sensitivity - Single CPU*

The portability layer running on a single CPU does not have any significant impact on performance as shown by the 1x speedup calculations in the table above. In general, data exchange within a single isolated process is more efficient than when two processes have shared memory between them, with the exception of process-to-process compared to task-to-task memory access in the RTP deployment. What is interesting is that Private exchange (using global memory) between two tasks is slower when the process is also connected to another process via shared memory. This suggests the OS memory manager is adding some overhead even for access within the same address space. The exception is Linux Native where the compiler may be optimizing inline coding. Finally, the portability layer actually reduces latency on Linux within an isolated process. This may be due to pipelined processing in the CPU.  

The portability layer running on multicore also does not have a significant impact on performance as shown by the 1x speedup calculations in the table below. Memory access within an isolated process does not perform much better (1.4x max.) than between two tasks in the presence of shared memory, and actually performs incrementally better between two processes on both Windows and Linux. Like single CPU, process-to-process memory access performs better than task-to-task in the RTP deployment.  

![MultiCPU](img/Portability Sensitivity – Multicore.png)

*Portability Sensitivity – Multicore*

## One-Way Communication Performance
The one-way communication channel is the building block for all other messaging structures, including full-duplex, one-to-many, etc. Prior studies of RTOS messaging performance was in the 10’s of thousands and the requirements were met by buffering many messages in the same transaction. Much higher transaction rates are possible using lock-free techniques, as shown in the table below. The throughputs are in the 100’s of thousands of transactions per second, and the channel propagation delays are less than 10 microseconds. Data exchange within isolated processes is more efficient by a significant amount so this must be balanced with the reliability that comes from real-time processes. Note also that as latency increases so does the throughput, making the number of requests bandwidth fairly constant.  

Linux performs better than Windows in a single CPU RTP deployment, but Windows out-paces Linux on both Kernel and RTP multicore deployments. For both platforms it makes sense to go to multicore because of the speedups (as much as 3.3x on Windows) that are achieved. The exception is Linux RTP where the latency increases by more than a factor of 2, but the throughput increases as well. The bottom line based on Requests bandwidth is that on multicore, Windows performs better with a pool of threads and Linux performs better with a pool of processes.  

![Tradeoffs](img/Platform and Memory Access Tradeoffs.png)

*Platform and Memory Access Tradeoffs*

## Design Rules
The I/O performance numbers indicate a single runtime implementation for both Windows and Linux does not provide optimal results. Here are some design rules<sup>[4](#DesignRules)</sup> from the Unix perspective that can guide the platform variants and deliver the best performance.
1.	If you want to make debugging easier, use threads.
2.	If you are on Windows, use threads (Processes are extremely heavyweight in Windows).
3.	If stability is a huge concern, try to use processes (One SIGSEGV/PIPE is all it takes…).
4.	If threads are not available, use processes (Not so common now, but it did happen).
5.	If your threads share resources that can’t be used from multiple processes, use threads. (Or provide an IPC mechanism to allow communicating with the “owner” thread of the resource).
6.	If you use resources that are only available on a one-per-process basis, obviously use processes.
7.	If your processing contexts share absolutely nothing (such as a socket server that spawns and forgets connections as it accept(s) them), and CPU is a bottleneck, use processes and single-threaded runtimes (which are devoid of all kinds of intensive locking such as on the heap and other places).
8.	One of the biggest differences between threads and processes is this: [On Linux] Threads use software constructs to protect data structures, processes use hardware (which is significantly faster).

## Atomic Operations Across Processes
One of the solution assumptions is that communication between tasks is “frictionless”, i.e. very low latency. The lock-free techniques use synchronization between tasks and for RTP deployments this must be available across processes. One task must not be able to corrupt the data used by another task. Our experiments demonstrate this capability. As shown in the figure below, the techniques are different for Windows and Linux.

![ProcessAtomic](img/Atomic Operations Across Processes.png)

*Atomic Operations Across Processes*

On Windows it is possible to duplicate a shared memory handle and pass it to another process so it can access the same physical memory. When the two processes are attached any InterlockedXXX operations work to block one process’ access to a memory location while the other is engaged with the same location. The atomic CPU instructions act as very lightweight locks for guaranteeing data is not corrupted.  

On Linux it is not possible to share the same physical memory between processes, so a spin wait technique is used instead. One process copies the data to be exchanged into shared memory, and the other process copies the data from shared memory to receive the exchange. A counter in the shared memory is incremented by the writer at the beginning of an update and again when the update is complete. The writer has ownership when the counter is odd, and the reader has ownership when the counter is even. Experiments show this technique does not consume substantial CPU resources and exchanges data reliably.  

