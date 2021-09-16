// Licensed under Common Public License 1.0.  See license.html for details.
#include "shash.h"
#include <iostream>

using namespace std;     
 
// fast count of the number of bits in an unsigned char
// adapted from http://www-db.stanford.edu/~manku/bitcount/bitcount.html

const short *bit_init(void)
{
	static short bic [256] ;  
	static bool b_init = false;
	if (!b_init) {
		// initialize bit counts
		b_init = true;
		for (int i=0; i<= 255; ++i)
		{
			int count=0; 
			int n = i;
			while (n)
			{
				count += n & 0x1u ;
				n >>= 1 ;
			}
			bic[i] = count;
		}
	}
	return bic;
}



short bitcount (unsigned char n)
{
	const static short *bits_in_char  = bit_init() ;
    return bits_in_char[n];
}


unsigned short shash::AlphaDNA(char c) {
    // Translate DNA letter to bit
	// If you expand this, you will need to change L beyond 2 bits.
	switch(c) {
		case 'A': return 0;
		case 'C': return 1;
		case 'G': return 2;
		case 'T': return 3;
		default: return 0;
	}

}

short *init_prot(void) {
    static short alpha[26];
    char letters[] = "ACDEFGHIKLMNPQRSTVWY"; // valid protein codes

    // initialize array to convert valid letter to number
    // we reserve 0 for unrecognized letter or gap
    memset(alpha, 0, 26);
    for(int pos=0; letters[pos] != 0; ++pos) {
        alpha[letters[pos] - 'A'] = pos + 1;
    }

    return alpha;
}

unsigned short shash::AlphaPROT(char c) {
    static short *alpha = init_prot();

    short offset = c - 'A';
    if (offset < 0 || offset >= 26)
        return 0;

    return alpha[offset];
}



	// Return number of unique matching k-mers between this and another shash
short shash::match(const shash &other) const {
		unsigned char c;
		short count = 0;
		for (int i=0; i<HASH_SIZE; ++i) {
			c = hbuf[i] & other.hbuf[i];
			count += bitcount(c);
		}
		return count;
	}


short dist(const shash &x, const shash &y) {
    return x.match(y);
}

// Compute a single similary measure for the set.
// For now, find the average of above diagonal entries in the distance matrix
// i.e. compute the average distance between all pairs
double slist_dist(shash **slist, int slist_cnt) {

    double sum = 0, count = 0;
    for (int i=0; i<slist_cnt; ++i) {
        for (int j=i+1; j<slist_cnt; ++j) {
            sum += dist(*(slist[i]), *(slist[j]));
            count++;
        }
    }

    return sum/count;
    
}

#if 0

int main(void) {
	shash sh("GATTACAGATTAGA"), sh2("GATTACAGATTAGA");
	cout << "Matching k-mers: " << sh.match(sh2) << "\n";

	return 0;
}

#endif