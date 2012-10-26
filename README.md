Author: Sean Myers
Project: Memory Allocator (Project 2)

readme: 
    It works for everything asked. 

    Implementation details:
        -Use a linked list to keep track of allocated virtual memory (malloc style) using a list_head.
        -Do a page table walk. If I can traverse to the next node, go but if not, I allocate space and put it in the table.
        -When freeing, I invalidate the memory page physical address then traverse the table. I go back up the table and if an entire table is free, I remove the table and go further up otherwise I exit.
        -When freeing, I also coallesce nodes.

    Works for the tests I tried (not many, just the base ones plus reallocating memory to see if it reuses freed memory, which it does)

    One note that may be wrong: I free a page table if it is empty, but in my test cases, none popped up with empty entries.
    You can look at this in attempt_free_physical_page(), but it looks right to me..
