xgps
====

This is a University 4 month solo project I did back in 2010. It dealt with flat .gps files that stored waypoints, routes and recorded track information (I forget the specification name).

### gputil

A library for dealing with reading, writing and parsing the flat file format.


### gpstool

A command line utility, utilizing gputil, that allowed manipulation of data stored in the .gps files, including:

* Counts of waypoints, routes, trackpoints and tracks
* Discarding waypoints, routes and trackpoints
* Sorting waypoints
* Merging .gps files

And it supported piping to itself!

### xgps

A TkInter GUI over gputil + gpstool

* Uses the Python C API + gputil to read + write .gps files
* Uses gpstool to provide all the capabilities of gpstool (popen3)
* Supported importing data into MySQL
* Run a few canned queries against MySQL data, or a custom query
* Display data on Google Maps
