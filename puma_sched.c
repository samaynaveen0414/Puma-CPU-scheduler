/* This is the only file you will be editing.
 * - puma_sched.c (Puma Scheduler Library Code)
 * - Copyright of Starter Code: Prof. Kevin Andrea, George Mason University. All Rights Reserved
 * - Copyright of Student Code: You!  
 * - Copyright of ASCII Art: Modified from an uncredited author's work:
 * -- https://www.asciiart.eu/animals/cats
 * - Restrictions on Student Code: Do not post your code on any public site (eg. Github).
 * -- Feel free to post your code on a PRIVATE Github and give interviewers access to it.
 * -- You are liable for the protection of your code from others.
 * - Date: Jan 2026
 */

/* CS367 Project 1, Spring Semester, 2026
 * Fill in your Name, GNumber, and Section Number in the following comment fields
 * Name: Samay Naveen  
 * GNumber:  G01456944
 * Section Number: CS367-006             (Replace the _ with your section number)
 */

/* puma CPU Scheduling Library
 *
 *    ("`-''-/").___..--''"`-._
 *     `6_ 6  )   `-.  (     ).`-.__.`)
 *     (_Y_.)'  ._   )  `._ `. ``-..-'
 *      _..`--'_..-_/  /--'_.'
 *     ((((.-''  ((((.'  (((.-'
 *
 */
 
/* Standard Library Includes */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
/* Unix System Includes */
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <pthread.h>
#include <sched.h>
/* DO NOT CHANGE THE FOLLOWING INCLUDES - Local Includes 
 * If you change these, it will not build on Zeus with the Makefile
 * If you change these, it will not run in the grader
 */
#include "puma_sched.h"
#include "strawhat_scheduler.h"
#include "strawhat_support.h"
#include "strawhat_process.h"
/* DO NOT CHANGE ABOVE INCLUDES - Local Includes */

/* Feel free to create any definitions or constants you like! */

/* Feel free to create any helper functions you like! */

/* strnlen() is POSIX, but some strict build environments don't expose it by default.
 * Provide a tiny local equivalent to avoid implicit-declaration warnings.
 */
static size_t puma_strnlen(const char *s, size_t max_len) {
	if(s == NULL){
		return 0;
	}
	size_t i = 0;
	for(i = 0; i < max_len; i++){
		if(s[i] == '\0'){
			break;
		}
	}
	return i;
}

/*** Puma Library API Functions to Complete ***/

/* Initializes the Puma_schedule_s Struct and all of the Puma_queue_s Structs
 * Follow the project documentation for this function.
 * Returns a pointer to the new Puma_schedule_s or NULL on any error.
 * - Hint: What does malloc return on an error?
 *
 */
Puma_schedule_s *puma_create(void) {
	Puma_schedule_s *schedule = NULL;

	schedule = malloc(sizeof(Puma_schedule_s));
	if (schedule == NULL) {
		return NULL;
	}

	/* Always initialize pointers so puma_destroy is safe if we bail out early. */
	schedule->ready_queue_high = NULL;
	schedule->ready_queue_normal = NULL;
	schedule->terminated_queue = NULL;

	schedule->ready_queue_high = malloc(sizeof(Puma_queue_s));
	if (schedule->ready_queue_high == NULL) {
		free(schedule);
		return NULL;
	}

	schedule->ready_queue_normal = malloc(sizeof(Puma_queue_s));
	if (schedule->ready_queue_normal == NULL) {
		free(schedule->ready_queue_high);
		free(schedule);
		return NULL;
	}

	schedule->terminated_queue = malloc(sizeof(Puma_queue_s));
	if (schedule->terminated_queue == NULL) {
		free(schedule->ready_queue_normal);
		free(schedule->ready_queue_high);
		free(schedule);
		return NULL;
	}

	/* Initialize each queue header. */
	schedule->ready_queue_high->count = 0;
	schedule->ready_queue_high->head = NULL;
	schedule->ready_queue_high->tail = NULL;

	schedule->ready_queue_normal->count = 0;
	schedule->ready_queue_normal->head = NULL;
	schedule->ready_queue_normal->tail = NULL;

	schedule->terminated_queue->count = 0;
	schedule->terminated_queue->head = NULL;
	schedule->terminated_queue->tail = NULL;

	return schedule;
}	
	

/* Allocate and Initialize a new Puma_process_s with the given information.
 * - Malloc and copy the command string, don't just assign it!
 * Follow the project documentation for this function.
 * - You may assume all arguments within data are Legal and Correct for this Function Only
 * Returns a pointer to the Puma_process_s on success or a NULL on any error.
 */
Puma_process_s *puma_new(Puma_create_data_s *data) {
  
	if(data == NULL){
		return NULL;
	}
	Puma_process_s *new_Process = NULL;
	new_Process = malloc(sizeof(Puma_process_s));
	if(new_Process == NULL){
		return NULL;
	}

	new_Process->pid = data->pid;

	new_Process->age = 0;

	new_Process->next = NULL;

	/* Initialize state exactly per spec:
	 * - Lower 8 bits (exit code) = 0
	 * - Ready bit set
	 * - Critical bit set iff is_critical
	 * - High bit set iff is_high OR is_critical
	 */
	new_Process->state = 0;
	new_Process->state |= (1u << 13); /* Ready */
	if(data->is_critical == 1){
		new_Process->state |= (1u << 11); /* Critical */
		new_Process->state |= (1u << 15); /* High (critical implies high) */
	}
	else if(data->is_high == 1){
		new_Process->state |= (1u << 15); /* High */
	}

	/* original_cmd is a fixed buffer; be defensive and avoid strlen() overflow. */
	size_t len = puma_strnlen(data->original_cmd, MAX_CMD);
	new_Process->cmd = malloc(len + 1);
	
	if(new_Process->cmd == NULL){
		free(new_Process);
		return NULL;
	}
	memcpy(new_Process->cmd, data->original_cmd, len);
	new_Process->cmd[len] = '\0';
	return new_Process;

}

/* Inserts a process into the appropriate Ready Queue (singly linked list).
 * Follow the project documentation for this function.
 * - Do not create a new process to insert, insert the SAME process passed in.
 * Returns a 0 on success or a -1 on any error.
 */
int puma_insert(Puma_schedule_s *schedule, Puma_process_s *process) {
  	
	if(schedule == NULL){
		return -1;
	}
	if(process == NULL){
		return -1;
	}
	if(schedule->ready_queue_high == NULL){
		return -1;
	}
	if(schedule->ready_queue_normal==NULL){
		return -1;
	}
	process->state = process->state & ~(1<<12);
	process->state = process->state & ~(1<<14);
	process->state = process->state | (1<<13);
	process->next = NULL;


	if((process->state & (1<<11)) ||( process->state & (1<<15))){
		if(schedule->ready_queue_high->head == NULL){
			schedule->ready_queue_high->head = process;
			schedule->ready_queue_high->tail = process;
			schedule->ready_queue_high->count++;
		}
		else{
			schedule->ready_queue_high->tail->next = process;
			schedule->ready_queue_high->tail = process;
			schedule->ready_queue_high->count++;
		}
	}
	else{
		if(schedule->ready_queue_normal->head == NULL){
			schedule->ready_queue_normal->head = process;
			schedule->ready_queue_normal->tail = process;
			schedule->ready_queue_normal->count++;
		}
		else{
			schedule->ready_queue_normal->tail->next = process;
			schedule->ready_queue_normal->tail = process;
			schedule->ready_queue_normal->count++;
		}
	}
	
	return 0;
}

/* Returns the number of items in a given Puma Queue (singly linked list).
 * Follow the project documentation for this function.
 * Returns the number of processes in the list or -1 on any errors.
 */
int puma_length(Puma_queue_s *queue) {
  
	if(queue==NULL){
		return -1;
	}
	return queue->count;
}

/* Selects the best process to run from the Ready Queues (singly linked list).
 * Follow the project documentation for this function.
 * Returns a pointer to the process selected or NULL if none available or on any errors.
 * - Do not create a new process to return, return a pointer to the SAME process selected.
 * - Return NULL if the ready queues were both empty OR if there were any errors.
 */
Puma_process_s *puma_select(Puma_schedule_s *schedule) {
 
	if(schedule==NULL){
		return NULL;
	}
	if(schedule->ready_queue_normal==NULL){
		return NULL;
	}
	if(schedule->ready_queue_high==NULL){
		return NULL;
	}
	if(schedule->ready_queue_high->head == NULL && schedule->ready_queue_normal->head == NULL){
		return NULL;
	}
	Puma_queue_s *q = NULL;
	if(schedule->ready_queue_high->head !=NULL){
		q = schedule->ready_queue_high;
	}
	else{
		q = schedule->ready_queue_normal;
	}
	Puma_process_s *current = q->head;
	Puma_process_s *prev = NULL;
	if(q == schedule->ready_queue_high){
	       	while(current !=NULL){
		       	if(current->state & (1<<11))
			{
			       	break;
		    	} 
			else{ 
				prev = current;
				current = current->next;
		       	}
		
		}
		if(current == NULL){
			current = q->head;
			prev = NULL;
		}
	}
	if(prev == NULL){
		q->head = current->next;
		if(q->head == NULL){
			q->tail = NULL;
		}
		q->count--;
	}
	else{
		prev->next = current->next;
		if(current == q->tail){
			q->tail = prev;
	
		}
		q->count--;
	}	
	current->next = NULL;
	current->age = 0;
	current->state = current->state & ~(1<<12);
        current->state = current->state | (1<<14);
        current->state = current->state & ~(1<<13);
	return current;
}

/* Ages all Process nodes in the Ready Queue - Normal and Promotes any that are Starving.
 * If the Ready Queue - Normal is empty, return 0.  (Success if nothing to do)
 * Follow the specification for this function.
 * Returns a 0 on success or a -1 on any error.
 */
int puma_promote(Puma_schedule_s *schedule) {
  	

	if(schedule == NULL){
		return -1;
	}
	if(schedule->ready_queue_normal == NULL){
		return -1;
	}
	if(schedule->ready_queue_high == NULL){
		return -1;
	}
	/* If there is nothing to age/promote, it's still a success. */
	if(schedule->ready_queue_normal->head == NULL){
		return 0;
	}

	Puma_process_s *prev = NULL;
	Puma_process_s *current = schedule->ready_queue_normal->head;

	while(current != NULL){
		/* Always increment age for processes that remain in Normal ready queue. */
		current->age += 1;

		if(current->age < STARVING_AGE){
			prev = current;
			current = current->next;
			continue;
		}

		/* Starving: remove from Normal queue... */
		Puma_process_s *next = current->next;
		if(prev == NULL){
			schedule->ready_queue_normal->head = next;
			if(schedule->ready_queue_normal->head == NULL){
				schedule->ready_queue_normal->tail = NULL;
			}
		}
		else{
			prev->next = next;
			if(current == schedule->ready_queue_normal->tail){
				schedule->ready_queue_normal->tail = prev;
			}
		}
		schedule->ready_queue_normal->count--;

		/* ...and append to High queue.
		 * Spec: do NOT change age, flags, or state. Only change next pointers.
		 */
		current->next = NULL;
		if(schedule->ready_queue_high->head == NULL){
			schedule->ready_queue_high->head = current;
			schedule->ready_queue_high->tail = current;
		}
		else{
			schedule->ready_queue_high->tail->next = current;
			schedule->ready_queue_high->tail = current;
		}
		schedule->ready_queue_high->count++;

		current = next;
		/* prev stays the same because we removed 'current' from the Normal list. */
	}

	return 0;
}

/* This is called when a process exits normally that was just Running.
 * Put the given node into the Terminated Queue and set the Exit Code 
 * - Do not create a new process to insert, insert the SAME process passed in.
 * Follow the project documentation for this function.
 * Returns a 0 on success or a -1 on any error.
 */
int puma_exited(Puma_schedule_s *schedule, Puma_process_s *process, int exit_code) {
	if(schedule == NULL){
		return -1;
	}
	if(process == NULL){
		return -1;
	}
	if(schedule->terminated_queue == NULL){
		return -1;
	}
	process->next = NULL;
	if(schedule->terminated_queue->head == NULL){
                        schedule->terminated_queue->head = process;
                        schedule->terminated_queue->tail = process;
                        schedule->terminated_queue->count++;
                }
                else{
                        schedule->terminated_queue->tail->next = process;
                        schedule->terminated_queue->tail = process;
                        schedule->terminated_queue->count++;
		}
	 process->state &= ~0xFF;
	 process->state |= (exit_code & 0xFF);
	 process->state = process->state |(1<<12);
         process->state = process->state & ~(1<<14);
         process->state = process->state & ~(1<<13);





	return 0;
}

/* This is called when the OS terminates a process early. 
 * - This will either be in your Ready Queue - High or Ready Queue - Normal.
 * - The difference with puma_exited is that this process is in one of your Queues already.
 * Remove the process with matching pid from either Ready Queue and add the Exit Code to it.
 * - You have to check both since it could be in either queue.
 * Follow the project documentation for this function.
 * Returns a 0 on success or a -1 on any error.
 */
int puma_terminated(Puma_schedule_s *schedule, pid_t pid, int exit_code) {


        if(schedule==NULL){
                return -1;
        }
        if(schedule->ready_queue_high==NULL){
                return -1;
        }
        if(schedule->ready_queue_normal==NULL){
                return -1;
        }
        if(schedule->terminated_queue==NULL){
                return -1;
        }
        Puma_queue_s *q = schedule->ready_queue_high;
        Puma_process_s *prev = NULL;
        Puma_process_s *current = q->head;
        while(current!=NULL){
                if(current->pid ==pid){
                        break;
                }
                else{
                        prev = current;
                        current = current->next;
                }

        }
        if(current ==NULL){
                q = schedule->ready_queue_normal;
                prev = NULL;
                current = q->head;
                while(current !=NULL){
                        if(current->pid == pid){
                                break;
                        }
                        else{
                                prev = current;
                                current = current->next;
                        }
                }
        }

        if(current ==NULL){
                return -1;
	}
        if(prev ==NULL){
                q->head = current->next;  if(q->head == NULL){
                        q->tail = NULL;
                }
                q->count--;
        }
        else{
                prev->next = current->next;
                if(current == q->tail){
                        q->tail = prev;
                }
                q->count--;
       
        }
	current->next = NULL;
	
	current->state &= ~0xFF;
        current->state |= (exit_code & 0xFF);
         current->state = current->state |(1<<12);
         current->state = current->state & ~(1<<14);
         current->state = current->state & ~(1<<13);
         if(schedule->terminated_queue->head==NULL){
                 schedule->terminated_queue->head = current;
                 schedule->terminated_queue->tail = current;
                 schedule->terminated_queue->count++;
         }
         else{
                 schedule->terminated_queue->tail->next = current;
                 schedule->terminated_queue->tail = current;
                 schedule->terminated_queue->count++;
         }
        return 0;
}

/* This is called when StrawHat reaps a Terminated (Defunct) Process.  (reap command).
 * Remove and free the process with matching pid from the Termainated Queue.
 * Follow the specification for this function.
 * Returns the exit_code on success or a -1 on any error (such as process not found).
 */
int puma_reap(Puma_schedule_s *schedule, pid_t pid) {

	if(schedule ==NULL){
		return -1;
	}
	if(schedule->terminated_queue==NULL){
		return -1;
	}
	if(schedule->terminated_queue->head == NULL){
		return -1;
	}
	Puma_queue_s *q = schedule->terminated_queue;
	Puma_process_s *current = q->head;
	Puma_process_s *prev = NULL;

	/* If pid is 0, reap the head process. */
	if(pid == 0){
		current = q->head;
		prev = NULL;
	}
	else{
		while(current != NULL){
			if(current->pid == pid){
				break;
			}
			prev = current;
			current = current->next;
		}
		if(current == NULL){
			return -1;
		}
	}

		int exit_code = current->state & 0xFF;
		if(prev ==NULL){
			q->head = current->next;
			if(q->head==NULL){
				q->tail = NULL;
			}

		}
		else{
			prev->next = current->next;
			if(current == q->tail){
				q->tail =prev;
			}
		}
		q->count--;
		current->next = NULL;
		if(current->cmd!=NULL){

		free(current->cmd);
		}
		free(current);


	return exit_code;
}

/* Gets the exit code from a terminated process.
 * (All Linux exit codes are between 0 and 255)
 * Follow the project documentation for this function.
 * Returns the exit code if the process is terminated.
 * If the process is not terminated, return -1.
 */
int puma_get_exitcode(Puma_process_s *process) {
  
	if(process==NULL){
		return -1;
	}
	if((process->state & (1<<12)) == 0){
		return -1;
	}
	int exit_code = process->state & 0xFF;

	
	return exit_code; 
}

/* Frees all allocated memory in the Puma_schedule_s, all of the Queues, and all of their Nodes.
 * Follow the project documentation for this function.
 * Returns void.
 */
void puma_destroy(Puma_schedule_s *schedule) {

        if(schedule == NULL){
                return;
        }

        /* Destroy normal queue */
        if(schedule->ready_queue_normal != NULL){

                Puma_process_s *current = schedule->ready_queue_normal->head;

                while(current != NULL){

                        Puma_process_s *next = current->next;

                        if(current->cmd != NULL){
                                free(current->cmd);
                        }

                        free(current);

                        current = next;
                }

                free(schedule->ready_queue_normal);
        }

        /* Destroy high queue */
        if(schedule->ready_queue_high != NULL){

                Puma_process_s *current = schedule->ready_queue_high->head;

                while(current != NULL){

                        Puma_process_s *next = current->next;

                        if(current->cmd != NULL){
                                free(current->cmd);
                        }

                        free(current);

                        current = next;
                }

                free(schedule->ready_queue_high);
        }

        /* Destroy terminated queue */
        if(schedule->terminated_queue != NULL){

                Puma_process_s *current = schedule->terminated_queue->head;

                while(current != NULL){

                        Puma_process_s *next = current->next;

                        if(current->cmd != NULL){
                                free(current->cmd);
                        }

                        free(current);

                        current = next;
                }

                free(schedule->terminated_queue);
        }

        /* Free schedule */
        free(schedule);

        return;
}

