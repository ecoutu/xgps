/********
gputil.h -- header file for gputil.c in Asmt 2
Last updated:  January 28, 2010 03:42:46 PM       

Eric Coutu
0523365
********/

#ifndef GPUTIL_H_
#define GPUTIL_H_ A2

#include <stdio.h>
#include <time.h>

typedef struct {    // lat-long coordinate, assume WGS 84 datum
    double lat,lon;     // fractional degrees, S & W < 0
} GpCoord;

/* data structures for GPS file in memory */

typedef struct {    // waypoint
    char *ID;           // identifier
    GpCoord coord;      // coordinate
    char *symbol;       // symbol used to display waypoint (default "")
    char textChoice;    // {-,I,C,&,+,^} text to display with waypoint (def. I)
    short textPlace;    // {0=N,1=NE,2=E..7=NW} placement of text (default E)
    char *comment;      // default ""
} GpWaypt;

typedef struct {    // route
    int number;         // route no.
    char *comment;      // default ""
    int npoints;        // no. of waypoints in route (=size of fol'g array)
    int leg[];          // subscripts in array of waypoints
} GpRoute;

typedef struct {    // trackpoint
    GpCoord coord;      // coordinate
    time_t dateTime;    // date & time at position (sec.)
    _Bool segFlag;
        // for segFlag=true, start new segment
    char *comment;      // default "" (NULL if segFlag=false)
        // for segFlag=false, continuing track
    float speed;        // average speed from previous point
    double dist;        // distance from previous point
    long duration;      // elapsed time since start of segment (sec.)
} GpTrkpt;

typedef struct {    // track
    int seqno;          // sequence no. (=trackpt subscript+1)
    time_t startTrk,endTrk;  // date & time of track start & end
    long duration;      // total duration of track (sec.)
    double dist;        // total distance
    float speed;        // average speed over track
    GpCoord NEcorner;   // farthest NE position in track
    GpCoord SWcorner;   // farthest SW position
    GpCoord meanCoord;  // mean coordinate (center of rectangle)
} GpTrack;

/* defaults to initialize settings in GpFile */
#define GP_DATEFORMAT "%d/%m/%y"    // Canadian convention
#define GP_TIMEZONE (-5)            // Eastern time
#define GP_UNITHORZ 'K'             // km.
#define GP_UNITTIME 'H'             // hour

typedef struct {    // GPSU file

    // settings
    char *dateFormat;   // arg to strftime to display date only
    int timeZone;       // relative to UTC
    char unitHorz;      // {M,K,F,N,S} horizontal unit
                        //  Metres, Kilometres, Feet, Nautical Mi., Statute Mi.
    char unitTime;      // {S,H} implied: F,M->Sec; K,N,S->Hour
    
    // no. of objects & pointer to array
    int nwaypts;
    GpWaypt *waypt;
    
    int nroutes;
    GpRoute **route;    // array of pointers to GpRoute
    
    int ntrkpts;
    GpTrkpt *trkpt;
} GpFile;


/* General status return from functions */

typedef enum { OK=0,
    IOERR,  // I/O error
    UNKREC, // unknown record type
    BADSEP, // bad field separator, only blank allowed
    FILTYP, // unacceptable file type
    DATUM,  // unacceptable datum
    COORD,  // coordinates in unacceptable format
    NOFORM, // no 'F' format record prior to data records
    FIELD,  // unknown field, or required field missing
    VALUE,  // a field had an invalid or out-of-range value
    DUPRT,  // duplicate route no.
    UNKWPT, // unknown waypoint ID
} GpError;
    
typedef struct {
    GpError code;   // error code
    int lineno;     // line no. of input file where error occurred
} GpStatus;    


/* File I/O functions */

GpStatus readGpFile( FILE *const gpf, GpFile *filep );
GpError scanGpWaypt( const char *buff, const char *fieldDef, GpWaypt *wp );
GpError scanGpRoute( const char *buff, GpRoute *rp );
GpError scanGpLeg( const char *buff, const char *fieldDef, const GpWaypt *wp,
    const int nwp, GpRoute *rp );
GpError scanGpTrkpt( const char *buff, const char *fieldDef,
    const char *dateFormat, GpTrkpt *tp );
void freeGpFile( GpFile *filep );
void freeGpRoutes(GpFile *filep);
void freeGpWaypts(GpFile *filep);
void freeGpTrkpts(GpFile *filep);
int writeGpFile( FILE *const gpf, const GpFile *filep );


/* File interpretation functions */

int getGpTracks( const GpFile *filep, GpTrack **tp );

#endif
