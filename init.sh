#!/usr/bin/env bash

sqlite3 urls.db <<EOF
CREATE TABLE urls (
	token VARCHAR UNIQUE NOT NULL,
	url VARCHAR UNIQUE NOT NULL
);
EOF
