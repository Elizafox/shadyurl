#!/usr/bin/env bash

if [ ! -e urls.db]; then
	echo "Creating database"
	(sqlite3 urls.db <<EOF
CREATE TABLE urls (
	token VARCHAR UNIQUE NOT NULL,
	url VARCHAR UNIQUE NOT NULL
);
EOF
	) || exit 1
fi

if [ ! -e dh.pem ]; then
	echo "Creating DH parameters"
	openssl dhparam -out dh.pem 2048 || exit 1
fi

echo
echo "Done! Don't forget to create cert.pem and key.pem."
