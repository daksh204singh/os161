/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Synchronization primitives.
 * The specifications of the functions are in synch.h.
 */

#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <wchan.h>
#include <thread.h>
#include <current.h>
#include <synch.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *name, unsigned initial_count)
{
	struct semaphore *sem;

	sem = kmalloc(sizeof(*sem));
	if (sem == NULL) {
		return NULL;
	}

	sem->sem_name = kstrdup(name);
	if (sem->sem_name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->sem_wchan = wchan_create(sem->sem_name);
	if (sem->sem_wchan == NULL) {
		kfree(sem->sem_name);
		kfree(sem);
		return NULL;
	}

	spinlock_init(&sem->sem_lock);
	sem->sem_count = initial_count;

	return sem;
}

void
sem_destroy(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	/* wchan_cleanup will assert if anyone's waiting on it */
	spinlock_cleanup(&sem->sem_lock);
	wchan_destroy(sem->sem_wchan);
	kfree(sem->sem_name);
	kfree(sem);
}

void
P(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	KASSERT(curthread->t_in_interrupt == false);

	/* Use the semaphore spinlock to protect the wchan as well. */
	spinlock_acquire(&sem->sem_lock);
	while (sem->sem_count == 0) {
		/*
		 *
		 * Note that we don't maintain strict FIFO ordering of
		 * threads going through the semaphore; that is, we
		 * might "get" it on the first try even if other
		 * threads are waiting. Apparently according to some
		 * textbooks semaphores must for some reason have
		 * strict ordering. Too bad. :-)
		 *
		 * Exercise: how would you implement strict FIFO
		 * ordering?
		 */
		wchan_sleep(sem->sem_wchan, &sem->sem_lock);
	}
	KASSERT(sem->sem_count > 0);
	sem->sem_count--;
	spinlock_release(&sem->sem_lock);
}

void
V(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	spinlock_acquire(&sem->sem_lock);

	sem->sem_count++;
	KASSERT(sem->sem_count > 0);
	wchan_wakeone(sem->sem_wchan, &sem->sem_lock);

	spinlock_release(&sem->sem_lock);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
	struct lock *lock;

	lock = kmalloc(sizeof(*lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->lk_name = kstrdup(name);
	if (lock->lk_name == NULL) {
		kfree(lock);
		return NULL;
	}

	HANGMAN_LOCKABLEINIT(&lock->lk_hangman, lock->lk_name);

	// add stuff here as needed
	lock->lk_wchan = wchan_create(lock->lk_name);
	if (lock->lk_wchan == NULL) {
		kfree(lock->lk_name);
		kfree(lock);
		return NULL;
	}

	spinlock_init(&lock->lk_lock);
	lock->lk_locked = false;
	lock->lk_holder = NULL;
	
	return lock;
}

void
lock_destroy(struct lock *lock)
{
	KASSERT(lock != NULL);
	KASSERT(lock->lk_locked == false);
	KASSERT(lock->lk_holder == NULL);

	// add stuff here as needed
	spinlock_cleanup(&lock->lk_lock);
	wchan_destroy(lock->lk_wchan);
	kfree(lock->lk_name);
	kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
	KASSERT(lock != NULL);
	KASSERT(curthread->t_in_interrupt == false);
	/* Call this (atomically) before waiting for a lock */
	HANGMAN_WAIT(&curthread->t_hangman, &lock->lk_hangman);

	// Write this
	spinlock_acquire(&lock->lk_lock);
	
	if (lock->lk_holder == curthread) {
			panic("Deadlock on lock %p\n", lock);
	}
	
	while (lock->lk_locked) {
		// put the thread in the wait queue of the lock
		wchan_sleep(lock->lk_wchan, &lock->lk_lock);
	}
	KASSERT(!lock->lk_locked);
	lock->lk_locked = true;
	lock->lk_holder = curthread;
	/* Call this (atomically) once the lock is acquired */
	HANGMAN_ACQUIRE(&curthread->t_hangman, &lock->lk_hangman);
	spinlock_release(&lock->lk_lock);

	// (void)lock;  // suppress warning until code gets written

}

void
lock_release(struct lock *lock)
{
	KASSERT(lock != NULL);
	KASSERT(lock_do_i_hold(lock));

	spinlock_acquire(&lock->lk_lock);	
	lock->lk_locked = false;
	lock->lk_holder = NULL;
	wchan_wakeone(lock->lk_wchan, &lock->lk_lock);
	/* Call this (atomically) when the lock is released */
	HANGMAN_RELEASE(&curthread->t_hangman, &lock->lk_hangman);
	spinlock_release(&lock->lk_lock);
	// Write this

	// (void)lock;  // suppress warning until code gets written
}

bool
lock_do_i_hold(struct lock *lock)
{
	// Write this

	// (void)lock;  // suppress warning until code gets written

	// return true; // dummy until code gets written
	return (lock->lk_holder == curthread);
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(*cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->cv_name = kstrdup(name);
	if (cv->cv_name==NULL) {
		kfree(cv);
		return NULL;
	}

	// add stuff here as needed
	cv->cv_wchan = wchan_create(cv->cv_name);
	if (cv->cv_wchan == NULL) {
		kfree(cv->cv_name);
		kfree(cv);
	}

	spinlock_init(&cv->cv_lock);

	return cv;
}

void
cv_destroy(struct cv *cv)
{
	KASSERT(cv != NULL);
	// add stuff here as needed
	spinlock_cleanup(&cv->cv_lock);
	wchan_destroy(cv->cv_wchan);
	kfree(cv->cv_name);
	kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	// Write this
	KASSERT(cv != NULL && lock != NULL);
	KASSERT(curthread->t_in_interrupt == false);	
	KASSERT(lock_do_i_hold(lock));

	spinlock_acquire(&cv->cv_lock);
	lock_release(lock);
	wchan_sleep(cv->cv_wchan, &cv->cv_lock);
	spinlock_release(&cv->cv_lock);
	
	lock_acquire(lock);
	//(void)cv;    // suppress warning until code gets written
	//(void)lock;  // suppress warning until code gets written
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	// Write this
	KASSERT(cv != NULL && lock != NULL);
	KASSERT(curthread->t_in_interrupt == false);
	KASSERT(lock_do_i_hold(lock));

	spinlock_acquire(&cv->cv_lock);
	wchan_wakeone(cv->cv_wchan, &cv->cv_lock);
	spinlock_release(&cv->cv_lock);
	// (void)cv;    // suppress warning until code gets written
	// (void)lock;  // suppress warning until code gets written
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	// Write this
	KASSERT(cv != NULL && lock != NULL);
	KASSERT(curthread->t_in_interrupt == false);
	KASSERT(lock_do_i_hold(lock));

	spinlock_acquire(&cv->cv_lock);
	wchan_wakeall(cv->cv_wchan, &cv->cv_lock);
	spinlock_release(&cv->cv_lock);
	// (void)cv;    // suppress warning until code gets written
	// (void)lock;  // suppress warning until code gets written
}

////////////////////////////////////////////////////////////
//
// RW lock

struct rwlock *
rwlock_create(const char *name)
{
	struct rwlock *rwlock = kmalloc(sizeof(*rwlock));
	if (rwlock == NULL)
		return NULL;

	rwlock->rw_name = kstrdup(name);
	if (rwlock->rw_name == NULL) {
		kfree(rwlock);
		return NULL;
	}

	rwlock->rw_lock = lock_create(rwlock->rw_name);
	if (rwlock->rw_lock == NULL) {
		kfree(rwlock->rw_name);
		kfree(rwlock);
		return NULL;
	}

	rwlock->rw_cvread = cv_create(rwlock->rw_name);
	if (rwlock->rw_cvread == NULL) {
		kfree(rwlock->rw_lock);
		kfree(rwlock->rw_name);
		kfree(rwlock);
		return NULL;
	}

	rwlock->rw_cvwrite = cv_create(rwlock->rw_name);
	if (rwlock->rw_cvwrite == NULL) {
		kfree(rwlock->rw_cvread);
		kfree(rwlock->rw_lock);
		kfree(rwlock->rw_name);
		kfree(rwlock);
		return NULL;
	}

	rwlock->rw_hold_readers = false;
	rwlock->rw_writer_in = false;
	rwlock->rw_readers_in = 0;
	rwlock->rw_writers_wt = 0;

	return rwlock;
}

void
rwlock_destroy(struct rwlock *rwlock)
{
	KASSERT(rwlock != NULL);
	KASSERT(rwlock->rw_writer_in == false && rwlock->rw_readers_in == 0);

	lock_destroy(rwlock->rw_lock);
	cv_destroy(rwlock->rw_cvread);
	cv_destroy(rwlock->rw_cvwrite);
	kfree(rwlock);
}

void
rwlock_acquire_read(struct rwlock *rwlock)
{
	KASSERT(rwlock != NULL);
	lock_acquire(rwlock->rw_lock);

	while (rwlock->rw_hold_readers == true || rwlock->rw_writer_in == true) {
		/*
		 * hold reader if their is a writer which tried to acquire the rwlock
		 * before it but was not able too, or hold it if their is a writer waiting.
		 */
		cv_wait(rwlock->rw_cvread, rwlock->rw_lock);
	}
	rwlock->rw_readers_in++;
	lock_release(rwlock->rw_lock);
}

void
rwlock_release_read(struct rwlock *rwlock)
{
	KASSERT(rwlock != NULL);
	KASSERT(rwlock->rw_readers_in > 0);
	KASSERT(rwlock->rw_writer_in == false);

	lock_acquire(rwlock->rw_lock);

	rwlock->rw_readers_in--;
	if (rwlock->rw_hold_readers == true && rwlock->rw_readers_in > 0) {
		/*
		 * When writers are waiting for the lock and 
		 * readers are still executing, let the readers finish.
		 */
	} else if (rwlock->rw_hold_readers == true  && rwlock->rw_readers_in == 0) {
		/*
		 * writers are wating, and all the readers are done executing
		 * Signal each reader and writer from their wait channel 
		 * using their conditional variables.
		 * We are doing so because each reader and writer and should get a fair 
		 * chance to execute.
		 */
		rwlock->rw_hold_readers = false;
		cv_signal(rwlock->rw_cvread, rwlock->rw_lock);
		cv_signal(rwlock->rw_cvwrite, rwlock->rw_lock);
	} else {
		/*
		 * No writers are waiting for the rwlock.
		 * Broadcast all readers.
		 */
		rwlock->rw_hold_readers = false;
		cv_broadcast(rwlock->rw_cvread, rwlock->rw_lock);
	}
		
	lock_release(rwlock->rw_lock);
}

void
rwlock_acquire_write(struct rwlock *rwlock)
{
	KASSERT(rwlock != NULL);
	lock_acquire(rwlock->rw_lock);
	
	rwlock->rw_writers_wt++;
	while (rwlock->rw_readers_in > 0 || rwlock->rw_writer_in == true) {
		/*
		 * If readers are executing or a writer is executing,  make the writer wait for
		 * the lock, until all the readers who have acquired the
		 * rwlock are done.
		 */
		rwlock->rw_hold_readers = true;
		cv_wait(rwlock->rw_cvwrite, rwlock->rw_lock);
	}
	rwlock->rw_hold_readers = false;
	rwlock->rw_writer_in = true;
	rwlock->rw_writers_wt--;
	
	lock_release(rwlock->rw_lock);
}

void
rwlock_release_write(struct rwlock *rwlock)
{
	KASSERT(rwlock != NULL);
	KASSERT(rwlock->rw_writer_in == true);
	KASSERT(rwlock->rw_readers_in == 0);
	
	lock_acquire(rwlock->rw_lock);

	rwlock->rw_writer_in = false;
	if (rwlock->rw_hold_readers == false && rwlock->rw_writers_wt == 0) { 	// Last writer seen so far
		/*
		 * If no writers are waiting, 
		 * call cv_broadcast on the cvread, to allow
		 * all readers to use the rwlock.
		 */
		cv_broadcast(rwlock->rw_cvread, rwlock->rw_lock);
	} else {
		/*
		 * Writers are waiting, and probaby readers
		 * are waiting too, though it does not matter
		 * if they are waiting or not as calling 
		 * cv_broadcast on the cv_write will not do any good,
		 * as they require exclusive access.
		 * And even cv_signal does not guarantee who is going
		 * to run so we caring for the order of execution is pointless.
		 */
		cv_signal(rwlock->rw_cvread, rwlock->rw_lock);
		cv_signal(rwlock->rw_cvwrite, rwlock->rw_lock);
	}

	lock_release(rwlock->rw_lock);
}
