/********
gpstool.h -- header file for gpstool.c in Asmt 2
Last updated:  January 28, 2010 03:42:46 PM       

Eric Coutu
#0523365
********/

#ifndef GPSTOOL_H_
#define GPSTOOL_H_ A2

#include "gputil.h"

static char codes[][48] = {
    "OK",                                           // OK
    "IO error",                                     // IOERR
    "unknown record type",                          // UNKREC
    "bad field seperator",                          // BADSEP
    "unacceptable file type",                       // FILTYP
    "unacceptable datum",                           // DATUM
    "coordinates in unacceptable format",           // COORD
    "no \'F\' format record prior to data records", // NOFORM
    "unknown field, or required field missing",     // FIELD
    "a field had an invalid or out-of-range value", // VALUE
    "duplicate route number",                       // DUPRT
    "unknown waypoint ID"                           // UNKWPT
};

int gpsInfo( FILE *const outfile, const GpFile *filep );
int gpsDiscard( GpFile *filep, const char *which );
int gpsSort( GpFile *filep );
int gpsMerge( GpFile *filep, const char *const fnameB );

#endif
