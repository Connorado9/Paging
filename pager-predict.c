// Least Recently Used Paging Algorithm
// programmed by Connor Humiston
// created by Andy Slayer & Dr. Alva Couch
// Algorithm Idea from Derek Gorthy (dsgorthy on git)
// References: Jack Dinkel, Luke Meszar, Izaak Weise, Alex Curtis
// 2021-04-25
// Goal: Create a paging algorithm that predicts the page transitions of 5 random programs in order to pre-emptively 
//  page in those pages in time for use and avoid blocking programs while pages are fetched from disk


#include <stdio.h>                                                      // standard functions
#include <stdlib.h>

#include "simulator.h"                                                  // simulator & its constants

/*  Called by the simulator anytime there is a memory access, page fault, process completion (basically every CPU cycle)
    in order to determine if a page needs swapped in (and if pages need swapped out if memory full) */
void pageit(Pentry q[MAXPROCESSES]) 
{     
    /* Local vars */
    int process;                                                        // process counter
    int page;                                                           // page counter
    int currpage;                                                       // what page the current program counter is on
    int i;                                                              // secondary counter

    /* Page Transition Table */                                         // from CSV Page Tracker (time tick, process, page, pid, kind, comment (in/out))
    // the most likely transiton is to the next page or the very first
    // 0 = not likely, 5 = most likely
    static int prediction[MAXPROCESSES][MAXPROCPAGES] = {               // Page #
        {0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   // 0  -> 1    
        {0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   // 1  -> 2
        {0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   // 2  -> 3
        {2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0},   // 3  -> 4,0,10
        {0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   // 4  -> 5
        {0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   // 5  -> 6
        {0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   // 6  -> 7
        {0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   // 7  -> 8
        {3, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   // 8  -> 9,0
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0},   // 9  -> 10
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0},   // 10 -> 11
        {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0},   // 11 -> 0,12
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0},   // 12 -> 13
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},   // 13 -> 0,9,14
        {5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0},   // 14 -> 0
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0},   // 15 -> 16
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0},   // 16 -> 17
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0},   // 17 -> 18
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5},   // 18 -> 19
        {5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}    // 19 -> 0
      // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19 - Most likely next page
    };

    for(process = 0; process < MAXPROCESSES; process++)                 // select a process
    {
        int transition[MAXPROCPAGES];                                   // transition array tracking which pages should stay in or be evicted
        for(i = 0; i < MAXPROCPAGES; i++)                               // 1 = keep in memory, 0 = evict
            transition[i] = 0;                                          // initialize to 0

        currpage = q[process].pc / PAGESIZE;                            // the current page is the program counter divided by the page size
        transition[currpage] = 1;                                       // since we need the current page, set it's bit to 1 in case it's not in memory

        /* Update the transition array to include most likely pages & their most likely jumps */
        for(page = 0; page < MAXPROCPAGES; page++)                      // loop through the pages
        {
            if(prediction[currpage][page] > 0)                          // find the most likely jumps for current page
            {
                transition[page] = 1;                                   // add the most likely transitions to the transition list
            }
            if(prediction[currpage][page] > 3)                          // Prediction 2x into the future:
            {                                                           // now, find the most likely jumps from the next page with high probability
                for(i = 0; i < MAXPROCPAGES; i++)                       // loop through the pages one more time
                {
                    if(prediction[page][i] > 0)                         // find the next page's probable jumps
                    { 
                        transition[i] = 1;                              // add the most likely transitions for the next page's jumps to the list
                    }
                }
            }

        }

        /* Page out the uneeded pages */
        for(i = 0; i < MAXPROCPAGES; i++)
        {
            if(transition[i] == 0)                                      // pages with no probability 
            {
                pageout(process,i);                                     // pageout only fails if something is being swapped into memory
            }
        }
        
        /* Page in the most likely pages */
        for(i = 0; i < MAXPROCPAGES; i++)
        {
            if(transition[i] == 1)                                      // pages with a likelihood of being jumped to next
            {
                if(pagein(process, i))                                  // try paging in
                    continue;                                           // if page in returns 1, either already on its way to memory or already in memory, so move on to next page
            }
        }
    }
} 

//from scratch
    // static int initialized = 0;
    // static int tick = 1;                                    // artificial time
    // static int timestamps[MAXPROCESSES][MAXPROCPAGES];      // EVICTIONS: timestamp matrix x: processes y: pages
    // static int popularity[MAXPROCESSES][MAXPROCPAGES];      // the popularity matrix keeps track of the most popular pages, so those should be more likely to stay in memory
    // static int probability[MAXPROCESSES][MAXPROCPAGES][MAXPROCPAGES]; // PREDICTIONS: an entry in the probability matrix at process x, page y, and next page z
    //                                                         //  says what the most likely page that page x will jumpt to next is the higest number among page # z
    // static int lastpage[MAXPROCPAGES];                      // stores the page used in the last pageit call to be used as a probability/transition for each process

    // int process;                                            // process counter
    // int page;                                               // page counter
    // int prob;                                               // probability counter
    // int currpage;                                           // the current page
    // int least;                                              // keeps track of the lowest page tick
    // int most;                                               // keeps track of the highest probability
    // int prediction;                                         // stores the prediction for what will come next
    // int leastprobable;                                      // stores the least probable page to be used next
    // int LRU;                                                // stores the page with the lowest timestamp
    // int sum;                                                // keeps track of how many pages are in memory

    // /* Initialize static vars on first run */
    // if(!initialized)                                        // only want this initialization to 0 to occur the very first time 
    // {
    //     for(process=0; process < MAXPROCESSES; process++)
    //     {
    //         for(page=0; page < MAXPROCPAGES; page++)
    //         {
    //             timestamps[process][page] = 0;              // initialize 
    //             popularity[process][page] = 0; 
    //             for(prob = 0; prob < MAXPROCPAGES; prob++)
    //             {
    //                 probability[process][page][prob] = 0; 
    //             }
    //         }
    //     }
    //     initialized = 1;
    // }

    // /* Page in if currently needed, page out if least recently used */
    // // there are 20 processes competing for 100 page spots, meaning each process can have 4 spots total until some exit, in which case you should increase the rest by 4
    // // so, ensure that each process only has 4 in memory at any time
    //     // meaning q[process].pages[0->20] should add up to 4 or have at most 4
    //     // if equal to or more than 4, you should evict ONE
    // // also im not sure, but if the memory is full, i think you should evict pages for more probable ones too
    // // must keep 3/4 memory spots clear to bring in the new page everytime

    // for(process = 0; process < MAXPROCESSES; process++)     // 1. Select a Process
    // {
    //     if(!q[process].active)                              // ensure the process is active
    //         continue;                                       // move on if process has finished

    //     currpage = q[process].pc/PAGESIZE;                  // get page the program counter is currently on
    //     timestamps[process][currpage] = tick;               // update the timestamp every time the pc is on a page (page being used)
    //     popularity[process][currpage]++;
        
    //     probability[process][lastpage[process]][currpage]++;// incremement the probability that the last page jumps to the current page
    //     lastpage[process] = currpage;                       // update the new last page for the next iteration

    //     // Current Page                                     // 2. Is page swapped in? Check for previous prediction misses
    //     if(q[process].pages[currpage])                      // move on to next process if page already in memory
    //         continue;                                       
    //     else                                                // Page NOT in memory
    //     {                                                   // 3. If not in memory, call pagein()
    //         if(pagein(process, currpage))                   // try paging in         
    //             continue;                                   // if pagein return 1, either already on its way to memory or already in memory, so move on
    //         else                                            // 4. MEMORY FULL: if pagein returns 0, memory is full so evict LRU
    //         {
    //             least = tick;                               // set the least to the current tick (the highest)
    //             for(page = 0; page < q[process].npages; page++) //loop through the pages of that process
    //             {   
    //                 /* if the page is not the current, in memory, and with a lower timestamp, it's the least recently used & should be swapped out */
    //                 if((page != currpage) && (q[process].pages[page] == 1) && (timestamps[process][page] < least) && (page != prediction))
    //                 {
    //                     least = timestamps[process][page];  // update the least number
    //                     LRU = page;                         // set the LRU page number
    //                 }
    //             }
    //             pageout(process, LRU);                      // page out after checking all the pages
    //         }
    //     }

    //     // Future Page                                      // 5. Determine future page 
    //     most = 0;
    //     least = INT_MAX;
    //     sum = 0;
    //     for(prob = 0; prob < MAXPROCPAGES; prob++)          // loop through the probabilities
    //     {
    //         if(probability[process][currpage][prob] > most) // if that page is more probable than the current most probable page transition
    //         {
    //             most = probability[process][currpage][prob];// update the most probable
    //             prediction = prob;                          // save the new prediction
    //         }
    //         else if((prob != currpage) && (q[process].pages[prob] == 1) && probability[process][currpage][prob] < least) //if that page is less probable than the current least probable transition
    //         {
    //             least = probability[process][currpage][prob]; //update the new least probable page transition
    //             leastprobable = prob;
    //         }
    //     }
    //     if(most == 0)                                       // if there is prediction data yet, move on to next process
    //         continue; 
        
    //     // Page in the Prediction 
    //     if(q[process].pages[currpage])                      // 6. Is page swapped in?
    //     {
    //         continue;                                       // already swapped in: continue to the next process
    //     }
    //     else                                                
    //     {
    //         if(pagein(process, prediction))                 // 7. if not swapped in, call pagein()
    //             continue;                                   // move to next process if already started, already running, or paged out
    //         else                                            // 8. MEMORY FULL: if pagein returns 0, memory is full so evict something
    //         {
    //             /* Evict the page that is least likely to be called */
    //             pageout(process, leastprobable);             // 9. Call pageout() and evict least probable page
    //             // combine with LRU?
    //         }
    //     }
        // if(most != 0)                                       // if there is an actual prediction setup
        //     pagein(process, prediction);                    // now page in the future prediction
        // printf("p# %d, cp# %d: ", process, currpage);
        // for(prob = 0; prob < MAXPROCPAGES; prob++)
        //     printf("|%d,%d| ", prob, probability[process][currpage][prob]);
        // printf("\n");
        // printf("most: %d\n", most);
        // if(most != 0)
        //     printf("paging in: %d\n", prediction);
    // }
    // tick++;
