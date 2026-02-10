Puma Scheduler
Language: C
Focus: Systems programming, memory management, scheduling algorithms
This project implements a priority-based CPU scheduling module in C, designed to operate within a larger operating-system–style framework.
The scheduler manages multiple ready queues (high and normal priority), selects processes for execution, and handles process lifecycle transitions including running, termination, and reaping. To prevent starvation, the scheduler implements an aging mechanism that automatically promotes long-waiting processes to a higher-priority queue.
Key Features
Priority-based scheduling with High and Normal ready queues
Starvation prevention via process aging and promotion
Explicit linked-list queue management with head/tail tracking
Bitwise state encoding for efficient process state transitions
Robust dynamic memory management with full cleanup of process resources
Graceful handling of edge cases (empty queues, partial allocation failures)
Technical Highlights
Manual memory ownership tracking (malloc / free)
Singly linked-list manipulation under concurrent state changes
Precise bit masking for process state and exit-code storage
Defensive programming against invalid inputs and failure paths
Note: This repository contains only the scheduling module authored by me. The surrounding framework used during development was instructor-provided and is not included
