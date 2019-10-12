#ASS1

## Spinlock
Spinlock is a lock that makes a thread acquiring  wait in a loop ("busy waiting") until the lock becomes free. The spinlock is used to prevent access to small critical sections to avoid the overhead related to other locks that makes the thread go to sleep and switching threads.

In OS/161 implementation, the spinlock uses test-test-and-set mechanism (atomic operation) to acquire the lock. It first checks the value of the spinlock, if the spinlock is available it proceeds to perform test-and-set operation.
The test-and-set operation is implemented using load and store conditional atomic operation. 
A test is performed before test-and-set operation to avoid bus contention in a cache coherent system(invalidation based). A cache invalidation is done using a cache coherence protocol. In such protocol, a processor changes a memory location and then invalidates the cached values of that meory location across the rest of the computer system. 

Spinning only makes sense in multiprocessor architectures. In single processor architectures, atomicity can be guaranteed by simply disabling the interrupts on the processor. In multi-processor, disabling interrupts by other processors is impractical and therefore have to use another approach to achieve atomicity on multi-processor environment.

**Spinlocks are associated with the cpu.**

### Load linked and store conditional
Load linked and store conditional is an atomic operation provided by the hardware to perform a read and write atomically. If an atomic read and write has to be performed, the pipeline has to add two memory stages to it. These two stages can slow down the pipeline. Instead, the atomic read write is broken down into two operations ll and sc.
* ll (load linked)
  * Like a normal lw operation
  * Saves the address from which it loaded into a special link register.
* sc (Store conditional)
  * Checks if the address same as in link register. Yes? Normal SW, Return 1 in the register.
  * No? Return 0 in the register without storing anything.
These operations are behaving as a single atomic operation due to the link register.

### LL/SC atomicity
```assembly
ll R1,lockvar
```
Snoops on cache invalidates for that cache line, store fails if an invalidate has been seen. 
```assembly
sc R2,lockvar
```
the store conditional will check if the address is same as in the link register. If it is 0, the operation will fail and return 0 in the register without storing anything. 
The LL/SC uses coherence by checking the link register. 

### Other ways to achieve atomicity (Hardware instructions)
* TAS (test and set) operation: write 1 regardless of previous value, return old value.
* Fetch and Add: Add to memory location, read previous value.
* Compare and Swap: Load location into value, if value == old, store new to location, return value.
* exchange (x86):  swaps value between register and memory

## Sleep Locks
Thread acquring the lock sleeps until the lock is free and is put back in the run queue when lock becomes free. Spinlocks makes the thread spin until the lock is free causing busy-waiting, while sleep locks makes the thread go to sleep and a context switch occurs. Sleep locks are synchronization primitive, and therefore all operations on the sleep locks must be atomic.

### Sleep lock structure:- 
```c
struct lock {
        char *lk_name; 					// for debugging purposes 
        HANGMAN_LOCKABLE(lk_hangman);   /* Deadlock detector hook. */
        // add what you need here
        // (don't forget to mark things volatile as needed)
		struct wchan *lk_wchan; 		// wait channel for threads sleeping on the lock.
		struct spinlock lk_lock;		// spinlock associated with the lock. 
		volatile bool lk_locked;
		struct thread *lk_holder;
};
```
### Functions of sleeplocks

```c

/*
 * lock_create() - When lock is created, no thread should be holding it.
 * lock_destroy() - When lock is destroyed, no thread should be holding it.
 */
struct lock *lock_create(const char *name);
void lock_destroy(struct lock *);

/*
 * Operations:
 *    lock_acquire - Get the lock. Only one thread can hold the lock at the
 *                   same time.
 *    lock_release - Free the lock. Only the thread holding the lock may do
 *                   this.
 *    lock_do_i_hold - Return true if the current thread holds the lock;
 *                   false otherwise.
 *
 * These operations must be atomic. You get to write them.
 */
void lock_acquire(struct lock *);
void lock_release(struct lock *);
bool lock_do_i_hold(struct lock *);

```
### Source files
**Function implementations are in kern/thread/synch.c**
**Header file associated with it is kern/include/synch.h**

### Test suite for sleep locks
**lt1, lt2, lt3, lt4, lt5 are tests that are in the test suite for sleep locks.**
* lt1: lt1 spawns spawns NTHREADS(defined in kern/test/synchtest.c) and checks whether the lock ensures mutual exclusion. Returns 0 on success, panics on failure 
* lt2: tests whether the lock\_release() panics the kernel when called by a thread that does not hold the lock. panics on success, returns 0 on failure.
* lt3: tests what happens when lock\_destroy() is called by a thread without releasing the lock. panics on success, returns 0 on failure.
* lt4: tests what happens when lock\_release() on the lock by a thread when it is held by some other thread at the same time. panics on success, returns 0 on failure. 
* lt5: tests the functionality of lock\_do\_i\_hold(). panics on success, returns 0 on failure. 

**Tests for sleep locks are in kern/test/synchtest.c**

## Condition variable(CV)
Condition variables are synchronization primitives that makes a thread wait until a condition is met. Wait makes the thread release the sleep lock, goes to sleep, when it is signaled by other thread, wakes up, reacquires the sleep lock. 

A condition variable indicates an event and has no value. One cannot store, or retrieve value from a condition variable. If a thread must wait for an event occur, it waits on the condition variable. The condition has a queue (cv\_whan) for those threads that are waiting for the corresponding event to occur. 

### Structure of CV
```c

struct cv {
        char *cv_name; 				// used for debugging purposes
        // add what you need here
        // (don't forget to mark things volatile as needed)
		struct wchan *cv_wchan;		// wait channel, contains threads waiting on the CV
		struct spinlock cv_lock;
};

```
### Functions of CV

```c

/*
 * cv_create - creates condition variable
 * cv_destroy - destroys condition variable.
 */
struct cv *cv_create(const char *name);
void cv_destroy(struct cv *);

/*
 * Operations:
 *    cv_wait      - Release the supplied lock, go to sleep, and, after
 *                   waking up again, re-acquire the lock.
 *    cv_signal    - Wake up one thread that's sleeping on this CV.
 *    cv_broadcast - Wake up all threads sleeping on this CV.
 *
 * For all three operations, the current thread must hold the lock passed
 * in. Note that under normal circumstances the same lock should be used
 * on all operations with any particular CV.
 *
 * These operations are atomic and implemented by acquring cv_lock(spinlock). 
 */

void cv_wait(struct cv *cv, struct lock *lock);
void cv_signal(struct cv *cv, struct lock *lock);
void cv_broadcast(struct cv *cv, struct lock *lock);

```
### Condition variable semantics
**Condition variables have two variety of semantics, hoare (blocking) and mesa (or non-blocking)**
* Hoare semantics: the thread that calls cv\_signal wil block until the signaled thread(if any) runs and releases the lock.
* Mesa semantics: the thread that calls cv\_signal will awaken one thread waiting on the condition variable but will not block.

**Implemented mesa semantics**
**Since cv is implemented using mesa semantics, no guarantees have been made about scheduling**

### Source files
**Functions are defined in kern/thread/synch.c**
**Function declaration and CV structure is in kern/include/synch.h**

### Test suite for condition variables (CV)
**cvt1, cvt2, cvt3, cvt4, cvt5 are tests that are in the test suite for condition variables.**
* cvt1: creates CREATELOOPS condition variables, and destroys except the last one. Uses the last condition variable to test the functions of cvt1. It creates NTHREADS using threadfork and waits using donesem semaphore, the thread calls V when it is done, and the driver calls P. In the newly created threads, every thread goes to sleep except the last thread, which then calls the cv\_broadcast on the cv. Now the real test begins, the test now checks whether the cv\_broadcast works properly(not releasing the lock, as it follows mesa semantics), and the other threads woken up race for the lock, checks testval1 with testval2. The test returns 0 on success, panics the kernel on failure.
* cvt2: cvt2 checks whether going to sleep is really atomic. It creates NCVS lock/CV pairs, with one thread sleeping and the other waking it up. If we miss a wakeup, the sleep thread won't go around enough times and the failif(condition) will fail and cause the kernel panic. Returns 0 on success, panics the kernel on failure.      
* cvt3: Tests the cv\_signal() function. Tries to wait on the cv without acquiring the lock. cv\_wait according to semantics require the lock to be acquired as it releases in its implementaion and goes to sleep. panics on sucess and returns 0 on failure.
* cvt4: tests cv\_broadcast function semantics. Calls cv\_broadcast() on the cv without acquiring the lock. If it follows the cv follows cv semantics, the kernel should panic. panics on success, returns 0 on failure.   
* cvt5: Focuses on inter-thread communication between the driver thread, sleeperthread and wakerthread. returns 0 on success, panics on failure. 
**Tests for conditional variables are in kern/test/synchtest.c**
