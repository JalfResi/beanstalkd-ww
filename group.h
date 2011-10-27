/* group.h - groups header */

/* Copyright (C) 2010 Wade Womersley and Stickyeyes

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

#ifndef group_h
#define group_h


#include "config.h"

#if HAVE_STDINT_H
# include <stdint.h>
#endif /* else we get int types from config.h */

#include "util.h"

typedef struct group {
    struct group *next;
    uint64_t id;
    uint64_t total;
    uint64_t finished;
    uint64_t delayed;
    uint64_t reserved;
    uint64_t buried;
    uint64_t ready;
    uint64_t *jobs;
} group;

#include "tube.h"
#include "job.h"

uint64_t create_group(uint64_t *jobs, int total);
group *find_group(uint64_t id);
int delete_group(uint64_t id);
int add_job_to_group(group *g, job j);

#endif /*group_h*/
