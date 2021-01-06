#!/bin/bash

#set -x

BASE_PATH="/usr/local/adbot/{{instance_name}}/data/"
GROUP_NAME="{{group_name}}"
NODE_ID="{{node_id}}"

SOURCE_BASE_DATA=$BASE_PATH"source_data/base/"
SOURCE_DELTA_DATA=$BASE_PATH"source_data/delta/"
ROCKSDB_DATA=$BASE_PATH"data/$GROUP_NAME/$NODE_ID/"

function clean_database_base() {
	if [ ! -d $SOURCE_BASE_DATA ]; then
		return;
	fi

	DATABASE_LIST=`ls $SOURCE_BASE_DATA`
	for database in ${DATABASE_LIST[@]}
	do
		echo "Start clean source data path:$SOURCE_BASE_DATA/$database"
		# 清理 source base
		SOURCE_BASE_TABLES=`ls $SOURCE_BASE_DATA/$database`
		for table in ${SOURCE_BASE_TABLES[@]}
		do
			clean_table_base $SOURCE_BASE_DATA/$database/$table
		done
	done
}

function clean_database_delta() {
	if [ ! -d $SOURCE_DELTA_DATA ]; then
		return
	fi

	DATABASE_LIST=`ls $SOURCE_DELTA_DATA`
	for database in ${DATABASE_LIST[@]}
	do
		echo "Start clean source delta data path:$SOURCE_DELTA_DATA/$database"
		# 清理 source base
		SOURCE_DELTA_TABLES=`ls $SOURCE_DELTA_DATA/$database`
		for table in ${SOURCE_DELTA_TABLES[@]}
		do
			clean_table_delta $SOURCE_DELTA_DATA/$database/$table
		done
	done
}


function clean_database_rocksdb() {
	if [ ! -d $ROCKSDB_DATA ]; then
		return
	fi

	DATABASE_LIST=`ls $ROCKSDB_DATA`
	for database in ${DATABASE_LIST[@]}
	do
		echo "Start clean rocksdb data path:$ROCKSDB_DATA/$database"
		# 清理 source base
		ROCKSDB_TABLES=`ls $ROCKSDB_DATA/$database`
		for table in ${ROCKSDB_TABLES[@]}
		do
			clean_table_rocksdb $ROCKSDB_DATA/$database/$table
		done
	done

}


function clean_dir() {
	if [ ! -d $1 ]; then
		return
	fi
	FILE_LIST=`ls -lthr $1 |head -n -{{keep_version_num}} | grep -v 'total' | awk '{print $9}'`
	for file in ${FILE_LIST[@]}
	do
		echo "Remove file: $1/$file"
		rm -rf $1/$file
	done
}

function clean_table_base() {
	if [ ! -d $1 ]; then
		return
	fi

	# 清理 source base
	echo "Start clean table path:$1"
	PARTS=`ls $1`
	for part in ${PARTS[@]}
	do
		clean_dir $1/$part
	done
}

function clean_table_rocksdb() {
	if [ ! -d $1 ]; then
		return
	fi

	# 清理 source base
	echo "Start clean rocksdb table path:$1"
	PARTS=`ls $1`
	for part in ${PARTS[@]}
	do
		clean_dir $1/$part
	done
}

function clean_table_delta() {
	if [ ! -d $1 ]; then
		return
	fi

	# 清理 source base
	echo "Start clean table path:$1"
	clean_dir $1
}

clean_database_base
clean_database_delta
clean_database_rocksdb
