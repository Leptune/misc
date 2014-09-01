#!/bin/bash
#Author: Leptune
#Email : leptune@live.cn
#Date  : 2014-9-1 15:23:30
#Desc  : export insert sql from oracle's table
#Usage : ./script_name table_name

db_addr="srbank/srbank@srbank"

function esql()
{
    local sql="$1"

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

function get_insert_sql()
{
    local table_name=$1
    local columns=(`esql "desc $table_name" | tail -n +3 | awk '{print $1}'`)

    # /x27 is single quotes(')
    esql "select 'insert into $table_name(`echo ${columns[@]} | sed 's/ /, /g'`) values(''' || `echo ${columns[@]} | sed 's/ / || \x27\x27\x27,\x27\x27\x27 || /g'` || ''');' from $table_name"
}

table_name="$1"
[ $# != 1 ] && echo "Usage: $0 table_name" && exit 1
get_insert_sql $table_name
exit 0
