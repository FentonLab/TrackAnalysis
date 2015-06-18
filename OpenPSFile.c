#include <stdio.h>
#include <stdlib.h>
#include "Track.h"

FILE    *OpenPSFile(char *file, float PointsPerPixel)
{
        static FILE    *fp;

        fp = fopen(file,"w");
        if(fp == NULL){
                (void) fprintf(MyStdErr,"\nCould not open PostScript file: %s\n", file);
                return (NULL);
       }

        /* start postscript file with this stuff */
        (void) fprintf(fp, "%%%%!PS-Adobe-2.0 EPSF-1.2\n");
        (void) fprintf(fp, "/mt {moveto} bind def\n");
        (void) fprintf(fp, "/lt {lineto} bind def\n");
        (void) fprintf(fp, "/pix { %0.3f mul } def\n", PointsPerPixel);
        (void) fprintf(fp, "/cm { 28.35 mul } def\n");
        (void) fprintf(fp, "/inch { 72 mul } def\n");
        (void) fprintf(fp, "/center {dup stringwidth pop 2 div neg 0 rmoveto} bind def\n");
        (void) fprintf(fp, "grestore\n");

        return(fp);
}

