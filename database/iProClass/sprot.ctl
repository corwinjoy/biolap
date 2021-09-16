LOAD DATA
INFILE sprot40_formatted.txt
INTO TABLE swissprot
REPLACE
FIELDS TERMINATED BY '|'
TRAILING NULLCOLS
(
seq_id,
creation_date,
organism,
seq_data char(100000)
)

