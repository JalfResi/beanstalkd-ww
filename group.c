/* group.c - a job group */

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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "group.h"

uint64_t current_id = 0;

group *first = NULL;

uint64_t create_group(uint64_t *jobs, int total)
{
	current_id += 1;
	
    group *g = malloc(sizeof(group) + sizeof(uint64_t) * total);
    
    g->id = current_id;
    g->jobs = jobs;
    g->total = total;
    g->finished = 0;
    g->reserved = 0;
    g->delayed = 0;
    g->ready = 0;
    g->buried = 0;
    g->next = NULL;
    
    int i;
	job j = NULL;
    for(i = 0; i < g->total; i++)
    {
	    j = job_find(g->jobs[i]);
	    
	    if(j)
	    	j->group = g;
    }
    
    if(first == NULL)
    {
		first = g;   
    }
    else
    {
	    group *head = first;
	    
	    while(head->next != NULL)
	    {
		    head = head->next;
	    }
	    head->next = g;
	}
    
    return current_id;
}

group *find_group(uint64_t id)
{	
	if(first == NULL)
		return NULL;
		
	group *head = first;
	
	if(head->id == id){
		return head;
	}
	
    while(head->next != NULL)
    {
		head = head->next;
		
		if(head->id == id)
			return head;
    }
    
	
	
	return NULL;
}

void remove_job_references(group *g)
{	
	int i;
	job j;
			
	for(i = 0; i < g->total; i++)
	{
		j = job_find(g->jobs[i]);
		if(!j) continue;
		
		j->group = NULL;
	}
}

int delete_group(uint64_t id)
{
	if(first == NULL)
		return 0;
		
	if(first->id == id)
	{
		group *tmp = first;
		first = first->next;
		remove_job_references(tmp);
		free(tmp);
		
		return 1;	
	}
	
	group *head = first;
	
	while(head->next != NULL)
	{
		if(head->next->id == id)
		{
			group *tmp = head->next;
			head->next = head->next->next;
			
			remove_job_references(tmp);
			
			free(tmp);
			
			return 1;
		}	
		
		head = head->next;
	}
	
	return 0;
}
	

int add_job_to_group(group *g, job j)
{
	int found = 0;
	int i;
	
	for(i = 0; i < g->total; i++)
	{
		if(g->jobs[i] == j->id)
		{
			found = 1;
			break;
		}	
	}
	
	if(found == 0)
	{
		g->total ++;
		uint64_t *ret = realloc(g->jobs, g->total * sizeof(uint64_t));
		
		g->jobs = ret;
		g->jobs[g->total - 1] = j->id;
	}
	
	return found == 0 ? 1 : 0;	
}