#include <stdlib.h>
#include <stdio.h>

#include "../cut.h"
#include "../tube.h"
#include "../job.h"
#include "../util.h"
#include "../pq.h"

static tube default_tube;

void
__CUT_BRINGUP__job()
{
    job_init();
    TUBE_ASSIGN(default_tube, make_tube("default"));
}

void
__CUT__job_test_creation()
{
    job j;

    j = make_job(1, 0, 1, 0, default_tube);
    ASSERT(j->pri == 1, "priority should match");
}

void
__CUT__job_test_cmp_pris()
{
    job a, b;

    a = make_job(1, 0, 1, 0, default_tube);
    b = make_job(1 << 27, 0, 1, 0, default_tube);

    ASSERT(job_pri_cmp(a, b) < 0, "should be a < b");
}

void
__CUT__job_test_cmp_ids()
{
    job a, b;

    a = make_job(1, 0, 1, 0, default_tube);
    b = make_job(1, 0, 1, 0, default_tube);

    b->id <<= 49;
    ASSERT(job_pri_cmp(a, b) < 0, "should be a < b");
}


void
__CUT__job_test_large_pris()
{
    job a, b;

    a = make_job(1, 0, 1, 0, default_tube);
    b = make_job(-5, 0, 1, 0, default_tube);

    ASSERT(job_pri_cmp(a, b) < 0, "should be a < b");

    a = make_job(-5, 0, 1, 0, default_tube);
    b = make_job(1, 0, 1, 0, default_tube);

    ASSERT(job_pri_cmp(a, b) > 0, "should be a > b");
}

void
__CUT__job_test_hash_free()
{
    job j;
    uint64_t jid = 83;

    j = make_job_with_id(0, 0, 1, 0, default_tube, jid);
    job_free(j);

    ASSERT(!job_find(jid), "job should be missing");
}

void
__CUT__job_test_hash_free_next()
{
    job a, b;
    uint64_t aid = 97, bid = 12386;

    b = make_job_with_id(0, 0, 1, 0, default_tube, bid);
    a = make_job_with_id(0, 0, 1, 0, default_tube, aid);

    ASSERT(a->ht_next == b, "b should be chained to a");

    job_free(b);

    ASSERT(a->ht_next == NULL, "job should be missing");
}

void
__CUT__job_test_all_jobs_used()
{
    job j, x;

    j = make_job(0, 0, 1, 0, default_tube);
    ASSERT(get_all_jobs_used() == 1, "should match");

    x = allocate_job(10);
    ASSERT(get_all_jobs_used() == 1, "should match");

    job_free(x);
    ASSERT(get_all_jobs_used() == 1, "should match");

    job_free(j);
    ASSERT(get_all_jobs_used() == 0, "should match");
}

void
__CUT_TAKEDOWN__job()
{
}

