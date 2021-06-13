/*
 * Copyright 2017-2018, 2020-2021 Uber Technologies, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>

#include "algos.h"
#include "constants.h"
#include "h3Index.h"
#include "latLng.h"
#include "test.h"
#include "utility.h"

// Fixtures
static LatLng sfVerts[] = {
    {0.659966917655, -2.1364398519396},  {0.6595011102219, -2.1359434279405},
    {0.6583348114025, -2.1354884206045}, {0.6581220034068, -2.1382437718946},
    {0.6594479998527, -2.1384597563896}, {0.6599990002976, -2.1376771158464}};
static GeoLoop sfGeoLoop = {.numVerts = 6, .verts = sfVerts};
static GeoPolygon sfGeoPolygon;

static LatLng holeVerts[] = {{0.6595072188743, -2.1371053983433},
                             {0.6591482046471, -2.1373141048153},
                             {0.6592295020837, -2.1365222838402}};
static GeoLoop holeGeoLoop = {.numVerts = 3, .verts = holeVerts};
static GeoPolygon holeGeoPolygon;

static LatLng emptyVerts[] = {{0.659966917655, -2.1364398519394},
                              {0.659966917655, -2.1364398519395},
                              {0.659966917655, -2.1364398519396}};
static GeoLoop emptyGeoLoop = {.numVerts = 3, .verts = emptyVerts};
static GeoPolygon emptyGeoPolygon;

/**
 * Return true if the cell crosses the meridian.
 */
static bool isTransmeridianCell(H3Index h) {
    CellBoundary bndry;
    H3_EXPORT(cellToBoundary)(h, &bndry);

    double minLng = M_PI, maxLng = -M_PI;
    for (int i = 0; i < bndry.numVerts; i++) {
        if (bndry.verts[i].lng < minLng) minLng = bndry.verts[i].lng;
        if (bndry.verts[i].lng > maxLng) maxLng = bndry.verts[i].lng;
    }

    return maxLng - minLng > M_PI - (M_PI / 4);
}

static void fillIndex_assertions(H3Index h) {
    if (isTransmeridianCell(h)) {
        // TODO: these do not work correctly
        return;
    }

    int currentRes = H3_EXPORT(getResolution)(h);
    // TODO: Not testing more than one depth because the assertions fail.
    for (int nextRes = currentRes; nextRes <= currentRes + 1; nextRes++) {
        CellBoundary bndry;
        H3_EXPORT(cellToBoundary)(h, &bndry);
        GeoPolygon polygon = {
            .geoloop = {.numVerts = bndry.numVerts, .verts = bndry.verts},
            .numHoles = 0,
            .holes = 0};

        int polygonToCellsSize =
            H3_EXPORT(maxPolygonToCellsSize)(&polygon, nextRes);
        H3Index *polygonToCellsOut =
            calloc(polygonToCellsSize, sizeof(H3Index));
        H3_EXPORT(polygonToCells)(&polygon, nextRes, polygonToCellsOut);

        int polygonToCellsCount =
            countNonNullIndexes(polygonToCellsOut, polygonToCellsSize);

        int64_t childrenSize = H3_EXPORT(cellToChildrenSize)(h, nextRes);
        H3Index *children = calloc(childrenSize, sizeof(H3Index));
        H3_EXPORT(cellToChildren)(h, nextRes, children);

        int cellToChildrenCount = countNonNullIndexes(children, childrenSize);

        t_assert(polygonToCellsCount == cellToChildrenCount,
                 "PolygonToCells count matches cellToChildren count");

        for (int i = 0; i < childrenSize; i++) {
            bool found = false;
            if (children[i] == H3_NULL) continue;
            for (int j = 0; j < polygonToCellsSize; j++) {
                if (polygonToCellsOut[j] == children[i]) {
                    found = true;
                    break;
                }
            }
            t_assert(
                found,
                "All indexes match between polygonToCells and cellToChildren");
        }

        free(polygonToCellsOut);
        free(children);
    }
}

SUITE(polygonToCells) {
    sfGeoPolygon.geoloop = sfGeoLoop;
    sfGeoPolygon.numHoles = 0;

    holeGeoPolygon.geoloop = sfGeoLoop;
    holeGeoPolygon.numHoles = 1;
    holeGeoPolygon.holes = &holeGeoLoop;

    emptyGeoPolygon.geoloop = emptyGeoLoop;
    emptyGeoPolygon.numHoles = 0;

    TEST(maxPolygonToCellsSize) {
        int numHexagons = H3_EXPORT(maxPolygonToCellsSize)(&sfGeoPolygon, 9);
        t_assert(numHexagons == 5613, "got expected max polygonToCells size");

        numHexagons = H3_EXPORT(maxPolygonToCellsSize)(&holeGeoPolygon, 9);
        t_assert(numHexagons == 5613,
                 "got expected max polygonToCells size (hole)");

        numHexagons = H3_EXPORT(maxPolygonToCellsSize)(&emptyGeoPolygon, 9);
        t_assert(numHexagons == 15,
                 "got expected max polygonToCells size (empty)");
    }

    TEST(polygonToCells) {
        int numHexagons = H3_EXPORT(maxPolygonToCellsSize)(&sfGeoPolygon, 9);
        H3Index *hexagons = calloc(numHexagons, sizeof(H3Index));

        H3_EXPORT(polygonToCells)(&sfGeoPolygon, 9, hexagons);
        int actualNumIndexes = countNonNullIndexes(hexagons, numHexagons);

        t_assert(actualNumIndexes == 1253, "got expected polygonToCells size");
        free(hexagons);
    }

    TEST(polygonToCellsHole) {
        int numHexagons = H3_EXPORT(maxPolygonToCellsSize)(&holeGeoPolygon, 9);
        H3Index *hexagons = calloc(numHexagons, sizeof(H3Index));

        H3_EXPORT(polygonToCells)(&holeGeoPolygon, 9, hexagons);
        int actualNumIndexes = countNonNullIndexes(hexagons, numHexagons);

        t_assert(actualNumIndexes == 1214,
                 "got expected polygonToCells size (hole)");
        free(hexagons);
    }

    TEST(polygonToCellsEmpty) {
        int numHexagons = H3_EXPORT(maxPolygonToCellsSize)(&emptyGeoPolygon, 9);
        H3Index *hexagons = calloc(numHexagons, sizeof(H3Index));

        H3_EXPORT(polygonToCells)(&emptyGeoPolygon, 9, hexagons);
        int actualNumIndexes = countNonNullIndexes(hexagons, numHexagons);

        t_assert(actualNumIndexes == 0,
                 "got expected polygonToCells size (empty)");
        free(hexagons);
    }

    TEST(polygonToCellsExact) {
        LatLng somewhere = {1, 2};
        H3Index origin;
        t_assertSuccess(H3_EXPORT(latLngToCell)(&somewhere, 9, &origin));
        CellBoundary boundary;
        H3_EXPORT(cellToBoundary)(origin, &boundary);

        LatLng *verts = calloc(boundary.numVerts + 1, sizeof(LatLng));
        for (int i = 0; i < boundary.numVerts; i++) {
            verts[i] = boundary.verts[i];
        }
        verts[boundary.numVerts] = boundary.verts[0];

        GeoLoop someGeoLoop;
        someGeoLoop.numVerts = boundary.numVerts + 1;
        someGeoLoop.verts = verts;
        GeoPolygon someHexagon;
        someHexagon.geoloop = someGeoLoop;
        someHexagon.numHoles = 0;

        int numHexagons = H3_EXPORT(maxPolygonToCellsSize)(&someHexagon, 9);
        H3Index *hexagons = calloc(numHexagons, sizeof(H3Index));

        H3_EXPORT(polygonToCells)(&someHexagon, 9, hexagons);
        int actualNumIndexes = countNonNullIndexes(hexagons, numHexagons);

        t_assert(actualNumIndexes == 1, "got expected polygonToCells size (1)");
        free(hexagons);
        free(verts);
    }

    TEST(polygonToCellsTransmeridian) {
        LatLng primeMeridianVerts[] = {
            {0.01, 0.01}, {0.01, -0.01}, {-0.01, -0.01}, {-0.01, 0.01}};
        GeoLoop primeMeridianGeoLoop = {.numVerts = 4,
                                        .verts = primeMeridianVerts};
        GeoPolygon primeMeridianGeoPolygon = {.geoloop = primeMeridianGeoLoop,
                                              .numHoles = 0};

        LatLng transMeridianVerts[] = {{0.01, -M_PI + 0.01},
                                       {0.01, M_PI - 0.01},
                                       {-0.01, M_PI - 0.01},
                                       {-0.01, -M_PI + 0.01}};
        GeoLoop transMeridianGeoLoop = {.numVerts = 4,
                                        .verts = transMeridianVerts};
        GeoPolygon transMeridianGeoPolygon = {.geoloop = transMeridianGeoLoop,
                                              .numHoles = 0};

        LatLng transMeridianHoleVerts[] = {{0.005, -M_PI + 0.005},
                                           {0.005, M_PI - 0.005},
                                           {-0.005, M_PI - 0.005},
                                           {-0.005, -M_PI + 0.005}};
        GeoLoop transMeridianHoleGeoLoop = {.numVerts = 4,
                                            .verts = transMeridianHoleVerts};
        GeoPolygon transMeridianHoleGeoPolygon = {
            .geoloop = transMeridianGeoLoop,
            .numHoles = 1,
            .holes = &transMeridianHoleGeoLoop};
        GeoPolygon transMeridianFilledHoleGeoPolygon = {
            .geoloop = transMeridianHoleGeoLoop, .numHoles = 0};

        int expectedSize;

        // Prime meridian case
        expectedSize = 4228;
        int numHexagons =
            H3_EXPORT(maxPolygonToCellsSize)(&primeMeridianGeoPolygon, 7);
        H3Index *hexagons = calloc(numHexagons, sizeof(H3Index));

        H3_EXPORT(polygonToCells)(&primeMeridianGeoPolygon, 7, hexagons);
        int actualNumIndexes = countNonNullIndexes(hexagons, numHexagons);

        t_assert(actualNumIndexes == expectedSize,
                 "got expected polygonToCells size (prime meridian)");

        // Transmeridian case
        // This doesn't exactly match the prime meridian count because of slight
        // differences in hex size and grid offset between the two cases
        expectedSize = 4238;
        numHexagons =
            H3_EXPORT(maxPolygonToCellsSize)(&transMeridianGeoPolygon, 7);
        H3Index *hexagonsTM = calloc(numHexagons, sizeof(H3Index));

        H3_EXPORT(polygonToCells)(&transMeridianGeoPolygon, 7, hexagonsTM);
        actualNumIndexes = countNonNullIndexes(hexagonsTM, numHexagons);

        t_assert(actualNumIndexes == expectedSize,
                 "got expected polygonToCells size (transmeridian)");

        // Transmeridian filled hole case -- only needed for calculating hole
        // size
        numHexagons = H3_EXPORT(maxPolygonToCellsSize)(
            &transMeridianFilledHoleGeoPolygon, 7);
        H3Index *hexagonsTMFH = calloc(numHexagons, sizeof(H3Index));

        H3_EXPORT(polygonToCells)
        (&transMeridianFilledHoleGeoPolygon, 7, hexagonsTMFH);
        int actualNumHoleIndexes =
            countNonNullIndexes(hexagonsTMFH, numHexagons);

        // Transmeridian hole case
        numHexagons =
            H3_EXPORT(maxPolygonToCellsSize)(&transMeridianHoleGeoPolygon, 7);
        H3Index *hexagonsTMH = calloc(numHexagons, sizeof(H3Index));

        H3_EXPORT(polygonToCells)(&transMeridianHoleGeoPolygon, 7, hexagonsTMH);
        actualNumIndexes = countNonNullIndexes(hexagonsTMH, numHexagons);

        t_assert(actualNumIndexes == expectedSize - actualNumHoleIndexes,
                 "got expected polygonToCells size (transmeridian hole)");

        free(hexagons);
        free(hexagonsTM);
        free(hexagonsTMFH);
        free(hexagonsTMH);
    }

    TEST(polygonToCellsTransmeridianComplex) {
        // This polygon is "complex" in that it has > 4 vertices - this
        // tests for a bug that was taking the max and min longitude as
        // the bounds for transmeridian polygons
        LatLng verts[] = {{0.1, -M_PI + 0.00001},  {0.1, M_PI - 0.00001},
                          {0.05, M_PI - 0.2},      {-0.1, M_PI - 0.00001},
                          {-0.1, -M_PI + 0.00001}, {-0.05, -M_PI + 0.2}};
        GeoLoop geoloop = {.numVerts = 6, .verts = verts};
        GeoPolygon polygon = {.geoloop = geoloop, .numHoles = 0};

        int numHexagons = H3_EXPORT(maxPolygonToCellsSize)(&polygon, 4);

        H3Index *hexagons = calloc(numHexagons, sizeof(H3Index));
        H3_EXPORT(polygonToCells)(&polygon, 4, hexagons);

        int actualNumIndexes = countNonNullIndexes(hexagons, numHexagons);

        t_assert(actualNumIndexes == 1204,
                 "got expected polygonToCells size (complex transmeridian)");

        free(hexagons);
    }

    TEST(polygonToCellsPentagon) {
        H3Index pentagon;
        setH3Index(&pentagon, 9, 24, 0);
        LatLng coord;
        H3_EXPORT(cellToLatLng)(pentagon, &coord);

        // Length of half an edge of the polygon, in radians
        double edgeLength2 = H3_EXPORT(degsToRads)(0.001);

        LatLng boundingTopRigt = coord;
        boundingTopRigt.lat += edgeLength2;
        boundingTopRigt.lng += edgeLength2;

        LatLng boundingTopLeft = coord;
        boundingTopLeft.lat += edgeLength2;
        boundingTopLeft.lng -= edgeLength2;

        LatLng boundingBottomRight = coord;
        boundingBottomRight.lat -= edgeLength2;
        boundingBottomRight.lng += edgeLength2;

        LatLng boundingBottomLeft = coord;
        boundingBottomLeft.lat -= edgeLength2;
        boundingBottomLeft.lng -= edgeLength2;

        LatLng verts[] = {boundingBottomLeft, boundingTopLeft, boundingTopRigt,
                          boundingBottomRight};

        GeoLoop geoloop;
        geoloop.verts = verts;
        geoloop.numVerts = 4;

        GeoPolygon polygon;
        polygon.geoloop = geoloop;
        polygon.numHoles = 0;

        int numHexagons = H3_EXPORT(maxPolygonToCellsSize)(&polygon, 9);
        H3Index *hexagons = calloc(numHexagons, sizeof(H3Index));

        H3_EXPORT(polygonToCells)(&polygon, 9, hexagons);

        int found = 0;
        int numPentagons = 0;
        for (int i = 0; i < numHexagons; i++) {
            if (hexagons[i] != 0) {
                found++;
            }
            if (H3_EXPORT(isPentagon)(hexagons[i])) {
                numPentagons++;
            }
        }
        t_assert(found == 1, "one index found");
        t_assert(numPentagons == 1, "one pentagon found");
        free(hexagons);
    }

    TEST(fillIndex) {
        iterateAllIndexesAtRes(0, fillIndex_assertions);
        iterateAllIndexesAtRes(1, fillIndex_assertions);
        iterateAllIndexesAtRes(2, fillIndex_assertions);
    }
}
