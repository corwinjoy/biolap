// Licensed under Common Public License 1.0.  See license.html for details.
#ifndef _SHASH_H_
#define _SHASH_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <cstdlib>
#include <malloc.h>
#include <memory.h>
   
/* From the MUSCLE documentation: */
/***
Some candidate alphabets considered because they
have high correlations and small table sizes.
Correlation coefficent is between k-mer distance
and %id D measured from a CLUSTALW alignment.
Table size is N^k where N is size of alphabet.
A is standard (uncompressed) amino alphabet.

                           Correlation
Alpha   N  k  Table Size   all   25-50%
-----  --  -  ----------   ----  ------
A      20  3       8,000  0.943   0.575
A      20  4     160,000  0.962   0.685 <<
LiA    14  4      38,416  0.966   0.645
SEB    14  4      38,416  0.964   0.634
LiA    13  4      28,561  0.965   0.640
LiA    12  4      20,736  0.963   0.620
LiA    10  5     100,000  0.964   0.652

We select A with k=4 because it has the best
correlations. The only drawback is a large table
size, but space is readily available and the only 
additional time cost is in resetting the table to
zero, which can be done quickly with memset or by
keeping a list of the k-mers that were found (should
test to see which is faster, and may vary by compiler
and processor type). It also has the minor advantage
that we don't need to convert the alphabet.

Fractional identity d is estimated as follows.

	F = fractional k-mer count
	if F is 0: F = 0.01
	Y = log(0.02 + F)
	d = -4.1 + 4.12*Y

The constant 0.02 was chosen to make the relationship
between Y and D linear. The constants -4.1 and 4.12
were chosen to fit a straight line to the scatterplot
of Y vs D.
***/

class shash {
private:
enum{
     K = 3, // length of k-mer to hash
     L = 21, // Alphabet size as number of distinct values
     HASH_MASK = L*L*L  // Set to L^K. Significant modulus for hashing
};

public:
enum{
     HASH_SIZE = HASH_MASK/8 + 1    // (L^K)/8 + 1
};

static unsigned int size() {return HASH_SIZE;}

private:
    unsigned char hbuf[HASH_SIZE];
   
    unsigned short AlphaDNA(char c);

    unsigned short AlphaPROT(char c);

	unsigned short AlphaBit(char c) {
        // For now, just hardcode which alphabet to pick
        return AlphaPROT(c);		
	}
	
	// set the bit in the hash buffer, hbuf, corresponding to posn
	void hash_set(unsigned int posn)
	{
		div_t ld = div(posn, 8);
		char bit = 1 << ld.rem;
		hbuf[ld.quot] |= bit;
	}

    void clear_hash() {
        memset(hbuf, 0, HASH_SIZE);
    }

	void init_hash(char *str) {
		unsigned char c;
		unsigned int hval = 0;
        clear_hash();
		for (int pos = 0; (c=*(str + pos)) != '\0'; ++pos) {
			hval += AlphaBit(c);
            hval %= HASH_MASK;
			if (pos >= K-1)
				hash_set(hval);
			hval *=  L;
		}

	}
public:
	shash(char *str)   {
		init_hash(str);
	}

    shash(void)  {
        clear_hash();
    }

	~shash() {
	}

    void clear(void) {
        clear_hash();
    }

    void init(char *str) {
        init_hash(str);
    }

	// Return number of unique matching k-mers between this and another shash
	short match(const shash &other) const;

    unsigned char * get(void) {return hbuf;}
    void set(unsigned char *h) {memcpy(hbuf, h, HASH_SIZE);}
};

short dist(const shash &x, const shash &y);
double slist_dist(shash **slist, int slist_cnt);
short bitcount (unsigned char n);


#endif