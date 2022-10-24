shadyurl - make your URLs shady
-------------------------------
This is a thing that makes your URLs shady.

Setup
=====
* Run `init.sh`
* Put the SSL `key.pem` and `cert.pem` from your certificate provider (or self-signed) along with `mimetypes.txt` and `config.toml` in the directory you intend to run `shadyurl`.

Building
========
This project uses Meson. Run `meson setup build && cd build && meson compile && meson install` to install it.

Dependencies
============
This project depends on a C++20 compiler, OpenSSL, Boost, pthreads, and sqlite3.
