#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<ctype.h>

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
#define cornersegments 3
#define helixsegments 15

/* This is the length of the feedpoint */
#define epsilon 10

/* Mmmmm... pi */
#define pi 3.14159265359

typedef struct {
    double H; //height
    double D; //diameter
    double R; //angle radius
    double turns; //number of turns
    double offset; //distance from origin
    double Theta; //initial theta angle
    double wire; //radius of wire
    char feed; //O=open, S=short, T=terminated to 50 ohms
    int feedpoint; //tag of feed segment
} helix;

void make_helix(FILE *, helix);

int ITG=1;
int feedpointhack;

int main(int argc, char*argv[])
{
    FILE *infile;
    FILE *outfile;
    int i, n, feed=0;
    helix *h;
    double fstart, fstop, fstep;
    
    if(argc!=3) {
        printf("Usage: helix2nec <inputfile> <outputfile>\n");
        exit(1);
    }
    if((infile=fopen(argv[1],"r"))==NULL) {
        printf("Could not open input file %s\n",argv[1]);
        exit(1);
    }
    if((outfile=fopen(argv[2],"w"))==NULL) {
        printf("Could not open output file %s\n",argv[2]);
        exit(1);
    }
    
    // Parse the input file
    if(fscanf(infile, "%d", &n)!=1) {
        printf("Error in input file %s (number of helices)\n",argv[1]);
        exit(1);
    }
    if((h=(helix*)malloc(2*n*sizeof(helix)))==NULL) {
        printf("Error allocating memory for %d helices\n",n);
        exit(1);
    }
    fprintf(outfile, "CM NEC2 Input File produced by helix2nec\n");
    fprintf(outfile, "CM Parameters:\n");
    for(i=0;i<n;i++) {
        if(fscanf(infile, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %c",
            &h[i*2].H, &h[i*2].D, &h[i*2+1].H, &h[i*2+1].D,
            &h[i*2].R, &h[i*2].wire,
            &h[i*2].turns, &h[i*2].offset, &h[i*2].Theta,
            &h[i*2].feed)!=10) {
            printf("Error in input file %s, "
            "helix number %d (helix data)\n",
                   argv[1], i);
            exit(1);
            }
            h[i*2+1].R=h[i*2].R;
        h[i*2+1].wire=h[i*2].wire;
        h[i*2+1].turns=h[i*2].turns;
        h[i*2+1].offset=h[i*2].offset;
        fprintf(outfile, "CM Helix %d:"
        " H1=%.5E D1=%.5E H2=%.5E D2=%.5E\n",
        i, h[i*2].H, h[i*2].D, h[i*2+1].H, h[i*2+1].D);
        fprintf(outfile, "CM turns=%.5E R=%.5E, wire=%.5E\n",
                h[i*2].turns, h[i*2].R, h[i*2].wire);
        fprintf(outfile, "CM offset=%.5E theta=%.5E ",
                h[i*2].offset, h[i*2].Theta);
        h[i*2+1].wire=(h[i*2].wire/=2); //diameter to radius
        h[i*2].Theta=h[i*2].Theta/360*2*pi; //degrees to radians
        h[i*2+1].Theta=h[i*2].Theta+pi/2;
        switch(toupper(h[i*2].feed)) {
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
                printf("Error in input file %s, "
                "helix number %d (termination type)\n",
                       argv[1], i);
                exit(1);
        }
    }
    if(feed==0) {
        printf("No feed helix in model???\n");
        exit(1);
    }
    if(feed>1) {
        printf("Too many feed helices in model!!!\n");
        exit(1);
    }
    
    // Frequency specification
    if(fscanf(infile, "%lf %lf %lf",
        &fstart, &fstop, &fstep)!=3) {
        printf("Error in input file %s, (frequency)\n",
               argv[1]);
        exit(1);
        }
        fprintf(outfile, "CM %.5E - %.5E MHz in %.5E MHz steps\n",
                fstart, fstop, fstep);
        
        fprintf(outfile, "CE\n");
    
    // do the helices
    for(i=0;i<n;i++) {
        feedpointhack=1;
        make_helix(outfile, h[2*i]);
        feedpointhack=0;
        make_helix(outfile, h[2*i+1]);
        if(toupper(h[2*i].feed)!='O') {
            fprintf(outfile, "GW %d %d %.5E %.5E %.5E "
            "%.5E %.5E %.5E %.5E\n",
            (h[2*i].feedpoint=ITG++), 1,
                    (epsilon/2)*cos(h[2*i].Theta+pi/4)/1000,
                    (epsilon/2)*sin(h[2*i].Theta+pi/4)/1000,
                    (h[2*i].offset)/1000,
                    -(epsilon/2)*cos(h[2*i].Theta+pi/4)/1000,
                    -(epsilon/2)*sin(h[2*i].Theta+pi/4)/1000,
                    (h[2*i].offset)/1000,
                    h[2*i].wire/1000);
            
        }
    }
    fprintf(outfile, "GE 0\n");
    
    // Frequency specification
    fprintf(outfile, "FR 0 %d 0 0 %.5E %.5E\n",
            (int)((fstop-fstart)/fstep)+1, fstart, fstep);
    
    // LD impedance loading to 50 ohms resistive
    for(i=0;i<n;i++) {
        if(toupper(h[2*i].feed)=='T') {
            fprintf(outfile, "LD 4 %d 1 1 "
            "5.00000E+01 0.00000E+00\n",
            h[2*i].feedpoint);
        }
    }
    
    // Voltage excitation
    for(i=0;i<n;i++) {
        if(toupper(h[2*i].feed)=='F') {
            fprintf(outfile, "EX 0 %d 1 0 "
            "1.00000E+00 0.00000E+00\n",
            h[2*i].feedpoint);
        }
    }
    
    // Compute radiation pattern with fixed increments
    fprintf(outfile, "RP 0 37 37 1000 0.00000E+00 0.00000E+00 "
    "5.00000E+00 1.00000E+01 0.00000E+00 0.00000E+00\n");
    
    // End of run
    fprintf(outfile, "EN\n");
    return 0;
}

/* This outputs NEC2 code to the output file, to produce the type of
 *   bifilar helix loop defined by struct helix h. No checking is done
 *   re the sanity of the parameters passed therein. */
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
