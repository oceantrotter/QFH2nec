/**
 *     This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 * 
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 * 
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<ctype.h>


#define VERSION 0.1

/*
 *    The input file's structure is as follows:
 *    The first line defines how many helices there will be.                                       
 *    The following lines define each helix, one helix per line. There must be as many lines as the first line specified. Each line contains, in this order:
 *    Height of the smaller loop
 *    Diameter of the smaller loop
 *    Height of the larger loop
 *    Diameter of the larger loop
 *    Bend radius (of the 90-degree turns)
 *    Wire diameter
 *    Number of twists in the helix
 *    Vertical offset of this helix
 *    Angle offset of this helix (in degrees)
 *    Feed type: F=feed, T=terminated, O=open, S=shorted 
 *    All lengths are in millimeters, measured from the center of the conductors. The model must contain precisely one helix with feed type F, other helices must be terminated, open or shorted.
 *    The last line specifies the start frequency, end frequency, and frequency increment, in MHz. 
 */

/* This defines how small segments to use in the produced NEC2 code */
#define radialsegments 5
#define cornersegments 5
#define helixsegments 20

/* This is the length of the feedpoint */
#define epsilon 2

/* Mmmmm... pi */
// actual better way to get the value
const double pi = 4.0 * atan(1.0);

typedef struct {
    double H; //height
    double D; //diameter
    double R; //corner radius
    double turns; //number of turns
    double offset; //distance from origin
    double Theta; //initial theta angle
    double wire; //radius of wire
    char feed; //O=open, S=short, T=terminated to 50 ohms
    int feedpoint; //tag of feed segment
} helix;

typedef struct {
    double freq; // Design frequency in MHz
    double turns; // Number of turns (twist)
    double length; // Length of one turn in wavelengths
    double radius; // Bending radius in mm
    double diam; // Conductor diameter in mm
    double ratio; // Width/height ratio
} design_req;

void make_helix(FILE *, helix);
void compute_design(design_req *requirements, helix *helixes);

int ITG=1; // Tag increment
int feedpointhack;

int main(int argc, char*argv[])
{
    FILE *outfile;
    int feed=0;
    helix h[2];
    design_req req;
    double fstart, fstop, fstep;
    fstep = 0.25;
    
    if(argc!=6+1) {
        printf("Usage:\nQFH2nec <Design frequency in MHz> <Number of turns> <Length of one turn in wavelengths> <Bending radius> <Conductor diameter> <Width/height ratio>\n");
        // TODO add more explanation about input
        exit(1);
    }
    
    // Design frequency validation
    sscanf(argv[1],"%lf",&req.freq);
    if(req.freq < 10 || req.freq > 5000) {
        printf("Design frequency %f is not in the range 10-5000MHz",req.freq);
        exit(1);
    }
    // Turn number validation
    sscanf(argv[2],"%lf",&req.turns);
    if(req.turns < 0.1 || req.turns > 50) {
        printf("Turn number %f is not in the range 0.1-50",req.turns);
        exit(1);
    }
    // One turn wavelength validation
    sscanf(argv[3],"%lf",&req.length);
    if(req.length < 0.1 || req.length > 5) {
        printf("One turn length %f is not in the range 0.1-5",req.length);
        exit(1);
    }
    // Bending radius validation
    sscanf(argv[4],"%lf",&req.radius);
    if(req.radius < 1 || req.radius > 1000) {
        printf("Bending radius %f is not in the range 1-1000",req.radius);
        exit(1);
    }
    // Conductor diameter validation
    sscanf(argv[5],"%lf",&req.diam);
    if(req.diam < 1 || req.diam > 50) {
        printf("Conductor diameter %f is not in the range 1-50",req.diam);
        exit(1);
    }
    // Width/height ratio validation
    sscanf(argv[6],"%lf",&req.ratio);
    if(req.ratio < 0.1 || req.ratio > 2) {
        printf("Width/height ratio %f is not in the range 0.1-2",req.ratio);
        exit(1);
    }
    /*printf("Here is what was scanned:\n");
    printf("Frequency %f\n",req.freq);
    printf("Turn number %f\n",req.turns);
    printf("One turn length %f\n",req.length);
    printf("Bending radius %f\n",req.radius);
    printf("Conductor diameter %f\n",req.diam);
    printf("Width/height ratio %f\n",req.ratio);*/
    
    char filename[50];
    sprintf(filename, "QFH %4.1f_%.1f_%.2f_%.1f_%.1f_%.1f.nec",req.freq, req.turns, req.ratio, req.length, req.radius, req.diam);
    if((outfile=fopen(filename,"w"))==NULL) {
        printf("Could not open output file %s\n",filename);
        exit(1);
    }
    printf("The output filename is %s\n",filename);
    
    compute_design(&req, h);
    
    fprintf(outfile, "CM NEC2 Input File produced by helix2nec\n");
    fprintf(outfile, "CM Parameters:\n");
    fprintf(outfile, "CM Helix 1:"
    " H1=%.5E D1=%.5E H2=%.5E D2=%.5E\n",
    h[0].H, h[0].D, h[1].H, h[1].D);
    fprintf(outfile, "CM turns=%.5E R=%.5E, wire=%.5E\n",
            h[0].turns, h[0].R, h[0].wire);
    fprintf(outfile, "CM offset=%.5E theta=%.5E ",
            h[0].offset, h[0].Theta);
    h[1].wire=(h[0].wire/=2); //diameter to radius
    h[0].Theta=h[0].Theta/360*2*pi; //degrees to radians
    h[1].Theta=h[0].Theta+pi/2;
    switch(toupper(h[0].feed)) {
        case 'O':
            fprintf(outfile,"open\n");
            break;
        case 'S':
            fprintf(outfile,"shorted\n");
            break;
        case 'T':
            fprintf(outfile,"terminated\n");
            break;
        case 'F':
            fprintf(outfile,"feed\n");
            feed+=1;
            break;
        default:
            printf("Error somewhere in the program, "
            "helix number 1 (termination type)\n");
            exit(1);
    }
    //}
    if(feed==0) {
        printf("No feed helix in model???\n");
        exit(1);
    }
    if(feed>1) {
        printf("Too many feed helices in model!!!\n");
        exit(1);
    }
    
    // Frequency specification
    fstart = req.freq - 5;
    fstop = req.freq + 5;
    fprintf(outfile, "CM %.5E - %.5E MHz in %.5E MHz steps\n",
            fstart, fstop, fstep);
    
    fprintf(outfile, "CE\n");
    
    // do the helices
    feedpointhack=1;
    make_helix(outfile, h[0]);
    feedpointhack=0;
    make_helix(outfile, h[1]);
    if(toupper(h[0].feed)!='O') {
        fprintf(outfile, "GW %d %d %.5E %.5E %.5E "
        "%.5E %.5E %.5E %.5E\n",
        (h[0].feedpoint=ITG++), 1,
                (epsilon/2)*cos(h[0].Theta+pi/4)/1000,
                (epsilon/2)*sin(h[0].Theta+pi/4)/1000,
                (h[0].offset)/1000,
                -(epsilon/2)*cos(h[0].Theta+pi/4)/1000,
                -(epsilon/2)*sin(h[0].Theta+pi/4)/1000,
                (h[0].offset)/1000,
                h[0].wire/1000);
        
    }
    
    fprintf(outfile, "GE 0\n");
    
    // Frequency specification
    fprintf(outfile, "FR 0 %d 0 0 %.5E %.5E\n",
            (int)((fstop-fstart)/fstep)+1, fstart, fstep);
    
    // LD impedance loading to 50 ohms resistive
    if(toupper(h[0].feed)=='T') {
        fprintf(outfile, "LD 4 %d 1 1 "
        "5.00000E+01 0.00000E+00\n",
        h[0].feedpoint);
    }
    
    // Voltage excitation
    if(toupper(h[0].feed)=='F') {
        fprintf(outfile, "EX 0 %d 1 0 "
        "1.00000E+00 0.00000E+00\n",
        h[0].feedpoint);
    }
    
    // Compute radiation pattern with fixed increments
    fprintf(outfile, "RP 0 37 37 1000 0.00000E+00 0.00000E+00 "
    "5.00000E+00 1.00000E+01 0.00000E+00 0.00000E+00\n");
    
    // End of run
    fprintf(outfile, "EN\n");
    return 0;
}

/* This outputs NEC2 code to the output file, to produce the type of
 * bifilar helix loop defined by struct helix h. No checking is done
 * re the sanity of the parameters passed therein. */
// TODO make the helixes inside one another centered in the middle
void make_helix(FILE *outfile, helix h)
{
    int i;
    double x, y, z, alpha, theta, r;
    double x1, y1, z1;
    
    // top radial wires
    if(feedpointhack) {
        x1=(epsilon/2)*cos(h.Theta+pi/4);
        y1=(epsilon/2)*sin(h.Theta+pi/4);
    } else {
        x1=(epsilon/2)*cos(h.Theta-pi/4);
        y1=(epsilon/2)*sin(h.Theta-pi/4);
    }
    z1=0;
    x=(h.D/2-h.R)*cos(h.Theta);
    y=(h.D/2-h.R)*sin(h.Theta);
    z=0;
    fprintf(outfile, "GW %d %d %.5E %.5E %.5E %.5E %.5E %.5E %.5E\n",
            ITG++, radialsegments,
            x1/1000, y1/1000, (z1+h.offset)/1000,
            x/1000, y/1000, (z+h.offset)/1000,
            h.wire/1000);
    fprintf(outfile, "GW %d %d %.5E %.5E %.5E %.5E %.5E %.5E %.5E\n",
            ITG++, radialsegments,
            -x1/1000, -y1/1000, (z1+h.offset)/1000,
            -x/1000, -y/1000, (z+h.offset)/1000,
            h.wire/1000);
    
    // top bends
    for(i=1;i<=cornersegments;i++) {
        x1=x; y1=y; z1=z;
        alpha=pi/2*(double)i/cornersegments;
        z=-h.R+h.R*cos(alpha);
        theta=z/h.H*h.turns*2*pi+h.Theta;
        r=h.D/2-h.R+h.R*sin(alpha);
        x=r*cos(theta);
        y=r*sin(theta);
        fprintf(outfile,
                "GW %d %d %.5E %.5E %.5E %.5E %.5E %.5E %.5E\n",
                ITG++, 1,
                x1/1000, y1/1000, (z1+h.offset)/1000,
                x/1000, y/1000, (z+h.offset)/1000,
                h.wire/1000);
        fprintf(outfile,
                "GW %d %d %.5E %.5E %.5E %.5E %.5E %.5E %.5E\n",
                ITG++, 1,
                -x1/1000, -y1/1000, (z1+h.offset)/1000,
                -x/1000, -y/1000, (z+h.offset)/1000,
                h.wire/1000);
    }
    
    // helical wires
    r=h.D/2;
    for(i=1;i<=helixsegments;i++) {
        x1=x; y1=y; z1=z;
        z=-h.R - (double)i/helixsegments*(h.H-2*h.R);
        theta=z/h.H*h.turns*2*pi+h.Theta;
        x=r*cos(theta);
        y=r*sin(theta);
        fprintf(outfile,
                "GW %d %d %.5E %.5E %.5E %.5E %.5E %.5E %.5E\n",
                ITG++, 1,
                x1/1000, y1/1000, (z1+h.offset)/1000,
                x/1000, y/1000, (z+h.offset)/1000,
                h.wire/1000);
        fprintf(outfile,
                "GW %d %d %.5E %.5E %.5E %.5E %.5E %.5E %.5E\n",
                ITG++, 1,
                -x1/1000, -y1/1000, (z1+h.offset)/1000,
                -x/1000, -y/1000, (z+h.offset)/1000,
                h.wire/1000);
    }
    
    // bottom bends
    for(i=1;i<=cornersegments;i++) {
        x1=x; y1=y; z1=z;
        alpha=pi/2*(double)i/cornersegments;
        z=-h.H+h.R - h.R*sin(alpha);
        theta=z/h.H*h.turns*2*pi+h.Theta;
        r=h.D/2-h.R+h.R*cos(alpha);
        x=r*cos(theta);
        y=r*sin(theta);
        fprintf(outfile,
                "GW %d %d %.5E %.5E %.5E %.5E %.5E %.5E %.5E\n",
                ITG++, 1,
                x1/1000, y1/1000, (z1+h.offset)/1000,
                x/1000, y/1000, (z+h.offset)/1000,
                h.wire/1000);
        fprintf(outfile,
                "GW %d %d %.5E %.5E %.5E %.5E %.5E %.5E %.5E\n",
                ITG++, 1,
                -x1/1000, -y1/1000, (z1+h.offset)/1000,
                -x/1000, -y/1000, (z+h.offset)/1000,
                h.wire/1000);
    }
    
    // bottom radial wire
    fprintf(outfile, "GW %d %d %.5E %.5E %.5E %.5E %.5E %.5E %.5E\n",
            ITG++, radialsegments*2-1,
            x/1000, y/1000, (z+h.offset)/1000,
            -x/1000, -y/1000, (z+h.offset)/1000,
            h.wire/1000);
    
    return;
}


/*
 * The following code has been adapted from the software published by 
 * John Coppens here: https://www.jcoppens.com/ant/qfh/calc.en.php
 * The following license was part of the original file.
 * 
 *    qfhcalc.js
 * 
 *    Copyright (C) 2000 John Coppens (jcoppens@usa.net)
 * 
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 * 
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 * 
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

double deltal(double diam) {
    double tbl[17] = {1.045, 1.053, 1.060, 1.064, 1.068, 1.070, 1.070, 1.071, 
    1.071, 1.070, 1.070, 1.070, 1.070, 1.069, 1.069, 1.068, 1.067};
    int intv = (int)diam;
    return (tbl[intv] + (tbl[intv+1]-tbl[intv])*(diam-intv));
}

double deltaf(double diam) {
    double tbl[17] = {1.013, 1.014, 1.015, 1.016, 1.017, 1.018, 1.020, 1.022, 
    1.025, 1.027, 1.030, 1.033, 1.036, 1.041, 1.044, 1.049, 1.054};
    int intv = (int)diam;
    return (tbl[intv] + (tbl[intv+1]-tbl[intv])*(diam-intv));
}

void compute_design(design_req *requirements, helix *helixes) {
    double freq = requirements->freq;
    double wdiam = requirements->diam;
    double wrad = requirements->radius;
    double ratio = requirements->ratio;
    double turns = requirements->turns;
    double nrwavel = requirements->length;
    
    helixes[0].turns = -turns;
    helixes[0].feed = 'F';
    helixes[0].offset = 0;
    helixes[0].R = wrad;
    helixes[0].wire = wdiam / 2;
    
    helixes[1].turns = -turns;
    helixes[1].feed = 'F';
    helixes[1].offset = 0;
    helixes[1].R = wrad;
    helixes[1].wire = wdiam / 2;
    
    double wavel = 299792/freq;
    double wd_eff = wdiam;
    if (wdiam > 15) wd_eff = 15;

    double wavelc = nrwavel * wavel * deltal(wd_eff);

    double bendcorr = 2*wrad - pi*wrad/2;
    
 //  double optdiam = 0.0088 * wavelc;
    
    double total1 = wavelc * 1.026;
    double total1c = total1 + 4*bendcorr;
    double rad1 = 0.5 * total1c / 
    (1 + sqrt(1/pow(ratio,2) + pow(turns*pi,2)));
    //double vert1 = (total1c - 2*rad1)/2;
    double height1 = rad1 / ratio;
    
    helixes[1].D = rad1;
    helixes[1].H = height1;
    
    
    double total2 = wavelc * 0.975;
    double total2c = total2 + 4*bendcorr;
    double rad2 = 0.5 * total2c /
    (1 + sqrt(1/pow(ratio,2) + pow(turns*pi,2)));
    //double vert2 = (total2c - 2*rad2)/2;
    double height2 = rad2 / ratio;
    
    helixes[0].D = rad2;
    helixes[0].H = height2;
}
