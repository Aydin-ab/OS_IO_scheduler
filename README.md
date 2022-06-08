# OS scheduler
Homework for OS course at NYU. Basic virtual memory manager to replicate OS behavior in C++

**Grade : 100/100 A**

## HOW TO USE
Compile the code with the ```make``` command
Execute the program with ```./iosched [ –s<schedalgo> | -v | -q | -f ] <inputfile>```.  
The schedulers implemented are FIFO (i), SSTF (j), LOOK (s), CLOOK (c), and FLOOK (f) (the letters in bracket define which parameter must be given in the –s program flag shown above).  

The output goes to the standard output.
Given a list of input files and a random file, you can use the ```runit.sh``` script to run the program on each of them and put the outputs in a output directory.


## CONTEXT
I implement and simulate the scheduling and optimization of I/O operations. 

Applications submit their IO requests to the IO subsystem (potentially via the filesystem), where they are maintained in an IO-queue until the disk device is ready for servicing another request.  
The IO-scheduler then selects a request from the IO-queue and submits it to the disk device. This selection is commonly known as the strategy() routine in operating systems. On completion, another request can be taken from the IO-queue and submitted to the disk. The scheduling policies will allow for some optimization as to reduce disk head movement or overall wait time in the system. I only provide the I/O Scheduler.


The input file is structured as follows:  
Lines starting with ‘#’ are comment lines and should be ignored.  
Any other line describes an IO operation where the 1st integer is the time step at which the IO operation is issued and the 2nd integer is the track that is accesses. Since IO operation latencies are largely dictated by seek delay (i.e. moving the head to the correct track), we ignore rotational and transfer delays for simplicity. The inputs are well formed.