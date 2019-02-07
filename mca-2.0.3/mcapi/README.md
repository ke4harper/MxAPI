# MCAPI

Multicore Association Communication APIs

MCAPI is the Multicore Communications API that supports synchronization and data exchange between tasks running on different cores. It provides blocking and non-blocking operations for connection-less messages, and connection-oriented packet and scalar data exchange. The reference implementation is designed with a user-mode (MRAPI) reader-write lock guarding a shared memory database for FIFO data exchange.  

Data exchange between tasks and processes using both state- and event-based communication is a key capability for any real-time software design. One approach is to depend on the RTOS runtime for IPC. The alternative is to design and implement a lightweight concurrency runtime that makes minimal use of the operating system and avoids the associated kernel overhead.  

## Unit Tests
The MCAPI unit tests exercise the internal APIs that make up the runtime implementation. The external (according to Multicore Association specification) APIs are thin wrappers over the internal functions. Each function is called in turn with invalid and valid parameters. Using white box techniques, internal data structures are exposed to the unit tests and these are validated for expected pre- and post-conditions implemented as assertions. The unit tests form a safety net so if the implementation is revised or changed, test execution will rapidly reveal the regression.  

Having these tests dramatically increases confidence for taking risk in major refactoring experiments. For example, first the lock-based MCAPI reference implementation was built and verified using an initial version of the unit tests. Then the locks were removed incrementally as the unit tests verified continuously that there were no regressions. Finally the unit tests were updated to reflect and confirm the internal white box changes made to enhance the lock-free implementation.  

The categories of MCAPI unit tests are:
1.	Runtime initialization and rundown
2.	Requests
3.	Endpoints
4.	Queues
5.	Messages
6.	Packets
7.	Scalars

## Message Topology

![Topology](img/Example Message Topology.png)

*Example Message Topology*

The test scenarios represent different message topologies, i.e. which tasks can communicate with each other and who initiates the transactions. The MCAPI specification provides the programmatic building blocks to configure the topology but this approach quickly becomes confusing and difficult to manage. As an alternative a declarative representation was adopted. The text box in the figure above shows a partial example where endpoints compose into channels that compose into links. This terminology draws from the telecommunications domain where:  

- Link – point-to-point, broadcast, multipoint, point to multipoint communications,
- Channel – used to convey an information signal from sender to receiver, and
- Endpoint – interface exposed by a communication channel.

The message topology is static for the life cycle of the device configuration. Node assignment to a task or process is independent of topology, and can vary based on the technology stack and deployment configuration. Each task parses the XML message topology specification and interprets the contents based on its domain and node assignment.  

The MCAPI runtime is enhanced to provide three functions:  

    PUBLIC  
    extern void mcapi_node_collect(  
        MCAPI_IN mca_domain_t domain_id,  
        MCAPI_IN mcapi_node_t node_num,  
        MCAPI_IN mxml_node_t* root,  
        MCAPI_OUT mcapi_config_t* config,  
        MCAPI_IN mca_timeout_t timeout,  
        MCAPI_OUT mcapi_status_t* mcapi_status);

    PUBLIC  
    extern void mcapi_node_connect(  
        MCAPI_IN mca_domain_t domain_id,  
        MCAPI_IN mcapi_node_t node_num,  
        MCAPI_OUT mcapi_config_t* config,  
        MCAPI_IN mca_timeout_t timeout,  
        MCAPI_OUT mcapi_status_t* mcapi_status);  

    PUBLIC  
    extern void mcapi_node_disconnect(  
        MCAPI_IN mca_domain_t domain_id,  
        MCAPI_IN mcapi_node_t node_num,  
        MCAPI_OUT mcapi_config_t* config,  
        MCAPI_IN mca_timeout_t timeout,  
        MCAPI_OUT mcapi_status_t* mcapi_status);  

where <i>root</i> is the parsed DOM (Document Object Model) for the XML message topology declaration and <i>config</i> is the corresponding in-memory representation. When a task calls the <b>mcapi_node_collect</b> function it parses all the endpoints, connections and links to an internal representation. When the <b>mcapi_node_connect</b> function is called it allocates all the necessary endpoints and connections associated with that task based on the configured domain and node IDs. When a task calls the <b>mcapi_node_disconnect</b> function the related connections and endpoints are run down and the resources are released.    

The runtime can be additionally enhanced to support a task querying the configuration to discover the declared configuration and link types. Full duplex link pattern is only one of many possible, for example *P/1C, 1P/*C, *P/*C, etc. where P represents producer and C consumer. Based on the link types there can be standard operations, for example subscribe to a server task or broadcast to a set of listeners.  

## Reference Design

![MCAPIReference](img/MCAPI Reference Design.png)

*MCAPI Reference Design*

The Multicore Association’s top priority is to guide and direct the API specifications leaving products and markets to the vendors. However, they provide reference implementations of the concurrency runtimes as a starting point and to validate the specifications. The MCAPI reference design uses traditional lock-based techniques for handling concurrency and appears to mainly address issues related to multiple threads on a single processor.  

The reference design is shown in the figure above. The implementation uses global shared memory, guarded by a single kernel lock, enabling data exchange between tasks and processors on a single device. The producer endpoints (ep) insert messages into consumer ep FIFO receive queues, where each queue entry (m) is bound to a reusable message buffer. Request objects are used to track asynchronous send and receive operations, allowing a separate task to complete an operation started by the originator. Consumers can reserve queue elements that will be filled later by producers.  

## Lock-Free Requests (Version 1)

![MCAPIVersion1](img/MCAPI Lock-free Requests, Version 1.png)

*MCAPI Lock-free Requests, Version 1*

The overall goal in refactoring MCAPI is to eliminate the single kernel lock without introducing any concurrency defects or data corruption. The first area of focus was the asynchronous message bookkeeping provided by the request double linked list. Two lists are maintained: 1) empty, containing the request IDs that are available for use, and 2) full, request IDs that are active or already marking reserved locations in a message queue. The kernel lock is completely reliable in guarding these data structure, but removing the lock creates problems from simultaneous request allocations from multiple clients and keeping the list linkages consistent.  

Each element in a doubly linked list has both a forward (next) and backward (prev) reference to reduce the processing overhead needed to insert and remove elements at any location in the list. Lock-free techniques make use of CPU atomic operations but it is not possible to simultaneously update two memory locations reliably. Sundell and Tsigas<sup>[1](#Sundell2008)</sup> provide a set of algorithms that reliably update the next references and then “clean up” any errors in the prev references.

The first version of this refactoring is shown in the figure above. In anticipation that the lock-free doubly linked list approach would be impossible using spinning on Linux, the request lists were moved from global shared memory to process memory and bindings previous made from queue messages to requests were reversed. This means an asynchronous request may only be completed by a task in the same process. Functionally the lock-free algorithms worked properly as validated by the unit tests but full confidence could not be confirmed until the kernel lock was removed.  

## Lock-Free Queues

![MCAPILockFree](img/MCAPI Lock-free Queues.png)

*MCAPI Lock-free Queues*

With the request lists refactored, the next focus was on the message receive queues. The original implementation uses a circular array with external head and tail references. The kernel lock is completely reliable in guarding this data structure, but removing the lock creates problems from simultaneous message buffer allocations from multiple clients.  

The head and tail references and queue bookkeeping are replaced by the lock-free algorithms<sup>[2](#Kim2007)</sup>. The FIFO receive queue is refactored to allow concurrent access across task and process boundaries. All object (e.g. message) state changes are performed with atomic operations, and the single kernel lock is finally removed.  

## Finite State Transitions

Debugging race conditions is non-trivial in high throughput, low latency designs. The main challenge is introduction of measurements or log output causes changes to the runtime dynamics. The concurrency defect is either hidden or shifted to other locations in the code. A robust technique that resolves this dilemma is to design finite state transitions for each object type and then assert that only those transitions are allowed.  

### Request Transitions

![MCAPIRequest](img/MCAPI Request Transitions.png)

*MCAPI Request Transitions*

Requests in the original implementation are marked with several boolean flags that indicate if the request is valid, completed, or cancelled. These flags were replaced with the state transition diagram shown in the figure above. All changes to state are performed using atomic operations where the previous state is verified.  

A request in the REQUEST_FREE state is available for any client to identify a pending asynchronous operation, e.g. opening a channel, sending a message, etc. Once the request is allocated its state changes to REQUEST_VALID. For all operations other than asynchronous send, completion of the request changes the state to REQUEST_COMPLETED. For the exceptional send case, the request is marked as REQUEST_RECEIVED until the buffer can be confirmed received, and then the request state changes to REQUEST_COMPLETED. The request is then transitioned back to the available pool by changing its state to REQUEST_FREE. Cancelling a pending receive request (send requests always complete) changes the state to REQUEST_CANCELLED, and then REQUEST_FREE to make the cancelled request ID available for future operations.  

### Queue Entry Transitions

![MCAPIQueue](img/MCAPI Queue Entry Transitions.png)

*MCAPI Queue Entry Transitions*

Receive FIFO queue entries are marked in the original implementation with a boolean flag indicating if the entry is valid or not. This flag was replaced with state transition diagram shown in the figure above. All changes to state are performed using atomic operations where the previous state is verified.  

A queue entry in the BUFFER_FREE state does not have a buffer associated with it. Once a queue entry is available to receive a message it transitions to the BUFFER_RESERVED state. This guards the entry from use by other clients until a free buffer can be linked to the entry, where the state transitions to BUFFER_ALLOCATED. When the message is at the head of the receive queue it is marked as BUFFER_RECEIVED to keep other listeners on the same endpoint from trying to read its buffer. The queue entry returns to the BUFFER_FREE state when the receive operation is complete.  

### Buffer Entry Transitions

![MCAPIBuffer](img/MCAPI Buffer Entry Transitions.png)

*MCAPI Buffer Entry Transitions*

State message data exchange uses the same buffer state definitions as FIFO queue but the transitions are managed slightly different as shown in the figure above. All changes to state are performed using atomic operations where the previous state is verified.  

A buffer entry in the BUFFER_FREE state does not have a buffer associated with it. The entry transitions to the BUFFER_RESERVED state once a buffer is allocated for it. This guards the entry from use by other clients until the buffer is written, where the state transitions to BUFFER_ALLOCATED. For non-packet messages the buffer remains allocated and overwritten. Otherwise, the receiver controls the buffer entry state. For packet messages, the state transitions to BUFFER_RECEIVED when the receiver decides to read the buffer entry. If during the read the writer detects the state and replaces the buffer with a new one. On the other hand, if the read completes without a collision the state transitions back to BUFFER_RESERVED for reuse.  

### Lock-Free Requests (Version 2)

![MCAPIVersion2](img/MCAPI Lock-Free Requests, Version 2.png)

*MCAPI Lock-Free Requests, Version 2*

There were open issues that resulted from removing the single kernel lock, especially regarding event message reliability and consistency. All indications pointed to problems with the request list bookkeeping, as the scalar messaging (which does not use requests) worked successfully under stress and the other messaging did not. A new refactoring of the request lists was executed with better success.  

The revelation was that request index management did not require lists. This is reinforced by a very interesting presentation<sup>[3](#Acton2012)</sup> regarding whether a doubly linked list can be successfully implemented as a lock-free algorithm. Instead, a request is either a member of the full set or not. This can be implemented as a bit set and managed using CPU atomic operations. Selection from the bit set for available request indexes are randomized to reduce the collision between concurrent clients. The changes are shown in the figure above. 

## Stress Tests

![Stress](img/MCAPI Stress Test.png)

*MCAPI Stress Test*

The unit tests verify MCAPI functionality and state transitions in carefully controlled sequencing. The real test of the concurrency runtime is when the algorithms are exposed to high frequency requests. This stress is applied within a single process by launching multiple threads that act as clients and servers communicating without any explicit delays between requests. The communication paths and directions are defined by the message configuration topology and each operation is marked with a monotonically increasing transaction ID so it can be tracked to completion. The stress is applied across processes by running multiple copies using a message topology that links between them.  

The figure above shows the sequence diagram for a task that serves as both client as server, sending and receiving messages as fast as possible. The main processing loop iterates over the active channels for the task in a round robin fashion. The runtime calls to send and receive are based on the channel type. If the endpoint for a channel is configured to send, the next message in sequence is asynchronously sent to the receiving endpoint of the channel. If the send fails, the task yields to allow another task to run and tries to send again the next time. For messages other than scalar type, once a message is sent the request ID is polled for completion (<b>mcapi_trans_wait</b>) before sending the next message in sequence.  

If the endpoint for a channel is configured to receive, the next available message in the receive queue is accepted and processed. If the queue is empty, an entry in the receive queue is reserved and the request ID is polled for completion each time it is that endpoint’s turn to be processed. Once the message is complete the transaction ID is verified to be in sequence and then the next endpoint is processed.  

The stress tests are run both with all tasks set for CPU affinity to one core, and then again with the tasks set for CPU affinity across the available cores. The sender typically completes successfully until the receive queue is filled, and then yields. Because a receive operation only tracks one request ID at a time, the first call succeeds either to process the available message or to reserve a queue entry. Once the queue is full, the receiver continues without blocking until the queue is empty. Then the receiver yields, allowing the sender to fill the queue again.  

The tasks run until a maximum number of transactions have been sent / received. At this point the runtime statistics, such as elapsed time and possibly CPU utilization, are collected and reported. Then the message configuration is used to release the channels and endpoints and the concurrency runtime is run down.  

## State Message Policy

The MCAPI reference implementation provides a FIFO policy for message ordering. The specification also defines that messages can be delivered in FIFO order for each priority level, although this is not implemented. But a vast majority of scenarios for multi-core communication have no requirement for ordering. In other words, we only communicate the current value of a variable reliably and without corruption. For this a state message policy is needed.  

The same receive queue and buffer resources can be used to implement state messages. The details of the lock-free algorithm are described below. Queue entries are used as multiple buffers indexed by an atomic counter. Requests are not needed as the sender is never blocked and receivers retry until there is no buffer collision, which will never occur because of the large number of queue entries.  

## Unix Considerations

All of the lock-free benefits from MCAPI refactoring are based on the assumption that atomic operations are possible across processes. This has been demonstrated and validated for MRAPI, but still needs to be proven for MCAPI. The MCAPI atomic operation APIs have a reserved argument that is intended to provide the necessary Unix support for spinning, as described in more detail below. For example,  

    void mrapi_atomic_cas(
        MRAPI_OUT void* sync,
        MRAPI_IN void* dest,
        MRAPI_IN void* exchange,
        MRAPI_IN void* compare,
        MRAPI_OUT void* previous,
        MRAPI_IN size_t size,
        MRAPI_OUT mrapi_status_t* status);

The first argument is a reference to a set of shared memory application types only used for spinning on Unix:

    /* atomic operation mode */
    typedef enum {
      MRAPI_ATOMIC_WRITE = 0,
      MRAPI_ATOMIC_READ,
      MRAPI_ATOMIC_NONE
    } mrapi_atomic_mode_t;
    
    /* base message object;
       messages include this as first member of their struct */
    typedef struct {
      mrapi_boolean_t valid; /* MRAPI_FALSE when MRAPI_ATOMIC_WRITE, MRAPI_TRUE otherwise */
      unsigned txn; /* transaction ID */
      uint16_t counter; /* non-blocking buffer synchronization */
      uint16_t tindex; /* reference to mca_timestamp_t (FUTURE) */
    } mrapi_msg_t;

    /* atomic sync object */
    typedef struct {
      unsigned* pindex; /* buffer index reference */
      unsigned last_counter; /* non-blocking buffer previous state */
      mrapi_boolean_t hold; /* MRAPI_TRUE if valid msg flag should be retained
                               across atomic calls; last call must release */
      mrapi_boolean_t override; /* one-time override to allow specified mode */
      mrapi_boolean_t active; /* MRAPI_TRUE if caller holds sync */
    } mrapi_atomic_sync_t;

    /* sync descriptor for non-Windows cross-process atomic operations */
    typedef struct {
      mrapi_atomic_mode_t mode;
      mrapi_boolean_t xchg; /* read only when valid, write only when invalid;
                               flip flag state on completion */
      mca_timeout_t timeout;
      pid_t src; /* local */
      pid_t dest; /* remote; local proc == remote proc, not sync required */
      unsigned elems; /* number of buffer elements */
      size_t size; /* element size */
      mrapi_atomic_sync_t sync;
      mrapi_msg_t* buffer; /* messages */
    } mrapi_atomic_barrier_t;

These structures are managed and used internally by the following routines:

    mrapi_boolean_t sys_atomic_barrier_init(
      mrapi_atomic_barrier_t* axb,pid_t src,pid_t dest,mrapi_msg_t* buffer,
      unsigned elems,size_t size,unsigned* pindex,mca_timeout_t timeout);
    mrapi_boolean_t sys_atomic_exchange_init(
      mrapi_atomic_barrier_t* axb,pid_t src,pid_t dest,mrapi_msg_t* buffer,
      unsigned elems,size_t size,unsigned* pindex,mca_timeout_t timeout);

    void atomic_barrier_mode(void* sync,mrapi_atomic_mode_t mode)
    void try_atomic_barrier_acquire(void* sync);
    void atomic_barrier_release(void* sync);

The barrier initialization function <b>sys_atomic_barrier_init</b> supplies the source and destination process IDs, an array of multiple buffers with its extent and element size, a reference to a buffer index, and a timeout for how long the atomic operation should wait before failing. The corresponding MRAPI function is <b>mrapi_barrier_init</b>. The buffer has as the first member of each element the <b>mrapi_msg_t</b> structure. A MRAPI barrier on non-Windows platforms where the source and destination process IDs differ uses the counter in the element at the referenced index to indicate to readers and other writers of that element that a write is in progress so tasks in other processes with conflicting access spin until they can obtain ownership.  

The exchange initialization function <b>sys_atomic_exchange_init</b> has the same parameters as the atomic barrier initialization, but provides slightly different semantics. The corresponding MRAPI function is <b>mrapi_exchange_init</b>. A MRAPI exchange barrier on non-Windows platforms where the source and destination process IDs differ uses the valid flag in the element at the referenced index to indicate to readers that a write is in progress, and to a writer that a read is in progress. The writer passes ownership to the reader when it completes and the reader passes ownership to the writer when it completes. This has little applicability other then specific concurrency test scenarios.  

The <b>atomic_barrier_mode</b>, <b>try_atomic_barrier_acquire</b> and <b>atomic_barrier_release</b> operations are called internal to the concurrency runtime. The mode function sets the mode for atomic barrier, e.g. <b>MRAPI_ATOMIC_READ</b> for read access or <b>MRAPI_ATOMIC_WRITE</b> for write access. A sequence diagram for an example atomic barrier read is shown below, and the corresponding one for write afterwards. The exchange barrier sequences are similar but only used for specific concurrency tests.  

The acquire operation is a NOOP if the source and destination process IDs are the same. Otherwise the index is used to select the current element in the buffer associated with the atomic operation. The acquire operation, based on the type of barrier and mode, spins waiting for the right conditions before returning. After the atomic operation is completed the release operation, based on the type of barrier and mode, spins waiting for the right conditions before returning.  

![BarrierRead](img/Example Atomic Barrier Read.png)

*Example Atomic Barrier Read*

![BarrierWrite](img/Example Atomic Barrier Write.png)

*Example Atomic Barrier Write*

## Performance

The stress tests were used to exercise the MCAPI concurrency runtime for single one-way data exchange with five different dimensions:

1.	Windows vs. Linux deployment on the same hardware,
2.	Single CPU vs. Multicore resources,
3.	Message type: message, packet, and scalar,
4.	FIFO lock-based vs. lock-free vs. state message, and
5.	Task vs. Process shared memory.

For the multicore scenarios the tests were run with the tasks constrained to run on specific cores using CPU affinity vs. no affinity. The results are shown below. The memory bus is most probably saturated in these tests and adding more channels will degrade the performance of each channel.  

![MessagePerformance](img/Message Exchange Performance.png)

*Message Exchange Performance*




<a name="Sundell2008">1</a>: Sundell, H., Tsigas, P., "Lock-free deques and doubly linked lists", Journal of Parallel and Distributed Computing , Vol. 68, pp. 1008-1020 (2008).  
<a name="Kim2007">2</a>: Kim, et.al., "Efficient Adaptations of the Non-Blocking Buffer for Event Communication", Proceedings of ISORC, pp. 29-40 (2007).  
<a name="Acton2012">3</a>: Acton, M., "Diving Down the Concurrency Rabbit Hole", Insomniac Games (2012).
