# MINIX 3 Custom Scheduler

This project focuses on modifying the default scheduler of the *MINIX 3* operating system to implement multiple *CPU scheduling algorithms*. The goal is to study the behavior and performance of different strategies in a real OS environment.

## Project Title
*Custom Scheduling for MINIX 3*

## Team Members
- *Saksham Latwal* (Team Lead) – Priority Scheduling, EDF  
- *Prasanna Pant* – LRTF  
- *Yogesh Chauhan* – SRTF  
- *Salil Tiwari* – FCFS  

## Scheduling Algorithms Implemented
- *FCFS (First Come First Serve)*  
- *SRTF (Shortest Remaining Time First)*  
- *LRTF (Longest Remaining Time First)*  
- *Priority-Based Scheduling*  
- *EDF (Earliest Deadline First)*

## File Modifications
Changes were made primarily in the following files within the MINIX 3 source:
- scheduler.c
- proc.c
- proc.h (for additional process metadata)

## Features
- Multiple CPU scheduling strategies
- Modified kernel-level logic in MINIX 3
- Uses MINIX's native message-passing system
- Measurable performance parameters (waiting time, turnaround time)
