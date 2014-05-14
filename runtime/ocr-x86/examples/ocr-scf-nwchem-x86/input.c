#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "cscc.h"

//void input(void) {
void input(int atoms) {

//.......................................................................
// Input configuration from an XYZ format file call be.inpt and set up
// initial data structures. Atomic numbers from XYZ file are ignored and
// an atomic number of 4 (Beryllium) is used instead.
//.......................................................................

  int i, j;
  FILE *fp = fopen("be.inpt","r");

  if (fscanf(fp,"%d",&natom) != 1) { printf("Invalid input file\n"); exit(1); }
  for (i = 0; i < maxatom; i++) { ax[i] = 0.0; ay[i] = 0.0; az[i] = 0.0; }


natom = atoms;

  /* Read in coordinates */

  for (i = 0; i < natom; i++) {
      if (fscanf(fp,"%d",&j) != 1) { printf("Invalid input file\n"); exit(1); }
      if (fscanf(fp,"%lf",&ax[i]) != 1) { printf("Invalid input file\n"); exit(1); }
      if (fscanf(fp,"%lf",&ay[i]) != 1) { printf("Invalid input file\n"); exit(1); }
      if (fscanf(fp,"%lf",&az[i]) != 1) { printf("Invalid input file\n"); exit(1); }
  }

  fclose(fp);

  // Set up s-function centers and nuclear charges

  for (i = 0; i < natom; i++) {
    q[i]      = 4.0;

    expnt[15 * i + 0]  = 1741.0;
    expnt[15 * i + 1]  = 262.1;
    expnt[15 * i + 2]  = 60.33;
    expnt[15 * i + 3]  = 17.62;
    expnt[15 * i + 4]  = 5.933;
    expnt[15 * i + 5]  = 2.185;
    expnt[15 * i + 6]  = 0.859;
    expnt[15 * i + 7]  = 0.1806;
    expnt[15 * i + 8]  = 0.05835;
    expnt[15 * i + 9]  = 0.3;
    expnt[15 * i + 10] = 0.3;
    expnt[15 * i + 11] = 0.3;
    expnt[15 * i + 12] = 0.3;
    expnt[15 * i + 13] = 0.3;
    expnt[15 * i + 14] = 0.3;

    for (j = 0; j < 15; j++) {
      x[15 * i + j] = ax[i];
      y[15 * i + j] = ay[i];
      z[15 * i + j] = az[i];
      if (j == 9)  x[15 * i + j] += 1.6;
      if (j == 10) x[15 * i + j] -= 1.6;
      if (j == 11) y[15 * i + j] += 1.6;
      if (j == 12) y[15 * i + j] -= 1.6;
      if (j == 13) z[15 * i + j] += 1.6;
      if (j == 14) z[15 * i + j] -= 1.6;
  } }

  //  evaluate repulsion energy

  enrep = 0.0;
  for (i = 0;     i < natom; i++) {
  for (j = i + 1; j < natom; j++) {
      double ax2 = (ax[i] - ax[j]) * (ax[i] - ax[j]);
      double ay2 = (ay[i] - ay[j]) * (ay[i] - ay[j]);
      double az2 = (az[i] - az[j]) * (az[i] - az[j]);
      enrep += q[i] * q[j] / sqrt(ax2 + ay2 + az2);
  } }

  nocc  = natom << 1;
  nbfn  = natom * 15;
  nnbfn = nbfn * (nbfn + 1) / 2;

  return;
}
