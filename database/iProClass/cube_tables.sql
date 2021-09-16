create table seq_facts as
select c.csqid, c.swp_ac, 
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 1),';')) tax1,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 2),';')) tax2,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 3),';')) tax3,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 4),';')) tax4,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 5),';')) tax5,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 6),';')) tax6,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 7),';')) tax7,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 8),';')) tax8,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 9),';')) tax9,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 10),';')) tax10,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 11),';')) tax11,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 12),';')) tax12,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 13),';')) tax13,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 14),';')) tax14,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 15),';')) tax15,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 16),';')) tax16,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 17),';')) tax17,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 18),';')) tax18,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 19),';')) tax19,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 20),';')) tax20,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 21),';')) tax21,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 22),';')) tax22,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 23),';')) tax23,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 24),';')) tax24,
ltrim(replace(regexp_substr(c.lineage, '[^;]+;', 1, 25),';')) tax25,
c.lineage,
t.acc go_acc, t.name go_name, s.seq_data  from CSQREPORT2_30 c, swissprot s, term t where
c.swp_ac = s.seq_id and
substr(c.go,1,10) = t.acc;

alter table seq_facts add(seq_hash blob);
update seq_facts set seq_hash = SeqHash(seq_data);
commit;

alter table seq_facts add(seq_data2 varchar2(4000));
update seq_facts set seq_data2 = substr(SEQ_DATA,1,4000);
commit;


-- create joined go view for use by cube
create or replace view go_term as
select t2.*, t.name from term2term t2, term t
where t2.term2_id = t.id;




-- mondrian aggregates, issue select commands to cmdRunner to get aggregate suggestions
/*
select {[Measures].[SequenceSimilarity], [Measures].[SequenceCount]} on columns,
  {([GO].[All Terms], [Taxonomy].[All Species])} ON rows
from Proteins;
*/

drop materialized view agg_l_a_SEQ_FACTS;
create materialized view agg_l_a_SEQ_FACTS
build immediate refresh complete
as
select
count(CSQID) as SEQ_DATA2,
system.seqsimhash(seq_hash) as SEQ_HASH,
count(CSQID) as "fact_count"
FROM
SEQ_FACTS;
create or replace view agg_c_a_SEQ_FACTS as select * from agg_l_a_SEQ_FACTS;


/*
select {[Measures].[SequenceSimilarity], [Measures].[SequenceCount]} on columns,
  {([GO].Members)} ON rows
from Proteins;
*/
    
drop materialized view agg_l_g_SEQ_FACTS ;
CREATE materialized view agg_l_g_SEQ_FACTS 
build immediate refresh complete
as
SELECT
    "SEQ_FACTS"."GO_ID" AS "GO_ID",
    COUNT("SEQ_FACTS"."CSQID") AS "SEQ_DATA2",
    SEQSIMHASH("SEQ_FACTS"."SEQ_HASH") AS "SEQ_HASH",
    COUNT(CSQID) AS "fact_count"
FROM 
    "SEQ_FACTS" "SEQ_FACTS"
GROUP BY 
    "SEQ_FACTS"."GO_ID";
    


drop materialized view agg_c_t_SEQ_FACTS ;
CREATE materialized view agg_c_t_SEQ_FACTS 
build immediate refresh complete
as
SELECT
    "GRAPH_PATH"."TERM1_ID" AS "TERM1_ID",
    COUNT("SEQ_FACTS"."CSQID") AS "CSQID",
    SEQSIMHASH("SEQ_FACTS"."SEQ_HASH") AS "SEQ_HASH",
    COUNT(CSQID) AS "fact_count"
FROM 
    "SEQ_FACTS" "SEQ_FACTS",
    "GRAPH_PATH" AS "GRAPH_PATH"
WHERE 
    "SEQ_FACTS"."GO_ID" = "GRAPH_PATH"."TERM2_ID"
GROUP BY 
    "GRAPH_PATH"."TERM1_ID";
    
/*    
select
  {[Measures].[SequenceSimilarity], [Measures].[SequenceCount]} on columns,
  {([GO].[All Terms], [Taxonomy].[TAX1].[cellular organisms])} ON rows
from Proteins;
*/

drop materialized view agg_l_t1_SEQ_FACTS ;
CREATE materialized view agg_l_t1_SEQ_FACTS 
build immediate refresh complete
as
SELECT
    "SEQ_FACTS"."TAX1" AS "TAX1",
    COUNT(*) AS "SEQ_DATA2",
    SEQSIMHASH("SEQ_FACTS"."SEQ_HASH") AS "SEQ_HASH",
    COUNT(*) AS "fact_count"
FROM 
    "SEQ_FACTS" "SEQ_FACTS"
GROUP BY 
    "SEQ_FACTS"."TAX1";
    
create or replace view agg_c_t1_SEQ_FACTS as select * from agg_l_t1_SEQ_FACTS;


/*
select
  {[Measures].[SequenceSimilarity], [Measures].[SequenceCount]} on columns,
  {([GO].[molecular_function], [Taxonomy].[TAX1].[cellular organisms])} ON rows
from Proteins;
*/

drop materialized view agg_l_gt_SEQ_FACTS ;
CREATE materialized view agg_l_gt_SEQ_FACTS 
build immediate refresh complete
as
SELECT
    "SEQ_FACTS"."TAX1" AS "TAX1",
    "SEQ_FACTS"."GO_ID" AS "GO_ID",
    COUNT(*) AS "SEQ_DATA2",
    SEQSIMHASH("SEQ_FACTS"."SEQ_HASH") AS "SEQ_HASH",
    COUNT(*) AS "fact_count"
FROM 
    "SEQ_FACTS" 
GROUP BY 
    "SEQ_FACTS"."TAX1",
    "SEQ_FACTS"."GO_ID";

drop materialized view agg_c_gt_SEQ_FACTS ;
CREATE materialized view agg_c_gt_SEQ_FACTS 
build immediate refresh complete
as
SELECT
    "GRAPH_PATH"."TERM1_ID" AS "TERM1_ID",
    "SEQ_FACTS"."TAX1" AS "TAX1",
    COUNT(*) AS "SEQ_DATA2",
    SEQSIMHASH("SEQ_FACTS"."SEQ_HASH") AS "SEQ_HASH",
    COUNT(*) AS "fact_count"
FROM 
    "GRAPH_PATH" AS "GRAPH_PATH",
    "SEQ_FACTS" AS "SEQ_FACTS"
WHERE 
    "SEQ_FACTS"."GO_ID" = "GRAPH_PATH"."TERM2_ID" and
GROUP BY 
    "GRAPH_PATH"."TERM1_ID",
    "SEQ_FACTS"."TAX1";





