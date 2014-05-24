/********
gputil.c -- Utility functions for interpreting GPSU formatted files

Eric Coutu
ID #0523365
********/

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif

#define BUFSIZE 1024
#define MAX_FIELD_LENGTH 64
#define SPACE " \t"
#define COUNT count
#define LATLEN (int)strlen("N00.000000")
#define LONLEN (int)strlen("W000.000000")
#define TIMELEN strlen("hh:mm:ss")

#ifdef NDEBUG
#define PDEB fprintf(stderr, "%s:%s:%d\n",__FILE__, __func__, __LINE__);
#else
#define PDEB ;
#endif

#define GPRINT(...) { if (fprintf(gpf, __VA_ARGS__) < 0) return 0; }
#define GPRINTLN(...) {  GPRINT(__VA_ARGS__); COUNT++; }

#include "gputil.h"
#include "mystring.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>

/*  All the possible F line column types    */
typedef enum {
    ID = 0,    // ID
    LAT,       // latitude
    LON,       // longitude
    ALT,       // altitude
    SYM,       // symbol
    TXCHO,     // text choice
    TXPLA,     // text placement
    COMMENT,   // comment
    DATE,      // date
    TIME,      // time
    SEGFLAG,   // new segment
    DUR,       // duration
    SECONDS,   // seconds
    DIST,      // distance
    SPEED,     // speed
    OTHER,     // undefined
} GpFieldType;

typedef struct {
    GpFieldType type;   // data type of field
    int len;            // field length. 0 = not set, -1 = rest of line
} GpFieldHeader;


/*  Convert coord into a formatted string and store it in dst. The results will
    be stored left justified with latitude first and longitude second, padded
    with lat_len and lon_len spaces respectivley and seperated with a space.
    Parameters: dst is a buffer large enough to hold lat_len + lon_len + 1 chars
                lat_len determines padding for the latitude field
                lon_len determines padding for the longitude field
                coord contains the coordinates to process    */
void coordToStr(char *dst, GpCoord coord) {

    char *lon = dst + LATLEN + 1;
    sprintf(dst, "%+0*lf ", LATLEN, coord.lat);
    dst[0] = (dst[0] == '+') ? 'N' : 'S';

    sprintf(lon, "%+0*lf", LONLEN, coord.lon);
    lon[0] = (lon[0] == '+') ? 'E' : 'W';
}


/*  Parse fieldDef for field identifiers, storing their type and (optional)
    lengths in head. If a length is not specified, it will be 0, or if it is a
    comment, -1 will indicate the rest of the line.
    Paramaters: fieldDef is an F line scanned from a GPSU file
                head is an allocated array large enough to hold the fields
    Returns:    FIELD for an unrecognized header  */    
GpError parseGpFieldDef(const char *fieldDef, GpFieldHeader *head) {

    char buf[strlen(fieldDef) + 1];    
    strcpy(buf, fieldDef);
    char *p = strtok(buf, " \t");
    
    for (int i = 0; (p = strtok(NULL, " \t")) != NULL; i++) {
        head[i].len = 0;
        // Parse the field definition keyword, ignoring case
        if (strbeg_ic(p, "ID") == true) {
            head[i].type = ID;
            head[i].len = strlen(p);
        }
        else if (strcmp_ic(p, "Latitude") == 0) {
            head[i].type = LAT;
        }
        else if (strcmp_ic(p, "Longitude") == 0) {
            head[i].type = LON;
        }
        else if (strbeg_ic(p, "Alt") == true) {
            head[i].type = ALT;
            continue;
        }
        else if (strbeg_ic(p, "Symbol") == true) {
            head[i].type = SYM;
            head[i].len = strlen(p);
        }
        else if (strcmp_ic(p, "T") == 0) {
            head[i].type = TXCHO;
            head[i].len = 1;
        }
        else if (strcmp_ic(p, "O") == 0) {
            head[i].type = TXPLA;
        }
        else if (strcmp_ic(p, "Comment") == 0) {
            head[i].type = COMMENT;
            head[i].len = -1;
        }
        else if (strcmp_ic(p, "Date") == 0) {
          head[i].type = DATE;
        }
        else if (strcmp_ic(p, "Time") == 0) {
          head[i].type = TIME;
        }
        else if (strcmp_ic(p, "S") == 0) {
            head[i].type = SEGFLAG;
            head[i].len = 1;
        }
        else if (strcmp_ic(p, "Duration") == 0) {
            head[i].type = DUR;
        }
        else if (strcmp_ic(p, "seconds") == 0) {
            head[i].type = SECONDS;
        }
        else if ( (strcmp_ic(p, "km") == 0) || (strcmp_ic(p, "m") == 0)
                 || (strcmp_ic(p, "miles") == 0) || (strcmp_ic(p, "ft") == 0)
                 || (strcmp_ic(p, "nm") == 0) )  {
            head[i].type = DIST;
        }
        else if ( (strcmp_ic(p, "km/h") == 0) || (strcmp_ic(p, "m/s") == 0)
                 || (strcmp_ic(p, "mph") == 0)|| (strcmp_ic(p, "ft/s") == 0)
                 || (strcmp_ic(p, "knots") == 0) )  {
            head[i].type = SPEED;
        }        
        else {
            head[i].type = OTHER;
        }
        
        // no duplicate field types
        for (int j = 0; j < i; j++) {
            if (head[j].type == head[i].type)
                return FIELD;
        }
    }    
    return OK;
}


/*  Parse buf using head as a guide for field lengths, storing each sucessfully
    parsed field in results. On sucess, each element of results will corespond
    to a sucesfully parsed element from it's definition in head.
    Paramaters: buf contains line to parse
                results is an allocated array of n_fields strings
                head contains n_fields objects used to parse buf
    Returns:    VALUE if a line is not correctly partitioned    */
GpError parseGpLine(const char *buf, char (*results)[MAX_FIELD_LENGTH],
                    GpFieldHeader *head, int n_fields) {

    const char *start = buf + 1;
    for (int i = 0; i < n_fields; i++) {
        int len;

        // check for valid field seperator
        if ( (head[i].type != COMMENT) && (chrset(*start, SPACE) == false) )
            return FIELD;

        start += strspn(start, SPACE);  
        
        // SEGFLAG is set
        if (head[i].type == SEGFLAG) {
            if ( (*(start+1) != '\0') && (chrset(*(start+1), SPACE) == false) )
                return VALUE;
            
            if (*start == '1') {
                strcpy(results[i], start+2);
                return OK;
            }
            else if (*start == '0') {
                strcpy(results[i],"0");
                len = 1;
            }
            else {
                return VALUE;
            }
        }
        else if (head[i].len == 0) {
            len = strcspn(start, SPACE);
        }
        else if (head[i].len == -1) {
               len = strlen(start);
        }
        else {
            len = head[i].len;
        }
        sprintf(results[i],"%-*.*s",len,len,start);
        start += len;
    }
    
    if (buf + strlen(buf) > start + strspn(start, SPACE))
        return FIELD;

    return OK;
}


/*  Parse lat and lon and store the results in coord.
    Paramaters: lat and lon must not have any characters after their values
                coord is allocated and will store parsed results
    Returns:    VALUE if lat or lon are invalid format or out of range  */
GpError parseGpCoords(char *lat, char *lon, GpCoord *coord) {

    char *p = NULL;
    if (*lat == 'N')    // N is positive
        *lat = '+';
    else if (*lat == 'S')   // S is negative
        *lat = '-';
    coord->lat = strtod(lat, &p);
    if ( (*p != '\0') || (coord->lat > 90) || (coord->lat < -90) )
        return VALUE;   // lat is not valid
    
    p = NULL;
    if (*lon == 'E')    // E is positive
        *lon = '+';
    else if (*lon == 'W')   // W is negative
        *lon = '-';
    coord->lon = strtod(lon,&p);
    if ( (*p != '\0') || (coord->lon > 180) || (coord->lon < -180) )    
        return VALUE;   // lon is not valid

    return OK;
}


GpStatus readGpFile( FILE *const gpf, GpFile *filep ) { 
   
    char buf[BUFSIZE];
    char fieldDef[BUFSIZE] = "";
    _Bool isRoute = false;    
    GpStatus status;

    GpFile f = {
        newstr(GP_DATEFORMAT), GP_TIMEZONE, GP_UNITHORZ, GP_UNITTIME, 0, NULL, 
        0, NULL, 0, NULL
    };
    *filep = f;
    
    for (status.lineno = 1, status.code = OK; feof(gpf) == 0; status.lineno++) {
        char code;
        memset(buf,'\0',BUFSIZE);
        if (fgets(buf, BUFSIZE, gpf) == NULL)
            break;

        // validate first 2 bytes
        code = buf[0];        
        if (chrset(code, "CAH\n\r") == true) {
            continue;
        }
        else if (chrset(code, "ISMUFWRT") == false) {
            status.code = UNKREC;
            break;
        }
        else if (buf[1] != ' ') {
            status.code = BADSEP;
            break;
        }

        // remove EOL character
        if (strpbrk(buf, "\n\r") != NULL)
            *strpbrk(buf, "\n\r") = '\0';

        // check if we are continuing to scan a route
        if ( (isRoute == true) && (chrset(code, "FW") == false) )
            isRoute = false;

        // 'I' line must start w/ "GPSU"
        if ( (code == 'I') && (strcmp_ic(strtok(buf+1, SPACE), "GPSU") != 0) ) {
            status.code = FILTYP;
            break;
        }
        // 'M' line must contain WGS 84
        else if ( (code == 'M') && (strstr_ic(buf, "WGS 84") == NULL) ) {
            status.code = DATUM;
            break;
        }
        // 'U' line must contain "LAT LON DEG"
        else if ( (code == 'U') && (strstr_ic(buf, "LAT LON DEG") == NULL) ) {
            status.code = COORD;
            break;
        }        
        // 'F' line defines new field header
        else if (code == 'F') {
            strcpy(fieldDef, buf);
        }
        // 'S' line stores a setting
        else if (code == 'S') {
            char *setting = strtok((buf + 1), " \t=");
            // DateFormat setting
            if (strcmp_ic(setting, "DateFormat") == 0) {
                char code[3];
                for (int j = 0; j < 3; j++) {
                    char *p = strtok(NULL, "/");
                    if (p == NULL) {
                        freeGpFile(filep);
                        status.code = VALUE;
                        return status;
                    }
                    if (strcmp(p, "dd") == 0) {
                        code[j] = 'd';
                    }
                    else if (strcmp(p, "mm") == 0) {
                        code[j] = 'm';
                    }
                    else if (strcmp(p, "mmm") == 0) {
                        code[j] = 'b';
                    }
                    else if (strcmp(p, "yy") == 0) {
                        code[j] = 'y';
                    }
                    else if (strcmp(p, "yyyy") == 0) {
                        code[j] = 'Y';
                    }
                    else {
                        freeGpFile(filep);
                        status.code = VALUE;
                        return status;
                    }
                }
                sprintf(filep->dateFormat, "%%%c/%%%c/%%%c", code[0], code[1],
                        code[2]);
            }
            // TimeZone setting
            else if (strcmp_ic(setting, "TimeZone") == 0) {
                int m;
                if (sscanf(strtok(NULL, ""), "%d:%d", &(filep->timeZone), &m)
                    != 2) {
                    status.code = VALUE;
                    break;
                }
            }
            // Units setting
            else if (strcmp_ic(setting, "Units") == 0) {
                if (sscanf(strtok(NULL, ""), "%[MKFNS]",
                    &(filep->unitHorz)) != 1) {
                    status.code = VALUE;
                    break;
                }
                if (chrset(filep->unitHorz, "FM") == true)
                    filep->unitTime = 'S';
                else if (chrset(filep->unitHorz, "KNS") == true)
                    filep->unitTime = 'H';

               }
        }
        // 'W', 'R', 'T' lines require previous field declaration 
        else if ( (chrset(code, "WT") == true) && (strlen(fieldDef) == 0) ) {
            status.code = NOFORM;
            break;
        }
        // 'W' line is route leg
        else if ( (code == 'W') && (isRoute == true) ) {
            int n_legs = ++(*(filep->route + filep->nroutes - 1))->npoints;
            GpRoute *rp = realloc( *(filep->route + filep->nroutes - 1),
                                  sizeof(GpRoute) + (n_legs * sizeof(int)) );
            assert(rp != NULL);
			*(filep->route + filep->nroutes - 1) = rp;
            
            status.code = scanGpLeg(buf,fieldDef,filep->waypt,filep->nwaypts,
                                    *(filep->route + filep->nroutes - 1));
            if (status.code != OK)
                break;
        }
        // 'W' line is waypoint
        else if ( (code == 'W') && (isRoute == false) ) {
            filep->waypt = realloc(filep->waypt,
                                   (filep->nwaypts + 1)*sizeof(GpWaypt));
            assert(filep->waypt != NULL);
            status.code = scanGpWaypt(buf,fieldDef,
                                      filep->waypt + filep->nwaypts);
            if (status.code != OK)
                break;
            filep->nwaypts++;
        }
        // 'R' line starts new route
        else if (code == 'R') {
            filep->route = realloc(filep->route,
                                   (filep->nroutes + 1)*sizeof(GpRoute *));
            assert(filep->route != NULL);
            *(filep->route + filep->nroutes) = malloc(sizeof(GpRoute));
            assert(*(filep->route + filep->nroutes) != NULL);
            status.code = scanGpRoute(buf, *(filep->route + filep->nroutes));
            if (status.code != OK)
                break;
            filep->nroutes++;
            // check for duplicate routes
            for (int i = 0; i < (filep->nroutes - 1); i++) {
                if ( (*(filep->route + i))->number == (*(filep->route
                        + filep->nroutes - 1))->number ) {
                    freeGpFile(filep);
                    status.code = DUPRT;
                    return status;
                }
            }
            isRoute = true;
        }
        // scan new trackpoint
        else if (code == 'T') {
            filep->trkpt = realloc(filep->trkpt,
                                   (filep->ntrkpts + 1) * sizeof(GpTrkpt));
            assert(filep->trkpt != NULL);
            status.code = scanGpTrkpt(buf, fieldDef, filep->dateFormat,
                                      filep->trkpt + filep->ntrkpts);
            if (status.code != OK)
                break;
            filep->ntrkpts++;
        }
    }
    // free memory if an error occured
    if (ferror(gpf) != 0)
        status.code = IOERR;
    if (status.code != OK)
        freeGpFile(filep);

    return status;
}

void freep(void **p) {
    if (p == NULL || *p == NULL)
        return;
    void *t = *p;
    *p = NULL;
    free(t);
}

void freeGpRoutes(GpFile *filep) {

    if (filep->route == NULL)
        return;

    for (int i = 0; i < filep->nroutes; i++) {
        freep((void **)&(*(filep->route + i))->comment);
        freep((void **)(filep->route + i));
    }
    freep((void **)&filep->route);
}


void freeGpWaypts(GpFile *filep) {

    if (filep->waypt == NULL)
        return;

    for (int i = 0; i < filep->nwaypts; i++) {
        freep((void **)&(filep->waypt + i)->ID);
        freep((void **)&(filep->waypt + i)->symbol);
        freep((void **)&(filep->waypt + i)->comment);
    }
    freep((void **)&filep->waypt);
}


void freeGpTrkpts(GpFile *filep) {

    if (filep->trkpt == NULL)
        return;

    for (int i = 0; i < filep->ntrkpts; i++)
        freep((void **)&(filep->trkpt + i)->comment);

    freep((void **)&filep->trkpt);
}


void freeGpFile( GpFile *filep ) {

    if (filep == NULL)
        return;

    freep((void **)&filep->dateFormat);
    freeGpWaypts(filep);
    freeGpRoutes(filep);
    freeGpTrkpts(filep);   
}


GpError scanGpWaypt( const char *buff, const char *fieldDef, GpWaypt *wp ) {

    int n_fields = str_count_toks(fieldDef, " \t") - 1;
    GpFieldHeader head[n_fields];
    char fields[n_fields][MAX_FIELD_LENGTH];
    char *id = NULL, *lat = NULL, *lon = NULL;
    char *symbol = NULL, *comment = NULL;
    GpError err = OK;
    
    wp->textChoice = 'I';
    wp->textPlace = 2;
    
    if (strspn(buff+1, SPACE) == strlen(buff+1))
        return FIELD;
    
    if ( (err = parseGpFieldDef(fieldDef, head)) != OK)
        return err;
    
    if ( (err = parseGpLine(buff, fields, head, n_fields)) != OK)
        return err;
    
    for (int i = 0; i < n_fields; i++) {
        switch(head[i].type) {
            case ID:
                id = fields[i];
                break;
            case LAT:
                lat = fields[i];
                break;
            case LON:
                lon = fields[i];
                break;
            case ALT:
                break;
            case SYM:
                symbol = fields[i];
                break;
            case TXCHO:
                if (chrset(*fields[i], "-IC&+^") == true)
                    wp->textChoice = *fields[i];
                else
                    return VALUE;
                break;
            case TXPLA:
                for (int j = 0; j <= 8; j++) {
                    char textPlace[][3] = { "N","NE","E","SE",
                                             "S","SW","W","NW" };
                    if (j == 8) {
                        return VALUE;
                    }
                    else if (strcmp(textPlace[j],fields[i]) == 0) {
                        wp->textPlace = j;
                        break;
                    }
                }
                break;
            case COMMENT:
                comment = fields[i];
                break;
            default:
                return FIELD;
        }
    }

    if ( (id == NULL) || (lat == NULL) || (lon == NULL) )
        return FIELD;
    
    if (parseGpCoords(lat, lon, &(wp->coord)) != OK)
        return VALUE;

    // parsing succeeded, allocate memory for id, symbol and comment
    wp->ID = newstr(id);
    wp->symbol = ( symbol == NULL ? newstr("") : newstr(symbol) );
    wp->comment = ( comment == NULL ? newstr("") : newstr(comment) );

    return OK;
}


GpError scanGpRoute( const char *buff, GpRoute *rp ) {

    char *comment;
    
    if (strspn(buff+1, SPACE) == strlen(buff+1))
        return FIELD;
    
    rp->number = (int)strtol(buff+1, &comment, 10);
    if ( (rp->number < 0) || ((buff+1) == comment) )
        return VALUE;
    else if ( (*comment != '\0') && (chrset(*comment, SPACE) == false) )
        return VALUE;

    comment += strspn(comment, SPACE);
    rp->comment = newstr(comment);
    rp->npoints = 0;
    return OK;
}


GpError scanGpLeg(const char *buff, const char *fieldDef, const GpWaypt *wp,
                  const int nwp, GpRoute *rp) {

    int n_fields = str_count_toks(fieldDef, SPACE) - 1;
    char *id = NULL;
    GpFieldHeader head[n_fields];    
    char fields[n_fields][MAX_FIELD_LENGTH];
    GpError err = OK;

    if (strspn(buff+1, SPACE) == strlen(buff+1))
        return FIELD;

    if ( (err = parseGpFieldDef(fieldDef, head)) != OK)
        return err;
    
    if ( (err = parseGpLine(buff, fields, head, n_fields)) != OK)
        return err;

    for (int i = 0; i < n_fields; i++) {
        if (head[i].type == ID) {
            id = fields[i];
            break;
        }
    }
    
    if (id == NULL)
        return FIELD;
        
    for (int i = 0; i < nwp; i++) {
        if (strcmp((wp + i)->ID, id) == 0) {
            rp->leg[rp->npoints - 1] = i;
            return OK;
        }
    }    
    
    return UNKWPT;
}


GpError scanGpTrkpt( const char *buff, const char *fieldDef,
                    const char *dateFormat, GpTrkpt *tp ) {

    int n_fields = str_count_toks(fieldDef, SPACE) - 1;
    GpFieldHeader head[n_fields];
    char fields[n_fields][MAX_FIELD_LENGTH];

    char *lat = NULL, *lon = NULL, *comment = NULL;
    char dateBuf[MAX_FIELD_LENGTH] = "";
    char dateFormBuf[MAX_FIELD_LENGTH] = "";
    int validFields = 0;
    struct tm tm;
    GpError err = OK;

    memset(&tm,0,sizeof(struct tm));
    memset(tp,0,sizeof(GpTrkpt));

    if ( (err = parseGpFieldDef(fieldDef, head)) != OK)
        return err;
    
    if ( (err = parseGpLine(buff, fields, head, n_fields)) != OK)
        return err;

    for (int i = 0; i < n_fields; i++) {
        GpFieldType t = head[i].type;

        if (t == LAT) {
            lat = fields[i];
        }
        else if (t == LON) {
            lon = fields[i];
        }
        else if (t == ALT) {
            continue;
        }
        else if (t == DATE) {
            strcat(dateBuf, fields[i]);
            strcat(dateFormBuf, dateFormat);
        }
        else if (t == TIME) {
            strcat(dateBuf, fields[i]);
            strcat(dateFormBuf, " %H:%M:%S ");
        }
        else if (t == SEGFLAG) {
            if (strcmp(fields[i],"0") == 0) {
                tp->segFlag = false;
            }
            else {
                comment = fields[i];
                tp->segFlag = true;
                break;
            }
        }
        else if (t == SECONDS) {
            char *p;
            tp->duration = strtol(fields[i], &p, 10);
            if ( (p == NULL) || (*p != '\0') )
                return VALUE;
        }
        else if (t == DUR) {
            int h, m, s;
            if (sscanf(fields[i], "%d:%d:%d", &h, &m, &s) != 3)
                return VALUE;            
            if ( (h >= 24) || (h < 0) )
                return VALUE;
            else
                tp->duration = h * 60 * 60;
                
            if ( (m >= 60) || (m < 0) )
                return VALUE;
            else
                tp->duration += m * 60;
                
            if ( (s >= 60) || (s < 0) )
                return VALUE;
            else
                tp->duration += s;
        }
        else if (t == DIST) {
            char *p = NULL;
            tp->dist = strtod(fields[i], &p);
            if (*p != '\0')
                return VALUE;
        }
        else if (t == SPEED) {
            char *p = NULL;
            tp->speed = (float)strtod(fields[i], &p);
            if (*p != '\0')
                return VALUE;
        }
        else {
            return FIELD;
        }
        validFields++;
    }
    
    if ( (tp->segFlag == false) && (validFields < 8) )
        return FIELD;

    if ( (lat != NULL) && (lon != NULL ) &&
         (parseGpCoords(lat, lon, &(tp->coord)) != OK) )
        return VALUE;

    char *p = strptime(dateBuf, dateFormBuf, &tm);
    if ( (p == NULL) || (*p != '\0') ) {
        return VALUE;
    }
    else {
        tm.tm_isdst = -1;
        tp->dateTime = mktime(&tm);
        if (tp->dateTime == -1)
            return VALUE;
    }
    tp->comment = newstr(comment);
    
    return OK;
}


int getGpTracks( const GpFile *filep, GpTrack **tp ) {

    int n_tracks = 0;
    *tp = NULL;
    GpCoord NE = {-91, -181}, SW = {91, 181};
    
    for (int i = 0; i < filep->ntrkpts; i++) {
        GpTrkpt *cur_tp = filep->trkpt + i;
        if (cur_tp->segFlag == false || n_tracks == 0) {
            if ((cur_tp->coord).lat > NE.lat)
                NE.lat = (cur_tp->coord).lat;
            if ((cur_tp->coord).lat < SW.lat)
                SW.lat = (cur_tp->coord).lat;
            if ((cur_tp->coord).lon > NE.lon)
                NE.lon = ((cur_tp)->coord).lon;
            if ((cur_tp->coord).lon < SW.lon)
                SW.lon = ((cur_tp)->coord).lon;
        }
        // end of segment
        if ( ( (cur_tp->segFlag == true) && (n_tracks > 0) )
            || (i == (filep->ntrkpts - 1))) {
            GpTrkpt *prev_tp = filep->trkpt + i;
            if (cur_tp->segFlag == true)
                prev_tp--;
            (*tp + n_tracks - 1)->endTrk = prev_tp->dateTime;
            (*tp + n_tracks - 1)->duration = prev_tp->duration;
            (*tp + n_tracks - 1)->dist = prev_tp->dist;
            (*tp + n_tracks - 1)->speed = prev_tp->dist / prev_tp->duration;
            if (filep->unitTime == 'H')
                (*tp + n_tracks - 1)->speed *= 3600;
            ((*tp + n_tracks - 1)->meanCoord).lat = (NE.lat + SW.lat) / 2;
            ((*tp + n_tracks - 1)->meanCoord).lon = (NE.lon + SW.lon) / 2;
            (*tp + n_tracks - 1)->NEcorner = NE;
            (*tp + n_tracks - 1)->SWcorner = SW;
        }
        // start of new segment
        if (cur_tp->segFlag == true) {
            *tp = realloc(*tp, ((n_tracks + 1) * sizeof (GpTrack)));
            assert(*tp != NULL);
            (*tp + n_tracks)->seqno = i+1;
            (*tp + n_tracks)->startTrk = cur_tp->dateTime;
            NE = SW = cur_tp->coord;
            n_tracks++;
        }
        // check NE and SW bounds
        else {

        }
    }    
    return n_tracks;
}


int writeGpFile( FILE *const gpf, const GpFile *filep ) {

    char buf[BUFSIZE] = "";
    int COUNT = 1;
    int id_len = 0, sym_len = 0, com_len = 0;

    // file header, I, S, M and U lines
    GPRINTLN("H  SOFTWARE NAME & VERSION\n");
    GPRINTLN("I  GPSU 4.20 01 FREEWARE VERSION\n");
    GPRINTLN("\n");

    sscanf(filep->dateFormat, "%%%*c/%%%c/%%%c", buf, buf + 1);
    GPRINTLN("S DateFormat=dd/%s/%s\n", (buf[0] == 'm') ? "mm" : "mmm",
            (buf[1] == 'y') ? "yy" : "yyyy");
    GPRINTLN("S Timezone=%+d:00\n", filep->timeZone);
    GPRINTLN("S Units=%c\n", filep->unitHorz);
    GPRINTLN("\n");
    
    GPRINTLN("H R DATUM\n");
    GPRINTLN("M E            WGS 84 100  0.0000000E+00  0.0000000E+00 0 0 0\n");
    GPRINTLN("\n");
    
    GPRINTLN("H  COORDINATE SYSTEM\n");
    GPRINTLN("U  LAT LON DEG\n");

    // write waypoints, if any
    if (filep->nwaypts > 0) {
        GPRINTLN("\n");
        // determine waypoint F line column sizes
        for (int i = 0; i < filep->nwaypts; i++) {
            if (strlen(filep->waypt[i].ID) > id_len)
                id_len = strlen(filep->waypt[i].ID);
            if (strlen(filep->waypt[i].symbol) > sym_len)
                sym_len = strlen(filep->waypt[i].symbol);
            if (strlen(filep->waypt[i].comment) > com_len)
                com_len = strlen(filep->waypt[i].comment);
        }
        
        // print waypoint F line
        GPRINT("F ID");
        for (int i = strlen("ID"); i < id_len; i++)
            GPRINT("-");
        GPRINT(" %-*s %-*s T O  ", LATLEN, "Latitude", LONLEN, "Longitude");
        if (sym_len > 0) {
            GPRINT("Symbol");
            for (int i = strlen("Symbol"); i < sym_len; i++)
                GPRINT("-");
        }
        if (com_len > 0)
            GPRINT(" Comment");
        GPRINTLN("\n");

        // print waypoints
        for (int i = 0; i < filep->nwaypts; i++) {
            char textPlace[][3] ={ "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
            GpWaypt wp = filep->waypt[i];
            GPRINT("W %-*s ", id_len, wp.ID);
            
            coordToStr(buf, wp.coord);
            GPRINT("%s %c %-2s", buf, wp.textChoice, textPlace[wp.textPlace]);
            
            if (sym_len > 0)
                GPRINT(" %-*s", sym_len, wp.symbol);

            if (com_len > 0)
                GPRINT(" %s", wp.comment);
            GPRINTLN("\n");
        }    
    }
    // write routes, if any
    for (int i = 0; i < filep->nroutes; i++) {
        GpRoute *rp = *(filep->route + i);
        id_len = 0;
        GPRINTLN("\n");
        // determine routes F line column sizes
        for (int j = 0; j < rp->npoints; j++) {
            if (strlen(filep->waypt[rp->leg[j]].ID) > id_len)
                id_len = strlen(filep->waypt[rp->leg[j]].ID);
        }
        // print R line
        GPRINTLN("R %.2d %s\n", rp->number, rp->comment);
        // print routes F line
        GPRINT("F ID");
        for (int i = strlen("ID"); i < id_len; i++)
            GPRINT("-");

        GPRINTLN("\n");
        
        // print route waypoints
        for (int j = 0; j < rp->npoints; j++)
            GPRINTLN("W %-*s\n", id_len, filep->waypt[rp->leg[j]].ID);
    }

    // write track info and trackpoints, if any
    if (filep->ntrkpts > 0) {
        GpTrack *tp;
        int n_tracks = getGpTracks(filep, &tp);
        char dist_units[][8] = {
            ['M'] = "m", ['K'] = "km", ['F'] = "ft", ['N'] = "nm",
            ['S'] = "miles" 
        };
        char speed_units[][8] = {
            ['M'] = "m/s", ['K'] = "km/h", ['F'] = "ft/s", ['N'] = "knots",
            ['S'] = "mph"
        };
        GPRINTLN("\n");
        
        // determine track header column sizes
        struct tm timebuf;
        int dist_len = strlen(dist_units[(int)filep->unitHorz]);
        int speed_len = strlen(speed_units[(int)filep->unitHorz]);
        localtime_r(&filep->trkpt->dateTime, &timebuf);
        int date_len = strftime(buf, MAX_FIELD_LENGTH, filep->dateFormat, &timebuf);

        for (int i = 0; i < n_tracks; i++) {
            sprintf(buf, "%lf", (tp + i)->dist);
            if (strlen(buf) > dist_len)
                dist_len = strlen(buf);
            sprintf(buf, "%f", (tp + i)->speed);
            if (strlen(buf) > speed_len)
                speed_len = strlen(buf);
        }
        
        // print track header column definitions
        GPRINTLN("H    Track    Pnts. %-*s Time     StopTime Duration %*s %*s\n",
                date_len, "Date", dist_len, dist_units[(int)filep->unitHorz],
                speed_len, speed_units[(int)filep->unitHorz]);
        
        // print track H line information
        for (int i = 0; i < n_tracks; i++) {
            int npts = (i < n_tracks - 1) 
                       ? (tp+i+1)->seqno - (tp+i)->seqno - 1
                       : filep->ntrkpts - (tp+i)->seqno;

            if (localtime_r(&(tp + i)->startTrk, &timebuf) == NULL
                || strftime(buf, date_len + 1, filep->dateFormat,
                            &timebuf) == 0
                || strftime(buf + strlen(buf), TIMELEN + 2, " %X",
                            &timebuf) == 0
                || localtime_r(&(tp + i)->endTrk, &timebuf) == NULL
                || strftime(buf + strlen(buf), TIMELEN + 2, " %X",
                            &timebuf) == 0) {
                freep((void **)&tp);
                return 0;
            }
            GPRINTLN("H %8d %8d %s %02d:%02d:%02d %*lf %*f\n",
                    (tp+i)->seqno, npts, buf, (int)(tp+i)->duration / 3600 % 60,
                    (int)(tp+i)->duration / 60 % 60, (int)(tp+i)->duration % 60,
                    dist_len, (tp+i)->dist, speed_len, (tp+i)->speed);
        }        
        GPRINTLN("\n");
        freep((void **)&tp);

        // determine trackpoint F line column sizes
        dist_len = strlen(dist_units[(int)filep->unitHorz]);
        speed_len = strlen(speed_units[(int)filep->unitHorz]);
        for (int i = 0; i < filep->ntrkpts; i++) {
            sprintf(buf, "%lf", filep->trkpt[i].dist);
            if (strlen(buf) > dist_len)
                dist_len = strlen(buf);
            sprintf(buf, "%f", filep->trkpt[i].speed);
            if (strlen(buf) > speed_len)
                speed_len = strlen(buf);
        }

        // print trackpoint F line
        GPRINTLN("F %-*s %-*s %-*s %-8s S %-8s %*s %*s\n",
                LATLEN, "Latitude", LONLEN, "Longitude", date_len, "Date",
                "Time", "Duration", dist_len, dist_units[(int)filep->unitHorz],
                speed_len, speed_units[(int)filep->unitHorz]);
        // print trackpoints
        for (int i = 0; i < filep->ntrkpts; i++) {
            GpTrkpt tp = filep->trkpt[i];

            coordToStr(buf, tp.coord);
            strcat(buf, " ");
            if ( (localtime_r(&tp.dateTime, &timebuf) == NULL)
                 || (strftime(buf + strlen(buf), date_len + 1, filep->dateFormat,
                                &timebuf) == 0)
                 || (strftime(buf + strlen(buf), TIMELEN + 2, " %X",
                     &timebuf) == 0) )
                return 0;

            GPRINT("T %s", buf);

            if (tp.segFlag == true) {
                GPRINTLN(" 1 %s\n", tp.comment);
            }
            else {
                GPRINTLN(" 0 %02d:%02d:%02d %*lf %*f\n",
                        (int)tp.duration / 3600 % 60,
                        (int)tp.duration / 60 % 60, (int)tp.duration % 60,
                        dist_len, tp.dist, speed_len, tp.speed);
            }
        }        
    }
    return COUNT;
}

