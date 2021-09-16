// Licensed under Common Public License 1.0.  See license.html for details.

// stdafx.cpp : source file that includes just the standard includes
//	ORA_SUFFIX.pch will be the pre-compiled header
//	stdafx.obj will contain the pre-compiled type information


#include "stdafx.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oci.h>
#include <ociextp.h>

#include "shash.h"

#include <stdio.h>

// #define DEBUG_PRINT

#ifdef DEBUG_PRINT
FILE *fp = NULL;
#endif

// Cannot get alloc & free to work properly under oracle.
// Use a global for storage.  This is bad because there is some risk that the DLL might
// get unloaded during the course of an aggregation call, also Oracle may eventually move to
// calling this in an multi-threaded way.
//
// When we want to make this more robust we could switch to a memory mapped file
const int MAX_STRINGS = 5;
const int MAX_LISTS = 1000;
shash g_slist[MAX_STRINGS]; 
int g_slist_used[MAX_STRINGS];

shash* g_slist_sets[MAX_LISTS][MAX_STRINGS]; 

int g_slist_set_cnt[MAX_LISTS]; 
int g_slist_cnt = 0;


// -----------------------------------------------------------------------------------------
// N.B.!! If you change and functions below then you need to change the Oracle interface
// defined in Suffix.sql
// -----------------------------------------------------------------------------------------


extern "C" __declspec(dllexport) ub4 AggregateInitialize(OCIExtProcContext * ctx, OCINumber dummy) 
{
  static bool bInit = false;
  if (!bInit) {
      memset(&g_slist_sets, 0, sizeof(g_slist_sets));
      memset(&g_slist_set_cnt, 0, sizeof(g_slist_set_cnt));
      memset(&g_slist_used, 0, sizeof(g_slist_used));
#ifdef DEBUG_PRINT
      fp = fopen("C:\\debug.txt", "a");
#endif
      bInit = true;
  }

  for (int i=0; i<MAX_LISTS; i++) {
      if(g_slist_set_cnt[i] == 0) // found an available list
          break;
  }

#ifdef DEBUG_PRINT
  fprintf(fp, "Aggie Init. Returned list slot %d \n", i);
  fflush(fp);
#endif

  /* Return slot of first available list */ 
  return((ub4)i); 

}


struct ocictx
{
  OCIEnv     *envhp;                           /* For OCI Environment Handle */
  OCISvcCtx  *svchp;                               /* For OCI Service Handle */
  OCIError   *errhp;                                /* For OCI Error Handle  */
  OCIStmt    *stmtp;                             /* For OCI Statement Handle */
  OCIStmt    *stm1p;                             /* For OCI Statement Handle */
  OCIBind    *bnd1p;                                  /* For OCI Bind Handle */
  OCIBind    *bnd2p;                                  /* For OCI Bind Handle */
  OCIBind    *bnd3p;                                  /* For OCI Bind Handle */
  OCIDefine  *dfn1p;                                /* For OCI Define Handle */
  OCIDefine  *dfn2p;                                /* For OCI Define Handle */
  OCIDefine  *dfn3p;                                /* For OCI Define Handle */
};
typedef struct ocictx ocictx;

void check_err(char *str, int err, char *errbuf, ocictx *oci_ctxp )
{
    if ( err ) 
    { 
      OCIErrorGet( (dvoid *)oci_ctxp->errhp, (ub4) 1, (text *)0, (sb4 *)&err, 
         (text *)errbuf, (ub4)sizeof( errbuf ), (ub4)OCI_HTYPE_ERROR ); 
#ifdef DEBUG_PRINT
   fprintf(fp,"%s returned errno: %d\nerrstr: %s\n", str, err, errbuf);
  
#endif
      
      return; 
    } 
    else {
#ifdef DEBUG_PRINT
   fprintf(fp,"%s returned: 0\n", str);
#endif
      
    }
}


void decode_clob (
                   OCIExtProcContext  *with_context,      /* With Context ptr */
		           OCILobLocator      *text_in,
                   char           **in_bufp, /* ptr set to allocated buffer for input text */
                   ocictx         *oci_ctxp /* store context details */
                   )
{
  int            err;

  ub4            in_amount;
  ub4            in_offset;       
  ub4            in_buf_len;

  char           errbuf[512];
 
  ub4            text_size;


  /* Obtain OCI handle for SQL statement using the context passed. */
  err = OCIExtProcGetEnv(with_context,                       /* With context */
                         &oci_ctxp->envhp,
                         &oci_ctxp->svchp,
                         &oci_ctxp->errhp);
  check_err("OCIExtProcGetEnv", err, errbuf, oci_ctxp);

  /* Get the size of the input lob */

  err = OCILobGetLength(oci_ctxp->svchp,
                        oci_ctxp->errhp,
                        text_in,
                        &text_size);
  check_err("OCILobRead", err, errbuf, oci_ctxp);
  if (err) {*in_bufp = NULL; return;}

  /* Allocate single buffer large enough for whole input LOB */

  *in_bufp = (char *)malloc(text_size+1);
  if (*in_bufp == (char *)NULL) {
#ifdef DEBUG_PRINT
    fprintf(fp, "Out of memory in user lexer\n");
#endif
    return;
  }

  /* Now read the LOB */

  in_amount = text_size;
  in_offset = 1;
  in_buf_len = text_size+1;

  err = OCILobRead(oci_ctxp->svchp,
                   oci_ctxp->errhp,
                   text_in,
                   &in_amount,
                   in_offset,
                   (dvoid *) *in_bufp,
                   in_buf_len,
                   (dvoid *) 0,
                   (sb4 (*)(dvoid *, CONST dvoid *, ub4, ub1 )) 0,
                   (ub2) 0, 
                   (ub1) SQLCS_IMPLICIT);
  check_err("OCILobRead", err, errbuf, oci_ctxp);
  if (err) {free(*in_bufp); *in_bufp = NULL; return;}

  /* null terminate the string in the buffer */
  (*in_bufp)[in_amount] = (char)NULL;

#ifdef DEBUG_PRINT
  fprintf(fp, "read clob: %s\n", *in_bufp);
#endif
}


// iterate, adding already encoded sequences
extern "C" __declspec(dllexport) int AggregateIterate(OCIExtProcContext * ctx, OCILobLocator   *clob_seq, ub4  slist) 
{ 
  ocictx         oci_ctx;

  if (g_slist_cnt >= MAX_STRINGS)
      return 1; // no more room in the string list

  int this_list = (int) slist;

  char   *str1 = NULL;
  decode_clob(ctx, clob_seq, &str1, &oci_ctx);

  /* Check for null inputs. */ 
  if (str1 == NULL  || str1[0] == 0) 
  { 
      return 0;  // nothing to do
  } 

                   

#ifdef DEBUG_PRINT
  fprintf(fp, "AggregateIterate. Passed list slot %d \n", this_list);
#endif

  if (this_list < 0 || this_list >= MAX_LISTS) {
      free(str1);
      return 1; // invalid string list
  }

  // find the first available string slot
  for (int i=0; i<MAX_STRINGS; i++) {
      if(g_slist_used[i] == 0)  // found an string slot
          break;
  }

  if (i >= MAX_STRINGS) {
      free(str1);
      return 1; // no more room in the string list
  }

  g_slist_used[i] = 1;

#ifdef DEBUG_PRINT
  fprintf(fp, "AggregateIterate. Found string slot %d \n", i);
  fflush(fp);
#endif

  // store processed string 
  // g_slist[i].init(str1);

  // offset of 1 because HashSet is called with initial dummy char
  // see the definition of the function SeqHash in suffix.sql where we prepend 'X' to the hash
  g_slist[i].set((unsigned char *)(str1+1));  
  
  g_slist_cnt++;
  free(str1);


  g_slist_sets[this_list][g_slist_set_cnt[this_list]] = &g_slist[i];
  g_slist_set_cnt[this_list]++;

  return 0; 
} 


// Take input sequence as clob_seq, return hashed version as hash_seq
extern "C" __declspec(dllexport) void SeqHash(OCIExtProcContext * ctx, 
OCILobLocator   *clob_seq, OCILobLocator   **hash_seq) 
{ 
  static int out_offset;
  ub4 out_len;  
  ub4 err;
  ub4 out_amount;
  char errbuf[512];
  ocictx         oci_ctx;
  ocictx         *oci_ctxp = &oci_ctx;
 

#ifdef DEBUG_PRINT
  if (fp == NULL)
    fp = fopen("C:\\debug.txt", "a");
#endif

#ifdef DEBUG_PRINT
  fprintf(fp, "Hash_SEQ called \n");
  fflush(fp);
#endif

  out_offset = 1;
  char   *str1 = NULL;
  decode_clob(ctx, clob_seq, &str1, oci_ctxp);


  /* Check for null inputs. */ 
  if (str1 == NULL  || str1[0] == 0) 
  { 
      return;  // nothing to do
  }   

#ifdef DEBUG_PRINT
  fprintf(fp, "Hash_SEQ called with %s\n", str1);
  fflush(fp);
#endif

  shash sh(str1);
  free(str1);

  out_len = sh.size();
  out_amount = out_len;

#ifdef DEBUG_PRINT
  fprintf(fp, "Hash_SEQ returns length %d\n", out_len);
  fflush(fp);
  for (int i=0; i< (int)out_len; ++i)
      fprintf(fp, "%c", (sh.get())[i]);
  fprintf(fp, "\n");
  fflush(fp);
#endif

  err = OCILobWriteAppend(oci_ctxp->svchp, 
                    oci_ctxp->errhp, 
                    *hash_seq, 
                    &out_amount, 
                          /* out_offset, - only needed for write, not append */
                    (dvoid *) sh.get(), 
                    (ub4) out_len, 
                    OCI_ONE_PIECE, 
                    (dvoid *)0, 
                    (sb4 (*)(dvoid *, dvoid *, ub4 *, ub1 * )) 0, 
                    (ub2)0, (ub1)SQLCS_IMPLICIT); 
  check_err("OCILobWrite", err, errbuf, oci_ctxp);

  return;
}



// Merge two lists of strings
extern "C" __declspec(dllexport) int AggregateMerge(OCIExtProcContext * ctx, ub4  this_slist,
                                                       ub4  other_slist) 
{ 
    int this_list = (int) this_slist;
    int other_list = (int) other_slist;

#ifdef DEBUG_PRINT
    fprintf(fp, "AggregateMerge. Passed this_list slot %d \n", this_list);
    fprintf(fp, "AggregateMerge. Passed other_list slot %d \n", other_list);
    fflush(fp);
#endif

    if (this_list < 0 || this_list >= MAX_LISTS)
      return 1; // invalid string list

    if (other_slist < 0 || other_slist >= MAX_LISTS)
      return 1; // invalid string list

    // copy list of strings from other --> this
    for (int i=0; i<g_slist_set_cnt[other_list]; i++) {
        g_slist_sets[this_list][g_slist_set_cnt[this_list]] = g_slist_sets[other_list][i];
        g_slist_set_cnt[this_list]++;
    }

    // free list slot
    g_slist_set_cnt[other_list] = 0;

    
    return 0;
}

void free_slist(int this_list)
{
     // free string slots
    for (int i=0; i<g_slist_set_cnt[this_list]; i++) {
      int slot = g_slist_sets[this_list][i] - g_slist;
      g_slist_used[slot] = 0;
      g_slist_cnt--;
    }

    // free list slot
    g_slist_set_cnt[this_list] = 0;
}

// Finally, put all the strings together
extern "C" __declspec(dllexport) double AggregateTerminate(OCIExtProcContext * ctx, ub4  slist) 
{ 
    int this_list = (int) slist;

#ifdef DEBUG_PRINT
    fprintf(fp, "AggregateTerminate. Passed list slot %d \n", this_list);
    fflush(fp);
#endif

     if (this_list < 0 || this_list >= MAX_LISTS)
      return 0; // invalid string list


    double result = slist_dist(g_slist_sets[this_list], g_slist_set_cnt[this_list]); // total size of strings

    free_slist(this_list);

    return result;
}


extern "C" __declspec(dllexport) int bitcount(char *str1, short str1_i, short str_l)  
{ 
  /* Check for null inputs. */ 
  if (str1_i == OCI_IND_NULL) 
  { 
      return(0);  
  } 
  
  int count = 0;
  // N.B.!! Start at 1.  
  // offset of 1 because HashSet is called with initial dummy char
  // see the definition of the function SeqHash in suffix.sql where we prepend 'X' to the hash
  for (int i=1; i<str_l; ++i)
      count += bitcount(str1[i]);

  return count;
} 


