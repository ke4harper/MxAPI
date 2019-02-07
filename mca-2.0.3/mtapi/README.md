# MTAPI

Multicore Association Task APIs

MTAPI is the Multicore Task Management API that handles task parallelism life cycle, placement, priority and scheduling.  

## Actions, Jobs, and Queues

The MTAPI runtime is intended to be an abstraction layer over operating system resources that provide execution context and scheduling. Keeping with the lightweight nature of the Multicore Association specifications, the implementation can avoid kernel mode dependencies especially by leveraging the MRAPI and MCAPI concurrency runtimes. The MTAPI contributions are bookkeeping and coordinating processing instructions that ideally run to completion without being blocked.  

Past approaches, e.g. MATLAB Simulink, have used code generation to statically order execution calls in embedded control systems. This technique works well with single processor deployments because only one action can be executed at a time. On multicore systems this becomes more of a challenge because the code needs to be partitioned into separate actions to allow parallelism.  

An action is a parameterized work request template typically implemented as an action function. This is very similar to the concept of function blocks, where the blocks can be composed into more complex arrangements. A job is an instance of an action launched with specific parameters. Job execution order is controlled by location in one or more queues, and queues can be linked to each other where completion of a job in one queue schedules a new job in another queue. Policies for the queues specify whether the entries are executed in FIFO order, in parallel, or as a group where all the jobs must complete (join) before further processing is allowed.  

## Priority Queue

Developers are familiar with setting priorities for function blocks in control application diagrams. They expect this configuration to tune the order in which function blocks are executed, eliminating race conditions and preventing intermediate output variables from being used before they are computed. With traditional code generation the order of the calls is changed if the function block priorities are adjusted.  

The MTAPI queue concept could be extended to support FIFO order for each priority level. Then, even with tasks dispatched to multiple cores, the priority could control the order that tasks are executed.  

## Variables

Control application developers think in terms of function blocks and variables. Variable values are exchanged as specified by the control diagram, e.g. data measurements are given as inputs to function blocks, function block outputs are passed to I/O control signals. If a variable is passed from one task to another, there is a natural mapping from the variable representation to IPC messages.  
