/********
gpstool.c -- a tool for manipulating GPSU files

Eric Coutu
ID #0523365
********/

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 1000
#endif

#ifndef NDEBUG
#define LN { fprintf(stderr, "%s:%s:%d",__FILE__, __func__, __LINE__); }
#define PDEB(...) { LN; fprintf(stderr, ": " __VA_ARGS__); fprintf(stderr, "\n"); }
#define PLN { PDEB("%c", '\0'); }
#else
#define LN ;
#define PDEB(...) ;
#define PLN ;
#endif

#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)

#define BUFSIZE 1024

#include "gpstool.h"
#include "mystring.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <getopt.h>
#include <unistd.h>
#include <error.h>

typedef enum {
    MISSING = 0,
    EXTRA,
    COMPONENT,
    UNKNOWN,
    HELP,
    WRITE,
    EMPTYFILE,
    SORT
} errorCode;

char errorCodes[][48] = {
    "missing command argument",
    "too many arguments",
    "unrecognized component",
    "unrecognized option",
    "",
    "unable to write to file",
    "no data left to write",
    "failed sorting waypoints"
};

char *prog_name = NULL;
GpFile *gpfileA = NULL;

void cleanUp() {

    freeGpFile(gpfileA);
    free(gpfileA);
}

int perr(char *format, ...) {

    int rv = 0;
    va_list args;
    
    va_start(args,format);
    rv = vfprintf(stderr,format,args);
    va_end(args);
    
    return rv;
}

void disperr(errorCode err) {

    if (err != HELP)
        perr("%s: %s\n", prog_name, errorCodes[err]);
    if (err <= HELP)
        perr("Try \'%s -help\' for more information.\n", prog_name);
}

int main(int argc, char *argv[]) {

    static struct option options[] = {
        { "help",       no_argument,        0, 'h' },
        { "info",       no_argument,        0, 'i' },
        { "sortwp",     no_argument,        0, 's' },
        { "discard",    required_argument,  0, 'd' },
        { "keep",       required_argument,  0, 'k' },
        { "merge",      required_argument,  0, 'm' },
        #ifndef NDEBUG
        { "write",      no_argument,        0, 'w' },
        #endif
        { 0, 0, 0, 0 }
    };
    char buf[BUFSIZE];
    int index = 0;
    int command = getopt_long_only(argc, argv, "i:s:d:k:m:", options, &index);
    
    prog_name = argv[0];
    if (atexit (cleanUp) != 0)
        return EXIT_FAILURE;
    
    if (optind < argc) {
        disperr(EXTRA);
        return EXIT_FAILURE;
    }
    else if (command == -1) {
        disperr(MISSING);
        return EXIT_FAILURE;
    }
    else if (command == '?') {
        disperr(HELP);
        return EXIT_FAILURE;
    }
    else if (strcmp(argv[1] + 1, options[index].name) != 0) {
        disperr(UNKNOWN);
        return EXIT_FAILURE;
    }

    if (chrset(command, "dkm") == true)
        strcpy(buf, optarg);

    if (command == 'h') {
        printf("Usage: %s COMMAND\n"
               "A tool for manipulating GPSU formatted files.\n"
               "COMMAND is one of the following:\n"
               "  -d, -discard COMPONENT     remove specified component(s)\n"
               "  -k, -keep COMPONENT        remove all components except"
                                             " those specified\n"
               "  -s, -sortwp                sort waypoints by ID, if not"
                                             " already\n"
               "  -m, -merge FILE            combine data from input w/ FILE\n"
               "COMPONENT is one or more of the letters: (in any order)\n"
               "   w    designates waypoints (note that discarding waypoints"
                        " will also discard routes)\n"
               "   r    designates routes (note that keeping routes will also"
                        " keep waypoints)\n"
               "   t    designates trackpoints (note that discarding"
                        " trackpoints will also discard tracks)\n"
               "Note: when discarding/keeping components, there must be at"
               " least one component left in the file.\n"
               "Examples:\n"
               "  %s -discard w      discard waypoints and routes\n"
               "  %s -keep rt        discard routes and trackpoints\n"
               "  %s -discard wrt    leaves an empty file and is invalid\n",
               prog_name, prog_name, prog_name, prog_name);
        return EXIT_SUCCESS;
    }
    else {
        gpfileA = calloc(1, sizeof(GpFile));
        assert(gpfileA != NULL);
        GpStatus rv = readGpFile(stdin, gpfileA);
        if (rv.code != OK) {
            perr("Input error: line %d: %s\n", rv.lineno, codes[rv.code]);
            return EXIT_FAILURE;
        }
    }

    switch (command) {
        case 'w':
            break;
        case 'i':
            if (gpsInfo(stdout, gpfileA) == EXIT_FAILURE)
                return EXIT_FAILURE;
            break;
        case 's':
            if (gpsSort(gpfileA) == EXIT_FAILURE)
                return EXIT_FAILURE;
            break;
        case 'k': ;
            // components to discard, bitwise ORed: 111 = wrt
            char components = 0x7;
            for (int i = 0; i < strlen(buf); i++) {
                if (buf[i] == 'w') {
                    components &= ~0x4; // keep w: 0--
                }
                else if (buf[i] == 'r') {
                    components &= ~0x6; // keep r and w: 00-
                }
                else if (buf[i] == 't') {
                    components &= ~0x1; // keep t: --0
                }
                else {
                    disperr(COMPONENT);
                    return EXIT_FAILURE;
                }
            }
            int i = 0;
            if ((components & 0x4) == 0x4)          
                buf[i++] = 'w';
            if ((components & 0x2) == 0x2)
                buf[i++] = 'r';
            if ((components & 0x1) == 0x1)
                buf[i++] = 't';
            buf[i] = '\0';
        case 'd':
            if (gpsDiscard(gpfileA, buf) == EXIT_FAILURE)
                return EXIT_FAILURE;
            break;
        case 'm':
            if (gpsMerge(gpfileA, buf) == EXIT_FAILURE)
                return EXIT_FAILURE;
            break;
        default:
            disperr(HELP);
            return EXIT_FAILURE;
    }
    
    if (command != 'i') {
        int rv = writeGpFile(stdout, gpfileA);
        PDEB("writeGpFile returned %d", rv);
        if (rv == 0) {
            disperr(WRITE);
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

int gpsInfo( FILE *const outfile, const GpFile *filep ) {

    _Bool sorted = true;
    GpTrack *tp;
    int n_tracks = getGpTracks(filep, &tp);
    char buf[BUFSIZE];
    GpCoord NE = { -91, -181 };
    GpCoord SW = { 91, 181 };
    char *p;

    for (int i = 0; i < n_tracks; i++) {
        NE.lat = MAX(tp[i].NEcorner.lat, NE.lat);
        NE.lon = MAX(tp[i].NEcorner.lon, NE.lon);
        SW.lat = MIN(tp[i].SWcorner.lat, SW.lat);
        SW.lon = MIN(tp[i].SWcorner.lon, SW.lon);
    }
        
    if (n_tracks > 0)
        free(tp);

    for (int i = 0; i < filep->nwaypts; i++) {
        if (i > 0 && strcmp(filep->waypt[i].ID, filep->waypt[i-1].ID) < 0)
            sorted = false;
        NE.lat = MAX(filep->waypt[i].coord.lat, NE.lat);
        NE.lon = MAX(filep->waypt[i].coord.lon, NE.lon);
        SW.lat = MIN(filep->waypt[i].coord.lat, SW.lat);
        SW.lon = MIN(filep->waypt[i].coord.lon, SW.lon);
    }

    sprintf(buf, "Extent: SW %+lf %+lf to NE %+lf %+lf",
            SW.lon, SW.lat, NE.lon, NE.lat);

    p = strpbrk(buf, "+-");
    *p = (*p == '+') ? 'E' : 'W';
    p = strpbrk(buf, "+-");
    *p = (*p == '+') ? 'N' : 'S';
    p = strpbrk(buf, "+-");
    *p = (*p == '+') ? 'E' : 'W';
    p = strpbrk(buf, "+-");
    *p = (*p == '+') ? 'N' : 'S';

    if (fprintf(outfile,
                "%d waypoints%s\n%d routes\n%d trackpoints\n%d tracks\n%s\n",
                filep->nwaypts,
                (filep->nwaypts > 0) ?
                    ((sorted == true) ? " (sorted)" : " (not sorted)") : "",
                filep->nroutes, filep->ntrkpts, n_tracks, buf) < 0) {
        disperr(WRITE);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


int gpsDiscard( GpFile *filep, const char *which ) {

    int nwaypts = filep->nwaypts;
    int nroutes = filep->nroutes;
    int ntrkpts = filep->ntrkpts;
    
    for (int i = 0; i < strlen(which); i++) {
        if (which[i] == 'w') {
            nwaypts = -1;
            nroutes = -1;
        }
        else if (which[i] == 'r') {
            nroutes = -1;
        }
        else if (which[i] == 't') {
            ntrkpts = -1;
        }
        else {
            disperr(COMPONENT);
            return EXIT_FAILURE;
        }
    }

    if (nwaypts <= 0 && nroutes <= 0 && ntrkpts <= 0) {
        disperr(EMPTYFILE);
        return EXIT_SUCCESS;
    }
    
    if (nwaypts == -1) {
        freeGpWaypts(filep);
        filep->nwaypts = 0;
    }
    if (nroutes == -1) {
        freeGpRoutes(filep);
        filep->nroutes = 0;
    }
    if (ntrkpts == -1) {
        freeGpTrkpts(filep);
        filep->ntrkpts = 0;
    }
    return EXIT_SUCCESS;
}


int compGpWaypt(const void *wp1, const void  *wp2) {

    return strcmp(((GpWaypt*)wp1)->ID, ((GpWaypt*)wp2)->ID);
}


int gpsSort( GpFile *filep ) {

    char **legIDs[filep->nroutes];
    
    for (int i = 0; i < filep->nroutes; i++) {
        GpRoute *rp = *(filep->route + i);
        legIDs[i] = malloc(rp->npoints * sizeof(char *));
        assert(legIDs[i] != NULL);
        for (int j = 0; j < rp->npoints; j++)
            legIDs[i][j] = (filep->waypt + rp->leg[j])->ID;
    }
    qsort(filep->waypt, filep->nwaypts, sizeof(GpWaypt), compGpWaypt);
    
    for (int i = 0; i < filep->nroutes; i++) {
        GpRoute *rp = *(filep->route + i);
        for (int j = 0; j < rp->npoints; j++) {
            for (int k = 0; k < filep->nwaypts; k++) {
                if (strcmp(legIDs[i][j], filep->waypt[k].ID) == 0) {
                    rp->leg[j] = k;
                    break;
                }
            }
        }
        free(legIDs[i]);
    }

    return EXIT_SUCCESS;
}


int gpsMerge( GpFile *filep, const char *const fnameB ) {

    FILE *fp = fopen(fnameB, "r");
    if (fp == NULL) {
        perror(fnameB);
        return EXIT_SUCCESS;
    }

    GpFile filepB;
    GpStatus status = readGpFile(fp, &filepB);

    fclose(fp);
    if (status.code != OK) {
        perr("Input error: line %d: %s\n", status.lineno, codes[status.code]);
        return EXIT_FAILURE;
    }

    if (filepB.nroutes > 0) {
        int b_start = 1000;

        // increment each leg of each route by filep->nwaypts
        for (int i = 0; i < filepB.nroutes; i++) {
            for (int j = 0; j < (*(filepB.route + i))->npoints; j++)
                (*(filepB.route + i))->leg[j] += filep->nwaypts;
        }

        // determine next avaliable block of route numbers
        for (int i = 0; i < filepB.nroutes; i++) {
            int start = (*(filepB.route + i))->number;
            if (start < b_start)
                b_start = start;
        }

        for (int i = 0; i < filep->nroutes; i++) {
            int block = (*(filep->route + i))->number;
            if (b_start <= block)
                b_start = (b_start % 100) + ((block/100) + 1) * 100;
        }

        // renumber file B's routes
        for (int i = 0; i < filepB.nroutes; i++) {
            (*(filepB.route + i))->number = b_start++;
        }
        
        // resize file A's route array and copy file B's to it's end
        filep->route = realloc(filep->route, (filep->nroutes + filepB.nroutes) *
                                             sizeof(GpRoute *));
        assert(filep->route != NULL);
        memcpy(filep->route + filep->nroutes, filepB.route,
               filepB.nroutes * sizeof(GpRoute *));

        filep->nroutes += filepB.nroutes;
    }
    
    if (filepB.nwaypts > 0) {
        int id_len = 0, sym_len = 0;

        // resize file A's waypt array and copy file B's to it's end
        filep->waypt = realloc(filep->waypt, (filep->nwaypts + filepB.nwaypts) *
                                             sizeof(GpWaypt));
        assert(filep->waypt != NULL);
        memcpy(filep->waypt + filep->nwaypts, filepB.waypt,
               filepB.nwaypts * sizeof(GpWaypt));

        // determine new size of id and symbol fields
        for (int i = 0; i < filep->nwaypts + filepB.nwaypts; i++) {
            if (strlen(filep->waypt[i].ID) > id_len)
                id_len = strlen(filep->waypt[i].ID);
            if (strlen(filep->waypt[i].symbol) > sym_len)
                sym_len = strlen(filep->waypt[i].symbol);
        }

        // resize id and symbols of each waypt in file A and B
        for (int i = 0; i < filep->nwaypts + filepB.nwaypts; i++) {
            char buf[BUFSIZE];
            filep->waypt[i].ID = realloc(filep->waypt[i].ID,
                                         (id_len + 1 ) * sizeof(char));
            assert(filep->waypt[i].ID != NULL);
            strcpy(buf,filep->waypt[i].ID);
            sprintf(filep->waypt[i].ID, "%-*s", id_len, buf);

            filep->waypt[i].symbol = realloc(filep->waypt[i].symbol,
                                             (sym_len + 1) * sizeof(char));
            assert(filep->waypt[i].symbol != NULL);
            strcpy(buf,filep->waypt[i].symbol);
            sprintf(filep->waypt[i].symbol, "%-*s", sym_len, buf);
        }        

        // deal w/ duplicate id's
        for (int i = filep->nwaypts; i < filep->nwaypts + filepB.nwaypts; i++) {
            char n = '0';
            for (int j = 0; j < filep->nwaypts + filepB.nwaypts; j++) {
                if (i == j)
                    continue;
                if (strcmp(filep->waypt[i].ID, filep->waypt[j].ID) == 0) {
                    filep->waypt[i].ID[strlen(filep->waypt[i].ID) - 1] = n++;
                    j = 0;
                }
            }
        }
        filep->nwaypts += filepB.nwaypts;
    }

    if (filepB.ntrkpts > 0) {
        if (filepB.timeZone != filep->timeZone) {
            int dif = filep->timeZone - filepB.timeZone;
            for (int i = 0; i < filepB.ntrkpts; i++)
                filepB.trkpt[i].dateTime += dif * 3600;
        }
        if (filepB.unitHorz != filep->unitHorz) {
            double dist_fact = 1, speed_fact = 1;
            // determine factor to convert B's units to nautical miles
            switch(filepB.unitHorz) {
                case 'K':
                    dist_fact *= 1000;
                case 'M':
                    dist_fact /= 1852;
                    break;
                case 'S':
                    dist_fact *= 5280;
                case 'F':
                    dist_fact /= (2315000.0 / 381);
                    break;
                default:
                    break;
            }
            // factor for converting from nm to A's units
            switch(filep->unitHorz) {
                case 'K':
                    dist_fact /= 1000;
                case 'M':
                    dist_fact *= 1852;
                    break;
                case 'S':
                    dist_fact /= 5280;
                case 'F':
                    dist_fact *= (2315000.0 / 381);
                    break;
                default:
                    break;
            }
            // factor for converting from B to A's speed units
            speed_fact = dist_fact;
            if (filepB.unitTime == 'H')
                speed_fact /= 3600;
            if (filep->unitTime == 'H')
                speed_fact *= 3600;
            // adjust each trackpoint's speed and distance
            for (int i = 0; i < filepB.ntrkpts; i++) {
                filepB.trkpt[i].dist *= dist_fact;
                filepB.trkpt[i].speed *= speed_fact;
            }
        }
        // resize file A's trackpoint array and copy file B's to it's end
        filep->trkpt = realloc(filep->trkpt, (filep->ntrkpts + filepB.ntrkpts) *
                                              sizeof(GpTrkpt));
        assert(filep->trkpt != NULL);
        memcpy(filep->trkpt + filep->ntrkpts, filepB.trkpt,
               filepB.ntrkpts * sizeof(GpTrkpt));
        filep->ntrkpts += filepB.ntrkpts;
    }

    free(filepB.dateFormat);
    free(filepB.waypt);
    free(filepB.route);
    free(filepB.trkpt);
    
    return EXIT_SUCCESS;
}
