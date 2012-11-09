Author: Sean Myers
Project 3: Swap Space

readme: 
    It works for everything asked. 

    Implementation details:
        -Handle-memory (my main allocator of pages) checks to see if buddy swap returns 0 or not. If it does it envokes the clock algorithm
        -Clock will use the referenced bit to determine which page to evict.
        -Once page is determined, it calls swap_out_page.
        -Swap out page has a swap space, that is just one gigantic "free" bitmap and it looks for and finds a space in the map.
        -When it finds a place it allocates that space(sets the bit to a 1) and then puts it in.
        -Now, any time I get down to the PTE and there is a 0 present, and 1 dirty, I know that it is a swapped page.
        -I pull out the swapped page and set the bits correctly and free up the swap space used.

    Notes:
        Last time I said there was an oddity with freeing, I fixed that. Turns out I was only freeing one page even if the request was of size 2 or more.
        Doesn't handle full swap space. If you reach full swap space, it just returns -1 but my on_demand module does nothing with that info so the info is just removed completely from existence





