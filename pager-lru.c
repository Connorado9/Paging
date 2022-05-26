// Least Recently Used Paging Algorithm
// programmed by Connor Humiston
// created by Andy Slayer & Dr. Alva Couch
// 2021/04/25

#include <stdio.h> 
#include <stdlib.h>

#include "simulator.h"

/*  Called by the simulator anytime there is a memory access, page fault, process completion (basically every CPU cycle)
    in order to determine if a page needs swapped in (and if pages need swapped out if memory full) */
void pageit(Pentry q[MAXPROCESSES]) 
{ 
    /* Static vars */
    static int initialized = 0;
    static int tick = 1; // artificial time
    static int timestamps[MAXPROCESSES][MAXPROCPAGES];      // timestamp matrix x: processes y: pages

    /* Local vars */
    int process;                                            // process counter
    int page;                                               // page counter
    int currpage;                                           // the current page
    int least;                                              // keeps track of the lowest page tick
    int LRU;                                                // stores the page with the lowest timestamp

    /* Initialize static vars on first run */
    if(!initialized)
    {
        for(process=0; process < MAXPROCESSES; process++)
        {
            for(page=0; page < MAXPROCPAGES; page++)
            {
                timestamps[process][page] = 0; 
            }
        }
        initialized = 1;
    }

    /* Page in if currently needed, page out if least recently used */
    for(process = 0; process < MAXPROCESSES; process++) 
    {
        if(!q[process].active)                              // move on if process has finished
            continue;

        currpage = q[process].pc/PAGESIZE;                  // get page the program counter is currently on
        timestamps[process][currpage] = tick;               // update the timestamp every time the pc is on a page (page being used)

        if(q[process].pages[currpage])                      // move on if page already in memory
            continue;
        else                                                // Page NOT in memory
        {
            if(pagein(process, currpage))                   // try paging in         
                continue;                                   // if pagein return 1, either already on its way to memory or already in memory, so move on
            else                                            // MEMORY FULL: if pagein returns 0, memory is full so page out LRU
            {
                least = tick;                               // set the least to the current tick (the highest)
                for(page = 0; page < q[process].npages; page++) //loop through the pages of that process
                {   
                    /* if the page is not the current, not in memory, and with a lower timestamp, it's the least recently used & should be swapped out */
                    if((page != currpage) && (q[process].pages[page] == 1) && (timestamps[process][page] < least))
                    {
                        least = timestamps[process][page];  // update the least number
                        LRU = page;                         // set the LRU page number
                    }
                }
                pageout(process, LRU);                      // page out after checking all the pages
            }

        }
    }
    tick++;                                                 // advance time for next pageit iteration
} 
