/* job.c - a job in the queue */

/* Copyright (C) 2007 Keith Rarick and Philotic Inc.

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>

#include "tube.h"
#include "job.h"
#include "primes.h"
#include "util.h"

static uint64_t next_id = 1;

static int cur_prime = 0;

static job *all_jobs = NULL;
static size_t all_jobs_cap = 12289; /* == primes[0] */
static size_t all_jobs_used = 0;

static int hash_table_was_oom = 0;

static void rehash();

static int
_get_job_hash_index(uint64_t job_id)
{
    return job_id % all_jobs_cap;
}

static void
store_job(job j)
{
    int index = 0;

    index = _get_job_hash_index(j->id);

    j->ht_next = all_jobs[index];
    all_jobs[index] = j;
    all_jobs_used++;

    /* accept a load factor of 4 */
    if (all_jobs_used > (all_jobs_cap << 2)) rehash();
}

static void
rehash()
{
    job *old = all_jobs;
    size_t old_cap = all_jobs_cap, old_used = all_jobs_used, i;

    if (cur_prime >= NUM_PRIMES) return;
    if (hash_table_was_oom) return;

    all_jobs_cap = primes[++cur_prime];
    all_jobs = calloc(all_jobs_cap, sizeof(job));
    if (!all_jobs) {
        twarnx("Failed to allocate %d new hash buckets", all_jobs_cap);
        hash_table_was_oom = 1;
        --cur_prime;
        all_jobs = old;
        all_jobs_cap = old_cap;
        all_jobs_used = old_used;
        return;
    }
    all_jobs_used = 0;

    for (i = 0; i < old_cap; i++) {
        while (old[i]) {
            job j = old[i];
            old[i] = j->ht_next;
            j->ht_next = NULL;
            store_job(j);
        }
    }
    free(old);
}

job
job_find(uint64_t job_id)
{
    job jh = NULL;
    int index = _get_job_hash_index(job_id);

    for (jh = all_jobs[index]; jh && jh->id != job_id; jh = jh->ht_next);

    return jh;
}

job
allocate_job(int body_size)
{
    job j;

    j = malloc(sizeof(struct job) + body_size);
    if (!j) return twarnx("OOM"), (job) 0;

    j->id = 0;
    j->state = JOB_STATE_INVALID;
    j->created_at = now_usec();
    j->reserve_ct = j->timeout_ct = j->release_ct = j->bury_ct = j->kick_ct = 0;
    j->body_size = body_size;
    j->next = j->prev = j; /* not in a linked list */
    j->ht_next = NULL;
    j->tube = NULL;
    j->binlog = NULL;
    j->group = NULL;
    j->heap_index = 0;
    j->reserved_binlog_space = 0;

    return j;
}

job
make_job_with_id(unsigned int pri, usec delay, usec ttr,
                 int body_size, tube tube, uint64_t id, group* grp)
{
    job j;

    j = allocate_job(body_size);
    if (!j) return twarnx("OOM"), (job) 0;

    if (id) {
        j->id = id;
        if (id >= next_id) next_id = id + 1;
    } else {
        j->id = next_id++;
    }
    j->pri = pri;
    j->delay = delay;
    j->ttr = ttr;
    j->group = grp;

    store_job(j);

    TUBE_ASSIGN(j->tube, tube);
    
    if(grp != NULL)
	    add_job_to_group(grp, j);

    return j;
}

static void
job_hash_free(job j)
{
    job *slot;

    slot = &all_jobs[_get_job_hash_index(j->id)];
    while (*slot && *slot != j) slot = &(*slot)->ht_next;
    if (*slot) {
        *slot = (*slot)->ht_next;
        --all_jobs_used;
    }
}

void
job_free(job j)
{
    if (j) {
        TUBE_ASSIGN(j->tube, NULL);
        if (j->state != JOB_STATE_COPY) job_hash_free(j);
    }

    free(j);
}

/* We can't substrct any of these values because there are too many bits */
int
job_pri_cmp(job a, job b)
{
    if (a->pri > b->pri) return 1;
    if (a->pri < b->pri) return -1;
    if (a->id > b->id) return 1;
    if (a->id < b->id) return -1;
    return 0;
}

/* We can't substrct any of these values because there are too many bits */
int
job_delay_cmp(job a, job b)
{
    if (a->deadline_at > b->deadline_at) return 1;
    if (a->deadline_at < b->deadline_at) return -1;
    if (a->id > b->id) return 1;
    if (a->id < b->id) return -1;
    return 0;
}

job
job_copy(job j)
{
    job n;

    if (!j) return NULL;

    n = malloc(sizeof(struct job) + j->body_size);
    if (!n) return twarnx("OOM"), (job) 0;

    memcpy(n, j, sizeof(struct job) + j->body_size);
    n->next = n->prev = n; /* not in a linked list */

    n->binlog = NULL; /* copies do not have refcnt on the binlog */

    n->tube = 0; /* Don't use memcpy for the tube, which we must refcount. */
    TUBE_ASSIGN(n->tube, j->tube);

    /* Mark this job as a copy so it can be appropriately freed later on */
    n->state = JOB_STATE_COPY;

    return n;
}

const char *
job_state(job j)
{
    if (j->state == JOB_STATE_READY) return "ready";
    if (j->state == JOB_STATE_RESERVED) return "reserved";
    if (j->state == JOB_STATE_BURIED) return "buried";
    if (j->state == JOB_STATE_DELAYED) return "delayed";
    return "invalid";
}

int
job_list_any_p(job head)
{
    return head->next != head || head->prev != head;
}

job
job_remove(job j)
{
    if (!j) return NULL;
    if (!job_list_any_p(j)) return NULL; /* not in a doubly-linked list */

    j->next->prev = j->prev;
    j->prev->next = j->next;

    j->prev = j->next = j;

    return j;
}

void
job_insert(job head, job j)
{
    if (job_list_any_p(j)) return; /* already in a linked list */

    j->prev = head->prev;
    j->next = head;
    head->prev->next = j;
    head->prev = j;
}

uint64_t
total_jobs()
{
    return next_id - 1;
}

/* for unit tests */
size_t
get_all_jobs_used()
{
    return all_jobs_used;
}

void
job_init()
{
    all_jobs = calloc(all_jobs_cap, sizeof(job));
    if (!all_jobs) {
        twarnx("Failed to allocate %d hash buckets", all_jobs_cap);
    }
}
