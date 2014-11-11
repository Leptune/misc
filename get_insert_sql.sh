#!/bin/bash
#Author: Leptune
#Email : leptune@live.cn
#Date  : 2014-9-1 15:23:30
#Desc  : export insert sql from oracle's table
#Usage : ./script_name db_addr table_name [condition_sql]

function esql()
{
    local db_addr="$1"
    local sql="$2"

    sql_len=${#sql}
    last_ch=${sql:($sql_len-1)}
    if [ "$last_ch" != ";" ]; then
        sql=`echo "$sql;"`
    fi
sqlplus -S /nolog<<EOF
set heading off feedback off pagesize 0 verify off echo off linesize 32767 trimspool on trimout on
conn $db_addr
$sql
exit
EOF
}

function is_blank_string()
{
    local string=$1
    local res=`echo $string | sed 's/ //g'`
    [ "z$res" = "z" ]
}

db_addr="$1"
table_name="$2"
cond_sql="$3"

[ $# != 2 -a $# != 3 ] && echo "Usage: $0  db_addr  table_name  [condition_sql]" \
            && echo "e.g1: $0 srbank/srbank@srbank fw_tran_codes" \
            && echo "e.g2: $0 srbank/srbank@srbank fw_tran_codes \"where code = '001003'\"" \
            && exit 1

columns=(`esql "$db_addr" "desc $table_name" | tail -n +3 | awk '{print $1}'`)
# /x27 is single quotes(')
sql="select 'insert into $table_name(`echo ${columns[@]} | sed 's/ /, /g'`) values(''' || `echo ${columns[@]} | sed 's/ / || \x27\x27\x27,\x27\x27\x27 || /g'` || ''');' from $table_name"
is_blank_string $cond_sql
[ $? = 0 ] && esql "$db_addr" "$sql" || esql "$db_addr" "$sql $cond_sql"
exit 0
