ETVDB
=====

1) What is it?
--------------
ETVDB is a tool and a library to get data from [The TV Database (TVDB)][1].
It is written in C and makes use of the [Enlightenment Foundation Libraries][2]
and [cURL][3].

[1]: http://thetvdb.com/
[2]: http://www.enlightenment.org/
[3]: http://curl.haxx.se/

2) File Structure
-----------------
A quick overview over the files and directories here:
```
bin/ - contains the binary/executable files
data/ - contains data used by the program
external/ - 3rd party libraries
lib/ - contains the library files
```

3) How to build
---------------
This is super easy:
```
mkdir BUILD/ && cd BUILD/
cmake .. && make && make install
```

for a debugging build pass this to cmake:
-D DEBUG=ON

The dependencies are Eina and libcurl, nothing else.

4) License
----------
ETVDB is available under the LGPLv2.1 or any later version.
See COPYING for details.
