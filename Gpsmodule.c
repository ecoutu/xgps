/********
Gpsmodule.c -- Wrapper functions for Gps module

Eric Coutu
ID #0523365
********/

#define BUFSIZE 1024

#include <Python.h>
#include "gpstool.h"
#include "gputil.h"

/*** prototypes for C functions and wrapper functions ***/
static PyObject* Gps_readFile(PyObject *self, PyObject *args);
static PyObject* Gps_getData(PyObject *self, PyObject *args);
static PyObject* Gps_freeFile(PyObject *self, PyObject *args);

/*** method list to export to python ***/
static PyMethodDef gpsMethods[] = {
	{"readFile", Gps_readFile, METH_VARARGS},
	{"getData", Gps_getData, METH_VARARGS},
	{"freeFile", Gps_freeFile, METH_VARARGS},
	{NULL, NULL}, //denotes end of list
};

static GpFile filep;
static GpTrack *tracksp;
static int ntracks;

void initGps(void) {
    Py_InitModule("Gps", gpsMethods);
}


static PyObject* Gps_readFile(PyObject *self, PyObject *args) {
    const char *filename;

    if (PyArg_ParseTuple(args, "s", &filename)) {
        GpStatus status;

        FILE *fp = fopen(filename, "r");
        if (fp == NULL)
            return Py_BuildValue("s", strerror(errno));
        status = readGpFile(fp, &filep);
        fclose(fp);
        if (status.code != OK) {
            char errbuf[BUFSIZE] = "";
            sprintf(errbuf, "Unable to open %s: %s on line %d", filename,
                    codes[status.code], status.lineno);
            return Py_BuildValue("s", errbuf);
        }
        ntracks = getGpTracks(&filep, &tracksp);
    }
    return Py_BuildValue("s", "OK");
}


static PyObject* Gps_getData(PyObject *self, PyObject *args) {
    PyObject *waypts, *routes, *trkpts, *tracks;
    char tPlace[][3] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
    if (PyArg_ParseTuple(args, "OOOO", &waypts, &routes, &trkpts, &tracks) == 0)
        return NULL;
    for (int i = 0; i < filep.nwaypts; i++) {
        // (id, lat, lon, symbol, textChoice, textPlace, comment)
        GpWaypt cwaypt = filep.waypt[i];
        PyObject *waypt = Py_BuildValue("(sffscss)", cwaypt.ID,
                                        cwaypt.coord.lat, cwaypt.coord.lon,
                                        cwaypt.symbol, cwaypt.textChoice,
                                        tPlace[cwaypt.textPlace], cwaypt.comment);
        if (waypt == NULL)
            return NULL;
        if (PyList_Append(waypts, waypt) == -1)
            return NULL;
    }
    for (int i = 0; i < filep.nroutes; i++) {
        PyObject *legs = Py_BuildValue("[]");
        if (legs == NULL)
            return NULL;
        for (int j = 0; j < (*(filep.route + i))->npoints; j++) {
            PyObject *leg = Py_BuildValue("i", (*(filep.route + i))->leg[j]);
            if (leg == NULL || PyList_Append(legs, leg) == -1)
                return NULL;
        }
        PyObject *route = Py_BuildValue("[isO]", (*(filep.route + i))->number,
                                        (*(filep.route + i))->comment, legs);
        if (route == NULL || PyList_Append(routes, route) == -1)
            return NULL;
    }
    for (int i = 0; i < filep.ntrkpts; i++) {
        PyObject *trkpt = Py_BuildValue("(ff)", filep.trkpt[i].coord.lat,
                                        filep.trkpt[i].coord.lon);
        if (trkpt == NULL)
            return NULL;
        if (PyList_Append(trkpts, trkpt) == -1)
            return NULL;
    }

    for (int i = 0; i < ntracks; i++) {
        // (seqno, startTrk, duration, dist, speed)
        struct tm timebuf;
        char buf[BUFSIZE];
        localtime_r(&tracksp[i].startTrk, &timebuf);
        strftime(buf, BUFSIZE, filep.dateFormat, &timebuf);
        strftime(buf + strlen(buf), BUFSIZE, " %X", &timebuf);

        PyObject *track = Py_BuildValue("(isldf)", tracksp[i].seqno, buf,
                                        tracksp[i].duration, tracksp[i].dist,
                                        tracksp[i].speed);
        if (track == NULL)
            return NULL;
        if (PyList_Append(tracks, track) == -1)
            return NULL;
    }

    return Py_BuildValue("(cc)", filep.unitHorz, filep.unitTime);
}


static PyObject* Gps_freeFile(PyObject *self, PyObject *args) {
    GpTrack *tp = tracksp;
    tracksp = NULL;
    free(tp);
    freeGpFile(&filep);
    
    return Py_BuildValue("s", "OK");
}

