-- To enable external procedure calls you will need to modify your network/admin/listener.ora file
-- to look like what is shown below to allow external procedures.  
-- This link gives further details: http://www.dbasupport.com/oracle/ora9i/extprocedures.shtml
/*

SID_LIST_LISTENER =
  (SID_LIST =
    (SID_DESC =
      (SID_NAME = PLSExtProc)
      (ORACLE_HOME = C:\oracle\product\10.1.0\db_1)
      (PROGRAM = extproc)
	(ENVS="EXTPROC_DLLS=ANY")
    )
    (SID_DESC =
      (SID_NAME = orcl)
      (ORACLE_HOME = C:\oracle\product\10.2.0\db_2)
    )
  )

LISTENER =
  (DESCRIPTION_LIST =
    (DESCRIPTION =
      (ADDRESS_LIST =
        (ADDRESS = (PROTOCOL = IPC)(KEY = EXTPROC))
      )
      (ADDRESS_LIST =
        (ADDRESS = (PROTOCOL = TCP)(HOST = localhost)(PORT = 1521))
      )
    )
  )

*/

-- Oracle declarations for the functions in StdAfx.cpp

CREATE OR REPLACE LIBRARY SEQ_LIB AS  
     'C:\ORA_SUFFIX.dll'; -- Location of your dll
/

-- interface to get hash of sequence
CREATE OR REPLACE PROCEDURE 
ProcSeqHash(seq IN clob, hash in out nocopy blob) AS LANGUAGE C NAME 
       "SeqHash" LIBRARY SEQ_LIB WITH 
       CONTEXT PARAMETERS(CONTEXT, seq, hash);
/

create or replace function SeqHash(seq in CLOB) return BLOB is
hash_seq BLOB;
begin
   hash_seq := utl_raw.cast_to_raw('X');
   ProcSeqHash(seq, hash_seq);
   return hash_seq;
end;
/

CREATE OR REPLACE FUNCTION c_bitcount ( 
   str1 IN VARCHAR2)  
RETURN PLS_INTEGER AS LANGUAGE C 
NAME "bitcount" 
LIBRARY SEQ_LIB 
PARAMETERS (   
str1   STRING,  
str1   INDICATOR short,
str1   LENGTH short ,   
RETURN int); 
/

-- Now we are ready to define an aggregate function to compute
-- sequence similarities.  To understand what this is doing
-- see "User-Defined Aggregate Functions" in the Oracle 9i manual here:
-- http://www.lc.leidenuniv.nl/awcourse/oracle/appdev.920/a96595/dci11agg.htm#1004572


-- fast aggregation interface for hashed values

drop type seqhashlist force;
CREATE or replace TYPE SeqHashList AS VARRAY(20) OF BLOB;
/

create or replace function bitcount(c raw) return number is
score number;
begin
    if c = '00' then
		return 0;
	end if;

	score := 0;
	if UTL_RAW.BIT_AND(hextoraw('01'),c) <> '00' then
	  score := score + 1;	
	end if;
	if UTL_RAW.BIT_AND(hextoraw('02'),c) <> '00' then
	  score := score + 1;	
	end if;
    if UTL_RAW.BIT_AND(hextoraw('04'),c) <> '00' then
	  score := score + 1;	
	end if;
	if UTL_RAW.BIT_AND(hextoraw('08'),c) <> '00' then
	  score := score + 1;	
	end if;
	if UTL_RAW.BIT_AND(hextoraw('10'),c) <> '00' then
	  score := score + 1;	
	end if;
	if UTL_RAW.BIT_AND(hextoraw('20'),c) <> '00' then
	  score := score + 1;	
	end if;
	if UTL_RAW.BIT_AND(hextoraw('40'),c) <> '00' then
	  score := score + 1;	
	end if;
	if UTL_RAW.BIT_AND(hextoraw('80'),c) <> '00' then
	  score := score + 1;	
	end if;

	return score;
end;
/



create or replace function hash_sim(h1 in blob, h2 in blob) return number is
match blob;
score number;
c raw(1);
begin
    match := utl_raw.bit_and(h1, h2);

    score := c_bitcount(UTL_RAW.CAST_TO_VARCHAR2(match)); 
	return score;

    -- old, slow version
	score := 0;
	-- N.B.!! Start at 2!  The hash blobs have a bogus leading character
	for i in 2 .. utl_raw.length(match) loop
		c := utl_raw.substr(match, i, 1);
		-- score := score + bitcount(c);
	end loop;

	return score;
end;
/

create or replace type SeqSimHashImpl as object
(
  hlist SeqHashList,
  static function ODCIAggregateInitialize(sctx IN OUT SeqSimHashImpl) 
    return number,
  member function ODCIAggregateIterate(self IN OUT SeqSimHashImpl, 
    seq IN blob) return number,
  member function ODCIAggregateTerminate(self IN SeqSimHashImpl, 
    returnValue OUT number, flags IN number) return number,
  member function ODCIAggregateMerge(self IN OUT SeqSimHashImpl, 
    ctx2 IN SeqSimHashImpl) return number
);
/

create or replace type body SeqSimHashImpl is 
static function ODCIAggregateInitialize(sctx IN OUT SeqSimHashImpl) 
return number is 
begin
  sctx := SeqSimHashImpl(SeqHashList());
  return ODCIConst.Success;
end;

member function ODCIAggregateIterate(self IN OUT SeqSimHashImpl, seq IN BLOB) 
return number is

begin
  -- add blob to list
  hlist.extend;
  hlist(hlist.Last) := seq;
  return ODCIConst.Success;

exception 
  -- went over the limit of the number of sequences we are allowed to 
  -- aggregate as defined by the size of the SeqHashList type
  when SUBSCRIPT_OUTSIDE_LIMIT then 
	return ODCIConst.Success;
end;


-- count number of bits that match when two hash strings are
-- ANDed together
--

member function ODCIAggregateTerminate(self IN SeqSimHashImpl, returnValue OUT number, 
flags IN number) return number is
ij_sim number;
sim number;
cnt number;
begin
-- dbms_output.enable;

  sim := 0;
  cnt := 0;
  FOR i IN hlist.FIRST..hlist.LAST LOOP
    FOR j IN i+1..hlist.LAST LOOP
		cnt := cnt + 1;
        ij_sim := hash_sim(hlist(i), hlist(j));
-- dbms_output.put_line('i = ' || i || ' j = ' || j || ' sim = ' || ij_sim);
		sim := sim + ij_sim; 
	END LOOP; 
  END LOOP; 

  if cnt > 0 then
     returnValue := sim / cnt;
  else
     returnValue := 0;
  end if;

  return ODCIConst.Success;
end;

member function ODCIAggregateMerge(self IN OUT SeqSimHashImpl, ctx2 IN SeqSimHashImpl) 
return number is
begin
  -- not implemented
  return ODCIConst.Success;
end;
end;
/

CREATE OR REPLACE FUNCTION SeqSimHash (input BLOB) RETURN NUMBER 
AGGREGATE USING SeqSimHashImpl;
/

-- select seqsimhash(seq_hash) from seq.sequences;
-- select seqsimhash(seq_hash) from iproclass.seq_facts;



--- friendly non-hash interface
--- essentially this is a duplicate of the code above, just take
--- CLOB / actual sequence as input and hash to blob before storing
------------------------------------------------------------------------
create or replace type SeqSimImpl as object
(
  hlist SeqHashList,
  static function ODCIAggregateInitialize(sctx IN OUT SeqSimImpl) 
    return number,
  member function ODCIAggregateIterate(self IN OUT SeqSimImpl, 
    seq IN CLOB) return number,
  member function ODCIAggregateTerminate(self IN SeqSimImpl, 
    returnValue OUT number, flags IN number) return number,
  member function ODCIAggregateMerge(self IN OUT SeqSimImpl, 
    ctx2 IN SeqSimImpl) return number
);
/

create or replace type body SeqSimImpl is 
static function ODCIAggregateInitialize(sctx IN OUT SeqSimImpl) 
return number is 
begin
  sctx := SeqSimImpl(SeqHashList());
  return ODCIConst.Success;
end;

member function ODCIAggregateIterate(self IN OUT SeqSimImpl, seq IN CLOB) 
return number is

begin
  -- add CLOB to list
  hlist.extend;
  hlist(hlist.Last) := SeqHash(seq);
  return ODCIConst.Success;
exception 
  -- went over the limit of the number of sequences we are allowed to 
  -- aggregate as defined by the size of the SeqHashList type
  when SUBSCRIPT_OUTSIDE_LIMIT then 
	return ODCIConst.Success;
end;


member function ODCIAggregateTerminate(self IN SeqSimImpl, returnValue OUT number, 
flags IN number) return number is
ij_sim number;
sim number;
cnt number;
begin
-- dbms_output.enable;

  sim := 0;
  cnt := 0;
  FOR i IN hlist.FIRST..hlist.LAST LOOP
    FOR j IN i+1..hlist.LAST LOOP
		cnt := cnt + 1;
        ij_sim := hash_sim(hlist(i), hlist(j));
-- dbms_output.put_line('i = ' || i || ' j = ' || j || ' sim = ' || ij_sim);
		sim := sim + ij_sim; 
	END LOOP; 
  END LOOP; 

  if cnt > 0 then
     returnValue := sim / cnt;
  else
     returnValue := 0;
  end if;

  return ODCIConst.Success;
end;

member function ODCIAggregateMerge(self IN OUT SeqSimImpl, ctx2 IN SeqSimImpl) 
return number is
begin
  -- not implemented
  return ODCIConst.Success;
end;
end;
/

CREATE OR REPLACE FUNCTION SeqSim (input CLOB) RETURN NUMBER 
AGGREGATE USING SeqSimImpl;
/

-- Test the above functions and create synonyms
drop table temp;
create table temp (
seq CLOB,
hash_seq BLOB
);

insert into temp values('GATTACA', NULL);
insert into temp values('GATTACA', NULL);
insert into temp values('ACA', NULL);
commit;

update temp set hash_seq = SeqHash(seq);
commit;

-- Similarity matrix:
-- X 5 1
-- 5 X 1
-- 1 1 X 

-- should get 5+1+1/3 = 7/3 = 2.333
select SeqSim(seq) from temp;
select SeqSimHash(hash_seq) from temp;    

drop public synonym seqhash;
grant execute on seqhash to public;
create public synonym seqhash for seqhash; 

drop public synonym seqsimhash;
grant execute on seqsimhash to public;
create public synonym seqsimhash for seqsimhash; 

drop public synonym seqsim;
grant execute on seqsim to public;
create public synonym seqsim for seqsim; 
