#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//  Copyright (C) 2013 Lindley Lentati

/*
*    This file is part of TempoNest 
* 
*    TempoNest is free software: you can redistribute it and/or modify 
*    it under the terms of the GNU General Public License as published by 
*    the Free Software Foundation, either version 3 of the License, or 
*    (at your option) any later version. 
*    TempoNest  is distributed in the hope that it will be useful, 
*    but WITHOUT ANY WARRANTY; without even the implied warranty of 
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
*    GNU General Public License for more details. 
*    You should have received a copy of the GNU General Public License 
*    along with TempoNest.  If not, see <http://www.gnu.org/licenses/>. 
*/

/*
*    If you use TempoNest and as a byproduct both Tempo2 and MultiNest
*    then please acknowledge it by citing Lentati L., Alexander P., Hobson M. P. (2013) for TempoNest,
*    Hobbs, Edwards & Manchester (2006) MNRAS, Vol 369, Issue 2, 
*    pp. 655-672 (bibtex: 2006MNRAS.369..655H)
*    or Edwards, Hobbs & Manchester (2006) MNRAS, VOl 372, Issue 4,
*    pp. 1549-1574 (bibtex: 2006MNRAS.372.1549E) when discussing the
*    timing model and MultiNest Papers here.
*/


#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <vector>
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_sf_bessel.h>
#include "dgemm.h"
#include "dgemv.h"
#include "dpotri.h"
#include "dpotrf.h"
#include "dpotrs.h"
#include "tempo2.h"
#include "TempoNest.h"
#include "dgesvd.h"
#include "qrdecomp.h"
#include "cholesky.h"
#include "T2toolkit.h"
#include <fstream>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <sstream>
#include <iterator>
#include <cstring>

/*
#ifdef HAVE_MLAPACK
#include <mpack/mblas_qd.h>
#include <mpack/mlapack_qd.h>
#include <mpack/mblas_dd.h>
#include <mpack/mlapack_dd.h>
#endif
*/
#include <fftw3.h>

using namespace std;

void *globalcontext;
//void SmallNelderMeadOptimum(int nParameters, double *ML);
//void  WriteProfileFreqEvo(std::string longname, int &ndim, int profiledimstart);

void assigncontext(void *context){
        globalcontext=context;
}

/*
int Wrap(int kX, int const kLowerBound, int const kUpperBound)
{
    int range_size = kUpperBound - kLowerBound + 1;

    if (kX < kLowerBound)
        kX += range_size * ((kLowerBound - kX) / range_size + 1);

    return kLowerBound + (kX - kLowerBound) % range_size;
}

*/
void TNothpl(int n,double x,double *pl){


        double a=2.0;
        double b=0.0;
        double c=1.0;
        double y0=1.0;
        double y1=2.0*x;
        pl[0]=1.0;
        pl[1]=2.0*x;


//	printf("I AM IN OTHPL %i \n", n);
        for(int k=2;k<n;k++){

                double c=2.0*(k-1.0);
//		printf("%i %g\n", k, sqrt(double(k*1.0)));
		y0=y0/sqrt(double(k*1.0));
		y1=y1/sqrt(double(k*1.0));
                double yn=(a*x+b)*y1-c*y0;
		yn=yn;///sqrt(double(k));
                pl[k]=yn;///sqrt(double(k));
                y0=y1;
                y1=yn;

        }



}


void TNothplMC(int n,double x,double *pl, int cpos){

	//printf("I AM IN OTHPL %i %i\n", n, cpos);
        double a=2.0;
        double b=0.0;
        double c=1.0;
        double y0=1.0;
        double y1=2.0*x;
        pl[0+cpos]=1.0;
	if(n>1){
        	pl[1+cpos]=2.0*x;
	}



        for(int k=2;k<n;k++){

                double c=2.0*(k-1.0);
//		printf("%i %g\n", k, sqrt(double(k*1.0)));
		y0=y0/sqrt(double(k*1.0));
		y1=y1/sqrt(double(k*1.0));
                double yn=(a*x+b)*y1-c*y0;
		yn=yn;///sqrt(double(k));
                pl[k+cpos]=yn;///sqrt(double(k));
                y0=y1;
                y1=yn;

        }



}

/*
void outputProfile(int ndim){

        int number_of_lines = 0;
        double weightsum=0;

	std::string longname="/home/ltl21/scratch/Pulsars/ProfileData/J1909-10cm/results/AllTOA-40Shape-EFJitter2-J1909-3744-";

        std::ifstream checkfile;
        std::string checkname = longname+".txt";
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        printf("number of lines %i \n",number_of_lines);
        checkfile.close();

        std::ifstream summaryfile;
        std::string fname = longname+".txt";
        summaryfile.open(fname.c_str());



        printf("Writing Profiles \n");

	double *shapecoeff=new double[((MNStruct *)globalcontext)->numshapecoeff];
	int otherdims =  ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps + ((MNStruct *)globalcontext)->numFitEQUAD + ((MNStruct *)globalcontext)->numFitEFAC;

	int Nbins=1000;
	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;
	double **Hermitepoly=new double*[Nbins];
	for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
	double *shapevec=new double[Nbins];

	std::ofstream profilefile;
	std::string dname = longname+"Profiles.txt";
	
	profilefile.open(dname.c_str());

	double *MeanProfile = new double[Nbins];
	double *ProfileErr = new double[Nbins];

	for(int i=0;i<Nbins;i++){
		MeanProfile[i] = 0;
		ProfileErr[i] = 0;
	}
	

	printf("getting mean \n");
	weightsum=0;
        for(int i=0;i<number_of_lines;i++){


		if(i%100 == 0)printf("line: %i of %i %g \n", i, number_of_lines, double(i)/double(number_of_lines));

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);

		double weight=paramlist[0];

                weightsum += weight;
                double like = paramlist[1];

		int cnum=0;
                for(int i = otherdims; i < otherdims+((MNStruct *)globalcontext)->numshapecoeff; i++){
			shapecoeff[cnum] = paramlist[i+2];
			//printf("coeff %i %g \n", cnum, shapecoeff[cnum]);
			cnum++;
			
                }

		double beta = paramlist[otherdims+((MNStruct *)globalcontext)->numshapecoeff+2];

		for(int j =0; j < Nbins; j++){
	

			double timeinterval = 0.0001;
			double time = -1.0*double(Nbins)*timeinterval/2 + j*timeinterval;

			


			TNothpl(numcoeff,time/beta,Hermitepoly[j]);
			for(int k =0; k < numcoeff; k++){
				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI)*((MNStruct *)globalcontext)->Factorials[k]);
				Hermitepoly[j][k]=Hermitepoly[j][k]*Bconst*exp(-0.5*time*time/beta/beta);
			}

		}

		shapecoeff[0] = 1.0;
		dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');

		//profilefile << weightsum;
		//profilefile << " ";


		for(int j = 0; j < Nbins; j++){

			//profilefile << shapevec[j];
			//profilefile << " ";
			//printf("shape %i %g \n", j, shapevec[j]);

			MeanProfile[j] = MeanProfile[j] + shapevec[j]*weight;

		}

		//profilefile << "\n";	

	}

	for(int j = 0; j < Nbins; j++){
		MeanProfile[j] = MeanProfile[j]/weightsum;
	}


	summaryfile.close();

	summaryfile.open(fname.c_str());

	printf("Getting Errors \n");


	weightsum=0;
	 for(int i=0;i<number_of_lines;i++){

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);

		double weight=paramlist[0];

                weightsum += weight;
                double like = paramlist[1];

		int cnum=0;
                for(int i = otherdims; i < otherdims+((MNStruct *)globalcontext)->numshapecoeff; i++){
			shapecoeff[cnum] = paramlist[i+2];
			//printf("coeff %i %g \n", cnum, shapecoeff[cnum]);
			cnum++;
			
                }

		double beta = paramlist[otherdims+((MNStruct *)globalcontext)->numshapecoeff+2];

		for(int j =0; j < Nbins; j++){
	

			double timeinterval = 0.0001;
			double time = -1.0*double(Nbins)*timeinterval/2 + j*timeinterval;

			


			TNothpl(numcoeff,time/beta,Hermitepoly[j]);
			for(int k =0; k < numcoeff; k++){
				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI)*((MNStruct *)globalcontext)->Factorials[k]);
				Hermitepoly[j][k]=Hermitepoly[j][k]*Bconst*exp(-0.5*time*time/beta/beta);
			}

		}

		shapecoeff[0] = 1.0;
		dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');

                for(int j = 0; j < Nbins; j++){
                        ProfileErr[j] += weight*(shapevec[j] - MeanProfile[j])*(shapevec[j] - MeanProfile[j]);
                }
        }


	for(int i = 0; i < Nbins; i++){
		ProfileErr[i] = sqrt(ProfileErr[i]/weightsum);
	}


	for(int i = 0; i < Nbins; i++){
		printf("%i %g %g \n", i, MeanProfile[i], ProfileErr[i]);
	}

	

	//profilefile.close();

}
*/
/*

double  WhiteLogLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){

	clock_t startClock,endClock;

	double *EFAC;
	double *EQUAD;
	int pcount=0;
	
	int numfit=((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps;
	long double LDparams[numfit];
	double Fitparams[numfit];
	double *Resvec=new double[((MNStruct *)globalcontext)->pulse->nobs];

	double NLphase=0;
	//for(int p=0;p<ndim;p++){

	//	Cube[p]=(((MNStruct *)globalcontext)->Dpriors[p][1]-((MNStruct *)globalcontext)->Dpriors[p][0])*Cube[p]+((MNStruct *)globalcontext)->Dpriors[p][0];
	//}

	if(((MNStruct *)globalcontext)->doLinear==0){
	
		for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){
			LDparams[p]=Cube[p]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		}

		NLphase=(double)LDparams[0];
		pcount++;
		for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
			((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
			pcount++;
		}
		for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
			((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
			pcount++;
		}
		
//		fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
		formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,1);       
//		
//		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
//			Resvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].residual+phase;
//		}
	
	}
	else if(((MNStruct *)globalcontext)->doLinear==1){
	
		for(int p=0;p < numfit; p++){
			Fitparams[p]=Cube[p];
			//printf("FitP: %i %g \n",p,Cube[p]);
			pcount++;
		}
		
		double *Fitvec=new double[((MNStruct *)globalcontext)->pulse->nobs];

		dgemv(((MNStruct *)globalcontext)->DMatrix,Fitparams,Fitvec,((MNStruct *)globalcontext)->pulse->nobs,numfit,'N');
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			Resvec[o]=((MNStruct *)globalcontext)->pulse->obsn[o].residual-Fitvec[o];
			//printf("FitVec: %i %g \n",o,Fitvec[o]);
		}
		
		delete[] Fitvec;
	}

	
	double **Steps;
	if(((MNStruct *)globalcontext)->incStep > 0){
		
		Steps=new double*[((MNStruct *)globalcontext)->incStep];
		
		for(int i = 0; i < ((MNStruct *)globalcontext)->incStep; i++){
			Steps[i]=new double[2];
			Steps[i][0] = Cube[pcount];
			pcount++;
			Steps[i][1] = Cube[pcount];
			pcount++;
		}
	}
		

	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0, Cube[pcount]);
		}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0, Cube[pcount]);
			pcount++;
		}
	}				

	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,2*Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,2*Cube[pcount]);
			pcount++;
                }
        }

        if(((MNStruct *)globalcontext)->doLinear==0){

                fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
                formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,1);      
                for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			  //printf("res: %i %g \n", o, (double)((MNStruct *)globalcontext)->pulse->obsn[o].residual);
                          Resvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].residual+NLphase;
                }
        }


	if(((MNStruct *)globalcontext)->incStep > 0){
		for(int i = 0; i < ((MNStruct *)globalcontext)->incStep; i++){
			double StepAmp = Steps[i][0];
			double StepTime = Steps[i][1];

			for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[o1].bat ;
				if( time > StepTime){
					Resvec[o1] += StepAmp;
				}
			}
		}
	}





	double Chisq=0;
	double noiseval=0;
	double detN=0;
	for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){

                if(((MNStruct *)globalcontext)->numFitEQUAD < 2 && ((MNStruct *)globalcontext)->numFitEFAC < 2){
			if(((MNStruct *)globalcontext)->whitemodel == 0){
				noiseval=pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6))*EFAC[0],2) + EQUAD[0];
			}
			else if(((MNStruct *)globalcontext)->whitemodel == 1){
				noiseval= EFAC[0]*EFAC[0]*(pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6)),2) + EQUAD[0]);
			}
                }
                else if(((MNStruct *)globalcontext)->numFitEQUAD > 1 || ((MNStruct *)globalcontext)->numFitEFAC > 1){
			if(((MNStruct *)globalcontext)->whitemodel == 0){
                       	 	noiseval=pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6))*EFAC[((MNStruct *)globalcontext)->sysFlags[o]],2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]];
			}
			else if(((MNStruct *)globalcontext)->whitemodel == 1){
                       	 	noiseval=EFAC[((MNStruct *)globalcontext)->sysFlags[o]]*EFAC[((MNStruct *)globalcontext)->sysFlags[o]]*(pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6)),2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]]);
			}
                }
	
	

		Chisq += pow(Resvec[o],2)/noiseval;
		detN += log(noiseval);
	}
	
	double lnew = 0;
	if(isnan(detN) || isinf(detN) || isnan(Chisq) || isinf(Chisq)){

		lnew=-pow(10.0,200);
	}
	else{
		lnew = -0.5*(((MNStruct *)globalcontext)->pulse->nobs*log(2*M_PI) + detN + Chisq);	
	}

	delete[] EFAC;
	delete[] Resvec;
	if(((MNStruct *)globalcontext)->incStep > 0){
		for(int i = 0; i < ((MNStruct *)globalcontext)->incStep; i++){
			delete[] Steps[i];
		}
		delete[] Steps;
	}

	return lnew;
}



void LRedLogLike(double *Cube, int &ndim, int &npars, double &lnew, void *globalcontext)
{

	clock_t startClock,endClock;
	double phase=0;
	double *EFAC;
	double *EQUAD;
	int pcount=0;
	int bad=0;
	int numfit=((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps;
	long double LDparams[numfit];
	double Fitparams[numfit];
	double *Resvec=new double[((MNStruct *)globalcontext)->pulse->nobs];

	for(int p=0;p<ndim;p++){

		Cube[p]=(((MNStruct *)globalcontext)->Dpriors[p][1]-((MNStruct *)globalcontext)->Dpriors[p][0])*Cube[p]+((MNStruct *)globalcontext)->Dpriors[p][0];
	}

	if(((MNStruct *)globalcontext)->doLinear==0){
	
		for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){
			LDparams[p]=Cube[p]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		}

		phase=(double)LDparams[0];
		pcount++;
		for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
			((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
			pcount++;
		}
		for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
			((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
			pcount++;
		}
		
		if(((MNStruct *)globalcontext)->pulse->param[param_dmmodel].fitFlag[0] == 1){
			int DMnum=((MNStruct *)globalcontext)->pulse[0].dmoffsDMnum;
			for(int i =0; i < DMnum; i++){
				((MNStruct *)globalcontext)->pulse[0].dmoffsDM[i]=Cube[ndim-DMnum+i];
			}
		}
		
		
		fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);      
		formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,1);       
		
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			Resvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].residual+phase;
			
		}
// 		printf("Phase: %g \n", phase);
	
	}
	else if(((MNStruct *)globalcontext)->doLinear==1){
	
		for(int p=0;p < numfit; p++){
			Fitparams[p]=Cube[p];
			pcount++;
		}
		
		double *Fitvec=new double[((MNStruct *)globalcontext)->pulse->nobs];

		dgemv(((MNStruct *)globalcontext)->DMatrix,Fitparams,Fitvec,((MNStruct *)globalcontext)->pulse->nobs,numfit,'N');
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			Resvec[o]=((MNStruct *)globalcontext)->pulse->obsn[o].residual-Fitvec[o];
		}
		
		delete[] Fitvec;
	}

	if(((MNStruct *)globalcontext)->incStep > 0){
		for(int i = 0; i < ((MNStruct *)globalcontext)->incStep; i++){
			double StepAmp = Cube[pcount];
			pcount++;
			double StepTime = Cube[pcount];
			pcount++;
			for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
				double time = (double)((MNStruct *)globalcontext)->pulse->obsn[o1].bat;
				if(time > StepTime){
					Resvec[o1] += StepAmp;
				}
			}
		}
	}
		

	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=Cube[pcount];
		}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=Cube[pcount];
			pcount++;
		}
	}				

	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,2*Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,2*Cube[pcount]);
			pcount++;
                }
        }

	int FitRedCoeff=2*(((MNStruct *)globalcontext)->numFitRedCoeff);
	int FitDMCoeff=2*(((MNStruct *)globalcontext)->numFitDMCoeff);
	if(((MNStruct *)globalcontext)->incFloatDM != 0)FitDMCoeff+=2*((MNStruct *)globalcontext)->incFloatDM;
	if(((MNStruct *)globalcontext)->incFloatRed != 0)FitRedCoeff+=2*((MNStruct *)globalcontext)->incFloatRed;
    	int totCoeff=0;
    	if(((MNStruct *)globalcontext)->incRED != 0)totCoeff+=FitRedCoeff;
    	if(((MNStruct *)globalcontext)->incDM != 0)totCoeff+=FitDMCoeff;

	double *WorkNoise=new double[((MNStruct *)globalcontext)->pulse->nobs];
	double *powercoeff=new double[totCoeff];

	double tdet=0;
	double timelike=0;
	double timelike2=0;

	if(((MNStruct *)globalcontext)->whitemodel == 0){
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			
			WorkNoise[o]=pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6))*EFAC[((MNStruct *)globalcontext)->sysFlags[o]],2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]];
			
			tdet=tdet+log(WorkNoise[o]);
			WorkNoise[o]=1.0/WorkNoise[o];
			timelike=timelike+pow(Resvec[o],2)*WorkNoise[o];
			timelike2=timelike2+pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].residual,2)*WorkNoise[o];

		}
	}
	else if(((MNStruct *)globalcontext)->whitemodel == 1){
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			
			WorkNoise[o]=EFAC[((MNStruct *)globalcontext)->sysFlags[o]]*EFAC[((MNStruct *)globalcontext)->sysFlags[o]]*(pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6)),2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]]);
			
			tdet=tdet+log(WorkNoise[o]);
			WorkNoise[o]=1.0/WorkNoise[o];
			timelike=timelike+pow(Resvec[o],2)*WorkNoise[o];
			timelike2=timelike2+pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].residual,2)*WorkNoise[o];

		}
	}

	
	double *NFd = new double[totCoeff];
	double **FMatrix=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		FMatrix[i]=new double[totCoeff];
	}

	double **NF=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		NF[i]=new double[totCoeff];
	}

	double **FNF=new double*[totCoeff];
	for(int i=0;i<totCoeff;i++){
		FNF[i]=new double[totCoeff];
	}





	double start,end;
	int go=0;
	for (int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++)
	  {
	    if (((MNStruct *)globalcontext)->pulse->obsn[i].deleted==0)
	      {
		if (go==0)
		  {
		    go = 1;
		    start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    end  = start;
		  }
		else
		  {
		    if (start > (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    if (end < (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      end = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		  }
	      }
	  }
// 	printf("Total time span = %.6f days = %.6f years\n",end-start,(end-start)/365.25);
	double maxtspan=end-start;

        double *freqs = new double[totCoeff];

        double *DMVec=new double[((MNStruct *)globalcontext)->pulse->nobs];
        double DMKappa = 2.410*pow(10.0,-16);
        int startpos=0;
        double freqdet=0;
        if(((MNStruct *)globalcontext)->incRED==2){
                for (int i=0; i<FitRedCoeff/2; i++){
                        int pnum=pcount;
                        double pc=Cube[pcount];
                        freqs[i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
                        freqs[i+FitRedCoeff/2]=freqs[i];

                        powercoeff[i]=pow(10.0,pc)/(maxtspan*24*60*60);///(365.25*24*60*60)/4;
                        powercoeff[i+FitRedCoeff/2]=powercoeff[i];
                        freqdet=freqdet+2*log(powercoeff[i]);
                        pcount++;
                }


                for(int i=0;i<FitRedCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);
                        }
                }

                for(int i=0;i<FitRedCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);
                        }
                }



                startpos=FitRedCoeff;

        }
   else if(((MNStruct *)globalcontext)->incRED==3){

                double redamp=Cube[pcount];
                pcount++;
                double redindex=Cube[pcount];
                pcount++;
// 		printf("red: %g %g \n", redamp, redindex);
                 redamp=pow(10.0, redamp);

                freqdet=0;
                 for (int i=0; i<FitRedCoeff/2; i++){

                        freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
                        freqs[startpos+i+FitRedCoeff/2]=freqs[startpos+i];

                        powercoeff[i]=redamp*redamp*pow((freqs[i]*365.25),-1.0*redindex)/(maxtspan*24*60*60);///(365.25*24*60*60)/4;
                        powercoeff[i+FitRedCoeff/2]=powercoeff[i];
                        freqdet=freqdet+2*log(powercoeff[i]);


                 }

                for(int i=0;i<FitRedCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);
                        }
                }

                for(int i=0;i<FitRedCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);
                        }
                }


                startpos=FitRedCoeff;

        }


       if(((MNStruct *)globalcontext)->incDM==2){

                for (int i=0; i<FitDMCoeff/2; i++){
                        int pnum=pcount;
                        double pc=Cube[pcount];
                        freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[startpos+i]/maxtspan;
                        freqs[startpos+i+FitDMCoeff/2]=freqs[startpos+i];

                        powercoeff[startpos+i]=pow(10.0,pc)/(maxtspan*24*60*60);///(365.25*24*60*60)/4;
                        powercoeff[startpos+i+FitDMCoeff/2]=powercoeff[startpos+i];
                        freqdet=freqdet+2*log(powercoeff[startpos+i]);
                        pcount++;
                }

                for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                        DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                }

                for(int i=0;i<FitDMCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                        }
                }

                for(int i=0;i<FitDMCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][startpos+i+FitDMCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                        }
                }



        }
        else if(((MNStruct *)globalcontext)->incDM==3){
                double DMamp=Cube[pcount];
                pcount++;
                double DMindex=Cube[pcount];
                pcount++;

                DMamp=pow(10.0, DMamp);

                 for (int i=0; i<FitDMCoeff/2; i++){
                        freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[startpos+i]/maxtspan;
                        freqs[startpos+i+FitDMCoeff/2]=freqs[startpos+i];

                        powercoeff[startpos+i]=DMamp*DMamp*pow((freqs[startpos+i]*365.25),-1.0*DMindex)/(maxtspan*24*60*60);///(365.25*24*60*60)/4;
                        powercoeff[startpos+i+FitDMCoeff/2]=powercoeff[startpos+i];
                        freqdet=freqdet+2*log(powercoeff[startpos+i]);


                 }
                for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                        DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                }


                for(int i=0;i<FitDMCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                        }
                }

                for(int i=0;i<FitDMCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][startpos+i+FitDMCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                        }
                }



        }





// 	makeFourier(((MNStruct *)globalcontext)->pulse, FitCoeff, FMatrix);

	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		for(int j=0;j<totCoeff;j++){
// 			printf("%i %i %g %g \n",i,j,WorkNoise[i],FMatrix[i][j]);
			NF[i][j]=WorkNoise[i]*FMatrix[i][j];
		}
	}
	dgemv(NF,Resvec,NFd,((MNStruct *)globalcontext)->pulse->nobs,totCoeff,'T');
	dgemm(FMatrix, NF , FNF, ((MNStruct *)globalcontext)->pulse->nobs, totCoeff, ((MNStruct *)globalcontext)->pulse->nobs, totCoeff, 'T', 'N');


	double **PPFM=new double*[totCoeff];
	for(int i=0;i<totCoeff;i++){
		PPFM[i]=new double[totCoeff];
		for(int j=0;j<totCoeff;j++){
			PPFM[i][j]=0;
		}
	}


	for(int c1=0; c1<totCoeff; c1++){
		PPFM[c1][c1]=1.0/powercoeff[c1];
	}



	for(int j=0;j<totCoeff;j++){
		for(int k=0;k<totCoeff;k++){
			PPFM[j][k]=PPFM[j][k]+FNF[j][k];
		}
	}

        double jointdet=0;
        double freqlike=0;
       double *WorkCoeff = new double[totCoeff];
       for(int o1=0;o1<totCoeff; o1++){
                WorkCoeff[o1]=NFd[o1];
        }


	    int info=0;
        dpotrfInfo(PPFM, totCoeff, jointdet,info);
        dpotrs(PPFM, WorkCoeff, totCoeff);
        for(int j=0;j<totCoeff;j++){
                freqlike += NFd[j]*WorkCoeff[j];
        }
	
	lnew=-0.5*(jointdet+tdet+freqdet+timelike-freqlike);

	if(isnan(lnew) || isinf(lnew)){

		lnew=-pow(10.0,200);
		
	}
	
	//printf("CPULike: %g %g %g %g %g %g \n", lnew, jointdet, tdet, freqdet, timelike, freqlike);
	delete[] DMVec;
	delete[] WorkCoeff;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] WorkNoise;
	delete[] powercoeff;
	delete[] Resvec;
	delete[] NFd;
	delete[] freqs;

	for (int j = 0; j < totCoeff; j++){
		delete[] PPFM[j];
	}
	delete[] PPFM;

	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
		delete[] NF[j];
	}
	delete[] NF;

	for (int j = 0; j < totCoeff; j++){
		delete[] FNF[j];
	}
	delete[] FNF;

	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
		delete[] FMatrix[j];
	}
	delete[] FMatrix;

}

*/

double  FastNewLRedMarginLogLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){




	
	printf("In fast like\n");
	int pcount=0;
	
	int numfit=((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps;
	long double LDparams[numfit];
	double Fitparams[numfit];
	double *Resvec=new double[((MNStruct *)globalcontext)->pulse->nobs];
	int fitcount=0;
	
	pcount=0;

	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){
		if(((MNStruct *)globalcontext)->Dpriors[p][1] != ((MNStruct *)globalcontext)->Dpriors[p][0]){

			double val = 0;
			if((((MNStruct *)globalcontext)->LDpriors[p][3]) == 0){
				val = Cube[fitcount];
			}
			if((((MNStruct *)globalcontext)->LDpriors[p][3]) == 1){
				val = pow(10.0,Cube[fitcount]);
			}
			LDparams[p]=val*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
			fitcount++;
		}
		else if(((MNStruct *)globalcontext)->Dpriors[p][1] == ((MNStruct *)globalcontext)->Dpriors[p][0]){
			LDparams[p]=((MNStruct *)globalcontext)->Dpriors[p][0]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		}


	}
	pcount=0;
	double phase=(double)LDparams[0];
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	
	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,1);       
	
	for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		Resvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].residual+phase;

	}

	
	pcount=fitcount;

	if(((MNStruct *)globalcontext)->incStep > 0){
		for(int i = 0; i < ((MNStruct *)globalcontext)->incStep; i++){
			double StepAmp = Cube[pcount];
			pcount++;
			double StepTime = Cube[pcount];
			pcount++;
			for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
				if(((MNStruct *)globalcontext)->pulse->obsn[o1].bat > StepTime){
					Resvec[o1] += StepAmp;
				}
			}
		}
	}	


/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Subtract GLitches/////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	for(int i = 0 ; i < ((MNStruct *)globalcontext)->incGlitch; i++){
		double GlitchMJD = Cube[pcount];
		pcount++;

		double *GlitchAmps = new double[2];
		if(((MNStruct *)globalcontext)->incGlitchTerms==1 || ((MNStruct *)globalcontext)->incGlitchTerms==3){
			GlitchAmps[0]  = Cube[pcount];
			pcount++;
		}
		else if(((MNStruct *)globalcontext)->incGlitchTerms==2){
			GlitchAmps[0]  = Cube[pcount];
                        pcount++;
			GlitchAmps[1]  = Cube[pcount];
                        pcount++;
		}


		for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
                        if(((MNStruct *)globalcontext)->pulse->obsn[o1].bat > GlitchMJD){

				if(((MNStruct *)globalcontext)->incGlitchTerms==1){

					long double arg=0;
					arg=((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0])*86400.0;
					double darg = (double)arg;
                                        Resvec[o1] += GlitchAmps[0]*darg;

				}
				else if(((MNStruct *)globalcontext)->incGlitchTerms==2){
					for(int j = 0; j < ((MNStruct *)globalcontext)->incGlitchTerms; j++){

						long double arg=0;
						if(j==0){
							arg=((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0])*86400.0;
						}
						if(j==1){
							arg=0.5*pow((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)*86400.0,2)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0];
						}
						double darg = (double)arg;
						Resvec[o1] += GlitchAmps[j]*darg;
					}
				}
				else if(((MNStruct *)globalcontext)->incGlitchTerms==3){
					long double arg=0;
					arg=0.5*pow((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)*86400.0,2)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0];
					double darg = (double)arg;
                                        Resvec[o1] += GlitchAmps[0]*darg;
				}


			}
		}
	}
	
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get White Noise vector///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  




	double *Noise;	
	Noise=new double[((MNStruct *)globalcontext)->pulse->nobs];
	
	
	for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		Noise[o] = ((MNStruct *)globalcontext)->PreviousNoise[o];
	}
		
	

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get Time domain likelihood//////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 


/* 
	static unsigned int oldcw;
	if(((MNStruct *)globalcontext)->uselongdouble > 0 ){
		fpu_fix_start(&oldcw);
	}
*/	


	double tdet=0;
	double timelike=0;

	for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		timelike+=Resvec[o]*Resvec[o]*Noise[o];
		tdet -= log(Noise[o]);
	}
/*
	dd_real ddtimelike=0.0;
	if(((MNStruct *)globalcontext)->uselongdouble ==1){
		for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			dd_real res = (dd_real)Resvec[o];
			dd_real Nval = (dd_real)Noise[o];
			dd_real chicomp = res*res*Nval;
			ddtimelike += chicomp;
		}
	}

	
        qd_real qdtimelike=0.0;
        if(((MNStruct *)globalcontext)->uselongdouble ==2){
                for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){

                        qd_real res = (qd_real)Resvec[o];
                        qd_real Nval = (qd_real)Noise[o];
                        qd_real chicomp = res*res*Nval;

                        qdtimelike+=chicomp;
                }
        }
*/

	printf("Fast time like %g %g %i\n", timelike, tdet, ((MNStruct *)globalcontext)->totCoeff);

//////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////Do Algebra/////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


        int totCoeff=((MNStruct *)globalcontext)->totCoeff;

        int TimetoMargin=0;
        for(int i =0; i < ((MNStruct *)globalcontext)->numFitTiming+((MNStruct *)globalcontext)->numFitJumps; i++){
                if(((MNStruct *)globalcontext)->LDpriors[i][2]==1)TimetoMargin++;
        }
	
	


	int totalsize=totCoeff + TimetoMargin;

	double *NTd = new double[totalsize];

	double **NT=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		NT[i]=new double[totalsize];
		for(int j=0;j<totalsize;j++){
                        NT[i][j]=((MNStruct *)globalcontext)->PreviousNT[i][j];
                }

	}

	double **TNT=new double*[totalsize];
	for(int i=0;i<totalsize;i++){
		TNT[i]=new double[totalsize];
		for(int j=0;j<totalsize;j++){
			TNT[i][j] = ((MNStruct *)globalcontext)->PreviousTNT[i][j];
		}
	}
	

	



	dgemv(NT,Resvec,NTd,((MNStruct *)globalcontext)->pulse->nobs,totalsize,'T');



	double freqlike=0;
	double *WorkCoeff = new double[totalsize];
	for(int o1=0;o1<totalsize; o1++){
	    WorkCoeff[o1]=NTd[o1];
	}

	int globalinfo=0;
	int info=0;
	double jointdet = ((MNStruct *)globalcontext)->PreviousJointDet;	
	double freqdet = ((MNStruct *)globalcontext)->PreviousFreqDet;
	double uniformpriorterm = ((MNStruct *)globalcontext)->PreviousUniformPrior;

	info=0;
	dpotrsInfo(TNT, WorkCoeff, totalsize, info);
	if(info != 0)globalinfo=1;


	for(int j=0;j<totalsize;j++){
	    freqlike += NTd[j]*WorkCoeff[j];

	}


	double lnew = -0.5*(tdet+jointdet+freqdet+timelike-freqlike) + uniformpriorterm;
	
	if(isnan(lnew) || isinf(lnew) || globalinfo != 0){

		lnew=-pow(10.0,20);
		
	}

/*
        if(((MNStruct *)globalcontext)->uselongdouble > 0 ){
                fpu_fix_end(&oldcw);
        }
*/


	delete[] WorkCoeff;
	delete[] NTd;
	delete[] Noise;
	delete[] Resvec;
	
	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
		delete[]NT[j];
	}
	delete[]NT;

	for (int j = 0; j < totalsize; j++){
		delete[]TNT[j];
	}
	delete[]TNT;

	return lnew;


}


void LRedLikeMNWrap(double *Cube, int &ndim, int &npars, double &lnew, void *context){



        for(int p=0;p<ndim;p++){

                Cube[p]=(((MNStruct *)globalcontext)->PriorsArray[p+ndim]-((MNStruct *)globalcontext)->PriorsArray[p])*Cube[p]+((MNStruct *)globalcontext)->PriorsArray[p];
        }


	
	double *DerivedParams = new double[npars];

	double result = NewLRedMarginLogLike(ndim, Cube, npars, DerivedParams, context);


	delete[] DerivedParams;

	lnew = result;

}

double  NewLRedMarginLogLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){

	double uniformpriorterm=0;
	clock_t startClock,endClock;

	double **EFAC;
	double *EQUAD;
	int pcount=0;
	
	int numfit=((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps;
	int TimetoMargin=((MNStruct *)globalcontext)->TimetoMargin;
	long double LDparams[numfit];
	for(int i = 0; i < numfit; i++){LDparams[i]=0;}
	double Fitparams[numfit];
	double *Resvec=new double[((MNStruct *)globalcontext)->pulse->nobs];
	int fitcount=0;
	

	if(((MNStruct *)globalcontext)->doGrades == 1){
		int slowlike = 0;
		double myeps = pow(10.0, -10);
		for(int i = 0; i < ndim; i ++){
			
			int slowindex = ((MNStruct *)globalcontext)->hypercube_indices[i]-1;


			 printf("Cube: %i %i %i %g %g %g \n", i, slowindex, ((MNStruct *)globalcontext)->PolyChordGrades[slowindex], Cube[i], ((MNStruct *)globalcontext)->LastParams[i], fabs(Cube[i] - ((MNStruct *)globalcontext)->LastParams[i]));

			if(((MNStruct *)globalcontext)->PolyChordGrades[slowindex] == 1){

//				printf("Slow Cube: %i %g %g %g \n", i, Cube[slowindex], ((MNStruct *)globalcontext)->LastParams[slowindex], fabs(Cube[slowindex] - ((MNStruct *)globalcontext)->LastParams[slowindex]));

				if(fabs(Cube[slowindex] - ((MNStruct *)globalcontext)->LastParams[slowindex]) > myeps){
					slowlike = 1;
				}
			}

			((MNStruct *)globalcontext)->LastParams[i] = Cube[i];
		}

		if(slowlike == 0){

			double lnew = 0;
			if(((MNStruct *)globalcontext)->PreviousInfo == 0){
				lnew = FastNewLRedMarginLogLike(ndim, Cube, npars, DerivedParams, context);
			}
			else if(((MNStruct *)globalcontext)->PreviousInfo == 1){
				lnew = -pow(10.0,20);
			}

			return lnew;
		}
	}
				

	pcount=0;
	///////////////hacky glitch thing for Vela, comment this out for *anything else*/////////////////////////
	//for(int p=1;p<9; p++){
		//Cube[p] = Cube[0];
	//	printf("Cube 0 %g \n", Cube[0]);
	//}
        //for(int p=10;p<17; p++){
        //      Cube[p] = Cube[9];
	//	printf("Cube 9 %g\n", Cube[9]);
        //}

	///////////////end of hacky glitch thing for Vela///////////////////////////////////////////////////////

	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){
		if(((MNStruct *)globalcontext)->Dpriors[p][1] != ((MNStruct *)globalcontext)->Dpriors[p][0]){
			double val = 0;
			if((((MNStruct *)globalcontext)->LDpriors[p][3]) == 0){
				val = Cube[fitcount];
			}
			if((((MNStruct *)globalcontext)->LDpriors[p][3]) == 1){
				val = pow(10.0,Cube[fitcount]);
			}
                        if((((MNStruct *)globalcontext)->LDpriors[p][3]) == 2){
                                val = pow(10.0,Cube[fitcount]);
				uniformpriorterm += log(val);
                        }


			LDparams[p]=val*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
			

			if(((MNStruct *)globalcontext)->TempoFitNums[p][0] == param_sini && ((MNStruct *)globalcontext)->usecosiprior == 1){
					val = Cube[fitcount];
					LDparams[p] = sqrt(1.0 - val*val);
			}

			fitcount++;

		}
		else if(((MNStruct *)globalcontext)->Dpriors[p][1] == ((MNStruct *)globalcontext)->Dpriors[p][0]){
			LDparams[p]=((MNStruct *)globalcontext)->Dpriors[p][0]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		}


	}

	pcount=0;
	double phase=(double)LDparams[0];
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}


	if(TimetoMargin != numfit){
		fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       /* Form Barycentric arrival times */
		formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,1);       /* Form residuals */
	}
	
	for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
	
		Resvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].residual+phase;
		//printf("Res: %i %g %g %g %g %g %g %g\n", o, Resvec[o], (double) phase, ((MNStruct *)globalcontext)->Dpriors[0][0], ((MNStruct *)globalcontext)->Dpriors[0][1], (double)((MNStruct *)globalcontext)->LDpriors[0][1] , (double)((MNStruct *)globalcontext)->LDpriors[0][0], (double)((MNStruct *)globalcontext)->pulse->obsn[o].residual);

	}

	
	pcount=fitcount;
	if(((MNStruct *)globalcontext)->incStep > 0){
		for(int i = 0; i < ((MNStruct *)globalcontext)->incStep; i++){


			int GrouptoFit=0;
			double GroupStartTime = 0;

			GrouptoFit = floor(Cube[pcount]);
			pcount++;

			double GLength = ((MNStruct *)globalcontext)->GroupStartTimes[GrouptoFit][1] - ((MNStruct *)globalcontext)->GroupStartTimes[GrouptoFit][0];
                        GroupStartTime = ((MNStruct *)globalcontext)->GroupStartTimes[GrouptoFit][0] + Cube[pcount]*GLength;
	                pcount++;

			double StepAmp = Cube[pcount];
			pcount++;

			//printf("Step details: Group %i S %g F %g SS %g A %g \n", GrouptoFit, ((MNStruct *)globalcontext)->GroupStartTimes[GrouptoFit][0],((MNStruct *)globalcontext)->GroupStartTimes[GrouptoFit][1],GroupStartTime,StepAmp);

			for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
				if(((MNStruct *)globalcontext)->pulse->obsn[o1].sat > GroupStartTime && ((MNStruct *)globalcontext)->GroupNoiseFlags[o1] == GrouptoFit ){
					Resvec[o1] += StepAmp;
				}
			}
		}
	}	


/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Subtract GLitches/////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	for(int i = 0 ; i < ((MNStruct *)globalcontext)->incGlitch; i++){
		double GlitchMJD = Cube[pcount];
		pcount++;

		double *GlitchAmps = new double[3];
		if(((MNStruct *)globalcontext)->incGlitchTerms==1){
			GlitchAmps[0]  = Cube[pcount];
			pcount++;
		}
		else if(((MNStruct *)globalcontext)->incGlitchTerms==2){
			GlitchAmps[0]  = Cube[pcount];
                        pcount++;
			GlitchAmps[1]  = Cube[pcount];
                        pcount++;
		}
		else if(((MNStruct *)globalcontext)->incGlitchTerms==3){
			GlitchAmps[0]  = Cube[pcount];
                        pcount++;
			GlitchAmps[1]  = pow(10.0, Cube[pcount]); //Decay Amp
                        pcount++;
			GlitchAmps[2]  = pow(10.0, Cube[pcount]); //Decay Timescale
                        pcount++;
		}

		for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
                        if(((MNStruct *)globalcontext)->pulse->obsn[o1].bat > GlitchMJD){

				if(((MNStruct *)globalcontext)->incGlitchTerms==1){

					long double arg=0;
					arg=((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0])*86400.0;
					double darg = (double)arg;
                                        Resvec[o1] += GlitchAmps[0]*darg;

				}
				else if(((MNStruct *)globalcontext)->incGlitchTerms==2){
					for(int j = 0; j < ((MNStruct *)globalcontext)->incGlitchTerms; j++){

						long double arg=0;
						if(j==0){
							arg=((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0])*86400.0;
						}
						if(j==1){
							arg=0.5*pow((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)*86400.0,2)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0];
						}
						double darg = (double)arg;
						Resvec[o1] += GlitchAmps[j]*darg;
					}
				}
				else if(((MNStruct *)globalcontext)->incGlitchTerms==3){
					long double arg=0;
					double time = (((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD);
					arg=((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0])*86400.0;
					double darg = (double)arg;
                                        Resvec[o1] += GlitchAmps[0]*darg + GlitchAmps[1]*darg*exp(-1*time/GlitchAmps[2]);
				}


			}
		}

		delete[] GlitchAmps;
	}
	
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get White Noise vector///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double*[((MNStruct *)globalcontext)->EPolTerms];
		for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
			EFAC[n-1]=new double[((MNStruct *)globalcontext)->systemcount];
			if(n==1){
				for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
					EFAC[n-1][o]=1;
				}
			}
			else{
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                   EFAC[n-1][o]=0;
                }
			}
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double*[((MNStruct *)globalcontext)->EPolTerms];
		for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
			
			EFAC[n-1]=new double[((MNStruct *)globalcontext)->systemcount];
			if(n==1){
				for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
					EFAC[n-1][o]=pow(10.0,Cube[pcount]);
					if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[n-1][o]);}
				}
				pcount++;
			}
			else{
                                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){

                                        EFAC[n-1][o]=pow(10.0,Cube[pcount]);
                                }
                                pcount++;
                        }
		}
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double*[((MNStruct *)globalcontext)->EPolTerms];
		for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
			EFAC[n-1]=new double[((MNStruct *)globalcontext)->systemcount];
			if(n==1){
				for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
					EFAC[n-1][p]=pow(10.0,Cube[pcount]);
					if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[n-1][p]);}
					pcount++;
				}
			}
			else{
                                for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
                                        EFAC[n-1][p]=pow(10.0,Cube[pcount]);
                                        pcount++;
                                }
                        }
		}
	}	

		
	//printf("Equad %i \n", ((MNStruct *)globalcontext)->numFitEQUAD);
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,2*Cube[pcount]);
			if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount]));}
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){

			if(((MNStruct *)globalcontext)->includeEQsys[o] == 1){
				//printf("Cube: %i %i %g \n", o, pcount, Cube[pcount]);
				EQUAD[o]=pow(10.0,2*Cube[pcount]);
				if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount])); }
				pcount++;
			}
			else{
				EQUAD[o]=0;
			}
			//printf("Equad? %i %g \n", o, EQUAD[o]);
		}
    	}
    

        double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,2*Cube[pcount]);
			if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount]));}
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
            SQUAD[o]=pow(10.0,2*Cube[pcount]);
	    if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount])); }

			pcount++;
        }
    }
 
	double *ECORRPrior;
	if(((MNStruct *)globalcontext)->incNGJitter >0){
		double *ECorrCoeffs=new double[((MNStruct *)globalcontext)->incNGJitter];	
		for(int i =0; i < ((MNStruct *)globalcontext)->incNGJitter; i++){
			ECorrCoeffs[i] = pow(10.0, 2*Cube[pcount]);
			if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount])); }
			pcount++;
		}
    		ECORRPrior = new double[((MNStruct *)globalcontext)->numNGJitterEpochs];
		for(int i =0; i < ((MNStruct *)globalcontext)->numNGJitterEpochs; i++){
			ECORRPrior[i] = ECorrCoeffs[((MNStruct *)globalcontext)->NGJitterSysFlags[i]];
		}

		delete[] ECorrCoeffs;
	} 

	double DMEQUAD = 0;
	if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
		DMEQUAD = pow(10.0, Cube[pcount]);
		pcount++;
	}

	double SolarWind=0;
	double WhiteSolarWind = 0;

	if(((MNStruct *)globalcontext)->FitSolarWind == 1){
		SolarWind = Cube[pcount];
		pcount++;

		//printf("Solar Wind: %g \n", SolarWind);
	}
	if(((MNStruct *)globalcontext)->FitWhiteSolarWind == 1){
		WhiteSolarWind = pow(10.0, Cube[pcount]);
		pcount++;

		//printf("White Solar Wind: %g \n", WhiteSolarWind);
	}



	double *Noise;	
	double *BATvec;
	Noise=new double[((MNStruct *)globalcontext)->pulse->nobs];
	//BATvec=new double[((MNStruct *)globalcontext)->pulse->nobs];
	
	
	//for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		//BATvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].bat;
	//}
		
		

	double DMKappa = 2.410*pow(10.0,-16);
	if(((MNStruct *)globalcontext)->whitemodel == 0){
	
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			double EFACterm=0;
			double noiseval=0;
			double ShannonJitterTerm=0;

			double SWTerm = WhiteSolarWind*((MNStruct *)globalcontext)->pulse->obsn[o].tdis2/((MNStruct *)globalcontext)->pulse->ne_sw;
			double DMEQUADTerm = DMEQUAD/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));	
						
			
			if(((MNStruct *)globalcontext)->useOriginalErrors==0){
				noiseval=((MNStruct *)globalcontext)->pulse->obsn[o].toaErr;
			}
			else if(((MNStruct *)globalcontext)->useOriginalErrors==1){
				noiseval=((MNStruct *)globalcontext)->pulse->obsn[o].origErr;
			}


			for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
				EFACterm=EFACterm + pow((noiseval*pow(10.0,-6))/pow(pow(10.0,-7),n-1),n)*EFAC[n-1][((MNStruct *)globalcontext)->sysFlags[o]];
			}	
			
			if(((MNStruct *)globalcontext)->incShannonJitter > 0){	
			 	ShannonJitterTerm=SQUAD[((MNStruct *)globalcontext)->sysFlags[o]]*((MNStruct *)globalcontext)->pulse->obsn[o].tobs/1000.0;
			}
			//printf("Noise: %i %g %g %g %g %g \n", EFACterm, EQUAD[((MNStruct *)globalcontext)->sysFlags[o]], ShannonJitterTerm, SWTerm, DMEQUADTerm);
			Noise[o]= 1.0/(pow(EFACterm,2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]]+ShannonJitterTerm + SWTerm*SWTerm + DMEQUADTerm*DMEQUADTerm);

		}
		
	}
	else if(((MNStruct *)globalcontext)->whitemodel == 1){
	
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){

			Noise[o]=1.0/(EFAC[0][((MNStruct *)globalcontext)->sysFlags[o]]*EFAC[0][((MNStruct *)globalcontext)->sysFlags[o]]*(pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6)),2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]]));
		}
		
	}

	

/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Initialise TotalMatrix////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////// 

	int totalsize = ((MNStruct *)globalcontext)->totalsize;
	
	double *TotalMatrix=((MNStruct *)globalcontext)->StoredTMatrix;


		
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form the Design Matrix////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////  


	if(TimetoMargin != ((MNStruct *)globalcontext)->numFitTiming+((MNStruct *)globalcontext)->numFitJumps){

		getCustomDVectorLike(globalcontext, TotalMatrix, ((MNStruct *)globalcontext)->pulse->nobs, TimetoMargin, totalsize);
		vector_dgesvd(TotalMatrix,((MNStruct *)globalcontext)->pulse->nobs, TimetoMargin);

		
	}




//////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////////////Set up Coefficients///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////  


	double maxtspan=((MNStruct *)globalcontext)->Tspan;
	double averageTSamp=2*maxtspan/((MNStruct *)globalcontext)->pulse->nobs;



	int FitRedCoeff=2*(((MNStruct *)globalcontext)->numFitRedCoeff);
	int FitDMCoeff=2*(((MNStruct *)globalcontext)->numFitDMCoeff);
	int FitBandCoeff=2*(((MNStruct *)globalcontext)->numFitBandNoiseCoeff);
	int FitGroupNoiseCoeff = 2*((MNStruct *)globalcontext)->numFitGroupNoiseCoeff;

	int totCoeff=((MNStruct *)globalcontext)->totCoeff;




	double *powercoeff=new double[totCoeff];
	for(int o=0;o<totCoeff; o++){
		powercoeff[o]=0;
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Red Noise///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	double *freqs = new double[totCoeff];
	double *DMVec=new double[((MNStruct *)globalcontext)->pulse->nobs];

	
	double freqdet=0;
	int startpos=0;

	if(((MNStruct *)globalcontext)->incRED > 0 || ((MNStruct *)globalcontext)->incGWB == 1){


		if(((MNStruct *)globalcontext)->FitLowFreqCutoff == 1){
			double fLow = pow(10.0, Cube[pcount]);
			pcount++;

			double deltaLogF = 0.1;
			double RedMidFreq = 2.0;

			double RedLogDiff = log10(RedMidFreq) - log10(fLow);
			int LogLowFreqs = floor(RedLogDiff/deltaLogF);

			double RedLogSampledDiff = LogLowFreqs*deltaLogF;
			double sampledFLow = floor(log10(fLow)/deltaLogF)*deltaLogF;
			
			int freqStartpoint = 0;


			for(int i =0; i < LogLowFreqs; i++){
				((MNStruct *)globalcontext)->sampleFreq[freqStartpoint]=pow(10.0, sampledFLow + i*RedLogSampledDiff/LogLowFreqs);
				freqStartpoint++;

			}

			for(int i =0;i < FitRedCoeff/2-LogLowFreqs; i++){
				((MNStruct *)globalcontext)->sampleFreq[freqStartpoint]=i+RedMidFreq;
				freqStartpoint++;
			}

		}

                if(((MNStruct *)globalcontext)->FitLowFreqCutoff == 2){
                        double fLow = pow(10.0, Cube[pcount]);
                        pcount++;

                        for(int i =0;i < FitRedCoeff/2; i++){
                                ((MNStruct *)globalcontext)->sampleFreq[i]=((double)(i+1))*fLow;
                        }


                }

		for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
			
			freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
			freqs[startpos+i+FitRedCoeff/2]=freqs[startpos+i];

		}

	}


	if(((MNStruct *)globalcontext)->storeFMatrices == 0){


		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
	
				TotalMatrix[k + (i+TimetoMargin+startpos)*((MNStruct *)globalcontext)->pulse->nobs]=cos(2*M_PI*freqs[i]*time);
				TotalMatrix[k + (i+FitRedCoeff/2+TimetoMargin+startpos)*((MNStruct *)globalcontext)->pulse->nobs] = sin(2*M_PI*freqs[i]*time);


			}
		}
	}

	if(((MNStruct *)globalcontext)->incRED==2){

    
		for (int i=0; i<FitRedCoeff/2; i++){
			int pnum=pcount;
			double pc=Cube[pcount];

			if(((MNStruct *)globalcontext)->RedPriorType ==1) { uniformpriorterm += log(pow(10.0,pc)); }

			powercoeff[i]=pow(10.0,2*pc);
			powercoeff[i+FitRedCoeff/2]=powercoeff[i];
			pcount++;
		}
		
		
	            
	    startpos = FitRedCoeff;

	}
	else if(((MNStruct *)globalcontext)->incRED==3 || ((MNStruct *)globalcontext)->incRED==4){

		
		for(int pl = 0; pl < ((MNStruct *)globalcontext)->numFitRedPL; pl ++){

			double Tspan = maxtspan;
                        double f1yr = 1.0/3.16e7;

			if(((MNStruct *)globalcontext)->FitLowFreqCutoff == 2){
				Tspan=Tspan/((MNStruct *)globalcontext)->sampleFreq[0];
			}



			double redamp=Cube[pcount];
			pcount++;
			double redindex=Cube[pcount];
			pcount++;

			
			double cornerfreq=0;
			if(((MNStruct *)globalcontext)->incRED==4){
				cornerfreq=pow(10.0, Cube[pcount])/Tspan;
				pcount++;
			}

	
			
			redamp=pow(10.0, redamp);
			if(((MNStruct *)globalcontext)->RedPriorType ==1) { uniformpriorterm +=log(redamp); }



			double Agw=redamp;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				
				
				double rho=0;
				if(((MNStruct *)globalcontext)->incRED==3){	
 					rho = (Agw*Agw/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-redindex))/(Tspan*24*60*60);
				}
				if(((MNStruct *)globalcontext)->incRED==4){
					
			rho = pow((1+(pow((1.0/365.25)/cornerfreq,redindex/2))),2)*(Agw*Agw/12.0/(M_PI*M_PI))/pow((1+(pow(freqs[i]/cornerfreq,redindex/2))),2)/(Tspan*24*60*60)*pow(f1yr,-3.0);
				}
				//if(rho > pow(10.0,15))rho=pow(10.0,15);
 				powercoeff[i]+= rho;
 				powercoeff[i+FitRedCoeff/2]+= rho;

				

			}
		}
		


		int coefftovary=0;
		double amptovary=0.0;
		if(((MNStruct *)globalcontext)->varyRedCoeff==1){
			coefftovary=int(pow(10.0,Cube[pcount]))-1;
			pcount++;
			amptovary=pow(10.0,Cube[pcount]);
			pcount++;

			powercoeff[coefftovary]=amptovary;
			powercoeff[coefftovary+FitRedCoeff/2]=amptovary;	
		}		
		

		

		startpos=FitRedCoeff;

    }


	if(((MNStruct *)globalcontext)->incGWB==1){
		double GWBAmp=pow(10.0,Cube[pcount]);
		pcount++;
		uniformpriorterm += log(GWBAmp);
		double Tspan = maxtspan;

		if(((MNStruct *)globalcontext)->FitLowFreqCutoff == 2){
			Tspan=Tspan/((MNStruct *)globalcontext)->sampleFreq[0];
		}

		double f1yr = 1.0/3.16e7;
		for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
			double rho = (GWBAmp*GWBAmp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-4.333))/(Tspan*24*60*60);
			powercoeff[i]+= rho;
			powercoeff[i+FitRedCoeff/2]+= rho;
			//printf("%i %g %g \n", i, freqs[i], powercoeff[i]);
		}

		startpos=FitRedCoeff;
	}

	for (int i=0; i<FitRedCoeff/2; i++){
		freqdet=freqdet+2*log(powercoeff[i]);
	}

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract  Red Shape Events////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


	int MarginRedShapelets = ((MNStruct *)globalcontext)->MarginRedShapeCoeff;
	int totalredshapecoeff = 0;
	int badshape = 0;

	double **RedShapeletMatrix;

	double globalwidth=0;
	if(((MNStruct *)globalcontext)->incRedShapeEvent != 0){


		if(MarginRedShapelets == 1){

			badshape = 1;

			totalredshapecoeff = ((MNStruct *)globalcontext)->numRedShapeCoeff*((MNStruct *)globalcontext)->incRedShapeEvent;

			

			RedShapeletMatrix = new double*[((MNStruct *)globalcontext)->pulse->nobs];
			for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
				RedShapeletMatrix[i] = new double[totalredshapecoeff];
					for(int j = 0; j < totalredshapecoeff; j++){
						RedShapeletMatrix[i][j] = 0;
					}
				}
			}


		        for(int i =0; i < ((MNStruct *)globalcontext)->incRedShapeEvent; i++){

				int numRedShapeCoeff=((MNStruct *)globalcontext)->numRedShapeCoeff;

		                double EventPos=Cube[pcount];
				pcount++;
		                double EventWidth=Cube[pcount];
				pcount++;

				globalwidth=EventWidth;



				double *RedshapeVec=new double[numRedShapeCoeff];

				double *RedshapeNorm=new double[numRedShapeCoeff];
				for(int c=0; c < numRedShapeCoeff; c++){
					RedshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
				}

				for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
					double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

					double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
					TNothpl(numRedShapeCoeff,HVal,RedshapeVec);

					for(int c=0; c < numRedShapeCoeff; c++){
						RedShapeletMatrix[k][i*numRedShapeCoeff+c] = RedshapeNorm[c]*RedshapeVec[c]*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
						if(RedShapeletMatrix[k][i*numRedShapeCoeff+c] > 0.01){ badshape = 0;}
						

					}

	
				}


				delete[] RedshapeVec;
				delete[] RedshapeNorm;

			}

		if(MarginRedShapelets == 0){

		        for(int i =0; i < ((MNStruct *)globalcontext)->incRedShapeEvent; i++){

				int numRedShapeCoeff=((MNStruct *)globalcontext)->numRedShapeCoeff;

		                double EventPos=Cube[pcount];
				pcount++;
		                double EventWidth=Cube[pcount];
				pcount++;

				globalwidth=EventWidth;


				double *Redshapecoeff=new double[numRedShapeCoeff];
				double *RedshapeVec=new double[numRedShapeCoeff];
				for(int c=0; c < numRedShapeCoeff; c++){
					Redshapecoeff[c]=Cube[pcount];
					pcount++;
				}


				double *RedshapeNorm=new double[numRedShapeCoeff];
				for(int c=0; c < numRedShapeCoeff; c++){
					RedshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
				}

				for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
					double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

					double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
					TNothpl(numRedShapeCoeff,HVal,RedshapeVec);
					double Redsignal=0;
					for(int c=0; c < numRedShapeCoeff; c++){
						Redsignal += RedshapeNorm[c]*RedshapeVec[c]*Redshapecoeff[c];
					}

					  Resvec[k] -= Redsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));

					  //printf("Shape Sig: %i %.10Lg %g \n", k, ((MNStruct *)globalcontext)->pulse->obsn[k].bat, Redsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2)));
	
				}




			delete[] Redshapecoeff;
			delete[] RedshapeVec;
			delete[] RedshapeNorm;

			}
		}
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract Sine Wave///////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


	if(((MNStruct *)globalcontext)->incsinusoid == 1){
		double sineamp=pow(10.0,Cube[pcount]);
		pcount++;
		double sinephase=Cube[pcount];
		pcount++;
		double sinefreq=pow(10.0,Cube[pcount])/maxtspan;
		pcount++;		
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			Resvec[o]-= sineamp*sin(2*M_PI*sinefreq*(double)((MNStruct *)globalcontext)->pulse->obsn[o].bat + sinephase);
		}
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////DM Variations////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 


	if(((MNStruct *)globalcontext)->incDM > 0){

                for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                        DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                }

        	for(int i=0;i<FitDMCoeff/2;i++){

			freqs[startpos+i]=((MNStruct *)globalcontext)->sampleFreq[startpos/2 - ((MNStruct *)globalcontext)->incFloatRed+i]/maxtspan;
			freqs[startpos+i+FitDMCoeff/2]=freqs[startpos+i];

			if(((MNStruct *)globalcontext)->storeFMatrices == 0){
				for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
					double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

					TotalMatrix[k + (i+TimetoMargin+startpos)*((MNStruct *)globalcontext)->pulse->nobs]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
					TotalMatrix[k + (i+FitDMCoeff/2+TimetoMargin+startpos)*((MNStruct *)globalcontext)->pulse->nobs] = sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];

				}
			}
		}
	} 


       if(((MNStruct *)globalcontext)->incDM==2){

		for (int i=0; i<FitDMCoeff/2; i++){
			int pnum=pcount;
			double pc=Cube[pcount];

			powercoeff[startpos+i]=pow(10.0,2*pc);
			powercoeff[startpos+i+FitDMCoeff/2]=powercoeff[startpos+i];
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
			pcount++;
		}
		startpos+=FitDMCoeff;
           	 
        }
        else if(((MNStruct *)globalcontext)->incDM==3){

		for(int pl = 0; pl < ((MNStruct *)globalcontext)->numFitDMPL; pl ++){
			double DMamp=Cube[pcount];
			pcount++;
			double DMindex=Cube[pcount];
			pcount++;

			
   			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
    			

			DMamp=pow(10.0, DMamp);
			if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(DMamp); }
			for (int i=0; i<FitDMCoeff/2; i++){
	
 				double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-DMindex))/(maxtspan*24*60*60);	
 				powercoeff[startpos+i]+=rho;
 				powercoeff[startpos+i+FitDMCoeff/2]+=rho;
			}
		}
		
		
		int coefftovary=0;
		double amptovary=0.0;
		if(((MNStruct *)globalcontext)->varyDMCoeff==1){
			coefftovary=int(pow(10.0,Cube[pcount]))-1;
			pcount++;
			amptovary=pow(10.0,Cube[pcount])/(maxtspan*24*60*60);
			pcount++;

			powercoeff[startpos+coefftovary]=amptovary;
			powercoeff[startpos+coefftovary+FitDMCoeff/2]=amptovary;	
		}	
			
		
		for (int i=0; i<FitDMCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}
		startpos+=FitDMCoeff;

        }




/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract  DM Shape Events////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	if(((MNStruct *)globalcontext)->incDMShapeEvent != 0){

		if(((MNStruct *)globalcontext)->incDM == 0){
			        for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                        		DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                		}
		}
                for(int i =0; i < ((MNStruct *)globalcontext)->incDMShapeEvent; i++){

			int numDMShapeCoeff=((MNStruct *)globalcontext)->numDMShapeCoeff;

                        double EventPos=Cube[pcount];
			pcount++;
                        double EventWidth=Cube[pcount];
			pcount++;


			globalwidth=EventWidth;

			double *DMshapecoeff=new double[numDMShapeCoeff];
			double *DMshapeVec=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapecoeff[c]=Cube[pcount];
				pcount++;
			}


			double *DMshapeNorm=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
			}

			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
				TNothpl(numDMShapeCoeff,HVal,DMshapeVec);
				double DMsignal=0;
				for(int c=0; c < numDMShapeCoeff; c++){
					DMsignal += DMshapeNorm[c]*DMshapeVec[c]*DMshapecoeff[c]*DMVec[k];
				}

				  Resvec[k] -= DMsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
	
			}




		delete[] DMshapecoeff;
		delete[] DMshapeVec;
		delete[] DMshapeNorm;

		}
	}





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract  DM Scatter Shape Events///////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


	if(((MNStruct *)globalcontext)->incDMScatterShapeEvent != 0){
                for(int i =0; i < ((MNStruct *)globalcontext)->incDMScatterShapeEvent; i++){

			int numDMShapeCoeff=((MNStruct *)globalcontext)->numDMScatterShapeCoeff;

                        double EventPos=Cube[pcount];
			pcount++;
                        double EventWidth=globalwidth;//Cube[pcount];
			pcount++;
                        double EventFScale=Cube[pcount];
			pcount++;

			//EventWidth=globalwidth;

			double *DMshapecoeff=new double[numDMShapeCoeff];
			double *DMshapeVec=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapecoeff[c]=Cube[pcount];
				pcount++;
			}


			double *DMshapeNorm=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
			}

			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
				TNothpl(numDMShapeCoeff,HVal,DMshapeVec);
				double DMsignal=0;
				for(int c=0; c < numDMShapeCoeff; c++){
					DMsignal += DMshapeNorm[c]*DMshapeVec[c]*DMshapecoeff[c]*pow(DMVec[k],EventFScale/2.0);
				}

				  Resvec[k] -= DMsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
	
			}




		delete[] DMshapecoeff;
		delete[] DMshapeVec;
		delete[] DMshapeNorm;

		}
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract Yearly DM//////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 


	if(((MNStruct *)globalcontext)->yearlyDM == 1){
		double yearlyamp=pow(10.0,Cube[pcount]);
		pcount++;
		double yearlyphase=Cube[pcount];
		pcount++;
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			Resvec[o]-= yearlyamp*sin((2*M_PI/365.25)*(double)((MNStruct *)globalcontext)->pulse->obsn[o].bat + yearlyphase)*DMVec[o];
		}
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Subtract Solar Wind//////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////// 

	if(((MNStruct *)globalcontext)->FitSolarWind == 1){

		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){

			Resvec[o]-= (SolarWind-((MNStruct *)globalcontext)->pulse->ne_sw)*((MNStruct *)globalcontext)->pulse->obsn[o].tdis2;
		}
	}

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Band DM/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


        if(((MNStruct *)globalcontext)->incBandNoise > 0){

                if(((MNStruct *)globalcontext)->incDM == 0){
                        for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                                DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                        }
                }

		for(int b = 0; b < ((MNStruct *)globalcontext)->incBandNoise; b++){

			double startfreq = ((MNStruct *)globalcontext)->FitForBand[b][0];
			double stopfreq = ((MNStruct *)globalcontext)->FitForBand[b][1];
			double BandScale = ((MNStruct *)globalcontext)->FitForBand[b][2];
			int BandPriorType = ((MNStruct *)globalcontext)->FitForBand[b][3];


			double Bandamp=Cube[pcount];
			pcount++;
			double Bandindex=Cube[pcount];
			pcount++;

			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;

			Bandamp=pow(10.0, Bandamp);
			if(BandPriorType == 1) { uniformpriorterm += log(Bandamp); }
			for (int i=0; i<FitBandCoeff/2; i++){

				freqs[startpos+i]=((double)(i+1))/maxtspan;
				freqs[startpos+i+FitBandCoeff/2]=freqs[startpos+i];
				
				double rho = (Bandamp*Bandamp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-Bandindex))/(maxtspan*24*60*60);	
				powercoeff[startpos+i]+=rho;
				powercoeff[startpos+i+FitBandCoeff/2]+=rho;
			}
			
			
			for (int i=0; i<FitBandCoeff/2; i++){
				freqdet=freqdet+2*log(powercoeff[startpos+i]);
			}
			if(((MNStruct *)globalcontext)->storeFMatrices == 0){
				for(int i=0;i<FitBandCoeff/2;i++){
					for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
						if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
							double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
							TotalMatrix[k + (i+TimetoMargin+startpos)*((MNStruct *)globalcontext)->pulse->nobs]=cos(2*M_PI*freqs[startpos+i]*time);
							TotalMatrix[k + (i+TimetoMargin+startpos+FitBandCoeff/2)*((MNStruct *)globalcontext)->pulse->nobs]=sin(2*M_PI*freqs[startpos+i]*time);
						}
						else{	
							TotalMatrix[k + (i+TimetoMargin+startpos)*((MNStruct *)globalcontext)->pulse->nobs]=0;
							TotalMatrix[k + (i+TimetoMargin+startpos+FitBandCoeff/2)*((MNStruct *)globalcontext)->pulse->nobs]=0;

						}


					}
				}
			}

			startpos += FitBandCoeff;
		}

    	}





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Add Group Noise/////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

        if(((MNStruct *)globalcontext)->incGroupNoise > 0){

		for(int g = 0; g < ((MNStruct *)globalcontext)->incGroupNoise; g++){

			int GrouptoFit=0;
			double GroupStartTime = 0;
			double GroupStopTime = 0;
			double GroupTSpan = 0;
			if(((MNStruct *)globalcontext)->FitForGroup[g][0] == -1){
				GrouptoFit = floor(Cube[pcount]);
				pcount++;
				//printf("Fit for group %i \n", GrouptoFit);
			}
			else{
				GrouptoFit = ((MNStruct *)globalcontext)->FitForGroup[g][0];
				
			}

                        if(((MNStruct *)globalcontext)->FitForGroup[g][1] == 1){

				double GLength = ((MNStruct *)globalcontext)->GroupStartTimes[GrouptoFit][1] - ((MNStruct *)globalcontext)->GroupStartTimes[GrouptoFit][0];
                                GroupStartTime = ((MNStruct *)globalcontext)->GroupStartTimes[GrouptoFit][0] + Cube[pcount]*GLength;
				
				double LengthLeft =  ((MNStruct *)globalcontext)->GroupStartTimes[GrouptoFit][1] - GroupStartTime;

                                pcount++;
				GroupStopTime = GroupStartTime + Cube[pcount]*LengthLeft;
                                pcount++;
				GroupTSpan = GroupStopTime-GroupStartTime;

				//printf("Start Stop %i %g %g %g %g \n", GrouptoFit, ((MNStruct *)globalcontext)->GroupStartTimes[GrouptoFit][0] , ((MNStruct *)globalcontext)->GroupStartTimes[GrouptoFit][1] , GLength, GroupStartTime);

                        }
                        else{
                                GroupStartTime = 0;
				GroupStopTime = 10000000.0;
				GroupTSpan = maxtspan;
                        }
			
			double GroupScale = ((MNStruct *)globalcontext)->FitForGroup[g][5];
		

			double GroupAmp=Cube[pcount];
			pcount++;
			double GroupIndex=Cube[pcount];
			pcount++;


		
			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
		

			GroupAmp=pow(10.0, GroupAmp);
			if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(GroupAmp); }

			for (int i=0; i<FitGroupNoiseCoeff/2; i++){

				freqs[startpos+i]=((double)(i+1.0))/GroupTSpan;//maxtspan;
				freqs[startpos+i+FitGroupNoiseCoeff/2]=freqs[startpos+i];
			
				double rho = (GroupAmp*GroupAmp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-GroupIndex))/(maxtspan*24*60*60);	
				powercoeff[startpos+i]+=rho;
				powercoeff[startpos+i+FitGroupNoiseCoeff/2]+=rho;
			}
		
		
			
			for (int i=0; i<FitGroupNoiseCoeff/2; i++){
				freqdet=freqdet+2*log(powercoeff[startpos+i]);
			}

			double sqrtDMKappa = sqrt(DMKappa);
			for(int i=0;i<FitGroupNoiseCoeff/2;i++){
				for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
					if(((MNStruct *)globalcontext)->GroupNoiseFlags[k] == GrouptoFit && (double)((MNStruct *)globalcontext)->pulse->obsn[k].bat > GroupStartTime && (double)((MNStruct *)globalcontext)->pulse->obsn[k].bat < GroupStopTime){
			//		if(((MNStruct *)globalcontext)->GroupNoiseFlags[k] == GrouptoFit){
				       		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
						double Fscale = 1.0/(pow(sqrtDMKappa*((double)((MNStruct *)globalcontext)->pulse->obsn[k].freqSSB), GroupScale));
						//printf("Scale: %i %i %g %g %g \n", g, k, GroupScale, Fscale);
						TotalMatrix[k + (i+TimetoMargin+startpos)*((MNStruct *)globalcontext)->pulse->nobs]=cos(2*M_PI*freqs[startpos+i]*time)*Fscale;
						TotalMatrix[k + (i+TimetoMargin+startpos+FitGroupNoiseCoeff/2)*((MNStruct *)globalcontext)->pulse->nobs]=sin(2*M_PI*freqs[startpos+i]*time)*Fscale;
					}
					else{
						TotalMatrix[k + (i+TimetoMargin+startpos)*((MNStruct *)globalcontext)->pulse->nobs]=0;
                                                TotalMatrix[k + (i+TimetoMargin+startpos+FitGroupNoiseCoeff/2)*((MNStruct *)globalcontext)->pulse->nobs]=0;	
					}
				}
			}



			startpos=startpos+FitGroupNoiseCoeff;
		}

    }






/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Add ECORR Coeffs////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	if(((MNStruct *)globalcontext)->incNGJitter >0){
		for(int i =0; i < ((MNStruct *)globalcontext)->numNGJitterEpochs; i++){
			powercoeff[startpos+i] = ECORRPrior[i];
			freqdet = freqdet + log(ECORRPrior[i]);
		}
	}






/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get Time domain likelihood//////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

/*
#ifdef HAVE_MLAPACK
	static unsigned int oldcw;
	if(((MNStruct *)globalcontext)->uselongdouble > 0 ){
		fpu_fix_start(&oldcw);
	//	printf("oldcw %i \n", oldcw);
	}
#endif	
*/

	double tdet=0;
	double timelike=0;

	for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		//printf("Res: %i  %g %g \n", o, Resvec[o], sqrt(Noise[o]));
		timelike+=Resvec[o]*Resvec[o]*Noise[o];
		tdet -= log(Noise[o]);
	}

/*
#ifdef HAVE_MLAPACK
	dd_real ddtimelike=0.0;
	if(((MNStruct *)globalcontext)->uselongdouble ==1){
		for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			dd_real res = (dd_real)Resvec[o];
			dd_real Nval = (dd_real)Noise[o];
			dd_real chicomp = res*res*Nval;
			ddtimelike += chicomp;
		}
	}
	//printf("timelike %g %g \n", timelike, cast2double(ddtimelike));
	
        qd_real qdtimelike=0.0;
        if(((MNStruct *)globalcontext)->uselongdouble ==2){
                for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){

                        qd_real res = (qd_real)Resvec[o];
                        qd_real Nval = (qd_real)Noise[o];
                        qd_real chicomp = res*res*Nval;

                        qdtimelike+=chicomp;
                }
		//printf("qdtimelike %15.10e\n", qdtimelike.x[0]);
        }
#endif	
*/

//////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////Form Total Matrices////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

	for(int i =0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		for(int j =0;j<totalredshapecoeff; j++){
			TotalMatrix[i + (j+TimetoMargin+totCoeff)*((MNStruct *)globalcontext)->pulse->nobs]=RedShapeletMatrix[i][j];
		}
	}


//////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////Do Algebra/////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

	int savememory = 0;
	
	double *NTd = new double[totalsize];
	double *TNT=new double[totalsize*totalsize];
	double *NT;	

	if(savememory == 0){
		NT=new double[((MNStruct *)globalcontext)->pulse->nobs*totalsize];
		std::memcpy(NT, TotalMatrix, ((MNStruct *)globalcontext)->pulse->nobs*totalsize*sizeof(double));
	
		for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
			for(int j=0;j<totalsize;j++){
				NT[i + j*((MNStruct *)globalcontext)->pulse->nobs] *= Noise[i];
			}
		}

		vector_dgemm(TotalMatrix, NT , TNT, ((MNStruct *)globalcontext)->pulse->nobs, totalsize, ((MNStruct *)globalcontext)->pulse->nobs, totalsize, 'T', 'N');

		vector_dgemv(NT,Resvec,NTd,((MNStruct *)globalcontext)->pulse->nobs,totalsize,'T');
	}
	if(savememory == 1){

		for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
			for(int j=0;j<totalsize;j++){
				TotalMatrix[i + j*((MNStruct *)globalcontext)->pulse->nobs] *= sqrt(Noise[i]);
			}
			Resvec[i] *= Noise[i];
		}

		vector_dgemm(TotalMatrix, TotalMatrix , TNT, ((MNStruct *)globalcontext)->pulse->nobs, totalsize, ((MNStruct *)globalcontext)->pulse->nobs, totalsize, 'T', 'N');

		for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
			for(int j=0;j<totalsize;j++){
				TotalMatrix[i + j*((MNStruct *)globalcontext)->pulse->nobs] /= sqrt(Noise[i]);
			}
		}


		vector_dgemv(TotalMatrix,Resvec,NTd,((MNStruct *)globalcontext)->pulse->nobs,totalsize,'T');

	}
/*
#ifdef HAVE_MLAPACK
        dd_real ddfreqlike = 0.0;
        dd_real ddsigmadet = 0.0;

        dd_real ddfreqlikeChol = 0.0;
        dd_real ddsigmadetChol = 0.0;

	int ddinfo = 0;
	if(((MNStruct *)globalcontext)->uselongdouble ==1){
		mpackint n = totalsize;
		mpackint m = ((MNStruct *)globalcontext)->pulse->nobs;


		dd_real *A = new dd_real[m * n];
		dd_real *B = new dd_real[m * n];
		dd_real *C = new dd_real[n * n];
		dd_real *CholC = new dd_real[n * n];

		dd_real *ddRes = new dd_real[m];
		dd_real *ddNTd = new dd_real[n];
		dd_real *ddNTdCopy = new dd_real[n];
		dd_real *ddPC = new dd_real[totCoeff];
		dd_real alpha, beta;


		for (int i=0; i<m; i++) for (int j=0; j<n; j++) A[i+j*m] = (dd_real) TotalMatrix[i+j*m];
		for (int i=0; i<m; i++) for (int j=0; j<n; j++) B[i+j*m] = (dd_real) NT[i+j*m];
		for(int i =0; i < m; i++) ddRes[i] = (dd_real)Resvec[i];
		for(int i =0; i < totCoeff; i++) ddPC[i] = (dd_real) (1.0/powercoeff[i]);

		alpha = 1.0;
		beta =  0.0;
		Rgemv("t", m,n,alpha, B, m, ddRes, 1, beta, ddNTd, 1);
		for(int i =0; i < n; i++) {
			ddNTdCopy[i] = ddNTd[i];
			//printf("ddNT %i %g \n", i, cast2double(ddNTd[i]));
		}


		Rgemm("t", "n", n, n, m, alpha, A, m, B, m, beta, C, n);



		for(int j=0;j<totCoeff;j++){
			int l = TimetoMargin+j;
			C[l+l*n] += ddPC[j];
		}

		for(int j=0;j<totalredshapecoeff;j++){
			int l = TimetoMargin+totCoeff+j;
			dd_real detfac = (dd_real)pow(10.0, -12);
			C[l+l*n] += detfac*C[l+l*n];
			
		}

		for(int i =0; i < n*n; i++)CholC[i] = C[i];

		mpackint lwork, info;

    		mpackint *ipiv = new mpackint[n];



		Rgetrf(n, n, C, n, ipiv, &info);
		if(info != 0)ddinfo = (int)info;
		for(int i =0; i < n; i++) ddsigmadet += log(abs(C[i+i*n]));

		Rgetrs("n", n, 1, C, n, ipiv, ddNTdCopy, n, &info);

		if(info != 0)ddinfo = (int)info;

		for(int i =0; i < totalsize; i++){
			ddfreqlike += ddNTd[i]*ddNTdCopy[i];
		}


		int printResiduals=0;

		if(printResiduals==1){
			dd_real *ddSignal = new dd_real[m];
			//for(int i = 0; i < TimetoMargin; i++){ddNTdCopy[i] = 0;}
			//for(int i = TimetoMargin+FitRedCoeff; i < totalsize; i++){ddNTdCopy[i] = 0;}
			Rgemv("n", m,n,alpha, A, m, ddNTdCopy, 1, beta, ddSignal, 1);

			for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
				printf("%.10Lg %g %g \n", ((MNStruct *)globalcontext)->pulse->obsn[i].bat, (double) ((MNStruct *)globalcontext)->pulse->obsn[i].residual, cast2double(ddSignal[i]));
			}
			
			delete[] ddSignal;
		}

			
		delete[] ddPC;
		delete[] ddNTd;
		delete[] ddNTdCopy;
		delete[]C;
		delete[]A;
		delete[] ipiv;



                dd_real *ddNTdChol = new dd_real[n];
                dd_real *ddNTdCholCopy = new dd_real[n];

		Rgemv("t", m,n,alpha, B, m, ddRes, 1, beta, ddNTdChol, 1);
                for(int i =0; i < n; i++) ddNTdCholCopy[i] = ddNTdChol[i];


                mpackint Cholinfo;


                Rpotrf("u", n, CholC, n, &Cholinfo);
                if(Cholinfo != 0)ddinfo = (int)Cholinfo;
                for(int i =0; i < n; i++) ddsigmadetChol += log(abs(CholC[i+i*n]));

                Rpotrs("u", n, 1, CholC, n,  ddNTdCholCopy, n, &Cholinfo);

                if(Cholinfo != 0)ddinfo = (int)Cholinfo;

                for(int i =0; i < totalsize; i++){
                        ddfreqlikeChol += ddNTdChol[i]*ddNTdCholCopy[i];
                }


                delete[] ddNTdChol;
                delete[] ddNTdCholCopy;
                delete[]CholC;
		delete[] ddRes;
		delete[] B;
	}


        qd_real qdfreqlike = 0.0;
        qd_real qdsigmadet = 0.0;

        qd_real qdfreqlikeChol = 0.0;
        qd_real qdsigmadetChol = 0.0;

	int qdinfo = 0;
	if(((MNStruct *)globalcontext)->uselongdouble ==2){
		mpackint n = totalsize;
		mpackint m = ((MNStruct *)globalcontext)->pulse->nobs;


		qd_real *A = new qd_real[m * n];
		qd_real *B = new qd_real[m * n];
		qd_real *C = new qd_real[n * n];
		qd_real *CholC = new qd_real[n * n];


		qd_real *qdRes = new qd_real[m];
		qd_real *qdNTd = new qd_real[n];
		qd_real *qdNTdCopy = new qd_real[n];
		qd_real *qdPC = new qd_real[totCoeff];
		qd_real alpha, beta;


		for (int i=0; i<m; i++) for (int j=0; j<n; j++) A[i+j*m] = (qd_real) TotalMatrix[i+j*m];
		for (int i=0; i<m; i++) for (int j=0; j<n; j++) B[i+j*m] = (qd_real) NT[i+j*m];
		for(int i =0; i < m; i++) qdRes[i] = (qd_real) Resvec[i];
		for(int i =0; i < totCoeff; i++) qdPC[i] = (qd_real) (1.0/powercoeff[i]);
		alpha = 1.0;
		beta =  0.0;

		Rgemv("t", m,n,alpha, B, m, qdRes, 1, beta, qdNTd, 1);
                for(int i =0; i < n; i++) qdNTdCopy[i] = qdNTd[i];

		Rgemm("t", "n", n, n, m, alpha, A, m, B, m, beta, C, n);



		for(int j=0;j<totCoeff;j++){
				int l = TimetoMargin+j;
				//printf("FNF %i %25.15e %g \n", j, C[l+l*n].x[0], cast2double(qdPC[j]));
				C[l+l*n] += qdPC[j];
		}

		for(int j=0;j<totalredshapecoeff;j++){
			int l = TimetoMargin+totCoeff+j;
			qd_real detfac = (qd_real)pow(10.0, -12);
			C[l+l*n] += detfac*C[l+l*n];
			
		}

		for(int i =0; i < n*n; i++)CholC[i] = C[i];

		mpackint lwork, info;

    		mpackint *ipiv = new mpackint[n];



		Rgetrf(n, n, C, n, ipiv, &info);
		if(info != 0)qdinfo = (int)info;
		for(int i =0; i < n; i++) qdsigmadet += log(abs(C[i+i*n]));

		Rgetrs("n", n, 1, C, n, ipiv, qdNTdCopy, n, &info);

		if(info != 0)qdinfo = (int)info;

		for(int i =0; i < totalsize; i++){
			qdfreqlike += qdNTd[i]*qdNTdCopy[i];
		}

			
		delete[] qdPC;
		delete[] qdNTd;
		delete[]C;
		delete[]A;
		delete[] ipiv;


                qd_real *qdNTdChol = new qd_real[n];
                qd_real *qdNTdCholCopy = new qd_real[n];



                mpackint Cholinfo;

                Rgemv("t", m,n,alpha, B, m, qdRes, 1, beta, qdNTdChol, 1);
                for(int i =0; i < n; i++) qdNTdCholCopy[i] = qdNTdChol[i];


                Rpotrf("u", n, CholC, n, &Cholinfo);
                if(Cholinfo != 0)qdinfo = (int)Cholinfo;
                for(int i =0; i < n; i++) qdsigmadetChol += log(abs(CholC[i+i*n]));

                Rpotrs("u", n, 1, CholC, n,  qdNTdCholCopy, n, &Cholinfo);

                if(Cholinfo != 0)qdinfo = (int)Cholinfo;

                for(int i =0; i < totalsize; i++){
                        qdfreqlikeChol += qdNTdChol[i]*qdNTdCholCopy[i];
		//	printf("diff %i %g %g %g \n", i, cast2double(qdNTdCholCopy[i] - qdNTdCopy[i]), cast2double(qdNTdCholCopy[i]), cast2double(qdNTdCopy[i]));
                }

		 delete[] qdNTdCopy;
		delete[] B;
		delete[] qdRes;
                delete[] qdNTdChol;
                delete[] qdNTdCholCopy;
                delete[]CholC;

	}
#endif
*/
	if(savememory == 0){
		delete[] NT;
	}
	for(int j=0;j<totCoeff;j++){
			TNT[TimetoMargin+j + (TimetoMargin+j)*totalsize] += 1.0/powercoeff[j];
			
	}
	for(int j=0;j<totalredshapecoeff;j++){
			freqdet=freqdet-log(pow(10.0, -12)*TNT[TimetoMargin+totCoeff+j + (TimetoMargin+totCoeff+j)*totalsize]);
			TNT[TimetoMargin+totCoeff+j + (TimetoMargin+totCoeff+j)*totalsize] += pow(10.0, -12)*TNT[TimetoMargin+totCoeff+j + (TimetoMargin+totCoeff+j)*totalsize];
			
	}

	double freqlike=0;
	double *WorkCoeff = new double[totalsize];
	double *WorkCoeff2 = new double[totalsize];
	double *TNT2=new double[totalsize*totalsize];
	
	for(int i =0; i < totalsize; i++){
		for(int j=0 ; j < totalsize; j++){
			TNT2[i + j*totalsize] = TNT[i + j*totalsize];			
		}
	}
	for(int o1=0;o1<totalsize; o1++){
		
		WorkCoeff[o1]=NTd[o1];
		WorkCoeff2[o1]=NTd[o1];
	}

	int globalinfo=0;
	int info=0;
	double jointdet = 0;	
	vector_dpotrfInfo(TNT, totalsize, jointdet, info);
	if(info != 0)globalinfo=1;

	info=0;
	vector_dpotrsInfo(TNT, WorkCoeff, totalsize, info);

	/*
	if(((MNStruct *)globalcontext)->doGrades == 1){

		for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
			((MNStruct *)globalcontext)->PreviousNoise[i] = Noise[i];
		}

		for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
			for(int j=0;j<totalsize;j++){
				((MNStruct *)globalcontext)->PreviousNT[i][j] = NT[i][j];
			}
		}

		for(int i =0; i < totalsize; i++){
			for(int j=0 ; j < totalsize; j++){
				((MNStruct *)globalcontext)->PreviousTNT[i][j] = TNT[i][j];
			}
		}
	}
*/
        if(info != 0)globalinfo=1;
	info=0;
	double det2=0;
	vector_TNqrsolve(TNT2, NTd, WorkCoeff2, totalsize, det2, info);
 
	double freqlike2 = 0;    
	for(int j=0;j<totalsize;j++){
		freqlike += NTd[j]*WorkCoeff[j];
		freqlike2+=NTd[j]*WorkCoeff2[j];
	}


	double lnewChol =-0.5*(tdet+jointdet+freqdet+timelike-freqlike) + uniformpriorterm;
	double lnew=-0.5*(tdet+det2+freqdet+timelike-freqlike2) + uniformpriorterm;
	//printf("Double %g %i\n", lnew, globalinfo);
	if(fabs(lnew-lnewChol)>0.05){
		globalinfo = 1;
	//	lnew=-pow(10.0,20);
	}
	if(isnan(lnew) || isinf(lnew) || globalinfo != 0){
		globalinfo = 1;
		lnew=-pow(10.0,20);
		
	}

/*
#ifdef HAVE_MLAPACK
	//printf("lnew double : %g", lnew);
	if(((MNStruct *)globalcontext)->uselongdouble ==1){

		dd_real ddAllLike = -0.5*(tdet+ddsigmadet+freqdet+ddtimelike-ddfreqlike) + uniformpriorterm;
		double ddlike = cast2double(ddAllLike);
		dd_real ddAllLikeChol = -0.5*(tdet+2*ddsigmadetChol+freqdet+ddtimelike-ddfreqlikeChol) + uniformpriorterm;
		double ddlikeChol = cast2double(ddAllLikeChol);
		lnew = ddlike;
		
		if(fabs(ddlikeChol-ddlike)>0.5){
			lnew=-pow(10.0,20);
		}


		if(isnan(lnew) || isinf(lnew) || ddinfo != 0){
			lnew=-pow(10.0,20);
		}

	//	printf("double double %g %g %g\n", lnew,ddlike,  fabs(ddlikeChol-ddlike));
	}

	

        if(((MNStruct *)globalcontext)->uselongdouble ==2){

		qd_real qdAllLike = -0.5*(tdet+qdsigmadet+freqdet+qdtimelike-qdfreqlike) + uniformpriorterm;
		double qdlike = cast2double(qdAllLike);
		qd_real qdAllLikeChol = -0.5*(tdet+2*qdsigmadetChol+freqdet+qdtimelike-qdfreqlikeChol) + uniformpriorterm;
		double qdlikeChol = cast2double(qdAllLikeChol);

                lnew = qdlike;

                if(fabs(qdlikeChol-qdlike)>0.05){
                        lnew=-pow(10.0,20);
                }


                if(isnan(lnew) || isinf(lnew) || ddinfo != 0){
                        lnew=-pow(10.0,20);
                }
        }


        if(((MNStruct *)globalcontext)->uselongdouble > 0 ){
                fpu_fix_end(&oldcw);
        }
#endif
*/
	if(badshape == 1){lnew=-pow(10.0,20);}


	delete[] DMVec;
	delete[] WorkCoeff;
	delete[] WorkCoeff2;
	for(int i=0; i < ((MNStruct *)globalcontext)->EPolTerms; i++) delete[] EFAC[i];
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] powercoeff;
	delete[] NTd;
	delete[] freqs;
	delete[] Noise;
	delete[] Resvec;
	delete[]TNT;
	delete[] TNT2;

	if(((MNStruct *)globalcontext)->incNGJitter >0){
		delete[] ECORRPrior;
	}

	if(totalredshapecoeff > 0){
		for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
			delete[] RedShapeletMatrix[j];
		}
		delete[] RedShapeletMatrix;
	}
	//printf("brace? %i \n", ((MNStruct *)globalcontext)->pulse->brace);
	if(((MNStruct *)globalcontext)->pulse->brace == -1){lnew = -pow(10.0, 10); }
        ((MNStruct *)globalcontext)->PreviousInfo = globalinfo;
        ((MNStruct *)globalcontext)->PreviousJointDet = jointdet;
        ((MNStruct *)globalcontext)->PreviousFreqDet = freqdet;
        ((MNStruct *)globalcontext)->PreviousUniformPrior = uniformpriorterm;

        // printf("tdet %g, jointdet %g, freqdet %g, lnew %g, timelike %g, freqlike %g\n", tdet, jointdet, freqdet, lnew, timelike, freqlike);


	//printf("CPUChisq: %g %g %g %g %g %g \n",lnew,jointdet,tdet,freqdet,timelike,freqlike);


	return lnew;

	


}

/*
double  LRedNumericalLogLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){

	double **EFAC;
	double *EQUAD;

	
	int numfit=((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps;
	long double LDparams[numfit];




	int fitcount=0;
	int pcount=0;
	
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){
		if(((MNStruct *)globalcontext)->Dpriors[p][1] != ((MNStruct *)globalcontext)->Dpriors[p][0]){

			double val = 0;
			if((((MNStruct *)globalcontext)->LDpriors[p][3]) == 0){
				val = Cube[fitcount];
			}
			if((((MNStruct *)globalcontext)->LDpriors[p][3]) == 1){
				val = pow(10.0,Cube[fitcount]);
			}
			LDparams[p]=val*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
			fitcount++;
		}
		else if(((MNStruct *)globalcontext)->Dpriors[p][1] == ((MNStruct *)globalcontext)->Dpriors[p][0]){
			LDparams[p]=((MNStruct *)globalcontext)->Dpriors[p][0]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		}


	}

	pcount=0;
	double phase=(double)LDparams[0];
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	
	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);      
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,1);      
	

	double *Resvec=new double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		Resvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].residual+phase;

	}
	
	
	
	pcount=fitcount;


/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Subtract Steps////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	if(((MNStruct *)globalcontext)->incStep > 0){
		for(int i = 0; i < ((MNStruct *)globalcontext)->incStep; i++){
			double StepAmp = Cube[pcount];
			pcount++;
			double StepTime = Cube[pcount];
			pcount++;
			for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
				if(((MNStruct *)globalcontext)->pulse->obsn[o1].bat > StepTime){
					Resvec[o1] += StepAmp;
				}
			}
		}
	}	


/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Subtract GLitches/////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 


 
	for(int i = 0 ; i < ((MNStruct *)globalcontext)->incGlitch; i++){
		double GlitchMJD = Cube[pcount];
		pcount++;

		double *GlitchAmps = new double[2];
		if(((MNStruct *)globalcontext)->incGlitchTerms==1 || ((MNStruct *)globalcontext)->incGlitchTerms==3){
			GlitchAmps[0]  = Cube[pcount];
			pcount++;
		}
		else if(((MNStruct *)globalcontext)->incGlitchTerms==2){
			GlitchAmps[0]  = Cube[pcount];
                        pcount++;
			GlitchAmps[1]  = Cube[pcount];
                        pcount++;
		}


		for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
                        if(((MNStruct *)globalcontext)->pulse->obsn[o1].bat > GlitchMJD){

				if(((MNStruct *)globalcontext)->incGlitchTerms==1){

					long double arg=0;
					arg=((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0])*86400.0;
					double darg = (double)arg;
                                        Resvec[o1] += GlitchAmps[0]*darg;

				}
				else if(((MNStruct *)globalcontext)->incGlitchTerms==2){
					for(int j = 0; j < ((MNStruct *)globalcontext)->incGlitchTerms; j++){

						long double arg=0;
						if(j==0){
							arg=((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0])*86400.0;
						}
						if(j==1){
							arg=0.5*pow((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)*86400.0,2)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0];
						}
						double darg = (double)arg;
						Resvec[o1] += GlitchAmps[j]*darg;
					}
				}
				else if(((MNStruct *)globalcontext)->incGlitchTerms==3){
					long double arg=0;
					arg=0.5*pow((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)*86400.0,2)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0];
					double darg = (double)arg;
                                        Resvec[o1] += GlitchAmps[0]*darg;
				}


			}
		}
	}
	
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get White Noise vector///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	double uniformpriorterm=0;

	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double*[((MNStruct *)globalcontext)->EPolTerms];
		for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
			EFAC[n-1]=new double[((MNStruct *)globalcontext)->systemcount];
			if(n==1){
				for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
					EFAC[n-1][o]=1;
				}
			}
			else{
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                   EFAC[n-1][o]=0;
                }
			}
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double*[((MNStruct *)globalcontext)->EPolTerms];
		for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
			
			EFAC[n-1]=new double[((MNStruct *)globalcontext)->systemcount];
			if(n==1){
				for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
					EFAC[n-1][o]=pow(10.0,Cube[pcount]);
					if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[n-1][o]);}
				}
				pcount++;
			}
			else{
                                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){

                                        EFAC[n-1][o]=pow(10.0,Cube[pcount]);
                                }
                                pcount++;
                        }
		}
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double*[((MNStruct *)globalcontext)->EPolTerms];
		for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
			EFAC[n-1]=new double[((MNStruct *)globalcontext)->systemcount];
			if(n==1){
				for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
					EFAC[n-1][p]=pow(10.0,Cube[pcount]);
					if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[n-1][p]);}
					pcount++;
				}
			}
			else{
                                for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
                                        EFAC[n-1][p]=pow(10.0,Cube[pcount]);
                                        pcount++;
                                }
                        }
		}
	}	

		
	//printf("Equad %i \n", ((MNStruct *)globalcontext)->numFitEQUAD);
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,2*Cube[pcount]);
			if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount]));}
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){

			if(((MNStruct *)globalcontext)->includeEQsys[o] == 1){
				//printf("Cube: %i %i %g \n", o, pcount, Cube[pcount]);
				EQUAD[o]=pow(10.0,2*Cube[pcount]);
				if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount])); }
				pcount++;
			}
			else{
				EQUAD[o]=0;
			}
			//printf("Equad? %i %g \n", o, EQUAD[o]);
		}
    	}
    

    double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,2*Cube[pcount]);
			if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount]));}
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
            SQUAD[o]=pow(10.0,2*Cube[pcount]);
	    if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount])); }

			pcount++;
        }
    }
 
	double *ECORRPrior;
	if(((MNStruct *)globalcontext)->incNGJitter >0){
		double *ECorrCoeffs=new double[((MNStruct *)globalcontext)->incNGJitter];	
		for(int i =0; i < ((MNStruct *)globalcontext)->incNGJitter; i++){
			ECorrCoeffs[i] = pow(10.0, 2*Cube[pcount]);
			pcount++;
		}
    		ECORRPrior = new double[((MNStruct *)globalcontext)->numNGJitterEpochs];
		for(int i =0; i < ((MNStruct *)globalcontext)->numNGJitterEpochs; i++){
			ECORRPrior[i] = ECorrCoeffs[((MNStruct *)globalcontext)->NGJitterSysFlags[i]];
		}

		delete[] ECorrCoeffs;
	} 



	double *Noise;	
	double *BATvec;
	Noise=new double[((MNStruct *)globalcontext)->pulse->nobs];
	BATvec=new double[((MNStruct *)globalcontext)->pulse->nobs];
	
	
	for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		BATvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].bat;
	}
		
		
	if(((MNStruct *)globalcontext)->whitemodel == 0){
	
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			double EFACterm=0;
			double noiseval=0;
			double ShannonJitterTerm=0;
			
			
			if(((MNStruct *)globalcontext)->useOriginalErrors==0){
				noiseval=((MNStruct *)globalcontext)->pulse->obsn[o].toaErr;
			}
			else if(((MNStruct *)globalcontext)->useOriginalErrors==1){
				noiseval=((MNStruct *)globalcontext)->pulse->obsn[o].origErr;
			}


			for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
				EFACterm=EFACterm + pow((noiseval*pow(10.0,-6))/pow(pow(10.0,-7),n-1),n)*EFAC[n-1][((MNStruct *)globalcontext)->sysFlags[o]];
			}	
			
			if(((MNStruct *)globalcontext)->incShannonJitter > 0){	
			 	ShannonJitterTerm=SQUAD[((MNStruct *)globalcontext)->sysFlags[o]]*((MNStruct *)globalcontext)->TobsInfo[o]/1000.0;
			}

			Noise[o]= 1.0/(pow(EFACterm,2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]]+ShannonJitterTerm);

		}
		
	}
	else if(((MNStruct *)globalcontext)->whitemodel == 1){
	
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){

			Noise[o]=1.0/(EFAC[0][((MNStruct *)globalcontext)->sysFlags[o]]*EFAC[0][((MNStruct *)globalcontext)->sysFlags[o]]*(pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6)),2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]]));
		}
		
	}

	
	

//////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////////////Set up Coefficients///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////  



	double start,end;
	int go=0;
	for (int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++)
	  {
	    if (((MNStruct *)globalcontext)->pulse->obsn[i].deleted==0)
	      {
		if (go==0)
		  {
		    go = 1;
		    start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    end  = start;
		  }
		else
		  {
		    if (start > (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    if (end < (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      end = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		  }
	      }
	  }

	double maxtspan=1*(end-start);
	double averageTSamp=2*maxtspan/((MNStruct *)globalcontext)->pulse->nobs;

	double **DMEventInfo;

	int FitRedCoeff=2*(((MNStruct *)globalcontext)->numFitRedCoeff);
	int FitDMCoeff=2*(((MNStruct *)globalcontext)->numFitDMCoeff);
	int FitBandNoiseCoeff=2*(((MNStruct *)globalcontext)->numFitBandNoiseCoeff);
	int FitGroupNoiseCoeff = 2*((MNStruct *)globalcontext)->numFitGroupNoiseCoeff;

	if(((MNStruct *)globalcontext)->incFloatDM != 0)FitDMCoeff+=2*((MNStruct *)globalcontext)->incFloatDM;
	if(((MNStruct *)globalcontext)->incFloatRed != 0)FitRedCoeff+=2*((MNStruct *)globalcontext)->incFloatRed;
	int DMEventCoeff=0;
	if(((MNStruct *)globalcontext)->incDMEvent != 0){
		DMEventInfo=new double*[((MNStruct *)globalcontext)->incDMEvent];
		for(int i =0; i < ((MNStruct *)globalcontext)->incDMEvent; i++){
			DMEventInfo[i]=new double[7];
			DMEventInfo[i][0]=Cube[pcount]; //Start time
			pcount++;
			DMEventInfo[i][1]=Cube[pcount]; //Length
			pcount++;
			DMEventInfo[i][2]=pow(10.0, Cube[pcount]); //Amplitude
			pcount++;
			DMEventInfo[i][3]=Cube[pcount]; //SpectralIndex
			pcount++;
			DMEventInfo[i][4]=Cube[pcount]; //Offset
			pcount++;
			DMEventInfo[i][5]=Cube[pcount]; //Linear
			pcount++;
			DMEventInfo[i][6]=Cube[pcount]; //Quadratic
			pcount++;
			DMEventCoeff+=2*int(DMEventInfo[i][1]/averageTSamp);

			}
	}


	int totCoeff=0;
	if(((MNStruct *)globalcontext)->incRED != 0)totCoeff+=FitRedCoeff;
	if(((MNStruct *)globalcontext)->incDM != 0)totCoeff+=FitDMCoeff;

	if(((MNStruct *)globalcontext)->incDMScatter == 1 || ((MNStruct *)globalcontext)->incDMScatter == 2  || ((MNStruct *)globalcontext)->incDMScatter == 3)totCoeff+=FitDMScatterCoeff;

	if(((MNStruct *)globalcontext)->incDMEvent != 0)totCoeff+=DMEventCoeff; 
	if(((MNStruct *)globalcontext)->incNGJitter >0)totCoeff+=((MNStruct *)globalcontext)->numNGJitterEpochs;

	if(((MNStruct *)globalcontext)->incGroupNoise > 0)totCoeff += ((MNStruct *)globalcontext)->incGroupNoise*FitGroupNoiseCoeff;





	double *powercoeff=new double[totCoeff];
	for(int o=0;o<totCoeff; o++){
		powercoeff[o]=0;
	}
	
	double **FMatrix=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		FMatrix[i]=new double[totCoeff];
		for(int j=0;j<totCoeff;j++){
			FMatrix[i][j]=0;
		}
	}

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Red Noise///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


    double *freqs = new double[totCoeff];

    double *DMVec=new double[((MNStruct *)globalcontext)->pulse->nobs];
    double *SignalCoeff = new double[totCoeff];

	for(int i = 0; i < totCoeff; i++){
		SignalCoeff[i] = 0;
	}


    double DMKappa = 2.410*pow(10.0,-16);
    int startpos=0;
    double freqdet=0;
    double GWBAmpPrior=0;
    int badAmp=0;
    if(((MNStruct *)globalcontext)->incRED==2){

    
		for (int i=0; i<FitRedCoeff/2; i++){
			int pnum=pcount;
			double pc=Cube[pcount];

			if(((MNStruct *)globalcontext)->RedPriorType ==1) { uniformpriorterm += log(pow(10.0,pc)); }

			freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
			freqs[startpos+i+FitRedCoeff/2]=freqs[startpos+i];
			powercoeff[i]=pow(10.0,2*pc);
			powercoeff[i+FitRedCoeff/2]=powercoeff[i];
			freqdet=freqdet+2*log(powercoeff[i]);
			pcount++;
		}
		
		
        for(int i=0;i<FitRedCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);

                }
        }

        for(int i=0;i<FitRedCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);
                }
        }
	            
	    startpos=FitRedCoeff;

    }
   else if(((MNStruct *)globalcontext)->incRED==5 || ((MNStruct *)globalcontext)->incRED==6){

		freqdet=0;
		if(((MNStruct *)globalcontext)->FitLowFreqCutoff == 1){
			double fLow = pow(10.0, Cube[pcount]);
			pcount++;

			double deltaLogF = 0.1;
			double RedMidFreq = 2.0;

			double RedLogDiff = log10(RedMidFreq) - log10(fLow);
			int LogLowFreqs = floor(RedLogDiff/deltaLogF);

			double RedLogSampledDiff = LogLowFreqs*deltaLogF;
			double sampledFLow = floor(log10(fLow)/deltaLogF)*deltaLogF;
			
			int freqStartpoint = 0;


			for(int i =0; i < LogLowFreqs; i++){
				((MNStruct *)globalcontext)->sampleFreq[freqStartpoint]=pow(10.0, sampledFLow + i*RedLogSampledDiff/LogLowFreqs);
				freqStartpoint++;

			}

			for(int i =0;i < FitRedCoeff/2-LogLowFreqs; i++){
				((MNStruct *)globalcontext)->sampleFreq[freqStartpoint]=i+RedMidFreq;
				freqStartpoint++;
			}

		}

                if(((MNStruct *)globalcontext)->FitLowFreqCutoff == 2){
                        double fLow = pow(10.0, Cube[pcount]);
                        pcount++;

                        for(int i =0;i < FitRedCoeff/2; i++){
                                ((MNStruct *)globalcontext)->sampleFreq[i]=((double)(i+1))*fLow;
                        }

                }



		for(int i = 0; i < FitRedCoeff; i++){
			SignalCoeff[startpos + i] = Cube[pcount];
			pcount++;
		}
			
		
		for(int pl = 0; pl < ((MNStruct *)globalcontext)->numFitRedPL; pl ++){

			double Tspan = maxtspan;
                        double f1yr = 1.0/3.16e7;


			double redamp=Cube[pcount];
			pcount++;
			double redindex=Cube[pcount];
			pcount++;

			
			double cornerfreq=0;
			if(((MNStruct *)globalcontext)->incRED==4){
				cornerfreq=pow(10.0, Cube[pcount])/Tspan;
				pcount++;
			}

	
			
			redamp=pow(10.0, redamp);
			if(((MNStruct *)globalcontext)->RedPriorType ==1) { uniformpriorterm +=log(redamp); }



			double Agw=redamp;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				
				freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
				freqs[startpos+i+FitRedCoeff/2]=freqs[startpos+i];
				
				double rho=0;
				if(((MNStruct *)globalcontext)->incRED==5){	
 					rho = (Agw*Agw/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-redindex))/(maxtspan*24*60*60);
				}
				if(((MNStruct *)globalcontext)->incRED==6){
					
			rho = pow((1+(pow((1.0/365.25)/cornerfreq,redindex/2))),2)*(Agw*Agw/12.0/(M_PI*M_PI))/pow((1+(pow(freqs[i]/cornerfreq,redindex/2))),2)/(maxtspan*24*60*60)*pow(f1yr,-3.0);
				}
				//if(rho > pow(10.0,15))rho=pow(10.0,15);
 				powercoeff[i]+= rho;
 				powercoeff[i+FitRedCoeff/2]+= rho;

				

			}
		}
		


		int coefftovary=0;
		double amptovary=0.0;
		if(((MNStruct *)globalcontext)->varyRedCoeff==1){
			coefftovary=int(pow(10.0,Cube[pcount]))-1;
			pcount++;
			amptovary=pow(10.0,Cube[pcount]);
			pcount++;

			powercoeff[coefftovary]=amptovary;
			powercoeff[coefftovary+FitRedCoeff/2]=amptovary;	
		}		
		
		double GWBAmp=0;
		if(((MNStruct *)globalcontext)->incGWB==1){
			GWBAmp=pow(10.0,Cube[pcount]);
			pcount++;
			GWBAmpPrior=log(GWBAmp);
			//uniformpriorterm +=GWBAmpPrior;
			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				double rho = (GWBAmp*GWBAmp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-4.333))/(maxtspan*24*60*60);
				powercoeff[i]+= rho;
				powercoeff[i+FitRedCoeff/2]+= rho;
			}
		}

		for (int i=0; i<FitRedCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[i]);
		}
		
		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);

			}
		}

		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);
			}
		}

		startpos=FitRedCoeff;

    }


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract  Red Shape Events////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	double globalwidth=0;
	if(((MNStruct *)globalcontext)->incRedShapeEvent != 0){
                for(int i =0; i < ((MNStruct *)globalcontext)->incRedShapeEvent; i++){

			int numRedShapeCoeff=((MNStruct *)globalcontext)->numRedShapeCoeff;

                        double EventPos=Cube[pcount];
			pcount++;
                        double EventWidth=Cube[pcount];
			pcount++;

			globalwidth=EventWidth;


			double *Redshapecoeff=new double[numRedShapeCoeff];
			double *RedshapeVec=new double[numRedShapeCoeff];
			for(int c=0; c < numRedShapeCoeff; c++){
				Redshapecoeff[c]=Cube[pcount];
				pcount++;
			}


			double *RedshapeNorm=new double[numRedShapeCoeff];
			for(int c=0; c < numRedShapeCoeff; c++){
				RedshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
			}

			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
				TNothpl(numRedShapeCoeff,HVal,RedshapeVec);
				double Redsignal=0;
				for(int c=0; c < numRedShapeCoeff; c++){
					Redsignal += RedshapeNorm[c]*RedshapeVec[c]*Redshapecoeff[c];
				}

				  Resvec[k] -= Redsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
	
			}




		delete[] Redshapecoeff;
		delete[] RedshapeVec;
		delete[] RedshapeNorm;

		}
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract Sine Wave///////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


	if(((MNStruct *)globalcontext)->incsinusoid == 1){
		double sineamp=pow(10.0,Cube[pcount]);
		pcount++;
		double sinephase=Cube[pcount];
		pcount++;
		double sinefreq=pow(10.0,Cube[pcount])/maxtspan;
		pcount++;		
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			Resvec[o]-= sineamp*sin(2*M_PI*sinefreq*(double)((MNStruct *)globalcontext)->pulse->obsn[o].bat + sinephase);
		}
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////DM Variations////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


       if(((MNStruct *)globalcontext)->incDM==2){

			for (int i=0; i<FitDMCoeff/2; i++){
				int pnum=pcount;
				double pc=Cube[pcount];
				freqs[startpos+i]=((MNStruct *)globalcontext)->sampleFreq[startpos/2 - ((MNStruct *)globalcontext)->incFloatRed+i]/maxtspan;
				freqs[startpos+i+FitDMCoeff/2]=freqs[startpos+i];
	
				powercoeff[startpos+i]=pow(10.0,pc)/(maxtspan*24*60*60);
				powercoeff[startpos+i+FitDMCoeff/2]=powercoeff[startpos+i];
				freqdet=freqdet+2*log(powercoeff[startpos+i]);
				pcount++;
			}
           	 


                for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                        DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                }
                
            for(int i=0;i<FitDMCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                }
        }

        for(int i=0;i<FitDMCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][startpos+i+FitDMCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                }
        }

        }
        else if(((MNStruct *)globalcontext)->incDM==5){


                for(int i = 0; i < FitDMCoeff; i++){
                        SignalCoeff[startpos + i] = Cube[pcount];
                        pcount++;
                }

		for(int pl = 0; pl < ((MNStruct *)globalcontext)->numFitDMPL; pl ++){
			double DMamp=Cube[pcount];
			pcount++;
			double DMindex=Cube[pcount];
			pcount++;

			
   			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
    			

			DMamp=pow(10.0, DMamp);
			if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(DMamp); }
			for (int i=0; i<FitDMCoeff/2; i++){
	
				freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[startpos/2 - ((MNStruct *)globalcontext)->incFloatRed +i]/maxtspan;
				freqs[startpos+i+FitDMCoeff/2]=freqs[startpos+i];
				
 				double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-DMindex))/(maxtspan*24*60*60);	
 				powercoeff[startpos+i]+=rho;
 				powercoeff[startpos+i+FitDMCoeff/2]+=rho;
			}
		}
		
		
		int coefftovary=0;
		double amptovary=0.0;
		if(((MNStruct *)globalcontext)->varyDMCoeff==1){
			coefftovary=int(pow(10.0,Cube[pcount]))-1;
			pcount++;
			amptovary=pow(10.0,Cube[pcount])/(maxtspan*24*60*60);
			pcount++;

			powercoeff[startpos+coefftovary]=amptovary;
			powercoeff[startpos+coefftovary+FitDMCoeff/2]=amptovary;	
		}	
			
		
		for (int i=0; i<FitDMCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}


        for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
        }

        for(int i=0;i<FitDMCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                }
        }

        for(int i=0;i<FitDMCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][startpos+i+FitDMCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                }
        }

	startpos+=FitDMCoeff;

    }
    

    


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////DM Power Spectrum Events////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	
	if(((MNStruct *)globalcontext)->incDMEvent != 0){
                for(int i =0; i < ((MNStruct *)globalcontext)->incDMEvent; i++){
                        double DMamp=DMEventInfo[i][2];
                        double DMindex=DMEventInfo[i][3];
			double DMOff=DMEventInfo[i][4];
			double DMLinear=0;//DMEventInfo[i][5];
			double DMQuad=0;//DMEventInfo[i][6];

                        double Tspan = DMEventInfo[i][1];
                        double f1yr = 1.0/3.16e7;
			int DMEventnumCoeff=int(Tspan/averageTSamp);

                        for (int c=0; c<DMEventnumCoeff; c++){
                                freqs[startpos+c]=((double)(c+1))/Tspan;
                                freqs[startpos+c+DMEventnumCoeff]=freqs[startpos+c];

                                double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freqs[startpos+c]*365.25,(-DMindex))/(maxtspan*24*60*60);
                                powercoeff[startpos+c]+=rho;
                                powercoeff[startpos+c+DMEventnumCoeff]+=rho;
				freqdet=freqdet+2*log(powercoeff[startpos+c]);
                        }


			double *DMshapecoeff=new double[3];
			double *DMshapeVec=new double[3];
			DMshapecoeff[0]=DMOff;
			DMshapecoeff[1]=DMLinear;			
			DMshapecoeff[2]=DMQuad;



			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;


				if(time < DMEventInfo[i][0]+Tspan && time > DMEventInfo[i][0]){

					Resvec[k] -= DMOff*DMVec[k];
					Resvec[k] -= (time-DMEventInfo[i][0])*DMLinear*DMVec[k];
					Resvec[k] -= pow((time-DMEventInfo[i][0]),2)*DMQuad*DMVec[k];

				}
			}

		 	for(int c=0;c<DMEventnumCoeff;c++){
                		for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
					if(time < DMEventInfo[i][0]+Tspan && time > DMEventInfo[i][0]){
						FMatrix[k][startpos+c]=cos(2*M_PI*freqs[startpos+c]*time)*DMVec[k];
                        			FMatrix[k][startpos+c+DMEventnumCoeff]=sin(2*M_PI*freqs[startpos+c]*time)*DMVec[k];
					}
					else{
						FMatrix[k][startpos+c]=0;
						FMatrix[k][startpos+c+DMEventnumCoeff]=0;
					}
                		}
		        }

			startpos+=2*DMEventnumCoeff;



		}
	}



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract  DM Shape Events////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	if(((MNStruct *)globalcontext)->incDMShapeEvent != 0){
                for(int i =0; i < ((MNStruct *)globalcontext)->incDMShapeEvent; i++){

			int numDMShapeCoeff=((MNStruct *)globalcontext)->numDMShapeCoeff;

                        double EventPos=Cube[pcount];
			pcount++;
                        double EventWidth=Cube[pcount];
			pcount++;


			globalwidth=EventWidth;

			double *DMshapecoeff=new double[numDMShapeCoeff];
			double *DMshapeVec=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapecoeff[c]=Cube[pcount];
				pcount++;
			}


			double *DMshapeNorm=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
			}

			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
				TNothpl(numDMShapeCoeff,HVal,DMshapeVec);
				double DMsignal=0;
				for(int c=0; c < numDMShapeCoeff; c++){
					DMsignal += DMshapeNorm[c]*DMshapeVec[c]*DMshapecoeff[c]*DMVec[k];
				}

				  Resvec[k] -= DMsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
	
			}




		delete[] DMshapecoeff;
		delete[] DMshapeVec;
		delete[] DMshapeNorm;

		}
	}





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract  DM Scatter Shape Events///////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


	if(((MNStruct *)globalcontext)->incDMScatterShapeEvent != 0){
                for(int i =0; i < ((MNStruct *)globalcontext)->incDMScatterShapeEvent; i++){

			int numDMShapeCoeff=((MNStruct *)globalcontext)->numDMScatterShapeCoeff;

                        double EventPos=Cube[pcount];
			pcount++;
                        double EventWidth=globalwidth;//Cube[pcount];
			pcount++;
                        double EventFScale=Cube[pcount];
			pcount++;

			//EventWidth=globalwidth;

			double *DMshapecoeff=new double[numDMShapeCoeff];
			double *DMshapeVec=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapecoeff[c]=Cube[pcount];
				pcount++;
			}


			double *DMshapeNorm=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
			}

			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
				TNothpl(numDMShapeCoeff,HVal,DMshapeVec);
				double DMsignal=0;
				for(int c=0; c < numDMShapeCoeff; c++){
					DMsignal += DMshapeNorm[c]*DMshapeVec[c]*DMshapecoeff[c]*pow(DMVec[k],EventFScale/2.0);
				}

				  Resvec[k] -= DMsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
	
			}




		delete[] DMshapecoeff;
		delete[] DMshapeVec;
		delete[] DMshapeNorm;

		}
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract Yearly DM//////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 


	if(((MNStruct *)globalcontext)->yearlyDM == 1){
		double yearlyamp=pow(10.0,Cube[pcount]);
		pcount++;
		double yearlyphase=Cube[pcount];
		pcount++;
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			Resvec[o]-= yearlyamp*sin((2*M_PI/365.25)*(double)((MNStruct *)globalcontext)->pulse->obsn[o].bat + yearlyphase)*DMVec[o];
		}
	}





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Band DM/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


        if(((MNStruct *)globalcontext)->incDMScatter == 1 || ((MNStruct *)globalcontext)->incDMScatter == 2  || ((MNStruct *)globalcontext)->incDMScatter == 3 ){

                if(((MNStruct *)globalcontext)->incDM == 0){
                        for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                                DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                        }
                }



		double startfreq=0;
		double stopfreq=0;


		if(((MNStruct *)globalcontext)->incDMScatter == 1){		
			startfreq = 0;
			stopfreq=1000;
		}
		else  if(((MNStruct *)globalcontext)->incDMScatter == 2){
	                startfreq = 1000;
	                stopfreq=1800;
		}
	        else if(((MNStruct *)globalcontext)->incDMScatter == 3){
	                startfreq = 1800;
	                stopfreq=10000;
	        }



		double DMamp=Cube[pcount];
		pcount++;
		double DMindex=Cube[pcount];
		pcount++;

		double Tspan = maxtspan;
		double f1yr = 1.0/3.16e7;

		DMamp=pow(10.0, DMamp);
		if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(DMamp); }
		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-DMindex))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}

		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;	
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}


		startpos += FitDMScatterCoeff;

    	}


        if(((MNStruct *)globalcontext)->incDMScatter == 4 ){


                if(((MNStruct *)globalcontext)->incDM == 0){
                        for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                                DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                        }
                }



		double Amp1=Cube[pcount];
		pcount++;
		double index1=Cube[pcount];
		pcount++;

		double Amp2=Cube[pcount];
		pcount++;
		double index2=Cube[pcount];
		pcount++;

		double Amp3=Cube[pcount];
		pcount++;
		double index3=Cube[pcount];
		pcount++;
		
		double Tspan = maxtspan;
		double f1yr = 1.0/3.16e7;
		

		Amp1=pow(10.0, Amp1);
		Amp2=pow(10.0, Amp2);
		Amp3=pow(10.0, Amp3);

		if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(Amp1) + log(Amp2) + log(Amp3); }


		double startfreq=0;
		double stopfreq=0;

		/////////////////////////Amp1/////////////////////////////////////////////////////////////////////////////////////////////

		startfreq = 0;
		stopfreq=1000;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (Amp1*Amp1)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index1))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}

		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time);//*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time);//*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}


		startpos=startpos+FitDMScatterCoeff;

		/////////////////////////Amp2/////////////////////////////////////////////////////////////////////////////////////////////

		startfreq = 1000;
		stopfreq=2000;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (Amp2*Amp2)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index2))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}


		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time);//*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time);//*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}


		startpos=startpos+FitDMScatterCoeff;

		/////////////////////////Amp3/////////////////////////////////////////////////////////////////////////////////////////////


		startfreq = 2000;
		stopfreq=10000;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (Amp3*Amp3)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index3))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}


		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time);//*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time);//*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}

		startpos=startpos+FitDMScatterCoeff;
	}	


        if(((MNStruct *)globalcontext)->incDMScatter == 5 ){

	        
		if(((MNStruct *)globalcontext)->incDM == 0){
			for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		       		DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
	      		}
		}

		double Amp1=Cube[pcount];
		pcount++;
		double index1=Cube[pcount];
		pcount++;


		double Tspan = maxtspan;
		double f1yr = 1.0/3.16e7;
		

		Amp1=pow(10.0, Amp1);

		if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(Amp1); }


		double startfreq=0;
		double stopfreq=0;

		/////////////////////////50cm/////////////////////////////////////////////////////////////////////////////////////////////

		startfreq = 0;
		stopfreq=1000;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (Amp1*Amp1)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index1))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}

		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}


		startpos=startpos+FitDMScatterCoeff;

		/////////////////////////20cm/////////////////////////////////////////////////////////////////////////////////////////////

		startfreq = 1000;
		stopfreq=1800;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (Amp1*Amp1)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index1))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}


		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}


		startpos=startpos+FitDMScatterCoeff;

		/////////////////////////10cm/////////////////////////////////////////////////////////////////////////////////////////////


		startfreq = 1800;
		stopfreq=10000;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (Amp1*Amp1)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index1))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}


		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}

		startpos=startpos+FitDMScatterCoeff;
	}	





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Add Group Noise/////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

        if(((MNStruct *)globalcontext)->incGroupNoise > 0){

		for(int g = 0; g < ((MNStruct *)globalcontext)->incGroupNoise; g++){

			int GrouptoFit=0;
			double GroupStartTime = 0;
			double GroupStopTime = 0;
			if(((MNStruct *)globalcontext)->FitForGroup[g][0] == -1){
				GrouptoFit = floor(Cube[pcount]);
				pcount++;
			}
			else{
				GrouptoFit = ((MNStruct *)globalcontext)->FitForGroup[g][0];
				
			}

                        if(((MNStruct *)globalcontext)->FitForGroup[g][1] == 1){
                                GroupStartTime = Cube[pcount];
                                pcount++;
				GroupStopTime = Cube[pcount];
                                pcount++;

                        }
                        else{
                                GroupStartTime = 0;
				GroupStopTime = 10000000.0;

                        }

		

			double GroupAmp=Cube[pcount];
			pcount++;
			double GroupIndex=Cube[pcount];
			pcount++;


		
			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
		

			GroupAmp=pow(10.0, GroupAmp);
			if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(GroupAmp); }

			for (int i=0; i<FitGroupNoiseCoeff/2; i++){

				freqs[startpos+i]=((double)(i+1.0))/maxtspan;
				freqs[startpos+i+FitGroupNoiseCoeff/2]=freqs[startpos+i];
			
				double rho = (GroupAmp*GroupAmp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-GroupIndex))/(maxtspan*24*60*60);	
				powercoeff[startpos+i]+=rho;
				powercoeff[startpos+i+FitGroupNoiseCoeff/2]+=rho;
			}
		
		
			
			for (int i=0; i<FitGroupNoiseCoeff/2; i++){
				freqdet=freqdet+2*log(powercoeff[startpos+i]);
			}


			for(int i=0;i<FitGroupNoiseCoeff/2;i++){
				for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
					//printf("Groups %i %i %i %i\n", i,k,GrouptoFit,((MNStruct *)globalcontext)->sysFlags[k]);
				if(((MNStruct *)globalcontext)->GroupNoiseFlags[k] == GrouptoFit && (double)((MNStruct *)globalcontext)->pulse->obsn[k].bat > GroupStartTime && (double)((MNStruct *)globalcontext)->pulse->obsn[k].bat < GroupStopTime){
				//	if(((MNStruct *)globalcontext)->GroupNoiseFlags[k] == GrouptoFit){
				       		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				        	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time);
						FMatrix[k][startpos+i+FitGroupNoiseCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time);
					}
					else{	
						FMatrix[k][startpos+i]=0;
						FMatrix[k][startpos+i+FitGroupNoiseCoeff/2]=0;
					}
				}
			}



			startpos=startpos+FitGroupNoiseCoeff;
		}

    }


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Add ECORR Coeffs////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	if(((MNStruct *)globalcontext)->incNGJitter >0){
		for(int i =0; i < ((MNStruct *)globalcontext)->numNGJitterEpochs; i++){
			powercoeff[startpos+i] = ECORRPrior[i];
			freqdet = freqdet + log(ECORRPrior[i]);
		}

		for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
			for(int i=0; i < ((MNStruct *)globalcontext)->numNGJitterEpochs; i++){
				FMatrix[k][startpos+i] = ((MNStruct *)globalcontext)->NGJitterMatrix[k][i];
			}
		}

	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Make stochastic signal//////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	double *SignalVec = new double[((MNStruct *)globalcontext)->pulse->nobs];

	
	dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->pulse->nobs,totCoeff,'N');

	for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){


		Resvec[o] = Resvec[o] - SignalVec[o];
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get Time domain likelihood//////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 

 
	static unsigned int oldcw;
	if(((MNStruct *)globalcontext)->uselongdouble > 0 ){
		fpu_fix_start(&oldcw);
	}
	


	double tdet=0;
	double timelike=0;

	for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		timelike+=Resvec[o]*Resvec[o]*Noise[o];
		tdet -= log(Noise[o]);
	}
	dd_real ddtimelike=0.0;
	if(((MNStruct *)globalcontext)->uselongdouble ==1){
		for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			dd_real res = (dd_real)Resvec[o];
			dd_real Nval = (dd_real)Noise[o];
			dd_real chicomp = res*res*Nval;
			ddtimelike += chicomp;
		}
	}

	
        qd_real qdtimelike=0.0;
        if(((MNStruct *)globalcontext)->uselongdouble ==2){
                for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){

                        qd_real res = (qd_real)Resvec[o];
                        qd_real Nval = (qd_real)Noise[o];
                        qd_real chicomp = res*res*Nval;

                        qdtimelike+=chicomp;
                }
        }


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get Fourier domain likelihood///////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 


	double freqlike = 0;
	for(int i = 0; i < totCoeff; i++){
		freqlike += SignalCoeff[i]*SignalCoeff[i]/powercoeff[i];
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get Final likelihood////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 



	double lnew=-0.5*(tdet+freqdet+timelike+freqlike) + uniformpriorterm;
	//printf("like, %g %g %g %g %g %g \n", lnew, tdet, freqdet, timelike, freqlike, uniformpriorterm);

	if(isnan(lnew) || isinf(lnew)){

		lnew=-pow(10.0,20);
		
	}



        if(((MNStruct *)globalcontext)->uselongdouble > 0 ){
                fpu_fix_end(&oldcw);
        }



	delete[] DMVec;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] powercoeff;
	delete[] freqs;
	delete[] Noise;
	delete[] Resvec;
	delete[] SignalCoeff;
	delete[] SignalVec;
	
	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
		delete[]FMatrix[j];
	}
	delete[]FMatrix;

	if(((MNStruct *)globalcontext)->incNGJitter >0){
		delete[] ECORRPrior;
	}

	//printf("CPUChisq: %g %g %g %g %g %g \n",lnew,jointdet,tdet,freqdet,timelike,freqlike);
	return lnew;


}

*/
/*
double  AllTOAJitterLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){



	double *EFAC;
	double *EQUAD;
        long double LDparams[ndim];
        int pcount;

	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
				//printf("Jump: %i %i %i %.25Lg \n", p, k, l, JumpVec[p]);
			}
                 }
           }

	}


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		//if(p==0){printf("before: %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->pulse->obsn[p].origsat, ((MNStruct *)globalcontext)->pulse->obsn[p].sat, ((MNStruct *)globalcontext)->pulse->obsn[p].bat);}
		//printf("sat: %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->pulse->obsn[p].origsat, ((MNStruct *)globalcontext)->pulse->obsn[p].sat, phase);
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;

	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       

	  
	//for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
	//	printf("res: %i %.25Lg %.25Lg \n", p, ((MNStruct *)globalcontext)->pulse->obsn[p].bat, ((MNStruct *)globalcontext)->pulse->obsn[p].residual);  
	//} 
	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }
	  
	  
	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr  - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	 }


	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;

	double shapecoeff[numcoeff];

	for(int i =0; i < numcoeff; i++){

		shapecoeff[i]= Cube[pcount];
		Cube[pcount] = shapecoeff[i];
		pcount++;
	}

	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;

	//printf("beta %g %g %g \n", Cube[pcount], beta, double(((MNStruct *)globalcontext)->ReferencePeriod));

	pcount++;
	
	double lnew = 0;
	int badshape = 0;
	
	 for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
	    int nTOA = t;
	    
	    double detN  = 0;
	    double Chisq  = 0;
	    double logMargindet = 0;
	    double Marginlike = 0;	 
	    double JitterPrior = 0;	   
 
	    double profilelike=0;
	    
	    long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
	    long double FoldingPeriodDays = FoldingPeriod/SECDAY;
	    int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
	    double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
	    double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
	    long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

	    double *Betatimes = new double[Nbins];
	    double **HermiteJitterpoly =  new double*[Nbins];
	    double **JitterBasis  =  new double*[Nbins];
	    double **Hermitepoly =  new double*[Nbins];
	    for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
            for(int i =0;i<Nbins;i++){HermiteJitterpoly[i]=new double[numcoeff+1];for(int j =0;j<numcoeff+1;j++){HermiteJitterpoly[i][j]=0;}}
	    for(int i =0;i<Nbins;i++){JitterBasis[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){JitterBasis[i][j]=0;}}
	
	



	

	    double sigma=noiseval;
	    sigma=pow(10.0, sigma)*sqrt(double(Nbins)/Tobs)*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

	    long double binpos = ModelBats[nTOA];
	    if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

	    long double minpos = binpos - FoldingPeriodDays/2;
	    if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
	    long double maxpos = binpos + FoldingPeriodDays/2;
	    if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];


	    
	    for(int j =0; j < Nbins; j++){
		    long double timediff = 0;
		    long double bintime = ProfileBats[t][j];
		    if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
		    }
		    else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
		    }
		    else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
		    }

		    timediff=timediff*SECDAY;


		    Betatimes[j]=timediff/beta;
		    TNothpl(numcoeff+1,Betatimes[j],HermiteJitterpoly[j]);
		    for(int k =0; k < numcoeff+1; k++){
			    double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI)*((MNStruct *)globalcontext)->Factorials[k]);
			    HermiteJitterpoly[j][k]=HermiteJitterpoly[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);
			    if(k<numcoeff){ Hermitepoly[j][k] = HermiteJitterpoly[j][k];}
			
		    }

			JitterBasis[j][0] = (1.0/sqrt(2.0))*(-1.0*HermiteJitterpoly[j][1]);
		   for(int k =1; k < numcoeff; k++){
			
			JitterBasis[j][k] = (1.0/sqrt(2.0))*(sqrt(double(k))*HermiteJitterpoly[j][k-1] - sqrt(double(k+1))*HermiteJitterpoly[j][k+1]);
		   }	
	    }


	    double *shapevec  = new double[Nbins];
	    double *Jittervec = new double[Nbins];
	
	    double OverallProfileAmp = shapecoeff[0];
	    
	    shapecoeff[0] = 1;

	    dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');
	    dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,numcoeff,'N');

	
	    double *NDiffVec = new double[Nbins];

	    double maxshape=0;
	    double modelflux=0;
	    double testmodelflux=0;	
            double dataflux = 0;
	//	printf("beta is %g \n", beta);

	    for(int j =0; j < Nbins; j++){

		    detN += log(sigma*sigma);

		    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

		    if(shapevec[j] < 0)badshape = 1;
		    NDiffVec[j] = datadiff/(sigma*sigma);
	           // printf("%i %g \n", j, shapevec[j]);
		    if(shapevec[j] > maxshape){ maxshape = shapevec[j];}


		    testmodelflux += shapevec[j]*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
                //    printf("%g %g %g \n", double(ProfileBats[t][1] - ProfileBats[t][0]), double(beta), double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24);
	    }
	    for(int j =0; j < numcoeff; j=j+2){
		modelflux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Factorials[j]/(((MNStruct *)globalcontext)->Factorials[j - j/2]*((MNStruct *)globalcontext)->Factorials[j/2]))*shapecoeff[j];
	   }
	   //printf("fluxes %g %g \n", modelflux, testmodelflux);
	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////
	

	    double **M = new double*[Nbins];
	    double **NM = new double*[Nbins];

	    for(int i =0; i < Nbins; i++){
		    M[i] = new double[3];
		    NM[i] = new double[3];
		    
		    M[i][0] = 1;
		    M[i][1] = shapevec[i];
		    M[i][2] = Jittervec[i];

		    NM[i][0] = M[i][0]/(sigma*sigma);
		    NM[i][1] = M[i][1]/(sigma*sigma);
		    NM[i][2] = M[i][2]/(sigma*sigma);
	    }


	    double **MNM = new double*[3];
	    for(int i =0; i < 3; i++){
		    MNM[i] = new double[3];
	    }

	    dgemm(M, NM , MNM, Nbins, 3,Nbins, 3, 'T', 'N');


	    MNM[2][2] += pow(10.0,20);

	    double *dNM = new double[3];
	    double *TempdNM = new double[3];
	    dgemv(M,NDiffVec,dNM,Nbins,3,'T');

	    for(int i =0; i < 3; i++){
		TempdNM[i] = dNM[i];
	    }


            int info=0;
	    double Margindet = 0;
            dpotrfInfo(MNM, 3, Margindet, info);
            dpotrs(MNM, TempdNM, 3);

	    
	    double maxoffset = TempdNM[0];
            double maxamp = TempdNM[1];
            double maxJitter = TempdNM[2];


	    
	    if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
	      
	      
		Chisq = 0;
		detN = 0;
	      
		double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];//sqrt(double(Nbins)/Tobs)*EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		profnoise = profnoise*profnoise;
		
		double MLSigma = 0;
		int MLSigmaCount = 0;
		for(int j =0; j < Nbins; j++){
		      double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxamp*shapevec[j] - maxoffset - maxJitter*Jittervec[j];
		      if(shapevec[j] <= maxshape*0.01){MLSigma += res*res; MLSigmaCount += 1;}
		}

		MLSigma = sqrt(MLSigma/MLSigmaCount);

		for(int j =0; j < Nbins; j++){
			if(shapevec[j] > maxshape*0.001){
				if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxoffset)/MLSigma > 1){
					dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-maxoffset)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
					//printf("flux: %i %i %g %g \n", t,j,dataflux,(double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-maxoffset);
				}
			}
		}	

		//printf("max: %i %g %g %g \n", t, maxamp, dataflux/modelflux, maxamp/(dataflux/modelflux));

		MLSigma = MLSigma*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];


		
		for(int j =0; j < Nbins; j++){
		  
			double noise = MLSigma*MLSigma;
		
			detN += log(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];

			if(shapevec[j] < 0)badshape = 1;
			NDiffVec[j] = datadiff/(noise);
		       

			Chisq += datadiff*datadiff/(noise);
			
			 NM[j][0] = M[j][0]/(noise);
			 NM[j][1] = M[j][1]/(noise);
			 
			 M[j][2] = M[j][2]*dataflux/modelflux/beta;
			 NM[j][2] = M[j][2]/(noise);
		
		}
		
		
			
		    dgemm(M, NM , MNM, Nbins, 3,Nbins, 3, 'T', 'N');


		    JitterPrior = log(profnoise);
		    MNM[2][2] += 1.0/profnoise;

		    dgemv(M,NDiffVec,dNM,Nbins,3,'T');

		    for(int i =0; i < 3; i++){
			TempdNM[i] = dNM[i];
		    }


		    info=0;
		    Margindet = 0;
		    dpotrfInfo(MNM, 3, Margindet, info);
		    dpotrs(MNM, TempdNM, 3);


			
			if(((MNStruct *)globalcontext)->sysFlags[nTOA] == 4 && nTOA == 317){
			printf("Jitter vals: %i %g %g\n", nTOA, TempdNM[2], sqrt(profnoise));
			for(int j =0; j < Nbins; j++){
				printf("Jitter vals: %i %i %g %g %g %g \n", nTOA, j, TempdNM[2], TempdNM[2]*M[j][2], TempdNM[0]*M[j][0]+TempdNM[1]*M[j][1], (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]);
			}}	
	

			//printf("Jitter vals: %g %g %g %g \n", maxJitter, maxamp, maxJitter/maxamp, TempdNM[2]);
			
		}
		    
		    logMargindet = Margindet;

		    Marginlike = 0;
		    for(int i =0; i < 3; i++){
		    	Marginlike += TempdNM[i]*dNM[i];
		    }

	    profilelike = -0.5*(detN + Chisq + logMargindet + JitterPrior - Marginlike);
	    lnew += profilelike;
	//    if(nTOA == 1){printf("Like: %i  %g %g %g %g \n", nTOA,detN, Chisq, logMargindet, Marginlike);}

	    delete[] shapevec;
	    delete[] Jittervec;
	    delete[] NDiffVec;
	    delete[] dNM;
	    delete[] Betatimes;

	    for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] JitterBasis[j];
		    delete[] HermiteJitterpoly[j];
		    delete[] M[j];
		    delete[] NM[j];
	    }
	    delete[] Hermitepoly;
	    delete[] HermiteJitterpoly;
	    delete[] JitterBasis;
	    delete[] M;
	    delete[] NM;

	    for (int j = 0; j < 3; j++){
		    delete[] MNM[j];
	    }
	    delete[] MNM;
	
	
	 }
	 
	 
	 
	 

	if(badshape == 1){
	//	lnew = -pow(10.0, 10.0);
	}

	//printf("Like: %.10g \n", lnew);
	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;


	return lnew;

}

double  AllTOAStocProfLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){

	//printf("In like \n");

	double *EFAC;
	double *EQUAD;
        long double LDparams[ndim];
        int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];
	//	printf("timing param %i %.25Lg	\n", p, LDparams[pcount]);
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;

	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);      




/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[0]);}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[p]);}
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }


        double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        	SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        	for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
           		SQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
        	}
    	}	  
	  

	//printf("End of White\n");



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Red Noise///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double start,end;
	int go=0;
	for (int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++)
	  {
	    if (((MNStruct *)globalcontext)->pulse->obsn[i].deleted==0)
	      {
		if (go==0)
		  {
		    go = 1;
		    start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    end  = start;
		  }
		else
		  {
		    if (start > (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    if (end < (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      end = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		  }
	      }
	  }

	double maxtspan=1*(end-start);


	
	int FitRedCoeff=2*(((MNStruct *)globalcontext)->numFitRedCoeff);
	int totCoeff=0;
	if(((MNStruct *)globalcontext)->incRED != 0)totCoeff+=FitRedCoeff;



	double FreqDet = 0;

	double *freqs = new double[totCoeff];
   	double *SignalCoeff = new double[totCoeff];

	for(int i = 0; i < totCoeff; i++){
		SignalCoeff[i] = 0;
	}	


	double *powercoeff=new double[totCoeff];
	for(int o=0;o<totCoeff; o++){
		powercoeff[o]=0;
	}
	
	double **FMatrix=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		FMatrix[i]=new double[totCoeff];
		for(int j=0;j<totCoeff;j++){
			FMatrix[i][j]=0;
		}
	}


	int startpos=0;
	if(((MNStruct *)globalcontext)->incRED==5){

		FreqDet=0;

		for(int i = 0; i < FitRedCoeff; i++){
			SignalCoeff[startpos + i] = Cube[pcount];
			pcount++;
		}
			
		
		for(int pl = 0; pl < ((MNStruct *)globalcontext)->numFitRedPL; pl ++){

			double Tspan = maxtspan;
                        double f1yr = 1.0/3.16e7;


			double redamp=Cube[pcount];
			pcount++;
			double redindex=Cube[pcount];
			pcount++;

			
			redamp=pow(10.0, redamp);
			if(((MNStruct *)globalcontext)->RedPriorType ==1) { uniformpriorterm +=log(redamp); }



			double Agw=redamp;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				
				freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
				freqs[startpos+i+FitRedCoeff/2]=freqs[startpos+i];
				
				double rho=0;

 				rho = (Agw*Agw/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-redindex))/(maxtspan*24*60*60);

 				powercoeff[i]+= rho;
 				powercoeff[i+FitRedCoeff/2]+= rho;

				

			}
		}
		
		
		double GWBAmp=0;
		if(((MNStruct *)globalcontext)->incGWB==1){
			GWBAmp=pow(10.0,Cube[pcount]);
			pcount++;

			uniformpriorterm += log(GWBAmp);
			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				double rho = (GWBAmp*GWBAmp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-4.333))/(maxtspan*24*60*60);
				powercoeff[i]+= rho;
				powercoeff[i+FitRedCoeff/2]+= rho;
			}
		}

		for (int i=0; i<FitRedCoeff/2; i++){
			FreqDet=FreqDet+2*log(powercoeff[i]);
		}
		
		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);

			}
		}

		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);
			}
		}

		startpos=FitRedCoeff;

    	}

	double *SignalVec = new double[((MNStruct *)globalcontext)->pulse->nobs];

	
	dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->pulse->nobs,totCoeff,'N');

	double FreqLike = 0;
	for(int i = 0; i < totCoeff; i++){
		FreqLike += SignalCoeff[i]*SignalCoeff[i]/powercoeff[i];
	}



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr  - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	 }


	int maxshapecoeff = 0;
	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;
	int numshapestoccoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;

	double shapecoeff[numcoeff];
	double StocProfCoeffs[numshapestoccoeff];

	for(int i =0; i < numcoeff; i++){
		if(((MNStruct *)globalcontext)->FixProfile==0){
			shapecoeff[i]= Cube[pcount];
			pcount++;
		}
		else{
			shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
			//printf("Fixing mean: %i %g n", i, shapecoeff[i]);
		}
	}

	double beta = ((MNStruct *)globalcontext)->MeanProfileBeta*((MNStruct *)globalcontext)->ReferencePeriod;;//Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;

	pcount++;

	for(int i =0; i < numshapestoccoeff; i++){
		StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
		//uniformpriorterm += log(pow(10.0, Cube[pcount]));
		pcount++;
	}

	if(numcoeff+1>=numshapestoccoeff+1){
		maxshapecoeff=numcoeff+1;
	}
	if(numshapestoccoeff+1 > numcoeff+1){
		maxshapecoeff=numshapestoccoeff+1;
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	double lnew = 0;
	int badshape = 0;

	int minoffpulse=0;
	int maxoffpulse=200;
       // int minoffpulse=500;
       // int maxoffpulse=1500;
	
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
	//	printf("In toa %i \n", t);
		int nTOA = t;

		double detN  = 0;
		double Chisq  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 
		double JitterPrior = 0;	   

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **AllHermiteBasis =  new double*[Nbins];
		double **JitterBasis  =  new double*[Nbins];
		double **Hermitepoly =  new double*[Nbins];

		for(int i =0;i<Nbins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];for(int j =0;j<maxshapecoeff;j++){AllHermiteBasis[i][j]=0;}}
		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
		for(int i =0;i<Nbins;i++){JitterBasis[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){JitterBasis[i][j]=0;}}
	
	



	

		double sigma=noiseval;
		sigma=pow(10.0, sigma);
//		printf("noise details %i %g %g %g \n", t, noiseval, Tobs, EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]]);

		long double binpos = ModelBats[nTOA];

		if(((MNStruct *)globalcontext)->incRED==5){
			long double rednoiseshift = (long double) SignalVec[t];
			binpos+=rednoiseshift/SECDAY;
			//printf("Red noise shift %i %Lg \n", t, rednoiseshift);
		}

		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];


	    
		for(int j =0; j < Nbins; j++){
			long double timediff = 0;
			long double bintime = ProfileBats[t][j];
			if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
			}
			else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
			}
			else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
			}

			timediff=timediff*SECDAY;
			//if(nTOA == 50){printf("timediff %i %g %g\n", j, (double)timediff, beta);}

			Betatimes[j]=timediff/beta;
			TNothpl(maxshapecoeff,Betatimes[j],AllHermiteBasis[j]);

			for(int k =0; k < maxshapecoeff; k++){
				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));//*((MNStruct *)globalcontext)->Factorials[k]);
				AllHermiteBasis[j][k]=AllHermiteBasis[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

				if(k<numcoeff){ Hermitepoly[j][k] = AllHermiteBasis[j][k];}
			
			}

			JitterBasis[j][0] = (1.0/sqrt(2.0))*(-1.0*AllHermiteBasis[j][1]);
			for(int k =1; k < numcoeff; k++){
				JitterBasis[j][k] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1] - sqrt(double(k+1))*AllHermiteBasis[j][k+1]);
			}	
	    }


	    double *shapevec  = new double[Nbins];
	    double *Jittervec = new double[Nbins];
	
	    double OverallProfileAmp = shapecoeff[0];
	    
	    shapecoeff[0] = 1;

	    dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');
    	    dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,numcoeff,'N');
	
	    double *NDiffVec = new double[Nbins];

	    double maxshape=0;
	    double modelflux=0;
	    double testmodelflux=0;	
            double dataflux = 0;


	    for(int j =0; j < Nbins; j++){

		    detN += log(sigma*sigma);

		    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

		    if(shapevec[j] < 0)badshape = 1;
		    NDiffVec[j] = datadiff/(sigma*sigma);

		    if(shapevec[j] > maxshape){ maxshape = shapevec[j];}


		    testmodelflux += shapevec[j]*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;

	    }
	    for(int j =0; j < numcoeff; j=j+2){
		//printf("modelflux: %i %g\n", j, ((MNStruct *)globalcontext)->Binomial[j]);
		modelflux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*shapecoeff[j];
	   }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


		double **M = new double*[Nbins];
		double **NM = new double*[Nbins];

		int Msize = 2+numshapestoccoeff;
		int Mcounter=0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			Msize++;
		}


		for(int i =0; i < Nbins; i++){
			Mcounter=0;
			M[i] = new double[Msize];
			NM[i] = new double[Msize];

			M[i][0] = 1;
			M[i][1] = shapevec[i];

			Mcounter = 2;

			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
//				printf("%i %i %g %g %g \n", nTOA, i, M[i][0], M[i][1], M[i][2]);
				M[i][Mcounter] = Jittervec[i];
				Mcounter++;
			}			



			for(int j = 0; j < numshapestoccoeff; j++){

			    M[i][Mcounter+j] = AllHermiteBasis[i][j];

			}

			for(int j = 0; j < Msize; j++){
				NM[i][j] = M[i][j]/(sigma*sigma);
			//	if(nTOA==50){printf("M %i %i %g %g \n",i,j,M[i][j], sigma); }
			}
		  
		}


		double **MNM = new double*[Msize];
		for(int i =0; i < Msize; i++){
		    MNM[i] = new double[Msize];
		}

		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');
		//printf("MNM %g %g %g %g \n", MNM[0][0], MNM[0][1], MNM[1][0], MNM[1][1]);

		for(int j = 2; j < Msize; j++){
			//printf("before: %i %g \n", j, MNM[j][j]);
			MNM[j][j] += pow(10.0,20);
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);


		double maxoffset = TempdNM[0];
		double maxamp = TempdNM[1];


		//printf("max %i %g %g %g\n", t, maxoffset, maxamp, Margindet);

		      
		Chisq = 0;
		detN = 0;

		double noisemean=0;
		for(int j =minoffpulse; j < maxoffpulse; j++){
			noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
		}

		noisemean = noisemean/(maxoffpulse-minoffpulse);

		double MLSigma = 0;
		int MLSigmaCount = 0;
		for(int j =0; j < Nbins; j++){
		      double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxamp*shapevec[j] - maxoffset;

			res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] -noisemean;
//		      if(shapevec[j] <= maxshape*0.001){MLSigma += res*res; MLSigmaCount += 1;}
		      if(j > minoffpulse && j < maxoffpulse){
				MLSigma += res*res; MLSigmaCount += 1;
		      }
//		      
		}

		MLSigma = sqrt(MLSigma/MLSigmaCount);
		double tempdataflux=0;
		for(int j =0; j < Nbins; j++){
			if(shapevec[j] > maxshape*0.001){
				if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxoffset)/MLSigma > 1){
					dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-maxoffset)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
					tempdataflux += ((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-maxoffset);
			//		if(t==1){
                          //                      printf("dataflux %i %g %g %g %g %g\n", j, maxoffset, ((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]), dataflux, tempdataflux, double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24);
                            //            }

				}
			}
		}	

 

//		MLSigma = MLSigma*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

//		double highfreqnoise = sqrt(double(Nbins)/Tobs)*SQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		double highfreqnoise = SQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		double flatnoise = EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

//		double *testflux=new double[numshapestoccoeff];
//		for(int j = 0; j < numshapestoccoeff; j++){
//			testflux[j] = 0;
//		}


		for(int i =0; i < Nbins; i++){
		  	Mcounter=2;
			double noise = MLSigma*MLSigma +  pow(highfreqnoise*maxamp*shapevec[i],2);

			if(shapevec[i] > maxshape*0.001){
				noise +=  pow(flatnoise*maxamp*maxshape,2);
			}
			//printf("detN: %i %i %g %g %g %g\n", nTOA, i, MLSigma, highfreqnoise, maxamp, noise);
			detN += log(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1];

			if(shapevec[i] < 0)badshape = 1;
			NDiffVec[i] = datadiff/(noise);
		       

			Chisq += datadiff*datadiff/(noise);
	
			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
				//printf("EQUAD: %i %g %g %g %g \n", i, dataflux, modelflux, beta, M[i][2]);
				M[i][2] = M[i][2]*dataflux/modelflux/beta;
				Mcounter++;
			}
			
		   	for(int j = 0; j < numshapestoccoeff; j++){
//				testflux[j] += pow(M[i][Mcounter+j]*sqrt(double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24)*dataflux, 2);
//				M[i][Mcounter+j] = M[i][Mcounter+j]*dataflux*sqrt(Tobs/double(Nbins));
				M[i][Mcounter+j] = M[i][Mcounter+j]*dataflux;

		    	}


			for(int j = 0; j < Msize; j++){
				NM[i][j] = M[i][j]/(noise);
			}


		}
		
//		for(int j = 0; j < numshapestoccoeff; j++){
//			printf("test flux %i %i %g %g \n", t, j, testflux[j], pow(dataflux,2));
//		}
//		delete[] testflux;
			
		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');


		Mcounter=2;
		double JitterDet = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];//sqrt(double(Nbins)/Tobs)*EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
			profnoise = profnoise*profnoise;
		    	JitterDet = log(profnoise);
			//printf("after: %i %g %g\n", Mcounter, MNM[2][2], 1.0/profnoise);
		    	MNM[2][2] += 1.0/profnoise;
			Mcounter++;
		}


		double StocProfDet = 0;
		for(int j = 0; j < numshapestoccoeff; j++){
			if(((MNStruct *)globalcontext)->pulse->obsn[nTOA].sat > 57075.0){
			StocProfDet += log(StocProfCoeffs[j]);
			MNM[Mcounter+j][Mcounter+j] +=  1.0/StocProfCoeffs[j];
			}
			else{
                        StocProfDet += log(pow(10.0, -20));
                        MNM[Mcounter+j][Mcounter+j] +=  1.0/pow(10.0, -20);
			}
		}

		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		info=0;
		Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);



//		if(t==1){
//			double *tempsignal = new double[Nbins];
//			dgemv(M,TempdNM,tempsignal,Nbins,Msize,'N');
	//		for(int j = 0; j < numshapestoccoeff; j++){
	//			double stocflux = 0;
	//			for(int k = 0; k < Nbins; k++){
	//				stocflux += pow(TempdNM[2+j]*M[k][2+j],2);
	//			}
	//			printf("%i %g %g %g %g %g\n", j, StocProfCoeffs[j], TempdNM[2+j], stocflux, dataflux*dataflux/(double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24), double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24);
	//		}
			
			
//			for(int j = 0; j < Nbins; j++){
				//double justprofile = TempdNM[0]*M[j][0] + TempdNM[1]*M[j][1];
//				printf("%i %g %g\n", j,(double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1],tempsignal[j]); 
				//printf("%i %g %g %g %g %g %g %g %g %g %g %g %g \n",j, TempdNM[2]*M[j][2], TempdNM[3]*M[j][3], TempdNM[4]*M[j][4], TempdNM[5]*M[j][5], TempdNM[6]*M[j][6], TempdNM[7]*M[j][7], TempdNM[8]*M[j][8], TempdNM[9]*M[j][9], TempdNM[10]*M[j][10],TempdNM[11]*M[j][11],justprofile, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] );
//			}
//			delete[] tempsignal;
//		}			
		
			    
		logMargindet = Margindet;

		Marginlike = 0;
		for(int i =0; i < Msize; i++){
			Marginlike += TempdNM[i]*dNM[i];
		}

		profilelike = -0.5*(detN + Chisq + logMargindet + StocProfDet + JitterDet - Marginlike);
		lnew += profilelike;
//		printf("Like: %i %.15g %.15g %.15g %.15g %.15g %.15g %.15g\n", nTOA,lnew, detN, Chisq, logMargindet, Marginlike, StocProfDet , JitterDet);

		delete[] shapevec;
		delete[] Jittervec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] JitterBasis[j];
		    delete[] AllHermiteBasis[j];
		    delete[] M[j];
		    delete[] NM[j];
		}
		delete[] Hermitepoly;
		delete[] AllHermiteBasis;
		delete[] JitterBasis;
		delete[] M;
		delete[] NM;

		for (int j = 0; j < Msize; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	}
	 
	 
	 
	 

	if(badshape == 1){
	//	lnew = -pow(10.0, 10.0);
	}

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] powercoeff;
	delete[] freqs;
	delete[] SignalCoeff;
	delete[] SignalVec;
	
	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
		delete[]FMatrix[j];
	}
	delete[]FMatrix;


	lnew += -0.5*(FreqDet + FreqLike) + uniformpriorterm;
	//printf("End Like: %.10g %.10g %.10g %.10g\n", lnew, FreqDet, FreqLike, uniformpriorterm);

	return lnew;

}


double  AllTOALike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){



	double *EFAC;
	double *EQUAD;
	//printf("In one toa like\n");
        long double LDparams[ndim];
        int pcount;

//	for(int i = 0; i < ndim; i++){
//		printf("Cube: %i %g \n", i, Cube[i]);
//	}
	
	
	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){


		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

		//printf("LDparams: %i %.15Lg %.15Lg \n", p,((MNStruct *)globalcontext)->LDpriors[p][0], ((MNStruct *)globalcontext)->LDpriors[p][1]); 


	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	//printf("Phase %.15Lg,  %.15Lg,  %.15Lg \n", phase, LDparams[0], ((MNStruct *)globalcontext)->ReferencePeriod/SECDAY);
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}


	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;



	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       
	  
	  
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		//if(p==0){printf("after: %.25Lg %.25Lg %.15g \n", ((MNStruct *)globalcontext)->pulse->obsn[p].sat, ((MNStruct *)globalcontext)->pulse->obsn[p].bat, (double)((MNStruct *)globalcontext)->pulse->obsn[p].residual);}
		//printf("sat: %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->pulse->obsn[p].origsat, ((MNStruct *)globalcontext)->pulse->obsn[p].sat, phase);
	}
	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }
	  
	  
	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      //long double sat2bat = ((MNStruct *)globalcontext)->pulse->obsn[i].bat - ((MNStruct *)globalcontext)->pulse->obsn[i].sat;
	      //printf("Sat2Bat %i %.25Lg  %.25Lg \n", i, ((MNStruct *)globalcontext)->pulse->obsn[i].sat, sat2bat/SECDAY);
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
// 		    printf("Sat2Bat %i %i %.25Lg %.25Lg %.25Lg \n", i, j, sat2bat/SECDAY, ((MNStruct *)globalcontext)->ProfileData[i][j][0], ProfileBats[i][j]);
	      }
	      
	      //ModelBats[i] = ((MNStruct *)globalcontext)->pulse->obsn[i].bat - phase ;//- ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY - phase;
	      //printf("Bats %i %.20Lg %.20Lg %.20Lg %.20Lg \n", i,ModelBats[i],((MNStruct *)globalcontext)->ProfileInfo[i][5], ((MNStruct *)globalcontext)->pulse->obsn[i].residual, ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr);
	     // printf("Sat2BatConmp %.10Lg %.10Lg \n",((MNStruct *)globalcontext)->ProfileInfo[i][6], ((MNStruct *)globalcontext)->pulse->obsn[i].bat);
	 }


	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;

	double shapecoeff[numcoeff];

	for(int i =0; i < numcoeff; i++){

		shapecoeff[i]= Cube[pcount];
		Cube[pcount] = shapecoeff[i];
		pcount++;
	}
	//printf("Beta: %g %.25Lg \n", Cube[pcount], ((MNStruct *)globalcontext)->ReferencePeriod);
	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;
	pcount++;

	double lnew = 0;
	int badshape = 0;
	
	 for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
	    int nTOA = t;
	    
	    double detN  = 0;
	    double Chisq  = 0;
	    double logMargindet = 0;
	    double Marginlike = 0;	 
	    
	    double profilelike=0;
	    
	    long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
	    long double FoldingPeriodDays = FoldingPeriod/SECDAY;
	    int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
	    double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
	    double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
	    long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

	    double *Betatimes = new double[Nbins];
	    double **Hermitepoly =  new double*[Nbins];
	    for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
	
	



	

	    double sigma=noiseval;
	    sigma=pow(10.0, sigma)*sqrt(double(Nbins)/Tobs)*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

	    //printf("CheckBat %i  %.20Lg %.20Lg %.20Lg  \n", nTOA, ProfileBats[nTOA][0], ProfileBats[nTOA][Nbins-1],((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat);
	    //printf("CheckSat %i  %.20Lg %.20Lg %.20Lg  \n", nTOA,((MNStruct *)globalcontext)->ProfileData[nTOA][0][0], ((MNStruct *)globalcontext)->ProfileData[nTOA][Nbins-1][0],((MNStruct *)globalcontext)->pulse->obsn[nTOA].sat );
	    long double binpos = ModelBats[nTOA];
	    if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;
	    long double minpos = binpos - FoldingPeriodDays/2;
	    //if(t==120){printf("minpos maxpos %i %.25Lg %.25Lg %.25Lg %.25Lg %.25Lg\n", t, ProfileBats[nTOA][0], ProfileBats[nTOA][Nbins-1], binpos, minpos, binpos + FoldingPeriodDays/2);}
	    if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
	    long double maxpos = binpos + FoldingPeriodDays/2;
	    if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];

	    //printf("Obvious time check: %.25Lg %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->ProfileData[nTOA][0][0], FoldingPeriodDays, ((MNStruct *)globalcontext)->ProfileData[nTOA][0][0]+FoldingPeriodDays, ((MNStruct *)globalcontext)->ProfileData[nTOA][Nbins-1][0]); 

	    for(int j =0; j < Nbins; j++){
		    long double timediff = 0;
		    long double bintime = ProfileBats[t][j];
		    if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
		    }
		    else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
		    }
		    else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
		    }

		    timediff=timediff*SECDAY;
	//	    timediff = timediff - 1.6674E-7 + 6.8229E-8 + 7.8479E-8;
	//	    if(t==120){printf("Bins: %i %i %.20Lg %.20Lg %.20Lg %.20Lg %.20Lg  \n", nTOA, j, minpos, maxpos, binpos, bintime, timediff);}
			if(j == 0){
                    //            printf("Model Bat: %i %s %g %g \n", t, ((MNStruct *)globalcontext)->pulse->obsn[t].fname , (double)ProfileBats[t][j], (double)timediff);
                        }


		    Betatimes[j]=timediff/beta;
		    TNothpl(numcoeff,Betatimes[j],Hermitepoly[j]);
		    for(int k =0; k < numcoeff; k++){
			    double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI)*((MNStruct *)globalcontext)->Factorials[k]);
			
		//		printf("%i %i  %i %g %g %g %.25Lg %g\n", t, j, k, Hermitepoly[j][k], Bconst, Betatimes[j], timediff, beta);

			    Hermitepoly[j][k]=Hermitepoly[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);
		    }
	    }


	    double *shapevec  = new double[Nbins];
	    double OverallProfileAmp = shapecoeff[0];
	    
	    shapecoeff[0] = 1;

	    dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');


	
	    double *NDiffVec = new double[Nbins];

	    double maxshape=0;
	    for(int j =0; j < Nbins; j++){

		    detN += log(sigma*sigma);

		    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

		    if(shapevec[j] < 0)badshape = 1;
		    NDiffVec[j] = datadiff/(sigma*sigma);
	            //printf("%i %i %g  %g \n", t, j, datadiff, shapevec[j]);
		    Chisq += datadiff*datadiff/(sigma*sigma);
		    if(shapevec[j] > maxshape){ maxshape = shapevec[j];}
	    }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////
	

	    double **M = new double*[Nbins];
	    double **NM = new double*[Nbins];

	    for(int i =0; i < Nbins; i++){
		    M[i] = new double[2];
		    NM[i] = new double[2];
		    
		    M[i][0] = 1;
		    M[i][1] = shapevec[i];

		    NM[i][0] = M[i][0]/(sigma*sigma);
		    NM[i][1] = M[i][1]/(sigma*sigma);
	    }


	    double **MNM = new double*[2];
	    for(int i =0; i < 2; i++){
		    MNM[i] = new double[2];
	    }

	    dgemm(M, NM , MNM, Nbins, 2,Nbins, 2, 'T', 'N');

	    double *dNM = new double[2];
	    dNM[0] = 0;
	    dNM[1] = 0;

	    for(int j =0; j < Nbins; j++){
		    dNM[0] += NDiffVec[j]*M[j][0];
		    dNM[1] += NDiffVec[j]*M[j][1];
	    }
	    
	    double Margindet = MNM[0][0]*MNM[1][1] - MNM[0][1]*MNM[1][0];

            double maxamp = (1.0/Margindet)*(-dNM[0]*MNM[0][1] + dNM[1]*MNM[0][0]);
            double maxoffset = (1.0/Margindet)*(dNM[0]*MNM[1][1] - dNM[1]*MNM[0][1]);
	    
//	    if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
	      
	      
		Chisq = 0;
		detN = 0;
	      
		double profnoise = sqrt(double(Nbins)/Tobs)*EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		//printf("%i %g %g \n", nTOA, sigma, profnoise*maxamp);
		
		double MLSigma = 0;
		int MLSigmaCount = 0;
		for(int j =0; j < Nbins; j++){
		      double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxamp*shapevec[j] - maxoffset;
		      if(shapevec[j] <= maxshape*0.01){MLSigma += res*res; MLSigmaCount += 1;}
//			if(t==120){printf("res: %i %i %g %g %g %g \n", t, j, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1],  maxamp*shapevec[j], maxoffset, maxamp*shapevec[j] + maxoffset);}
		}
		MLSigma=sqrt(MLSigma/MLSigmaCount)*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];
	        //MLSigma=0.1;	
		for(int j =0; j < Nbins; j++){
		  
			double noise = MLSigma*MLSigma + pow(profnoise*maxamp*shapevec[j],2);
		
			detN += log(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

			if(shapevec[j] < 0)badshape = 1;
			NDiffVec[j] = datadiff/(noise);
//		        printf("%i %i %g %g\n", t, j, maxamp*shapevec[j] + maxoffset, datadiff);
			Chisq += datadiff*datadiff/(noise);
			
			 NM[j][0] = M[j][0]/(noise);
			 NM[j][1] = M[j][1]/(noise);

		
		}
		
		dgemm(M, NM , MNM, Nbins, 2,Nbins, 2, 'T', 'N');
		
		dNM[0] = 0;
		dNM[1] = 0;

		for(int j =0; j < Nbins; j++){
			dNM[0] += NDiffVec[j]*M[j][0];
			dNM[1] += NDiffVec[j]*M[j][1];

		}
		
		Margindet = MNM[0][0]*MNM[1][1] - MNM[0][1]*MNM[1][0];
		
	    maxamp = (1.0/Margindet)*(-dNM[0]*MNM[0][1] + dNM[1]*MNM[0][0]);
            maxoffset = (1.0/Margindet)*(dNM[0]*MNM[1][1] - dNM[1]*MNM[0][1]);

	                for(int j =0; j < Nbins; j++){

                        if(((MNStruct *)globalcontext)->sysFlags[nTOA] == 0){
                                //printf("%i %i %g %g %g %g %g\n", nTOA, j, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1], maxoffset+maxamp*shapevec[j], MLSigma, profnoise*maxamp*shapevec[j], sqrt(MLSigma*MLSigma + pow(profnoise*maxamp*shapevec[j],2)));
                        }
                }

	
//	    }
	    
	    logMargindet += log(Margindet);

	    Marginlike += (1.0/Margindet)*(dNM[0]*MNM[1][1]*dNM[0] + dNM[1]*MNM[0][0]*dNM[1] - 2*dNM[0]*MNM[0][1]*dNM[1]);

	    profilelike = -0.5*(detN + Chisq + logMargindet - Marginlike);
	    lnew += profilelike;
	///	printf("profile like %i %g %g \n", t, profilelike, lnew);
	    //if(t == 1){printf("Like: %i  %g %g %g %g \n", nTOA,detN, Chisq, logMargindet, Marginlike);}

	    delete[] shapevec;
	    delete[] NDiffVec;
	    delete[] dNM;
	    delete[] Betatimes;

	    for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] M[j];
		    delete[] NM[j];
	    }
	    delete[] Hermitepoly;
	    delete[] M;
	    delete[] NM;

	    for (int j = 0; j < 2; j++){
		    delete[] MNM[j];
	    }
	    delete[] MNM;
	
	
	 }
	 
	 
	 
	 

	if(badshape == 1){
	//	lnew = -pow(10.0, 10.0);
	}
	if(isnan(lnew)){
		lnew= -pow(10.0, 10.0);
	}
	//printf("Like: %.10g \n", lnew);
	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;


	return lnew;

}


double  AllTOASim(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){



	double *EFAC;
	double *EQUAD;
	//printf("In one toa like\n");
        long double LDparams[ndim];
        int pcount;

//	for(int i = 0; i < ndim; i++){
//		printf("Cube: %i %g \n", i, Cube[i]);
//	}
	
	
	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){


		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

		printf("LDparams: %i %.15Lg \n", p, LDparams[p]); 


	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	//printf("Phase %.15Lg,  %.15Lg,  %.15Lg \n", phase, LDparams[0], ((MNStruct *)globalcontext)->ReferencePeriod/SECDAY);
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}





	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;



	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       
	  
	  
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		//if(p==0){printf("after: %.25Lg %.25Lg %.15g \n", ((MNStruct *)globalcontext)->pulse->obsn[p].sat, ((MNStruct *)globalcontext)->pulse->obsn[p].bat, (double)((MNStruct *)globalcontext)->pulse->obsn[p].residual);}
		//printf("sat: %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->pulse->obsn[p].origsat, ((MNStruct *)globalcontext)->pulse->obsn[p].sat, phase);
	}
	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }
	  
	  
	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      //long double sat2bat = ((MNStruct *)globalcontext)->pulse->obsn[i].bat - ((MNStruct *)globalcontext)->pulse->obsn[i].sat;
	      //printf("Sat2Bat %i %.25Lg  %.25Lg \n", i, ((MNStruct *)globalcontext)->pulse->obsn[i].sat, sat2bat/SECDAY);
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
// 		    printf("Sat2Bat %i %i %.25Lg %.25Lg %.25Lg \n", i, j, sat2bat/SECDAY, ((MNStruct *)globalcontext)->ProfileData[i][j][0], ProfileBats[i][j]);
	      }
	      
	      //ModelBats[i] = ((MNStruct *)globalcontext)->pulse->obsn[i].bat - phase ;//- ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY - phase;
	      //printf("Bats %i %.20Lg %.20Lg %.20Lg %.20Lg \n", i,ModelBats[i],((MNStruct *)globalcontext)->ProfileInfo[i][5], ((MNStruct *)globalcontext)->pulse->obsn[i].residual, ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr);
	     // printf("Sat2BatConmp %.10Lg %.10Lg \n",((MNStruct *)globalcontext)->ProfileInfo[i][6], ((MNStruct *)globalcontext)->pulse->obsn[i].bat);
	 }


	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;

	double shapecoeff[numcoeff];

	for(int i =0; i < numcoeff; i++){

		shapecoeff[i]= Cube[pcount];
		Cube[pcount] = shapecoeff[i];
		pcount++;
	}
	//printf("Beta: %g %.25Lg \n", Cube[pcount], ((MNStruct *)globalcontext)->ReferencePeriod);
	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;
	pcount++;

	double lnew = 0;
	int badshape = 0;
	
	 for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){

		int nTOA = t;

		double detN  = 0;
		double Chisq  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **Hermitepoly =  new double*[Nbins];
		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}

	



	

		double sigma=noiseval;
		sigma=pow(10.0, sigma)*sqrt(double(Nbins)/Tobs)*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

		//printf("CheckBat %i  %.20Lg %.20Lg %.20Lg  \n", nTOA, ProfileBats[nTOA][0], ProfileBats[nTOA][Nbins-1],((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat);
		//printf("CheckSat %i  %.20Lg %.20Lg %.20Lg  \n", nTOA,((MNStruct *)globalcontext)->ProfileData[nTOA][0][0], ((MNStruct *)globalcontext)->ProfileData[nTOA][Nbins-1][0],((MNStruct *)globalcontext)->pulse->obsn[nTOA].sat );
		long double binpos = ModelBats[nTOA];



		long equadseed = 1000;
                long double equadnoiseval = (long double)TKgaussDev(&equadseed)*pow(10.0, -6.3);
		//if(((MNStruct *)globalcontext)->sysFlags[nTOA] == 4){printf("EQUAD: %i %.25Lg %.25Lg %.25Lg %.25Lg \n",t,equadnoiseval/SECDAY,equadnoiseval, binpos, binpos+equadnoiseval/SECDAY);}
	        //if(((MNStruct *)globalcontext)->sysFlags[nTOA] == 4){binpos+=equadnoiseval/SECDAY;}



		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;
		long double minpos = binpos - FoldingPeriodDays/2;
		//if(t==120){printf("minpos maxpos %i %.25Lg %.25Lg %.25Lg %.25Lg %.25Lg\n", t, ProfileBats[nTOA][0], ProfileBats[nTOA][Nbins-1], binpos, minpos, binpos + FoldingPeriodDays/2);}
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];

		//printf("Obvious time check: %.25Lg %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->ProfileData[nTOA][0][0], FoldingPeriodDays, ((MNStruct *)globalcontext)->ProfileData[nTOA][0][0]+FoldingPeriodDays, ((MNStruct *)globalcontext)->ProfileData[nTOA][Nbins-1][0]); 


		for(int j =0; j < Nbins; j++){

		    long double timediff = 0;
		    long double bintime = ProfileBats[t][j];

		    
		   
		    if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
		    }
		    else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
		    }
		    else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
		    }

		    timediff=timediff*SECDAY;
	//	    timediff = timediff - 1.6674E-7 + 6.8229E-8 + 7.8479E-8;
	//	    if(t==120){printf("Bins: %i %i %.20Lg %.20Lg %.20Lg %.20Lg %.20Lg  \n", nTOA, j, minpos, maxpos, binpos, bintime, timediff);}
			if(j == 0){
                    //            printf("Model Bat: %i %s %g %g \n", t, ((MNStruct *)globalcontext)->pulse->obsn[t].fname , (double)ProfileBats[t][j], (double)timediff);
                        }


		    Betatimes[j]=timediff/beta;
		    TNothpl(numcoeff,Betatimes[j],Hermitepoly[j]);
		    for(int k =0; k < numcoeff; k++){
			    double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI)*((MNStruct *)globalcontext)->Factorials[k]);
			
		//		printf("%i %i  %i %g %g %g %.25Lg %g\n", t, j, k, Hermitepoly[j][k], Bconst, Betatimes[j], timediff, beta);

			    Hermitepoly[j][k]=Hermitepoly[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);
		    }
		}


	    double *shapevec  = new double[Nbins];
	    double OverallProfileAmp = shapecoeff[0];
	    
	    shapecoeff[0] = 1;

	    dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');


	
	    double *NDiffVec = new double[Nbins];

	    double maxshape=0;
	    for(int j =0; j < Nbins; j++){

		    detN += log(sigma*sigma);

		    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

		    if(shapevec[j] < 0)badshape = 1;
		    NDiffVec[j] = datadiff/(sigma*sigma);
	            //printf("%i %i %g  %g \n", t, j, datadiff, shapevec[j]);
		    Chisq += datadiff*datadiff/(sigma*sigma);
		    if(shapevec[j] > maxshape){ maxshape = shapevec[j];}
	    }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////
	

	    double **M = new double*[Nbins];
	    double **NM = new double*[Nbins];

	    for(int i =0; i < Nbins; i++){
		    M[i] = new double[2];
		    NM[i] = new double[2];
		    
		    M[i][0] = 1;
		    M[i][1] = shapevec[i];

		    NM[i][0] = M[i][0]/(sigma*sigma);
		    NM[i][1] = M[i][1]/(sigma*sigma);
	    }


		double **MNM = new double*[2];
		for(int i =0; i < 2; i++){
		    MNM[i] = new double[2];
		}

		dgemm(M, NM , MNM, Nbins, 2,Nbins, 2, 'T', 'N');

		double *dNM = new double[2];
		dNM[0] = 0;
		dNM[1] = 0;

		for(int j =0; j < Nbins; j++){
		    dNM[0] += NDiffVec[j]*M[j][0];
		    dNM[1] += NDiffVec[j]*M[j][1];
		}

		double Margindet = MNM[0][0]*MNM[1][1] - MNM[0][1]*MNM[1][0];

		double maxamp = (1.0/Margindet)*(-dNM[0]*MNM[0][1] + dNM[1]*MNM[0][0]);
		double maxoffset = (1.0/Margindet)*(dNM[0]*MNM[1][1] - dNM[1]*MNM[0][1]);

		double MLSigma = 0;
		int MLSigmaCount = 0;
		for(int j =0; j < Nbins; j++){
		      double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxamp*shapevec[j] - maxoffset;
		      if(shapevec[j] <= maxshape*0.01){MLSigma += res*res; MLSigmaCount += 1;}
		}

		MLSigma = sqrt(MLSigma/MLSigmaCount);

		double dataflux = 0;
		for(int j =0; j < Nbins; j++){
			if(shapevec[j] > maxshape*0.001){
				if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxoffset)/MLSigma > 1){
					dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-maxoffset)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
				}
			}
		}	


		std::string ProfileName =  ((MNStruct *)globalcontext)->pulse->obsn[t].fname;
		std::string fname = "/home/ltl21/scratch/Pulsars/ProfileData/J1909-10cm/profiles/"+ProfileName+".ASCII";
		std::ifstream ProfileFile;
                                
		ProfileFile.open(fname.c_str());
		

		std::string line; 
		getline(ProfileFile,line);
		
		ProfileFile.close();


		std::string fname2 = "/home/ltl21/scratch/Pulsars/ProfileData/J1909-10cm/SimNoNoiseProfiles/"+ProfileName+".ASCII";
	        std::ofstream simfile;
		simfile.open(fname2.c_str());
		simfile << line << "\n";
		for(int i = 0; i < Nbins; i++){
			long seed = 1000;
                        double noiseval = TKgaussDev(&seed)*MLSigma;
			double gaussflux = sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0));
			//printf("%i %i %g %g %g \n", t, i, dataflux, gaussflux, Hermitepoly[i][0]);
			simfile << i << " " <<  std::fixed  << std::setprecision(8) << (dataflux/gaussflux)*Hermitepoly[i][0] << "\n";
		}

		simfile.close();

		delete[] shapevec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] M[j];
		    delete[] NM[j];
		}
		delete[] Hermitepoly;
		delete[] M;
		delete[] NM;

		for (int j = 0; j < 2; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	 }
	 
	 
	 
	printf("made sim \n");
	//printf("Like: %.10g \n", lnew);
	usleep(5000000);
	if(badshape == 1){
	//	lnew = -pow(10.0, 10.0);
	}

	
	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;


	return lnew;

}


double  AllTOAMaxLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){



	double *EFAC;
	double *EQUAD;
	//printf("In one toa like\n");
        long double LDparams[ndim];
        int pcount;

//	for(int i = 0; i < ndim; i++){
//		printf("Cube: %i %g \n", i, Cube[i]);
//	}
	
	
	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){


		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

		//printf("LDparams: %i %.15Lg %.15Lg \n", p,((MNStruct *)globalcontext)->LDpriors[p][0], ((MNStruct *)globalcontext)->LDpriors[p][1]); 


	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	//printf("Phase %.15Lg,  %.15Lg,  %.15Lg \n", phase, LDparams[0], ((MNStruct *)globalcontext)->ReferencePeriod/SECDAY);
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}


	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;



	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);      
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);     
	  
	  
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		//if(p==0){printf("after: %.25Lg %.25Lg %.15g \n", ((MNStruct *)globalcontext)->pulse->obsn[p].sat, ((MNStruct *)globalcontext)->pulse->obsn[p].bat, (double)((MNStruct *)globalcontext)->pulse->obsn[p].residual);}
		//printf("sat: %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->pulse->obsn[p].origsat, ((MNStruct *)globalcontext)->pulse->obsn[p].sat, phase);
	}
	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }
	  
	  
	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      //long double sat2bat = ((MNStruct *)globalcontext)->pulse->obsn[i].bat - ((MNStruct *)globalcontext)->pulse->obsn[i].sat;
	      //printf("Sat2Bat %i %.25Lg  %.25Lg \n", i, ((MNStruct *)globalcontext)->pulse->obsn[i].sat, sat2bat/SECDAY);
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
// 		    printf("Sat2Bat %i %i %.25Lg %.25Lg %.25Lg \n", i, j, sat2bat/SECDAY, ((MNStruct *)globalcontext)->ProfileData[i][j][0], ProfileBats[i][j]);
	      }
	      
	      //ModelBats[i] = ((MNStruct *)globalcontext)->pulse->obsn[i].bat - phase ;//- ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY - phase;
	      //printf("Bats %i %.20Lg %.20Lg %.20Lg %.20Lg \n", i,ModelBats[i],((MNStruct *)globalcontext)->ProfileInfo[i][5], ((MNStruct *)globalcontext)->pulse->obsn[i].residual, ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr);
	     // printf("Sat2BatConmp %.10Lg %.10Lg \n",((MNStruct *)globalcontext)->ProfileInfo[i][6], ((MNStruct *)globalcontext)->pulse->obsn[i].bat);
	 }


	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;

	double shapecoeff[numcoeff];

	for(int i =0; i < numcoeff; i++){

		shapecoeff[i]= Cube[pcount];
		Cube[pcount] = shapecoeff[i];
		pcount++;
	}
	//printf("Beta: %g %.25Lg \n", Cube[pcount], ((MNStruct *)globalcontext)->ReferencePeriod);
	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;
	pcount++;

	double lnew = 0;
	int badshape = 0;
	
	 for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
	    int nTOA = t;
	    
	    double detN  = 0;
	    double Chisq  = 0;
	    double logMargindet = 0;
	    double Marginlike = 0;	 
	    
	    double profilelike=0;
	    
	    long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
	    long double FoldingPeriodDays = FoldingPeriod/SECDAY;
	    int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
	    double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
	    double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
	    long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

	    double *Betatimes = new double[Nbins];
	    double **Hermitepoly =  new double*[Nbins];
	    for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
	
	



	

	    double sigma=noiseval;
	    sigma=pow(10.0, sigma)*sqrt(double(Nbins)/Tobs)*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

	    //printf("CheckBat %i  %.20Lg %.20Lg %.20Lg  \n", nTOA, ProfileBats[nTOA][0], ProfileBats[nTOA][Nbins-1],((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat);
	    //printf("CheckSat %i  %.20Lg %.20Lg %.20Lg  \n", nTOA,((MNStruct *)globalcontext)->ProfileData[nTOA][0][0], ((MNStruct *)globalcontext)->ProfileData[nTOA][Nbins-1][0],((MNStruct *)globalcontext)->pulse->obsn[nTOA].sat );
	    long double binpos = ModelBats[nTOA];
	    if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;
	    long double minpos = binpos - FoldingPeriodDays/2;
	    //if(t==120){printf("minpos maxpos %i %.25Lg %.25Lg %.25Lg %.25Lg %.25Lg\n", t, ProfileBats[nTOA][0], ProfileBats[nTOA][Nbins-1], binpos, minpos, binpos + FoldingPeriodDays/2);}
	    if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
	    long double maxpos = binpos + FoldingPeriodDays/2;
	    if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];

	    //printf("Obvious time check: %.25Lg %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->ProfileData[nTOA][0][0], FoldingPeriodDays, ((MNStruct *)globalcontext)->ProfileData[nTOA][0][0]+FoldingPeriodDays, ((MNStruct *)globalcontext)->ProfileData[nTOA][Nbins-1][0]); 

	    for(int j =0; j < Nbins; j++){
		    long double timediff = 0;
		    long double bintime = ProfileBats[t][j];
		    if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
		    }
		    else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
		    }
		    else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
		    }

		    timediff=timediff*SECDAY;
	//	    timediff = timediff - 1.6674E-7 + 6.8229E-8 + 7.8479E-8;
	//	    if(t==120){printf("Bins: %i %i %.20Lg %.20Lg %.20Lg %.20Lg %.20Lg  \n", nTOA, j, minpos, maxpos, binpos, bintime, timediff);}
			if(j == 0){
                    //            printf("Model Bat: %i %s %g %g \n", t, ((MNStruct *)globalcontext)->pulse->obsn[t].fname , (double)ProfileBats[t][j], (double)timediff);
                        }


		    Betatimes[j]=timediff/beta;
		    TNothpl(numcoeff,Betatimes[j],Hermitepoly[j]);
		    for(int k =0; k < numcoeff; k++){
			    double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI)*((MNStruct *)globalcontext)->Factorials[k]);
			
				//printf("%i %i  %i %g %g %g %.25Lg %g %g\n", t, j, k, Hermitepoly[j][k], Bconst, Betatimes[j], timediff, beta,Hermitepoly[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]));

			    Hermitepoly[j][k]=Hermitepoly[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);
		    }
	    }


	    double *shapevec  = new double[Nbins];
	    double OverallProfileAmp = shapecoeff[0];
	    
	    shapecoeff[0] = 1;

	    dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');


		sigma=1;
	    double *NDiffVec = new double[Nbins];

	    double maxshape=0;
	    for(int j =0; j < Nbins; j++){

		    detN += log(sigma*sigma);

		    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

		    if(shapevec[j] < 0)badshape = 1;
		    NDiffVec[j] = datadiff/(sigma*sigma);
	            //printf("%i %i %g  %g \n", t, j, datadiff, shapevec[j]);
		    Chisq += datadiff*datadiff/(sigma*sigma);
		    if(shapevec[j] > maxshape){ maxshape = shapevec[j];}
	    }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////
	
		
		double **M = new double*[Nbins];
		double **NM = new double*[Nbins];

		int Msize = 1 + numcoeff;

		for(int i =0; i < Nbins; i++){
		    M[i] = new double[Msize];
		    NM[i] = new double[Msize];
		    
		    M[i][0] = 1;

		    for(int k =0; k < numcoeff; k++){
			
			M[i][1+k] = Hermitepoly[i][k];
		    }

		    for(int k =0; k < Msize; k++){
		   	 NM[i][k] = M[i][k]/(sigma*sigma);
		    }

		}


		double **MNM = new double*[Msize];
		for(int i =0; i < Msize; i++){
		    MNM[i] = new double[Msize];
		}

		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');

		for(int k =0; k < numcoeff; k++){
			for(int i =0; i < Nbins; i++){
				//printf("H: %i %i %g \n", i, k, Hermitepoly[i][k]);
			}
		}
		for(int i =0; i < Msize; i++){
			for(int j =0; j < Msize; j++){
				//printf("M: %i %i %g \n", i, j, MNM[i][j]);
			}
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];


		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
			MNM[i][i] += pow(10.0, -12)*MNM[i][i];
		}

		


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);


		

		dgemv(M,TempdNM,shapevec,Nbins,Msize,'N');
			    

	      
	      
		Chisq = 0;
		detN = 0;

		double profnoise = sqrt(double(Nbins)/Tobs)*EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];


		double MLSigma = 0;
		int MLSigmaCount = 0;
		for(int j =0; j < Nbins; j++){
			double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - shapevec[j];
			//printf("%i %g %g \n", j, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] , shapevec[j]);
			MLSigma += res*res; MLSigmaCount += 1;
		}
		MLSigma=sqrt(MLSigma/MLSigmaCount)*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		//printf("MLSigma: %g \n", MLSigma);
		for(int j =0; j < Nbins; j++){

			double noise = MLSigma*MLSigma;

			detN += log(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

			if(shapevec[j] < 0)badshape = 1;
			NDiffVec[j] = datadiff/(noise);
			Chisq += datadiff*datadiff/(noise);
			
		 	for(int k =0; k < Msize; k++){
				NM[j][k] = M[j][k]/(noise);
		    	}


		
		}
		
		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
			MNM[i][i] += pow(10.0, -12)*MNM[i][i];
		}


		info=0;
		Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);


	    
		logMargindet += Margindet;

		Marginlike = 0;
		for(int i =0; i < Msize; i++){
			Marginlike += TempdNM[i]*dNM[i];
		}
		//printf("%g %g %g %g \n", detN, Chisq, logMargindet, Marginlike);
		profilelike = -0.5*(detN + Chisq + logMargindet - Marginlike);
		lnew += profilelike;

		delete[] shapevec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] TempdNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] M[j];
		    delete[] NM[j];
		}
		delete[] Hermitepoly;
		delete[] M;
		delete[] NM;

		for (int j = 0; j < Msize; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	 }
	 
	 
	 
	 

	if(badshape == 1){
	//	lnew = -pow(10.0, 10.0);
	}
	if(isnan(lnew)){
		lnew= -pow(10.0, 10.0);
	}
	printf("Like: %.10g \n", lnew);
	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;


	return lnew;

}


double  AllTOAWriteMaxLike(std::string longname, int &ndim){



	double *Cube = new double[ndim];
        int number_of_lines = 0;

        std::ifstream checkfile;
        std::string checkname = longname+"_phys_live.txt";
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        checkfile.close();

	std::ifstream summaryfile;
	std::string fname = longname+"_phys_live.txt";
	summaryfile.open(fname.c_str());



	printf("Getting ML \n");
	double maxlike = -1.0*pow(10.0,10);
	for(int i=0;i<number_of_lines;i++){

		std::string line;
		getline(summaryfile,line);
		std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);

		double like = paramlist[ndim];

		if(like > maxlike){
			maxlike = like;
			 for(int i = 0; i < ndim; i++){
                        	 Cube[i] = paramlist[i];
                	 }
		}

	}
	summaryfile.close();

	double *EFAC;
	double *EQUAD;
        long double LDparams[ndim];
        int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];
	//	printf("timing param %i %.25Lg	\n", p, LDparams[pcount]);
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;

	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);      




/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }


        double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        	SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        	for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
           		SQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
        	}
    	}	  
	  





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Red Noise///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double start,end;
	int go=0;
	for (int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++)
	  {
	    if (((MNStruct *)globalcontext)->pulse->obsn[i].deleted==0)
	      {
		if (go==0)
		  {
		    go = 1;
		    start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    end  = start;
		  }
		else
		  {
		    if (start > (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    if (end < (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      end = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		  }
	      }
	  }

	double maxtspan=1*(end-start);


	
	int FitRedCoeff=2*(((MNStruct *)globalcontext)->numFitRedCoeff);
	int totCoeff=0;
	if(((MNStruct *)globalcontext)->incRED != 0)totCoeff+=FitRedCoeff;



	double FreqDet = 0;

	double *freqs = new double[totCoeff];
   	double *SignalCoeff = new double[totCoeff];

	for(int i = 0; i < totCoeff; i++){
		SignalCoeff[i] = 0;
	}	


	double *powercoeff=new double[totCoeff];
	for(int o=0;o<totCoeff; o++){
		powercoeff[o]=0;
	}
	
	double **FMatrix=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		FMatrix[i]=new double[totCoeff];
		for(int j=0;j<totCoeff;j++){
			FMatrix[i][j]=0;
		}
	}


	int startpos=0;
	if(((MNStruct *)globalcontext)->incRED==5){

		FreqDet=0;

		for(int i = 0; i < FitRedCoeff; i++){
			SignalCoeff[startpos + i] = Cube[pcount];
			pcount++;
		}
			
		
		for(int pl = 0; pl < ((MNStruct *)globalcontext)->numFitRedPL; pl ++){

			double Tspan = maxtspan;
                        double f1yr = 1.0/3.16e7;


			double redamp=Cube[pcount];
			pcount++;
			double redindex=Cube[pcount];
			pcount++;

			
			redamp=pow(10.0, redamp);
			if(((MNStruct *)globalcontext)->RedPriorType ==1) { uniformpriorterm +=log(redamp); }



			double Agw=redamp;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				
				freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
				freqs[startpos+i+FitRedCoeff/2]=freqs[startpos+i];
				
				double rho=0;

 				rho = (Agw*Agw/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-redindex))/(maxtspan*24*60*60);

 				powercoeff[i]+= rho;
 				powercoeff[i+FitRedCoeff/2]+= rho;

				

			}
		}
		
		
		double GWBAmp=0;
		if(((MNStruct *)globalcontext)->incGWB==1){
			GWBAmp=pow(10.0,Cube[pcount]);
			pcount++;

			uniformpriorterm += log(GWBAmp);
			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				double rho = (GWBAmp*GWBAmp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-4.333))/(maxtspan*24*60*60);
				powercoeff[i]+= rho;
				powercoeff[i+FitRedCoeff/2]+= rho;
			}
		}

		for (int i=0; i<FitRedCoeff/2; i++){
			FreqDet=FreqDet+2*log(powercoeff[i]);
		}
		
		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);

			}
		}

		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);
			}
		}

		startpos=FitRedCoeff;

    	}

	double *SignalVec = new double[((MNStruct *)globalcontext)->pulse->nobs];

	
	dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->pulse->nobs,totCoeff,'N');

	double FreqLike = 0;
	for(int i = 0; i < totCoeff; i++){
		FreqLike += SignalCoeff[i]*SignalCoeff[i]/powercoeff[i];
	}



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr  - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	 }


	int maxshapecoeff = 0;
	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;
	int numshapestoccoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;

	double shapecoeff[numcoeff];
	double StocProfCoeffs[numshapestoccoeff];

	for(int i =0; i < numcoeff; i++){
		if(((MNStruct *)globalcontext)->FixProfile==0){
			shapecoeff[i]= Cube[pcount];
			pcount++;
		}
		else{
			shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
			printf("Fixing mean: %i %g n", i, shapecoeff[i]);
		}
	}
	
	double beta=0;
	if(((MNStruct *)globalcontext)->FixProfile==0){
		beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;
		pcount++;
	}
	else{
		beta = ((MNStruct *)globalcontext)->MeanProfileBeta*((MNStruct *)globalcontext)->ReferencePeriod;
	}	

	for(int i =0; i < numshapestoccoeff; i++){
		StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
		//uniformpriorterm += log(pow(10.0, Cube[pcount]));
		pcount++;
	}

	if(numcoeff+1>=numshapestoccoeff+1){
		maxshapecoeff=numcoeff+1;
	}
	if(numshapestoccoeff+1 > numcoeff+1){
		maxshapecoeff=numshapestoccoeff+1;
	}

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	double lnew = 0;
	int badshape = 0;

	double *meanshape = new double[numcoeff];
	double *maxshape = new double[numcoeff];
	double *minshape = new double[numcoeff];

	 for(int c = 0; c < numcoeff; c++){
		meanshape[c] = 0;
		maxshape[c] = -1000;
		minshape[c] = 1000;
	}
			
        int minoffpulse=750;
        int maxoffpulse=1250;
       //int minoffpulse=500;
       //int maxoffpulse=1500;

	
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){

		int nTOA = t;

		double detN  = 0;
		double Chisq  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 
		double JitterPrior = 0;	   

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **AllHermiteBasis =  new double*[Nbins];
		double **JitterBasis  =  new double*[Nbins];
		double **Hermitepoly =  new double*[Nbins];

		for(int i =0;i<Nbins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];for(int j =0;j<maxshapecoeff;j++){AllHermiteBasis[i][j]=0;}}
		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
		for(int i =0;i<Nbins;i++){JitterBasis[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){JitterBasis[i][j]=0;}}
	
	



	

		double sigma=noiseval;
		sigma=pow(10.0, sigma);
		//printf("noise details %i %g %g %g \n", t, noiseval, Tobs, EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]]);

		long double binpos = ModelBats[nTOA];

		if(((MNStruct *)globalcontext)->incRED==5){
			long double rednoiseshift = (long double) SignalVec[t];
			binpos+=rednoiseshift/SECDAY;
			//printf("Red noise shift %i %Lg \n", t, rednoiseshift);
		}

		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];


	    
		for(int j =0; j < Nbins; j++){
			long double timediff = 0;
			long double bintime = ProfileBats[t][j];
			if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
			}
			else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
			}
			else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
			}

			timediff=timediff*SECDAY;


			Betatimes[j]=timediff/beta;
			TNothpl(maxshapecoeff,Betatimes[j],AllHermiteBasis[j]);

			for(int k =0; k < maxshapecoeff; k++){

				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));//*((MNStruct *)globalcontext)->Factorials[k]);
				AllHermiteBasis[j][k]=AllHermiteBasis[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);
			//	if(nTOA==0 && j == 0){printf("%i %g \n", k, AllHermiteBasis[j][k]);}
				if(k<numcoeff){ Hermitepoly[j][k] = AllHermiteBasis[j][k];}
			
			}

			JitterBasis[j][0] = (1.0/sqrt(2.0))*(-1.0*AllHermiteBasis[j][1]);
			for(int k =1; k < numcoeff; k++){
				JitterBasis[j][k] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1] - sqrt(double(k+1))*AllHermiteBasis[j][k+1]);
			}	
	    }


	    double *shapevec  = new double[Nbins];
	    double *Jittervec = new double[Nbins];
	
	    double OverallProfileAmp = shapecoeff[0];
	    
	    shapecoeff[0] = 1;

	    dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');
    	    dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,numcoeff,'N');
	
	    double *NDiffVec = new double[Nbins];

	    double maxshape=0;
	    double modelflux=0;
	    double testmodelflux=0;	
            double dataflux = 0;


	    for(int j =0; j < Nbins; j++){

		    detN += log(sigma*sigma);

		    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

		    if(shapevec[j] < 0)badshape = 1;
		    NDiffVec[j] = datadiff/(sigma*sigma);

		    if(shapevec[j] > maxshape){ maxshape = shapevec[j];}


		    testmodelflux += shapevec[j]*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;

	    }
	    for(int j =0; j < numcoeff; j=j+2){
		modelflux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*shapecoeff[j];
		//printf("binomial %i %g %g\n", j, ((MNStruct *)globalcontext)->Binomial[j], ((MNStruct *)globalcontext)->Factorials[j]/(((MNStruct *)globalcontext)->Factorials[j - j/2]*((MNStruct *)globalcontext)->Factorials[j/2]));
	   }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


		double **M = new double*[Nbins];
		double **NM = new double*[Nbins];

		int Msize = 2+numshapestoccoeff;
		int Mcounter=0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			Msize++;
		}


		for(int i =0; i < Nbins; i++){
			Mcounter=0;
			M[i] = new double[Msize];
			NM[i] = new double[Msize];

			M[i][0] = 1;
			M[i][1] = shapevec[i];

			Mcounter = 2;

			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
//				printf("%i %i %g %g %g \n", nTOA, i, M[i][0], M[i][1], M[i][2]);
				M[i][Mcounter] = Jittervec[i];
				Mcounter++;
			}			



			for(int j = 0; j < numshapestoccoeff; j++){

			    M[i][Mcounter+j] = AllHermiteBasis[i][j];

			}

			for(int j = 0; j < Msize; j++){
				NM[i][j] = M[i][j]/(sigma*sigma);
		//		printf("M %i %i %g %g \n",i,j,M[i][j], sigma); 
			}
		  
		}


		double **MNM = new double*[Msize];
		for(int i =0; i < Msize; i++){
		    MNM[i] = new double[Msize];
		}

		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');
//		printf("MNM %g %g %g %g \n", MNM[0][0], MNM[0][1], MNM[1][0], MNM[1][1]);

		for(int j = 2; j < Msize; j++){
			MNM[j][j] += pow(10.0,20);
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);


		double maxoffset = TempdNM[0];
		double maxamp = TempdNM[1];



		printf("max %i %g %g \n", t, maxoffset, maxamp);

		      
		Chisq = 0;
		detN = 0;

		double noisemean=0.0;

		for(int j =minoffpulse; j < maxoffpulse; j++){
			noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
		}

		noisemean=noisemean/(maxoffpulse-minoffpulse);
		double MLSigma = 0;
		double MLSigma2 = 0;
		int MLSigmaCount2 = 0;
		int MLSigmaCount = 0;
		for(int j =0; j < Nbins; j++){
		      double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxamp*shapevec[j] - maxoffset;
		      //if(shapevec[j] <= maxshape*0.001){MLSigma += res*res; MLSigmaCount += 1; printf("Adding to Noise %i %i %g %g \n", t, j,(double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1], maxamp*shapevec[j] + maxoffset);}
			if(j > minoffpulse && j < maxoffpulse){
			//	if(nTOA == 0){printf("res: %i %g \n", j, res);}
				MLSigma += ((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)*((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean); MLSigmaCount += 1;
			}
			else{
				MLSigma2 += res*res; MLSigmaCount2 += 1;
			}
		}

		MLSigma = sqrt(MLSigma/MLSigmaCount);
		MLSigma2 = sqrt(MLSigma2/MLSigmaCount2);
		double tempdataflux=0;
		for(int j =0; j < Nbins; j++){
			if(shapevec[j] > maxshape*0.001){
				if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxoffset)/MLSigma > 1){
					dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-maxoffset)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
					tempdataflux += ((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-maxoffset);
			//		if(t==1){
                          //                      printf("dataflux %i %g %g %g %g %g\n", j, maxoffset, ((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]), dataflux, tempdataflux, double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24);
                            //            }

				}
			}
		}	

 
//		printf("TOA %i %g %g %g %g\n", t, EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]], MLSigma, MLSigma2, MLSigma2/MLSigma);
		MLSigma = MLSigma;//*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

//		double highfreqnoise = sqrt(double(Nbins)/Tobs)*SQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		double highfreqnoise = SQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		double flatnoise = EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

//		double *testflux=new double[numshapestoccoeff];
//		for(int j = 0; j < numshapestoccoeff; j++){
//			testflux[j] = 0;
//		}



		double *profilenoise = new double[Nbins];
		for(int i =0; i < Nbins; i++){
		  	Mcounter=2;
			double noise = MLSigma*MLSigma +  pow(highfreqnoise*maxamp*shapevec[i],2);

			if(shapevec[i] > maxshape*0.001){
				noise +=  pow(flatnoise*maxamp*maxshape,2);
			}


			profilenoise[i] = sqrt(noise);
			//printf("detN: %i %i %g %g %g %g\n", nTOA, i, MLSigma, highfreqnoise, maxamp, noise);
			detN += log(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1];

			if(shapevec[i] < 0)badshape = 1;
			NDiffVec[i] = datadiff/(noise);
		       

			Chisq += datadiff*datadiff/(noise);
	
			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
				M[i][2] = M[i][2]*dataflux/modelflux/beta;
				Mcounter++;
			}
			
		   	for(int j = 0; j < numshapestoccoeff; j++){
//				testflux[j] += pow(M[i][Mcounter+j]*sqrt(double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24)*dataflux, 2);
//				M[i][Mcounter+j] = M[i][Mcounter+j]*dataflux*sqrt(Tobs/double(Nbins));
				M[i][Mcounter+j] = M[i][Mcounter+j]*dataflux;

		    	}


			for(int j = 0; j < Msize; j++){
				NM[i][j] = M[i][j]/(noise);
			}


		}
		
//		for(int j = 0; j < numshapestoccoeff; j++){
//			printf("test flux %i %i %g %g \n", t, j, testflux[j], pow(dataflux,2));
//		}
//		delete[] testflux;
			
		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');


		Mcounter=2;
		double JitterDet = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];//sqrt(double(Nbins)/Tobs)*EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
			profnoise = profnoise*profnoise;
		    	JitterDet = log(profnoise);
		    	MNM[2][2] += 1.0/profnoise;
			Mcounter++;
		}


		double StocProfDet = 0;
		for(int j = 0; j < numshapestoccoeff; j++){
			StocProfDet += log(StocProfCoeffs[j]);
			MNM[Mcounter+j][Mcounter+j] += 1.0/StocProfCoeffs[j];
		}

		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		info=0;
		Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);

		Marginlike = 0;
                for(int i =0; i < Msize; i++){
                        Marginlike += TempdNM[i]*dNM[i];
                }



		double finaloffset = TempdNM[0];
		double finalamp = TempdNM[1];
		double finalJitter = TempdNM[2];

		double *StocVec = new double[Nbins];


		dgemv(M,TempdNM,shapevec,Nbins,Msize,'N');


		 for(int c = 0; c < numcoeff; c++){
			meanshape[c] += shapecoeff[c];
		}
		

		TempdNM[0] = 0;
		TempdNM[1] = 0;
		TempdNM[2] = 0;

                 for(int c = 3; c < Msize; c++){
                        meanshape[c-3] += (TempdNM[c]/finalamp)*dataflux;
			//printf("stoc: %i %g %g\n", c, (TempdNM[c]/finalamp)*dataflux, (TempdNM[c]/finalamp)/dataflux);
                }


		dgemv(M,TempdNM,StocVec,Nbins,Msize,'N');

		std::ofstream profilefile;
		std::string ProfileName =  ((MNStruct *)globalcontext)->pulse->obsn[t].fname;
		std::string dname = longname+ProfileName+"-Profile.txt";
	
		profilefile.open(dname.c_str());
		double MLSigma3 = 0;	
		double MLWeight3 = 0;	
		for(int i =0; i < Nbins; i++){
			MLSigma3 += pow(((double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1]-shapevec[i]),2)*pow(1.0/profilenoise[i], 2);
			MLWeight3 += pow(1.0/profilenoise[i], 2);
			profilefile << i << " " << std::setprecision(10) << (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1] << " " << shapevec[i] << " " << profilenoise[i] << " " << finaloffset + finalamp*M[i][1] << " " << finalJitter*M[i][2] << " " << finaloffset + StocVec[i] << "\n";

		}
		printf("MLsigma: %s %g %g %g \n", ProfileName.c_str(), MLSigma, MLSigma2, MLSigma2/MLSigma);
	    	profilefile.close();
		delete[] profilenoise;
		delete[] StocVec;

		logMargindet = Margindet;


		profilelike = -0.5*(detN + Chisq + logMargindet + StocProfDet + JitterDet - Marginlike);
		lnew += profilelike;
//		printf("Like: %i  %.15g %.15g %.15g %.15g %.15g %.15g\n", nTOA,detN, Chisq, logMargindet, Marginlike, StocProfDet , JitterDet);

		delete[] shapevec;
		delete[] Jittervec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] JitterBasis[j];
		    delete[] AllHermiteBasis[j];
		    delete[] M[j];
		    delete[] NM[j];
		}
		delete[] Hermitepoly;
		delete[] AllHermiteBasis;
		delete[] JitterBasis;
		delete[] M;
		delete[] NM;

		for (int j = 0; j < Msize; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	}
	 
	 
	 
	 for(int c = 0; c < numcoeff; c++){
		double mval = meanshape[c]/meanshape[0];
		printf("ShapePriors[%i][0] = %.10g         %.10g\n", c, mval-mval*0.5, mval);
		printf("ShapePriors[%i][1] = %.10g         %.10g\n",c, mval+mval*0.5, mval);
	}
	 

	if(badshape == 1){
	//	lnew = -pow(10.0, 10.0);
	}

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] powercoeff;
	delete[] freqs;
	delete[] SignalCoeff;
	delete[] SignalVec;
	
	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
		delete[]FMatrix[j];
	}
	delete[]FMatrix;


	printf("lnew: %g \n", lnew);

	return lnew;

}


double  AllTOAMarginStocProfLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){

	printf("in like\n");

	double *EFAC;
	double *EQUAD;
        long double LDparams[ndim];
        int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];
	//	printf("timing param %i %.25Lg	\n", p, LDparams[pcount]);
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;

	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);      
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       




/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[0]);}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[p]);}
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }


        double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        	SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        	for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
           		SQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
        	}
    	}	  
	  





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Red Noise///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double start,end;
	int go=0;
	for (int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++)
	  {
	    if (((MNStruct *)globalcontext)->pulse->obsn[i].deleted==0)
	      {
		if (go==0)
		  {
		    go = 1;
		    start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    end  = start;
		  }
		else
		  {
		    if (start > (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    if (end < (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      end = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		  }
	      }
	  }

	double maxtspan=1*(end-start);


	
	int FitRedCoeff=2*(((MNStruct *)globalcontext)->numFitRedCoeff);
	int totCoeff=0;
	if(((MNStruct *)globalcontext)->incRED != 0)totCoeff+=FitRedCoeff;



	double FreqDet = 0;

	double *freqs = new double[totCoeff];
   	double *SignalCoeff = new double[totCoeff];

	for(int i = 0; i < totCoeff; i++){
		SignalCoeff[i] = 0;
	}	


	double *powercoeff=new double[totCoeff];
	for(int o=0;o<totCoeff; o++){
		powercoeff[o]=0;
	}
	
	double **FMatrix=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		FMatrix[i]=new double[totCoeff];
		for(int j=0;j<totCoeff;j++){
			FMatrix[i][j]=0;
		}
	}


	int startpos=0;
	if(((MNStruct *)globalcontext)->incRED==5){

		FreqDet=0;

		for(int i = 0; i < FitRedCoeff; i++){
			SignalCoeff[startpos + i] = Cube[pcount];
			pcount++;
		}
			
		
		for(int pl = 0; pl < ((MNStruct *)globalcontext)->numFitRedPL; pl ++){

			double Tspan = maxtspan;
                        double f1yr = 1.0/3.16e7;


			double redamp=Cube[pcount];
			pcount++;
			double redindex=Cube[pcount];
			pcount++;

			
			redamp=pow(10.0, redamp);
			if(((MNStruct *)globalcontext)->RedPriorType ==1) { uniformpriorterm +=log(redamp); }



			double Agw=redamp;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				
				freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
				freqs[startpos+i+FitRedCoeff/2]=freqs[startpos+i];
				
				double rho=0;

 				rho = (Agw*Agw/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-redindex))/(maxtspan*24*60*60);

 				powercoeff[i]+= rho;
 				powercoeff[i+FitRedCoeff/2]+= rho;

				

			}
		}
		
		
		double GWBAmp=0;
		if(((MNStruct *)globalcontext)->incGWB==1){
			GWBAmp=pow(10.0,Cube[pcount]);
			pcount++;

			uniformpriorterm += log(GWBAmp);
			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				double rho = (GWBAmp*GWBAmp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-4.333))/(maxtspan*24*60*60);
				powercoeff[i]+= rho;
				powercoeff[i+FitRedCoeff/2]+= rho;
			}
		}

		for (int i=0; i<FitRedCoeff/2; i++){
			FreqDet=FreqDet+2*log(powercoeff[i]);
		}
		
		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);

			}
		}

		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);
			}
		}

		startpos=FitRedCoeff;

    	}

	double *SignalVec = new double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
		SignalVec[k] = 0;
	}

	
	dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->pulse->nobs,totCoeff,'N');

	double FreqLike = 0;
	for(int i = 0; i < totCoeff; i++){
		FreqLike += SignalCoeff[i]*SignalCoeff[i]/powercoeff[i];
	}



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract  DM Shape Events////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	if(((MNStruct *)globalcontext)->incDMShapeEvent != 0){
                for(int i =0; i < ((MNStruct *)globalcontext)->incDMShapeEvent; i++){

			int numDMShapeCoeff=((MNStruct *)globalcontext)->numDMShapeCoeff;

                        double EventPos=Cube[pcount];
			pcount++;
                        double EventWidth=Cube[pcount];
			pcount++;



			double *DMshapecoeff=new double[numDMShapeCoeff];
			double *DMshapeVec=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapecoeff[c]=Cube[pcount];
				pcount++;
			}


			double *DMshapeNorm=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
			}

			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
				TNothpl(numDMShapeCoeff,HVal,DMshapeVec);
				double DMsignal=0;
				for(int c=0; c < numDMShapeCoeff; c++){
					DMsignal += DMshapeNorm[c]*DMshapeVec[c]*DMshapecoeff[c];
				}

				  SignalVec[k] += DMsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
	
			}




		delete[] DMshapecoeff;
		delete[] DMshapeVec;
		delete[] DMshapeNorm;

		}
	}



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr  - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	 }


	int maxshapecoeff = 0;
	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;
	int numshapestoccoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;


	double StocProfCoeffs[numshapestoccoeff];
	double profileamps[((MNStruct *)globalcontext)->pulse->nobs];

	profileamps[0] = 1;
	for(int i =1; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
		profileamps[i]= Cube[pcount];
		pcount++;
	}

	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;

	pcount++;

	for(int i =0; i < numshapestoccoeff; i++){
		StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
		//uniformpriorterm += log(pow(10.0, Cube[pcount]));
		pcount++;
	}

	if(numcoeff+1>numshapestoccoeff+1){
		maxshapecoeff=numcoeff+1;
	}
	if(numshapestoccoeff+1 > numcoeff+1){
		maxshapecoeff=numshapestoccoeff+1;
	}

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	printf("starting profile stuff \n");
	double lnew = 0;
	int badshape = 0;

	int totalcoeff = numcoeff;
	int totalbins = 0;
	
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
		totalbins  += (int)((MNStruct *)globalcontext)->ProfileInfo[t][1];
		totalcoeff += numshapestoccoeff;
		totalcoeff += 1;    //For the arbitrary offset
	}

	double *AllNoise = new double[totalbins];
	double *AllD = new double[totalbins];
	double *dNM = new double[totalcoeff];
	double *TempdNM = new double[totalcoeff];

	double **AllProfiles = new double*[totalcoeff];
	double **NAllProfiles = new double*[totalcoeff];
	for(int t = 0; t < totalcoeff; t++){
		AllProfiles[t] = new double[totalbins];
		NAllProfiles[t] = new double[totalbins];
		for(int i = 0; i < totalcoeff; i++){
			AllProfiles[t][i] = 0;
			NAllProfiles[t][i] = 0;
		}
	}
	
	int stocprofcount = numcoeff+((MNStruct *)globalcontext)->pulse->nobs;
	int bincount = 0;
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){



		double MLSigma = 0;
		int MLSigmaCount = 0;
		double noisemean=0;

		for(int j =500; j < 1500; j++){
			noisemean += (double)((MNStruct *)globalcontext)->ProfileData[t][j][1];
			MLSigmaCount += 1;
		}

		noisemean = noisemean/MLSigmaCount;

		for(int j =500; j < 1500; j++){
			double res = (double)((MNStruct *)globalcontext)->ProfileData[t][j][1] - noisemean;
			MLSigma += res*res; 
		}

		MLSigma = sqrt(MLSigma/MLSigmaCount);


		printf("toa %i sig %g \n", t, MLSigma);

		int nTOA = t;


		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **AllHermiteBasis =  new double*[Nbins];
		double **Hermitepoly =  new double*[Nbins];

		for(int i =0;i<Nbins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];for(int j =0;j<maxshapecoeff;j++){AllHermiteBasis[i][j]=0;}}
		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}

	

		long double binpos = ModelBats[nTOA];

	//	if(((MNStruct *)globalcontext)->incRED==5){
			long double rednoiseshift = (long double) SignalVec[t];
			binpos+=rednoiseshift/SECDAY;
			//printf("Red noise shift %i %Lg \n", t, rednoiseshift);
	//	}

		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];


	    	double flatnoise = EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]]*profileamps[t];


		for(int j =0; j < Nbins; j++){

			AllD[bincount+j] =  (double)((MNStruct *)globalcontext)->ProfileData[t][j][1];

			long double timediff = 0;
			long double bintime = ProfileBats[t][j];
			if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
			}
			else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
			}
			else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
			}

			timediff=timediff*SECDAY;


			Betatimes[j]=timediff/beta;
			TNothpl(maxshapecoeff,Betatimes[j],AllHermiteBasis[j]);

			//printf("%i %i %g \n", t, j, AllD[bincount+j]);
			for(int k =0; k < maxshapecoeff; k++){
				//printf("%i %i %i %i %i\n", k, stocprofcount+k, bincount+j, totalcoeff, totalbins);
				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));
				AllHermiteBasis[j][k]=AllHermiteBasis[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

				if(k < numcoeff){ 
					AllProfiles[k][bincount+j] = AllHermiteBasis[j][k]*profileamps[t];
				}

				if(k < numshapestoccoeff){ 
					AllProfiles[stocprofcount+k][bincount+j] = AllHermiteBasis[j][k]*profileamps[t];
				}
			
			}

			AllProfiles[numcoeff+t][bincount+j] = 1;


			AllNoise[bincount+j] = MLSigma*MLSigma;
			if(j < 500 || j > 1500){ 
				AllNoise[bincount+j] += flatnoise*flatnoise;
			}



	   	 }

		stocprofcount += numshapestoccoeff;
		bincount += Nbins;

		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] AllHermiteBasis[j];
		}
		delete[] Hermitepoly;
		delete[] AllHermiteBasis;

	
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Do the Linear Algebra///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	int SVDSize = numcoeff+((MNStruct *)globalcontext)->pulse->nobs;
	double **M = new double*[totalbins];
	for(int j =0; j < totalbins; j++){
	 	M[j] = new double[SVDSize];
		for(int k =0; k < SVDSize; k++){
			M[j][k] = AllProfiles[k][j];
		}
	}
	 

	double* S = new double[SVDSize];
	double** U = new double*[totalbins];
	for(int k=0; k < totalbins; k++){
		U[k] = new double[totalbins];
	}
	double** VT = new double*[SVDSize]; 
	for (int k=0; k<SVDSize; k++) VT[k] = new double[SVDSize];

	printf("doing the svd \n"); 
	dgesvd(M,totalbins, SVDSize, S, U, VT);
	printf("done\n");

	for(int j =0; j < totalbins; j++){
		for(int k =0; k < SVDSize; k++){
			AllProfiles[k][j] = U[j][k];
		}
	}

	delete[]S;	

	for (int j = 0; j < SVDSize; j++){
		delete[]VT[j];
	}

	delete[]VT;	


 
	for (int j = 0; j < totalbins; j++){
		delete[] U[j];
		delete[] M[j];
	}

	delete[]U;
	delete[]M;


	for(int j =0; j < totalbins; j++){
		for(int k =0; k < SVDSize; k++){
			AllProfiles[k][j] = U[j][k];
		}
	}


	double detN = 0;
	for(int j =0; j < totalbins; j++){
		detN += log(AllNoise[j]);
		for(int k =0; k < totalcoeff; k++){
			NAllProfiles[k][j] = AllProfiles[k][j]/AllNoise[j];
		}
	}

	 dgemv(NAllProfiles,AllD,dNM,totalcoeff,totalbins,'N');

	double **MNM = new double*[totalcoeff];
	for(int i =0; i < totalcoeff; i++){
		MNM[i] = new double[totalcoeff];
	}

	printf("doing dgemm \n");
	dgemm(NAllProfiles, AllProfiles , MNM, totalcoeff, totalbins,totalcoeff, totalbins, 'N', 'T');
 	printf("done\n");

	stocprofcount = numcoeff+((MNStruct *)globalcontext)->pulse->nobs;
	double StocProfDet = 0;
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
		for(int j = 0; j < numshapestoccoeff; j++){
			StocProfDet += log(StocProfCoeffs[j]);
			MNM[stocprofcount+j][stocprofcount+j] += 1.0/StocProfCoeffs[j];
		}

		stocprofcount += numshapestoccoeff;
	}

	int info=0;
	double Margindet = 0;
	dpotrfInfo(MNM, totalcoeff, Margindet, info);
	dpotrs(MNM, TempdNM, totalcoeff);

	double Marginlike = 0;
        for(int i =0; i < totalcoeff; i++){
                Marginlike += TempdNM[i]*dNM[i];
        }

	lnew = -0.5*(detN + StocProfDet + Margindet - Marginlike);

	if(badshape == 1){
	//	lnew = -pow(10.0, 10.0);
	}

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] powercoeff;
	delete[] freqs;
	delete[] SignalCoeff;
	delete[] SignalVec;
	
	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
		delete[]FMatrix[j];
	}
	delete[]FMatrix;


	lnew += -0.5*(FreqDet + FreqLike) + uniformpriorterm;
	printf("Like: %.10g %g %g %g %g\n", lnew, detN , StocProfDet , Margindet , Marginlike);

	return lnew;

}


*/
void TemplateProfLikeMNWrap(double *Cube, int &ndim, int &npars, double &lnew, void *context){



        for(int p=0;p<ndim;p++){

                Cube[p]=(((MNStruct *)globalcontext)->PriorsArray[p+ndim]-((MNStruct *)globalcontext)->PriorsArray[p])*Cube[p]+((MNStruct *)globalcontext)->PriorsArray[p];
        }


	double *DerivedParams = new double[npars];

	double result = TemplateProfLike(ndim, Cube, npars, DerivedParams, context);


	delete[] DerivedParams;

	lnew = result;

}

double  TemplateProfLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){

	//printf("In like \n");

	double *EFAC;
	double *EQUAD;
	double TemplateFlux=0;
	double FakeRMS = 0;
	double TargetSN=0;
        long double LDparams;
        int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	LDparams=Cube[0]*(((MNStruct *)globalcontext)->LDpriors[0][1]) + (((MNStruct *)globalcontext)->LDpriors[0][0]);
	
	
	pcount=0;
	long double phase = LDparams*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	TemplateFlux=0;
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0];
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileData[i][0][0]+phase;
	 }

	int totalProfCoeff = 0;
	int *numcoeff= new int[((MNStruct *)globalcontext)->numProfComponents];
	//numcoeff[0] = ((MNStruct *)globalcontext)->numshapecoeff;
	//numcoeff[1] = 10;
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		numcoeff[i] =  ((MNStruct *)globalcontext)->numshapecoeff[i];
		totalProfCoeff += numcoeff[i];
	}

	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;

	pcount++;

	long double *CompSeps = new long double[((MNStruct *)globalcontext)->numProfComponents];
	CompSeps[0] = 0;
	for(int i = 1; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		CompSeps[i] = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;
		pcount++;
		//printf("CompSep: %g \n", Cube[pcount-1]);
	}
	

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	double lnew = 0;
	int badshape = 0;

	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
	//	printf("In toa %i \n", t);
		int nTOA = t;

		double detN  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **Hermitepoly =  new double*[Nbins];

		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[totalProfCoeff];for(int j =0;j<totalProfCoeff;j++){Hermitepoly[i][j]=0;}}
	

		long double binpos = ModelBats[nTOA];


		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];


	    	int cpos = 0;
		for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
			for(int j =0; j < Nbins; j++){
				long double timediff = 0;
				long double bintime = ProfileBats[t][j]+CompSeps[i]/SECDAY;
				if(bintime  >= minpos && bintime <= maxpos){
				    timediff = bintime - binpos;
				}
				else if(bintime < minpos){
				    timediff = FoldingPeriodDays+bintime - binpos;
				}
				else if(bintime > maxpos){
				    timediff = bintime - FoldingPeriodDays - binpos;
				}

				timediff=timediff*SECDAY;
			

				Betatimes[j]=(timediff)/beta;
				TNothplMC(numcoeff[i],Betatimes[j],Hermitepoly[j], cpos);

				for(int k =0; k < numcoeff[i]; k++){
					//if(k==0)printf("HP %i %i %g %g \n", i, j, Betatimes[j],Hermitepoly[j][cpos+k]*exp(-0.5*Betatimes[j]*Betatimes[j]));
					double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));//*((MNStruct *)globalcontext)->Factorials[k]);
					Hermitepoly[j][cpos+k]=Hermitepoly[j][cpos+k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

				}
					
			}
			cpos += numcoeff[i];
	   	 }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


		double **M = new double*[Nbins];

		int Msize = totalProfCoeff+1;
		for(int i =0; i < Nbins; i++){
			M[i] = new double[Msize];

			M[i][0] = 1;


			for(int j = 0; j < totalProfCoeff; j++){
				M[i][j+1] = Hermitepoly[i][j];
				//if(j==0)printf("%i %i %g \n", i, j, M[i][j+1]);
			}
		  
		}


		double **MNM = new double*[Msize];
		for(int i =0; i < Msize; i++){
		    MNM[i] = new double[Msize];
		}

		dgemm(M, M , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');

		for(int j = 0; j < Msize; j++){
			MNM[j][j] += pow(10.0,-14);
		}
	
		double minFlux = pow(10.0,100); 
		for(int j = 0; j < Nbins; j++){
			if((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] < minFlux){minFlux = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];}
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];

		double *NDiffVec = new double[Nbins];
		TemplateFlux = 0;
		for(int j = 0; j < Nbins; j++){
			TemplateFlux +=  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - minFlux;
			NDiffVec[j] = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - minFlux;
		}
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);


//		printf("Margindet %g \n", Margindet);
		double *shapevec = new double[Nbins];
		dgemv(M,TempdNM,shapevec,Nbins,Msize,'N');


		TargetSN = 10000.0;
		FakeRMS = TemplateFlux/TargetSN;
		FakeRMS = 1.0/(FakeRMS*FakeRMS);
		    double Chisq=0;
		    for(int j =0; j < Nbins; j++){


			    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - shapevec[j] - minFlux;
			    Chisq += datadiff*datadiff*FakeRMS;
//				printf("%i %.10g %.10g \n", j, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] , shapevec[j]);		

		    }

		profilelike = -0.5*(Chisq);
		lnew += profilelike;

		delete[] shapevec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] M[j];
		}
		delete[] Hermitepoly;
		delete[] M;

		for (int j = 0; j < Msize; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	}
	 
	 
	 
	 

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] numcoeff;
	delete[] CompSeps;

	//printf("End Like: %.10g %g %g\n", lnew, TemplateFlux, FakeRMS);

	return lnew;

}

void  WriteMaxTemplateProf(std::string longname, int &ndim){

	//printf("In like \n");


        double *Cube = new double[ndim];
        int number_of_lines = 0;

        std::ifstream checkfile;
	std::string checkname;
	if(((MNStruct *)globalcontext)->sampler == 0){
	        checkname = longname+"phys_live.points";
	}
        if(((MNStruct *)globalcontext)->sampler == 1){
                checkname = longname+"_phys_live.txt";
        }
	printf("%i %s \n", ((MNStruct *)globalcontext)->sampler, checkname.c_str());
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        checkfile.close();

        std::ifstream summaryfile;
	std::string fname;
	if(((MNStruct *)globalcontext)->sampler == 0){
		fname = longname+"phys_live.points";
	}
	if(((MNStruct *)globalcontext)->sampler == 1){
	        fname = longname+"_phys_live.txt";
	}
        summaryfile.open(fname.c_str());



        printf("Getting ML \n");
        double maxlike = -1.0*pow(10.0,10);
        for(int i=0;i<number_of_lines;i++){

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);

                double like = paramlist[ndim];

                if(like > maxlike){
                        maxlike = like;
                         for(int i = 0; i < ndim; i++){
                                 Cube[i] = paramlist[i];
                         }
                }

        }
        summaryfile.close();


	double *EFAC;
	double *EQUAD;
	double TemplateFlux=0;
	double FakeRMS = 0;
	double TargetSN=0;
        long double LDparams;
        int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	LDparams=Cube[0]*(((MNStruct *)globalcontext)->LDpriors[0][1]) + (((MNStruct *)globalcontext)->LDpriors[0][0]);
	
	
	pcount=0;
	long double phase = LDparams*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	TemplateFlux=0;
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0];
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileData[i][0][0]+phase;
	 }

	int totalProfCoeff = 0;
	int *numcoeff= new int[((MNStruct *)globalcontext)->numProfComponents];

	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		numcoeff[i] =  ((MNStruct *)globalcontext)->numshapecoeff[i];
		totalProfCoeff += numcoeff[i];
	}

	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;

	pcount++;

	long double *CompSeps = new long double[((MNStruct *)globalcontext)->numProfComponents];
	CompSeps[0] = 0;
	for(int i = 1; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		CompSeps[i] = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;
		pcount++;
		//printf("CompSep: %g \n", Cube[pcount-1]);
	}
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	double lnew = 0;
	int badshape = 0;

	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
	//	printf("In toa %i \n", t);
		int nTOA = t;

		double detN  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **Hermitepoly =  new double*[Nbins];

		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[totalProfCoeff];for(int j =0;j<totalProfCoeff;j++){Hermitepoly[i][j]=0;}}
	

		long double binpos = ModelBats[nTOA];


		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];


		int cpos = 0;
		for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
			printf("Sep %.10Lg \n", CompSeps[i]);
			for(int j =0; j < Nbins; j++){
				long double timediff = 0;
				long double bintime = ProfileBats[t][j]+CompSeps[i]/SECDAY;
				if(bintime  >= minpos && bintime <= maxpos){
				    timediff = bintime - binpos;
				}
				else if(bintime < minpos){
				    timediff = FoldingPeriodDays+bintime - binpos;
				}
				else if(bintime > maxpos){
				    timediff = bintime - FoldingPeriodDays - binpos;
				}

				timediff=timediff*SECDAY;
			

				Betatimes[j]=(timediff)/beta;
				TNothplMC(numcoeff[i],Betatimes[j],Hermitepoly[j], cpos);

				for(int k =0; k < numcoeff[i]; k++){
					//if(k==0)printf("HP %i %i %g %g \n", i, j, Betatimes[j],Hermitepoly[j][cpos+k]*exp(-0.5*Betatimes[j]*Betatimes[j]));
					double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));//*((MNStruct *)globalcontext)->Factorials[k]);
					Hermitepoly[j][cpos+k]=Hermitepoly[j][cpos+k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

				}
					
			}
			cpos += numcoeff[i];
	   	 }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


		double **M = new double*[Nbins];

		int Msize = totalProfCoeff+1;
		for(int i =0; i < Nbins; i++){
			M[i] = new double[Msize];

			M[i][0] = 1;


			for(int j = 0; j < totalProfCoeff; j++){
				M[i][j+1] = Hermitepoly[i][j];
				//if(j==0)printf("%i %i %g \n", i, j, M[i][j+1]);
			}
		  
		}


		double **MNM = new double*[Msize];
		for(int i =0; i < Msize; i++){
		    MNM[i] = new double[Msize];
		}

		dgemm(M, M , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');

		for(int j = 0; j < Msize; j++){
			MNM[j][j] += pow(10.0,-14);
		}

		double minFlux = pow(10.0,100); 
		for(int j = 0; j < Nbins; j++){
			if((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] < minFlux){minFlux = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];}
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];

		double *NDiffVec = new double[Nbins];
		TemplateFlux = 0;
		for(int j = 0; j < Nbins; j++){
			TemplateFlux +=  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-minFlux;
			NDiffVec[j] = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-minFlux;
		}
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);


//		printf("Margindet %g \n", Margindet);
		double *shapevec = new double[Nbins];
		dgemv(M,TempdNM,shapevec,Nbins,Msize,'N');


		TargetSN = 10000.0;
		FakeRMS = TemplateFlux/TargetSN;
		FakeRMS = 1.0/(FakeRMS*FakeRMS);
		    double Chisq=0;
		    for(int j =0; j < Nbins; j++){


			    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - shapevec[j]-minFlux;
			    Chisq += datadiff*datadiff*FakeRMS;
				printf("Bin %i Data %.10g Template %.10g Noise %.10g \n", j, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] , shapevec[j], 1.0/sqrt(FakeRMS));		

		    }

		cpos = 0;
		for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
			for(int j = 0; j < numcoeff[i]; j++){
				printf("P%i %.10g \n", i, TempdNM[cpos+j+1]/TempdNM[1]);
			}
			cpos+=numcoeff[i];
		}
                printf("B %.10g \n", Cube[1]);


		profilelike = -0.5*(Chisq);
		lnew += profilelike;

		delete[] shapevec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] M[j];
		}
		delete[] Hermitepoly;
		delete[] M;

		for (int j = 0; j < Msize; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	}
	 
	 
	 
	 

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] numcoeff;
	delete[] CompSeps;

	//printf("End Like: %.10g %g %g\n", lnew, TemplateFlux, FakeRMS);


}

/*
void SubIntStocProfLikeMNWrap(double *Cube, int &ndim, int &npars, double &lnew, void *context){



        for(int p=0;p<ndim;p++){

                Cube[p]=(((MNStruct *)globalcontext)->PriorsArray[p+ndim]-((MNStruct *)globalcontext)->PriorsArray[p])*Cube[p]+((MNStruct *)globalcontext)->PriorsArray[p];
        }


	double *DerivedParams = new double[npars];

	double result = SubIntStocProfLike(ndim, Cube, npars, DerivedParams, context);


	delete[] DerivedParams;

	lnew = result;

}



double  SubIntStocProfLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){


	struct timeval tval_before, tval_after, tval_resultone, tval_resulttwo;


//	gettimeofday(&tval_before, NULL);


	int debug = 0;

	if(debug == 1)printf("In like \n");

	double *EFAC;
	double *EQUAD;
        long double LDparams;
        int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	LDparams=Cube[0]*(((MNStruct *)globalcontext)->LDpriors[0][1]) + (((MNStruct *)globalcontext)->LDpriors[0][0]);
	
	pcount=0;
	long double phase = LDparams*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	
	if(debug == 1)printf("Phase: %g \n", (double)phase);


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	//for(int i = 0; i < 10; i ++){
	//	double pval = i*0.1;
	//	phase = pval*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;

	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase;
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }


	fastformSubIntBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);      

	//for(int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
	//	printf("%i %g %i %.15g %.15g \n", i, pval, j, (double)((MNStruct *)globalcontext)->pulse->obsn[j].batCorr, (double)((MNStruct *)globalcontext)->pulse->obsn[j].residual	);
	//}/


	//}
	//sleep(5);

	if(debug == 1)printf("Formed Residuals \n");

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[0]);}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[p]);}
			pcount++;

		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }


        double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        	SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        	for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
           		SQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
        	}
    	}	  
	  

	double HighFreqStoc1 = 0;
	double HighFreqStoc2 = 0;
	if(((MNStruct *)globalcontext)->incHighFreqStoc == 1){
		HighFreqStoc1 = pow(10.0,Cube[pcount]);
		pcount++;
	}
        if(((MNStruct *)globalcontext)->incHighFreqStoc == 2){
                HighFreqStoc1 = pow(10.0,Cube[pcount]);
                pcount++;
		HighFreqStoc2 = pow(10.0,Cube[pcount]);
                pcount++;
        }


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	      //printf("badcorr? %i %g %g \n", i,  (double)((MNStruct *)globalcontext)->pulse->obsn[i].batCorr, (double)((MNStruct *)globalcontext)->pulse->obsn[i].residual);
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[0].batCorr;
		
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[0].batCorr - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	 }


	int maxshapecoeff = 0;
	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;
	int numshapestoccoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;

	double shapecoeff[numcoeff];
	double StocProfCoeffs[numshapestoccoeff];

	for(int i =0; i < numcoeff; i++){
		shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
	}
	double beta = ((MNStruct *)globalcontext)->MeanProfileBeta*((MNStruct *)globalcontext)->ReferencePeriod;


	for(int i =0; i < numshapestoccoeff; i++){
		StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
		pcount++;
	}

	if(numcoeff+1>=numshapestoccoeff+1){
		maxshapecoeff=numcoeff+1;
	}
	if(numshapestoccoeff+1 > numcoeff+1){
		maxshapecoeff=numshapestoccoeff+1;
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double modelflux=0;
	    for(int j =0; j < numcoeff; j=j+2){
		modelflux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*shapecoeff[j];
	   }

	double lnew = 0;
	int badshape = 0;

	int minoffpulse=750;
	int maxoffpulse=1250;
//        int minoffpulse=200;
//        int maxoffpulse=800;



	int GlobalNBins = (int)((MNStruct *)globalcontext)->ProfileInfo[0][1];


	double **AllHermiteBasis =  new double*[GlobalNBins];
	double **JitterBasis  =  new double*[GlobalNBins];
	double **Hermitepoly =  new double*[GlobalNBins];

	for(int i =0;i<GlobalNBins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];}
	for(int i =0;i<GlobalNBins;i++){Hermitepoly[i]=new double[numcoeff];}
	for(int i =0;i<GlobalNBins;i++){JitterBasis[i]=new double[numcoeff];}


	double **M = new double*[GlobalNBins];
	double **NM = new double*[GlobalNBins];

	int Msize = 2+numshapestoccoeff;
	int Mcounter=0;
	if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
		Msize++;
	}


	for(int i =0; i < GlobalNBins; i++){
		Mcounter=0;
		M[i] = new double[Msize];
		NM[i] = new double[Msize];

	}


	double **MNM = new double*[Msize];
	for(int i =0; i < Msize; i++){
	    MNM[i] = new double[Msize];
	}


//		gettimeofday(&tval_after, NULL);

//		timersub(&tval_after, &tval_before, &tval_resultone);


//		gettimeofday(&tval_before, NULL);
	
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){

	
//		gettimeofday(&tval_before, NULL);

		if(debug == 1)printf("In toa %i \n", t);
		int nTOA = t;

		double detN  = 0;
		double Chisq  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 
		double JitterPrior = 0;	   

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
	

	        double *shapevec  = new double[Nbins];
	        double *Jittervec = new double[Nbins];

		double noisemean=0;
//		for(int j =minoffpulse; j < maxoffpulse; j++){
//			noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
//		}

//		noisemean = noisemean/(maxoffpulse-minoffpulse);

		double MLSigma = 0;
//		int MLSigmaCount = 0;
		double dataflux = 0;
//		for(int j = 0; j < Nbins; j++){
//			if(j>=minoffpulse && j < maxoffpulse){
//				double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean;
//				MLSigma += res*res; MLSigmaCount += 1;
//			}
//			else{
//				if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
//					dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-noisemean)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
//				}
//			}
//		}
//
//		MLSigma = sqrt(MLSigma/MLSigmaCount);
		double maxamp = 0;
		double maxshape=0;
//
//		if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g \n", MLSigma, dataflux, modelflux, maxamp);


		long double binpos = ModelBats[nTOA];

		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];



		int InterpBin = 0;
		double FirstInterpTimeBin = 0;
		int  NumWholeBinInterpOffset = 0;

		if(((MNStruct *)globalcontext)->InterpolateProfile == 1){

		
			long double timediff = 0;
			long double bintime = ProfileBats[t][0];


			if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
			}
			else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
			}
			else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
			}

			timediff=timediff*SECDAY;

			double OneBin = FoldingPeriod/Nbins;
			int NumBinsInTimeDiff = floor(timediff/OneBin + 0.5);
			double WholeBinsInTimeDiff = NumBinsInTimeDiff*FoldingPeriod/Nbins;
			double OneBinTimeDiff = -1*((double)timediff - WholeBinsInTimeDiff);

			double PWrappedTimeDiff = (OneBinTimeDiff - floor(OneBinTimeDiff/OneBin)*OneBin);

			if(debug == 1)printf("Making InterpBin: %g %g %i %g %g %g\n", (double)timediff, OneBin, NumBinsInTimeDiff, WholeBinsInTimeDiff, OneBinTimeDiff, PWrappedTimeDiff);

			InterpBin = floor(PWrappedTimeDiff/((MNStruct *)globalcontext)->InterpolatedTime+0.5);
			if(InterpBin >= ((MNStruct *)globalcontext)->NumToInterpolate)InterpBin -= ((MNStruct *)globalcontext)->NumToInterpolate;

			FirstInterpTimeBin = -1*(InterpBin-1)*((MNStruct *)globalcontext)->InterpolatedTime;

			if(debug == 1)printf("Interp Time Diffs: %g %g %g %g \n", ((MNStruct *)globalcontext)->InterpolatedTime, InterpBin*((MNStruct *)globalcontext)->InterpolatedTime, PWrappedTimeDiff, InterpBin*((MNStruct *)globalcontext)->InterpolatedTime-PWrappedTimeDiff);

			double FirstBinOffset = timediff-FirstInterpTimeBin;
			double dNumWholeBinOffset = FirstBinOffset/(FoldingPeriod/Nbins);
			int  NumWholeBinOffset = 0;

			NumWholeBinInterpOffset = floor(dNumWholeBinOffset+0.5);
	
			if(debug == 1)printf("Interp bin is: %i , First Bin is %g, Offset is %i \n", InterpBin, FirstInterpTimeBin, NumWholeBinInterpOffset);


		}
	   
		if(((MNStruct *)globalcontext)->InterpolateProfile == 0){

 
			for(int j =0; j < Nbins; j++){


				long double timediff = 0;
				long double bintime = ProfileBats[t][j];
					

				if(bintime  >= minpos && bintime <= maxpos){
				    timediff = bintime - binpos;
				}
				else if(bintime < minpos){
				    timediff = FoldingPeriodDays+bintime - binpos;
				}
				else if(bintime > maxpos){
				    timediff = bintime - FoldingPeriodDays - binpos;
				}


				timediff=timediff*SECDAY;


				Betatimes[j]=timediff/beta;
				TNothpl(maxshapecoeff,Betatimes[j],AllHermiteBasis[j]);

				for(int k =0; k < maxshapecoeff; k++){
					double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));
					AllHermiteBasis[j][k]=AllHermiteBasis[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

					if(k<numcoeff){ Hermitepoly[j][k] = AllHermiteBasis[j][k];}


			//		if(((MNStruct *)globalcontext)->InterpolateProfile == 1 && k == 0){	
			//			printf("Interped: %i %.10g %.10g %.10g \n", j, (double)timediff,AllHermiteBasis[j][k], ((MNStruct *)globalcontext)->InterpolatedShapelets[InterpBin][Nj][k]); 
			//		}
				}

				JitterBasis[j][0] = (1.0/sqrt(2.0))*(-1.0*Hermitepoly[j][1]);
				for(int k =1; k < numcoeff; k++){
					JitterBasis[j][k] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1] - sqrt(double(k+1))*AllHermiteBasis[j][k+1]);
				}

			}

			dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');
			dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,numcoeff,'N');


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////Get Noise Level//////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



			maxshape=0;
			for(int j =0; j < Nbins; j++){
				if(shapevec[j] > maxshape){ maxshape = shapevec[j];}
			}
			noisemean=0;
			int numoffpulse = 0;
			for(int j = 0; j < Nbins; j++){
				if(shapevec[j] < maxshape*0.001){
					noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
					numoffpulse += 1;
				}	
			}


			noisemean = noisemean/(numoffpulse);

			MLSigma = 0;
			int MLSigmaCount = 0;
			dataflux = 0;
			for(int j = 0; j < Nbins; j++){
				if(shapevec[j] < maxshape*0.001){
					double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean;
					MLSigma += res*res; MLSigmaCount += 1;
				}
				else{
					if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
						dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-noisemean)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
					}
				}
			}

			MLSigma = sqrt(MLSigma/MLSigmaCount);
			MLSigma = MLSigma*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];
			maxamp = dataflux/modelflux;
			if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g \n", MLSigma, dataflux, modelflux, maxamp);

                        for(int j =0; j < Nbins; j++){

				
				M[j][0] = 1;
				M[j][1] = shapevec[j];

				Mcounter = 2;

				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
					M[j][Mcounter] = Jittervec[j]*dataflux/modelflux/beta;
					Mcounter++;
				}

				for(int k = 0; k < numshapestoccoeff; k++){
				    M[j][Mcounter+k] = AllHermiteBasis[j][k]*dataflux;
				}

			}
		}

		if(((MNStruct *)globalcontext)->InterpolateProfile == 1){


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////Fill Arrays with interpolated state//////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			for(int j =0; j < Nbins; j++){

				double NewIndex = (j + NumWholeBinInterpOffset);
				int Nj = (int)(NewIndex - floor(NewIndex/Nbins)*Nbins);

				M[j][0] = 1;
				M[j][1] = ((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpBin][Nj];

				Mcounter = 2;
				
				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
					M[j][Mcounter] = ((MNStruct *)globalcontext)->InterpolatedJitterProfile[InterpBin][Nj];
					Mcounter++;
				}			

				for(int k = 0; k < numshapestoccoeff; k++){
				  //  M[j][Mcounter+k] = ((MNStruct *)globalcontext)->InterpolatedShapelets[InterpBin][Nj][k];
				}

				shapevec[j] = ((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpBin][Nj];
			}



		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////Get Noise Level//////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			noisemean=0;
			int numoffpulse = 0;
                        maxshape=((MNStruct *)globalcontext)->MaxShapeAmp;

			for(int j = 0; j < Nbins; j++){
				if(shapevec[j] < ((MNStruct *)globalcontext)->MaxShapeAmp*0.001){
					noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
					numoffpulse += 1;
				}	
			}


			noisemean = noisemean/(numoffpulse);

			for(int j =minoffpulse; j < maxoffpulse; j++){
				noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
			}
	                noisemean = noisemean/(maxoffpulse-minoffpulse);

			MLSigma = 0;
			int MLSigmaCount = 0;
			dataflux = 0;
			for(int j = 0; j < Nbins; j++){
				if(shapevec[j] < ((MNStruct *)globalcontext)->MaxShapeAmp*0.001){
					double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean;
					MLSigma += res*res; MLSigmaCount += 1;
				}
			//	else{
			//		if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
			//			dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-noisemean)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
			//		}
			//	}
			}


		MLSigma = 0;
		MLSigmaCount = 0;
		dataflux = 0;
		for(int j = 0; j < Nbins; j++){
			if(j>=minoffpulse && j < maxoffpulse){
				double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] -noisemean;
				MLSigma += res*res; MLSigmaCount += 1;
			}
			else{
				if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
					dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-noisemean)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
				}
			}
		}

			MLSigma = sqrt(MLSigma/MLSigmaCount);
			MLSigma = MLSigma*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

			dataflux = 0;
			for(int j = 0; j < Nbins; j++){
//				printf("shapevec %i %g %g %g \n", j, shapevec[j], ((MNStruct *)globalcontext)->MaxShapeAmp, ((MNStruct *)globalcontext)->MaxShapeAmp*0.001);
				if(shapevec[j] < ((MNStruct *)globalcontext)->MaxShapeAmp*0.001){
//					if(t==0){printf("in %i \n", j);}
				}
				else{
					if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
						dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-noisemean)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
					}
				}
			}

			maxamp = dataflux/modelflux;
			//if(t==0){printf("prof %i MLSigmaCount %i, noise %g, dataflux %g, modelflux %g, maxamp %g noisemean %g\n", t, MLSigmaCount, MLSigma, dataflux, modelflux, maxamp, noisemean);}
			if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g \n", MLSigma, dataflux, modelflux, maxamp);


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////Rescale Basis Vectors////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			for(int j =0; j < Nbins; j++){

				Mcounter = 2;
				
				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
					M[j][Mcounter] = M[j][Mcounter]*dataflux/modelflux/beta;
					Mcounter++;
				}			

				for(int k = 0; k < numshapestoccoeff; k++){
				    M[j][Mcounter+k] = M[j][Mcounter+k]*dataflux;
				}
			}
	    }

	
	    double *NDiffVec = new double[Nbins];

//	    double maxshape=0;
//	    for(int j =0; j < Nbins; j++){
// 	        if(shapevec[j] > maxshape){ maxshape = shapevec[j];}
//	    }


	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


		      
		Chisq = 0;
		detN = 0;

	
 
		double highfreqnoise = HighFreqStoc1;
		double flatnoise = HighFreqStoc2;


		for(int i =0; i < Nbins; i++){
		  	Mcounter=2;
			double noise = MLSigma*MLSigma +  pow(highfreqnoise*maxamp*shapevec[i],2);

			if(shapevec[i] > maxshape*0.001){
				noise +=  pow(flatnoise*maxamp*maxshape,2);
			}
			detN += log(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1];

			NDiffVec[i] = datadiff/(noise);
		       

			Chisq += datadiff*datadiff/(noise);
	


			for(int j = 0; j < Msize; j++){
				NM[i][j] = M[i][j]/(noise);
			}


		}
	
	
	//	gettimeofday(&tval_after, NULL);

	//	timersub(&tval_after, &tval_before, &tval_resultone);


	//	gettimeofday(&tval_before, NULL);
			
		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');


		Mcounter=2;
		double JitterDet = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
			profnoise = profnoise*profnoise;
		    	JitterDet = log(profnoise);
		    	MNM[2][2] += 1.0/profnoise;
			Mcounter++;
		}


		double StocProfDet = 0;
		for(int j = 0; j < numshapestoccoeff; j++){
			StocProfDet += log(StocProfCoeffs[j]);
			MNM[Mcounter+j][Mcounter+j] +=  1.0/StocProfCoeffs[j];
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);
			    
		logMargindet = Margindet;

		Marginlike = 0;
		for(int i =0; i < Msize; i++){
			Marginlike += TempdNM[i]*dNM[i];
		}

		profilelike = -0.5*(detN + Chisq + logMargindet + StocProfDet + JitterDet - Marginlike);
		lnew += profilelike;
		if(debug == 1)printf("Like: %i %.15g %.15g %.15g %.15g %.15g %.15g %.15g\n", nTOA,lnew, detN, Chisq, logMargindet, Marginlike, StocProfDet , JitterDet);

		delete[] shapevec;
		delete[] Jittervec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] TempdNM;
		delete[] Betatimes;


	
//        	gettimeofday(&tval_after, NULL);

  //      	timersub(&tval_after, &tval_before, &tval_resulttwo);
    //    	printf("Time elapsed: %ld.%06ld %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec, (long int)tval_resulttwo.tv_sec, (long int)tval_resulttwo.tv_usec);
	
	}
	 
	 
	for (int j = 0; j < GlobalNBins; j++){
	    delete[] Hermitepoly[j];
	    delete[] JitterBasis[j];
	    delete[] AllHermiteBasis[j];
	    delete[] M[j];
	    delete[] NM[j];
	}
	delete[] Hermitepoly;
	delete[] AllHermiteBasis;
	delete[] JitterBasis;
	delete[] M;
	delete[] NM;

	for (int j = 0; j < Msize; j++){
	    delete[] MNM[j];
	}
	delete[] MNM;
	 

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;


	lnew += uniformpriorterm;
	if(debug == 1)printf("End Like: %.10g \n", lnew);
	//printf("End Like: %.10g \n", lnew);
	//}


       	//gettimeofday(&tval_after, NULL);

       	//timersub(&tval_after, &tval_before, &tval_resulttwo);
       	//printf("Time elapsed: %ld.%06ld %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec, (long int)tval_resulttwo.tv_sec, (long int)tval_resulttwo.tv_usec);
	
	return lnew;
	//sleep(5);
	//return bluff;
	//sleep(5);

}

void  WriteSubIntStocProfLike(std::string longname, int &ndim){



        double *Cube = new double[ndim];
        int number_of_lines = 0;

        std::ifstream checkfile;
	std::string checkname;
	if(((MNStruct *)globalcontext)->sampler == 0){
	        checkname = longname+"phys_live.points";
	}
        if(((MNStruct *)globalcontext)->sampler == 1){
                checkname = longname+"_phys_live.txt";
        }
	//printf("%i %s \n", ((MNStruct *)globalcontext)->sampler, checkname.c_str());
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        checkfile.close();

        std::ifstream summaryfile;
	std::string fname;
	if(((MNStruct *)globalcontext)->sampler == 0){
		fname = longname+"phys_live.points";
	}
	if(((MNStruct *)globalcontext)->sampler == 1){
	        fname = longname+"_phys_live.txt";
	}

        summaryfile.open(fname.c_str());



        printf("Getting ML \n");
        double maxlike = -1.0*pow(10.0,10);
        for(int i=0;i<number_of_lines;i++){

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);
                double like = paramlist[ndim];

                if(like > maxlike){
                        maxlike = like;
                         for(int i = 0; i < ndim; i++){
                                 Cube[i] = paramlist[i];
                         }
                }

        }
        summaryfile.close();
	printf("ML val: %.10g \n", maxlike);

	int debug = 0;

	if(debug == 1)printf("In like \n");

	double *EFAC;
	double *EQUAD;
	long double LDparams;
	int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	LDparams=Cube[0]*(((MNStruct *)globalcontext)->LDpriors[0][1]) + (((MNStruct *)globalcontext)->LDpriors[0][0]);
	
	pcount=0;
	long double phase = LDparams*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	
	if(debug == 1)printf("Phase: %g \n", (double)phase);


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase;
	}


	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
	}


	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       


	if(debug == 1)printf("Formed Residuals \n");

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[0]);}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[p]);}
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
		}
	}


	double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
		}
	}	  
	  
	double HighFreqStoc1 = 0;
	double HighFreqStoc2 = 0;
	if(((MNStruct *)globalcontext)->incHighFreqStoc == 1){
		HighFreqStoc1 = pow(10.0,Cube[pcount]);
		pcount++;
	}
        if(((MNStruct *)globalcontext)->incHighFreqStoc == 2){
                HighFreqStoc1 = pow(10.0,Cube[pcount]);
                pcount++;
		HighFreqStoc2 = pow(10.0,Cube[pcount]);
                pcount++;
        }
	//printf("End of White\n");


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	      //printf("badcorr? %i %g %g \n", i,  (double)((MNStruct *)globalcontext)->pulse->obsn[i].batCorr, (double)((MNStruct *)globalcontext)->pulse->obsn[i].residual);
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[0].batCorr;
		
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[0].batCorr - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	 }


	int maxshapecoeff = 0;
	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;
	int numshapestoccoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;

	double shapecoeff[numcoeff];
	double StocProfCoeffs[numshapestoccoeff];

	for(int i =0; i < numcoeff; i++){
		shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
	}
	double beta = ((MNStruct *)globalcontext)->MeanProfileBeta*((MNStruct *)globalcontext)->ReferencePeriod;


	for(int i =0; i < numshapestoccoeff; i++){
		StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
		pcount++;
	}

	if(numcoeff+1>=numshapestoccoeff+1){
		maxshapecoeff=numcoeff+1;
	}
	if(numshapestoccoeff+1 > numcoeff+1){
		maxshapecoeff=numshapestoccoeff+1;
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double modelflux=0;
	    for(int j =0; j < numcoeff; j=j+2){
		modelflux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*shapecoeff[j];
	   }

	double lnew = 0;
	int badshape = 0;

//	int minoffpulse=750;
//	int maxoffpulse=1250;
       // int minoffpulse=200;
       // int maxoffpulse=800;
	
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
		if(debug == 1)printf("In toa %i \n", t);
		int nTOA = t;

		double detN  = 0;
		double Chisq  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 
		double JitterPrior = 0;	   

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **AllHermiteBasis =  new double*[Nbins];
		double **JitterBasis  =  new double*[Nbins];
		double **Hermitepoly =  new double*[Nbins];

		for(int i =0;i<Nbins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];for(int j =0;j<maxshapecoeff;j++){AllHermiteBasis[i][j]=0;}}
		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
		for(int i =0;i<Nbins;i++){JitterBasis[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){JitterBasis[i][j]=0;}}

	
	

		long double binpos = ModelBats[nTOA];


		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];


	    
		for(int j =0; j < Nbins; j++){
			long double timediff = 0;
			long double bintime = ProfileBats[t][j];
			if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
			}
			else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
			}
			else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
			}

			timediff=timediff*SECDAY;

			Betatimes[j]=timediff/beta;
			TNothpl(maxshapecoeff,Betatimes[j],AllHermiteBasis[j]);

			for(int k =0; k < maxshapecoeff; k++){
				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));
				AllHermiteBasis[j][k]=AllHermiteBasis[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

				if(k<numcoeff){ Hermitepoly[j][k] = AllHermiteBasis[j][k];}
			
			}

			JitterBasis[j][0] = (1.0/sqrt(2.0))*(-1.0*AllHermiteBasis[j][1]);
			for(int k =1; k < numcoeff; k++){
				JitterBasis[j][k] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1] - sqrt(double(k+1))*AllHermiteBasis[j][k+1]);
			}
			
	
	    }


		double *shapevec  = new double[Nbins];
		double *Jittervec = new double[Nbins];

		double OverallProfileAmp = shapecoeff[0];

		shapecoeff[0] = 1;

		dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');
		dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,numcoeff,'N');

		double *NDiffVec = new double[Nbins];

		double maxshape=0;


		for(int j =0; j < Nbins; j++){

		    if(shapevec[j] > maxshape){ maxshape = shapevec[j];}

		}




			double noisemean=0;
			int numoffpulse = 0;
			for(int j = 0; j < Nbins; j++){
				if(shapevec[j] < maxshape*0.001){
					noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
					numoffpulse += 1;
				}	
			}


			noisemean = noisemean/(numoffpulse);

			double MLSigma = 0;
			int MLSigmaCount = 0;
			double dataflux = 0;
			for(int j = 0; j < Nbins; j++){
				if(shapevec[j] < maxshape*0.001){
					double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean;
					MLSigma += res*res; MLSigmaCount += 1;
				}
				else{
					if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
						dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-noisemean)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
					}
				}
			}

			MLSigma = sqrt(MLSigma/MLSigmaCount);
			MLSigma = MLSigma*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];
			double maxamp = dataflux/modelflux;
			if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g \n", MLSigma, dataflux, modelflux, maxamp);


	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


		double **M = new double*[Nbins];
		double **NM = new double*[Nbins];

		int Msize = 2+numshapestoccoeff;
		int Mcounter=0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			Msize++;
		}


		for(int i =0; i < Nbins; i++){
			Mcounter=0;
			M[i] = new double[Msize];
			NM[i] = new double[Msize];

			M[i][0] = 1;
			M[i][1] = shapevec[i];

			Mcounter = 2;

			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
				M[i][Mcounter] = Jittervec[i];
				Mcounter++;
			}			

			for(int j = 0; j < numshapestoccoeff; j++){
			    M[i][Mcounter+j] = AllHermiteBasis[i][j];
			}
		}


		double **MNM = new double*[Msize];
		for(int i =0; i < Msize; i++){
		    MNM[i] = new double[Msize];
		}

		      
		Chisq = 0;
		detN = 0;

	
 		double highfreqnoise = HighFreqStoc1;
		double flatnoise = HighFreqStoc2;
		
		double *profilenoise = new double[Nbins];

		for(int i =0; i < Nbins; i++){
			Mcounter=2;
			double noise = MLSigma*MLSigma +  pow(highfreqnoise*maxamp*shapevec[i],2);
			if(shapevec[i] > maxshape*0.001){
				noise +=  pow(flatnoise*maxamp*maxshape,2);
			}
			detN += log(noise);
			profilenoise[i] = sqrt(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1];

			NDiffVec[i] = datadiff/(noise);
		       

			Chisq += datadiff*datadiff/(noise);
	
			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
				M[i][2] = M[i][2]*dataflux/modelflux/beta;
				Mcounter++;
			}
			
			for(int j = 0; j < numshapestoccoeff; j++){
				M[i][Mcounter+j] = M[i][Mcounter+j]*dataflux;

			}


			for(int j = 0; j < Msize; j++){
				NM[i][j] = M[i][j]/(noise);
			}


		}
		
			
		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');


		Mcounter=2;
		double JitterDet = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
			profnoise = profnoise*profnoise;
			JitterDet = log(profnoise);
			MNM[2][2] += 1.0/profnoise;
			Mcounter++;
		}


		double StocProfDet = 0;
		for(int j = 0; j < numshapestoccoeff; j++){
			StocProfDet += log(StocProfCoeffs[j]);
			MNM[Mcounter+j][Mcounter+j] +=  1.0/StocProfCoeffs[j];
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);
			    
		logMargindet = Margindet;

		Marginlike = 0;
		for(int i =0; i < Msize; i++){
			Marginlike += TempdNM[i]*dNM[i];
		}


		double finaloffset = TempdNM[0];
		double finalamp = TempdNM[1];
		double finalJitter = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0)finalJitter = TempdNM[2];

		double *StocVec = new double[Nbins];
		
		for(int i =0; i < Nbins; i++){
			if(((MNStruct *)globalcontext)->numFitEQUAD > 0)Jittervec[i] = M[i][2];
			if(((MNStruct *)globalcontext)->numFitEQUAD == 0)Jittervec[i] = 0;
		}

		dgemv(M,TempdNM,shapevec,Nbins,Msize,'N');

		

		TempdNM[0] = 0;
		TempdNM[1] = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0)TempdNM[2] = 0;
		

		dgemv(M,TempdNM,StocVec,Nbins,Msize,'N');

		std::ofstream profilefile;
		char toanum[15];
		sprintf(toanum, "%d", nTOA);
		std::string ProfileName =  toanum;
		std::string dname = longname+ProfileName+"-Profile.txt";
	
		profilefile.open(dname.c_str());
		double MLChisq = 0;
		for(int i =0; i < Nbins; i++){
			MLChisq += pow(((double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1] - shapevec[i])/profilenoise[i], 2);
			profilefile << i << " " << std::setprecision(10) << (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1] << " " << shapevec[i] << " " << profilenoise[i] << " " << finaloffset + finalamp*M[i][1] << " " << finalJitter*Jittervec[i] << " " << StocVec[i] << "\n";

		}
	    	profilefile.close();
		delete[] profilenoise;
		delete[] StocVec;
		
		profilelike = -0.5*(detN + Chisq + logMargindet + StocProfDet + JitterDet - Marginlike);
		lnew += profilelike;

		printf("Profile chisq and like: %g %g \n", MLChisq, profilelike);
//		printf("Like: %i %.15g %.15g %.15g %.15g %.15g %.15g %.15g\n", nTOA,lnew, detN, Chisq, logMargindet, Marginlike, StocProfDet , JitterDet);

		delete[] shapevec;
		delete[] Jittervec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] JitterBasis[j];
		    delete[] AllHermiteBasis[j];
		    delete[] M[j];
		    delete[] NM[j];
		}
		delete[] Hermitepoly;
		delete[] AllHermiteBasis;
		delete[] JitterBasis;
		delete[] M;
		delete[] NM;

		for (int j = 0; j < Msize; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	}
	 
	 
	 
	 

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;


	lnew += uniformpriorterm;
	printf("End Like: %.10g \n", lnew);


}
*/

/*
void PreComputeShapelets(double **StoredShapelets, double **StoredJitterShapelets, double ** StoredWidthShapelets, double **InterpolatedMeanProfile, double **InterpolatedJitterProfile, double **InterpolatedWidthProfile, long double finalInterpTime, int numtointerpolate, double *MeanBeta, double &MaxShapeAmp){


	printf("In function %Lg %i %g \n", finalInterpTime, numtointerpolate, MeanBeta[0]);

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	int maxshapecoeff = 0;
	int totshapecoeff = ((MNStruct *)globalcontext)->totshapecoeff;

	int *numcoeff= new int[((MNStruct *)globalcontext)->numProfComponents];

	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		numcoeff[i] =  ((MNStruct *)globalcontext)->numshapecoeff[i];
	}

	int totalshapestoccoeff = ((MNStruct *)globalcontext)->totalshapestoccoeff;
	int *numProfileStocCoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;

	int *numEvoCoeff = ((MNStruct *)globalcontext)->numEvoCoeff;
	int totalEvoCoeff = ((MNStruct *)globalcontext)->totalEvoCoeff;

	int *numProfileFitCoeff = ((MNStruct *)globalcontext)->numProfileFitCoeff;	
	int totalProfileFitCoeff = ((MNStruct *)globalcontext)->totalProfileFitCoeff;

        int *numShapeToSave = new int[((MNStruct *)globalcontext)->numProfComponents];
        for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
                numShapeToSave[i] = numcoeff[i];
		//if(numEvoCoeff[i] >  numShapeToSave[i]){numShapeToSave[i] = numEvoCoeff[i];}
		//if(numProfileFitCoeff[i] >  numShapeToSave[i]){numShapeToSave[i] = numProfileFitCoeff[i];}
		printf("saving %i %i \n", i, numShapeToSave[i]);
        }
	int totShapeToSave = 0;
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		totShapeToSave += numShapeToSave[i];
	}


	double shapecoeff[totshapecoeff];

	for(int i =0; i < totshapecoeff; i++){
		shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
		printf("ShapeCoeff: %i %g \n", i, shapecoeff[i]);
	}


	double *betas = new double[((MNStruct *)globalcontext)->numProfComponents]();
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		betas[i] = MeanBeta[i]*((MNStruct *)globalcontext)->ReferencePeriod;
	}


	maxshapecoeff = totshapecoeff +  ((MNStruct *)globalcontext)->numProfComponents;

	long double *CompSeps = new long double[((MNStruct *)globalcontext)->numProfComponents];
	CompSeps[0] = 0;
	for(int i = 1; i < ((MNStruct *)globalcontext)->numProfComponents; i++){


		CompSeps[i] = ((MNStruct *)globalcontext)->ProfCompSeps[i]*((MNStruct *)globalcontext)->ReferencePeriod;


	}

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	int Nbins = ((MNStruct *)globalcontext)->LargestNBins;
	long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;
	long double FoldingPeriodDays = ReferencePeriod/SECDAY;
	double maxshapeamp = 0;

	int JitterProfComp = 0;
		
	if(((MNStruct *)globalcontext)->JitterProfComp > 0){
		JitterProfComp = ((MNStruct *)globalcontext)->JitterProfComp-1;
	}

	printf("Computing %i Interpolated Profiles \n", numtointerpolate);

	for(int t = 0; t < numtointerpolate; t++){



		double **WidthBasis  =  new double*[Nbins];
		double **JitterBasis  =  new double*[Nbins];
		double **Hermitepoly =  new double*[Nbins];
		double **AllHermiteBasis = new double*[Nbins];

                for(int i =0;i<Nbins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];for(int j =0;j<maxshapecoeff;j++){AllHermiteBasis[i][j]=0;}}
		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[totshapecoeff];for(int j =0;j<totshapecoeff;j++){Hermitepoly[i][j]=0;}}
		for(int i =0;i<Nbins;i++){JitterBasis[i]=new double[totshapecoeff];for(int j =0;j<totshapecoeff;j++){JitterBasis[i][j]=0;}}
		for(int i =0;i<Nbins;i++){WidthBasis[i]=new double[totshapecoeff];for(int j =0;j<totshapecoeff;j++){WidthBasis[i][j]=0;}}

		long double interpStep = finalInterpTime;

	    
		int cpos = 0;
		int jpos=0;
		int spos = 0;
		for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){

			long double binpos = t*interpStep/SECDAY+CompSeps[i]/SECDAY;

	//		printf("Interp step %i %.10Lg %.10Lg \n", t, ReferencePeriod/Nbins, binpos);
		
			long double minpos = binpos - FoldingPeriodDays/2;
			if(minpos < 0 ) minpos = 0;
			long double maxpos = binpos + FoldingPeriodDays/2;
			if(maxpos > FoldingPeriodDays)maxpos = FoldingPeriodDays;


	
			//double oneAndThree = 0;
	
			for(int j =0; j < Nbins; j++){
				long double timediff = 0;
				long double bintime = j*FoldingPeriodDays/Nbins;
				if(bintime  >= minpos && bintime <= maxpos){
				    timediff = bintime - binpos;
				}
				else if(bintime < minpos){
				    timediff = FoldingPeriodDays+bintime - binpos;
				}
				else if(bintime > maxpos){
				    timediff = bintime - FoldingPeriodDays - binpos;
				}

				timediff=timediff*SECDAY;
				double Betatime = timediff/betas[i];
				TNothplMC(numcoeff[i]+1,Betatime,AllHermiteBasis[j], jpos);
			
//				if(t==0){printf("BinTime: %i %i %g \n", i, j, Betatime);}

				for(int k =0; k < numcoeff[i]+1; k++){
					double Bconst=(1.0/sqrt(betas[i]))/sqrt(pow(2.0,k)*sqrt(M_PI));
					AllHermiteBasis[j][k+jpos] = AllHermiteBasis[j][k+jpos]*Bconst*exp(-0.5*Betatime*Betatime);
					if(t == 1687 && k == 0)printf("%i %i %i %g %g %g\n", i, j, k, Betatime, AllHermiteBasis[j][k+jpos], AllHermiteBasis[j][k+jpos]);

					if(k<numcoeff[i]){ Hermitepoly[j][k+cpos] = AllHermiteBasis[j][k+jpos]; }
	 			}


 
				//printf("Jitter Prof Comp is: %i \n",((MNStruct *)globalcontext)->JitterProfComp );
				if(((MNStruct *)globalcontext)->JitterProfComp == 0){
					JitterBasis[j][0+cpos] = (1.0/sqrt(2.0))*(-1.0*AllHermiteBasis[j][1+jpos])/betas[i];
					for(int k =1; k < numcoeff[i]; k++){
						JitterBasis[j][k+cpos] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1+jpos] - sqrt(double(k+1))*AllHermiteBasis[j][k+1+jpos])/betas[i];
					}


					WidthBasis[j][0+cpos] = (Betatime*Betatime - 0.5)*AllHermiteBasis[j][0+jpos];

					for(int k =1; k < numcoeff[i]; k++){
						WidthBasis[j][k+cpos] = ((Betatime*Betatime - 0.5)*AllHermiteBasis[j][k+jpos] - sqrt(double(2*k))*Betatime*AllHermiteBasis[j][k-1+jpos]);
					}




					
				}
				else{
					if(i == ((MNStruct *)globalcontext)->JitterProfComp - 1){
						JitterBasis[j][0+cpos] = (1.0/sqrt(2.0))*(-1.0*AllHermiteBasis[j][1+jpos])/betas[i];
						for(int k =1; k < numcoeff[i]; k++){
							JitterBasis[j][k+cpos] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1+jpos] - sqrt(double(k+1))*AllHermiteBasis[j][k+1+jpos])/betas[i];
						}
						WidthBasis[j][0+cpos] = (Betatime*Betatime - 0.5)*AllHermiteBasis[j][0+jpos];
						for(int k =1; k < numcoeff[i]; k++){
							WidthBasis[j][k+cpos] = ((Betatime*Betatime - 0.5)*AllHermiteBasis[j][k+jpos] - sqrt(double(2*k))*Betatime*AllHermiteBasis[j][k-1+jpos]);
						}

					}
				}
				int refbasis = 0;
//				if(i==0){refbasis = 1;}
				if(i==0){refbasis = 0;}
				for(int k = 0; k < numShapeToSave[i]; k++){
					//if(t==0){printf("Save: %i %i %i %g \n", i, k+refbasis, j, spos, AllHermiteBasis[j][k+refbasis+jpos]);}
					
					StoredShapelets[t][j+(k+spos)*Nbins] = AllHermiteBasis[j][k+refbasis+jpos];
					StoredJitterShapelets[t][j+(k+spos)*Nbins] = JitterBasis[j][k+refbasis+cpos];


					if(((MNStruct *)globalcontext)->FitLinearProfileWidth == 1 || ((MNStruct *)globalcontext)->incWidthEvoTime > 0){
						StoredWidthShapelets[t][j+(k+spos)*Nbins] = WidthBasis[j][k+refbasis+cpos];
					}

				
				}	
		    	}

			//if(oneAndThree > pow(10.0, -10)){printf("One and Three: %i %g \n", t, oneAndThree);}
 
			spos += numShapeToSave[i];
			cpos += numcoeff[i];
			jpos += numcoeff[i]+1;
		}

		double *shapevec  = new double[Nbins];
		double *Jittervec = new double[Nbins];
		double *Widthvec = new double[Nbins];

		dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,totshapecoeff,'N');
		dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,totshapecoeff,'N');
		dgemv(WidthBasis,shapecoeff,Widthvec,Nbins,totshapecoeff,'N');

		double checkmax = pow(10.0, 100);
		for(int j =0; j < Nbins; j++){
			if(shapevec[j] < checkmax){ checkmax = shapevec[j]; }
		}



			

		for(int j =0; j < Nbins; j++){
			InterpolatedMeanProfile[t][j] = shapevec[j];//-checkmax;
			InterpolatedJitterProfile[t][j] = Jittervec[j];
			InterpolatedWidthProfile[t][j] = Widthvec[j];
			if(fabs(shapevec[j]-checkmax) > maxshapeamp){ maxshapeamp = fabs(shapevec[j]-checkmax); }
//			if(t==0){printf("mean %i %g \n", j, InterpolatedMeanProfile[t][j]);}
		}	


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////If we are saving shifts/widths for individual comps, need to do this again :'( /////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		int widthsavecount = 0;
		int possavecount = 0;
		cpos = 0;
		jpos=0;
		spos = 0;



		for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){




			if(((MNStruct *)globalcontext)->FitCompWidths[i] > 0 || ((MNStruct *)globalcontext)->FitCompPos[i] > 0){


				for(int m =0;m<Nbins;m++){for(int n =0;n<maxshapecoeff;n++){AllHermiteBasis[m][n]=0;}}
				for(int m =0;m<Nbins;m++){for(int n =0;n<totshapecoeff;n++){JitterBasis[m][n]=0;}}
				for(int m =0;m<Nbins;m++){for(int n =0;n<totshapecoeff;n++){WidthBasis[m][n]=0;}}


				if(t==0){printf("Getting width for comp %i \n", i);}
				long double binpos = t*interpStep/SECDAY+CompSeps[i]/SECDAY;

				long double minpos = binpos - FoldingPeriodDays/2;
				if(minpos < 0 ) minpos= 0;
				long double maxpos = binpos + FoldingPeriodDays/2;
				if(maxpos > FoldingPeriodDays)maxpos = FoldingPeriodDays;


	
				for(int j =0; j < Nbins; j++){
					long double timediff = 0;
					long double bintime = j*FoldingPeriodDays/Nbins;
					if(bintime  >= minpos && bintime <= maxpos){
					    timediff = bintime - binpos;
					}
					else if(bintime < minpos){
					    timediff = FoldingPeriodDays+bintime - binpos;
					}
					else if(bintime > maxpos){
					    timediff = bintime - FoldingPeriodDays - binpos;
					}

					timediff=timediff*SECDAY;
					double Betatime = timediff/betas[i];
					TNothplMC(numcoeff[i]+1,Betatime,AllHermiteBasis[j], jpos);

			
				

					for(int k =0; k < numcoeff[i]+1; k++){
						double Bconst=(1.0/sqrt(betas[i]))/sqrt(pow(2.0,k)*sqrt(M_PI));
						AllHermiteBasis[j][k+jpos] = AllHermiteBasis[j][k+jpos]*Bconst*exp(-0.5*Betatime*Betatime);
		 			}


	 

					JitterBasis[j][0+cpos] = (1.0/sqrt(2.0))*(-1.0*AllHermiteBasis[j][1+jpos])/betas[i];
					for(int k =1; k < numcoeff[i]; k++){
						JitterBasis[j][k+cpos] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1+jpos] - sqrt(double(k+1))*AllHermiteBasis[j][k+1+jpos])/betas[i];
					}


					WidthBasis[j][0+cpos] = (Betatime*Betatime - 0.5)*AllHermiteBasis[j][0+jpos];

					for(int k =1; k < numcoeff[i]; k++){
						WidthBasis[j][k+cpos] = ((Betatime*Betatime - 0.5)*AllHermiteBasis[j][k+jpos] - sqrt(double(2*k))*Betatime*AllHermiteBasis[j][k-1+jpos]);
					}


	
			    	}


				dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,totshapecoeff,'N');
				dgemv(WidthBasis,shapecoeff,Widthvec,Nbins,totshapecoeff,'N');
		
				if(((MNStruct *)globalcontext)->FitCompWidths[i] > 0){
					for(int j =0; j < Nbins; j++){
						InterpolatedWidthProfile[t][j+Nbins*(widthsavecount+1)] = Widthvec[j];
					}

					widthsavecount++;
				}

				if(((MNStruct *)globalcontext)->FitCompPos[i] > 0){
					for(int j =0; j < Nbins; j++){
						InterpolatedWidthProfile[t][j+Nbins*(((MNStruct *)globalcontext)->NumCompswithWidth+possavecount+1)] = Jittervec[j];
					}

					possavecount++;
				}				
	 
				spos += numShapeToSave[i];
				cpos += numcoeff[i];
				jpos += numcoeff[i]+1;
			}

		}


	



		delete[] shapevec;
		delete[] Jittervec;
		delete[] Widthvec;


		for (int j = 0; j < Nbins; j++){
		    delete[] AllHermiteBasis[j]; 
		    delete[] Hermitepoly[j];
		    delete[] JitterBasis[j];
		    delete[] WidthBasis[j];
		}
		delete[] AllHermiteBasis;
		delete[] Hermitepoly;
		delete[] JitterBasis;	
		delete[] WidthBasis;

	}
	printf("Finished Computing Interpolated Profiles, max amp is %g \n", maxshapeamp);
	MaxShapeAmp = maxshapeamp;

	delete[] numcoeff;
	delete[] CompSeps;	
	delete[] betas;
}
*/

/*

void FFTProfileData(double **StoredFourierShapelets, double **StoredFourierJitterShapelets, int &NFBasis, int FindThreshold, double **FourierProfileData, void *context){


	double *betas = new double[((MNStruct *)context)->numProfComponents]();
	for(int i = 0; i < ((MNStruct *)context)->numProfComponents; i++){
		betas[i] = ((MNStruct *)context)->MeanProfileBeta[i];
	}

	int Nbins = (int)((MNStruct *)context)->LargestNBins;
	int NFreqs = Nbins/2;

	double *BetaFreqs = new double[NFreqs];

	double **HermiteFMatrix =  new double*[2*NFreqs];
	for(int i =0;i<2*NFreqs;i++){HermiteFMatrix[i]=new double[((MNStruct *)context)->totshapecoeff+((MNStruct *)context)->numProfComponents]();}


    	int cpos = 0;
	for(int i = 0; i < ((MNStruct *)context)->numProfComponents; i++){


		for(int j =0; j < NFreqs; j++){
			BetaFreqs[j] = 2*M_PI*betas[i]*double(j+1);
	
		}

		for(int j =0; j < NFreqs; j++){

		
			TNothplMC(((MNStruct *)context)->numshapecoeff[i]+1,BetaFreqs[j],HermiteFMatrix[j], cpos);
		
			for(int c =0; c <  ((MNStruct *)context)->numshapecoeff[i]+1; c++){

				if(c%4 == 1 || c%4 == 3){
					HermiteFMatrix[j+NFreqs][cpos+c] = -1*HermiteFMatrix[j][cpos+c];
					HermiteFMatrix[j][cpos+c] = 0;
				}

				if(c%4 == 2 || c%4 == 3){
					HermiteFMatrix[j+NFreqs][cpos+c] *= -1;
					HermiteFMatrix[j][cpos+c] *= -1;
				}
			}

			
		}




		for(int k =0; k <  ((MNStruct *)context)->numshapecoeff[i]+1; k++){

			double Bconst=(1.0/sqrt(betas[i]*((MNStruct *)context)->ReferencePeriod))/sqrt(pow(2.0,k)*sqrt(M_PI));
			for(int j =0; j < NFreqs; j++){

				HermiteFMatrix[j][cpos+k]=HermiteFMatrix[j][cpos+k]*Bconst*exp(-0.5*BetaFreqs[j]*BetaFreqs[j])*Nbins*sqrt(2*M_PI*betas[i]*betas[i]);
				HermiteFMatrix[j+NFreqs][cpos+k]=HermiteFMatrix[j+NFreqs][cpos+k]*Bconst*exp(-0.5*BetaFreqs[j]*BetaFreqs[j])*Nbins*sqrt(2*M_PI*betas[i]*betas[i]);
			}



		}

		cpos +=  ((MNStruct *)context)->numshapecoeff[i]+1;
 	}


	//printf("in FFTPD: %i \n", FindThreshold);
	if(FindThreshold == 1){

		double threshold = pow(10.0, -10);

		int ThreshIndex = -1;
		double max = -1000;		
		int cpos = 0;
		for(int c = 0; c < ((MNStruct *)context)->numProfComponents; c++){
			for(int k = 0; k < ((MNStruct *)context)->numshapecoeff[c]; k++){
				//printf("Loading basis vector %i \n", k+cpos);

	

				for(int b = 0; b < NFreqs; b++){
					double real= HermiteFMatrix[b][k+cpos];
					double comp =  HermiteFMatrix[b+NFreqs][k+cpos];
					//printf("BV: %i %i %g %g\n", k,b,real, comp);
					if(sqrt(real*real + comp*comp) > max){ max = sqrt(real*real + comp*comp); printf("New max: %i %i %g \n", k,b,max); }

					if(sqrt(real*real + comp*comp)/max > threshold && b > ThreshIndex){
						ThreshIndex = b;	
						printf("New max: %i %i %g %g %g %g %g \n", k, b, real, comp, sqrt(real*real + comp*comp), max, sqrt(real*real + comp*comp)/max);
					}
				}
			}
			cpos += ((MNStruct *)context)->numProfileFitCoeff[c]+1;
		}
		printf("Max index required: %i \n", ThreshIndex);
		NFBasis = ThreshIndex-1;

		delete[] betas;
		delete[] BetaFreqs;
		delete[] HermiteFMatrix;

		return;
	}

	if(FindThreshold == 0){

		double **HermiteFJMatrix =  new double*[2*NFreqs];
		for(int i =0;i<2*NFreqs;i++){HermiteFJMatrix[i]=new double[((MNStruct *)context)->totshapecoeff]();}

		int cpos = 0;
		int jpos = 0;
		for(int c = 0; c < ((MNStruct *)context)->numProfComponents; c++){

			for(int i =0;i<2*NFreqs;i++){
				HermiteFJMatrix[i][0] = (1.0/sqrt(2.0))*(-1.0*HermiteFMatrix[i][1])/(betas[c]*((MNStruct *)context)->ReferencePeriod);
			}

		
			for(int k = 1; k < ((MNStruct *)context)->numshapecoeff[c]; k++){
				for(int i =0;i < 2*NFreqs; i++){
					HermiteFJMatrix[i][k+cpos] = (1.0/sqrt(2.0))*(sqrt(1.0*k)*HermiteFMatrix[i][k+jpos-1] - sqrt(1.0*(k+1.0))*HermiteFMatrix[i][k+jpos+1])/(betas[c]*((MNStruct *)context)->ReferencePeriod);
				}
			}
			cpos += ((MNStruct *)context)->numshapecoeff[c];
			jpos += ((MNStruct *)context)->numshapecoeff[c] + 1;
		}


		for(int t = 0; t < ((MNStruct *)context)->NumToInterpolate; t++){	
			int cpos = 0;
			int jpos = 0;
			for(int c = 0; c < ((MNStruct *)context)->numProfComponents; c++){
				for(int k = 0; k < ((MNStruct *)context)->numshapecoeff[c]; k++){
					//printf("Loading basis vector %i \n", k+cpos);

				

					for(int b = 0; b < NFBasis; b++){
						double real= HermiteFMatrix[b][k+jpos];
						double comp =  HermiteFMatrix[b+NFreqs][k+jpos];

						

						double freq = (b+1.0)/Nbins;
						double theta = 2*M_PI*(double(t)/((MNStruct *)context)->NumToInterpolate)*freq;
						double RealShift = cos(theta);
						double ImagShift = sin(-theta);


						double RealRolledData = real*RealShift - comp*ImagShift;
						double ImagRolledData = real*ImagShift + comp*RealShift;

								
						StoredFourierShapelets[t][b+(k+cpos)*2*NFBasis] = RealRolledData;
						StoredFourierShapelets[t][(b+NFBasis)+(k+cpos)*2*NFBasis] = ImagRolledData;

						//if(t==100){printf("BM: %i %i %g %g %g %g \n", k, b, real, comp,RealRolledData, ImagRolledData );}

					}


					for(int b = 0; b < NFBasis; b++){

						double real= HermiteFJMatrix[b][k+cpos];
						double comp =  HermiteFJMatrix[b+NFreqs][k+cpos];

						

						double freq = (b+1.0)/Nbins;
						double theta = 2*M_PI*(double(t)/((MNStruct *)context)->NumToInterpolate)*freq;
						double RealShift = cos(theta);
						double ImagShift = sin(-theta);


						double RealRolledData = real*RealShift - comp*ImagShift;
						double ImagRolledData = real*ImagShift + comp*RealShift;
				
						StoredFourierJitterShapelets[t][(b)+(k+cpos)*2*NFBasis] = RealRolledData;
						StoredFourierJitterShapelets[t][(b+NFBasis)+(k+cpos)*2*NFBasis] = ImagRolledData;
					
						//if(t==100){printf("BM: %i %i %g %g %g %g \n", k, b, real, comp,RealRolledData, ImagRolledData );}

					}

				}
				cpos += ((MNStruct *)context)->numProfileFitCoeff[c];
				jpos += ((MNStruct *)context)->numProfileFitCoeff[c] + 1;
			}

		}

		double *OneBasis = new double[Nbins]();
		fftw_plan plan;
		fftw_complex *FFTBasis;

		FFTBasis = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (Nbins/2+1));
		plan = fftw_plan_dft_r2c_1d(Nbins, OneBasis, FFTBasis, FFTW_ESTIMATE);

		for(int i = 0; i < ((MNStruct *)context)->pulse->nobs; i++){
			for(int b = 0; b < Nbins; b++){
				OneBasis[b] = (double)((MNStruct *)globalcontext)->ProfileData[i][b][1];
			}
			fftw_execute(plan);

			for(int b = 1; b < Nbins/2+1; b++){
				double real= FFTBasis[b][0];
				double comp =  FFTBasis[b][1];

			
				FourierProfileData[i][b] = real;
				FourierProfileData[i][b+Nbins/2+1] = comp;

				
				//printf("Data: %i %i %g %g \n", i, b, real, comp);
				


			}

		}

		delete[] OneBasis;
		fftw_free(FFTBasis);
		fftw_destroy_plan ( plan );

	}

	delete[] betas;
	delete[] BetaFreqs;
	delete[] HermiteFMatrix;


}

*/
/*
void ProfileDomainLikeMNWrap(double *Cube, int &ndim, int &npars, double &lnew, void *context){



        for(int p=0;p<ndim;p++){

                Cube[p]=(((MNStruct *)globalcontext)->PriorsArray[p+ndim]-((MNStruct *)globalcontext)->PriorsArray[p])*Cube[p]+((MNStruct *)globalcontext)->PriorsArray[p];
        }


	double *DerivedParams = new double[npars];

	double result = ProfileDomainLike(ndim, Cube, npars, DerivedParams, context);


	delete[] DerivedParams;

	lnew = result;

}
*/

/*
double  ProfileDomainLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){

	int dotime = 0;
	int debug = ((MNStruct *)globalcontext)->debug; 


	struct timeval tval_before, tval_after, tval_resultone, tval_resulttwo;

	if(dotime == 1){
		gettimeofday(&tval_before, NULL);
	}

	//double bluff = 0;
	//for(int loop = 0; loop < 5; loop++){
	//	Cube[0] = loop*0.2;

	
	if(debug == 1)printf("In like \n");

	double *EFAC;
	double *EQUAD;
        long double LDparams[ndim];
        int pcount;

	double uniformpriorterm = 0;



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	if(((MNStruct *)globalcontext)->doLinear== 2){phase =((MNStruct *)globalcontext)->LDpriors[0][0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;}
	pcount++;

	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}
		 if(((MNStruct *)globalcontext)->doLinear==0){

		long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
		for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

		   JumpVec[p] = 0;
		   for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
			for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
				if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){

					if(((MNStruct *)globalcontext)->pulse->fitJump[k] == 0){


						JumpVec[p] += (long double)((MNStruct *)globalcontext)->PreJumpVals[k]/SECDAY;
					}
					else{

						JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
					}
				 }
			}
		   }

		}
		//sleep(5);
		long double DM = ((MNStruct *)globalcontext)->pulse->param[param_dm].val[0];	
		long double RefFreq = pow(10.0, 6)*3100.0L;
		long double FirstFreq = pow(10.0, 6)*2646.0L;
		long double DMPhaseShift = DM*(1.0/(FirstFreq*FirstFreq) - 1.0/(RefFreq*RefFreq))/(2.410*pow(10.0,-16));


	/////////////////////////////////////////////////////////////////////////////////////////////  
	/////////////////////////Form SATS///////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////


		for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
			((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
		}

		for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
			((MNStruct *)globalcontext)->pulse->jumpVal[k]= 0;
		}

		delete[] JumpVec;


		if(debug == 1){
			for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
				printf("Pre res: %i %g %g %g \n", p, (double)((MNStruct *)globalcontext)->pulse->obsn[p].sat, (double)((MNStruct *)globalcontext)->pulse->obsn[p].bat, (double)((MNStruct *)globalcontext)->pulse->obsn[p].residual);
			}
		}

	//if(((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps >1){
		fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
		formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       
		
		//printf("in here \n");
	//}
	}

	if(debug == 1)printf("Phase: %g \n", (double)phase);
	if(debug == 1)printf("Formed Residuals \n");


	if(debug == 1){
		for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
			printf("Post res: %i %g %g %g \n", p, (double)((MNStruct *)globalcontext)->pulse->obsn[p].sat, (double)((MNStruct *)globalcontext)->pulse->obsn[p].bat, (double)((MNStruct *)globalcontext)->pulse->obsn[p].residual);
		}
	}

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[0]);}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[p]);}
			pcount++;

		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		if(((MNStruct *)globalcontext)->EQUADPriorType ==1) {uniformpriorterm += log(EQUAD[0]);}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			if(((MNStruct *)globalcontext)->EQUADPriorType ==1) {uniformpriorterm += log(EQUAD[o]);}
			pcount++;
                }
        }


        double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        	SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        	for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
           		SQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
        	}
    	}	  
	  
	double HighFreqStoc1 = 0;
	double HighFreqStoc2 = 0;
	if(((MNStruct *)globalcontext)->incHighFreqStoc == 1){
		HighFreqStoc1 = pow(10.0,Cube[pcount]);
		pcount++;
	}
        if(((MNStruct *)globalcontext)->incHighFreqStoc == 2){
                HighFreqStoc1 = pow(10.0,Cube[pcount]);
                pcount++;
		HighFreqStoc2 = pow(10.0,Cube[pcount]);
                pcount++;
        }

	double DMEQUAD = 0;
	if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
   		DMEQUAD=pow(10.0,Cube[pcount]);
		pcount++;
    	}	
	
	double WidthJitter = 0;
	if(((MNStruct *)globalcontext)->incWidthJitter > 0){
   		WidthJitter=pow(10.0,Cube[pcount]);
		if(((MNStruct *)globalcontext)->EQUADPriorType ==1) {uniformpriorterm += log(WidthJitter);}
		pcount++;
    	}

	double *ProfileNoiseAmps = new double[((MNStruct *)globalcontext)->ProfileNoiseCoeff]();
	int ProfileNoiseCoeff = 0;
	if(((MNStruct *)globalcontext)->incProfileNoise == 1){

		ProfileNoiseCoeff = ((MNStruct *)globalcontext)->ProfileNoiseCoeff;
		double ProfileNoiseAmp = pow(10.0, Cube[pcount]);
		pcount++;
		double ProfileNoiseSpec = Cube[pcount];
		pcount++;
		for(int q = 0; q < ProfileNoiseCoeff; q++){
			double profnoise = ProfileNoiseAmp*ProfileNoiseAmp*pow(double(q+1.0)/((MNStruct *)globalcontext)->ProfileInfo[0][1], ProfileNoiseSpec);
			ProfileNoiseAmps[q] = profnoise;
		}
	}

	if(((MNStruct *)globalcontext)->incProfileNoise == 2){

		ProfileNoiseCoeff = ((MNStruct *)globalcontext)->ProfileNoiseCoeff;
		double ProfileNoiseAmp = pow(10.0, Cube[pcount]);
		for(int q = 0; q < ProfileNoiseCoeff; q++){
			double ProfileNoiseAmp = pow(10.0, Cube[pcount]);
			pcount++;
			double profnoise = ProfileNoiseAmp*ProfileNoiseAmp;
			ProfileNoiseAmps[q] = profnoise;
		}
	}

        if(((MNStruct *)globalcontext)->incProfileNoise == 3){

                ProfileNoiseCoeff = ((MNStruct *)globalcontext)->ProfileNoiseCoeff;
                for(int q = 0; q < ProfileNoiseCoeff; q++){
                        double ProfileNoiseAmp = pow(10.0, Cube[pcount]);
                        pcount++;
                        double profnoise = ProfileNoiseAmp;
                        ProfileNoiseAmps[q] = profnoise;
                }
        }

        if(((MNStruct *)globalcontext)->incProfileNoise == 4){

                ProfileNoiseCoeff = ((MNStruct *)globalcontext)->ProfileNoiseCoeff;
                for(int q = 0; q < ProfileNoiseCoeff; q++){
                        double ProfileNoiseAmp = 1;
                        double profnoise = ProfileNoiseAmp;
                        ProfileNoiseAmps[q] = profnoise;
                }
        }

	double *RedSignalVec;
	double *SignalVec;
	double FreqDet = 0;
	double JDet = 0;
	double FreqLike = 0;
	int startpos = 0;





/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Get Red Signal//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////// 



	if(((MNStruct *)globalcontext)->incRED > 4){

		int FitRedCoeff = 2*((MNStruct *)globalcontext)->numFitRedCoeff;
		double *SignalCoeff = new double[FitRedCoeff];
		for(int i = 0; i < FitRedCoeff; i++){
			SignalCoeff[i] = Cube[pcount];
			//printf("coeffs %i %g \n", i, SignalCoeff[i]);
			pcount++;
		}
			
		

		double Tspan = ((MNStruct *)globalcontext)->Tspan;
		double f1yr = 1.0/3.16e7;


		if(((MNStruct *)globalcontext)->incRED==5){

			double Redamp=Cube[pcount];
			pcount++;
			double Redindex=Cube[pcount];
			pcount++;

			
			Redamp=pow(10.0, Redamp);
			if(((MNStruct *)globalcontext)->RedPriorType == 1) { uniformpriorterm +=log(Redamp); }



			for (int i=0; i< FitRedCoeff/2; i++){
				
				double freq = ((double)(i+1.0))/Tspan;
				
				double rho = (Redamp*Redamp/(12*M_PI*M_PI))*pow(f1yr,(-3)) * pow(freq*365.25,(-Redindex))/(Tspan*24*60*60);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitRedCoeff/2] = SignalCoeff[i+FitRedCoeff/2]*sqrt(rho);  
				//FreqDet += 2*log(rho);	
				//JDet += 2*log(sqrt(rho));
				FreqLike += SignalCoeff[i]*SignalCoeff[i]/rho + SignalCoeff[i+FitRedCoeff/2]*SignalCoeff[i+FitRedCoeff/2]/rho;
			}
		}


		if(((MNStruct *)globalcontext)->incRED==6){




			for (int i=0; i< FitRedCoeff/2; i++){
			

				double RedAmp = pow(10.0, Cube[pcount]);	
				double freq = ((double)(i+1.0))/Tspan;
				
				if(((MNStruct *)globalcontext)->RedPriorType == 1) { uniformpriorterm +=log(RedAmp); }
				double rho = (RedAmp*RedAmp);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitRedCoeff/2] = SignalCoeff[i+FitRedCoeff/2]*sqrt(rho);  
				FreqLike += SignalCoeff[i]*SignalCoeff[i]/rho + SignalCoeff[i+FitRedCoeff/2]*SignalCoeff[i+FitRedCoeff/2]/rho;
				pcount++;
			}
		}

		double *FMatrix = new double[FitRedCoeff*((MNStruct *)globalcontext)->numProfileEpochs];
		for(int i=0;i< FitRedCoeff/2;i++){
			int DMt = 0;
			for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[DMt].bat;
	
				FMatrix[k + (i+startpos)*((MNStruct *)globalcontext)->numProfileEpochs]=cos(2*M_PI*(double(i+1)/Tspan)*time);
				FMatrix[k + (i+FitRedCoeff/2+startpos)*((MNStruct *)globalcontext)->numProfileEpochs] = sin(2*M_PI*(double(i+1)/Tspan)*time);
				DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];

			}
		}

		RedSignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs];
		vector_dgemv(FMatrix,SignalCoeff,RedSignalVec,((MNStruct *)globalcontext)->numProfileEpochs, FitRedCoeff,'N');
		startpos=FitRedCoeff;
		delete[] FMatrix;	
		delete[] SignalCoeff;
		

    	}


/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Get  DM Variations////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////  

	if(((MNStruct *)globalcontext)->incDM > 4){

		int FitDMCoeff = 2*((MNStruct *)globalcontext)->numFitDMCoeff;
		double *SignalCoeff = new double[FitDMCoeff];
		for(int i = 0; i < FitDMCoeff; i++){
			SignalCoeff[i] = Cube[pcount];
			//printf("coeffs %i %g \n", i, SignalCoeff[i]);
			pcount++;
		}
			
		

		double Tspan = ((MNStruct *)globalcontext)->Tspan;
		double f1yr = 1.0/3.16e7;


		if(((MNStruct *)globalcontext)->incDM==5){

			double DMamp=Cube[pcount];
			pcount++;
			double DMindex=Cube[pcount];
			pcount++;

			
			DMamp=pow(10.0, DMamp);
			if(((MNStruct *)globalcontext)->DMPriorType == 1) { uniformpriorterm +=log(DMamp); }



			for (int i=0; i< FitDMCoeff/2; i++){
				
				double freq = ((double)(i+1.0))/Tspan;
				
				double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freq*365.25,(-DMindex))/(Tspan*24*60*60);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitDMCoeff/2] = SignalCoeff[i+FitDMCoeff/2]*sqrt(rho);  
				//FreqDet += 2*log(rho);	
				//JDet += 2*log(sqrt(rho));
				FreqLike += SignalCoeff[i]*SignalCoeff[i]/rho + SignalCoeff[i+FitDMCoeff/2]*SignalCoeff[i+FitDMCoeff/2]/rho;
			}
		}


		if(((MNStruct *)globalcontext)->incDM==6){




			for (int i=0; i< FitDMCoeff/2; i++){
			

				double DMAmp = pow(10.0, Cube[pcount]);	
				double freq = ((double)(i+1.0))/Tspan;
				
				if(((MNStruct *)globalcontext)->DMPriorType == 1) { uniformpriorterm +=log(DMAmp); }
				double rho = (DMAmp*DMAmp);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitDMCoeff/2] = SignalCoeff[i+FitDMCoeff/2]*sqrt(rho);  
				FreqLike += SignalCoeff[i]*SignalCoeff[i]/rho + SignalCoeff[i+FitDMCoeff/2]*SignalCoeff[i+FitDMCoeff/2]/rho;
				pcount++;
			}
		}

		double *FMatrix = new double[FitDMCoeff*((MNStruct *)globalcontext)->numProfileEpochs];
		for(int i=0;i< FitDMCoeff/2;i++){
			int DMt = 0;
			for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[DMt].bat;
	
				FMatrix[k + (i+startpos)*((MNStruct *)globalcontext)->numProfileEpochs]=cos(2*M_PI*(double(i+1)/Tspan)*time);
				FMatrix[k + (i+FitDMCoeff/2+startpos)*((MNStruct *)globalcontext)->numProfileEpochs] = sin(2*M_PI*(double(i+1)/Tspan)*time);
				DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];

			}
		}

		SignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs];
		vector_dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->numProfileEpochs, FitDMCoeff,'N');
		startpos=FitDMCoeff;
		delete[] FMatrix;	
		delete[] SignalCoeff;
		

    	}



/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Subtract  DM Shape Events////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////  


	double *DMShapeCoeffParams = new double[2+((MNStruct *)globalcontext)->numDMShapeCoeff];
	if(((MNStruct *)globalcontext)->numDMShapeCoeff > 0){
		for(int i = 0; i < 2+((MNStruct *)globalcontext)->numDMShapeCoeff; i++){
			DMShapeCoeffParams[i] = Cube[pcount];
			pcount++;
		}
	}





	double sinAmp = 0;
	double cosAmp = 0;
	if(((MNStruct *)globalcontext)->yearlyDM == 1){
		if(((MNStruct *)globalcontext)->incDM == 0){ SignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs]();}
		sinAmp = Cube[pcount];
		pcount++;
		cosAmp = Cube[pcount];
		pcount++;
		int DMt = 0;
		for(int o=0;o<((MNStruct *)globalcontext)->numProfileEpochs; o++){
			double time=(double)((MNStruct *)globalcontext)->pulse->obsn[DMt].bat - ((MNStruct *)globalcontext)->pulse->param[param_dmepoch].val[0];
			SignalVec[o] += sinAmp*sin((2*M_PI/365.25)*time) + cosAmp*cos((2*M_PI/365.25)*time);
			DMt += ((MNStruct *)globalcontext)->numChanPerInt[o];
		}
	}
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){

		int ProfNbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
		ProfileBats[i] = new long double[2];



		ProfileBats[i][0] = ((MNStruct *)globalcontext)->ProfileData[i][0][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
		ProfileBats[i][1] = ((MNStruct *)globalcontext)->ProfileData[i][ProfNbin-1][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;


		ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
		if(debug == 1){printf("MBat %i %.15Lg %.15Lg %.15Lg %.15Lg %.15Lg %.15Lg\n", i, ((MNStruct *)globalcontext)->pulse->obsn[i].sat, ModelBats[i], ((MNStruct *)globalcontext)->ProfileInfo[i][5], ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr, phase, ((MNStruct *)globalcontext)->pulse->obsn[i].residual);}
	 }


	if(((MNStruct *)globalcontext)->doLinear>0){

		//printf("in do linear %i %i\n", phaseDim+1, phaseDim+((MNStruct *)globalcontext)->numFitTiming);

		double *TimingParams = new double[((MNStruct *)globalcontext)->numFitTiming+((MNStruct *)globalcontext)->numFitJumps]();
		double *TimingSignal = new double[((MNStruct *)globalcontext)->pulse->nobs];

		int fitcount = 1;
		if(((MNStruct *)globalcontext)->doLinear == 2){fitcount = 0;}
		for(int i = fitcount; i < ((MNStruct *)globalcontext)->numFitTiming+((MNStruct *)globalcontext)->numFitJumps; i++){
			TimingParams[i] = Cube[i];
		}

		vector_dgemv(((MNStruct *)globalcontext)->DMatrixVec, TimingParams,TimingSignal,((MNStruct *)globalcontext)->pulse->nobs,((MNStruct *)globalcontext)->numFitTiming+((MNStruct *)globalcontext)->numFitJumps,'N');

		for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
			//printf("ltime: %i %g \n", i, TimingSignal[i]);
	     		 ModelBats[i] -=  (long double) TimingSignal[i]/SECDAY;

		}	

		delete[] TimingSignal;
		delete[] TimingParams;	

	}
	int maxshapecoeff = 0;
	int totshapecoeff = ((MNStruct *)globalcontext)->totshapecoeff; 

	int *numcoeff= new int[((MNStruct *)globalcontext)->numProfComponents];
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		numcoeff[i] =  ((MNStruct *)globalcontext)->numshapecoeff[i];
		if(debug == 1){printf("num coeff in comp %i: %i \n", i, numcoeff[i]);}
	}

	
        int *numProfileStocCoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;
        int totalshapestoccoeff = ((MNStruct *)globalcontext)->totalshapestoccoeff;


	int *numEvoCoeff = ((MNStruct *)globalcontext)->numEvoCoeff;
	int totalEvoCoeff = ((MNStruct *)globalcontext)->totalEvoCoeff;

	int *numEvoFitCoeff = ((MNStruct *)globalcontext)->numEvoFitCoeff;
	int totalEvoFitCoeff = ((MNStruct *)globalcontext)->totalEvoFitCoeff;

	int *numProfileFitCoeff = ((MNStruct *)globalcontext)->numProfileFitCoeff;
	int totalProfileFitCoeff = ((MNStruct *)globalcontext)->totalProfileFitCoeff;



	int totalCoeffForMult = 0;
	int *NumCoeffForMult = new int[((MNStruct *)globalcontext)->numProfComponents];
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		NumCoeffForMult[i] = 0;
		if(numEvoCoeff[i] > NumCoeffForMult[i]){NumCoeffForMult[i]=numEvoCoeff[i];}
		if(numProfileFitCoeff[i] > NumCoeffForMult[i]){NumCoeffForMult[i]=numProfileFitCoeff[i];}
		totalCoeffForMult += NumCoeffForMult[i];
		if(debug == 1){printf("num coeff for mult from comp %i: %i \n", i, NumCoeffForMult[i]);}
	}

        int *numShapeToSave = new int[((MNStruct *)globalcontext)->numProfComponents];
        for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
                numShapeToSave[i] = numProfileStocCoeff[i];
                if(numEvoCoeff[i] >  numShapeToSave[i]){numShapeToSave[i] = numEvoCoeff[i];}
                if(numProfileFitCoeff[i] >  numShapeToSave[i]){numShapeToSave[i] = numProfileFitCoeff[i];}
                if(debug == 1){printf("saved %i %i \n", i, numShapeToSave[i]);}
        }
        int totShapeToSave = 0;
        for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
                totShapeToSave += numShapeToSave[i];
        }

	double shapecoeff[totshapecoeff];
	double StocProfCoeffs[totalshapestoccoeff];
	double **EvoCoeffs=new double*[((MNStruct *)globalcontext)->NProfileEvoPoly]; 
	for(int i = 0; i < ((MNStruct *)globalcontext)->NProfileEvoPoly; i++){EvoCoeffs[i] = new double[totalEvoCoeff];}

	double ProfileFitCoeffs[totalProfileFitCoeff];
	double ProfileModCoeffs[totalCoeffForMult];

	for(int i =0; i < totshapecoeff; i++){
		shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
	}
	for(int p = 0; p < ((MNStruct *)globalcontext)->NProfileEvoPoly; p++){	
		for(int i =0; i < totalEvoCoeff; i++){
			EvoCoeffs[p][i]=((MNStruct *)globalcontext)->MeanProfileEvo[p][i];
			//printf("loaded evo coeff %i %g \n", i, EvoCoeffs[i]);
		}
	}
	if(debug == 1){printf("Filled %i Coeff, %i EvoCoeff \n", totshapecoeff, totalEvoCoeff);}


	double *betas = new double[((MNStruct *)globalcontext)->numProfComponents]();
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		betas[i] = ((MNStruct *)globalcontext)->MeanProfileBeta[i]*((MNStruct *)globalcontext)->ReferencePeriod;
	}



	int cpos = 0;
	int epos = 0;
	int fpos = 0;
	
	for(int i =0; i < totalshapestoccoeff; i++){
		//uniformpriorterm += log(pow(10.0, Cube[pcount]));
		StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
		pcount++;
	}
	//printf("TotalProfileFit: %i \n",totalProfileFitCoeff );
	for(int i =0; i < totalProfileFitCoeff; i++){
		ProfileFitCoeffs[i]= Cube[pcount];
		pcount++;
	}
	double LinearProfileWidth=0;
	if(((MNStruct *)globalcontext)->FitLinearProfileWidth == 1){
		LinearProfileWidth = Cube[pcount];
		pcount++;
	}

	double *LinearCompWidthTime;
	if(((MNStruct *)globalcontext)->NumFitCompWidths+((MNStruct *)globalcontext)->NumFitCompPos > 0){
		LinearCompWidthTime = new double[((MNStruct *)globalcontext)->NumFitCompWidths+((MNStruct *)globalcontext)->NumFitCompPos];
		for(int i = 0; i < ((MNStruct *)globalcontext)->NumFitCompWidths+((MNStruct *)globalcontext)->NumFitCompPos; i++){
			LinearCompWidthTime[i] = Cube[pcount];
			pcount++;
		}
        }

	double *LinearWidthEvoTime;
	if(((MNStruct *)globalcontext)->incWidthEvoTime > 0){
		LinearWidthEvoTime = new double[((MNStruct *)globalcontext)->incWidthEvoTime];
		for(int i = 0; i < ((MNStruct *)globalcontext)->incWidthEvoTime; i++){
			LinearWidthEvoTime[i] = Cube[pcount];
			pcount++;
		}
        }


	int  EvoFreqExponent = 1;
	if(((MNStruct *)globalcontext)->FitEvoExponent == 1){
		EvoFreqExponent =  floor(Cube[pcount]);
		//printf("Evo Exp set: %i %i \n", pcount, EvoFreqExponent);
		pcount++;
	}
	
	for(int p = 0; p < ((MNStruct *)globalcontext)->NProfileEvoPoly; p++){	
		cpos = 0;
		for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
			for(int i =0; i < numEvoFitCoeff[c]; i++){
				//printf("Evo P: %i %i %i %g %g\n", p, c, i, EvoCoeffs[p][i+cpos],Cube[pcount]);
				EvoCoeffs[p][i+cpos] += Cube[pcount];
				
				pcount++;
			}
			cpos += numEvoCoeff[c];
		
		}
	}

	double EvoProfileWidth=0;
	if(((MNStruct *)globalcontext)->incProfileEvo == 2){
		EvoProfileWidth = Cube[pcount];
		pcount++;
	}

	double EvoEnergyProfileWidth=0;
	if(((MNStruct *)globalcontext)->incProfileEnergyEvo == 2){
		EvoEnergyProfileWidth = Cube[pcount];
		pcount++;
	}


	double **ProfTimeEvoPoly;
	int *numTimeEvoCoeff;
	int NProfileTimePoly = 0;
	int NumTimeEvoEpochs = 0;
	if(((MNStruct *)globalcontext)->NProfileTimePoly > 0){

		NProfileTimePoly=((MNStruct *)globalcontext)->NProfileTimePoly;
		NumTimeEvoEpochs=((MNStruct *)globalcontext)->NumTimeEvoEpochs;

		ProfTimeEvoPoly = new double*[NumTimeEvoEpochs];
		


		numTimeEvoCoeff= new int[((MNStruct *)globalcontext)->numProfComponents]();

		for(int ep = 0; ep < NumTimeEvoEpochs; ep++){
			ProfTimeEvoPoly[ep] = new double[(((MNStruct *)globalcontext)->numProfComponents-1)*NProfileTimePoly]();

			for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents-1; i++){
				numTimeEvoCoeff[i+1] = 1;

				for(int p = 0; p < ((MNStruct *)globalcontext)->NProfileTimePoly; p++){
					ProfTimeEvoPoly[ep][i*((MNStruct *)globalcontext)->NProfileTimePoly + p] = Cube[pcount];
					pcount++;
				}
			}
		}
	}
		


	double **ExtraCompProps=new double*[((MNStruct *)globalcontext)->incExtraProfComp];	
	double **ExtraCompBasis=new double*[((MNStruct *)globalcontext)->incExtraProfComp];
	int incExtraProfComp = ((MNStruct *)globalcontext)->incExtraProfComp;


	for(int i = 0; i < incExtraProfComp; i++){	
		if((int)((MNStruct *)globalcontext)->FitForExtraComp[i][0] == 1){
			//Step Model, time, width, amp
			ExtraCompProps[i] = new double[4+(int)((MNStruct *)globalcontext)->FitForExtraComp[i][1]];
			ExtraCompBasis[i] = new double[(int)((MNStruct *)globalcontext)->FitForExtraComp[i][1]];
			
			ExtraCompProps[i][0] = Cube[pcount]; //Position in time
			pcount++;
			ExtraCompProps[i][1] = Cube[pcount]; // position in phase
			pcount++;
			ExtraCompProps[i][2] = pow(10.0, Cube[pcount])*((MNStruct *)globalcontext)->ReferencePeriod; // comp width
			pcount++;

			for(int c = 0; c < (int)((MNStruct *)globalcontext)->FitForExtraComp[i][1]; c++){
	//			printf("Amps: %i %g \n", pcount,  Cube[pcount]);

				ExtraCompProps[i][3+c] = Cube[pcount]; //amp
				pcount++;
			}

			if((int)((MNStruct *)globalcontext)->FitForExtraComp[i][2] == 1){
				ExtraCompProps[i][3+(int)((MNStruct *)globalcontext)->FitForExtraComp[i][1]] = Cube[pcount]; // shift
                                pcount++;
                        }

		
		}
	//	printf("details: %i %i \n", incExtraProfComp, ((MNStruct *)globalcontext)->NumExtraCompCoeffs);
		if((int)((MNStruct *)globalcontext)->FitForExtraComp[i][0] == 3){
			//exponential decay: time, comp width, max comp amp, log decay timescale

			ExtraCompProps[i] = new double[5+(int)((MNStruct *)globalcontext)->FitForExtraComp[i][1]];
			ExtraCompBasis[i] = new double[(int)((MNStruct *)globalcontext)->FitForExtraComp[i][1]];

			ExtraCompProps[i][0] = Cube[pcount]; //Position
			pcount++;
			ExtraCompProps[i][1] = Cube[pcount]; // position in phase
			pcount++;
			ExtraCompProps[i][2] = pow(10.0, Cube[pcount])*((MNStruct *)globalcontext)->ReferencePeriod; // comp width
			pcount++;
			for(int c = 0; c < (int)((MNStruct *)globalcontext)->FitForExtraComp[i][1]; c++){
	//			printf("Amps: %i %g \n", pcount,  Cube[pcount]);

				ExtraCompProps[i][3+c] = Cube[pcount]; //amp
				pcount++;
			}
			ExtraCompProps[i][3+(int)((MNStruct *)globalcontext)->FitForExtraComp[i][1]] = pow(10.0, Cube[pcount]); //Event width
			pcount++;

			if((int)((MNStruct *)globalcontext)->FitForExtraComp[i][2] == 1){
				ExtraCompProps[i][4+(int)((MNStruct *)globalcontext)->FitForExtraComp[i][1]] = Cube[pcount];
				pcount++;
			}
		}
	}

	double *PrecessionParams;
	int incPrecession = ((MNStruct *)globalcontext)->incPrecession;
	if(incPrecession == 1){
		PrecessionParams = new double[totalProfileFitCoeff+1];

		PrecessionParams[0] = pow(10.0, Cube[pcount]); //Timescale in years
		pcount++;
		for(int i = 0; i < totalProfileFitCoeff; i++){
				PrecessionParams[1+i] = Cube[pcount];
				pcount++;
		
		}
	}

	double *PrecBasis;

	if(incPrecession == 2){

		PrecessionParams = new double[((MNStruct *)globalcontext)->numProfComponents + (((MNStruct *)globalcontext)->numProfComponents-1) + (((MNStruct *)globalcontext)->numProfComponents-1)*(((MNStruct *)globalcontext)->FitPrecAmps+1)];
		PrecBasis = new double[(((MNStruct *)globalcontext)->numProfComponents)*((MNStruct *)globalcontext)->LargestNBins];
		int preccount = 0;

		for(int p2 = 0; p2 < ((MNStruct *)globalcontext)->numProfComponents; p2++){
			PrecessionParams[preccount] = pow(10.0, Cube[pcount]);
			preccount++;
			pcount++;
		}
		for(int p2 = 0; p2 < ((MNStruct *)globalcontext)->numProfComponents-1; p2++){

			PrecessionParams[preccount] = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
			pcount++;
			preccount++;

		}
		for(int p2 = 0; p2 < (((MNStruct *)globalcontext)->numProfComponents-1); p2++){
			for(int p3 = 0; p3 < (((MNStruct *)globalcontext)->FitPrecAmps+1); p3++){
				PrecessionParams[preccount] =Cube[pcount];

				if(p3==0){uniformpriorterm += log(pow(10.0, PrecessionParams[preccount]));}
				if(((MNStruct *)globalcontext)->doMax == 1){uniformpriorterm += log(pow(10.0, PrecessionParams[preccount]));}
				preccount++;
				pcount++;
			}
		}

	}



	double *TimeCorrProfileParams;
	int incTimeCorrProfileNoise = ((MNStruct *)globalcontext)->incTimeCorrProfileNoise;

	if(incTimeCorrProfileNoise == 1){
		TimeCorrProfileParams = new double[2*((MNStruct *)globalcontext)->totalTimeCorrCoeff];

		for(int c = 0; c < ((MNStruct *)globalcontext)->totalTimeCorrCoeff; c++){
			for(int i = 0; i < 2; i++){
//				printf("Cube: %i %i %g \n", ((MNStruct *)globalcontext)->totalTimeCorrCoeff, pcount, Cube[pcount]);
				TimeCorrProfileParams[2*c+i] = Cube[pcount];
				FreqLike += TimeCorrProfileParams[2*c+i]*TimeCorrProfileParams[2*c+i];
				pcount++;
			}
//			printf("Cube: %i %g \n", pcount, Cube[pcount]);
			double TimeCorrPower = pow(10.0, Cube[pcount]);
			pcount++;

			for(int i = 0; i < 2; i++){
				TimeCorrProfileParams[2*c+i] = TimeCorrProfileParams[2*c+i]*TimeCorrPower;
			}
		}


	}



	double ProfileScatter = 0;
	double ProfileScatter2 = 0;
	double ProfileScatterCut = 0;
	if(((MNStruct *)globalcontext)->incProfileScatter == 1){
		ProfileScatter = pow(10.0, Cube[pcount]);
		pcount++;
		
		if( ((MNStruct *)globalcontext)->ScatterPBF ==  4 || ((MNStruct *)globalcontext)->ScatterPBF ==  6 ){
	
			ProfileScatter2 = pow(10.0, Cube[pcount]);
			if(ProfileScatter2 < ProfileScatter){
                                double PSTemp = ProfileScatter;
                                ProfileScatter = ProfileScatter2;
                                ProfileScatter2 = PSTemp;

			 	double LogPSTemp = Cube[pcount-1];
				Cube[pcount-1] = Cube[pcount];
				Cube[pcount] = LogPSTemp;

	
                        }

			pcount++;
		}		
	

		if( ((MNStruct *)globalcontext)->ScatterPBF ==  5){
			ProfileScatterCut = Cube[pcount];
			pcount++;
		}

				
	}


//	sleep(5);
	if(totshapecoeff+1>=totalshapestoccoeff+1){
		maxshapecoeff=totshapecoeff+1;
	}
	if(totalshapestoccoeff+1 > totshapecoeff+1){
		maxshapecoeff=totalshapestoccoeff+1;
	}

//	printf("max shape coeff: %i \n", maxshapecoeff);
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double modelflux=0;
	cpos = 0;
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		for(int j =0; j < numcoeff[i]; j=j+2){
			modelflux+=sqrt(sqrt(M_PI))*sqrt(betas[i])*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*shapecoeff[cpos+j];
		}
		cpos+= numcoeff[i];
	}
	if(debug == 1){printf("model flux: %g \n", modelflux);}
	double ModModelFlux = modelflux;
	double OPLevel = ((MNStruct *)globalcontext)->offPulseLevel;


	double lnew = 0;
	int badshape = 0;

	int GlobalNBins = ((MNStruct *)globalcontext)->LargestNBins;

	int ProfileBaselineTerms = ((MNStruct *)globalcontext)->ProfileBaselineTerms;
	int PerEpochSize = ProfileBaselineTerms + 1 + 2*ProfileNoiseCoeff;
	int Msize = 1+ProfileBaselineTerms+2*ProfileNoiseCoeff + totalshapestoccoeff;
	int Mcounter=0;
	if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
		Msize++;
	}
	if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
		Msize++;
	}
	if(((MNStruct *)globalcontext)->incWidthJitter > 0){
		Msize++;
	}



	double *MNM = new double[Msize*Msize];




	if(dotime == 1){

		gettimeofday(&tval_after, NULL);
		timersub(&tval_after, &tval_before, &tval_resultone);
		printf("Time elapsed Up to Start of main loop: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
		//gettimeofday(&tval_before, NULL);
		
	}	
	//for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){


	int minep = 0;
	int maxep = ((MNStruct *)globalcontext)->numProfileEpochs;
	int t = 0;
	if(((MNStruct *)globalcontext)->SubIntToFit != -1){
		minep = ((MNStruct *)globalcontext)->SubIntToFit;
		maxep = minep+1;
		for(int ep = 0; ep < minep; ep++){	
			t += ((MNStruct *)globalcontext)->numChanPerInt[ep];
		}
	}
	//printf("eps: %i %i \n", minep, maxep);
	

	for(int ep = minep; ep < maxep; ep++){

		int EpochNbins = (int)((MNStruct *)globalcontext)->ProfileInfo[t][1];

		//printf("Epoch NBins: %i %i %i %i \n", ep, t, EpochNbins, GlobalNBins);

		double *M = new double[EpochNbins*Msize];
		double *NM = new double[EpochNbins*Msize];
	
		double *EpochMNM;
		double *EpochdNM;
		double *EpochTempdNM;
		int EpochMsize = 0;

		int NChanInEpoch = ((MNStruct *)globalcontext)->numChanPerInt[ep];
		int NEpochBins = NChanInEpoch*GlobalNBins;

		if(((MNStruct *)globalcontext)->incWideBandNoise == 1){

			EpochMsize = PerEpochSize*NChanInEpoch+totalshapestoccoeff;
			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
				EpochMsize++;
			}
			if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
				EpochMsize++;
			}
			if(((MNStruct *)globalcontext)->incWidthJitter > 0){
				EpochMsize++;
			}

			EpochMNM = new double[EpochMsize*EpochMsize]();

			EpochdNM = new double[EpochMsize]();
			EpochTempdNM = new double[EpochMsize]();
		}

		double EpochChisq = 0;	
		double EpochdetN = 0;
		double EpochLike = 0;


		for(int ch = 0; ch < NChanInEpoch; ch++){

			if(dotime == 4){	
				gettimeofday(&tval_before, NULL);
			}
			if(debug == 1){
				printf("In toa %i \n", t);
				printf("sat: %.15Lg \n", ((MNStruct *)globalcontext)->pulse->obsn[t].sat);
			}
			int nTOA = t;

			double detN  = 0;
			double Chisq  = 0;
			double logMargindet = 0;
			double Marginlike = 0;	
			double badlike = 0; 
			double JitterPrior = 0;	   

			double profilelike=0;

			long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
			long double FoldingPeriodDays = FoldingPeriod/SECDAY;

			int Nbins = GlobalNBins;
			int ProfNbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
			int BinRatio = Nbins/ProfNbins;

			double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
			double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
			long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

			//double *Betatimes = new double[Nbins];
	

			double *shapevec  = new double[Nbins]();
			double *ProfileModVec = new double[Nbins]();
			double *ProfileJitterModVec = new double[Nbins]();
		

			double noisemean=0;
			double MLSigma = 0;
			int MLSigmaCountCheck=0;
			double dataflux = 0;

			//printf("Bins: %i %i \n", t, Nbins);
			double maxamp = 0;
			double maxshape=0;

			//if(ep==36){printf("SSB %i %.10g %.10g \n", nTOA, (double)((MNStruct *)globalcontext)->pulse->obsn[t].freqSSB/pow(10.0, 6), (double)((MNStruct *)globalcontext)->pulse->obsn[t].freq);}

	//
	//		if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g \n", MLSigma, dataflux, modelflux, maxamp);


		    
			long double binpos = ModelBats[nTOA]; 


			if(((MNStruct *)globalcontext)->incRED > 4){
				long double Redshift = (long double)RedSignalVec[ep];

				
		 		binpos+=Redshift/SECDAY;

			}

			if(((MNStruct *)globalcontext)->incDM > 4 || ((MNStruct *)globalcontext)->yearlyDM == 1){
	                	double DMKappa = 2.410*pow(10.0,-16);
        		        double DMScale = 1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[t].freqSSB,2));
				long double DMshift = (long double)(SignalVec[ep]*DMScale);

				
		 		binpos+=DMshift/SECDAY;

			}
			for(int ev = 0; ev < incExtraProfComp; ev++){
				if((int)((MNStruct *)globalcontext)->FitForExtraComp[ev][2] == 1){

					if((int)((MNStruct *)globalcontext)->FitForExtraComp[ev][0] == 1){
						if(((MNStruct *)globalcontext)->pulse->obsn[t].bat >= ExtraCompProps[ev][0]){
							   long double TimeDelay = (long double) ExtraCompProps[ev][3+(int)((MNStruct *)globalcontext)->FitForExtraComp[ev][1]];
							   binpos+=TimeDelay/SECDAY;

						}
					}

					if((int)((MNStruct *)globalcontext)->FitForExtraComp[ev][0] == 3){
						double ExtraCompDecayScale = 0;
						if(((MNStruct *)globalcontext)->pulse->obsn[t].bat < ExtraCompProps[ev][0]){
								   ExtraCompDecayScale = 0;
						}
						else{
							ExtraCompDecayScale = exp((ExtraCompProps[ev][0]-((MNStruct *)globalcontext)->pulse->obsn[t].bat)/ExtraCompProps[ev][3+(int)((MNStruct *)globalcontext)->FitForExtraComp[ev][1]]);
						}

						long double TimeDelay = (long double) ExtraCompDecayScale*ExtraCompProps[ev][4+(int)((MNStruct *)globalcontext)->FitForExtraComp[ev][1]];
						 binpos+=TimeDelay/SECDAY;
					}
				}
			}

			if(((MNStruct *)globalcontext)->incDMShapeEvent != 0){

				for(int i =0; i < ((MNStruct *)globalcontext)->incDMShapeEvent; i++){

					int numDMShapeCoeff=((MNStruct *)globalcontext)->numDMShapeCoeff;

					double EventPos = DMShapeCoeffParams[0];
					double EventWidth=DMShapeCoeffParams[1];



					double *DMshapecoeff=new double[numDMShapeCoeff];
					double *DMshapeVec=new double[numDMShapeCoeff];
					for(int c=0; c < numDMShapeCoeff; c++){
						DMshapecoeff[c]=DMShapeCoeffParams[2+c];
					}


					double time=(double)((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat;

					double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
					TNothpl(numDMShapeCoeff,HVal,DMshapeVec);
					double DMsignal=0;
					double DMKappa = 2.410*pow(10.0,-16);
					double DMScale = 1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[nTOA].freqSSB,2));
					for(int c=0; c < numDMShapeCoeff; c++){
						DMsignal += (1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c)))*DMshapeVec[c]*DMshapecoeff[c]*DMScale;
					}

					DMsignal = DMsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
					


					binpos += DMsignal/SECDAY;


					delete[] DMshapecoeff;
					delete[] DMshapeVec;

				}
			}
			//if(binpos < ProfileBats[nTOA][0]){printf("UnderBoard! %.10Lg %.10Lg %.10Lg\n", binpos, ProfileBats[nTOA][0], ProfileBats[nTOA][Nbins-1]);}
			
			if(debug == 1){printf("binpos: %i  %.15Lg %.15Lg %.15Lg \n", t, binpos, ProfileBats[nTOA][0], ProfileBats[nTOA][1]);}

			if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

			if(binpos > ProfileBats[nTOA][1])binpos-=FoldingPeriodDays;


			if(binpos > ProfileBats[nTOA][1]){printf("OverBoard! %.10Lg %.10Lg %.10Lg\n", binpos, ProfileBats[nTOA][1], (binpos-ProfileBats[nTOA][1])/FoldingPeriodDays);}

			long double minpos = binpos - FoldingPeriodDays/2;
			if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
			long double maxpos = binpos + FoldingPeriodDays/2;
			if(maxpos> ProfileBats[nTOA][1])maxpos =ProfileBats[nTOA][1];

			
			if(incPrecession == 0){

				int InterpBin = 0;
				double FirstInterpTimeBin = 0;
				int  NumWholeBinInterpOffset = 0;


		
				long double timediff = 0;
				long double bintime = ProfileBats[t][0];


				if(bintime  >= minpos && bintime <= maxpos){
				    timediff = bintime - binpos;
				}
				else if(bintime < minpos){
				    timediff = FoldingPeriodDays+bintime - binpos;
				}
				else if(bintime > maxpos){
				    timediff = bintime - FoldingPeriodDays - binpos;
				}

				timediff=timediff*SECDAY;

				double OneBin = FoldingPeriod/Nbins;
				int NumBinsInTimeDiff = floor(timediff/OneBin + 0.5);
				double WholeBinsInTimeDiff = NumBinsInTimeDiff*FoldingPeriod/Nbins;
				double OneBinTimeDiff = -1*((double)timediff - WholeBinsInTimeDiff);

				double PWrappedTimeDiff = (OneBinTimeDiff - floor(OneBinTimeDiff/OneBin)*OneBin);

				if(debug == 1)printf("Making InterpBin: %g %g %i %g %g %g\n", (double)timediff, OneBin, NumBinsInTimeDiff, WholeBinsInTimeDiff, OneBinTimeDiff, PWrappedTimeDiff);

				InterpBin = floor(PWrappedTimeDiff/((MNStruct *)globalcontext)->InterpolatedTime+0.5);
				if(InterpBin >= ((MNStruct *)globalcontext)->NumToInterpolate)InterpBin -= ((MNStruct *)globalcontext)->NumToInterpolate;

				FirstInterpTimeBin = -1*(InterpBin-1)*((MNStruct *)globalcontext)->InterpolatedTime;

				if(debug == 1)printf("Interp Time Diffs: %g %g %g %g \n", ((MNStruct *)globalcontext)->InterpolatedTime, InterpBin*((MNStruct *)globalcontext)->InterpolatedTime, PWrappedTimeDiff, InterpBin*((MNStruct *)globalcontext)->InterpolatedTime-PWrappedTimeDiff);

				double FirstBinOffset = timediff-FirstInterpTimeBin;
				double dNumWholeBinOffset = FirstBinOffset/(FoldingPeriod/Nbins);
				int  NumWholeBinOffset = 0;

				NumWholeBinInterpOffset = floor(dNumWholeBinOffset+0.5);
	
				if(debug == 1)printf("Interp bin %i is: %i , First Bin is %g, Offset is %i \n", t, InterpBin, FirstInterpTimeBin, NumWholeBinInterpOffset);


	
			if(dotime == 2){
				gettimeofday(&tval_after, NULL);
				timersub(&tval_after, &tval_before, &tval_resultone);
				printf("Time elapsed up to start of Interp: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
				gettimeofday(&tval_before, NULL);
			}

			double reffreq = ((MNStruct *)globalcontext)->EvoRefFreq;
			double freqdiff =  (((MNStruct *)globalcontext)->pulse->obsn[t].freq - reffreq)/1000.0;
			double freqscale = pow(freqdiff, EvoFreqExponent);


//			double snr = ((MNStruct *)globalcontext)->pulse->obsn[t].snr;
//			double tobs = ((MNStruct *)globalcontext)->pulse->obsn[t].tobs; 

			//printf("snr: %i %g %g %g \n", t, snr, tobs, snr*3600/tobs);
//			snr = snr*3600/tobs;
//
//			double refSN = 1000;
//			double SNdiff =  snr/refSN;
//			double SNscale = snr-refSN;
			//double SNscale = log10(snr/refSN);//pow(freqdiff, EvoFreqExponent);

			//printf("snr: %i %g %g %g \n", t, snr, SNdiff, SNscale);
			//printf("Evo Exp %i %i %g %g \n", t, EvoFreqExponent, freqdiff, freqscale);




			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Add in any Profile Changes///////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


				for(int i =0; i < totalCoeffForMult; i++){
					ProfileModCoeffs[i]=0;	
				}				

				cpos = 0;
				epos = 0;
				fpos = 0;
				int tpos = 0;

				for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
					if(incPrecession == 0){
						for(int i =0; i < numProfileFitCoeff[c]; i++){
							ProfileModCoeffs[i+cpos] += ProfileFitCoeffs[i+fpos];
							if(((MNStruct *)globalcontext)->NProfileTimePoly > 0){ProfileModCoeffs[i+cpos] = 0;}
						}
						for(int p = 0; p < ((MNStruct *)globalcontext)->NProfileEvoPoly; p++){	
							for(int i =0; i < numEvoCoeff[c]; i++){
								ProfileModCoeffs[i+cpos] += EvoCoeffs[p][i+epos]*pow(freqscale, p+1);
							}
						}

						for(int p = 0; p < ((MNStruct *)globalcontext)->NProfileTimePoly; p++){
							double time = (((MNStruct *)globalcontext)->TimeEpochs[((MNStruct *)globalcontext)->TimeEpochIndex[nTOA]] - ((MNStruct *)globalcontext)->pulse->obsn[nTOA].sat)/365.25;
							for(int i =0; i < numTimeEvoCoeff[c]; i++){

								ProfileModCoeffs[i+cpos] += ProfTimeEvoPoly[((MNStruct *)globalcontext)->TimeEpochIndex[nTOA]][(c-1)*NProfileTimePoly + p]*pow(time, p);
							}
						}

						if(incTimeCorrProfileNoise == 1){
							double time=(double)((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat;

							for(int i = 0; i < ((MNStruct *)globalcontext)->numTimeCorrCoeff[c]; i++){
				//			printf("tpos %i %i\n", tpos, i);
								double cosAmp = TimeCorrProfileParams[tpos+2*i+0]*cos(2*M_PI*(double(0+1)/((MNStruct *)globalcontext)->Tspan)*time);	
								double sinAmp = TimeCorrProfileParams[tpos+2*i+1]*sin(2*M_PI*(double(0+1)/((MNStruct *)globalcontext)->Tspan)*time);

								ProfileModCoeffs[i+cpos] += cosAmp + sinAmp;
							}

						}

					}



					cpos += NumCoeffForMult[c];
					epos += numEvoCoeff[c];
					fpos += numProfileFitCoeff[c];	
					tpos += ((MNStruct *)globalcontext)->numTimeCorrCoeff[c];
				}



				if(totalCoeffForMult > 0){
					vector_dgemv(((MNStruct *)globalcontext)->InterpolatedShapeletsVec[InterpBin], ProfileModCoeffs,ProfileModVec,Nbins,totalCoeffForMult,'N');

					if(((MNStruct *)globalcontext)->numFitEQUAD > 0 || ((MNStruct *)globalcontext)->incDMEQUAD > 0 ){
						vector_dgemv(((MNStruct *)globalcontext)->InterpolatedJitterProfileVec[InterpBin], ProfileModCoeffs,ProfileJitterModVec,Nbins,totalCoeffForMult,'N');
					}
				}
				ModModelFlux = modelflux;



				cpos=0;
				for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
					for(int j =0; j < NumCoeffForMult[c]; j=j+2){
						ModModelFlux+=sqrt(sqrt(M_PI))*sqrt(betas[c])*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*ProfileModCoeffs[cpos+j];
					}
					cpos+= NumCoeffForMult[c];
				}

				if(dotime == 2){
					gettimeofday(&tval_after, NULL);
					timersub(&tval_after, &tval_before, &tval_resultone);
					printf("Time elapsed,  Modded Profile: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
					gettimeofday(&tval_before, NULL);
				}

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Fill Arrays with interpolated state//////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				dataflux = 0;
				for(int j = 0; j < ProfNbins; j++){
					if((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-((MNStruct *)globalcontext)->pulse->obsn[nTOA].bline >= 2*((MNStruct *)globalcontext)->pulse->obsn[nTOA].pnoise){
						dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-((MNStruct *)globalcontext)->pulse->obsn[nTOA].bline)*double((ProfileBats[t][1] - ProfileBats[t][0])/(ProfNbins-1))*60*60*24;
					}
				}


				noisemean=0;
				int numoffpulse = 0;
		                maxshape=0;
				double minshape = 0;//pow(10.0, 100);	
				int ZeroWrap = Wrap(0 + NumWholeBinInterpOffset, 0, Nbins-1);

				for(int l = 0; l < 2; l++){
					int startindex = 0;
					int stopindex = 0;
					int step = 0;

					if(l==0){
						startindex = 0; 
						stopindex = Nbins-ZeroWrap;
						step = ZeroWrap;
					}
					else{
						startindex = Nbins-ZeroWrap + BinRatio - (Nbins-ZeroWrap-1)%BinRatio - 1; 
						stopindex = Nbins;
						step = ZeroWrap-Nbins;
					}


					for(int j = startindex; j < stopindex; j+=BinRatio){



						double NewIndex = (j + NumWholeBinInterpOffset);
						int Nj =  step+j;

						double widthTerm = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj]*(LinearProfileWidth); 
				
						int cterms = 0;	
						int totcterms = 0;
						for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){	
							for(int wc = 0; wc < ((MNStruct *)globalcontext)->FitCompWidths[c]; wc++){
								widthTerm += ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj+(cterms+1)*Nbins]*LinearCompWidthTime[totcterms]*pow( (((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat - 52880)/365.25, wc);
								totcterms++;
							}
							if(((MNStruct *)globalcontext)->FitCompWidths[c]>0){ cterms++;}
						}


						for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
							for(int wc = 0; wc < ((MNStruct *)globalcontext)->FitCompPos[c]; wc++){
								widthTerm += ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj+(cterms+1)*Nbins]*LinearCompWidthTime[totcterms]*pow( (((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat - 52880)/365.25, wc);
								totcterms++;
							}
							if(((MNStruct *)globalcontext)->FitCompPos[c] > 0){ cterms ++;}
						}


						for(int ew = 0; ew < ((MNStruct *)globalcontext)->incWidthEvoTime; ew++){
							widthTerm += ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj]*LinearWidthEvoTime[ew]*pow( (((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat - 55250)/365.25, ew+1);
						}

						double evoWidthTerm = 0;//((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj]*EvoProfileWidth*freqscale;
						double SNWidthTerm = 0;// ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj]*EvoEnergyProfileWidth*SNscale;

						shapevec[j] = ((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpBin][Nj] + widthTerm + ProfileModVec[Nj] + evoWidthTerm + SNWidthTerm;
						if(((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpBin][Nj] > maxshape){ maxshape = ((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpBin][Nj];}




					for(int ev = 0; ev < incExtraProfComp; ev++){

							if(debug == 1){printf("EC: %i %g %g %g %g \n", incExtraProfComp, ExtraCompProps[ev][0], ExtraCompProps[ev][1], ExtraCompProps[ev][2], ExtraCompProps[ev][3]);}
							long double ExtraCompPos = binpos+ExtraCompProps[ev][1]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
							long double timediff = 0;
							long double bintime = ProfileBats[t][0] + j*(ProfileBats[t][1] - ProfileBats[t][0])/(ProfNbins-1);
							if(bintime  >= minpos && bintime <= maxpos){
							    timediff = bintime - ExtraCompPos;
							}
							else if(bintime < minpos){
							    timediff = FoldingPeriodDays+bintime - ExtraCompPos;
							}
							else if(bintime > maxpos){
							    timediff = bintime - FoldingPeriodDays - ExtraCompPos;
							}

							timediff=timediff*SECDAY;

							double Betatime = timediff/ExtraCompProps[ev][2];
							
							double ExtraCompDecayScale = 1;

                                                        if((int)((MNStruct *)globalcontext)->FitForExtraComp[ev][0] == 1){
								if(((MNStruct *)globalcontext)->pulse->obsn[t].bat < ExtraCompProps[ev][0]){
	                                                                ExtraCompDecayScale = 0;
								}
                                                        }

//
							if((int)((MNStruct *)globalcontext)->FitForExtraComp[ev][0] == 3){
								if(((MNStruct *)globalcontext)->pulse->obsn[t].bat < ExtraCompProps[ev][0]){
                                                                        ExtraCompDecayScale = 0;
                                                                }
                                                                else{
									ExtraCompDecayScale = exp((ExtraCompProps[ev][0]-((MNStruct *)globalcontext)->pulse->obsn[t].bat)/ExtraCompProps[ev][3+(int)((MNStruct *)globalcontext)->FitForExtraComp[ev][1]]);
								}
                                                        }
							
							TNothpl(((MNStruct *)globalcontext)->FitForExtraComp[ev][1], Betatime, ExtraCompBasis[ev]);


							double ExtraCompAmp = 0;
							for(int k =0; k < (int)((MNStruct *)globalcontext)->FitForExtraComp[ev][1]; k++){
								double Bconst=(1.0/sqrt(betas[0]))/sqrt(pow(2.0,k)*sqrt(M_PI));
								ExtraCompAmp += ExtraCompBasis[ev][k]*Bconst*ExtraCompProps[ev][3+k];

							
							}

							double ExtraCompSignal = ExtraCompAmp*exp(-0.5*Betatime*Betatime)*ExtraCompDecayScale;

							shapevec[j] += ExtraCompSignal;
					    }



						Mcounter = 0;
						for(int q = 0; q < ProfileBaselineTerms; q++){
							M[j/BinRatio+Mcounter*ProfNbins] = pow(double(j)/Nbins, q);
							Mcounter++;
						}
					
						M[j/BinRatio + Mcounter*ProfNbins] = shapevec[j];
						
						Mcounter++;


						for(int q = 0; q < ProfileNoiseCoeff; q++){

							M[j/BinRatio + Mcounter*ProfNbins] =     cos(2*M_PI*(double(q+1)/Nbins)*j);
							M[j/BinRatio + (Mcounter+1)*ProfNbins] = sin(2*M_PI*(double(q+1)/Nbins)*j);

							Mcounter += 2;
						}

				
						if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
							M[j/BinRatio + Mcounter*ProfNbins] = ((MNStruct *)globalcontext)->InterpolatedJitterProfile[InterpBin][Nj] + ProfileJitterModVec[Nj];

							Mcounter++;
						}		

						if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
							double DMKappa = 2.410*pow(10.0,-16);
							double DMScale = 1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[t].freqSSB,2));
							M[j/BinRatio + Mcounter*ProfNbins] = (((MNStruct *)globalcontext)->InterpolatedJitterProfile[InterpBin][Nj] + ProfileJitterModVec[Nj])*DMScale;
							Mcounter++;
						}	


						if(((MNStruct *)globalcontext)->incWidthJitter > 0){
							M[j/BinRatio + Mcounter*ProfNbins] = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj];
							Mcounter++;
						}

						int stocpos=0;
						int savepos=0;
						for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
							for(int k = 0; k < numProfileStocCoeff[c]; k++){
							    M[j/BinRatio + (Mcounter+k+stocpos)*ProfNbins] = ((MNStruct *)globalcontext)->InterpolatedShapeletsVec[InterpBin][Nj + (k+savepos)*Nbins];
								if(incPrecession == 2){  M[j/BinRatio + (Mcounter+k+stocpos)*ProfNbins] = PrecBasis[j + c*ProfNbins];}
							}
							
							stocpos+=numProfileStocCoeff[c];
							savepos+=numShapeToSave[c];

						}	
					}
				}
				maxshape -=minshape;

			} //incPrecession == 0



			if(dotime == 4){
				gettimeofday(&tval_after, NULL);
				timersub(&tval_after, &tval_before, &tval_resultone);
				printf("Time elapsed,  Start of Prec: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
				gettimeofday(&tval_before, NULL);
			}
			if(incPrecession == 2){


				double rootrootpi = sqrt(sqrt(M_PI));
				for(int j = 0; j < ProfNbins; j++){

					double Bconst= 0;
					double CompSignal = 0;


				//	double JitterBconst=0;
				//	double JitterBasis = 0;
				//	double JitterSignal = 0;


					double CompWidth = PrecessionParams[0];
					long double CompPos = binpos;
					double CompAmp = 1;

					long double bintime = ProfileBats[t][0]+j*(ProfileBats[t][1] - ProfileBats[t][0])/(ProfNbins-1);

					if(bintime < minpos){
						bintime += FoldingPeriodDays;
					}
					else if(bintime > maxpos){
						bintime -= FoldingPeriodDays;
					}

					long double timediff=(bintime-CompPos)*SECDAY;

					double Betatime = timediff/CompWidth;
					//	if(nTOA > 400){printf("Beta time 0 %i %g %g \n", j, Betatime, exp(-0.5*Betatime*Betatime)*(1.0/sqrt(CompWidth))/sqrt(sqrt(M_PI)));}

					if(Betatime*Betatime < 25){
						Bconst= (1.0/sqrt(CompWidth))/rootrootpi;
						CompSignal = exp(-0.5*Betatime*Betatime)*Bconst;


				//		JitterBconst = (1.0/sqrt(CompWidth))/sqrt(2.0*sqrt(M_PI));
				//		JitterBasis = 2*Betatime*JitterBconst*exp(-0.5*Betatime*Betatime);
				//		JitterSignal = (1.0/sqrt(2.0))*(-1.0*JitterBasis)/CompWidth
					}

					//PrecBasis[j + 0*ProfNbins] = CompSignal;
					for(int c = 1; c < ((MNStruct *)globalcontext)->numProfComponents; c++){


						CompWidth = PrecessionParams[c];
						CompPos = binpos+PrecessionParams[((MNStruct *)globalcontext)->numProfComponents+c-1];
						CompAmp = 1;
						double OneCompSignal = 0;
						double CompSignalNoAmp = 0;

						timediff=(bintime-CompPos)*SECDAY;
						Betatime = timediff/CompWidth;
						if(Betatime*Betatime < 25){


			//				if(((MNStruct *)globalcontext)->FitPrecAmps >= 0){
							//double refMJD = 54250;
							double MJD = (((MNStruct *)globalcontext)->PrecRefMJDs[c] - ((MNStruct *)globalcontext)->pulse->obsn[nTOA].sat)/365.25;
							double powMJD = MJD;
							//get 0th Amp term first
							double logAmp = PrecessionParams[((MNStruct *)globalcontext)->numProfComponents+(((MNStruct *)globalcontext)->numProfComponents-1)+(((MNStruct *)globalcontext)->FitPrecAmps+1)*(c-1)+0];
							double FitAmp = logAmp;

							for(int a = 1; a <= ((MNStruct *)globalcontext)->FitPrecAmps; a++){
								logAmp = PrecessionParams[((MNStruct *)globalcontext)->numProfComponents+(((MNStruct *)globalcontext)->numProfComponents-1)+(((MNStruct *)globalcontext)->FitPrecAmps+1)*(c-1)+a];

								FitAmp +=  logAmp*powMJD;
//
//									if(t==0){printf("Amps: %i %i %i %g %g %g %g \n", c, j, a, pow((53900-((MNStruct *)globalcontext)->pulse->obsn[nTOA].sat)/365.25, a), powMJD, logAmp, FitAmp);}
		
								powMJD *= MJD;

							}

							FitAmp = pow(10.0, FitAmp);
							CompAmp = FitAmp;
			//				}
							Bconst= (1.0/sqrt(CompWidth))/rootrootpi;
							CompSignalNoAmp = exp(-0.5*Betatime*Betatime)*Bconst;
							OneCompSignal = CompAmp*CompSignalNoAmp;

//								if(j < 10){printf("Beta time %i %i %g %g\n", c, j, Betatime, OneCompSignal);}
							CompSignal += OneCompSignal;
//								if(t==0){printf("One COpm: %i %i %g %g %g \n", c, j, CompAmp, OneCompSignal, Bconst);}
							//JitterBconst= (1.0/sqrt(CompWidth))/sqrt(2.0*sqrt(M_PI));
							//JitterBasis = 2*Betatime*JitterBconst*exp(-0.5*Betatime*Betatime);
							//JitterSignal += CompAmp*(1.0/sqrt(2.0))*(-1.0*JitterBasis)/CompWidth;
						}

//							PrecBasis[j + c*ProfNbins] = OneCompSignal;
						for(int k = 0; k < numProfileStocCoeff[c]; k++){							
							M[j + (1+c)*ProfNbins] = CompSignalNoAmp;
						}

					}
//						if(j < 10){printf("comp: %i %i %g %g \n", t, j, CompSignal, shapevec[j]);}
//						shapevec[j] = CompSignal;

//						Mcounter = 0;
					M[j] = 1;
//						Mcounter++;
				
					M[j + ProfNbins] = CompSignal;

//						if(t==0){printf("Sig: %i %g \n", j, CompSignal);}
					if(CompSignal > maxshape){ maxshape = CompSignal;}
					
					//Mcounter++;
				//	int stocpos = 0;
				//	for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
				//		for(int k = 0; k < numProfileStocCoeff[c]; k++){
				//			M[j + (Mcounter+k+stocpos)*ProfNbins] = PrecBasis[j + c*ProfNbins];
				//		}
						
				//		stocpos+=numProfileStocCoeff[c];

				//	}	
				}

			}

			if(dotime == 4){
				gettimeofday(&tval_after, NULL);
				timersub(&tval_after, &tval_before, &tval_resultone);
				printf("Time elapsed,  End of Prec %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
				gettimeofday(&tval_before, NULL);
			}

			if(incPrecession == 2  && ((MNStruct *)globalcontext)->FitPrecAmps == -1){

				double *D = new double[ProfNbins];
				double *PP = new double[(((MNStruct *)globalcontext)->numProfComponents+1)*(((MNStruct *)globalcontext)->numProfComponents+1)];	
				double *PD = new double[((MNStruct *)globalcontext)->numProfComponents+1];	

				for(int b = 0; b < ProfNbins; b++){
					D[b] = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][b][1];
				}

				((MNStruct *)globalcontext)->PrecBasis = PrecBasis;
				((MNStruct *)globalcontext)->PrecD = D;
				((MNStruct *)globalcontext)->PrecN = ((MNStruct *)globalcontext)->pulse->obsn[nTOA].snr;


				double *Phys=new double[((MNStruct *)globalcontext)->numProfComponents];
				
				SmallNelderMeadOptimum(((MNStruct *)globalcontext)->numProfComponents, Phys);
				double sfac = pow(10.0, Phys[0]);
				for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
					Phys[c] = pow(10.0, Phys[c]);
					Phys[c] = Phys[c]/sfac;
				}
				

				double *ML = new double[ProfNbins];
				vector_dgemv(PrecBasis, Phys, ML, ProfNbins, ((MNStruct *)globalcontext)->numProfComponents, 'N');

				for(int b = 0; b < ProfNbins; b++){
					M[b + 1*ProfNbins] = ML[b];
				}

				delete[] Phys;
				delete[] D;
				delete[] ML;
				
			}



			if(dotime == 2){
				gettimeofday(&tval_after, NULL);
				timersub(&tval_after, &tval_before, &tval_resultone);
				printf("Time elapsed,  Filled Arrays: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
				gettimeofday(&tval_before, NULL);
			}

//				if(debug == 1){sleep(5);}


			int doscatter = 0;

			if(((MNStruct *)globalcontext)->incProfileScatter == 1){

				fftw_plan plan;
				fftw_complex *cp;
				fftw_complex *cpbf;

				double *pbf = new double[Nbins]();


				double ScatterScale = pow( ((MNStruct *)globalcontext)->pulse->obsn[t].freqSSB, 4)/pow(10.0, 9.0*4.0);
				double STime = ProfileScatter/ScatterScale;



				if(((MNStruct *)globalcontext)->ScatterPBF == 0){ 
					pbf[0] = 1;
				}
				else if(((MNStruct *)globalcontext)->ScatterPBF == 1){
					double P = ((MNStruct *)globalcontext)->ReferencePeriod;
					for (int i = 0; i < Nbins; i++ ){
						double time = (P*i)/Nbins;
						pbf[i] = exp(-time/STime); 
					}
				}
				else if(((MNStruct *)globalcontext)->ScatterPBF == 2){

					double P = ((MNStruct *)globalcontext)->ReferencePeriod;
					for (int i = 1; i < Nbins; i++ ){
						double time = (P*i)/Nbins;
						pbf[i] = sqrt(M_PI*STime/(4*pow(time, 3)))*exp(-M_PI*M_PI*STime/(16*time)); 

					}
				}
				else if(((MNStruct *)globalcontext)->ScatterPBF == 3){ 

					double P = ((MNStruct *)globalcontext)->ReferencePeriod;
					for (int i = 1; i < Nbins; i++ ){
						double time = (P*i)/Nbins;
						pbf[i] = sqrt(pow(M_PI,5)*pow(STime,3)/(8*pow(time, 5)))*exp(-M_PI*M_PI*STime/(4*time)); 
					}
				}
				else if(((MNStruct *)globalcontext)->ScatterPBF == 4){ 

					double STime2=ProfileScatter2/ScatterScale;

					double P = ((MNStruct *)globalcontext)->ReferencePeriod;
					for (int i = 0; i < Nbins; i++ ){
						double time = (P*i)/Nbins;
						pbf[i] = exp(-time/STime)*exp(-time/STime2); 
					}
				}
				else if(((MNStruct *)globalcontext)->ScatterPBF == 5){


                                        double P = ((MNStruct *)globalcontext)->ReferencePeriod;
                                        for (int i = 0; i < int(Nbins*ProfileScatterCut); i++ ){
				//		printf("%i %i %g \n", t, i, ((P*i)/Nbins)/STime);
                                                double time = (P*i)/Nbins;
                                                pbf[i] = exp(-time/STime);
                                        }
                                }
				else if(((MNStruct *)globalcontext)->ScatterPBF == 6){ 

					double STime2=ProfileScatter2/ScatterScale;

					double P = ((MNStruct *)globalcontext)->ReferencePeriod;
					for (int i = 0; i < Nbins; i++ ){
						double time = (P*i)/Nbins;
						double arg=0.5*time*(1/STime - 1/STime2);

						if(arg > 700){
							arg=700;
						}
						if(arg < -700){
							arg=-700;
						}

						double bfunc = 0;
						gsl_sf_result result;
						int status = gsl_sf_bessel_I0_e(arg, &result);

						if(status == GSL_SUCCESS){
						//	printf("success: %g %g\n", t,i,arg, exp(-0.5*time*(1/STime + 1/STime2))*result.val);
							bfunc = result.val;
						}

						pbf[i] = exp(-0.5*time*(1/STime + 1/STime2))*bfunc; //exp(-0.5*time*(1/STime + 1/STime2) + fabs(arg))*bfunc; 

					}
				}
				//printf("Scatter PBF %i %g %i \n", ((MNStruct *)globalcontext)->ScatterPBF, ProfileScatterCut, int(double(Nbins)*ProfileScatterCut));

				cp = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * Nbins);
				cpbf = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * Nbins);

				plan = fftw_plan_dft_r2c_1d(Nbins, shapevec, cp, FFTW_ESTIMATE); 
				fftw_execute(plan);

				plan = fftw_plan_dft_r2c_1d(Nbins, pbf, cpbf, FFTW_ESTIMATE); 
				fftw_execute(plan);

				double oldpower = 0;

				for (int i = 0; i < Nbins; i++ ){
				
					oldpower += shapevec[i]*shapevec[i];

					double real =  cp[i][0]*cpbf[i][0] - cp[i][1]*cpbf[i][1];
					double imag =  cp[i][0]*cpbf[i][1] + cp[i][1]*cpbf[i][0];

					cp[i][0] =  real;
					cp[i][1] =  imag;

				}



//				double *dp2 = new double[Nbins]();

				fftw_plan plan_back = fftw_plan_dft_c2r_1d(Nbins, cp, shapevec, FFTW_ESTIMATE);

				fftw_execute ( plan_back );

//				printf ( "\n" );
//				printf ( "  Recovered input data divided by N:\n" );
//				printf ( "\n" );

				double newmax = 0;
				double newpower = 0;
				double newmean = 0;
				for (int i = 0; i < Nbins; i++ ){
					//newpower += shapevec[i]*shapevec[i];
					newmean += shapevec[i];
					if(shapevec[i] > newmax)newmax = shapevec[i];
					
				}
				newmean=newmean/Nbins;
				for (int i = 0; i < Nbins; i++ ){
					newpower += (shapevec[i]-newmean)*(shapevec[i]-newmean);
				}
				newpower = sqrt(newpower/Nbins);
				for (int i = 0; i < Nbins; i++ ){
					M[i + ProfNbins] = (shapevec[i]-newmean)/newpower;
				}

				  fftw_destroy_plan ( plan );
				  fftw_destroy_plan ( plan_back );

				  fftw_free ( cp );
				  fftw_free ( cpbf );


			}

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Get Noise Level//////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				double newmax = 0;
				double newpower = 0;
				double newmean = 0;
				for (int i = 0; i < Nbins; i++ ){
					newmean += shapevec[i];				
				}
				newmean=newmean/Nbins;
				for (int i = 0; i < Nbins; i++ ){
					newpower += (shapevec[i]-newmean)*(shapevec[i]-newmean);
				}
				newpower = sqrt(newpower/Nbins);
				for (int i = 0; i < Nbins; i++ ){
					M[i + ProfNbins] = (shapevec[i]-newmean)/newpower;
				}


				dataflux = 0;


				if(((MNStruct *)globalcontext)->ProfileNoiseMethod == 1){
					MLSigma=((MNStruct *)globalcontext)->pulse->obsn[nTOA].snr;
				}
				if(((MNStruct *)globalcontext)->ProfileNoiseMethod == 2){
                                        MLSigma=((MNStruct *)globalcontext)->pulse->obsn[nTOA].pnoise;
                                }

//				printf("MLSigma: %i %g %g \n", nTOA, MLSigma, ((MNStruct *)globalcontext)->pulse->obsn[nTOA].snr);


				
				MLSigma = MLSigma*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]]*((MNStruct *)globalcontext)->MLProfileNoise[ep][0];

				dataflux = 0;
				for(int j = 0; j < ProfNbins; j++){
					if(M[j/BinRatio + ProfileBaselineTerms*ProfNbins] >= maxshape*OPLevel){
						dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-((MNStruct *)globalcontext)->pulse->obsn[nTOA].bline)*double((ProfileBats[t][1] - ProfileBats[t][0])/(ProfNbins-1))*60*60*24;
					}
					//M[j/BinRatio + ProfileBaselineTerms*ProfNbins] /= maxshape;
				}
				maxamp = dataflux/(ModModelFlux/maxshape);

				if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g \n", MLSigma, dataflux, ModModelFlux, maxamp);


				if(dotime == 2){
					gettimeofday(&tval_after, NULL);
					timersub(&tval_after, &tval_before, &tval_resultone);
					printf("Time elapsed,  Get Noise: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
					gettimeofday(&tval_before, NULL);
				}

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Rescale Basis Vectors////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


				Mcounter = 1+ProfileBaselineTerms+ProfileNoiseCoeff*2;
				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
					for(int j =0; j < ProfNbins; j++){
						M[j + Mcounter*ProfNbins] = M[j + Mcounter*ProfNbins]*dataflux/ModModelFlux;
					}			
					Mcounter++;
				}
				if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
					for(int j =0; j < ProfNbins; j++){
						M[j + Mcounter*ProfNbins] = M[j + Mcounter*ProfNbins]*dataflux/ModModelFlux;
					}			
					Mcounter++;
				}
				if(((MNStruct *)globalcontext)->incWidthJitter > 0){
					for(int j =0; j < ProfNbins; j++){
						M[j + Mcounter*ProfNbins] = M[j + Mcounter*ProfNbins]*dataflux/ModModelFlux;
					}
					Mcounter++;
				}
				for(int k = 0; k < totalshapestoccoeff; k++){
					for(int j =0; j < ProfNbins; j++){
				   		M[j + (Mcounter+k)*ProfNbins] = M[j + (Mcounter+k)*ProfNbins]*dataflux;
					}
				}


	
		    double *NDiffVec = new double[ProfNbins];

	
		///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////

		if(incPrecession == 0){
			      
			Chisq = 0;
			detN = 0;
			double OffPulsedetN = log(MLSigma*MLSigma);
			double OneDetN = 0;
			double logconvfactor = 1.0/log2(exp(1));

	
	 
			double highfreqnoise = HighFreqStoc1;
			double flatnoise = (HighFreqStoc2*maxamp*maxshape)*(HighFreqStoc2*maxamp*maxshape);
			

			for(int i =0; i < ProfNbins; i++){
			  	Mcounter=2;
				double noise = MLSigma*MLSigma;

				OneDetN=OffPulsedetN;
				if(shapevec[i*BinRatio] > maxshape*OPLevel && ((MNStruct *)globalcontext)->incHighFreqStoc > 0){
					noise +=  flatnoise + (highfreqnoise*maxamp*shapevec[i*BinRatio])*(highfreqnoise*maxamp*shapevec[i*BinRatio]);
					OneDetN=log2(noise)*logconvfactor;
				}

				detN += OneDetN;
				noise=1.0/noise;
				
				double datadiff =  ((MNStruct *)globalcontext)->ProfileData[nTOA][i][1];
				//printf("Data: %g %i %g %g %g\n", Cube[0], i, datadiff, 1.0/sqrt(noise), shapevec[i]);
				NDiffVec[i] = datadiff*noise;

				Chisq += datadiff*NDiffVec[i];
	

				
				for(int j = 0; j < Msize; j++){
//					printf("M: %i %i %g \n", i, j, M[i + j*ProfNbins]);
					NM[i + j*ProfNbins] = M[i + j*ProfNbins]*noise;
				}

				

			}

			
			vector_dgemm(M, NM , MNM, ProfNbins, Msize,ProfNbins, Msize, 'T', 'N');

		}
		if(incPrecession == 2){

			Mcounter = 2;
			double noise = MLSigma*MLSigma;
			detN = log(MLSigma*MLSigma)*ProfNbins;

			noise = 1.0/noise;

			Chisq = ((MNStruct *)globalcontext)->pulse->obsn[nTOA].chisq*noise;
			for(int i =0; i < ProfNbins; i++){
				NDiffVec[i] = ((MNStruct *)globalcontext)->ProfileData[nTOA][i][1]*noise;
				//if(nTOA> 400){printf("NDiff: %i %i %g %g \n", nTOA, i, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1], noise);}
				//for(int j = 0; j < Msize; j++){
				//	if(nTOA> 400){printf("NDiff: %i %i %i %g \n", nTOA, i, j, M[i + j*ProfNbins]);}
			//		NM[i + j*ProfNbins] = M[i + j*ProfNbins]*noise;
				//}

			}

			
			vector_dgemm(M, M , MNM, ProfNbins, Msize,ProfNbins, Msize, 'T', 'N');


			for(int i = 0; i < Msize*Msize; i++){
//				if(nTOA> 400){printf("NDiff: %i %i %g \n", nTOA, i,MNM[i]);}
				MNM[i] *= noise;

			}
			//delete[] M;

		}

			if(dotime == 4){
				gettimeofday(&tval_after, NULL);
				timersub(&tval_after, &tval_before, &tval_resultone);
				printf("Time elapsed,  Up to LA: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
				gettimeofday(&tval_before, NULL);
			}



			double *dNM = new double[Msize];
			double *TempdNM = new double[Msize];
			vector_dgemv(M,NDiffVec,dNM,ProfNbins,Msize,'T');

		
			if(((MNStruct *)globalcontext)->incWideBandNoise == 1){

				int ppterms = PerEpochSize;
				if(debug==1){printf("Filling EpochMNM: %i %i %i \n", ppterms, Msize, EpochMsize);}
				for(int j = 0; j < ppterms; j++){
					EpochdNM[ppterms*ch+j] = dNM[j];
					for(int k = 0; k < ppterms; k++){
						if(debug==1){if(j==k && ep == 0){printf("Filling EpochMNM diag: %i %i %i %g \n", j,k,ppterms*ch+j + (ppterms*ch+k)*EpochMsize, MNM[j + k*Msize]);}}
						EpochMNM[ppterms*ch+j + (ppterms*ch+k)*EpochMsize] = MNM[j + k*Msize];
					}
				}


				for(int j = 0; j < Msize-ppterms; j++){

					EpochdNM[ppterms*NChanInEpoch+j] += dNM[ppterms+j];

				//	for(int k = 0; k < ppterms; k++){

						for(int q = 0; q < ppterms; q++){
							EpochMNM[ppterms*ch+q + (ppterms*NChanInEpoch+j)*EpochMsize] = MNM[q + (ppterms+j)*Msize];
							//EpochMNM[ppterms*ch+1 + (ppterms*NChanInEpoch+j)*EpochMsize] = MNM[1 + (ppterms+j)*Msize];

							EpochMNM[ppterms*NChanInEpoch+j + (ppterms*ch+q)*EpochMsize] = MNM[ppterms+j + q*Msize];
							//EpochMNM[ppterms*NChanInEpoch+j + (ppterms*ch+1)*EpochMsize] = MNM[ppterms+j + 1*Msize];
						}
				//	}

					for(int k = 0; k < Msize-ppterms; k++){

						EpochMNM[ppterms*NChanInEpoch+j + (ppterms*NChanInEpoch+k)*EpochMsize] += MNM[ppterms+j + (ppterms+k)*Msize];
					}
				}
			}


	

			double Margindet = 0;
			double StocProfDet = 0;
			double JitterDet = 0;
			if(((MNStruct *)globalcontext)->incWideBandNoise == 0){


				Mcounter=1+ProfileBaselineTerms;

		

				if(((MNStruct *)globalcontext)->incProfileNoise == 1 || ((MNStruct *)globalcontext)->incProfileNoise == 2){
					for(int q = 0; q < ProfileNoiseCoeff; q++){
						if(debug == 1){if(ep==0){printf("PN NBW %i %i %i %i %g %g \n", ep, ch, Mcounter, q, MNM[Mcounter + Mcounter*Msize], MNM[Mcounter+1 + (Mcounter+1)*Msize]);}}
						double profnoise = ProfileNoiseAmps[q];
						JitterDet += 2*log(profnoise);
						MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
						Mcounter++;
						MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
						Mcounter++;
					}
				}

				if(((MNStruct *)globalcontext)->incProfileNoise == 3 || ((MNStruct *)globalcontext)->incProfileNoise == 4){
					for(int q = 0; q < ProfileNoiseCoeff; q++){
						if(debug == 1){if(ep==0){printf("PN BW %i %i %i %i %g %g \n", ep, ch, Mcounter, q, MNM[Mcounter + Mcounter*Msize], MNM[Mcounter+1 + (Mcounter+1)*Msize]);}}
						double profnoise = ProfileNoiseAmps[q]*((MNStruct *)globalcontext)->MLProfileNoise[ep][q+1];
						JitterDet += 2*log(profnoise);
						MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
						Mcounter++;
						MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
						Mcounter++;
					}
				}

				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){

					double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
					profnoise = profnoise*profnoise;
				    	JitterDet += log(profnoise);
				    	MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
					Mcounter++;
				}

				if(((MNStruct *)globalcontext)->incDMEQUAD > 0){

					double profnoise = DMEQUAD;
					profnoise = profnoise*profnoise;
				    	JitterDet += log(profnoise);
				    	MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
					Mcounter++;
				}


				if(((MNStruct *)globalcontext)->incWidthJitter > 0){

					double profnoise = WidthJitter;
					profnoise = profnoise*profnoise;
				    	JitterDet += log(profnoise);
				    	MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
					Mcounter++;
				}
				
				for(int j = 0; j < totalshapestoccoeff; j++){
					StocProfDet += log(StocProfCoeffs[j]);
					MNM[Mcounter+j + (Mcounter+j)*Msize] +=  1.0/StocProfCoeffs[j];
				}

				double *MNM2 = new double[Msize*Msize]();
				double *TempdNM2 = new double[Msize];
				for(int i =0; i < Msize; i++){
					TempdNM[i] = dNM[i];
					TempdNM2[i] = dNM[i];

				}
				for(int i =0; i < Msize*Msize; i++){
					MNM2[i] = MNM[i];
					//if(nTOA == 0){printf("MNM: %i %i %g\n", nTOA, i,MNM[i]);} 
				}

				int info = 0;
				int robust = 2;


				info=0;
				double CholDet = 0;
				double CholLike = 0;
				Marginlike = 0;
				if(robust == 1 || robust == 3){
					vector_dpotrfInfo(MNM2, Msize, CholDet, info);
					vector_dpotrsInfo(MNM2, TempdNM2, Msize, info);
					Margindet = CholDet;

					for(int i =0; i < Msize; i++){
        	                                CholLike += TempdNM2[i]*dNM[i];
	                                }
					Marginlike = CholLike;

				}
				info = 0;					    
				if(robust == 2 || robust ==3 ){	
			
					vector_TNqrsolve(MNM, dNM, TempdNM, Msize, Margindet, info);

				 	Marginlike = 0;
					for(int i =0; i < Msize; i++){
						
						Marginlike += TempdNM[i]*dNM[i];

						//if(nTOA == 1){printf("TdNM: %i %i %.15g %.15g %.15g %.15g\n", nTOA, i, dNM[i], TempdNM[i], TempdNM[i]*dNM[i], Marginlike);}
					}

				}


				logMargindet = Margindet;

				delete[] MNM2;
				delete[] TempdNM2;
				
				if(robust == 3 &&  fabs((CholDet-CholLike) - (Margindet-Marginlike)) > 0.05){
					//printf("LA Unstable ! %g %g \n", CholDet-CholLike, Margindet-Marginlike); 
					badlike = 1;
				}
			}

			profilelike = -0.5*(detN + Chisq + logMargindet + StocProfDet + JitterDet - Marginlike);
			if(badlike == 1){profilelike = -pow(10.0, 100);}
			//lnew += profilelike;
			if(debug == 1)printf("Like: %i %.15g %.15g %.15g %.15g %.15g\n", nTOA,lnew, detN, Chisq, logMargindet, Marginlike);
			//if(isnan(profilelike)){printf("Like: %i %.15g %.15g %.15g %.15g %.15g %i\n", nTOA,detN, Chisq, logMargindet, Marginlike, MLSigma, MLSigmaCountCheck); profilelike= -pow(10.0, 100);}
			EpochChisq+=Chisq;
			EpochdetN+=detN;
			EpochLike+=profilelike;

			delete[] shapevec;
			//delete[] Jittervec;
			delete[] ProfileModVec;
			delete[] ProfileJitterModVec;
			delete[] NDiffVec;
			delete[] dNM;
			delete[] TempdNM;
			//delete[] Betatimes;

			if(dotime == 4){
				gettimeofday(&tval_after, NULL);
				timersub(&tval_after, &tval_before, &tval_resultone);
				printf("Time elapsed,  End of Epoch: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
			}

			t++;
		}


///////////////////////////////////////////

		if(dotime == 2){
			gettimeofday(&tval_before, NULL);
		}

		if(((MNStruct *)globalcontext)->incWideBandNoise == 0){
                        delete[] M;
                        delete[] NM;


			lnew += EpochLike;
		}
		if(((MNStruct *)globalcontext)->incWideBandNoise == 1){

			for(int i =0; i < EpochMsize; i++){
				EpochTempdNM[i] = EpochdNM[i];
			}

			int EpochMcount=0;
			if(((MNStruct *)globalcontext)->incProfileNoise == 0){EpochMcount = (1+ProfileBaselineTerms)*NChanInEpoch;}

			double BandJitterDet = 0;

			if(((MNStruct *)globalcontext)->incProfileNoise == 1 || ((MNStruct *)globalcontext)->incProfileNoise == 2){
				for(int ch = 0; ch < ((MNStruct *)globalcontext)->numChanPerInt[ep]; ch++){
					
					EpochMcount += 1+ProfileBaselineTerms;
					for(int q = 0; q < ProfileNoiseCoeff; q++){
						if(debug == 1){printf("PN BW %i %i %i %i %g %g \n", ep, ch, EpochMcount, q, EpochMNM[EpochMcount + EpochMcount*EpochMsize], EpochMNM[EpochMcount+1 + (EpochMcount+1)*EpochMsize]);}
						double profnoise = ProfileNoiseAmps[q];
						BandJitterDet += 2*log(profnoise);
						EpochMNM[EpochMcount + EpochMcount*EpochMsize] += 1.0/profnoise;
						EpochMcount++;
						EpochMNM[EpochMcount + EpochMcount*EpochMsize] += 1.0/profnoise;
						EpochMcount++;
					}
				}
			}
			if(((MNStruct *)globalcontext)->incProfileNoise == 3 || ((MNStruct *)globalcontext)->incProfileNoise == 4){
				for(int ch = 0; ch < ((MNStruct *)globalcontext)->numChanPerInt[ep]; ch++){
					
					EpochMcount += 1+ProfileBaselineTerms;
					for(int q = 0; q < ProfileNoiseCoeff; q++){
						if(debug == 1){printf("PN BW %i %i %i %i %g %g \n", ep, ch, EpochMcount, q, EpochMNM[EpochMcount + EpochMcount*EpochMsize], EpochMNM[EpochMcount+1 + (EpochMcount+1)*EpochMsize]);}
						double profnoise = ((MNStruct *)globalcontext)->MLProfileNoise[ep][q+1];
						BandJitterDet += 2*log(profnoise);
						EpochMNM[EpochMcount + EpochMcount*EpochMsize] += 1.0/profnoise;
						EpochMcount++;
						EpochMNM[EpochMcount + EpochMcount*EpochMsize] += 1.0/profnoise;
						EpochMcount++;
					}
				}
			}

			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){

				double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[0]];
				profnoise = profnoise*profnoise;
			    	BandJitterDet += log(profnoise);

				EpochMNM[EpochMcount + (EpochMcount)*EpochMsize] += 1.0/profnoise;
				EpochMcount++;
			}

			if(((MNStruct *)globalcontext)->incDMEQUAD > 0){

				double profnoise = DMEQUAD;
				profnoise = profnoise*profnoise;
			    	BandJitterDet += log(profnoise);

				EpochMNM[EpochMcount + (EpochMcount)*EpochMsize] += 1.0/profnoise;
				EpochMcount++;
			}

			if(((MNStruct *)globalcontext)->incWidthJitter > 0){

				double profnoise = WidthJitter;
				profnoise = profnoise*profnoise;
			    	BandJitterDet += log(profnoise);

				EpochMNM[EpochMcount + (EpochMcount)*EpochMsize] += 1.0/profnoise;
				EpochMcount++;
			}

			double BandStocProfDet = 0;
			for(int j = 0; j < totalshapestoccoeff; j++){
				BandStocProfDet += log(StocProfCoeffs[j]);
				EpochMNM[EpochMcount+j + (EpochMcount+j)*EpochMsize] += 1.0/StocProfCoeffs[j];
			}

			int Epochinfo=0;
			double EpochMargindet = 0;
			vector_dpotrfInfo(EpochMNM, EpochMsize, EpochMargindet, Epochinfo);
			vector_dpotrsInfo(EpochMNM, EpochTempdNM, EpochMsize, Epochinfo);
				    
			double EpochMarginlike = 0;
			for(int i =0; i < EpochMsize; i++){
				EpochMarginlike += EpochTempdNM[i]*EpochdNM[i];
			}

			double EpochProfileLike = -0.5*(EpochdetN + EpochChisq + EpochMargindet - EpochMarginlike + BandJitterDet + BandStocProfDet);
			lnew += EpochProfileLike;
			if(debug == 1){printf("BWLike %i %g %g %g %g %g %g \n", ep,EpochdetN ,EpochChisq ,EpochMargindet , EpochMarginlike, BandJitterDet, BandStocProfDet );}



//			printf("Compare likes: %i %g %g %g %g %g %g\n", ep, EpochProfileLike, BandJitterDet, EpochMarginlike, EpochMargindet, EpochChisq, EpochdetN);
			//printf("like %i %g \n", t, EpochProfileLike);

			//for (int j = 0; j < EpochMsize; j++){
			//    delete[] EpochMNM[j];
			//}
			delete[] EpochMNM;
			delete[] EpochdNM;
			delete[] EpochTempdNM;
			delete[] M;
			delete[] NM;

			if(dotime == 2){
				gettimeofday(&tval_after, NULL);
				timersub(&tval_after, &tval_before, &tval_resultone);
				printf("Time elapsed,  End of Band: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
			}
		}

////////////////////////////////////////////
	}
	 

	if(dotime == 1){
		gettimeofday(&tval_after, NULL);
		timersub(&tval_after, &tval_before, &tval_resultone);
		printf("Time elapsed,  End of loop: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
		gettimeofday(&tval_before, NULL);

	}

	delete[] MNM;
	 

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] numcoeff;
	delete[] NumCoeffForMult;
	delete[] numShapeToSave;
	delete[] betas;

	for(int j =0; j< ((MNStruct *)globalcontext)->NProfileEvoPoly; j++){
	    delete[] EvoCoeffs[j];
	}
	delete[] EvoCoeffs;
	delete[] ProfileNoiseAmps;
	if(((MNStruct *)globalcontext)->incDM > 0 || ((MNStruct *)globalcontext)->yearlyDM == 1){
		delete[] SignalVec;
	}
	
	if(((MNStruct *)globalcontext)->incRED > 0 ){
		delete[] RedSignalVec;
	}
	
	if(((MNStruct *)globalcontext)->incDMShapeEvent != 0){
	delete[] DMShapeCoeffParams;	
	}
	for(int ev = 0; ev < incExtraProfComp; ev++){
                delete[] ExtraCompProps[ev];
		delete[] ExtraCompBasis[ev];
	}
	if(incExtraProfComp > 0){
		delete[] ExtraCompProps;
		delete[] ExtraCompBasis;
	}

	if(incPrecession > 0){
		delete[] PrecBasis;
		delete[] PrecessionParams;
	}
	if(incTimeCorrProfileNoise == 1){
		delete[] TimeCorrProfileParams; 
	}

	if(((MNStruct *)globalcontext)->NumFitCompWidths+((MNStruct *)globalcontext)->NumFitCompPos > 0){
		delete[] LinearCompWidthTime;
		
		
        }

	lnew += uniformpriorterm - 0.5*(FreqLike + FreqDet) + JDet;
	//printf("Cube: %g %g %g %g %g \n", lnew, FreqLike, FreqDet, Cube[1], Cube[0]);
	if(debug == 1)printf("End Like: %.10g \n", lnew);
	//printf("End Like: %.10g \n", lnew);
	//}


	if(dotime == 2){
		gettimeofday(&tval_after, NULL);
		timersub(&tval_after, &tval_before, &tval_resultone);
		printf("Time elapsed,  End of Like: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
		gettimeofday(&tval_before, NULL);

	}
     //	gettimeofday(&tval_after, NULL);
       //	timersub(&tval_after, &tval_before, &tval_resulttwo);
      // 	printf("Time elapsed: %ld.%06ld %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec, (long int)tval_resulttwo.tv_sec, (long int)tval_resulttwo.tv_usec);
	//}
	//sleep(5);
	return lnew;
	
	//return bluff;
	//sleep(5);

}

*/

/*
void  WriteProfileDomainLike(std::string longname, int &ndim){
  
  	int writeprofiles=1;
	int writeascii=1;

	int profiledimstart=0;
        double *Cube = new double[ndim];
        int number_of_lines = 0;

        std::ifstream checkfile;
	std::string checkname;
	if(((MNStruct *)globalcontext)->sampler == 0){
	        checkname = longname+"phys_live.points";
	}
        if(((MNStruct *)globalcontext)->sampler == 1){
                checkname = longname+"_phys_live.txt";
        }
	//printf("%i %s \n", ((MNStruct *)globalcontext)->sampler, checkname.c_str());
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        checkfile.close();

        std::ifstream summaryfile;
	std::string fname;
	if(((MNStruct *)globalcontext)->sampler == 0){
		fname = longname+"phys_live.points";
	}
	if(((MNStruct *)globalcontext)->sampler == 1){
	        fname = longname+"_phys_live.txt";
	}

        summaryfile.open(fname.c_str());



        printf("Getting ML \n");
        double maxlike = -1.0*pow(10.0,10);
        for(int i=0;i<number_of_lines;i++){

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);
                double like = paramlist[ndim];

                if(like > maxlike){
                        maxlike = like;
                         for(int i = 0; i < ndim; i++){
                                 Cube[i] = paramlist[i];
			//	printf("ML params %i %g \n", i, Cube[i]);
                         }
                }

        }
        summaryfile.close();
	printf("ML val: %.10g \n", maxlike);

  
	int dotime = 0;
	int debug = ((MNStruct *)globalcontext)->debug; 


	struct timeval tval_before, tval_after, tval_resultone, tval_resulttwo;

	if(dotime == 1){
		gettimeofday(&tval_before, NULL);
	}

	//double bluff = 0;
	//for(int loop = 0; loop < 5; loop++){
	//	Cube[0] = loop*0.2;

	
	if(debug == 1)printf("In like \n");

	double *EFAC;
	double *EQUAD;
        long double LDparams[ndim];
        int pcount;

	double uniformpriorterm = 0;



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

	}	
	
	
	pcount=0;

	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	if(((MNStruct *)globalcontext)->doLinear== 2){phase =((MNStruct *)globalcontext)->LDpriors[0][0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;}
	pcount++;

	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}
	if(((MNStruct *)globalcontext)->doLinear==0){

	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
	   	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
	        	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){

				if(((MNStruct *)globalcontext)->pulse->fitJump[k] == 0){


					//char str1[100],str2[100],str3[100],str4[100],str5[100];
				//	int nread=sscanf(((MNStruct *)globalcontext)->pulse->jumpStr[k],"%s %s %s %s %s",str1,str2,str3,str4,str5);
				//	double prejump=atof(str3);
					JumpVec[p] += (long double)((MNStruct *)globalcontext)->PreJumpVals[k]/SECDAY;
				//	printf("Jump Not Fitted: %i %g\n", k,((MNStruct *)globalcontext)->PreJumpVals[k] );
				}
				else{

				//	printf("Jump Fitted %i %i %i %g \n", p, k, l, (double)((MNStruct *)globalcontext)->pulse->jumpVal[k] );
			        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
				}
	        	 }
		}
           }

	}
	//sleep(5);
	long double DM = ((MNStruct *)globalcontext)->pulse->param[param_dm].val[0];	
	long double RefFreq = pow(10.0, 6)*3100.0L;
	long double FirstFreq = pow(10.0, 6)*2646.0L;
	long double DMPhaseShift = DM*(1.0/(FirstFreq*FirstFreq) - 1.0/(RefFreq*RefFreq))/(2.410*pow(10.0,-16));
	//phase -= DMPhaseShift/SECDAY;
	//printf("DM Shift: %.15Lg %.15Lg\n", DM, DMPhaseShift);


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}

	for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
		((MNStruct *)globalcontext)->pulse->jumpVal[k]= 0;
	}
       //for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
        //      ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        //}

	delete[] JumpVec;

//if(((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps >1){
	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       
	
	//printf("in here \n");
//}
	}
	if(debug == 1)printf("Phase: %g \n", (double)phase);
	if(debug == 1)printf("Formed Residuals \n");

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[0]);}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[p]);}
			pcount++;

		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		if(((MNStruct *)globalcontext)->EQUADPriorType ==1) {uniformpriorterm += log(EQUAD[0]);}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			if(((MNStruct *)globalcontext)->EQUADPriorType ==1) {uniformpriorterm += log(EQUAD[o]);}
			pcount++;
                }
        }


        double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        	SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        	for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
           		SQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
        	}
    	}	  
	  
	double HighFreqStoc1 = 0;
	double HighFreqStoc2 = 0;
	if(((MNStruct *)globalcontext)->incHighFreqStoc == 1){
		HighFreqStoc1 = pow(10.0,Cube[pcount]);
		pcount++;
	}
        if(((MNStruct *)globalcontext)->incHighFreqStoc == 2){
                HighFreqStoc1 = pow(10.0,Cube[pcount]);
                pcount++;
		HighFreqStoc2 = pow(10.0,Cube[pcount]);
                pcount++;
        }

	double DMEQUAD = 0;
	if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
   		DMEQUAD=pow(10.0,Cube[pcount]);
		pcount++;
    	}	
	
	double WidthJitter = 0;
	if(((MNStruct *)globalcontext)->incWidthJitter > 0){
   		WidthJitter=pow(10.0,Cube[pcount]);
		if(((MNStruct *)globalcontext)->EQUADPriorType ==1) {uniformpriorterm += log(WidthJitter);}
		pcount++;
    	}

	double *ProfileNoiseAmps = new double[((MNStruct *)globalcontext)->ProfileNoiseCoeff]();
	int ProfileNoiseCoeff = 0;
	if(((MNStruct *)globalcontext)->incProfileNoise == 1){

		ProfileNoiseCoeff = ((MNStruct *)globalcontext)->ProfileNoiseCoeff;
		double ProfileNoiseAmp = pow(10.0, Cube[pcount]);
		pcount++;
		double ProfileNoiseSpec = Cube[pcount];
		pcount++;
		for(int q = 0; q < ProfileNoiseCoeff; q++){
			double profnoise = ProfileNoiseAmp*ProfileNoiseAmp*pow(double(q+1.0)/((MNStruct *)globalcontext)->ProfileInfo[0][1], ProfileNoiseSpec);
			ProfileNoiseAmps[q] = profnoise;
		}
	}

	if(((MNStruct *)globalcontext)->incProfileNoise == 2){

		ProfileNoiseCoeff = ((MNStruct *)globalcontext)->ProfileNoiseCoeff;
		double ProfileNoiseAmp = pow(10.0, Cube[pcount]);
		for(int q = 0; q < ProfileNoiseCoeff; q++){
			double ProfileNoiseAmp = pow(10.0, Cube[pcount]);
			pcount++;
			double profnoise = ProfileNoiseAmp*ProfileNoiseAmp;
			ProfileNoiseAmps[q] = profnoise;
		}
	}

        if(((MNStruct *)globalcontext)->incProfileNoise == 3){

                ProfileNoiseCoeff = ((MNStruct *)globalcontext)->ProfileNoiseCoeff;
                for(int q = 0; q < ProfileNoiseCoeff; q++){
                        double ProfileNoiseAmp = pow(10.0, Cube[pcount]);
                        pcount++;
                        double profnoise = ProfileNoiseAmp;
                        ProfileNoiseAmps[q] = profnoise;
                }
        }

        if(((MNStruct *)globalcontext)->incProfileNoise == 4){

                ProfileNoiseCoeff = ((MNStruct *)globalcontext)->ProfileNoiseCoeff;
                for(int q = 0; q < ProfileNoiseCoeff; q++){
                        double ProfileNoiseAmp = 1;
                        double profnoise = ProfileNoiseAmp;
                        ProfileNoiseAmps[q] = profnoise;
                }
        }


	double *SignalVec;
	double FreqDet = 0;
	double JDet = 0;
	double FreqLike = 0;
	int startpos = 0;


	double *RedSignalVec;

	if(((MNStruct *)globalcontext)->incRED > 4){

		int FitRedCoeff = 2*((MNStruct *)globalcontext)->numFitRedCoeff;
		double *SignalCoeff = new double[FitRedCoeff];
		for(int i = 0; i < FitRedCoeff; i++){
			SignalCoeff[i] = Cube[pcount];
			//printf("coeffs %i %g \n", i, SignalCoeff[i]);
			pcount++;
		}
			
		

		double Tspan = ((MNStruct *)globalcontext)->Tspan;
		double f1yr = 1.0/3.16e7;


		if(((MNStruct *)globalcontext)->incRED==5){

			double Redamp=Cube[pcount];
			pcount++;
			double Redindex=Cube[pcount];
			pcount++;

			
			Redamp=pow(10.0, Redamp);
			if(((MNStruct *)globalcontext)->RedPriorType == 1) { uniformpriorterm +=log(Redamp); }



			for (int i=0; i< FitRedCoeff/2; i++){
				
				double freq = ((double)(i+1.0))/Tspan;
				
				double rho = (Redamp*Redamp/(12*M_PI*M_PI))*pow(f1yr,(-3)) * pow(freq*365.25,(-Redindex))/(Tspan*24*60*60);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitRedCoeff/2] = SignalCoeff[i+FitRedCoeff/2]*sqrt(rho);  
				//FreqDet += 2*log(rho);	
				//JDet += 2*log(sqrt(rho));
				FreqLike += SignalCoeff[i]*SignalCoeff[i]/rho + SignalCoeff[i+FitRedCoeff/2]*SignalCoeff[i+FitRedCoeff/2]/rho;
			}
		}


		if(((MNStruct *)globalcontext)->incRED==6){




			for (int i=0; i< FitRedCoeff/2; i++){
			

				double RedAmp = pow(10.0, Cube[pcount]);	
				double freq = ((double)(i+1.0))/Tspan;
				
				if(((MNStruct *)globalcontext)->RedPriorType == 1) { uniformpriorterm +=log(RedAmp); }
				double rho = (RedAmp*RedAmp);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitRedCoeff/2] = SignalCoeff[i+FitRedCoeff/2]*sqrt(rho);  
				FreqLike += SignalCoeff[i]*SignalCoeff[i]/rho + SignalCoeff[i+FitRedCoeff/2]*SignalCoeff[i+FitRedCoeff/2]/rho;
				pcount++;
			}
		}

		double *FMatrix = new double[FitRedCoeff*((MNStruct *)globalcontext)->numProfileEpochs];
		for(int i=0;i< FitRedCoeff/2;i++){
			int DMt = 0;
			for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[DMt].bat;
	
				FMatrix[k + (i+startpos)*((MNStruct *)globalcontext)->numProfileEpochs]=cos(2*M_PI*(double(i+1)/Tspan)*time);
				FMatrix[k + (i+FitRedCoeff/2+startpos)*((MNStruct *)globalcontext)->numProfileEpochs] = sin(2*M_PI*(double(i+1)/Tspan)*time);
				DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];

			}
		}

		RedSignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs];
		vector_dgemv(FMatrix,SignalCoeff,RedSignalVec,((MNStruct *)globalcontext)->numProfileEpochs, FitRedCoeff,'N');
		startpos=FitRedCoeff;
		delete[] FMatrix;	
		delete[] SignalCoeff;
		

    	}


	if(((MNStruct *)globalcontext)->incDM > 4){

		int FitDMCoeff = 2*((MNStruct *)globalcontext)->numFitDMCoeff;
		double *SignalCoeff = new double[FitDMCoeff];
		for(int i = 0; i < FitDMCoeff; i++){
			SignalCoeff[startpos + i] = Cube[pcount];
			//printf("coeffs %i %g \n", i, SignalCoeff[i]);
			pcount++;
		}
			
		

		double Tspan = ((MNStruct *)globalcontext)->Tspan;
		double f1yr = 1.0/3.16e7;


		if(((MNStruct *)globalcontext)->incDM==5){

			double DMamp=Cube[pcount];
			pcount++;
			double DMindex=Cube[pcount];
			pcount++;

			
			DMamp=pow(10.0, DMamp);
			if(((MNStruct *)globalcontext)->DMPriorType == 1) { uniformpriorterm +=log(DMamp); }



			for (int i=0; i< FitDMCoeff/2; i++){
				
				double freq = ((double)(i+1.0))/Tspan;
				
				double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freq*365.25,(-DMindex))/(Tspan*24*60*60);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitDMCoeff/2] = SignalCoeff[i+FitDMCoeff/2]*sqrt(rho);  
				//FreqDet += 2*log(rho);	
				//JDet += 2*log(sqrt(rho));
				FreqLike += SignalCoeff[i]*SignalCoeff[i]/rho + SignalCoeff[i+FitDMCoeff/2]*SignalCoeff[i+FitDMCoeff/2]/rho;
			}
		}


		if(((MNStruct *)globalcontext)->incDM==6){




			for (int i=0; i< FitDMCoeff/2; i++){
			

				double DMAmp = pow(10.0, Cube[pcount]);	
				double freq = ((double)(i+1.0))/Tspan;
				
				if(((MNStruct *)globalcontext)->DMPriorType == 1) { uniformpriorterm +=log(DMAmp); }
				double rho = (DMAmp*DMAmp);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitDMCoeff/2] = SignalCoeff[i+FitDMCoeff/2]*sqrt(rho);  
				FreqDet += 2*log(rho);	
				JDet += 2*log(sqrt(rho));
				FreqLike += SignalCoeff[i]*SignalCoeff[i]/rho + SignalCoeff[i+FitDMCoeff/2]*SignalCoeff[i+FitDMCoeff/2]/rho;
				pcount++;
			}
		}

		double *FMatrix = new double[FitDMCoeff*((MNStruct *)globalcontext)->numProfileEpochs];
		for(int i=0;i< FitDMCoeff/2;i++){
			int DMt = 0;
			for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[DMt].bat;
	
				FMatrix[k + (i+startpos)*((MNStruct *)globalcontext)->numProfileEpochs]=cos(2*M_PI*(double(i+1)/Tspan)*time);
				FMatrix[k + (i+FitDMCoeff/2+startpos)*((MNStruct *)globalcontext)->numProfileEpochs] = sin(2*M_PI*(double(i+1)/Tspan)*time);
				DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];

			}
		}

		SignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs];
		vector_dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->numProfileEpochs, FitDMCoeff,'N');
		startpos=FitDMCoeff;
		delete[] FMatrix;	
		delete[] SignalCoeff;
		

    	}



/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Subtract  DM Shape Events////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////  


	double *DMShapeCoeffParams = new double[2+((MNStruct *)globalcontext)->numDMShapeCoeff];
	if(((MNStruct *)globalcontext)->numDMShapeCoeff > 0){
		for(int i = 0; i < 2+((MNStruct *)globalcontext)->numDMShapeCoeff; i++){
			DMShapeCoeffParams[i] = Cube[pcount];
			pcount++;
		}
	}





	double sinAmp = 0;
	double cosAmp = 0;
	if(((MNStruct *)globalcontext)->yearlyDM == 1){
		if(((MNStruct *)globalcontext)->incDM == 0){ SignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs]();}
		sinAmp = Cube[pcount];
		pcount++;
		cosAmp = Cube[pcount];
		pcount++;
		int DMt = 0;
		for(int o=0;o<((MNStruct *)globalcontext)->numProfileEpochs; o++){
			double time=(double)((MNStruct *)globalcontext)->pulse->obsn[DMt].bat - ((MNStruct *)globalcontext)->pulse->param[param_dmepoch].val[0];
			SignalVec[o] += sinAmp*sin((2*M_PI/365.25)*time) + cosAmp*cos((2*M_PI/365.25)*time);
			DMt += ((MNStruct *)globalcontext)->numChanPerInt[o];
		}
	}
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){

		int ProfNbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
		ProfileBats[i] = new long double[2];



		ProfileBats[i][0] = ((MNStruct *)globalcontext)->ProfileData[i][0][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
		ProfileBats[i][1] = ((MNStruct *)globalcontext)->ProfileData[i][ProfNbin-1][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;


		ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
		printf("MBat %i %.20Lg %.20Lg %.15Lg %.15Lg %.15Lg %.15Lg \n", i, ((MNStruct *)globalcontext)->pulse->obsn[i].sat, ((MNStruct *)globalcontext)->pulse->obsn[i].bat, ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr, ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY, ModelBats[i], ModelBats[i]+phase);
	 }


	if(((MNStruct *)globalcontext)->doLinear>0){

		//printf("in do linear %i %i\n", phaseDim+1, phaseDim+((MNStruct *)globalcontext)->numFitTiming);

		double *TimingParams = new double[((MNStruct *)globalcontext)->numFitTiming+((MNStruct *)globalcontext)->numFitJumps]();
		double *TimingSignal = new double[((MNStruct *)globalcontext)->pulse->nobs];

		int fitcount = 1;
		if(((MNStruct *)globalcontext)->doLinear == 2){fitcount = 0;}
		for(int i = fitcount; i < ((MNStruct *)globalcontext)->numFitTiming+((MNStruct *)globalcontext)->numFitJumps; i++){
			TimingParams[i] = Cube[i];
		}

		vector_dgemv(((MNStruct *)globalcontext)->DMatrixVec, TimingParams,TimingSignal,((MNStruct *)globalcontext)->pulse->nobs,((MNStruct *)globalcontext)->numFitTiming+((MNStruct *)globalcontext)->numFitJumps,'N');

		for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
			//printf("ltime: %i %g \n", i, TimingSignal[i]);
	     		 ModelBats[i] -=  (long double) TimingSignal[i]/SECDAY;

		}	

		delete[] TimingSignal;
		delete[] TimingParams;	

	}
	int maxshapecoeff = 0;
	int totshapecoeff = ((MNStruct *)globalcontext)->totshapecoeff; 

	int *numcoeff= new int[((MNStruct *)globalcontext)->numProfComponents];
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		numcoeff[i] =  ((MNStruct *)globalcontext)->numshapecoeff[i];
		if(debug == 1){printf("num coeff in comp %i: %i \n", i, numcoeff[i]);}
	}

	
        int *numProfileStocCoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;
        int totalshapestoccoeff = ((MNStruct *)globalcontext)->totalshapestoccoeff;


	int *numEvoCoeff = ((MNStruct *)globalcontext)->numEvoCoeff;
	int totalEvoCoeff = ((MNStruct *)globalcontext)->totalEvoCoeff;

	int *numEvoFitCoeff = ((MNStruct *)globalcontext)->numEvoFitCoeff;
	int totalEvoFitCoeff = ((MNStruct *)globalcontext)->totalEvoFitCoeff;

	int *numProfileFitCoeff = ((MNStruct *)globalcontext)->numProfileFitCoeff;
	int totalProfileFitCoeff = ((MNStruct *)globalcontext)->totalProfileFitCoeff;



	int totalCoeffForMult = 0;
	int *NumCoeffForMult = new int[((MNStruct *)globalcontext)->numProfComponents];
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		NumCoeffForMult[i] = 0;
		if(numEvoCoeff[i] > NumCoeffForMult[i]){NumCoeffForMult[i]=numEvoCoeff[i];}
		if(numProfileFitCoeff[i] > NumCoeffForMult[i]){NumCoeffForMult[i]=numProfileFitCoeff[i];}
		totalCoeffForMult += NumCoeffForMult[i];
		if(debug == 1){printf("num coeff for mult from comp %i: %i \n", i, NumCoeffForMult[i]);}
	}

        int *numShapeToSave = new int[((MNStruct *)globalcontext)->numProfComponents];
        for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
                numShapeToSave[i] = numProfileStocCoeff[i];
                if(numEvoCoeff[i] >  numShapeToSave[i]){numShapeToSave[i] = numEvoCoeff[i];}
                if(numProfileFitCoeff[i] >  numShapeToSave[i]){numShapeToSave[i] = numProfileFitCoeff[i];}
                if(debug == 1){printf("saved %i %i \n", i, numShapeToSave[i]);}
        }
        int totShapeToSave = 0;
        for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
                totShapeToSave += numShapeToSave[i];
        }

	double shapecoeff[totshapecoeff];
	double StocProfCoeffs[totalshapestoccoeff];
	double **EvoCoeffs=new double*[((MNStruct *)globalcontext)->NProfileEvoPoly]; 
	for(int i = 0; i < ((MNStruct *)globalcontext)->NProfileEvoPoly; i++){EvoCoeffs[i] = new double[totalEvoCoeff];}

	double ProfileFitCoeffs[totalProfileFitCoeff];
	double ProfileModCoeffs[totalCoeffForMult];

	for(int i =0; i < totshapecoeff; i++){
		shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
	}
	for(int p = 0; p < ((MNStruct *)globalcontext)->NProfileEvoPoly; p++){	
		for(int i =0; i < totalEvoCoeff; i++){
			EvoCoeffs[p][i]=((MNStruct *)globalcontext)->MeanProfileEvo[p][i];
			//printf("loaded evo coeff %i %g \n", i, EvoCoeffs[i]);
		}
	}
	if(debug == 1){printf("Filled %i Coeff, %i EvoCoeff \n", totshapecoeff, totalEvoCoeff);}


	double *betas = new double[((MNStruct *)globalcontext)->numProfComponents]();
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		betas[i] = ((MNStruct *)globalcontext)->MeanProfileBeta[i]*((MNStruct *)globalcontext)->ReferencePeriod;
	}



	int cpos = 0;
	int epos = 0;
	int fpos = 0;
	
	for(int i =0; i < totalshapestoccoeff; i++){
		//uniformpriorterm += log(pow(10.0, Cube[pcount]));
		StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
		pcount++;
	}
	//printf("TotalProfileFit: %i \n",totalProfileFitCoeff );
	for(int i =0; i < totalProfileFitCoeff; i++){
		printf("PC: %i %g %g %g \n", i, shapecoeff[i+1], Cube[pcount], shapecoeff[i+1]+Cube[pcount]);
		ProfileFitCoeffs[i]= Cube[pcount];
		pcount++;
	}
	double LinearProfileWidth=0;
	if(((MNStruct *)globalcontext)->FitLinearProfileWidth == 1){
		LinearProfileWidth = Cube[pcount];
		pcount++;
	}

	double *LinearCompWidthTime;
	if(((MNStruct *)globalcontext)->NumFitCompWidths+((MNStruct *)globalcontext)->NumFitCompPos > 0){
		LinearCompWidthTime = new double[((MNStruct *)globalcontext)->NumFitCompWidths+((MNStruct *)globalcontext)->NumFitCompPos];
		for(int i = 0; i < ((MNStruct *)globalcontext)->NumFitCompWidths+((MNStruct *)globalcontext)->NumFitCompPos; i++){
			LinearCompWidthTime[i] = Cube[pcount];
			pcount++;
		}
        }


	double *LinearWidthEvoTime;
	if(((MNStruct *)globalcontext)->incWidthEvoTime > 0){
		LinearWidthEvoTime = new double[((MNStruct *)globalcontext)->incWidthEvoTime];
		for(int i = 0; i < ((MNStruct *)globalcontext)->incWidthEvoTime; i++){
			LinearWidthEvoTime[i] = Cube[pcount];
			pcount++;
		}
        }


	int  EvoFreqExponent = 1;
	if(((MNStruct *)globalcontext)->FitEvoExponent == 1){
		EvoFreqExponent =  floor(Cube[pcount]);
		//printf("Evo Exp set: %i %i \n", pcount, EvoFreqExponent);
		pcount++;
	}
	
	for(int p = 0; p < ((MNStruct *)globalcontext)->NProfileEvoPoly; p++){	
		cpos = 0;
		for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
			for(int i =0; i < numEvoFitCoeff[c]; i++){
				//printf("Evo P: %i %i %i %g %g\n", p, c, i, EvoCoeffs[p][i+cpos],Cube[pcount]);
				EvoCoeffs[p][i+cpos] += Cube[pcount];
				
				pcount++;
			}
			cpos += numEvoCoeff[c];
		
		}
	}

	double EvoProfileWidth=0;
	if(((MNStruct *)globalcontext)->incProfileEvo == 2){
		EvoProfileWidth = Cube[pcount];
		pcount++;
	}

	double EvoEnergyProfileWidth=0;
	if(((MNStruct *)globalcontext)->incProfileEnergyEvo == 2){
		EvoEnergyProfileWidth = Cube[pcount];
		pcount++;
	}



	double **ExtraCompProps=new double*[((MNStruct *)globalcontext)->incExtraProfComp];	
	double **ExtraCompBasis=new double*[((MNStruct *)globalcontext)->incExtraProfComp];
	int incExtraProfComp = ((MNStruct *)globalcontext)->incExtraProfComp;


	for(int i = 0; i < incExtraProfComp; i++){	
		if((int)((MNStruct *)globalcontext)->FitForExtraComp[i][0] == 1){
			//Step Model, time, width, amp
			ExtraCompProps[i] = new double[4+(int)((MNStruct *)globalcontext)->FitForExtraComp[i][1]];
			ExtraCompBasis[i] = new double[(int)((MNStruct *)globalcontext)->FitForExtraComp[i][1]];
			
			ExtraCompProps[i][0] = Cube[pcount]; //Position in time
			pcount++;
			ExtraCompProps[i][1] = Cube[pcount]; // position in phase
			pcount++;
			ExtraCompProps[i][2] = pow(10.0, Cube[pcount])*((MNStruct *)globalcontext)->ReferencePeriod; // comp width
			pcount++;

			for(int c = 0; c < (int)((MNStruct *)globalcontext)->FitForExtraComp[i][1]; c++){
	//			printf("Amps: %i %g \n", pcount,  Cube[pcount]);

				ExtraCompProps[i][3+c] = Cube[pcount]; //amp
				pcount++;
			}

			if((int)((MNStruct *)globalcontext)->FitForExtraComp[i][2] == 1){
				ExtraCompProps[i][3+(int)((MNStruct *)globalcontext)->FitForExtraComp[i][1]] = Cube[pcount]; // shift
                                pcount++;
                        }

		
		}
	//	printf("details: %i %i \n", incExtraProfComp, ((MNStruct *)globalcontext)->NumExtraCompCoeffs);
		if((int)((MNStruct *)globalcontext)->FitForExtraComp[i][0] == 3){
			//exponential decay: time, comp width, max comp amp, log decay timescale

			ExtraCompProps[i] = new double[5+(int)((MNStruct *)globalcontext)->FitForExtraComp[i][1]];
			ExtraCompBasis[i] = new double[(int)((MNStruct *)globalcontext)->FitForExtraComp[i][1]];

			ExtraCompProps[i][0] = Cube[pcount]; //Position
			pcount++;
			ExtraCompProps[i][1] = Cube[pcount]; // position in phase
			pcount++;
			ExtraCompProps[i][2] = pow(10.0, Cube[pcount])*((MNStruct *)globalcontext)->ReferencePeriod; // comp width
			pcount++;
			for(int c = 0; c < (int)((MNStruct *)globalcontext)->FitForExtraComp[i][1]; c++){
	//			printf("Amps: %i %g \n", pcount,  Cube[pcount]);

				ExtraCompProps[i][3+c] = Cube[pcount]; //amp
				pcount++;
			}
			ExtraCompProps[i][3+(int)((MNStruct *)globalcontext)->FitForExtraComp[i][1]] = pow(10.0, Cube[pcount]); //Event width
			pcount++;

			if((int)((MNStruct *)globalcontext)->FitForExtraComp[i][2] == 1){
				ExtraCompProps[i][4+(int)((MNStruct *)globalcontext)->FitForExtraComp[i][1]] = Cube[pcount];
				pcount++;
			}
		}
	//	sleep(5);
	}

	double *PrecessionParams;
	int incPrecession = ((MNStruct *)globalcontext)->incPrecession;
	if(incPrecession == 1){
		PrecessionParams = new double[totalProfileFitCoeff+1];

		PrecessionParams[0] = pow(10.0, Cube[pcount]); //Timescale in years
		pcount++;
		for(int i = 0; i < totalProfileFitCoeff; i++){
				PrecessionParams[1+i] = Cube[pcount];
				pcount++;
		
		}
	}

	double *FinalPrecAmps;
	double *PrecBasis;
	if(incPrecession == 2){

		PrecessionParams = new double[((MNStruct *)globalcontext)->numProfComponents + (((MNStruct *)globalcontext)->numProfComponents-1) + (((MNStruct *)globalcontext)->numProfComponents-1)*(((MNStruct *)globalcontext)->FitPrecAmps+1)];
		PrecBasis = new double[(((MNStruct *)globalcontext)->numProfComponents)*((MNStruct *)globalcontext)->LargestNBins];
		FinalPrecAmps = new double[(((MNStruct *)globalcontext)->pulse->nobs)*(((MNStruct *)globalcontext)->numProfComponents-1)];
		int preccount = 0;

		for(int p2 = 0; p2 < ((MNStruct *)globalcontext)->numProfComponents; p2++){
			PrecessionParams[preccount] = pow(10.0, Cube[pcount]);
			preccount++;
			pcount++;
		}
		for(int p2 = 0; p2 < ((MNStruct *)globalcontext)->numProfComponents-1; p2++){
			PrecessionParams[preccount] = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;;
			pcount++;
			preccount++;

		}
		for(int p2 = 0; p2 < (((MNStruct *)globalcontext)->numProfComponents-1)*(((MNStruct *)globalcontext)->FitPrecAmps+1); p2++){
			PrecessionParams[preccount] =Cube[pcount];
			preccount++;
			pcount++;
		}

	}



	double *TimeCorrProfileParams;
	int incTimeCorrProfileNoise = ((MNStruct *)globalcontext)->incTimeCorrProfileNoise;

	if(incTimeCorrProfileNoise == 1){
		TimeCorrProfileParams = new double[2*((MNStruct *)globalcontext)->totalTimeCorrCoeff];

		for(int c = 0; c < ((MNStruct *)globalcontext)->totalTimeCorrCoeff; c++){
			for(int i = 0; i < 2; i++){
//				printf("Cube: %i %i %g \n", ((MNStruct *)globalcontext)->totalTimeCorrCoeff, pcount, Cube[pcount]);
				TimeCorrProfileParams[2*c+i] = Cube[pcount];
				FreqLike += TimeCorrProfileParams[2*c+i]*TimeCorrProfileParams[2*c+i];
				pcount++;
			}
//			printf("Cube: %i %g \n", pcount, Cube[pcount]);
			double TimeCorrPower = pow(10.0, Cube[pcount]);
			pcount++;

			for(int i = 0; i < 2; i++){
				TimeCorrProfileParams[2*c+i] = TimeCorrProfileParams[2*c+i]*TimeCorrPower;
			}
		}


	}

       double ProfileScatter = 0;
        double ProfileScatter2 = 0;
        double ProfileScatterCut = 0;
        if(((MNStruct *)globalcontext)->incProfileScatter == 1){
                ProfileScatter = pow(10.0, Cube[pcount]);

                pcount++;

                if( ((MNStruct *)globalcontext)->ScatterPBF ==  4 || ((MNStruct *)globalcontext)->ScatterPBF ==  6 ){
                        ProfileScatter2 = pow(10.0, Cube[pcount]);
                        if(ProfileScatter2 < ProfileScatter){
                                double PSTemp = ProfileScatter;
                                ProfileScatter = ProfileScatter2;
                                ProfileScatter2 = PSTemp;
                        }
                        pcount++;
                }


                if( ((MNStruct *)globalcontext)->ScatterPBF ==  5){
                        ProfileScatterCut = Cube[pcount];
                        pcount++;
                }


        }




//	sleep(5);
	if(totshapecoeff+1>=totalshapestoccoeff+1){
		maxshapecoeff=totshapecoeff+1;
	}
	if(totalshapestoccoeff+1 > totshapecoeff+1){
		maxshapecoeff=totalshapestoccoeff+1;
	}

//	printf("max shape coeff: %i \n", maxshapecoeff);
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double modelflux=0;
	cpos = 0;
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		for(int j =0; j < numcoeff[i]; j=j+2){
			modelflux+=sqrt(sqrt(M_PI))*sqrt(betas[i])*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*shapecoeff[cpos+j];
		}
		cpos+= numcoeff[i];
	}
	if(debug == 1){printf("model flux: %g \n", modelflux);}
	double ModModelFlux = modelflux;
	double OPLevel = ((MNStruct *)globalcontext)->offPulseLevel;


	double lnew = 0;
	int badshape = 0;

	int GlobalNBins = ((MNStruct *)globalcontext)->LargestNBins;

	int ProfileBaselineTerms = ((MNStruct *)globalcontext)->ProfileBaselineTerms;
	int PerEpochSize = ProfileBaselineTerms + 1 + 2*ProfileNoiseCoeff;
	int Msize = 1+ProfileBaselineTerms+2*ProfileNoiseCoeff + totalshapestoccoeff;
	int Mcounter=0;
	if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
		Msize++;
	}
	if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
		Msize++;
	}
	if(((MNStruct *)globalcontext)->incWidthJitter > 0){
		Msize++;
	}



	double *MNM = new double[Msize*Msize];




	if(dotime == 1){

		gettimeofday(&tval_after, NULL);
		timersub(&tval_after, &tval_before, &tval_resultone);
		printf("Time elapsed Up to Start of main loop: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
		//gettimeofday(&tval_before, NULL);
		
	}	
	//for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){


	int minep = 0;
	int maxep = ((MNStruct *)globalcontext)->numProfileEpochs;
	int t = 0;
	if(((MNStruct *)globalcontext)->SubIntToFit != -1){
		minep = ((MNStruct *)globalcontext)->SubIntToFit;
		maxep = minep+1;
		for(int ep = 0; ep < minep; ep++){	
			t += ((MNStruct *)globalcontext)->numChanPerInt[ep];
		}
	}
	//printf("eps: %i %i \n", minep, maxep);
	

	for(int ep = minep; ep < maxep; ep++){

		int EpochNbins = (int)((MNStruct *)globalcontext)->ProfileInfo[t][1];

		//printf("Epoch NBins: %i %i %i %i \n", ep, t, EpochNbins, GlobalNBins);

		double *M = new double[EpochNbins*Msize];
		double *NM = new double[EpochNbins*Msize];
	
		double *EpochMNM;
		double *EpochdNM;
		double *EpochTempdNM;
		int EpochMsize = 0;

		int NChanInEpoch = ((MNStruct *)globalcontext)->numChanPerInt[ep];
		int NEpochBins = NChanInEpoch*GlobalNBins;

		if(((MNStruct *)globalcontext)->incWideBandNoise == 1){

			EpochMsize = PerEpochSize*NChanInEpoch+totalshapestoccoeff;
			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
				EpochMsize++;
			}
			if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
				EpochMsize++;
			}
			if(((MNStruct *)globalcontext)->incWidthJitter > 0){
				EpochMsize++;
			}

			EpochMNM = new double[EpochMsize*EpochMsize]();

			EpochdNM = new double[EpochMsize]();
			EpochTempdNM = new double[EpochMsize]();
		}

		double EpochChisq = 0;	
		double EpochdetN = 0;
		double EpochLike = 0;

		double *PrecSignalVec = new double[GlobalNBins*((MNStruct *)globalcontext)->numProfComponents]();
		for(int ch = 0; ch < NChanInEpoch; ch++){

			if(dotime == 1){	
				gettimeofday(&tval_before, NULL);
			}
			if(debug == 1){
				printf("In toa %i \n", t);
				printf("sat: %.15Lg \n", ((MNStruct *)globalcontext)->pulse->obsn[t].sat);
			}
			int nTOA = t;

			double detN  = 0;
			double Chisq  = 0;
			double logMargindet = 0;
			double Marginlike = 0;	
			double badlike = 0; 
			double JitterPrior = 0;	   

			double profilelike=0;

			long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
			long double FoldingPeriodDays = FoldingPeriod/SECDAY;

			int Nbins = GlobalNBins;
			int ProfNbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
			int BinRatio = Nbins/ProfNbins;

			double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
			double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
			long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;
			//printf("Reference period: %.15Lg \n", ReferencePeriod);
			//double *Betatimes = new double[Nbins];
	

			double *shapevec  = new double[Nbins];
			double *ProfileModVec = new double[Nbins]();
			double *ProfileJitterModVec = new double[Nbins]();
		

			double noisemean=0;
			double MLSigma = 0;
			double dataflux = 0;

			//printf("Bins: %i %i \n", t, Nbins);
			double maxamp = 0;
			double maxshape=0;

			//if(ep==36){printf("SSB %i %.10g %.10g \n", nTOA, (double)((MNStruct *)globalcontext)->pulse->obsn[t].freqSSB/pow(10.0, 6), (double)((MNStruct *)globalcontext)->pulse->obsn[t].freq);}

	//
	//		if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g \n", MLSigma, dataflux, modelflux, maxamp);


		    
			long double binpos = ModelBats[nTOA]; 


			if(((MNStruct *)globalcontext)->incRED > 4){
				long double Redshift = (long double)RedSignalVec[ep];

				
		 		binpos+=Redshift/SECDAY;

			}
			//binpos += (long double)(((MNStruct *)globalcontext)->StoredDMVals[ep]/SECDAY);
			if(((MNStruct *)globalcontext)->incDM > 4 || ((MNStruct *)globalcontext)->yearlyDM == 1){
	                	double DMKappa = 2.410*pow(10.0,-16);
        		        double DMScale = 1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[t].freqSSB,2));
				//printf("DMSig %i %g\n", t, SignalVec[ep]*DMScale);
				long double DMshift = (long double)(SignalVec[ep]*DMScale);

				//DMshift = (long double)(((((MNStruct *)globalcontext)->StoredDMVals[ep]-((MNStruct *)globalcontext)->pulse->param[param_dm].val[0])*DMScale));

//				printf("DMSig: %i %.10g %Lg \n", nTOA, (double)((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat, DMshift);
				
		 		binpos+=DMshift/SECDAY;
			//	printf("After %.15Lg \n", binpos);

			}
			for(int ev = 0; ev < incExtraProfComp; ev++){
				if((int)((MNStruct *)globalcontext)->FitForExtraComp[ev][2] == 1){

					if((int)((MNStruct *)globalcontext)->FitForExtraComp[ev][0] == 1){
						if(((MNStruct *)globalcontext)->pulse->obsn[t].bat >= ExtraCompProps[ev][0]){
							   long double TimeDelay = (long double) ExtraCompProps[ev][3+(int)((MNStruct *)globalcontext)->FitForExtraComp[ev][1]];
							   binpos+=TimeDelay/SECDAY;

						}
					}

					if((int)((MNStruct *)globalcontext)->FitForExtraComp[ev][0] == 3){
						double ExtraCompDecayScale = 0;
						if(((MNStruct *)globalcontext)->pulse->obsn[t].bat < ExtraCompProps[ev][0]){
								   ExtraCompDecayScale = 0;
						}
						else{
							ExtraCompDecayScale = exp((ExtraCompProps[ev][0]-((MNStruct *)globalcontext)->pulse->obsn[t].bat)/ExtraCompProps[ev][3+(int)((MNStruct *)globalcontext)->FitForExtraComp[ev][1]]);
						}

						long double TimeDelay = (long double) ExtraCompDecayScale*ExtraCompProps[ev][4+(int)((MNStruct *)globalcontext)->FitForExtraComp[ev][1]];
						 binpos+=TimeDelay/SECDAY;
					}
				}
			}


			if(((MNStruct *)globalcontext)->incDMShapeEvent != 0){

				for(int i =0; i < ((MNStruct *)globalcontext)->incDMShapeEvent; i++){

					int numDMShapeCoeff=((MNStruct *)globalcontext)->numDMShapeCoeff;

					double EventPos = DMShapeCoeffParams[0];
					double EventWidth=DMShapeCoeffParams[1];



					double *DMshapecoeff=new double[numDMShapeCoeff];
					double *DMshapeVec=new double[numDMShapeCoeff];
					for(int c=0; c < numDMShapeCoeff; c++){
						DMshapecoeff[c]=DMShapeCoeffParams[2+c];
					}


					double time=(double)((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat;

					double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
					TNothpl(numDMShapeCoeff,HVal,DMshapeVec);
					double DMsignal=0;
					double DMKappa = 2.410*pow(10.0,-16);
					double DMScale = 1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[nTOA].freqSSB,2));
					for(int c=0; c < numDMShapeCoeff; c++){
						DMsignal += (1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c)))*DMshapeVec[c]*DMshapecoeff[c]*DMScale;
					}

					DMsignal = DMsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
					


					binpos += DMsignal/SECDAY;


					delete[] DMshapecoeff;
					delete[] DMshapeVec;

				}
			}
			//if(binpos < ProfileBats[nTOA][0]){printf("UnderBoard! %.10Lg %.10Lg %.10Lg\n", binpos, ProfileBats[nTOA][0], ProfileBats[nTOA][Nbins-1]);}
			if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

			if(binpos > ProfileBats[nTOA][1])binpos-=FoldingPeriodDays;


			if(binpos > ProfileBats[nTOA][1]){printf("OverBoard! %.10Lg %.10Lg %.10Lg\n", binpos, ProfileBats[nTOA][1], (binpos-ProfileBats[nTOA][1])/FoldingPeriodDays);}

			long double minpos = binpos - FoldingPeriodDays/2;
			if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
			long double maxpos = binpos + FoldingPeriodDays/2;
			if(maxpos> ProfileBats[nTOA][1])maxpos =ProfileBats[nTOA][1];

		//	printf("minpos? %i binpos - FoldingPeriodDays/2: %.15Lg,  ProfileBats[nTOA][0]: %.15Lg, minpos: %.15Lg \n", t, binpos - FoldingPeriodDays/2, ProfileBats[nTOA][0], minpos);

			int InterpBin = 0;
			double FirstInterpTimeBin = 0;
			int  NumWholeBinInterpOffset = 0;

			if(((MNStruct *)globalcontext)->InterpolateProfile == 1){

		
				long double timediff = 0;
				long double bintime = ProfileBats[t][0];


				if(bintime  >= minpos && bintime <= maxpos){
				    timediff = bintime - binpos;
				}
				else if(bintime < minpos){
				    timediff = FoldingPeriodDays+bintime - binpos;
				}
				else if(bintime > maxpos){
				    timediff = bintime - FoldingPeriodDays - binpos;
				}

				//printf("BT: %i %.20Lg %.20Lg %.20Lg %.20Lg %.20Lg %.20Lg %.20Lg\n", t, ModelBats[t], binpos, ProfileBats[t][0], ProfileBats[t][1], timediff, bintime-binpos, FoldingPeriodDays);

				timediff=timediff*SECDAY;

				double OneBin = FoldingPeriod/Nbins;
				int NumBinsInTimeDiff = floor(timediff/OneBin + 0.5);
				double WholeBinsInTimeDiff = NumBinsInTimeDiff*FoldingPeriod/Nbins;
				double OneBinTimeDiff = -1*((double)timediff - WholeBinsInTimeDiff);

				double PWrappedTimeDiff = (OneBinTimeDiff - floor(OneBinTimeDiff/OneBin)*OneBin);

				if(debug == 1)printf("Making InterpBin: %g %g %i %g %g %g\n", (double)timediff, OneBin, NumBinsInTimeDiff, WholeBinsInTimeDiff, OneBinTimeDiff, PWrappedTimeDiff);

				InterpBin = floor(PWrappedTimeDiff/((MNStruct *)globalcontext)->InterpolatedTime+0.5);
				if(InterpBin >= ((MNStruct *)globalcontext)->NumToInterpolate)InterpBin -= ((MNStruct *)globalcontext)->NumToInterpolate;

				FirstInterpTimeBin = -1*(InterpBin-1)*((MNStruct *)globalcontext)->InterpolatedTime;

				if(debug == 1)printf("Interp Time Diffs: %g %g %g %g \n", ((MNStruct *)globalcontext)->InterpolatedTime, InterpBin*((MNStruct *)globalcontext)->InterpolatedTime, PWrappedTimeDiff, InterpBin*((MNStruct *)globalcontext)->InterpolatedTime-PWrappedTimeDiff);

				double FirstBinOffset = timediff-FirstInterpTimeBin;
				double dNumWholeBinOffset = FirstBinOffset/(FoldingPeriod/Nbins);
				int  NumWholeBinOffset = 0;

				NumWholeBinInterpOffset = floor(dNumWholeBinOffset+0.5);
	
				if(debug == 1)printf("Interp bin %i is: %i , First Bin is %g, Offset is %i \n", t, InterpBin, FirstInterpTimeBin, NumWholeBinInterpOffset);
				printf("Interp: %i %.20Lg %i %i \n", t, timediff, InterpBin,  NumWholeBinInterpOffset);

			}

	
			if(dotime == 1){
				gettimeofday(&tval_after, NULL);
				timersub(&tval_after, &tval_before, &tval_resultone);
				printf("Time elapsed up to start of Interp: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
				gettimeofday(&tval_before, NULL);
			}

			double reffreq = ((MNStruct *)globalcontext)->EvoRefFreq;
			double freqdiff =  (((MNStruct *)globalcontext)->pulse->obsn[t].freq - reffreq)/1000.0;
			double freqscale = pow(freqdiff, EvoFreqExponent);


			double snr = ((MNStruct *)globalcontext)->pulse->obsn[t].snr;
			double tobs = ((MNStruct *)globalcontext)->pulse->obsn[t].tobs; 

			//printf("snr: %i %g %g %g \n", t, snr, tobs, snr*3600/tobs);
			snr = snr*3600/tobs;

			double refSN = 1000;
			double SNdiff =  snr/refSN;
			double SNscale = snr-refSN;
			//double SNscale = log10(snr/refSN);//pow(freqdiff, EvoFreqExponent);

			//printf("snr: %i %g %g %g \n", t, snr, SNdiff, SNscale);
			//printf("Evo Exp %i %i %g %g \n", t, EvoFreqExponent, freqdiff, freqscale);



			if(((MNStruct *)globalcontext)->InterpolateProfile == 1){

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Add in any Profile Changes///////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				for(int i =0; i < totalCoeffForMult; i++){
					ProfileModCoeffs[i]=0;	
				}				

				cpos = 0;
				epos = 0;
				fpos = 0;
				int tpos = 0;

				for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
					if(incPrecession == 0){
						for(int i =0; i < numProfileFitCoeff[c]; i++){
							ProfileModCoeffs[i+cpos] += ProfileFitCoeffs[i+fpos];
						}
						for(int p = 0; p < ((MNStruct *)globalcontext)->NProfileEvoPoly; p++){	
							for(int i =0; i < numEvoCoeff[c]; i++){
								ProfileModCoeffs[i+cpos] += EvoCoeffs[p][i+epos]*pow(freqscale, p+1);
							}
						}

						if(incTimeCorrProfileNoise == 1){
							double time=(double)((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat;

							for(int i = 0; i < ((MNStruct *)globalcontext)->numTimeCorrCoeff[c]; i++){
				//			printf("tpos %i %i\n", tpos, i);
								double cosAmp = TimeCorrProfileParams[tpos+2*i+0]*cos(2*M_PI*(double(0+1)/((MNStruct *)globalcontext)->Tspan)*time);	
								double sinAmp = TimeCorrProfileParams[tpos+2*i+1]*sin(2*M_PI*(double(0+1)/((MNStruct *)globalcontext)->Tspan)*time);

								ProfileModCoeffs[i+cpos] += cosAmp + sinAmp;
							}

						}

					}


				if(incPrecession == 1){

					double scale = 1;
					if(PrecessionParams[0]/(((MNStruct *)globalcontext)->Tspan/365.25) > 4){
						scale=sin(2*M_PI*(((MNStruct *)globalcontext)->Tspan/365.25)/PrecessionParams[0]);

					}
					double timediff = (((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat - ((MNStruct *)globalcontext)->pulse->obsn[0].bat)/365.25;
					for(int i =0; i < numProfileFitCoeff[c]; i++){
						
						double MeanCosAmp = ((MNStruct *)globalcontext)->PrecAmps[i+cpos];
						double MeanSinAmp = ((MNStruct *)globalcontext)->PrecAmps[i+cpos+numProfileFitCoeff[c]];
						
						double SinAmp = (MeanSinAmp + PrecessionParams[1+i+fpos])*sin(2*M_PI*timediff/PrecessionParams[0])/scale;
						double CosAmp = (MeanCosAmp + ProfileFitCoeffs[i+fpos])*cos(2*M_PI*timediff/PrecessionParams[0]); 


						ProfileModCoeffs[i+cpos] += SinAmp + CosAmp;	
					}
				}

					cpos += NumCoeffForMult[c];
					epos += numEvoCoeff[c];
					fpos += numProfileFitCoeff[c];	
					tpos += ((MNStruct *)globalcontext)->numTimeCorrCoeff[c];
				}


				

				if(totalCoeffForMult > 0){
					vector_dgemv(((MNStruct *)globalcontext)->InterpolatedShapeletsVec[InterpBin], ProfileModCoeffs,ProfileModVec,Nbins,totalCoeffForMult,'N');

					if(((MNStruct *)globalcontext)->numFitEQUAD > 0 || ((MNStruct *)globalcontext)->incDMEQUAD > 0 ){
						vector_dgemv(((MNStruct *)globalcontext)->InterpolatedJitterProfileVec[InterpBin], ProfileModCoeffs,ProfileJitterModVec,Nbins,totalCoeffForMult,'N');
					}
				}
				ModModelFlux = modelflux;

				cpos=0;
				for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
					for(int j =0; j < NumCoeffForMult[c]; j=j+2){
						ModModelFlux+=sqrt(sqrt(M_PI))*sqrt(betas[c])*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*ProfileModCoeffs[cpos+j];
					}
					cpos+= NumCoeffForMult[c];
				}

				if(dotime == 1){
					gettimeofday(&tval_after, NULL);
					timersub(&tval_after, &tval_before, &tval_resultone);
					printf("Time elapsed,  Modded Profile: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
					gettimeofday(&tval_before, NULL);
				}

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Fill Arrays with interpolated state//////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



				dataflux = 0;
				for(int j = 0; j < ProfNbins; j++){
					if((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-((MNStruct *)globalcontext)->pulse->obsn[nTOA].bline >= 2*((MNStruct *)globalcontext)->pulse->obsn[nTOA].pnoise){
						dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-((MNStruct *)globalcontext)->pulse->obsn[nTOA].bline)*double((ProfileBats[t][1] - ProfileBats[t][0])/(ProfNbins-1))*60*60*24;
					}
				}

				noisemean=0;
				int numoffpulse = 0;
		                maxshape=0;
				double minshape = 0;//pow(10.0, 100);	
				int ZeroWrap = Wrap(0 + NumWholeBinInterpOffset, 0, Nbins-1);

				for(int l = 0; l < 2; l++){
					int startindex = 0;
					int stopindex = 0;
					int step = 0;

					if(l==0){
						startindex = 0; 
						stopindex = Nbins-ZeroWrap;
						step = ZeroWrap;
					}
					else{
						startindex = Nbins-ZeroWrap + BinRatio - (Nbins-ZeroWrap-1)%BinRatio - 1; 
						stopindex = Nbins;
						step = ZeroWrap-Nbins;
					}


					for(int j = startindex; j < stopindex; j+=BinRatio){


						//if(ProfNbins == 256)printf("Filling M: %i %i %i %i %i %i %g \n", t, j, ZeroWrap, stopindex, ProfNbins, BinRatio - (Nbins-ZeroWrap-1)%BinRatio - 1, double(j)/BinRatio);

						double NewIndex = (j + NumWholeBinInterpOffset);
						int Nj =  step+j;//Wrap(j + NumWholeBinInterpOffset, 0, Nbins-1);

						double widthTerm = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj]*(LinearProfileWidth); 



						int cterms = 0;	
						int totcterms = 0;
						for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){	
							for(int wc = 0; wc < ((MNStruct *)globalcontext)->FitCompWidths[c]; wc++){
								widthTerm += ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj+(cterms+1)*Nbins]*LinearCompWidthTime[totcterms]*pow( (((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat - 52880)/365.25, wc);
								totcterms++;
							}
							if(((MNStruct *)globalcontext)->FitCompWidths[c]>0){ cterms++;}
						}


						for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
							for(int wc = 0; wc < ((MNStruct *)globalcontext)->FitCompPos[c]; wc++){
								widthTerm += ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj+(cterms+1)*Nbins]*LinearCompWidthTime[totcterms]*pow( (((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat - 52880)/365.25, wc);
								totcterms++;
							}
							if(((MNStruct *)globalcontext)->FitCompPos[c] > 0){ cterms ++;}
						}


						for(int ew = 0; ew < ((MNStruct *)globalcontext)->incWidthEvoTime; ew++){
							widthTerm += ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj]*LinearWidthEvoTime[ew]*pow( (((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat - 55250)/365.25, ew+1);
						}

						double evoWidthTerm = 0;//((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj]*EvoProfileWidth*freqscale;
						double SNWidthTerm = 0;// ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj]*EvoEnergyProfileWidth*SNscale;
						if(t==2206){
							printf("Interp Vector: %i %g \n", j, ((MNStruct *)globalcontext)->InterpolatedMeanProfile[1687][j]);
						}
						shapevec[j] = ((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpBin][Nj] + widthTerm + ProfileModVec[Nj] + evoWidthTerm + SNWidthTerm;

//						if(((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpBin][Nj] > maxshape){ maxshape = ((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpBin][Nj];}
						//if(t==0){printf("Profile %i %g %g %g \n", j, ((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpBin][Nj], ProfileModVec[Nj], shapevec[j] );}



					if(incPrecession == 2){



						double CompWidth = PrecessionParams[0];
						long double CompPos = binpos;
						double CompAmp = 1;
						long double timediff = 0;
						long double bintime = ProfileBats[t][0];
						long double ExtraTime = j*(ProfileBats[t][1] - ProfileBats[t][0])/(ProfNbins-1);
						if(j > 0) bintime  += ExtraTime;
						if(bintime  >= minpos && bintime <= maxpos){
						    timediff = bintime - CompPos;
						}
						else if(bintime < minpos){
						    timediff = FoldingPeriodDays+bintime - CompPos;
						}
						else if(bintime > maxpos){
						    timediff = bintime - FoldingPeriodDays - CompPos;
						}

						timediff=timediff*SECDAY;

//						if(t==3 && j<3){printf("Bintime: %i ExtraTime: %.15Lg bintime: %.15Lg timediff: %.15Lg\n", j, ExtraTime, bintime, timediff);}


						double Betatime = timediff/CompWidth;
						double Bconst=(1.0/sqrt(CompWidth))/sqrt(sqrt(M_PI));
						double CompSignal = CompAmp*exp(-0.5*Betatime*Betatime)*Bconst;
                                                PrecSignalVec[j + 0*ProfNbins] = CompAmp*exp(-0.5*Betatime*Betatime)*Bconst;;
//						PrecBasis[j + 0*ProfNbins] = 1;
						PrecBasis[j + 0*ProfNbins] = exp(-0.5*Betatime*Betatime)*Bconst;
//						if(t==3){printf("Basis: 0 %i %g %g\n", j , PrecBasis[j + 0*ProfNbins], Betatime);}
						for(int c = 1; c < ((MNStruct *)globalcontext)->numProfComponents; c++){


							double CompWidth = PrecessionParams[c];
							long double CompOffset = PrecessionParams[((MNStruct *)globalcontext)->numProfComponents+c-1];//*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
							long double CompPos = binpos+CompOffset;
							double CompAmp = 1;
					
							if(((MNStruct *)globalcontext)->FitPrecAmps >= 0){

								double FitAmp = 0;
								double refMJD = ((MNStruct *)globalcontext)->PrecRefMJDs[c];
//								double refMJD = 53900;
								double MJD = ((MNStruct *)globalcontext)->pulse->obsn[nTOA].sat;

								for(int a = 0; a <= ((MNStruct *)globalcontext)->FitPrecAmps; a++){
									double logAmp = PrecessionParams[((MNStruct *)globalcontext)->numProfComponents+(((MNStruct *)globalcontext)->numProfComponents-1)+(((MNStruct *)globalcontext)->FitPrecAmps+1)*(c-1)+a];
									FitAmp +=  logAmp*pow((refMJD-MJD)/365.25, a);
								//	if(t==0){printf("Amps: %i %i %i %g %g %g \n", c, j, a, pow((refMJD-MJD)/365.25, a), logAmp, FitAmp);}
								}

								FitAmp = pow(10.0, FitAmp);
								CompAmp *= FitAmp;
							}



//							if(j==0)printf("P2 check: %i %i %i %i %g %g %g \n", nTOA, c, ((MNStruct *)globalcontext)->numProfComponents+c-1, ((MNStruct *)globalcontext)->numProfComponents+(((MNStruct *)globalcontext)->numProfComponents-1)+(((MNStruct *)globalcontext)->numProfComponents-1)*nTOA+c-1, CompWidth, PrecessionParams[((MNStruct *)globalcontext)->numProfComponents+c-1], CompAmp);

							long double timediff = 0;

							long double bintime = ProfileBats[t][0];
							long double ExtraTime = j*(ProfileBats[t][1] - ProfileBats[t][0])/(ProfNbins-1);
							if(j > 0) bintime  += ExtraTime;
							
							if(bintime  >= minpos && bintime <= maxpos){
							    timediff = bintime - CompPos;
							}
							else if(bintime < minpos){
							    timediff = FoldingPeriodDays+bintime - CompPos;
							}
							else if(bintime > maxpos){
							    timediff = bintime - FoldingPeriodDays - CompPos;
							}

							timediff=timediff*SECDAY;

							double Betatime = timediff/CompWidth;
							double Bconst=(1.0/sqrt(CompWidth))/sqrt(sqrt(M_PI));
							CompSignal += CompAmp*exp(-0.5*Betatime*Betatime)*Bconst;
							FinalPrecAmps[t*(((MNStruct *)globalcontext)->numProfComponents-1) + c-1] = CompAmp;
							PrecBasis[j + c*ProfNbins] = exp(-0.5*Betatime*Betatime)*Bconst;

								//if(t==0){printf("One COpm: %i %i %g %g %g \n", c, j, CompAmp, CompAmp*exp(-0.5*Betatime*Betatime)*Bconst, Bconst);}
							PrecSignalVec[j + c*ProfNbins] = CompAmp*exp(-0.5*Betatime*Betatime)*Bconst;
//							if(t==3){printf("Basis: %i %i %g %g\n", c, j , PrecBasis[j + c*ProfNbins], Betatime);}
						}

						shapevec[j] = CompSignal;

//						if(t==246){printf("Sig: %i %g \n", j, CompSignal);}

						if(CompSignal > maxshape){ maxshape = CompSignal;}
					}

					for(int ev = 0; ev < incExtraProfComp; ev++){

							if(debug == 1){printf("EC: %i %g %g %g %g \n", incExtraProfComp, ExtraCompProps[ev][0], ExtraCompProps[ev][1], ExtraCompProps[ev][2], ExtraCompProps[ev][3]);}
							long double ExtraCompPos = binpos+ExtraCompProps[ev][1]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
							long double timediff = 0;
							long double bintime = ProfileBats[t][0] + j*(ProfileBats[t][1] - ProfileBats[t][0])/(ProfNbins-1);
							if(bintime  >= minpos && bintime <= maxpos){
							    timediff = bintime - ExtraCompPos;
							}
							else if(bintime < minpos){
							    timediff = FoldingPeriodDays+bintime - ExtraCompPos;
							}
							else if(bintime > maxpos){
							    timediff = bintime - FoldingPeriodDays - ExtraCompPos;
							}

							timediff=timediff*SECDAY;

							double Betatime = timediff/ExtraCompProps[ev][2];
							
							double ExtraCompDecayScale = 1;

                                                        if((int)((MNStruct *)globalcontext)->FitForExtraComp[ev][0] == 1){
								if(((MNStruct *)globalcontext)->pulse->obsn[t].bat < ExtraCompProps[ev][0]){
	                                                                ExtraCompDecayScale = 0;
								}
                                                        }

//
							if((int)((MNStruct *)globalcontext)->FitForExtraComp[ev][0] == 3){
								if(((MNStruct *)globalcontext)->pulse->obsn[t].bat < ExtraCompProps[ev][0]){
                                                                        ExtraCompDecayScale = 0;
                                                                }
                                                                else{
									ExtraCompDecayScale = exp((ExtraCompProps[ev][0]-((MNStruct *)globalcontext)->pulse->obsn[t].bat)/ExtraCompProps[ev][3+(int)((MNStruct *)globalcontext)->FitForExtraComp[ev][1]]);
								}
                                                        }
							
							TNothpl(((MNStruct *)globalcontext)->FitForExtraComp[ev][1], Betatime, ExtraCompBasis[ev]);


							double ExtraCompAmp = 0;
							for(int k =0; k < (int)((MNStruct *)globalcontext)->FitForExtraComp[ev][1]; k++){
								double Bconst=(1.0/sqrt(betas[0]))/sqrt(pow(2.0,k)*sqrt(M_PI));
								ExtraCompAmp += ExtraCompBasis[ev][k]*Bconst*ExtraCompProps[ev][3+k];

							
							}

							double ExtraCompSignal = ExtraCompAmp*exp(-0.5*Betatime*Betatime)*ExtraCompDecayScale;

							shapevec[j] += ExtraCompSignal;
					    }


					//	if(t==0 || t == 10 || t == 100 || t == 300){printf("Shape: %i %i %g\n", t, j, shapevec[j]);}
						Mcounter = 0;
						for(int q = 0; q < ProfileBaselineTerms; q++){
							M[j/BinRatio+Mcounter*ProfNbins] = pow(double(j)/Nbins, q);
							Mcounter++;
						}
					
						M[j/BinRatio + Mcounter*ProfNbins] = shapevec[j];
						if(debug == 1){ printf("Shapevec: %i %i %g \n", t, j, shapevec[j]);}
						if(shapevec[j] > maxshape){ maxshape = shapevec[j];}
//						if(shapevec[j] < minshape){ minshape = shapevec[j];}
						Mcounter++;


						for(int q = 0; q < ProfileNoiseCoeff; q++){

							M[j/BinRatio + Mcounter*ProfNbins] =     cos(2*M_PI*(double(q+1)/Nbins)*j);
							M[j/BinRatio + (Mcounter+1)*ProfNbins] = sin(2*M_PI*(double(q+1)/Nbins)*j);

							Mcounter += 2;
						}

				
						if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
							M[j/BinRatio + Mcounter*ProfNbins] = ((MNStruct *)globalcontext)->InterpolatedJitterProfile[InterpBin][Nj] + ProfileJitterModVec[Nj];

							Mcounter++;
						}		

						if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
							double DMKappa = 2.410*pow(10.0,-16);
							double DMScale = 1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[t].freqSSB,2));
							M[j/BinRatio + Mcounter*ProfNbins] = (((MNStruct *)globalcontext)->InterpolatedJitterProfile[InterpBin][Nj] + ProfileJitterModVec[Nj])*DMScale;
							Mcounter++;
						}	


						if(((MNStruct *)globalcontext)->incWidthJitter > 0){
							M[j/BinRatio + Mcounter*ProfNbins] = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj];
							Mcounter++;
						}

						int stocpos=0;
						int savepos=0;
						for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
							for(int k = 0; k < numProfileStocCoeff[c]; k++){
							    M[j/BinRatio + (Mcounter+k+stocpos)*ProfNbins] = ((MNStruct *)globalcontext)->InterpolatedShapeletsVec[InterpBin][Nj + (k+savepos)*Nbins];
								if(incPrecession == 2){  M[j/BinRatio + (Mcounter+k+stocpos)*ProfNbins] = PrecBasis[j + c*ProfNbins]; }
							}
							stocpos+=numProfileStocCoeff[c];
							savepos+=numShapeToSave[c];

						}	
					}
				}
				maxshape -=minshape;


				if(incPrecession == 2 && ((MNStruct *)globalcontext)->FitPrecAmps == -1){

					double *D = new double[ProfNbins];
					double *PP = new double[(((MNStruct *)globalcontext)->numProfComponents+1)*(((MNStruct *)globalcontext)->numProfComponents+1)];	
					double *PD = new double[((MNStruct *)globalcontext)->numProfComponents+1];	

					for(int b = 0; b < ProfNbins; b++){
						D[b] = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][b][1];
					}

					((MNStruct *)globalcontext)->PrecBasis = PrecBasis;
					((MNStruct *)globalcontext)->PrecD = D;
					((MNStruct *)globalcontext)->PrecN = ((MNStruct *)globalcontext)->pulse->obsn[nTOA].snr;


					double *Phys=new double[((MNStruct *)globalcontext)->numProfComponents];
					
					SmallNelderMeadOptimum(((MNStruct *)globalcontext)->numProfComponents, Phys);
					double sfac = pow(10.0, Phys[0]);
					for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
						Phys[c] = pow(10.0, Phys[c]);
						Phys[c] = Phys[c]/sfac;
						printf("ML Amps: %i %i %g %g \n", t, c, (double)((MNStruct *)globalcontext)->pulse->obsn[t].bat, Phys[c]);
						for(int b = 0; b < ProfNbins; b++){
							PrecSignalVec[b + c*ProfNbins] = Phys[c]*PrecBasis[b + c*ProfNbins];
						}
					}
					

					double *ML = new double[ProfNbins];
					vector_dgemv(PrecBasis, Phys, ML, ProfNbins, ((MNStruct *)globalcontext)->numProfComponents, 'N');

					for(int b = 0; b < ProfNbins; b++){
						M[b + 1*ProfNbins] = ML[b];
						//if(t==3){printf("ML signal: %i %g %g \n",  b, ML[b], D[b]);}
					}

					delete[] Phys;
					delete[] D;
					delete[] ML;
					
				}



			int doscatter = 0;

			if(((MNStruct *)globalcontext)->incProfileScatter == 1){

				fftw_plan plan;
				fftw_complex *cp;
				fftw_complex *cpbf;

				double *dp = new double[Nbins]();
				for (int i = 0; i < Nbins; i++ ){
					dp[i] = shapevec[i];
				}

			//	double DMKappa = 2.410*pow(10.0,-16);
				double ScatterScale = pow( ((MNStruct *)globalcontext)->pulse->obsn[t].freqSSB, 4)/pow(10.0, 9.0*4.0);
				double STime = ProfileScatter/ScatterScale;

				//printf("Scatter: %i %g %g %g %g\n", t, (double)((MNStruct *)globalcontext)->pulse->obsn[t].freqSSB, ProfileScatter, ScatterScale, STime);
				double *pbf = new double[Nbins]();
				if(((MNStruct *)globalcontext)->ScatterPBF == 1){
					double P = ((MNStruct *)globalcontext)->ReferencePeriod;
					for (int i = 0; i < Nbins; i++ ){
						double time = (P*i)/Nbins;
						pbf[i] = exp(-time/STime); 
					}
				}
				else if(((MNStruct *)globalcontext)->ScatterPBF == 2){
					//STime *= ((MNStruct *)globalcontext)->ReferencePeriod;
					double P = ((MNStruct *)globalcontext)->ReferencePeriod;
					for (int i = 1; i < Nbins; i++ ){
						double time = (P*i)/Nbins;
						pbf[i] = sqrt(M_PI*STime/(4*pow(time, 3)))*exp(-M_PI*M_PI*STime/(16*time)); 
					}
				}
				else if(((MNStruct *)globalcontext)->ScatterPBF == 3){ 
					//STime *= ((MNStruct *)globalcontext)->ReferencePeriod;
					double P = ((MNStruct *)globalcontext)->ReferencePeriod;
					for (int i = 1; i < Nbins; i++ ){
						double time = (P*i)/Nbins;
						pbf[i] = sqrt(pow(M_PI,5)*pow(STime,3)/(8*pow(time, 5)))*exp(-M_PI*M_PI*STime/(4*time)); 
					}
				}
				else if(((MNStruct *)globalcontext)->ScatterPBF == 5){


                                        double P = ((MNStruct *)globalcontext)->ReferencePeriod;
                                        for (int i = 0; i < int(Nbins*ProfileScatterCut); i++ ){
                                                double time = (P*i)/Nbins;
                                                pbf[i] = exp(-time/STime);
                                        }
                                }
				else if(((MNStruct *)globalcontext)->ScatterPBF == 6){ 

					double STime2=ProfileScatter2/ScatterScale;

					double P = ((MNStruct *)globalcontext)->ReferencePeriod;
					for (int i = 0; i < Nbins; i++ ){
						double time = (P*i)/Nbins;
						double arg=0.5*time*(1/STime - 1/STime2);

                                                if(arg > 700){
                                                        arg=700;
                                                }
                                                if(arg < -700){
                                                        arg=-700;
                                                }

                                                double bfunc = 0;
                                                gsl_sf_result result;
                                                int status = gsl_sf_bessel_I0_e(arg, &result);

                                                if(status == GSL_SUCCESS){
                                                //      printf("success: %g %g\n", t,i,arg, exp(-0.5*time*(1/STime + 1/STime2))*result.val);
                                                        bfunc = result.val;
                                                }
			                        pbf[i] = exp(-0.5*time*(1/STime + 1/STime2))*bfunc; //exp(-0.5*time*(1/STime + 1/STime2) + fabs(arg))*bfunc; 
				//		printf("%i %i %g %g %g %g %g \n", t, i, STime, STime2, time, bfunc, (1.0/sqrt(STime*STime2))*exp(-0.5*time*(1/STime + 1/STime2))*bfunc);
					}
				}

				//for (int i = 0; i < Nbins; i++ ){

				//	printf ( "Real Before:  %i %g %g  \n", i, shapevec[i], pbf[i]);
				//}

				cp = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * Nbins);
				cpbf = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * Nbins);

				plan = fftw_plan_dft_r2c_1d(Nbins, dp, cp, FFTW_ESTIMATE); 
				fftw_execute(plan);

				plan = fftw_plan_dft_r2c_1d(Nbins, pbf, cpbf, FFTW_ESTIMATE); 
				fftw_execute(plan);

				double oldpower = 0;

				for (int i = 0; i < Nbins; i++ ){
				
					oldpower += shapevec[i]*shapevec[i];
					double real =  cp[i][0]*cpbf[i][0] - cp[i][1]*cpbf[i][1];
					double imag =  cp[i][0]*cpbf[i][1] + cp[i][1]*cpbf[i][0];

					cp[i][0] =  real;
					cp[i][1] =  imag;

				}



//				double *dp2 = new double[Nbins]();

				fftw_plan plan_back = fftw_plan_dft_c2r_1d(Nbins, cp, dp, FFTW_ESTIMATE);

				fftw_execute ( plan_back );

//				printf ( "\n" );
//				printf ( "  Recovered input data divided by N:\n" );
//				printf ( "\n" );

				double newmax = 0;
				double newpower = 0;
				double newmean = 0;
				for (int i = 0; i < Nbins; i++ ){
					newmean += dp[i];

				}
				newmean=newmean/Nbins;
				for (int i = 0; i < Nbins; i++ ){
					newpower += (dp[i]-newmean)*(dp[i]-newmean);
				}
				newpower = sqrt(newpower/Nbins);
				for (int i = 0; i < Nbins; i++ ){
					M[i + ProfNbins] = (dp[i]-newmean)/newpower;
				}


				  fftw_destroy_plan ( plan );
				  fftw_destroy_plan ( plan_back );

				  fftw_free ( cp );
				  fftw_free ( cpbf );


			}



//				sleep(5);

	
				if(fabs(shapevec[0]-minshape) < maxshape*OPLevel){
					int b = 0;
					while(fabs(shapevec[b*BinRatio]-minshape) < maxshape*OPLevel && b < ProfNbins){
						noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][b][1];
						numoffpulse += 1;
						b++;
					}
//					printf("Offpulse one A %i %i\n", numoffpulse,b);
					b=ProfNbins-1;
					while(fabs(shapevec[b*BinRatio]-minshape) < maxshape*OPLevel && b >= 0){
						noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][b][1];
						numoffpulse += 1;
						b--;
					}
//					printf("Offpulse two A %i %i\n", numoffpulse,b);
				}else{
					int b=ProfNbins/2;
					while(fabs(shapevec[b*BinRatio]-minshape) < maxshape*OPLevel && b < ProfNbins){
//						printf("Offpulse one %i %i\n", numoffpulse,b);
                                                noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][b][1];
                                                numoffpulse += 1;
                                                b++;
                                        }
//					printf("Offpulse one B %i %i\n", numoffpulse,b);
                                        b=ProfNbins/2-1;
					while(fabs(shapevec[b*BinRatio]-minshape) < maxshape*OPLevel && b >= 0){
                                                noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][b][1];
                                                numoffpulse += 1;
                                                b--;
                                        }
  //                                      printf("Offpulse two B %i %i %g %g \n", numoffpulse,b, shapevec[(b+1)*BinRatio]-minshape, maxshape*OPLevel);
                                }

				if(debug == 1)printf("noise mean %g num off pulse %i\n", noisemean, numoffpulse);

				if(dotime == 1){
					gettimeofday(&tval_after, NULL);
					timersub(&tval_after, &tval_before, &tval_resultone);
					printf("Time elapsed,  Filled Arrays: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
					gettimeofday(&tval_before, NULL);
				}
//				if(debug == 1){sleep(5);}
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Get Noise Level//////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				noisemean = noisemean/(numoffpulse);

				MLSigma = 0;
				int MLSigmaCount = 0;
				dataflux = 0;




				if(fabs(shapevec[0]-minshape) < maxshape*OPLevel){
					int b = 0;
					while(fabs(shapevec[b*BinRatio]-minshape) < maxshape*OPLevel && b < ProfNbins){
						double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][b][1] - noisemean;
                                                MLSigma += res*res; MLSigmaCount += 1;

						b++;
					}
//					printf("Offpulse one %i %i\n", numoffpulse,b);
					b=ProfNbins-1;
					while(fabs(shapevec[b*BinRatio]-minshape) < maxshape*OPLevel && b >= 0){
						double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][b][1] - noisemean;
                                                MLSigma += res*res; MLSigmaCount += 1;

						b--;
					}
//					printf("Offpulse two %i %i\n", numoffpulse,b);
				}else{
					int b=ProfNbins/2;
					while(fabs(shapevec[b*BinRatio]-minshape) < maxshape*OPLevel && b < ProfNbins){
						double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][b][1] - noisemean;
                                                MLSigma += res*res; MLSigmaCount += 1;

                                                b++;
                                        }
//					printf("Offpulse one %i %i\n", numoffpulse,b);
                                        b=ProfNbins/2-1;
					while(fabs(shapevec[b*BinRatio]-minshape) < maxshape*OPLevel && b >= 0){

						double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][b][1] - noisemean;
						MLSigma += res*res; MLSigmaCount += 1;
                                                b--;
                                        }
//                                        printf("Offpulse two %i %i %g %g \n", numoffpulse,b, shapevec[(b+1)*BinRatio], maxshape*OPLevel);
                                }


				MLSigma = sqrt(MLSigma/MLSigmaCount);

				if(((MNStruct *)globalcontext)->ProfileNoiseMethod == 1){
					MLSigma=((MNStruct *)globalcontext)->pulse->obsn[nTOA].snr;
				}
				if(((MNStruct *)globalcontext)->ProfileNoiseMethod == 2){
                                        MLSigma=((MNStruct *)globalcontext)->pulse->obsn[nTOA].pnoise;
                                }

//				printf("MLSigma: %i %g %g \n", nTOA, MLSigma, ((MNStruct *)globalcontext)->pulse->obsn[nTOA].snr);


				
				MLSigma = MLSigma*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]]*((MNStruct *)globalcontext)->MLProfileNoise[ep][0];
				dataflux = 0;
				for(int j = 0; j < ProfNbins; j++){
					//if(((MNStruct *)globalcontext)->pulse->obsn[nTOA].bline > 5){printf("bline %i %i %g %g %g %g\n", nTOA, j, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1], ((MNStruct *)globalcontext)->pulse->obsn[nTOA].bline, shapevec[j*BinRatio], maxshape);}
					if(M[j/BinRatio + ProfileBaselineTerms*ProfNbins] >= maxshape*OPLevel){
						//if(((MNStruct *)globalcontext)->pulse->obsn[nTOA].bline > 5){printf("bline %i %i %g %g %g %g\n", nTOA, j, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1], ((MNStruct *)globalcontext)->pulse->obsn[nTOA].bline, M[j/BinRatio + ProfileBaselineTerms*ProfNbins], maxshape);}
					//	if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
							dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-((MNStruct *)globalcontext)->pulse->obsn[nTOA].bline)*double((ProfileBats[t][1] - ProfileBats[t][0])/(ProfNbins-1))*60*60*24;
					//	}
					}
					M[j/BinRatio + ProfileBaselineTerms*ProfNbins] /= maxshape;
				}

				maxamp = dataflux/(ModModelFlux/maxshape);

				if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g maxshape %g \n", MLSigma, dataflux, ModModelFlux, maxamp, maxshape);


				if(dotime == 1){
					gettimeofday(&tval_after, NULL);
					timersub(&tval_after, &tval_before, &tval_resultone);
					printf("Time elapsed,  Get Noise: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
					gettimeofday(&tval_before, NULL);
				}

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Rescale Basis Vectors////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


				Mcounter = 1+ProfileBaselineTerms+ProfileNoiseCoeff*2;
				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
					for(int j =0; j < ProfNbins; j++){
						M[j + Mcounter*ProfNbins] = M[j + Mcounter*ProfNbins]*dataflux/ModModelFlux;
					}			
					Mcounter++;
				}
				if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
					for(int j =0; j < ProfNbins; j++){
						M[j + Mcounter*ProfNbins] = M[j + Mcounter*ProfNbins]*dataflux/ModModelFlux;
					}			
					Mcounter++;
				}
				if(((MNStruct *)globalcontext)->incWidthJitter > 0){
					for(int j =0; j < ProfNbins; j++){
						M[j + Mcounter*ProfNbins] = M[j + Mcounter*ProfNbins]*dataflux/ModModelFlux;
					}
					Mcounter++;
				}
				for(int k = 0; k < totalshapestoccoeff; k++){
					for(int j =0; j < ProfNbins; j++){
				   		M[j + (Mcounter+k)*ProfNbins] = M[j + (Mcounter+k)*ProfNbins]*dataflux;
					}
				}


		    }

	
		    double *NDiffVec = new double[ProfNbins];

	
		///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


			      
			Chisq = 0;
			detN = 0;
			double OffPulsedetN = log(MLSigma*MLSigma);
			double OneDetN = 0;
			double logconvfactor = 1.0/log2(exp(1));

	
	 
			double highfreqnoise = HighFreqStoc1;
			double flatnoise = (HighFreqStoc2*maxamp*maxshape)*(HighFreqStoc2*maxamp*maxshape);
			

			for(int i =0; i < ProfNbins; i++){
			  	Mcounter=2;
				double noise = MLSigma*MLSigma;

				OneDetN=OffPulsedetN;
				if(shapevec[i*BinRatio] > maxshape*OPLevel && ((MNStruct *)globalcontext)->incHighFreqStoc > 0){
					noise +=  flatnoise + (highfreqnoise*maxamp*shapevec[i*BinRatio])*(highfreqnoise*maxamp*shapevec[i*BinRatio]);
					OneDetN=log2(noise)*logconvfactor;
				}

				detN += OneDetN;
				noise=1.0/noise;
				
				double datadiff =  ((MNStruct *)globalcontext)->ProfileData[nTOA][i][1];
				//printf("Data: %g %i %g %g %g\n", Cube[0], i, datadiff, 1.0/sqrt(noise), shapevec[i]);
				NDiffVec[i] = datadiff*noise;

				Chisq += datadiff*NDiffVec[i];
	

				
				for(int j = 0; j < Msize; j++){

					NM[i + j*ProfNbins] = M[i + j*ProfNbins]*noise;
					//printf("Filling NM: %i %i %i %g %g %g\n", t, i, j, M[i + j*ProfNbins], noise, MLSigma);
				}

				

			}

			if(dotime == 1){
				gettimeofday(&tval_after, NULL);
				timersub(&tval_after, &tval_before, &tval_resultone);
				printf("Time elapsed,  Up to LA: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
				gettimeofday(&tval_before, NULL);
			}

			vector_dgemm(M, NM , MNM, ProfNbins, Msize,ProfNbins, Msize, 'T', 'N');


			double *dNM = new double[Msize];
			double *TempdNM = new double[Msize];
			vector_dgemv(M,NDiffVec,dNM,ProfNbins,Msize,'T');

		
			if(((MNStruct *)globalcontext)->incWideBandNoise == 1){

				int ppterms = PerEpochSize;
				if(debug==1){printf("Filling EpochMNM: %i %i %i \n", ppterms, Msize, EpochMsize);}
				for(int j = 0; j < ppterms; j++){
					EpochdNM[ppterms*ch+j] = dNM[j];
					for(int k = 0; k < ppterms; k++){
						if(debug==1){if(j==k && ep == 0){printf("Filling EpochMNM diag: %i %i %i %g \n", j,k,ppterms*ch+j + (ppterms*ch+k)*EpochMsize, MNM[j + k*Msize]);}}
						EpochMNM[ppterms*ch+j + (ppterms*ch+k)*EpochMsize] = MNM[j + k*Msize];
					}
				}


				for(int j = 0; j < Msize-ppterms; j++){

					EpochdNM[ppterms*NChanInEpoch+j] += dNM[ppterms+j];

				//	for(int k = 0; k < ppterms; k++){

						for(int q = 0; q < ppterms; q++){
							EpochMNM[ppterms*ch+q + (ppterms*NChanInEpoch+j)*EpochMsize] = MNM[q + (ppterms+j)*Msize];
							//EpochMNM[ppterms*ch+1 + (ppterms*NChanInEpoch+j)*EpochMsize] = MNM[1 + (ppterms+j)*Msize];

							EpochMNM[ppterms*NChanInEpoch+j + (ppterms*ch+q)*EpochMsize] = MNM[ppterms+j + q*Msize];
							//EpochMNM[ppterms*NChanInEpoch+j + (ppterms*ch+1)*EpochMsize] = MNM[ppterms+j + 1*Msize];
						}
				//	}

					for(int k = 0; k < Msize-ppterms; k++){

						EpochMNM[ppterms*NChanInEpoch+j + (ppterms*NChanInEpoch+k)*EpochMsize] += MNM[ppterms+j + (ppterms+k)*Msize];
					}
				}
			}


	

			double Margindet = 0;
			double StocProfDet = 0;
			double JitterDet = 0;
			if(((MNStruct *)globalcontext)->incWideBandNoise == 0){


				Mcounter=1+ProfileBaselineTerms;

		

				if(((MNStruct *)globalcontext)->incProfileNoise == 1 || ((MNStruct *)globalcontext)->incProfileNoise == 2){
					for(int q = 0; q < ProfileNoiseCoeff; q++){
						if(debug == 1){if(ep==0){printf("PN NBW %i %i %i %i %g %g \n", ep, ch, Mcounter, q, MNM[Mcounter + Mcounter*Msize], MNM[Mcounter+1 + (Mcounter+1)*Msize]);}}
						double profnoise = ProfileNoiseAmps[q];
						JitterDet += 2*log(profnoise);
						MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
						Mcounter++;
						MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
						Mcounter++;
					}
				}

				if(((MNStruct *)globalcontext)->incProfileNoise == 3 || ((MNStruct *)globalcontext)->incProfileNoise == 4){
					for(int q = 0; q < ProfileNoiseCoeff; q++){
						if(debug == 1){if(ep==0){printf("PN BW %i %i %i %i %g %g \n", ep, ch, Mcounter, q, MNM[Mcounter + Mcounter*Msize], MNM[Mcounter+1 + (Mcounter+1)*Msize]);}}
						double profnoise = ProfileNoiseAmps[q]*((MNStruct *)globalcontext)->MLProfileNoise[ep][q+1];
						JitterDet += 2*log(profnoise);
						MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
						Mcounter++;
						MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
						Mcounter++;
					}
				}

				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){

					double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
					profnoise = profnoise*profnoise;
				    	JitterDet += log(profnoise);
				    	MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
					Mcounter++;
				}

				if(((MNStruct *)globalcontext)->incDMEQUAD > 0){

					double profnoise = DMEQUAD;
					profnoise = profnoise*profnoise;
				    	JitterDet += log(profnoise);
				    	MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
					Mcounter++;
				}


				if(((MNStruct *)globalcontext)->incWidthJitter > 0){

					double profnoise = WidthJitter;
					profnoise = profnoise*profnoise;
				    	JitterDet += log(profnoise);
				    	MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
					Mcounter++;
				}
				
				for(int j = 0; j < totalshapestoccoeff; j++){
					StocProfDet += log(StocProfCoeffs[j]);
					MNM[Mcounter+j + (Mcounter+j)*Msize] +=  1.0/StocProfCoeffs[j];
				}

				double *MNM2 = new double[Msize*Msize]();
				double *TempdNM2 = new double[Msize];
				for(int i =0; i < Msize; i++){
					TempdNM[i] = dNM[i];
					TempdNM2[i] = dNM[i];

				}
				for(int i =0; i < Msize*Msize; i++){
					MNM2[i] = MNM[i];
					//printf("MNM %i %i %g \n", t, i, MNM[i]);
				}

				int info = 0;
				int robust = 2;


				info=0;
				double CholDet = 0;
				double CholLike = 0;
				Marginlike = 0;
				if(robust == 1 || robust == 3){
					vector_dpotrfInfo(MNM2, Msize, CholDet, info);
					vector_dpotrsInfo(MNM2, TempdNM2, Msize, info);
					Margindet = CholDet;

					for(int i =0; i < Msize; i++){
        	                                CholLike += TempdNM2[i]*dNM[i];
	                                }
					Marginlike = CholLike;

				}
				
				info = 0;					    
				if(robust == 2 || robust ==3 ){	
			
					vector_TNqrsolve(MNM, dNM, TempdNM, Msize, Margindet, info);

				 	Marginlike = 0;
					for(int i =0; i < Msize; i++){
						Marginlike += TempdNM[i]*dNM[i];
					}

				}


				logMargindet = Margindet;
				
				
				
				double finaloffset = TempdNM[0];
				double finalamp = TempdNM[ProfileBaselineTerms];
				double finalJitter = 0;
				double finalWidthJitter = 0;
				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
					finalJitter = TempdNM[1+ProfileBaselineTerms];
					printf("FJ: %s %i %i %i %g %g %g\n",((MNStruct *)globalcontext)->pulse->obsn[t].fname,  t, ep, ch, finalJitter, sqrt(MNM2[1+ProfileBaselineTerms + Msize*(1+ProfileBaselineTerms)]), finalJitter/sqrt(MNM2[1+ProfileBaselineTerms + Msize*(1+ProfileBaselineTerms)]));
                                }


				if(((MNStruct *)globalcontext)->incWidthJitter > 0){
					finalWidthJitter = TempdNM[1+ProfileBaselineTerms];
					printf("FWJ: %i %g\n", t, finalWidthJitter);
				}
				double *StocVec = new double[ProfNbins];
				double *Jittervec = new double[ProfNbins];
				double *MLShapeVec  = new double[ProfNbins];

				if(t==500){
					for(int i =0; i < ProfNbins; i++){
				//		printf("Shapevec: %i %g %g %g\n", i, shapevec[i], TempdNM[1]*shapevec[i], (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i/BinRatio][1]-TempdNM[0]);
	//					if(((MNStruct *)globalcontext)->numFitEQUAD > 0)Jittervec[i] = M[i + (1+ProfileBaselineTerms)*ProfNbins];
	//					if(((MNStruct *)globalcontext)->numFitEQUAD == 0)Jittervec[i] = pow(10.0, -5)*MLJitterVec[i*BinRatio]*dataflux/ModModelFlux;
					}
				}
				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){TempdNM[1+ProfileBaselineTerms] = 0;}
				vector_dgemv(M,TempdNM,MLShapeVec,ProfNbins,Msize,'N');

		
				for(int q = 0; q < 1+ProfileBaselineTerms; q++){
					TempdNM[q] = 0;
				}
				if(((MNStruct *)globalcontext)->numFitEQUAD > 0)TempdNM[1+ProfileBaselineTerms] = 0;
				//printf("Stoc SIG: %i %g %g %g %g %g %g %g\n", t, (double)((MNStruct *)globalcontext)->pulse->obsn[t].sat, finalamp*(TempdNM[1+ProfileBaselineTerms]/FinalPrecAmps[t*(((MNStruct *)globalcontext)->numProfComponents-1) + 0]+FinalPrecAmps[t*(((MNStruct *)globalcontext)->numProfComponents-1) + 0])/dataflux, finalamp*(TempdNM[2+ProfileBaselineTerms]/FinalPrecAmps[t*(((MNStruct *)globalcontext)->numProfComponents-1) + 1]+FinalPrecAmps[t*(((MNStruct *)globalcontext)->numProfComponents-1) + 1])/dataflux,finalamp*(TempdNM[3+ProfileBaselineTerms]/FinalPrecAmps[t*(((MNStruct *)globalcontext)->numProfComponents-1) + 2]+FinalPrecAmps[t*(((MNStruct *)globalcontext)->numProfComponents-1) + 2])/dataflux, finalamp*(TempdNM[4+ProfileBaselineTerms]/FinalPrecAmps[t*(((MNStruct *)globalcontext)->numProfComponents-1) + 3]+FinalPrecAmps[t*(((MNStruct *)globalcontext)->numProfComponents-1) + 3])/dataflux, finalamp*(TempdNM[5+ProfileBaselineTerms]/FinalPrecAmps[t*(((MNStruct *)globalcontext)->numProfComponents-1) + 4]+FinalPrecAmps[t*(((MNStruct *)globalcontext)->numProfComponents-1) + 4])/dataflux, finalamp*(TempdNM[6+ProfileBaselineTerms]/FinalPrecAmps[t*(((MNStruct *)globalcontext)->numProfComponents-1) + 5]+FinalPrecAmps[t*(((MNStruct *)globalcontext)->numProfComponents-1) + 5])/dataflux);	

				if(incPrecession == 2){printf("Prec Amps: %i  %g %g %g %g %g %g %g %g\n", t, (double)((MNStruct *)globalcontext)->pulse->obsn[t].sat, finalamp/maxshape, finalamp*FinalPrecAmps[t*(((MNStruct *)globalcontext)->numProfComponents-1) + 0]/maxshape, finalamp*FinalPrecAmps[t*(((MNStruct *)globalcontext)->numProfComponents-1) + 1]/maxshape, finalamp*FinalPrecAmps[t*(((MNStruct *)globalcontext)->numProfComponents-1) + 2]/maxshape, finalamp*FinalPrecAmps[t*(((MNStruct *)globalcontext)->numProfComponents-1) + 3]/maxshape, finalamp*FinalPrecAmps[t*(((MNStruct *)globalcontext)->numProfComponents-1) + 4]/maxshape, finalamp*FinalPrecAmps[t*(((MNStruct *)globalcontext)->numProfComponents-1) + 5]/maxshape);}



				vector_dgemv(M,TempdNM,StocVec,ProfNbins,Msize,'N');

				double StocSN = 0;
				double DataStocSN = 0;
				if(debug == 1)printf("Writing to File \n");
				if(writeprofiles == 1){
					std::ofstream profilefile;
					char toanum[15];
					sprintf(toanum, "%d", nTOA);
					std::string ProfileName =  toanum;
					std::string dname = longname+ProfileName+"-Profile.txt";
	
					profilefile.open(dname.c_str());
					double MLChisq = 0;
					profilefile << "#" << std::setprecision(20) << ((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat << " " << ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0] << " " << ((MNStruct *)globalcontext)->pulse->obsn[nTOA].freq <<"\n";
					for(int i =0; i < ProfNbins; i++){
						StocSN += StocVec[i]*StocVec[i]/MLSigma/MLSigma;
					}
					//if(debug == 0)printf("StocSN %i %s %.15Lg %g \n", nTOA, ((MNStruct *)globalcontext)->pulse->obsn[nTOA].fname,  ((MNStruct *)globalcontext)->pulse->obsn[nTOA].sat, sqrt(StocSN));

					for(int i =0; i < Nbins; i+=BinRatio){

						int Nj =  Wrap(i+Nbins/2 + NumWholeBinInterpOffset, 0, Nbins-1);
						
						//MLChisq += pow(((double)((MNStruct *)globalcontext)->ProfileData[nTOA][i/BinRatio][1] - MLShapeVec[i/BinRatio])/profilenoise[i/BinRatio], 2);


						profilefile << Nj << " " << std::setprecision(10) << ((double)((MNStruct *)globalcontext)->ProfileData[nTOA][i/BinRatio][1]-finaloffset)/finalamp << " " << (MLShapeVec[i/BinRatio]-finaloffset)/finalamp << " " << MLSigma/finalamp << " " << StocVec[i/BinRatio]/finalamp << " " << shapevec[i/BinRatio]/maxshape; //profilenoise[i/BinRatio] << " " << finaloffset+finalamp*M[i/BinRatio + ProfileBaselineTerms*ProfNbins] << " " << Jittervec[i/BinRatio] << " " << finaloffset << " " << StocVec[i/BinRatio] << " " << M[i/BinRatio + ProfileBaselineTerms*ProfNbins];
						printf("%i %i %g %g %g \n", nTOA, Nj, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i/BinRatio][1], finaloffset, finalamp);

						if(incExtraProfComp > 0){
							//profilefile << " " << 	finaloffset+finalamp*ExtraCompSignalVec[i] << " " << sqrt(MNM2[ProfileBaselineTerms + Msize*(ProfileBaselineTerms)]);
						}

						if(incPrecession == 2){
							for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
								profilefile << " " << PrecSignalVec[c*Nbins+i]/maxshape;
							}
						}

						profilefile << "\n";


					}
			    		profilefile.close();
					//epochstocsn+=StocSN;

				//delete[] MNM2;
				}
				
				if(writeascii == 1){

					double MaxAscii = 0;
					double MinAscii = pow(10,10);
					for(int i =0; i < Nbins; i+=BinRatio){

						if(MLShapeVec[i/BinRatio]-finaloffset > MaxAscii){
							MaxAscii = MLShapeVec[i/BinRatio]-finaloffset;
						}
						if(MLShapeVec[i/BinRatio]-finaloffset < MinAscii){
							MinAscii = MLShapeVec[i/BinRatio]-finaloffset;
						}
					}

					if(fabs(MinAscii) > fabs(MaxAscii)){
						MaxAscii = MinAscii;
					}

					std::ofstream profilefile;
		

					std::string fname = ((MNStruct *)globalcontext)->pulse->obsn[t].fname;
					char toanum[15];
					sprintf(toanum, "%d", nTOA);
					std::string ProfileName =  toanum;
					std::string dname = fname+".ASCII";
	
					profilefile.open(dname.c_str());
					//profilefile << "#" << std::setprecision(20) << ((MNStruct *)globalcontext)->pulse->obsn[t].sat_day << " " << (((MNStruct *)globalcontext)->pulse->obsn[t].sat_sec+((MNStruct *)globalcontext)->pulse->obsn[nTOA].residual/SECDAY)*SECDAY << " " << ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0] << " 1 " << ((MNStruct *)globalcontext)->pulse->obsn[nTOA].freq << " " << ((MNStruct *)globalcontext)->pulse->param[param_dm].val[0] << " " << ProfNbins <<  " 7 1 " << ((MNStruct *)globalcontext)->pulse->name << " 0.0000000 \n";
					//printf("binpos %i %g %g %g \n", t, (double)binpos-((MNStruct *)globalcontext)->pulse->obsn[nTOA].batCorr, (double)(binpos-((MNStruct *)globalcontext)->pulse->obsn[nTOA].batCorr)*SECDAY, (double)((MNStruct *)globalcontext)->pulse->obsn[t].sat_sec*SECDAY);
					for(int i =0; i < Nbins; i+=BinRatio){

						profilefile << i/BinRatio << " " << std::setprecision(10) << (MLShapeVec[i/BinRatio]-finaloffset)/MaxAscii << "\n";


					}
			    		profilefile.close();
				}
				delete[] StocVec;
				delete[] Jittervec;
				delete[] MLShapeVec;

				delete[] MNM2;
				delete[] TempdNM2;
				
				if(robust == 3 &&  fabs((CholDet-CholLike) - (Margindet-Marginlike)) > 0.05){printf("LA Unstable ! %g %g \n", CholDet-CholLike, Margindet-Marginlike); badlike = 1;}
				
				
				
				
			}
			

			profilelike = -0.5*(detN + Chisq + logMargindet + StocProfDet + JitterDet - Marginlike);
			if(badlike == 1){profilelike = -pow(10.0, 100);}
			//lnew += profilelike;
			if(debug == 1)printf("Like: %i %.15g %.15g %.15g %.15g %.15g %.15g %.15g\n", nTOA,lnew, detN, Chisq, logMargindet, Marginlike, StocProfDet , JitterDet);

			EpochChisq+=Chisq;
			EpochdetN+=detN;
			EpochLike+=profilelike;

			delete[] shapevec;
			//delete[] Jittervec;
			delete[] ProfileModVec;
			delete[] ProfileJitterModVec;
			delete[] NDiffVec;
			delete[] dNM;
			delete[] TempdNM;
			//delete[] Betatimes;

			if(dotime == 1){
				gettimeofday(&tval_after, NULL);
				timersub(&tval_after, &tval_before, &tval_resultone);
				printf("Time elapsed,  End of Epoch: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
			}

			t++;
		}


///////////////////////////////////////////

		if(dotime == 1){
			gettimeofday(&tval_before, NULL);
		}

		if(((MNStruct *)globalcontext)->incWideBandNoise == 0){
                        delete[] M;
                        delete[] NM;


			lnew += EpochLike;
		}

		if(((MNStruct *)globalcontext)->incWideBandNoise == 1){

			for(int i =0; i < EpochMsize; i++){
				EpochTempdNM[i] = EpochdNM[i];
			}

			int EpochMcount=0;
			if(((MNStruct *)globalcontext)->incProfileNoise == 0){EpochMcount = (1+ProfileBaselineTerms)*NChanInEpoch;}

			double BandJitterDet = 0;

			if(((MNStruct *)globalcontext)->incProfileNoise == 1 || ((MNStruct *)globalcontext)->incProfileNoise == 2){
				for(int ch = 0; ch < ((MNStruct *)globalcontext)->numChanPerInt[ep]; ch++){
					
					EpochMcount += 1+ProfileBaselineTerms;
					for(int q = 0; q < ProfileNoiseCoeff; q++){
						if(debug == 1){printf("PN BW %i %i %i %i %g %g \n", ep, ch, EpochMcount, q, EpochMNM[EpochMcount + EpochMcount*EpochMsize], EpochMNM[EpochMcount+1 + (EpochMcount+1)*EpochMsize]);}
						double profnoise = ProfileNoiseAmps[q];
						BandJitterDet += 2*log(profnoise);
						EpochMNM[EpochMcount + EpochMcount*EpochMsize] += 1.0/profnoise;
						EpochMcount++;
						EpochMNM[EpochMcount + EpochMcount*EpochMsize] += 1.0/profnoise;
						EpochMcount++;
					}
				}
			}
			if(((MNStruct *)globalcontext)->incProfileNoise == 3 || ((MNStruct *)globalcontext)->incProfileNoise == 4){
				for(int ch = 0; ch < ((MNStruct *)globalcontext)->numChanPerInt[ep]; ch++){
					
					EpochMcount += 1+ProfileBaselineTerms;
					for(int q = 0; q < ProfileNoiseCoeff; q++){
						if(debug == 1){printf("PN BW %i %i %i %i %g %g \n", ep, ch, EpochMcount, q, EpochMNM[EpochMcount + EpochMcount*EpochMsize], EpochMNM[EpochMcount+1 + (EpochMcount+1)*EpochMsize]);}
						double profnoise = ((MNStruct *)globalcontext)->MLProfileNoise[ep][q+1];
						BandJitterDet += 2*log(profnoise);
						EpochMNM[EpochMcount + EpochMcount*EpochMsize] += 1.0/profnoise;
						EpochMcount++;
						EpochMNM[EpochMcount + EpochMcount*EpochMsize] += 1.0/profnoise;
						EpochMcount++;
					}
				}
			}

			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){

				double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[0]];
				profnoise = profnoise*profnoise;
			    	BandJitterDet += log(profnoise);

				EpochMNM[EpochMcount + (EpochMcount)*EpochMsize] += 1.0/profnoise;
				EpochMcount++;
			}

			if(((MNStruct *)globalcontext)->incDMEQUAD > 0){

				double profnoise = DMEQUAD;
				profnoise = profnoise*profnoise;
			    	BandJitterDet += log(profnoise);

				EpochMNM[EpochMcount + (EpochMcount)*EpochMsize] += 1.0/profnoise;
				EpochMcount++;
			}

			if(((MNStruct *)globalcontext)->incWidthJitter > 0){

				double profnoise = WidthJitter;
				profnoise = profnoise*profnoise;
			    	BandJitterDet += log(profnoise);

				EpochMNM[EpochMcount + (EpochMcount)*EpochMsize] += 1.0/profnoise;
				EpochMcount++;
			}

			double BandStocProfDet = 0;
			for(int j = 0; j < totalshapestoccoeff; j++){
				BandStocProfDet += log(StocProfCoeffs[j]);
				EpochMNM[EpochMcount+j + (EpochMcount+j)*EpochMsize] += 1.0/StocProfCoeffs[j];
			}

			int Epochinfo=0;
			double EpochMargindet = 0;
			vector_dpotrfInfo(EpochMNM, EpochMsize, EpochMargindet, Epochinfo);
			vector_dpotrsInfo(EpochMNM, EpochTempdNM, EpochMsize, Epochinfo);
				    
			double EpochMarginlike = 0;
			for(int i =0; i < EpochMsize; i++){
				EpochMarginlike += EpochTempdNM[i]*EpochdNM[i];
			}

			double EpochProfileLike = -0.5*(EpochdetN + EpochChisq + EpochMargindet - EpochMarginlike + BandJitterDet + BandStocProfDet);
			lnew += EpochProfileLike;
			if(debug == 1){printf("BWLike %i %g %g %g %g %g %g \n", ep,EpochdetN ,EpochChisq ,EpochMargindet , EpochMarginlike, BandJitterDet, BandStocProfDet );}



//			printf("Compare likes: %i %g %g %g %g %g %g\n", ep, EpochProfileLike, BandJitterDet, EpochMarginlike, EpochMargindet, EpochChisq, EpochdetN);
			//printf("like %i %g \n", t, EpochProfileLike);

			//for (int j = 0; j < EpochMsize; j++){
			//    delete[] EpochMNM[j];
			//}
			delete[] EpochMNM;
			delete[] EpochdNM;
			delete[] EpochTempdNM;
			delete[] M;
			delete[] NM;

			if(dotime == 1){
				gettimeofday(&tval_after, NULL);
				timersub(&tval_after, &tval_before, &tval_resultone);
				printf("Time elapsed,  End of Band: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
			}
		}

////////////////////////////////////////////
	
	}
	 


	delete[] MNM;
	 

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] numcoeff;
	delete[] NumCoeffForMult;
	delete[] numShapeToSave;
	delete[] betas;

	for(int j =0; j< ((MNStruct *)globalcontext)->NProfileEvoPoly; j++){
	    delete[] EvoCoeffs[j];
	}
	delete[] EvoCoeffs;
	delete[] ProfileNoiseAmps;
	if(((MNStruct *)globalcontext)->incDM > 0 || ((MNStruct *)globalcontext)->yearlyDM == 1){
		delete[] SignalVec;
	}
	if(((MNStruct *)globalcontext)->incDMShapeEvent != 0){
	delete[] DMShapeCoeffParams;	
	}
	for(int ev = 0; ev < incExtraProfComp; ev++){
                delete[] ExtraCompProps[ev];
		delete[] ExtraCompBasis[ev];
	}
	if(incExtraProfComp > 0){
		delete[] ExtraCompProps;
		delete[] ExtraCompBasis;
	}

	if(incPrecession > 0){
		delete[] PrecBasis;
		delete[] PrecessionParams;
	}
	if(incTimeCorrProfileNoise == 1){
		delete[] TimeCorrProfileParams; 
	}

	lnew += uniformpriorterm - 0.5*(FreqLike + FreqDet) + JDet;
	//printf("Cube: %g %g %g %g %g \n", lnew+5.8185e6, FreqLike, FreqDet, Cube[19], Cube[20]);
	if(debug == 0)printf("End Like: %.10g \n", lnew);
	//printf("End Like: %.10g \n", lnew);
	//}


     //	gettimeofday(&tval_after, NULL);
       //	timersub(&tval_after, &tval_before, &tval_resulttwo);
      // 	printf("Time elapsed: %ld.%06ld %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec, (long int)tval_resulttwo.tv_sec, (long int)tval_resulttwo.tv_usec);
	//}
	//sleep(5);
	//return lnew;
	
	//return bluff;
	//sleep(5);

}

*/
/*
void  WriteProfileDomainLike(std::string longname, int &ndim){



	int writeprofiles=1;
	int writeascii=1;

	int profiledimstart=0;
        double *Cube = new double[ndim];
        int number_of_lines = 0;

        std::ifstream checkfile;
	std::string checkname;
	if(((MNStruct *)globalcontext)->sampler == 0){
	        checkname = longname+"phys_live.points";
	}
        if(((MNStruct *)globalcontext)->sampler == 1){
                checkname = longname+"_phys_live.txt";
        }
	//printf("%i %s \n", ((MNStruct *)globalcontext)->sampler, checkname.c_str());
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        checkfile.close();

        std::ifstream summaryfile;
	std::string fname;
	if(((MNStruct *)globalcontext)->sampler == 0){
		fname = longname+"phys_live.points";
	}
	if(((MNStruct *)globalcontext)->sampler == 1){
	        fname = longname+"_phys_live.txt";
	}

        summaryfile.open(fname.c_str());



        printf("Getting ML \n");
        double maxlike = -1.0*pow(10.0,10);
        for(int i=0;i<number_of_lines;i++){

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);
                double like = paramlist[ndim];

                if(like > maxlike){
                        maxlike = like;
                         for(int i = 0; i < ndim; i++){
                                 Cube[i] = paramlist[i];
                         }
                }

        }
        summaryfile.close();
	printf("ML val: %.10g \n", maxlike);

	int dotime = 0;
	struct timeval tval_before, tval_after, tval_resultone, tval_resulttwo;


	//gettimeofday(&tval_before, NULL);
	//double bluff = 0;
	//for(int loop = 0; loop < 5; loop++){
	//	Cube[0] = loop*0.2;

	int debug = ((MNStruct *)globalcontext)->debug; 

	if(debug == 1)printf("In like \n");

	double *EFAC;
	double *EQUAD;
        long double LDparams[ndim];
        int pcount;

	double uniformpriorterm = 0;



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
	   	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
	        	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){

				if(((MNStruct *)globalcontext)->pulse->fitJump[k] == 0){


					//char str1[100],str2[100],str3[100],str4[100],str5[100];
				//	int nread=sscanf(((MNStruct *)globalcontext)->pulse->jumpStr[k],"%s %s %s %s %s",str1,str2,str3,str4,str5);
				//	double prejump=atof(str3);
					JumpVec[p] += (long double)((MNStruct *)globalcontext)->PreJumpVals[k]/SECDAY;
					//printf("Jump Not Fitted: %i %g %g\n", k, prejump,((MNStruct *)globalcontext)->PreJumpVals[k] );
				}
				else{

					//printf("Jump Fitted %i %i %i %g \n", p, k, l, (double)((MNStruct *)globalcontext)->pulse->jumpVal[k] );
			        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
				}
	        	 }
		}
           }

	}
	//sleep(5);
	long double DM = ((MNStruct *)globalcontext)->pulse->param[param_dm].val[0];	
	long double RefFreq = pow(10.0, 6)*3100.0L;
	long double FirstFreq = pow(10.0, 6)*2646.0L;
	long double DMPhaseShift = DM*(1.0/(FirstFreq*FirstFreq) - 1.0/(RefFreq*RefFreq))/(2.410*pow(10.0,-16));
	//phase -= DMPhaseShift/SECDAY;
	//printf("DM Shift: %.15Lg %.15Lg\n", DM, DMPhaseShift);


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}

	for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
		((MNStruct *)globalcontext)->pulse->jumpVal[k]= 0;
	}
       //for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
        //      ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        //}

	delete[] JumpVec;


	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);    
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);     
	



	
	if(debug == 1)printf("Phase: %g \n", (double)phase);
	if(debug == 1)printf("Formed Residuals \n");

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[0]);}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[p]);}
			pcount++;

		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }


        double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        	SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        	for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
           		SQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
        	}
    	}	  
	  

	double HighFreqStoc1 = 0;
	double HighFreqStoc2 = 0;
	if(((MNStruct *)globalcontext)->incHighFreqStoc == 1){
		HighFreqStoc1 = pow(10.0,Cube[pcount]);
		pcount++;
	}
        if(((MNStruct *)globalcontext)->incHighFreqStoc == 2){
                HighFreqStoc1 = pow(10.0,Cube[pcount]);
                pcount++;
		HighFreqStoc2 = pow(10.0,Cube[pcount]);
                pcount++;
        }

	double DMEQUAD = 0;
	if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
   		DMEQUAD=pow(10.0,Cube[pcount]);
		pcount++;
    	}	
	
	double WidthJitter = 0;
	if(((MNStruct *)globalcontext)->incWidthJitter > 0){
   		WidthJitter=pow(10.0,Cube[pcount]);
		pcount++;
    	}



	double *ProfileNoiseAmps = new double[((MNStruct *)globalcontext)->ProfileNoiseCoeff]();
	int ProfileNoiseCoeff = 0;
	if(((MNStruct *)globalcontext)->incProfileNoise == 1){

		ProfileNoiseCoeff = ((MNStruct *)globalcontext)->ProfileNoiseCoeff;
		double ProfileNoiseAmp = pow(10.0, Cube[pcount]);
		pcount++;
		double ProfileNoiseSpec = Cube[pcount];
		pcount++;
		for(int q = 0; q < ProfileNoiseCoeff; q++){
			double profnoise = ProfileNoiseAmp*ProfileNoiseAmp*pow(double(q+1)/((MNStruct *)globalcontext)->ProfileInfo[0][1], ProfileNoiseSpec);
			ProfileNoiseAmps[q] = profnoise;
		}
	}

	if(((MNStruct *)globalcontext)->incProfileNoise == 2){

		ProfileNoiseCoeff = ((MNStruct *)globalcontext)->ProfileNoiseCoeff;
		for(int q = 0; q < ProfileNoiseCoeff; q++){
			double ProfileNoiseAmp = pow(10.0, Cube[pcount]);
			pcount++;
			double profnoise = ProfileNoiseAmp*ProfileNoiseAmp;
			ProfileNoiseAmps[q] = profnoise;
		}
	}

        if(((MNStruct *)globalcontext)->incProfileNoise == 3){

                ProfileNoiseCoeff = ((MNStruct *)globalcontext)->ProfileNoiseCoeff;
                for(int q = 0; q < ProfileNoiseCoeff; q++){
                        double ProfileNoiseAmp = pow(10.0, Cube[pcount]);
                        pcount++;
                        double profnoise = ProfileNoiseAmp;
                        ProfileNoiseAmps[q] = profnoise;
                }
        }

        if(((MNStruct *)globalcontext)->incProfileNoise == 4){

                ProfileNoiseCoeff = ((MNStruct *)globalcontext)->ProfileNoiseCoeff;
                for(int q = 0; q < ProfileNoiseCoeff; q++){
                        double ProfileNoiseAmp = 1;
                        double profnoise = ProfileNoiseAmp;
                        ProfileNoiseAmps[q] = profnoise;
                }
        }



	double *SignalVec;
	double FreqDet = 0;
	double JDet = 0;
	double FreqLike = 0;
	int startpos = 0;
	if(((MNStruct *)globalcontext)->incDM > 4){

		int FitDMCoeff = 2*((MNStruct *)globalcontext)->numFitDMCoeff;
		double *SignalCoeff = new double[FitDMCoeff];
		for(int i = 0; i < FitDMCoeff; i++){
			SignalCoeff[startpos + i] = Cube[pcount];
			//printf("coeffs %i %g \n", i, SignalCoeff[i]);
			pcount++;
		}
			
		

		double Tspan = ((MNStruct *)globalcontext)->Tspan;
		double f1yr = 1.0/3.16e7;


		if(((MNStruct *)globalcontext)->incDM==5){

			double DMamp=Cube[pcount];
			pcount++;
			double DMindex=Cube[pcount];
			pcount++;

			
			DMamp=pow(10.0, DMamp);
			if(((MNStruct *)globalcontext)->DMPriorType == 1) { uniformpriorterm +=log(DMamp); }



			for (int i=0; i< FitDMCoeff/2; i++){
				
				double freq = ((double)(i+1.0))/Tspan;
				
				double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freq*365.25,(-DMindex))/(Tspan*24*60*60);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitDMCoeff/2] = SignalCoeff[i+FitDMCoeff/2]*sqrt(rho);  
				FreqDet += 2*log(rho);	
				JDet += 2*log(sqrt(rho));
				FreqLike += SignalCoeff[i]*SignalCoeff[i]/rho + SignalCoeff[i+FitDMCoeff/2]*SignalCoeff[i+FitDMCoeff/2]/rho;
			}
		}


		if(((MNStruct *)globalcontext)->incDM==6){




			for (int i=0; i< FitDMCoeff/2; i++){
			

				double DMAmp = pow(10.0, Cube[pcount]);	
				double freq = ((double)(i+1.0))/Tspan;
				
				if(((MNStruct *)globalcontext)->DMPriorType == 1) { uniformpriorterm +=log(DMAmp); }
				double rho = (DMAmp*DMAmp);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitDMCoeff/2] = SignalCoeff[i+FitDMCoeff/2]*sqrt(rho);  
				FreqDet += 2*log(rho);	
				JDet += 2*log(sqrt(rho));
				FreqLike += SignalCoeff[i]*SignalCoeff[i]/rho + SignalCoeff[i+FitDMCoeff/2]*SignalCoeff[i+FitDMCoeff/2]/rho;
				pcount++;
			}
		}

		double *FMatrix = new double[FitDMCoeff*((MNStruct *)globalcontext)->numProfileEpochs];
		for(int i=0;i< FitDMCoeff/2;i++){
			int DMt = 0;
			for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[DMt].bat;
	
				FMatrix[k + (i+startpos)*((MNStruct *)globalcontext)->numProfileEpochs]=cos(2*M_PI*(double(i+1)/Tspan)*time);
				FMatrix[k + (i+FitDMCoeff/2+startpos)*((MNStruct *)globalcontext)->numProfileEpochs] = sin(2*M_PI*(double(i+1)/Tspan)*time);
				DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];

			}
		}

		SignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs];
		vector_dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->numProfileEpochs, FitDMCoeff,'N');
		startpos=FitDMCoeff;
		delete[] FMatrix;	
		

    	}



/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Subtract  DM Shape Events////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////  


	double *DMShapeCoeffParams = new double[2+((MNStruct *)globalcontext)->numDMShapeCoeff];
	if(((MNStruct *)globalcontext)->numDMShapeCoeff > 0){
		for(int i = 0; i < 2+((MNStruct *)globalcontext)->numDMShapeCoeff; i++){
			DMShapeCoeffParams[i] = Cube[pcount];
			pcount++;
		}
	}




	double sinAmp = 0;
	double cosAmp = 0;
	if(((MNStruct *)globalcontext)->yearlyDM == 1){
		if(((MNStruct *)globalcontext)->incDM == 0){ SignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs]();}
		sinAmp = Cube[pcount];
		pcount++;
		cosAmp = Cube[pcount];
		pcount++;
		int DMt = 0;
		for(int o=0;o<((MNStruct *)globalcontext)->numProfileEpochs; o++){
			double time=(double)((MNStruct *)globalcontext)->pulse->obsn[DMt].bat - ((MNStruct *)globalcontext)->pulse->param[param_dmepoch].val[0];
			SignalVec[o] += sinAmp*sin((2*M_PI/365.25)*time) + cosAmp*cos((2*M_PI/365.25)*time);
			DMt += ((MNStruct *)globalcontext)->numChanPerInt[o];
		}
	}
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){

		int ProfNbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
		ProfileBats[i] = new long double[2];



		ProfileBats[i][0] = ((MNStruct *)globalcontext)->ProfileData[i][0][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
		ProfileBats[i][1] = ((MNStruct *)globalcontext)->ProfileData[i][ProfNbin-1][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;


		ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
		//printf("loop %i %i %g %.15Lg %.15Lg \n", loop, i, Cube[0], ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr, ((MNStruct *)globalcontext)->pulse->obsn[i].residual);
	 }


	int maxshapecoeff = 0;
	int totshapecoeff = ((MNStruct *)globalcontext)->totshapecoeff; 

	int *numcoeff= new int[((MNStruct *)globalcontext)->numProfComponents];
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		numcoeff[i] =  ((MNStruct *)globalcontext)->numshapecoeff[i];
		if(debug == 1){printf("num coeff in comp %i: %i \n", i, numcoeff[i]);}
	}

	
        int *numProfileStocCoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;
        int totalshapestoccoeff = ((MNStruct *)globalcontext)->totalshapestoccoeff;


	int *numEvoCoeff = ((MNStruct *)globalcontext)->numEvoCoeff;
	int totalEvoCoeff = ((MNStruct *)globalcontext)->totalEvoCoeff;

	int *numEvoFitCoeff = ((MNStruct *)globalcontext)->numEvoFitCoeff;
	int totalEvoFitCoeff = ((MNStruct *)globalcontext)->totalEvoFitCoeff;

	int *numProfileFitCoeff = ((MNStruct *)globalcontext)->numProfileFitCoeff;
	int totalProfileFitCoeff = ((MNStruct *)globalcontext)->totalProfileFitCoeff;



	int totalCoeffForMult = 0;
	int *NumCoeffForMult = new int[((MNStruct *)globalcontext)->numProfComponents];
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		NumCoeffForMult[i] = 0;
		if(numEvoCoeff[i] > NumCoeffForMult[i]){NumCoeffForMult[i]=numEvoCoeff[i];}
		if(numProfileFitCoeff[i] > NumCoeffForMult[i]){NumCoeffForMult[i]=numProfileFitCoeff[i];}
		totalCoeffForMult += NumCoeffForMult[i];
		if(debug == 1){printf("num coeff for mult from comp %i: %i \n", i, NumCoeffForMult[i]);}
	}

        int *numShapeToSave = new int[((MNStruct *)globalcontext)->numProfComponents];
        for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
                numShapeToSave[i] = numProfileStocCoeff[i];
                if(numEvoCoeff[i] >  numShapeToSave[i]){numShapeToSave[i] = numEvoCoeff[i];}
                if(numProfileFitCoeff[i] >  numShapeToSave[i]){numShapeToSave[i] = numProfileFitCoeff[i];}
                if(debug == 1){printf("saved %i %i \n", i, numShapeToSave[i]);}
        }
        int totShapeToSave = 0;
        for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
                totShapeToSave += numShapeToSave[i];
        }

	double shapecoeff[totshapecoeff];
	double StocProfCoeffs[totalshapestoccoeff];

	double **EvoCoeffs=new double*[((MNStruct *)globalcontext)->NProfileEvoPoly]; 
	for(int i = 0; i < ((MNStruct *)globalcontext)->NProfileEvoPoly; i++){EvoCoeffs[i] = new double[totalEvoCoeff];}

	double ProfileFitCoeffs[totalProfileFitCoeff];
	double ProfileModCoeffs[totalCoeffForMult];

	for(int i =0; i < totshapecoeff; i++){
		shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
		//printf("mean coeff %i %g \n", i, shapecoeff[i]);
	}
	for(int p = 0; p < ((MNStruct *)globalcontext)->NProfileEvoPoly; p++){	
		for(int i =0; i < totalEvoCoeff; i++){
			EvoCoeffs[p][i]=((MNStruct *)globalcontext)->MeanProfileEvo[p][i];
			//printf("loaded evo coeff %i %i %g \n", p, i, EvoCoeffs[p][i]);
		}
	}

	if(debug == 1){printf("Filled %i Coeff, %i EvoCoeff \n", totshapecoeff, totalEvoCoeff);}
	profiledimstart=pcount;


	double *betas = new double[((MNStruct *)globalcontext)->numProfComponents]();
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		betas[i] = ((MNStruct *)globalcontext)->MeanProfileBeta[i]*((MNStruct *)globalcontext)->ReferencePeriod;
	}



	int cpos = 0;
	int epos = 0;
	int fpos = 0;
	for(int i =0; i < totalshapestoccoeff; i++){
		StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
		pcount++;
	}
	//printf("TotalProfileFit: %i \n",totalProfileFitCoeff );
	for(int i =0; i < totalProfileFitCoeff; i++){
	//	printf("ProfileFit: %i %i %g \n", i, pcount, Cube[pcount]);
		ProfileFitCoeffs[i]= Cube[pcount];
		pcount++;
	}

	int fitpsum=0;
	int nofitpsum=0;
	for(int p2 = 0; p2 < ((MNStruct *)globalcontext)->numProfComponents; p2++){
		int skipone=0;
		if(p2==0 && nofitpsum == 0){
			printf("P0: %i %g %g\n", nofitpsum, ((MNStruct *)globalcontext)->MeanProfileShape[nofitpsum], 0.0);
			skipone=1;
			nofitpsum++;
		}
		for(int p3 = 0; p3 < ((MNStruct *)globalcontext)->numProfileFitCoeff[p2]; p3++){
			printf("P: %g \n", nofitpsum+p3, shapecoeff[nofitpsum+p3]+ProfileFitCoeffs[fitpsum+p3]);
		}
		for(int p3 = ((MNStruct *)globalcontext)->numProfileFitCoeff[p2]; p3 < ((MNStruct *)globalcontext)->numshapecoeff[p2]-skipone; p3++){
			printf("P: %g\n", nofitpsum+p3, shapecoeff[nofitpsum+p3]);
		}
		fitpsum+=((MNStruct *)globalcontext)->numProfileFitCoeff[p2];
		nofitpsum+=((MNStruct *)globalcontext)->numshapecoeff[p2]-skipone;
	}

	double LinearProfileWidth=0;
	if(((MNStruct *)globalcontext)->FitLinearProfileWidth == 1){
		LinearProfileWidth = Cube[pcount];
		pcount++;
	}

        double *LinearWidthEvoTime;
        if(((MNStruct *)globalcontext)->incWidthEvoTime > 0){
                LinearWidthEvoTime = new double[((MNStruct *)globalcontext)->incWidthEvoTime];
                for(int i = 0; i < ((MNStruct *)globalcontext)->incWidthEvoTime; i++){
                        LinearWidthEvoTime[i] = Cube[pcount];
                        pcount++;
                }
        }

	int  EvoFreqExponent = 1;
	if(((MNStruct *)globalcontext)->FitEvoExponent == 1){
		EvoFreqExponent =  floor(Cube[pcount]);
		//printf("Evo Exp set: %i %i \n", pcount, EvoFreqExponent);
		pcount++;
	}
	
	
	for(int p = 0; p < ((MNStruct *)globalcontext)->NProfileEvoPoly; p++){	
		cpos = 0;
		for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
			for(int i =0; i < numEvoFitCoeff[c]; i++){
				EvoCoeffs[p][i+cpos] += Cube[pcount];
				//printf("changed evo coeff %i %i %i %g \n", p, i, i+cpos, EvoCoeffs[i]);
				pcount++;
			}
			cpos += numEvoCoeff[c];
		
		}
	}
	
	for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
		printf("B: %g \n", ((MNStruct *)globalcontext)->MeanProfileBeta[c]);
	}
	for(int p = 0; p < ((MNStruct *)globalcontext)->NProfileEvoPoly; p++){	
		for(int i =0; i < totalEvoCoeff; i++){
			printf("E: %g \n", EvoCoeffs[p][i]);
		}
	}
	if(debug == 1){printf("Done with Profile\n");}
	double EvoProfileWidth=0;
	if(((MNStruct *)globalcontext)->incProfileEvo == 2){
		EvoProfileWidth = Cube[pcount];
		pcount++;
	}



	double *ExtraCompProps;	
	double *ExtraCompBasis;
	int incExtraProfComp = ((MNStruct *)globalcontext)->incExtraProfComp;
	
	if(incExtraProfComp == 1){
		//Step Model, time, width, amp
		ExtraCompProps = new double[4];
		
		ExtraCompProps[0] = Cube[pcount]; //Position in time
		pcount++;
		ExtraCompProps[1] = Cube[pcount]; // position in phase
		pcount++;
		ExtraCompProps[2] = pow(10.0, Cube[pcount])*((MNStruct *)globalcontext)->ReferencePeriod; // comp width
		pcount++;
		ExtraCompProps[3] = pow(10.0, Cube[pcount]); //amp
		pcount++;
	
	}
	if(incExtraProfComp == 2){
		//Gaussian decay model: time, comp width, max comp amp, log decay width
                ExtraCompProps = new double[5];

                ExtraCompProps[0] = Cube[pcount]; //Position
                pcount++;
		ExtraCompProps[1] = Cube[pcount]; // position in phase
                pcount++;
                ExtraCompProps[2] = pow(10.0, Cube[pcount])*((MNStruct *)globalcontext)->ReferencePeriod; // comp width
                pcount++;
                ExtraCompProps[3] = pow(10.0, Cube[pcount]); //amp
                pcount++;
		ExtraCompProps[4] = pow(10.0, Cube[pcount]); //Event width
		pcount++;

	}

	if(incExtraProfComp == 3){
		//exponential decay: time, comp width, max comp amp, log decay timescale

                ExtraCompProps = new double[5+((MNStruct *)globalcontext)->NumExtraCompCoeffs];
		ExtraCompBasis = new double[((MNStruct *)globalcontext)->NumExtraCompCoeffs];

                ExtraCompProps[0] = Cube[pcount]; //Position
                pcount++;
                ExtraCompProps[1] = Cube[pcount]; // position in phase
                pcount++;
                ExtraCompProps[2] = pow(10.0, Cube[pcount])*((MNStruct *)globalcontext)->ReferencePeriod; // comp width
                pcount++;
		for(int i = 0; i < ((MNStruct *)globalcontext)->NumExtraCompCoeffs; i++){
			ExtraCompProps[3+i] = Cube[pcount]; //amp
			pcount++;
		}
		ExtraCompProps[3+((MNStruct *)globalcontext)->NumExtraCompCoeffs] = pow(10.0, Cube[pcount]); //Event width
		pcount++;
		ExtraCompProps[4+((MNStruct *)globalcontext)->NumExtraCompCoeffs] = Cube[pcount];
                pcount++;


	}
	if(incExtraProfComp == 4){
		ExtraCompProps = new double[5+2];
		ExtraCompProps[0] = Cube[pcount]; //Position
		pcount++;
		ExtraCompProps[1] = pow(10.0, Cube[pcount]); //Event width
		pcount++;
		for(int i = 0; i < 5; i++){
			ExtraCompProps[2+i] = Cube[pcount]; // Coeff Amp
			pcount++;

		}


	}

	double *PrecessionParams;
	int incPrecession = ((MNStruct *)globalcontext)->incPrecession;
	if(incPrecession == 1){
		PrecessionParams = new double[totalProfileFitCoeff+1];

		PrecessionParams[0] = pow(10.0, Cube[pcount]); //Timescale in years
		pcount++;
		printf("Prec time scale %g \n", PrecessionParams[0] );
		for(int i = 0; i < totalProfileFitCoeff; i++){
				PrecessionParams[1+i] = Cube[pcount];
				printf("Prec amp %i %g %g \n", i, Cube[pcount], PrecessionParams[1+i] );

				pcount++;
		
		}
	}

	double *PrecBasis;
	if(incPrecession == 2){

		PrecessionParams = new double[((MNStruct *)globalcontext)->numProfComponents + (((MNStruct *)globalcontext)->numProfComponents-1)];// + (((MNStruct *)globalcontext)->numProfComponents-1)*((MNStruct *)globalcontext)->pulse->nobs];
		PrecBasis = new double[(((MNStruct *)globalcontext)->numProfComponents)*((MNStruct *)globalcontext)->LargestNBins];
		int preccount = 0;

		for(int p2 = 0; p2 < ((MNStruct *)globalcontext)->numProfComponents; p2++){
			PrecessionParams[preccount] = pow(10.0, Cube[pcount]);
			preccount++;
			pcount++;
		}
		for(int p2 = 0; p2 < ((MNStruct *)globalcontext)->numProfComponents-1; p2++){
			PrecessionParams[preccount] = Cube[pcount];
			pcount++;
			preccount++;

		}
		for(int p2 = 0; p2 < (((MNStruct *)globalcontext)->numProfComponents-1)*((MNStruct *)globalcontext)->pulse->nobs; p2++){
		//	PrecessionParams[preccount] = pow(10.0, Cube[pcount]);
		//	preccount++;
		//	pcount++;
		}

	}

	double *TimeCorrProfileParams;
	int incTimeCorrProfileNoise = ((MNStruct *)globalcontext)->incTimeCorrProfileNoise;

	if(incTimeCorrProfileNoise == 1){
		TimeCorrProfileParams = new double[3];
		for(int i = 0; i < 2; i++){
//			printf("TimeCorr: %i %i %g  \n", i, pcount, Cube[pcount]);
			TimeCorrProfileParams[i] = Cube[pcount];
			pcount++;
		}
		//printf("TimeCorr:  %i %g  \n",  pcount, Cube[pcount]);
		TimeCorrProfileParams[2] = pow(10.0, Cube[pcount]);
//		FreqDet += 2*log(Cube[pcount]);
		pcount++;

                for(int i = 0; i < 2; i++){
		//	printf("TimeCorr: %i %g %g \n", i, TimeCorrProfileParams[i], TimeCorrProfileParams[2]);
                        TimeCorrProfileParams[i] = TimeCorrProfileParams[i]*TimeCorrProfileParams[2];
                }


	}
	if(totshapecoeff+1>=totalshapestoccoeff+1){
		maxshapecoeff=totshapecoeff+1;
	}
	if(totalshapestoccoeff+1 > totshapecoeff+1){
		maxshapecoeff=totalshapestoccoeff+1;
	}

//	printf("max shape coeff: %i \n", maxshapecoeff);
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double modelflux=0;
	cpos = 0;
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		for(int j =0; j < numcoeff[i]; j=j+2){
			modelflux+=sqrt(sqrt(M_PI))*sqrt(betas[i])*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*shapecoeff[cpos+j];
		}
		cpos+= numcoeff[i];
	}
	if(debug == 1){printf("model flux: %g \n", modelflux);}
	double ModModelFlux = modelflux;
	double OPLevel = ((MNStruct *)globalcontext)->offPulseLevel;


	double lnew = 0;
	int badshape = 0;

	int GlobalNBins = (int)((MNStruct *)globalcontext)->LargestNBins;

	int ProfileBaselineTerms = ((MNStruct *)globalcontext)->ProfileBaselineTerms;
	int Msize = 1+ProfileBaselineTerms+2*ProfileNoiseCoeff+totalshapestoccoeff;
	int Mcounter=0;
	if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
		Msize++;
	}
	if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
		Msize++;
	}
	if(((MNStruct *)globalcontext)->incWidthJitter > 0){
		Msize++;
	}




	double *MNM = new double[Msize*Msize];

	
	int minep = 0;
	int maxep = ((MNStruct *)globalcontext)->numProfileEpochs;

	int t = 0;
	if(((MNStruct *)globalcontext)->SubIntToFit != -1){
		minep = ((MNStruct *)globalcontext)->SubIntToFit;
		maxep = minep+1;
		for(int ep = 0; ep < minep; ep++){	
			t += ((MNStruct *)globalcontext)->numChanPerInt[ep];
		}
	}
	int nTOA = 0;

	for(int ep = minep; ep < maxep; ep++){
	
		if(debug == 1)printf("In epoch %i \n", ep);

		int EpochNbins = (int)((MNStruct *)globalcontext)->ProfileInfo[t][1];	

		double *M = new double[EpochNbins*Msize];
		double *NM = new double[EpochNbins*Msize];

		double *EpochM;
		double *EpochMNM;
		double *EpochdNM;
		double *EpochTempdNM;
		int EpochMsize = 0;

		int NChanInEpoch = ((MNStruct *)globalcontext)->numChanPerInt[ep];
		int NEpochBins = NChanInEpoch*EpochNbins;
		double *profilenoise = new double[EpochNbins];
		double *Bandprofilenoise = new double[NChanInEpoch*EpochNbins];
		int *NumBinsArray = new int[NChanInEpoch];

		if(((MNStruct *)globalcontext)->incWideBandNoise == 1){

			EpochMsize = (1+ProfileBaselineTerms)*NChanInEpoch+totalshapestoccoeff;
			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
				EpochMsize++;
			}
			if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
				EpochMsize++;
			}
			if(((MNStruct *)globalcontext)->incWidthJitter > 0){
				EpochMsize++;
			}

			EpochM = new double[EpochMsize*NChanInEpoch*EpochNbins]();

			EpochMNM = new double[EpochMsize*EpochMsize]();

			EpochdNM = new double[EpochMsize]();
			EpochTempdNM = new double[EpochMsize]();
		}

		double EpochChisq = 0;	
		double EpochdetN = 0;
		double EpochLike = 0;
		double epochstocsn=0;
		double *ExtraCompSignalVec = new double[GlobalNBins]();
		double *PrecSignalVec = new double[GlobalNBins*((MNStruct *)globalcontext)->numProfComponents]();
		for(int ch = 0; ch < NChanInEpoch; ch++){

	
			//gettimeofday(&tval_before, NULL);

			if(debug == 1)printf("In toa %i \n", t);
			nTOA = t;

			double detN  = 0;
			double Chisq  = 0;
			double logMargindet = 0;
			double Marginlike = 0;	 
			double JitterPrior = 0;	   

			double profilelike=0;

			long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
			long double FoldingPeriodDays = FoldingPeriod/SECDAY;

			int Nbins = GlobalNBins;
			int ProfNbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
			int BinRatio = Nbins/ProfNbins;

			double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
			double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
			long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;


	
			double *MLJitterVec = new double[Nbins]();
			double *shapevec  = new double[Nbins];
			double *ProfileModVec = new double[Nbins]();
			double *ProfileJitterModVec = new double[Nbins]();
		

			double noisemean=0;
			double MLSigma = 0;
			double dataflux = 0;


			double maxamp = 0;
			double maxshape=0;

		    
			long double binpos = ModelBats[nTOA];

			if(((MNStruct *)globalcontext)->incDM==5){
	                	double DMKappa = 2.410*pow(10.0,-16);
        		        double DMScale = 1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[t].freqSSB,2));
				//printf("DMSig %i %g\n", t, SignalVec[ep]*DMScale);
				long double DMshift = (long double)(SignalVec[ep]*DMScale);
				binpos+=DMshift/SECDAY;
			}


			if(incExtraProfComp == 3){
				double ExtraCompDecayScale = 0;
				if(((MNStruct *)globalcontext)->pulse->obsn[t].bat < ExtraCompProps[0]){
                                                   ExtraCompDecayScale = 0;
				}
				else{
					ExtraCompDecayScale = exp((ExtraCompProps[0]-((MNStruct *)globalcontext)->pulse->obsn[t].bat)/ExtraCompProps[3+((MNStruct *)globalcontext)->NumExtraCompCoeffs]);
				}

				long double TimeDelay = (long double) ExtraCompDecayScale*ExtraCompProps[4+((MNStruct *)globalcontext)->NumExtraCompCoeffs];
				 binpos+=TimeDelay/SECDAY;
			}

			if(((MNStruct *)globalcontext)->incDMShapeEvent != 0){

				for(int i =0; i < ((MNStruct *)globalcontext)->incDMShapeEvent; i++){

					int numDMShapeCoeff=((MNStruct *)globalcontext)->numDMShapeCoeff;

					double EventPos = DMShapeCoeffParams[0];
					double EventWidth=DMShapeCoeffParams[1];



					double *DMshapecoeff=new double[numDMShapeCoeff];
					double *DMshapeVec=new double[numDMShapeCoeff];
					for(int c=0; c < numDMShapeCoeff; c++){
						DMshapecoeff[c]=DMShapeCoeffParams[2+c];
					}


					double time=(double)((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat;

					double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
					TNothpl(numDMShapeCoeff,HVal,DMshapeVec);
					double DMsignal=0;
					double DMKappa = 2.410*pow(10.0,-16);
					double DMScale = 1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[nTOA].freqSSB,2));
					for(int c=0; c < numDMShapeCoeff; c++){
						DMsignal += (1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c)))*DMshapeVec[c]*DMshapecoeff[c]*DMScale;
					}

					DMsignal = DMsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
					


					binpos += DMsignal/SECDAY;

					printf("Shapelet Signal: %i %g %g \n", t, time, DMsignal);
					delete[] DMshapecoeff;
					delete[] DMshapeVec;

				}
			}

			if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;
			if(binpos > ProfileBats[nTOA][1])binpos-=FoldingPeriodDays;


			if(binpos > ProfileBats[nTOA][1]){printf("OverBoard! %.10Lg %.10Lg %.10Lg\n", binpos, ProfileBats[nTOA][1], (binpos-ProfileBats[nTOA][1])/FoldingPeriodDays);}

			long double minpos = binpos - FoldingPeriodDays/2;
			if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
			long double maxpos = binpos + FoldingPeriodDays/2;
			if(maxpos> ProfileBats[nTOA][1])maxpos =ProfileBats[nTOA][1];



			int InterpBin = 0;
			double FirstInterpTimeBin = 0;
			int  NumWholeBinInterpOffset = 0;

			if(((MNStruct *)globalcontext)->InterpolateProfile == 1){

		
				long double timediff = 0;
				long double bintime = ProfileBats[t][0];


				if(bintime  >= minpos && bintime <= maxpos){
				    timediff = bintime - binpos;
				}
				else if(bintime < minpos){
				    timediff = FoldingPeriodDays+bintime - binpos;
				}
				else if(bintime > maxpos){
				    timediff = bintime - FoldingPeriodDays - binpos;
				}

				timediff=timediff*SECDAY;

				double OneBin = FoldingPeriod/Nbins;
				int NumBinsInTimeDiff = floor(timediff/OneBin + 0.5);
				double WholeBinsInTimeDiff = NumBinsInTimeDiff*FoldingPeriod/Nbins;
				double OneBinTimeDiff = -1*((double)timediff - WholeBinsInTimeDiff);

				double PWrappedTimeDiff = (OneBinTimeDiff - floor(OneBinTimeDiff/OneBin)*OneBin);

				if(debug == 1)printf("Making InterpBin: %g %g %i %g %g %g\n", (double)timediff, OneBin, NumBinsInTimeDiff, WholeBinsInTimeDiff, OneBinTimeDiff, PWrappedTimeDiff);

				InterpBin = floor(PWrappedTimeDiff/((MNStruct *)globalcontext)->InterpolatedTime+0.5);
				if(InterpBin >= ((MNStruct *)globalcontext)->NumToInterpolate)InterpBin -= ((MNStruct *)globalcontext)->NumToInterpolate;

				FirstInterpTimeBin = -1*(InterpBin-1)*((MNStruct *)globalcontext)->InterpolatedTime;

				if(debug == 1)printf("Interp Time Diffs: %g %g %g %g \n", ((MNStruct *)globalcontext)->InterpolatedTime, InterpBin*((MNStruct *)globalcontext)->InterpolatedTime, PWrappedTimeDiff, InterpBin*((MNStruct *)globalcontext)->InterpolatedTime-PWrappedTimeDiff);

				double FirstBinOffset = timediff-FirstInterpTimeBin;
				double dNumWholeBinOffset = FirstBinOffset/(FoldingPeriod/Nbins);
				int  NumWholeBinOffset = 0;

				NumWholeBinInterpOffset = floor(dNumWholeBinOffset+0.5);
				NumBinsArray[ch] = NumWholeBinInterpOffset;
	
				if(debug == 1)printf("Interp bin is: %i , First Bin is %g, Offset is %i \n", InterpBin, FirstInterpTimeBin, NumWholeBinInterpOffset);


			}

	//gettimeofday(&tval_after, NULL);
	//timersub(&tval_after, &tval_before, &tval_resultone);
	//printf("Time elapsed Up to Evo: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
	//gettimeofday(&tval_before, NULL);


			double reffreq = ((MNStruct *)globalcontext)->EvoRefFreq;
			double freqdiff =  (((MNStruct *)globalcontext)->pulse->obsn[t].freq - reffreq)/1000.0;
			double freqscale = pow(freqdiff, EvoFreqExponent);

			//printf("Evo Exp %i %i %g %g \n", t, EvoFreqExponent, freqdiff, freqscale);

			if(((MNStruct *)globalcontext)->InterpolateProfile == 1){

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Add in any Profile Changes///////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				for(int i =0; i < totalCoeffForMult; i++){
					ProfileModCoeffs[i]=0;	
				}				

				cpos = 0;
				epos = 0;
				fpos = 0;
				for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
					
					if(incPrecession == 0){
						for(int i =0; i < numProfileFitCoeff[c]; i++){
							ProfileModCoeffs[i+cpos] += ProfileFitCoeffs[i+fpos];
						}
						for(int p = 0; p < ((MNStruct *)globalcontext)->NProfileEvoPoly; p++){	
							for(int i =0; i < numEvoCoeff[c]; i++){
								ProfileModCoeffs[i+cpos] += EvoCoeffs[p][i+epos]*pow(freqscale, p+1);
							}
						}


						if(incTimeCorrProfileNoise == 1){
							double time=(double)((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat;

							if(c==4){

								double cosAmp = TimeCorrProfileParams[0]*cos(2*M_PI*(double(0+1)/((MNStruct *)globalcontext)->Tspan)*time);	
								double sinAmp = TimeCorrProfileParams[1]*sin(2*M_PI*(double(0+1)/((MNStruct *)globalcontext)->Tspan)*time);

								ProfileModCoeffs[0+cpos] += cosAmp + sinAmp;
							}

						}
					}

				if(incPrecession == 1){

					double scale = 1;
					if(PrecessionParams[0]/(((MNStruct *)globalcontext)->Tspan/365.25) > 4){
						scale=sin(2*M_PI*(((MNStruct *)globalcontext)->Tspan/365.25)/PrecessionParams[0]);

					}
					double timediff = (((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat - ((MNStruct *)globalcontext)->pulse->obsn[0].bat)/365.25;
					for(int i =0; i < numProfileFitCoeff[c]; i++){

						double MeanCosAmp = ((MNStruct *)globalcontext)->PrecAmps[i+cpos];
						double MeanSinAmp = ((MNStruct *)globalcontext)->PrecAmps[i+cpos+numProfileFitCoeff[c]];
						
						double SinAmp = (MeanSinAmp + PrecessionParams[1+i+fpos])*sin(2*M_PI*timediff/PrecessionParams[0])/scale;
						double CosAmp = (MeanCosAmp + ProfileFitCoeffs[i+fpos])*cos(2*M_PI*timediff/PrecessionParams[0]); 
						ProfileModCoeffs[i+cpos] += SinAmp + CosAmp;

						printf("Prec: %i %g %g %g %g %g %g \n", nTOA, (double)((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat , timediff, PrecessionParams[1+i], ProfileFitCoeffs[i+fpos], SinAmp, CosAmp);	
					}
				}
					cpos += NumCoeffForMult[c];
					epos += numEvoCoeff[c];
					fpos += numProfileFitCoeff[c];	
				}


				

				if(totalCoeffForMult > 0){
					vector_dgemv(((MNStruct *)globalcontext)->InterpolatedShapeletsVec[InterpBin], ProfileModCoeffs,ProfileModVec,Nbins,totalCoeffForMult,'N');

//					if(((MNStruct *)globalcontext)->numFitEQUAD > 0 || ((MNStruct *)globalcontext)->incDMEQUAD > 0 ){
						vector_dgemv(((MNStruct *)globalcontext)->InterpolatedJitterProfileVec[InterpBin], ProfileModCoeffs,ProfileJitterModVec,Nbins,totalCoeffForMult,'N');
//					}
				}
				ModModelFlux = modelflux;

				cpos=0;
				for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
					for(int j =0; j < NumCoeffForMult[c]; j=j+2){
						ModModelFlux+=sqrt(sqrt(M_PI))*sqrt(betas[c])*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*ProfileModCoeffs[cpos+j];
					}
					cpos+= NumCoeffForMult[c];
				}


	//gettimeofday(&tval_after, NULL);
	//timersub(&tval_after, &tval_before, &tval_resultone);
	//printf("Time elapsed,  Evo: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
	//gettimeofday(&tval_before, NULL);


			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Fill Arrays with interpolated state//////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


				
				if(debug == 1){printf("Getting noise levels \n");}
				noisemean=0;
				int numoffpulse = 0;
		                maxshape=0;
				double minshape = pow(10.0, 100);				
				int ZeroWrap = Wrap(0 + NumWholeBinInterpOffset, 0, Nbins-1);

				for(int l = 0; l < 2; l++){
					int startindex = 0;
					int stopindex = 0;
					int step = 0;

					if(l==0){
						startindex = 0; 
						stopindex = Nbins-ZeroWrap;
						step = ZeroWrap;
					}
					else{
						startindex = Nbins-ZeroWrap + BinRatio - (Nbins-ZeroWrap-1)%BinRatio - 1; 
						stopindex = Nbins;
						step = ZeroWrap-Nbins;
					}


					for(int j = startindex; j < stopindex; j+=BinRatio){


						//printf("Filling M: %i %i %i %i %i %i %g \n", t, j, ZeroWrap, stopindex, ProfNbins, BinRatio - (Nbins-ZeroWrap-1)%BinRatio - 1, double(j)/BinRatio);

						double NewIndex = (j + NumWholeBinInterpOffset);
						int Nj =  step+j;//Wrap(j + NumWholeBinInterpOffset, 0, Nbins-1);

						double widthTerm = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj]*(LinearProfileWidth); 

						for(int ew = 0; ew < ((MNStruct *)globalcontext)->incWidthEvoTime; ew++){
							widthTerm += ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj]*LinearWidthEvoTime[ew]*pow( (((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat - 55250)/365.25, ew+1);
						}
						double evoWidthTerm = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj]*EvoProfileWidth*freqscale;
						//double SNWidthTerm = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj]*EvoEnergyProfileWidth*SNscale;

						shapevec[j] = ((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpBin][Nj] + widthTerm + ProfileModVec[Nj] + evoWidthTerm;// + SNWidthTerm;
						MLJitterVec[j] = ProfileJitterModVec[Nj]+((MNStruct *)globalcontext)->InterpolatedJitterProfile[InterpBin][Nj];

					if(incPrecession == 2){



						double CompWidth = PrecessionParams[0];
						long double CompPos = binpos;
						double CompAmp = 1;
						long double timediff = 0;
						long double bintime = ProfileBats[t][0] + (j-1)*(ProfileBats[t][1] - ProfileBats[t][0])/(ProfNbins-1);
						if(bintime  >= minpos && bintime <= maxpos){
						    timediff = bintime - CompPos;
						}
						else if(bintime < minpos){
						    timediff = FoldingPeriodDays+bintime - CompPos;
						}
						else if(bintime > maxpos){
						    timediff = bintime - FoldingPeriodDays - CompPos;
						}

						timediff=timediff*SECDAY;

						double Betatime = timediff/CompWidth;
						double Bconst=(1.0/sqrt(CompWidth))/sqrt(sqrt(M_PI));
						double CompSignal = CompAmp*exp(-0.5*Betatime*Betatime)*Bconst;

//						PrecBasis[j + 0*ProfNbins] = 1;
						PrecBasis[j + 0*ProfNbins] = exp(-0.5*Betatime*Betatime)*Bconst;

						for(int c = 1; c < ((MNStruct *)globalcontext)->numProfComponents; c++){


							double CompWidth = PrecessionParams[c];
							long double CompOffset = PrecessionParams[((MNStruct *)globalcontext)->numProfComponents+c-1]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
							long double CompPos = binpos+CompOffset;
							double CompAmp = 1;//PrecessionParams[((MNStruct *)globalcontext)->numProfComponents+(((MNStruct *)globalcontext)->numProfComponents-1)+(((MNStruct *)globalcontext)->numProfComponents-1)*nTOA+c-1];



//							if(j==0)printf("P2 check: %i %i %i %i %g %g %g \n", nTOA, c, ((MNStruct *)globalcontext)->numProfComponents+c-1, ((MNStruct *)globalcontext)->numProfComponents+(((MNStruct *)globalcontext)->numProfComponents-1)+(((MNStruct *)globalcontext)->numProfComponents-1)*nTOA+c-1, CompWidth, PrecessionParams[((MNStruct *)globalcontext)->numProfComponents+c-1], CompAmp);

							long double timediff = 0;
							long double bintime = ProfileBats[t][0] + (j-1)*(ProfileBats[t][1] - ProfileBats[t][0])/(ProfNbins-1);
							if(bintime  >= minpos && bintime <= maxpos){
							    timediff = bintime - CompPos;
							}
							else if(bintime < minpos){
							    timediff = FoldingPeriodDays+bintime - CompPos;
							}
							else if(bintime > maxpos){
							    timediff = bintime - FoldingPeriodDays - CompPos;
							}

							timediff=timediff*SECDAY;

							double Betatime = timediff/CompWidth;
							double Bconst=(1.0/sqrt(CompWidth))/sqrt(sqrt(M_PI));
							CompSignal += CompAmp*exp(-0.5*Betatime*Betatime)*Bconst;

							PrecBasis[j + c*ProfNbins] = exp(-0.5*Betatime*Betatime)*Bconst;

						}

						shapevec[j] = CompSignal;


					}
					if(incExtraProfComp > 0 && incExtraProfComp < 4){



							long double ExtraCompPos = binpos+ExtraCompProps[1]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
							long double timediff = 0;
							long double bintime = ProfileBats[t][0] + (j-1)*(ProfileBats[t][1] - ProfileBats[t][0])/(ProfNbins-1);
							if(bintime  >= minpos && bintime <= maxpos){
							    timediff = bintime - ExtraCompPos;
							}
							else if(bintime < minpos){
							    timediff = FoldingPeriodDays+bintime - ExtraCompPos;
							}
							else if(bintime > maxpos){
							    timediff = bintime - FoldingPeriodDays - ExtraCompPos;
							}

							timediff=timediff*SECDAY;

							double Betatime = timediff/ExtraCompProps[2];
							
							double ExtraCompDecayScale = 1;

                                                        if(incExtraProfComp == 1){
								if(((MNStruct *)globalcontext)->pulse->obsn[t].bat < ExtraCompProps[0]){
	                                                                ExtraCompDecayScale = 0;
								}
                                                        }

							if(incExtraProfComp == 2){
								ExtraCompDecayScale = exp(-0.5*pow((ExtraCompProps[0]-((MNStruct *)globalcontext)->pulse->obsn[t].bat)/ExtraCompProps[4],2));
							}
							if(incExtraProfComp == 3){
								if(((MNStruct *)globalcontext)->pulse->obsn[t].bat < ExtraCompProps[0]){
                                                                        ExtraCompDecayScale = 0;
                                                                }
                                                                else{
									ExtraCompDecayScale = exp((ExtraCompProps[0]-((MNStruct *)globalcontext)->pulse->obsn[t].bat)/ExtraCompProps[3+((MNStruct *)globalcontext)->NumExtraCompCoeffs]);
								}
                                                        }

							
							TNothpl(((MNStruct *)globalcontext)->NumExtraCompCoeffs, Betatime, ExtraCompBasis);


							double ExtraCompAmp = 0;
							for(int k =0; k < ((MNStruct *)globalcontext)->NumExtraCompCoeffs; k++){
								double Bconst=(1.0/sqrt(betas[0]))/sqrt(pow(2.0,k)*sqrt(M_PI));
								ExtraCompAmp += ExtraCompBasis[k]*Bconst*ExtraCompProps[3+k];

							
							}

							double ExtraCompSignal = ExtraCompAmp*exp(-0.5*Betatime*Betatime)*ExtraCompDecayScale;

							if(fabs(ExtraCompProps[1]) < 0.0005){ExtraCompSignal=0;}

					//		printf("Extra Comp: %i %i %g %g %g %g\n", t, j, Betatime, ExtraCompProps[1], ExtraCompSignal, shapevec[j]);

							shapevec[j] += ExtraCompSignal;
							ExtraCompSignalVec[j] += ExtraCompSignal;

					    }
						Mcounter = 0;
						for(int q = 0; q < ProfileBaselineTerms; q++){
							M[j/BinRatio+Mcounter*ProfNbins] = pow(double(j)/Nbins, q);
							Mcounter++;
						}
					
						M[j/BinRatio + Mcounter*ProfNbins] = shapevec[j];
						if(shapevec[j] > maxshape){ maxshape = shapevec[j];}

						if(shapevec[j] < minshape){ minshape = shapevec[j];}
						Mcounter++;


						for(int q = 0; q < ProfileNoiseCoeff; q++){

							M[j/BinRatio + Mcounter*ProfNbins] =     cos(2*M_PI*(double(q+1)/Nbins)*j);
							M[j/BinRatio + (Mcounter+1)*ProfNbins] = sin(2*M_PI*(double(q+1)/Nbins)*j);

							Mcounter += 2;
						}

				
						if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
							M[j/BinRatio + Mcounter*ProfNbins] = ((MNStruct *)globalcontext)->InterpolatedJitterProfile[InterpBin][Nj] + ProfileJitterModVec[Nj];

							Mcounter++;
						}		

						if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
							double DMKappa = 2.410*pow(10.0,-16);
							double DMScale = 1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[t].freqSSB,2));
							M[j/BinRatio + Mcounter*ProfNbins] = (((MNStruct *)globalcontext)->InterpolatedJitterProfile[InterpBin][Nj] + ProfileJitterModVec[Nj])*DMScale;
							Mcounter++;
						}	


						if(((MNStruct *)globalcontext)->incWidthJitter > 0){
							M[j/BinRatio + Mcounter*ProfNbins] = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpBin][Nj];
							Mcounter++;
						}

						int stocpos=0;
						int savepos=0;
						for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
							for(int k = 0; k < numProfileStocCoeff[c]; k++){
							    M[j/BinRatio + (Mcounter+k+stocpos)*ProfNbins] = ((MNStruct *)globalcontext)->InterpolatedShapeletsVec[InterpBin][Nj + (k+savepos)*Nbins];
							}
							stocpos+=numProfileStocCoeff[c];
							savepos+=numShapeToSave[c];

						}	
					}
				}


				if(incPrecession == 2){

					double *D = new double[ProfNbins];

					for(int b = 0; b < ProfNbins; b++){
						D[b] = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][b][1];
					}

					((MNStruct *)globalcontext)->PrecBasis = PrecBasis;
					((MNStruct *)globalcontext)->PrecD = D;
					((MNStruct *)globalcontext)->PrecN = ((MNStruct *)globalcontext)->pulse->obsn[nTOA].snr;


					double *Phys=new double[((MNStruct *)globalcontext)->numProfComponents];
					
					SmallNelderMeadOptimum(((MNStruct *)globalcontext)->numProfComponents, Phys);
					double sfac = pow(10.0, Phys[0]);
					for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
						Phys[c] = pow(10.0, Phys[c]);
						Phys[c] = Phys[c]/sfac;
						printf("Max Amps: %i %i %g \n", t, c, Phys[c]);
					}
					

					double *ML = new double[ProfNbins];
					vector_dgemv(PrecBasis, Phys, ML, ProfNbins, ((MNStruct *)globalcontext)->numProfComponents, 'N');

					for(int b = 0; b < ProfNbins; b++){
						
						M[b + 1*ProfNbins] = ML[b];
					}

					delete[] Phys;
					delete[] D;
					delete[] ML;
					
				}

				maxshape-=minshape;
				if((shapevec[0]-minshape) < maxshape*OPLevel){
					int b = 0;
					while(fabs(shapevec[b*BinRatio]-minshape) < maxshape*OPLevel){
						noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][b][1];
						numoffpulse += 1;
						b++;
					}
					if(debug == 1){printf("Offpulse one A %i %i\n", numoffpulse,b);}
					b=ProfNbins-1;
					while(fabs(shapevec[b*BinRatio]-minshape) < maxshape*OPLevel){
						noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][b][1];
						numoffpulse += 1;
						b--;
					}
					if(debug == 1){printf("Offpulse two A %i %i\n", numoffpulse,b);}
				}else{
					int b=ProfNbins/2;
					while(fabs(shapevec[b*BinRatio]-minshape) < maxshape*OPLevel){
                                                noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][b][1];
                                                numoffpulse += 1;
                                                b++;
                                        }
					if(debug == 1){printf("Offpulse one B %i %i\n", numoffpulse,b);}
                                        b=ProfNbins/2-1;
					while(fabs(shapevec[b*BinRatio]-minshape) < maxshape*OPLevel){
                                                noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][b][1];
                                                numoffpulse += 1;
                                                b--;
                                        }
                                        if(debug == 1){printf("Offpulse two B %i %i %g %g \n", numoffpulse,b, shapevec[(b+1)*BinRatio], maxshape*OPLevel);}
                                }
				if(debug == 1)printf("noise mean %g num off pulse %i\n", noisemean, numoffpulse);

	//gettimeofday(&tval_after, NULL);
	//timersub(&tval_after, &tval_before, &tval_resultone);
	//printf("Time elapsed,  FillArrays: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
	//gettimeofday(&tval_before, NULL);

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Get Noise Level//////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				noisemean = noisemean/(numoffpulse);

				MLSigma = 0;
				int MLSigmaCount = 0;
				dataflux = 0;




				if(shapevec[0]-minshape < maxshape*OPLevel){
					int b = 0;
					while(fabs(shapevec[b*BinRatio]-minshape) < maxshape*OPLevel){
						double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][b][1] - noisemean;
                                                MLSigma += res*res; MLSigmaCount += 1;

						b++;
					}
//					printf("Offpulse one %i %i\n", numoffpulse,b);
					b=ProfNbins-1;
					while(fabs(shapevec[b*BinRatio]-minshape) < maxshape*OPLevel){
						double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][b][1] - noisemean;
                                                MLSigma += res*res; MLSigmaCount += 1;

						b--;
					}
//					printf("Offpulse two %i %i\n", numoffpulse,b);
				}else{
					int b=ProfNbins/2;
					while(fabs(shapevec[b*BinRatio]-minshape) < maxshape*OPLevel){
						double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][b][1] - noisemean;
                                                MLSigma += res*res; MLSigmaCount += 1;

                                                b++;
                                        }
//					printf("Offpulse one %i %i\n", numoffpulse,b);
                                        b=ProfNbins/2-1;
					while(fabs(shapevec[b*BinRatio]-minshape) < maxshape*OPLevel){

						double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][b][1] - noisemean;
						MLSigma += res*res; MLSigmaCount += 1;
                                                b--;
                                        }
//                                        printf("Offpulse two %i %i %g %g \n", numoffpulse,b, shapevec[(b+1)*BinRatio], maxshape*OPLevel);
                                }

				MLSigma = sqrt(MLSigma/MLSigmaCount);

				if(((MNStruct *)globalcontext)->ProfileNoiseMethod == 1){
					MLSigma=((MNStruct *)globalcontext)->pulse->obsn[nTOA].snr;
				}

				MLSigma = MLSigma*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]]*((MNStruct *)globalcontext)->MLProfileNoise[ep][0];

				dataflux = 0;
				for(int j = 0; j < ProfNbins; j++){
					if(shapevec[j*BinRatio] >= maxshape*OPLevel){
						if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
							dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-noisemean)*double((ProfileBats[t][1] - ProfileBats[t][0])/(ProfNbins-1))*60*60*24;
						}
					}
				}

				maxamp = dataflux/ModModelFlux;

				if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g \n", MLSigma, dataflux, ModModelFlux, maxamp);

	//gettimeofday(&tval_after, NULL);
	//timersub(&tval_after, &tval_before, &tval_resultone);
	//printf("Time elapsed,  Get Noise: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
	//gettimeofday(&tval_before, NULL);

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Rescale Basis Vectors////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


				Mcounter = 1+ProfileBaselineTerms+ProfileNoiseCoeff*2;
				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
					for(int j =0; j < ProfNbins; j++){
						M[j + Mcounter*ProfNbins] = M[j + Mcounter*ProfNbins]*dataflux/ModModelFlux;
					}			
					Mcounter++;
				}
				if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
					for(int j =0; j < ProfNbins; j++){
						M[j + Mcounter*ProfNbins] = M[j + Mcounter*ProfNbins]*dataflux/ModModelFlux;
					}			
					Mcounter++;
				}
				if(((MNStruct *)globalcontext)->incWidthJitter > 0){
					for(int j =0; j < ProfNbins; j++){
						M[j + Mcounter*ProfNbins] = M[j + Mcounter*ProfNbins]*dataflux/ModModelFlux;
					}
					Mcounter++;
				}
				for(int k = 0; k < totalshapestoccoeff; k++){
					for(int j =0; j < ProfNbins; j++){
				   		M[j + (Mcounter+k)*ProfNbins] = M[j + (Mcounter+k)*ProfNbins]*dataflux;
					}
				}

				if(((MNStruct *)globalcontext)->incWideBandNoise == 1){
					for(int j =0; j < ProfNbins; j++){
						for(int k = 0; k < 1+ProfileBaselineTerms; k++){
							EpochM[ch*ProfNbins+j +    (ch*(1+ProfileBaselineTerms)+k)*NChanInEpoch*ProfNbins]  = M[j + k*ProfNbins];
						}
						for(int k = 0; k < Msize-(1+ProfileBaselineTerms); k++){
							EpochM[ch*ProfNbins+j + ((1+ProfileBaselineTerms)*NChanInEpoch+k)*NChanInEpoch*ProfNbins] = M[j + (1+ProfileBaselineTerms+k)*ProfNbins];
						}
					}
				}



		    }

	
		    double *NDiffVec = new double[ProfNbins];

		if(debug == 1)printf("Rescaled BAsis Vectors \n");

	
		///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


			      
			Chisq = 0;
			detN = 0;
			double OffPulsedetN = log(MLSigma*MLSigma);
			double OneDetN = 0;
			double logconvfactor = 1.0/log2(exp(1));

	
	 
			double highfreqnoise = HighFreqStoc1;
			double flatnoise = (HighFreqStoc2*maxamp*maxshape)*(HighFreqStoc2*maxamp*maxshape);
			
			
			for(int i =0; i < ProfNbins; i++){
			  	Mcounter=2;
				double noise = MLSigma*MLSigma;

				OneDetN=OffPulsedetN;
				if(shapevec[i*BinRatio] > maxshape*OPLevel && ((MNStruct *)globalcontext)->incHighFreqStoc > 0){
					noise +=  flatnoise + (highfreqnoise*maxamp*shapevec[i*BinRatio])*(highfreqnoise*maxamp*shapevec[i*BinRatio]);
					OneDetN=log2(noise)*logconvfactor;
				}

				detN += OneDetN;
				profilenoise[i] = sqrt(noise);
				Bandprofilenoise[ch*ProfNbins + i] = sqrt(noise);
				noise=1.0/noise;
				
				double datadiff =  ((MNStruct *)globalcontext)->ProfileData[nTOA][i][1];
//				printf("Data: %i %g %g \n",i,  datadiff, noise);
				NDiffVec[i] = datadiff*noise;

				Chisq += datadiff*NDiffVec[i];
	

				
				for(int j = 0; j < Msize; j++){
					NM[i + j*ProfNbins] = M[i + j*ProfNbins]*noise;
				}

				

			}


	//gettimeofday(&tval_after, NULL);
	//timersub(&tval_after, &tval_before, &tval_resultone);
	//printf("Time elapsed,  Up to LA: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);
	//gettimeofday(&tval_before, NULL);

			vector_dgemm(M, NM , MNM, ProfNbins, Msize,ProfNbins, Msize, 'T', 'N');


			double *dNM = new double[Msize];
			double *TempdNM = new double[Msize];
			vector_dgemv(M,NDiffVec,dNM,ProfNbins,Msize,'T');

		
			if(((MNStruct *)globalcontext)->incWideBandNoise == 1){

				int ppterms = 1+ProfileBaselineTerms;

				for(int j = 0; j < ppterms; j++){
					EpochdNM[ppterms*ch+j] = dNM[j];
					for(int k = 0; k < ppterms; k++){
						EpochMNM[ppterms*ch+j + (ppterms*ch+k)*EpochMsize] = MNM[j + k*Msize];
					}
				}

				for(int j = 0; j < Msize-ppterms; j++){

					EpochdNM[ppterms*NChanInEpoch+j] += dNM[ppterms+j];

				//	for(int k = 0; k < ppterms; k++){

						for(int q = 0; q < ppterms; q++){
							EpochMNM[ppterms*ch+q + (ppterms*NChanInEpoch+j)*EpochMsize] = MNM[q + (ppterms+j)*Msize];
							//EpochMNM[ppterms*ch+1 + (ppterms*NChanInEpoch+j)*EpochMsize] = MNM[1 + (ppterms+j)*Msize];

							EpochMNM[ppterms*NChanInEpoch+j + (ppterms*ch+q)*EpochMsize] = MNM[ppterms+j + q*Msize];
							//EpochMNM[ppterms*NChanInEpoch+j + (ppterms*ch+1)*EpochMsize] = MNM[ppterms+j + 1*Msize];
						}
				//	}

					for(int k = 0; k < Msize-ppterms; k++){

						EpochMNM[ppterms*NChanInEpoch+j + (ppterms*NChanInEpoch+k)*EpochMsize] += MNM[ppterms+j + (ppterms+k)*Msize];
					}
				}
			}


			if(debug == 1)printf("Made MNM \n");

			double Margindet = 0;
			double StocProfDet = 0;
			double JitterDet = 0;
			if(((MNStruct *)globalcontext)->incWideBandNoise == 0){


				Mcounter=1+ProfileBaselineTerms;

				if(((MNStruct *)globalcontext)->incProfileNoise == 1 || ((MNStruct *)globalcontext)->incProfileNoise == 2){
					for(int q = 0; q < ProfileNoiseCoeff; q++){
						double profnoise = ProfileNoiseAmps[q];
						JitterDet += 2*log(profnoise);
						MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
						Mcounter++;
						MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
						Mcounter++;
					}
				}

				if(((MNStruct *)globalcontext)->incProfileNoise == 3 || ((MNStruct *)globalcontext)->incProfileNoise == 4){
					for(int q = 0; q < ProfileNoiseCoeff; q++){
						double profnoise = ProfileNoiseAmps[q]*((MNStruct *)globalcontext)->MLProfileNoise[ep][q+1];
						JitterDet += 2*log(profnoise);
						MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
						Mcounter++;
						MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
						Mcounter++;
					}
				}
				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){

					double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
					profnoise = profnoise*profnoise;
				    	JitterDet += log(profnoise);
				    	MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
					Mcounter++;
				}

				if(((MNStruct *)globalcontext)->incDMEQUAD > 0){

					double profnoise = DMEQUAD;
					profnoise = profnoise*profnoise;
				    	JitterDet += log(profnoise);
				    	MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
					Mcounter++;
				}


				if(((MNStruct *)globalcontext)->incWidthJitter > 0){

					double profnoise = WidthJitter;
					profnoise = profnoise*profnoise;
				    	JitterDet += log(profnoise);
				    	MNM[Mcounter + Mcounter*Msize] += 1.0/profnoise;
					Mcounter++;
				}
				
				for(int j = 0; j < totalshapestoccoeff; j++){
					StocProfDet += log(StocProfCoeffs[j]);
					MNM[Mcounter+j + (Mcounter+j)*Msize] +=  1.0/StocProfCoeffs[j];
				}


				for(int i =0; i < Msize; i++){
					TempdNM[i] = dNM[i];
				}


				int info=0;

				double *MNM2=new double[Msize*Msize]();
				for(int i = 0; i < Msize*Msize; i++){
					MNM2[i] = MNM[i];
				}
				
				vector_dpotrfInfo(MNM, Msize, Margindet, info);
				vector_dpotrsInfo(MNM, TempdNM, Msize, info);

				vector_dpotrfInfo(MNM2, Msize, Margindet, info);
	                        vector_dpotri(MNM2, Msize);

					    
				logMargindet = Margindet;

				Marginlike = 0;
				for(int i =0; i < Msize; i++){
					Marginlike += TempdNM[i]*dNM[i];
					//printf("one dnm: %i %i %g\n", ep, ch, TempdNM[i]);
				}


				double finaloffset = TempdNM[0];
				double finalamp = TempdNM[ProfileBaselineTerms];
				double finalJitter = 0;
				double finalWidthJitter = 0;
				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
					finalJitter = TempdNM[1+ProfileBaselineTerms];
					printf("FJ: %s %i %i %i %g %g %g\n",((MNStruct *)globalcontext)->pulse->obsn[t].fname,  t, ep, ch, finalJitter, sqrt(MNM2[1+ProfileBaselineTerms + Msize*(1+ProfileBaselineTerms)]), finalJitter/sqrt(MNM2[1+ProfileBaselineTerms + Msize*(1+ProfileBaselineTerms)]));
                                }


				if(((MNStruct *)globalcontext)->incWidthJitter > 0){
					finalWidthJitter = TempdNM[1+ProfileBaselineTerms];
					printf("FWJ: %i %g\n", t, finalWidthJitter);
				}
				double *StocVec = new double[ProfNbins];
				double *Jittervec = new double[ProfNbins];
				double *MLShapeVec  = new double[ProfNbins];

				for(int i =0; i < ProfNbins; i++){
					if(((MNStruct *)globalcontext)->numFitEQUAD > 0)Jittervec[i] = M[i + (1+ProfileBaselineTerms)*ProfNbins];
					if(((MNStruct *)globalcontext)->numFitEQUAD == 0)Jittervec[i] = pow(10.0, -5)*MLJitterVec[i*BinRatio]*dataflux/ModModelFlux;
//					if(t==0){printf("p: %i %g %g %g  %g\n", i, M[i+0*Nbins], M[i+1*Nbins], M[i+2*Nbins], M[i+3*Nbins]);}
				}
				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){TempdNM[1+ProfileBaselineTerms] = 0;}
				vector_dgemv(M,TempdNM,MLShapeVec,ProfNbins,Msize,'N');

		
				for(int q = 0; q < 1+ProfileBaselineTerms; q++){
					TempdNM[q] = 0;
				}
				if(((MNStruct *)globalcontext)->numFitEQUAD > 0)TempdNM[1+ProfileBaselineTerms] = 0;
		

				vector_dgemv(M,TempdNM,StocVec,ProfNbins,Msize,'N');

				double StocSN = 0;
				double DataStocSN = 0;
				if(debug == 1)printf("Writing to File \n");
				if(writeprofiles == 1){
					std::ofstream profilefile;
					char toanum[15];
					sprintf(toanum, "%d", nTOA);
					std::string ProfileName =  toanum;
					std::string dname = longname+ProfileName+"-Profile.txt";
	
					profilefile.open(dname.c_str());
					double MLChisq = 0;
					profilefile << "#" << std::setprecision(20) << ((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat << " " << ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0] << " " << ((MNStruct *)globalcontext)->pulse->obsn[nTOA].freq <<"\n";
					for(int i =0; i < ProfNbins; i++){
						StocSN += StocVec[i]*StocVec[i]/profilenoise[i]/profilenoise[i];
					}
					if(debug == 1)printf("StocSN %g \n", StocSN);

					for(int i =0; i < Nbins; i+=BinRatio){

						int Nj =  Wrap(i+Nbins/2 + NumWholeBinInterpOffset, 0, Nbins-1);
						
						MLChisq += pow(((double)((MNStruct *)globalcontext)->ProfileData[nTOA][i/BinRatio][1] - MLShapeVec[i/BinRatio])/profilenoise[i/BinRatio], 2);


						profilefile << Nj << " " << std::setprecision(10) << (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i/BinRatio][1] << " " << MLShapeVec[i/BinRatio] << " " << profilenoise[i/BinRatio] << " " << finaloffset+finalamp*M[i/BinRatio + ProfileBaselineTerms*ProfNbins] << " " << Jittervec[i/BinRatio] << " " << finaloffset << " " << StocVec[i/BinRatio] << " " << M[i/BinRatio + ProfileBaselineTerms*ProfNbins];

						if(incExtraProfComp > 0){
							profilefile << " " << 	finaloffset+finalamp*ExtraCompSignalVec[i] << " " << sqrt(MNM2[ProfileBaselineTerms + Msize*(ProfileBaselineTerms)]);
						}

						if(incPrecession == 2){
							for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
								profilefile << " " << finaloffset+finalamp*PrecSignalVec[c*Nbins+i];
							}
						}

						profilefile << "\n";


					}
			    		profilefile.close();
					epochstocsn+=StocSN;

				delete[] MNM2;
				}
				
				if(writeascii == 1){

					std::ofstream profilefile;
		

					std::string fname = ((MNStruct *)globalcontext)->pulse->obsn[t].fname;
					std::string dname = fname+".ASCII";
	
					profilefile.open(dname.c_str());
					//profilefile << "#" << std::setprecision(20) << ((MNStruct *)globalcontext)->pulse->obsn[t].sat_day << " " << (((MNStruct *)globalcontext)->pulse->obsn[t].sat_sec+((MNStruct *)globalcontext)->pulse->obsn[nTOA].residual/SECDAY)*SECDAY << " " << ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0] << " 1 " << ((MNStruct *)globalcontext)->pulse->obsn[nTOA].freq << " " << ((MNStruct *)globalcontext)->pulse->param[param_dm].val[0] << " " << ProfNbins <<  " 7 1 " << ((MNStruct *)globalcontext)->pulse->name << " 0.0000000 \n";
					//printf("binpos %i %g %g %g \n", t, (double)binpos-((MNStruct *)globalcontext)->pulse->obsn[nTOA].batCorr, (double)(binpos-((MNStruct *)globalcontext)->pulse->obsn[nTOA].batCorr)*SECDAY, (double)((MNStruct *)globalcontext)->pulse->obsn[t].sat_sec*SECDAY);
					for(int i =0; i < Nbins; i+=BinRatio){

						profilefile << i/BinRatio << " " << std::setprecision(10) << MLShapeVec[i/BinRatio]  << "\n";


					}
			    		profilefile.close();
				}
				delete[] StocVec;
				delete[] Jittervec;
				delete[] MLShapeVec;

			}


			
			
			profilelike = -0.5*(detN + Chisq + logMargindet + StocProfDet + JitterDet - Marginlike);
			//lnew += profilelike;
			if(debug == 1)printf("Like: %i %.15g %.15g %.15g %.15g %.15g %.15g %.15g\n", nTOA,lnew, detN, Chisq, logMargindet, Marginlike, StocProfDet , JitterDet);

			EpochChisq+=Chisq;
			EpochdetN+=detN;
			EpochLike+=profilelike;

			delete[] shapevec;
			delete[] MLJitterVec;
			delete[] ProfileModVec;
			delete[] ProfileJitterModVec;
			delete[] NDiffVec;
			delete[] dNM;
			delete[] TempdNM;


	//gettimeofday(&tval_after, NULL);
	//timersub(&tval_after, &tval_before, &tval_resultone);
	//printf("Time elapsed,  LA: %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec);

			t++;

	
		}

		if(incExtraProfComp > 0){
//			delete[] ExtraCompProps;
//			delete[] ExtraCompBasis;
//			delete[] ExtraCompSignalVec;
		}

///////////////////////////////////////////


		if(((MNStruct *)globalcontext)->incWideBandNoise == 0){

			lnew += EpochLike;
		}

		if(((MNStruct *)globalcontext)->incWideBandNoise == 1){

			for(int i =0; i < EpochMsize; i++){
				EpochTempdNM[i] = EpochdNM[i];
			}

			int EpochMcount=0;
			if(((MNStruct *)globalcontext)->incProfileNoise == 0){EpochMcount = (1+ProfileBaselineTerms)*NChanInEpoch;}

			double BandJitterDet = 0;

			if(((MNStruct *)globalcontext)->incProfileNoise == 1 || ((MNStruct *)globalcontext)->incProfileNoise == 2){
				for(int ch = 0; ch < ((MNStruct *)globalcontext)->numChanPerInt[ep]; ch++){
					
					EpochMcount += 1+ProfileBaselineTerms;
					for(int q = 0; q < ProfileNoiseCoeff; q++){
						if(debug == 1){printf("PN BW %i %i %i %i %g %g \n", ep, ch, EpochMcount, q, EpochMNM[EpochMcount + EpochMcount*EpochMsize], EpochMNM[EpochMcount+1 + (EpochMcount+1)*EpochMsize]);}
						double profnoise = ProfileNoiseAmps[q];
						BandJitterDet += 2*log(profnoise);
						EpochMNM[EpochMcount + EpochMcount*EpochMsize] += 1.0/profnoise;
						EpochMcount++;
						EpochMNM[EpochMcount + EpochMcount*EpochMsize] += 1.0/profnoise;
						EpochMcount++;
					}
				}
			}

			if(((MNStruct *)globalcontext)->incProfileNoise == 3 || ((MNStruct *)globalcontext)->incProfileNoise == 4){
				for(int ch = 0; ch < ((MNStruct *)globalcontext)->numChanPerInt[ep]; ch++){
					
					EpochMcount += 1+ProfileBaselineTerms;
					for(int q = 0; q < ProfileNoiseCoeff; q++){
						if(debug == 1){printf("PN BW %i %i %i %i %g %g \n", ep, ch, EpochMcount, q, EpochMNM[EpochMcount + EpochMcount*EpochMsize], EpochMNM[EpochMcount+1 + (EpochMcount+1)*EpochMsize]);}
						double profnoise = ((MNStruct *)globalcontext)->MLProfileNoise[ep][q+1];
						BandJitterDet += 2*log(profnoise);
						EpochMNM[EpochMcount + EpochMcount*EpochMsize] += 1.0/profnoise;
						EpochMcount++;
						EpochMNM[EpochMcount + EpochMcount*EpochMsize] += 1.0/profnoise;
						EpochMcount++;
					}
				}
			}

			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){

				double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[0]];
				profnoise = profnoise*profnoise;
			    	BandJitterDet += log(profnoise);

				EpochMNM[EpochMcount + (EpochMcount)*EpochMsize] += 1.0/profnoise;
				EpochMcount++;
			}

			if(((MNStruct *)globalcontext)->incDMEQUAD > 0){

				double profnoise = DMEQUAD;
				profnoise = profnoise*profnoise;
			    	BandJitterDet += log(profnoise);

				EpochMNM[EpochMcount + (EpochMcount)*EpochMsize] += 1.0/profnoise;
				EpochMcount++;
			}

			if(((MNStruct *)globalcontext)->incWidthJitter > 0){

				double profnoise = WidthJitter;
				profnoise = profnoise*profnoise;
			    	BandJitterDet += log(profnoise);

				EpochMNM[EpochMcount + (EpochMcount)*EpochMsize] += 1.0/profnoise;
				EpochMcount++;
			}

			double BandStocProfDet = 0;
			for(int j = 0; j < totalshapestoccoeff; j++){
				BandStocProfDet += log(StocProfCoeffs[j]);
				EpochMNM[EpochMcount+j + (EpochMcount+j)*EpochMsize] += 1.0/StocProfCoeffs[j];
			}

			int Epochinfo=0;
			double EpochMargindet = 0;
			vector_dpotrfInfo(EpochMNM, EpochMsize, EpochMargindet, Epochinfo);
			vector_dpotrsInfo(EpochMNM, EpochTempdNM, EpochMsize, Epochinfo);

				double *finaloffsets = new double[NChanInEpoch];
				double *finalamps = new double[NChanInEpoch];
				double *StocVec = new double[EpochNbins*NChanInEpoch];
				double *maxvec  = new double[EpochNbins*NChanInEpoch];
				double *Jittervec = new double[EpochNbins*NChanInEpoch];

				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
					for(int q = 0; q < NChanInEpoch*EpochNbins; q++){
						Jittervec[q] = EpochM[q + (1+ProfileBaselineTerms+2*ProfileNoiseCoeff)*NChanInEpoch*EpochNbins];
					}
				}

				double finalJitter = 0;

				vector_dgemv(EpochM,EpochTempdNM,maxvec,NChanInEpoch*EpochNbins,EpochMsize,'N');

		
				for(int q = 0; q < NChanInEpoch; q++){
					finaloffsets[q]=EpochTempdNM[(1+ProfileBaselineTerms+2*ProfileNoiseCoeff)*q];
					finalamps[q] = EpochTempdNM[(1+ProfileBaselineTerms+2*ProfileNoiseCoeff)*q + 1];
					for(int qq = 0; qq < 1+ProfileBaselineTerms+2*ProfileNoiseCoeff; qq++){
						EpochTempdNM[(1+ProfileBaselineTerms+2*ProfileNoiseCoeff)*q + qq] = 0;
					}
				}
				printf("One Stoc Max: %i %g %g\n", ep, EpochTempdNM[EpochMsize-1],EpochTempdNM[EpochMsize-2]);
				//if(((MNStruct *)globalcontext)->numFitEQUAD > 0)EpochTempdNM[(1+ProfileBaselineTerms+2*ProfileNoiseCoeff)*NChanInEpoch] = 0;
		

				vector_dgemv(EpochM,EpochTempdNM,StocVec,NChanInEpoch*EpochNbins,EpochMsize,'N');

			if(writeprofiles == 1){
				std::ofstream EAProfileFile;
				char EPnum[15];
				sprintf(EPnum, "%d", ep);
				std::string EPString = EPnum;
				std::string EAPname = longname+EPString+"-EAProfile.txt";
				EAProfileFile.open(EAPname.c_str());
				double *AverageProfile = new double[EpochNbins]();
				double *AverageModel = new double[EpochNbins]();
				double *AverageStoc = new double[EpochNbins]();

				double weightsum = 0;			

				int currentnToA = t-NChanInEpoch-1;
				//printf("currentNTOA: %s %i %i \n", ((MNStruct *)globalcontext)->pulse->obsn[currentnToA].fname, t, currentnToA);

				for(int j = 0; j < NChanInEpoch; j++){
					double StocSN = 0;
					double ProfSN = 0;
					currentnToA++;
					std::ofstream profilefile;
					char toanum[15];
					
					sprintf(toanum, "%d", currentnToA);
					std::string ProfileName =  toanum;
					std::string dname = longname+ProfileName+"-Profile.txt";

					profilefile.open(dname.c_str());
					double MLChisq = 0;

					double weight = 0;
					for(int i =0; i < EpochNbins; i++){
						weight += Bandprofilenoise[j*EpochNbins + i];
					}
					weight = 1.0/(weight/EpochNbins);
					weight = weight*weight;
				//	 printf("currentNTOA In: %i %i \n", t, currentnToA);

					for(int i =0; i < GlobalNBins; i++){
						int Nj =  Wrap(i+GlobalNBins/2 + NumBinsArray[j], 0, GlobalNBins-1);

						StocSN += StocVec[j*GlobalNBins + i]*StocVec[j*GlobalNBins + i];
						ProfSN += finalamps[j]*EpochM[j*GlobalNBins+i +(j*(1+ProfileBaselineTerms)+1)*NChanInEpoch*GlobalNBins]*finalamps[j]*EpochM[j*GlobalNBins+i +(j*(1+ProfileBaselineTerms)+1)*NChanInEpoch*GlobalNBins];
						//MLChisq += pow(((double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1] - maxvec[j*GlobalNBins + i])/Bandprofilenoise[j*GlobalNBins + i], 2);
						profilefile << Nj  << " " << std::setprecision(10) << (double)((MNStruct *)globalcontext)->ProfileData[currentnToA][i][1] << " " << maxvec[j*GlobalNBins + i] << " " << Bandprofilenoise[j*GlobalNBins + i] << " " << finaloffsets[j] + finalamps[j]*EpochM[j*GlobalNBins+i +    (j*(1+ProfileBaselineTerms)+1)*NChanInEpoch*GlobalNBins] << " " << finalJitter*Jittervec[i] << " " << StocVec[j*GlobalNBins + i] << "\n";

						AverageProfile[Nj] += (double)((MNStruct *)globalcontext)->ProfileData[currentnToA][i][1]*weight;
						AverageModel[Nj] += maxvec[j*GlobalNBins + i]*weight;
						AverageStoc[Nj] += StocVec[j*GlobalNBins + i]*weight;
	//					if(ep == 45){printf("JV: %i %i %g \n", j, i, Jittervec[j*GlobalNBins + i]);}
					}
					weightsum+=weight;

			    		profilefile.close();
					epochstocsn+=StocSN/ProfSN;
				}
				double stocSum = 0;
				double profsn = 0;
				double AvRMS = 0;
				for(int i =0; i < EpochNbins; i++){
					AvRMS += (AverageProfile[i]/weightsum - AverageModel[i]/weightsum)*(AverageProfile[i]/weightsum - AverageModel[i]/weightsum);
				}
				AvRMS= sqrt(AvRMS/EpochNbins);
				for(int i =0; i < EpochNbins; i++){
					if(fabs(AverageStoc[i]/weightsum)/AvRMS > 0.5){
						stocSum +=  (AverageStoc[i]/fabs(AverageStoc[i]))*((AverageProfile[i]-AverageModel[i]+AverageStoc[i])/weightsum)/AvRMS;
					}
					EAProfileFile << i  << " " << std::setprecision(10) << AverageProfile[i]/weightsum << " " << AverageModel[i]/weightsum << " " << AverageStoc[i]/weightsum << "\n";
				}
				//printf("stoc sum: %i  %g %g\n", ep, stocSum, AvRMS, profsn);
				EAProfileFile.close();

				delete[] AverageProfile;
				delete[] AverageModel;
				delete[] AverageStoc;
			}

			if(debug == 1){printf("deallocating 1\n");}
			delete[] profilenoise;
			delete[] Bandprofilenoise;
			if(debug == 1){printf("deallocating 2\n");}
			delete[] StocVec;
			delete[] finaloffsets;
			if(debug == 1){printf("deallocating 3\n");}
			delete[] finalamps;
			delete[] Jittervec;
			delete[] maxvec;
			delete[] NumBinsArray;

			double EpochProfileLike = 0;//-0.5*(EpochdetN + EpochChisq + EpochMargindet - EpochMarginlike + BandJitterDet + BandStocProfDet);
			lnew += EpochProfileLike;
			//if(debug == 1){printf("Like: %i %g %g %g %g %g %g \n",ep,  EpochdetN , EpochChisq ,EpochMargindet , EpochMarginlike , BandJitterDet , BandStocProfDet);}

			delete[] EpochMNM;
			delete[] EpochdNM;
			if(debug == 1){printf("deallocating 4\n");}
			delete[] EpochTempdNM;
			delete[] EpochM;
			delete[] M;
			delete[] NM;
		
			if(debug == 1){printf("deallocating done\n");}

		}

////////////////////////////////////////////
	
	}
	 
	 



	//for (int j = 0; j < Msize; j++){
	//    delete[] MNM[j];
	//}
	delete[] MNM;
	 

	if(((MNStruct *)globalcontext)->incDM > 0){
		delete[] SignalVec;
	}
	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] numcoeff;
	delete[] NumCoeffForMult;
	delete[] EvoCoeffs;
	delete[] ProfileNoiseAmps;
	delete[] betas;

	lnew += uniformpriorterm - 0.5*(FreqLike + FreqDet);
	if(debug == 0)printf("End Like: %.10g \n", lnew);
	//printf("End Like: %.10g \n", lnew);
	//}

//	if(((MNStruct *)globalcontext)->incDM > 0){
		//WriteDMSignal(longname, ndim);
//	}
	//WriteProfileFreqEvo(longname, ndim,profiledimstart);


}

*/

/*
void  WriteDMSignal(std::string longname, int &ndim){


	int InterpIndex = 0;
        double *Cube = new double[ndim];
        int number_of_lines = 0;

        std::ifstream checkfile;
	std::string checkname;
	if(((MNStruct *)globalcontext)->sampler == 0){
	        checkname = longname+".txt";
	}
        if(((MNStruct *)globalcontext)->sampler == 1){
                checkname = longname+".txt";
        }
	//printf("%i %s \n", ((MNStruct *)globalcontext)->sampler, checkname.c_str());
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        checkfile.close();

        std::ifstream summaryfile;
	std::string fname;
	if(((MNStruct *)globalcontext)->sampler == 0){
		fname = longname+".txt";
	}
	if(((MNStruct *)globalcontext)->sampler == 1){
	        fname = longname+".txt";
	}

        summaryfile.open(fname.c_str());


	double *DMSignal = new double[((MNStruct *)globalcontext)->numProfileEpochs]();

	double *DMSignalErr = new double[((MNStruct *)globalcontext)->numProfileEpochs]();
	printf("Getting Means with %i lines \n", number_of_lines);
	double weightsum=0;
        for(int l=0;l<number_of_lines;l++){
		if(l%1000==0){printf("Line: %i \n", l);}

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);
                double weight = paramlist[0];
		weightsum+=weight;

		 for(int i = 0; i < ndim; i++){
		         Cube[i] = paramlist[i+2];
		 }


		int dotime = 0;
		struct timeval tval_before, tval_after, tval_resultone, tval_resulttwo;


		//gettimeofday(&tval_before, NULL);
		//double bluff = 0;
		//for(int loop = 0; loop < 5; loop++){
		//	Cube[0] = loop*0.2;

		int debug = 0;

		if(debug == 1)printf("In like \n");

		long double LDparams[ndim];
		int pcount;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


		int fitcount=0;
		for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

			LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
			fitcount++;

		}	
		
		
		pcount=0;
		long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
		pcount++;
		for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
			((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];
			pcount++;
		}
		for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
			((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
			pcount++;
		}

	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
	   	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
	        	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){

				if(((MNStruct *)globalcontext)->pulse->fitJump[k] == 0){


					//char str1[100],str2[100],str3[100],str4[100],str5[100];
				//	int nread=sscanf(((MNStruct *)globalcontext)->pulse->jumpStr[k],"%s %s %s %s %s",str1,str2,str3,str4,str5);
				//	double prejump=atof(str3);
					JumpVec[p] += (long double)((MNStruct *)globalcontext)->PreJumpVals[k]/SECDAY;
					//printf("Jump Not Fitted: %i %g %g\n", k, prejump,((MNStruct *)globalcontext)->PreJumpVals[k] );
				}
				else{

					//printf("Jump Fitted %i %i %i %g \n", p, k, l, (double)((MNStruct *)globalcontext)->pulse->jumpVal[k] );
			        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
				}
	        	 }
		}
           }

	}
	//sleep(5);
	long double DM = ((MNStruct *)globalcontext)->pulse->param[param_dm].val[0];	
	long double RefFreq = pow(10.0, 6)*3100.0L;
	long double FirstFreq = pow(10.0, 6)*2646.0L;
	long double DMPhaseShift = DM*(1.0/(FirstFreq*FirstFreq) - 1.0/(RefFreq*RefFreq))/(2.410*pow(10.0,-16));
	//phase -= DMPhaseShift/SECDAY;
	//printf("DM Shift: %.15Lg %.15Lg\n", DM, DMPhaseShift);


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}

	for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
		((MNStruct *)globalcontext)->pulse->jumpVal[k]= 0;
	}
       //for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
        //      ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        //}

	delete[] JumpVec;


//		fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);    
//		formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);     
		



		
		if(debug == 1)printf("Phase: %g \n", (double)phase);
		if(debug == 1)printf("Formed Residuals \n");

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


		  
		if(((MNStruct *)globalcontext)->numFitEFAC == 1){
			pcount++;
			
		}
		else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
			for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
				pcount++;
			}
		}	
		  
		if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		}
		else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
			pcount++;
		}
		else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
			for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
				pcount++;
			}
		}


		if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		}
		else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
			pcount++;
		}
		else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
			for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
				pcount++;
			}
		}	  
		  

		if(((MNStruct *)globalcontext)->incHighFreqStoc == 1){
			pcount++;
		}
		if(((MNStruct *)globalcontext)->incHighFreqStoc == 2){
			pcount++;
			pcount++;
		}

		if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
			pcount++;
		}	
		
		if(((MNStruct *)globalcontext)->incWidthJitter > 0){
			pcount++;
		}



		if(((MNStruct *)globalcontext)->incProfileNoise == 1){

			pcount++;
			pcount++;
		}

		if(((MNStruct *)globalcontext)->incProfileNoise == 2){

			for(int q = 0; q < ((MNStruct *)globalcontext)->ProfileNoiseCoeff; q++){
				pcount++;
			}
		}

		if(((MNStruct *)globalcontext)->incProfileNoise == 3){

			for(int q = 0; q < ((MNStruct *)globalcontext)->ProfileNoiseCoeff; q++){
				pcount++;
			}
		}


		 int DMt = 0;
		 for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
			double dmDot = 0;
			longdouble yrs = (((MNStruct *)globalcontext)->pulse->obsn[DMt].bat - ((MNStruct *)globalcontext)->pulse->param[param_dmepoch].val[0])/365.25;
			longdouble arg = 1.0;

			for (int d=0;d<9;d++){
			    if(d>0){
				arg *= yrs;
			    }
			    if (((MNStruct *)globalcontext)->pulse->param[param_dm].paramSet[d]==1){
				if(d>0){
				    dmDot+=(double)(((MNStruct *)globalcontext)->pulse->param[param_dm].val[d]*arg);
				}
			    }
			}

			if (((MNStruct *)globalcontext)->pulse->param[param_dm_sin1yr].paramSet[0]==1){
			    dmDot += ((MNStruct *)globalcontext)->pulse->param[param_dm_sin1yr].val[0]*sin(2*M_PI*yrs);
			}
			if (((MNStruct *)globalcontext)->pulse->param[param_dm_cos1yr].paramSet[0]==1){
			    dmDot += ((MNStruct *)globalcontext)->pulse->param[param_dm_cos1yr].val[0]*cos(2*M_PI*yrs);
			}
			if(l==0){printf("DMQ: %i %g \n", k, dmDot);}
			DMSignal[k] += (dmDot)*weight;
			DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];
		}

		double *SignalVec;
		double FreqDet = 0;
		double FreqLike = 0;
		int startpos = 0;
		if(((MNStruct *)globalcontext)->incDM==5){

			int FitDMCoeff = 2*((MNStruct *)globalcontext)->numFitDMCoeff;
			double *SignalCoeff = new double[FitDMCoeff];
			for(int i = 0; i < FitDMCoeff; i++){
				SignalCoeff[startpos + i] = Cube[pcount];
				pcount++;
			}

			double Tspan = ((MNStruct *)globalcontext)->Tspan;
			double f1yr = 1.0/3.16e7;

			double DMamp=Cube[pcount];
			pcount++;
			double DMindex=Cube[pcount];
			pcount++;
			
			DMamp=pow(10.0, DMamp);

			for (int i=0; i< FitDMCoeff/2; i++){
				
				double freq = ((double)(i+1.0))/Tspan;
				
				double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freq*365.25,(-DMindex))/(Tspan*24*60*60);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitDMCoeff/2] = SignalCoeff[i+FitDMCoeff/2]*sqrt(rho);  
				FreqDet += 2*log(rho);	
				FreqLike += SignalCoeff[i]*SignalCoeff[i]/rho + SignalCoeff[i+FitDMCoeff/2]*SignalCoeff[i+FitDMCoeff/2]/rho;
			}
			double *FMatrix = new double[FitDMCoeff*((MNStruct *)globalcontext)->numProfileEpochs];
			for(int i=0;i< FitDMCoeff/2;i++){
				int DMt = 0;
				for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){

					double time=(double)((MNStruct *)globalcontext)->pulse->obsn[DMt].bat;
		
					FMatrix[k + (i+startpos)*((MNStruct *)globalcontext)->numProfileEpochs]=cos(2*M_PI*(double(i+1)/Tspan)*time);
					FMatrix[k + (i+FitDMCoeff/2+startpos)*((MNStruct *)globalcontext)->numProfileEpochs] = sin(2*M_PI*(double(i+1)/Tspan)*time);
					DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];

				}
			}

			SignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs];
			vector_dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->numProfileEpochs, FitDMCoeff,'N');
			startpos=FitDMCoeff;
			
			 for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
				DMSignal[k] += (SignalVec[k])*weight;//(SignalVec[k]+dmDot)*weight;
			}
			delete[] FMatrix;	
			delete[] SignalCoeff;	
			delete[] SignalVec;

		}
	

		if(((MNStruct *)globalcontext)->incDM==6){

			int FitDMCoeff = 2*((MNStruct *)globalcontext)->numFitDMCoeff;
			double *SignalCoeff = new double[FitDMCoeff];
			for(int i = 0; i < FitDMCoeff; i++){
				SignalCoeff[startpos + i] = Cube[pcount];
				//printf("coeffs %i %g \n", i, SignalCoeff[i]);
				pcount++;
			}
				
			

			double Tspan = ((MNStruct *)globalcontext)->Tspan;
			double f1yr = 1.0/3.16e7;



			for (int i=0; i< FitDMCoeff/2; i++){
		
				double DMamp=pow(10.0, Cube[pcount]);
				pcount++;
				
				double freq = ((double)(i+1.0))/Tspan;
				
				double rho = (DMamp*DMamp);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitDMCoeff/2] = SignalCoeff[i+FitDMCoeff/2]*sqrt(rho);  
				FreqDet += 2*log(rho);	
				FreqLike += SignalCoeff[i]*SignalCoeff[i]/rho + SignalCoeff[i+FitDMCoeff/2]*SignalCoeff[i+FitDMCoeff/2]/rho;
			}
			double *FMatrix = new double[FitDMCoeff*((MNStruct *)globalcontext)->numProfileEpochs];
			for(int i=0;i< FitDMCoeff/2;i++){
				int DMt = 0;
				for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){

					double time=(double)((MNStruct *)globalcontext)->pulse->obsn[DMt].bat;
		
					FMatrix[k + (i+startpos)*((MNStruct *)globalcontext)->numProfileEpochs]=cos(2*M_PI*(double(i+1)/Tspan)*time);
					FMatrix[k + (i+FitDMCoeff/2+startpos)*((MNStruct *)globalcontext)->numProfileEpochs] = sin(2*M_PI*(double(i+1)/Tspan)*time);
					DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];

				}
			}

			SignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs];
			vector_dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->numProfileEpochs, FitDMCoeff,'N');
			startpos=FitDMCoeff;
			
			 for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
				DMSignal[k] += (SignalVec[k])*weight;//(SignalVec[k]+dmDot)*weight;
			}
			delete[] FMatrix;	
			delete[] SignalCoeff;	
			delete[] SignalVec;

		}
	
        }       

 
		
	 for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
		DMSignal[k] = DMSignal[k]/weightsum;
	}

	summaryfile.close();



	printf("Getting Errors with %i lines \n", number_of_lines);
	summaryfile.open(fname.c_str());	
	weightsum=0;
        for(int l=0;l<number_of_lines;l++){

		if(l%1000==0){printf("Line: %i \n", l);}

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);
                double weight = paramlist[0];
		weightsum+=weight;

		 for(int i = 0; i < ndim; i++){
		         Cube[i] = paramlist[i+2];
		 }


		int dotime = 0;
		struct timeval tval_before, tval_after, tval_resultone, tval_resulttwo;

		int debug = 0;

		if(debug == 1)printf("In like \n");
		long double LDparams[ndim];
		int pcount;




/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


		int fitcount=0;
		for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

			LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
			fitcount++;

		}	
		
		
		pcount=0;
		long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
		pcount++;
		for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
			((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];
			pcount++;
		}
		for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
			((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
			pcount++;
		}

	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
	   	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
	        	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){

				if(((MNStruct *)globalcontext)->pulse->fitJump[k] == 0){


					//char str1[100],str2[100],str3[100],str4[100],str5[100];
				//	int nread=sscanf(((MNStruct *)globalcontext)->pulse->jumpStr[k],"%s %s %s %s %s",str1,str2,str3,str4,str5);
				//	double prejump=atof(str3);
					JumpVec[p] += (long double)((MNStruct *)globalcontext)->PreJumpVals[k]/SECDAY;
					//printf("Jump Not Fitted: %i %g %g\n", k, prejump,((MNStruct *)globalcontext)->PreJumpVals[k] );
				}
				else{

					//printf("Jump Fitted %i %i %i %g \n", p, k, l, (double)((MNStruct *)globalcontext)->pulse->jumpVal[k] );
			        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
				}
	        	 }
		}
           }

	}
	//sleep(5);
	long double DM = ((MNStruct *)globalcontext)->pulse->param[param_dm].val[0];	
	long double RefFreq = pow(10.0, 6)*3100.0L;
	long double FirstFreq = pow(10.0, 6)*2646.0L;
	long double DMPhaseShift = DM*(1.0/(FirstFreq*FirstFreq) - 1.0/(RefFreq*RefFreq))/(2.410*pow(10.0,-16));
	//phase -= DMPhaseShift/SECDAY;
	//printf("DM Shift: %.15Lg %.15Lg\n", DM, DMPhaseShift);


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}

	for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
		((MNStruct *)globalcontext)->pulse->jumpVal[k]= 0;
	}
       //for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
        //      ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        //}

	delete[] JumpVec;


//		fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);     
		//formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);     
		



		
		if(debug == 1)printf("Phase: %g \n", (double)phase);
		if(debug == 1)printf("Formed Residuals \n");

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


		  
		if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		}
		else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
			pcount++;
			
		}
		else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
			for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
				pcount++;

			}
		}	
		  
		if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		}
		else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
			pcount++;
		}
		else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
			for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
				pcount++;
			}
		}


		if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		}
		else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
			pcount++;
		}
		else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
			for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
				pcount++;
			}
		}	  
		  

		if(((MNStruct *)globalcontext)->incHighFreqStoc == 1){
			pcount++;
		}
		if(((MNStruct *)globalcontext)->incHighFreqStoc == 2){
			pcount++;
			pcount++;
		}

		if(((MNStruct *)globalcontext)->incDMEQUAD > 0){
			pcount++;
		}	
		
		if(((MNStruct *)globalcontext)->incWidthJitter > 0){
			pcount++;
		}



		if(((MNStruct *)globalcontext)->incProfileNoise == 1){

			pcount++;
			pcount++;
		}

		if(((MNStruct *)globalcontext)->incProfileNoise == 2){

			for(int q = 0; q < ((MNStruct *)globalcontext)->ProfileNoiseCoeff; q++){
				pcount++;
			}
		}

		if(((MNStruct *)globalcontext)->incProfileNoise == 3){

			for(int q = 0; q < ((MNStruct *)globalcontext)->ProfileNoiseCoeff; q++){
				pcount++;
			}
		}

		 double *SignalVec;
		 double *NonPLSignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs]();
		 int DMt = 0;
		 for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
			double dmDot = 0;
			longdouble yrs = (((MNStruct *)globalcontext)->pulse->obsn[DMt].bat - ((MNStruct *)globalcontext)->pulse->param[param_dmepoch].val[0])/365.25;
			longdouble arg = 1.0;

			for (int d=0;d<9;d++){
			    if(d>0){
				arg *= yrs;
			    }
			    if (((MNStruct *)globalcontext)->pulse->param[param_dm].paramSet[d]==1){
				if(d>0){
				    dmDot+=(double)(((MNStruct *)globalcontext)->pulse->param[param_dm].val[d]*arg);
				}
			    }
			}

			if (((MNStruct *)globalcontext)->pulse->param[param_dm_sin1yr].paramSet[0]==1){
			    dmDot += ((MNStruct *)globalcontext)->pulse->param[param_dm_sin1yr].val[0]*sin(2*M_PI*yrs);
			}
			if (((MNStruct *)globalcontext)->pulse->param[param_dm_cos1yr].paramSet[0]==1){
			    dmDot += ((MNStruct *)globalcontext)->pulse->param[param_dm_cos1yr].val[0]*cos(2*M_PI*yrs);
			}

			NonPLSignalVec[k] += (dmDot);
			DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];
		}


		
		double FreqDet = 0;
		double FreqLike = 0;
		int startpos = 0;
		if(((MNStruct *)globalcontext)->incDM==5){

			int FitDMCoeff = 2*((MNStruct *)globalcontext)->numFitDMCoeff;
			double *SignalCoeff = new double[FitDMCoeff];
			for(int i = 0; i < FitDMCoeff; i++){
				SignalCoeff[startpos + i] = Cube[pcount];
				pcount++;
			}
				
			

			double Tspan = ((MNStruct *)globalcontext)->Tspan;
			double f1yr = 1.0/3.16e7;


			double DMamp=Cube[pcount];
			pcount++;
			double DMindex=Cube[pcount];
			pcount++;

			
			DMamp=pow(10.0, DMamp);



			for (int i=0; i< FitDMCoeff/2; i++){
				
				double freq = ((double)(i+1.0))/Tspan;
				
				double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freq*365.25,(-DMindex))/(Tspan*24*60*60);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitDMCoeff/2] = SignalCoeff[i+FitDMCoeff/2]*sqrt(rho);  



			}
			//double *TempSignalVec   = new double[((MNStruct *)globalcontext)->numProfileEpochs]();
			double *FMatrix = new double[FitDMCoeff*((MNStruct *)globalcontext)->numProfileEpochs];
			for(int i=0;i< FitDMCoeff/2;i++){
				int DMt = 0;
				for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
					double time=(double)((MNStruct *)globalcontext)->pulse->obsn[DMt].bat;

					//TempSignalVec[k] += SignalCoeff[i]*cos(2*M_PI*(double(i+1)/Tspan)*time) + SignalCoeff[i+FitDMCoeff/2]*sin(2*M_PI*(double(i+1)/Tspan)*time);
					

					if(l==0 && k==0){printf("Freqs: %i %g %g %g\n", i, Tspan, time, (double(i+1)/Tspan));}
					FMatrix[k + (i+startpos)*((MNStruct *)globalcontext)->numProfileEpochs]=cos(2*M_PI*(double(i+1)/Tspan)*time);
					FMatrix[k + (i+FitDMCoeff/2+startpos)*((MNStruct *)globalcontext)->numProfileEpochs] = sin(2*M_PI*(double(i+1)/Tspan)*time);
					//printf("FM: %i %i %g %g \n", i, k, FMatrix[k + (i+startpos)*((MNStruct *)globalcontext)->numProfileEpochs], Tspan);
					DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];

				}
			}

			SignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs];
			vector_dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->numProfileEpochs, FitDMCoeff,'N');
			startpos=FitDMCoeff;
			 int DMt = 0;	
			 for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
				NonPLSignalVec[k] += SignalVec[k];
				DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];
			}





			delete[] FMatrix;	
			delete[] SignalCoeff;	
			delete[] SignalVec;
			//delete[] TempSignalVec;

		}

		if(((MNStruct *)globalcontext)->incDM==6){

			int FitDMCoeff = 2*((MNStruct *)globalcontext)->numFitDMCoeff;
			double *SignalCoeff = new double[FitDMCoeff];
			for(int i = 0; i < FitDMCoeff; i++){
				SignalCoeff[startpos + i] = Cube[pcount];
				//printf("coeffs %i %g \n", i, SignalCoeff[i]);
				pcount++;
			}
				
			

			double Tspan = ((MNStruct *)globalcontext)->Tspan;
			double f1yr = 1.0/3.16e7;



			for (int i=0; i< FitDMCoeff/2; i++){
		
				double DMamp=pow(10.0, Cube[pcount]);
				pcount++;
				
				double freq = ((double)(i+1.0))/Tspan;
				
				double rho = (DMamp*DMamp);
				SignalCoeff[i] = SignalCoeff[i]*sqrt(rho);
				SignalCoeff[i+FitDMCoeff/2] = SignalCoeff[i+FitDMCoeff/2]*sqrt(rho);  
				FreqDet += 2*log(rho);	
				FreqLike += SignalCoeff[i]*SignalCoeff[i]/rho + SignalCoeff[i+FitDMCoeff/2]*SignalCoeff[i+FitDMCoeff/2]/rho;
			}
			double *FMatrix = new double[FitDMCoeff*((MNStruct *)globalcontext)->numProfileEpochs];
			for(int i=0;i< FitDMCoeff/2;i++){
				int DMt = 0;
				for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){

					double time=(double)((MNStruct *)globalcontext)->pulse->obsn[DMt].bat;
		
					FMatrix[k + (i+startpos)*((MNStruct *)globalcontext)->numProfileEpochs]=cos(2*M_PI*(double(i+1)/Tspan)*time);
					FMatrix[k + (i+FitDMCoeff/2+startpos)*((MNStruct *)globalcontext)->numProfileEpochs] = sin(2*M_PI*(double(i+1)/Tspan)*time);
					DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];

				}
			}

			SignalVec = new double[((MNStruct *)globalcontext)->numProfileEpochs];
			vector_dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->numProfileEpochs, FitDMCoeff,'N');
			startpos=FitDMCoeff;
			 
			int DMt = 0;	
			 for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
				NonPLSignalVec[k] += SignalVec[k];
				DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];
			}
			delete[] FMatrix;	
			delete[] SignalCoeff;	
			delete[] SignalVec;

		}
		 for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
			DMSignalErr[k] += (NonPLSignalVec[k] - DMSignal[k])*(NonPLSignalVec[k] - DMSignal[k])*weight;
		}


		
		delete[] NonPLSignalVec;
        }       

	int DMt = 0;
	for(int k=0;k<((MNStruct *)globalcontext)->numProfileEpochs;k++){
		DMSignalErr[k] = sqrt(DMSignalErr[k]/weightsum);
		printf("DM: %i %.10g %g %g \n", k, (double) ((MNStruct *)globalcontext)->pulse->obsn[DMt].bat, DMSignal[k], DMSignalErr[k]);
		DMt += ((MNStruct *)globalcontext)->numChanPerInt[k];
	}

//	summaryfile.close();


}
*/
/*
void  WriteProfileDomainLike2(std::string longname, int &ndim){


	int writeprofiles=1;

	int profiledimstart=0;
        double *Cube = new double[ndim];
        int number_of_lines = 0;

        std::ifstream checkfile;
	std::string checkname;
	if(((MNStruct *)globalcontext)->sampler == 0){
	        checkname = longname+"phys_live.points";
	}
        if(((MNStruct *)globalcontext)->sampler == 1){
                checkname = longname+"_phys_live.txt";
        }
	//printf("%i %s \n", ((MNStruct *)globalcontext)->sampler, checkname.c_str());
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        checkfile.close();

        std::ifstream summaryfile;
	std::string fname;
	if(((MNStruct *)globalcontext)->sampler == 0){
		fname = longname+"phys_live.points";
	}
	if(((MNStruct *)globalcontext)->sampler == 1){
	        fname = longname+"_phys_live.txt";
	}

        summaryfile.open(fname.c_str());



        printf("Getting ML \n");
        double maxlike = -1.0*pow(10.0,10);
        for(int i=0;i<number_of_lines;i++){

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);
                double like = paramlist[ndim];

                if(like > maxlike){
                        maxlike = like;
                         for(int i = 0; i < ndim; i++){
                                 Cube[i] = paramlist[i];
                         }
                }

        }
        summaryfile.close();
	printf("ML val: %.10g \n", maxlike);
//	for(int i = 5; i < ndim; i++){
//		printf("ShapePriors[%i][0] =  %g \n", i-4, ((MNStruct *)globalcontext)->MeanProfileShape[i-4] + Cube[i]);
//		printf("ShapePriors[%i][1] =  %g \n", i-4, ((MNStruct *)globalcontext)->MeanProfileShape[i-4] + Cube[i]);
//	}
	int debug = 0;

	if(debug == 1)printf("In like \n");

	double *EFAC;
	double *EQUAD;



        long double LDparams[ndim];
        int pcount;

	double uniformpriorterm = 0;



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;

	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);   



	
	if(debug == 1)printf("Phase: %g \n", (double)phase);
	if(debug == 1)printf("Formed Residuals \n");

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		for(int j =0; j < numcoeff[i]; j=j+2){
			modelflux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*shapecoeff[cpos+j];
		}
		cpos+= numcoeff[i];
	}

	double OPLevel = ((MNStruct *)globalcontext)->offPulseLevel;

	double lnew = 0;
	int badshape = 0;

//	int minoffpulse=750;
//	int maxoffpulse=1250;
       // int minoffpulse=200;
       // int maxoffpulse=800;
	double maxchisq = 0;	
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
		if(debug == 1)printf("In toa %i \n", t);
		int nTOA = t;

		double detN  = 0;
		double Chisq  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 
		double JitterPrior = 0;	   

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **AllHermiteBasis =  new double*[Nbins];
		double **JitterBasis  =  new double*[Nbins];
		double **Hermitepoly =  new double*[Nbins];

		for(int i =0;i<Nbins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];for(int j =0;j<maxshapecoeff;j++){AllHermiteBasis[i][j]=0;}}
		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[totshapecoeff];for(int j =0;j<totshapecoeff;j++){Hermitepoly[i][j]=0;}}
		for(int i =0;i<Nbins;i++){JitterBasis[i]=new double[totshapecoeff];for(int j =0;j<totshapecoeff;j++){JitterBasis[i][j]=0;}}
	
	

		long double binpos = ModelBats[nTOA];


		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];

		int cpos=0;
		int jpos=0;
	   	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
			for(int j =0; j < Nbins; j++){
				long double timediff = 0;
				long double bintime = ProfileBats[t][j] + CompSeps[i]/SECDAY;
				if(bintime  >= minpos && bintime <= maxpos){
				    timediff = bintime - binpos;
				}
				else if(bintime < minpos){
				    timediff = FoldingPeriodDays+bintime - binpos;
				}
				else if(bintime > maxpos){
				    timediff = bintime - FoldingPeriodDays - binpos;
				}

				timediff=timediff*SECDAY;

				Betatimes[j]=timediff/beta;
				TNothplMC(numcoeff[i]+1,Betatimes[j],AllHermiteBasis[j], jpos);

				for(int k =0; k < numcoeff[i]+1; k++){
					double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));
					AllHermiteBasis[j][k+jpos]=AllHermiteBasis[j][k+jpos]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

					if(k<numcoeff[i]){ Hermitepoly[j][k+cpos] = AllHermiteBasis[j][k+jpos];}
				
				}

				JitterBasis[j][0+cpos] = (1.0/sqrt(2.0))*(-1.0*AllHermiteBasis[j][1+jpos]);
				for(int k =1; k < numcoeff[i]; k++){
					JitterBasis[j][k+cpos] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1+jpos] - sqrt(double(k+1))*AllHermiteBasis[j][k+1+jpos]);
				}	
		        } 
			cpos += numcoeff[i];
			jpos += numcoeff[i]+1;
		}

		double *shapevec  = new double[Nbins];
		double *Jittervec = new double[Nbins];
		double *ProfileModVec = new double[Nbins]();
		double *ProfileJitterModVec = new double[Nbins]();

		double OverallProfileAmp = shapecoeff[0];

		shapecoeff[0] = 1;

		dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,totshapecoeff,'N');
		dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,totshapecoeff,'N');


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////Add in any Profile Changes///////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		for(int i =0; i < totalCoeffForMult; i++){
			ProfileModCoeffs[i]=0;	
		}				

		if(nTOA==0){printf("outputting profile parameters\n");}


		cpos = 0;
		epos = 0;
		fpos = 0;

		for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){

			for(int i =0; i < numProfileFitCoeff[c]; i++){
				ProfileModCoeffs[i+cpos] += ProfileFitCoeffs[i+fpos];

				if(nTOA==0){printf("P %g \n", ((MNStruct *)globalcontext)->MeanProfileShape[i+npos] + ProfileFitCoeffs[i+fpos]);}

			}
			for(int i = numProfileFitCoeff[c]; i < numcoeff[c]; i++){
				if(nTOA==0){printf("P %g \n", ((MNStruct *)globalcontext)->MeanProfileShape[i+npos]);}
			}
		
			if(nTOA==0 && c == 0){printf("B %g \n", ((MNStruct *)globalcontext)->MeanProfileBeta );}

			double reffreq = ((MNStruct *)globalcontext)->EvoRefFreq;
			double freqdiff =  (((MNStruct *)globalcontext)->pulse->obsn[t].freq - reffreq)/1000.0;

			for(int i =0; i < numEvoCoeff[c]; i++){
				ProfileModCoeffs[i+cpos] += EvoCoeffs[i+epos]*freqdiff;
				if(nTOA==0){printf("E %g \n", EvoCoeffs[i+epos]);}
			}
	
			cpos += NumCoeffForMult[c];
			epos += numEvoCoeff[c];
			fpos += numProfileFitCoeff[c];
			npos += numcoeff[c];


		}

		if(nTOA==0){printf("finished outputting profile parameters\n");}

 		double ModModelFlux = modelflux;
		cpos = 0;
		for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
			for(int j =0; j < NumCoeffForMult[i]; j=j+2){
				modelflux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*ProfileModCoeffs[cpos+j];
			}
			cpos += NumCoeffForMult[i];
		}


		if(totalCoeffForMult > 0){
			dgemv(Hermitepoly, ProfileModCoeffs,ProfileModVec,Nbins,totalCoeffForMult,'N');

			if(((MNStruct *)globalcontext)->numFitEQUAD > 0 || ((MNStruct *)globalcontext)->incDMEQUAD > 0 ){
				dgemv(JitterBasis, ProfileModCoeffs,ProfileJitterModVec,Nbins,totalCoeffForMult,'N');
			}
		}





		double *NDiffVec = new double[Nbins];

		double maxshape=0;


		for(int j =0; j < Nbins; j++){
			shapevec[j] += ProfileModVec[j];
		  	if(shapevec[j] > maxshape){ maxshape = shapevec[j];}

		}


		//maxshape=((MNStruct *)globalcontext)->MaxShapeAmp;

		double noisemean=0;
		int numoffpulse = 0;
		for(int j = 0; j < Nbins; j++){
			if(shapevec[j] < maxshape*OPLevel){
				noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
				numoffpulse += 1;
			}	
		}


		noisemean = noisemean/(numoffpulse);

		double MLSigma = 0;
		int MLSigmaCount = 0;
		double dataflux = 0;
		for(int j = 0; j < Nbins; j++){
			if(shapevec[j] < maxshape*OPLevel){
				double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean;
				MLSigma += res*res; MLSigmaCount += 1;
			}
			else{
				if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
					dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-noisemean)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
				}
			}
		}

		MLSigma = sqrt(MLSigma/MLSigmaCount);
		MLSigma = MLSigma*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		double maxamp = dataflux/ModModelFlux;
		if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g \n", MLSigma, dataflux, ModModelFlux, maxamp);


	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


		double **M = new double*[Nbins];
		double **NM = new double*[Nbins];

		int Msize = 2+numshapestoccoeff;
		int Mcounter=0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			Msize++;
		}


		for(int i =0; i < Nbins; i++){
			Mcounter=0;
			M[i] = new double[Msize];
			NM[i] = new double[Msize];

			M[i][0] = 1;
			M[i][1] = shapevec[i];

			Mcounter = 2;

			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
				M[i][Mcounter] = Jittervec[i];
				Mcounter++;
			}			

			for(int j = 0; j < numshapestoccoeff; j++){
			    M[i][Mcounter+j] = AllHermiteBasis[i][j];
			}
		}


		double **MNM = new double*[Msize];
		for(int i =0; i < Msize; i++){
		    MNM[i] = new double[Msize];
		}

		      
		Chisq = 0;
		detN = 0;

	
 		double highfreqnoise = HighFreqStoc1;
		double flatnoise = HighFreqStoc2;
		
		double *profilenoise = new double[Nbins];

		for(int i =0; i < Nbins; i++){
			Mcounter=2;
			double noise = MLSigma*MLSigma +  pow(highfreqnoise*maxamp*shapevec[i],2);
			if(shapevec[i] > maxshape*OPLevel){
				noise +=  pow(flatnoise*maxamp*maxshape,2);
			}
			detN += log(noise);
			profilenoise[i] = sqrt(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1];

			NDiffVec[i] = datadiff/(noise);
		       

			Chisq += datadiff*datadiff/(noise);
	
			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
				M[i][2] = M[i][2]*dataflux/ModModelFlux/beta;
				Mcounter++;
			}
			
			for(int j = 0; j < numshapestoccoeff; j++){
				M[i][Mcounter+j] = M[i][Mcounter+j]*dataflux;

			}


			for(int j = 0; j < Msize; j++){
				NM[i][j] = M[i][j]/(noise);
			}


		}
		
			
		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');


		Mcounter=2;
		double JitterDet = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
			profnoise = profnoise*profnoise;
			JitterDet = log(profnoise);
			MNM[2][2] += 1.0/profnoise;
			Mcounter++;
		}


		double StocProfDet = 0;
		for(int j = 0; j < numshapestoccoeff; j++){
			StocProfDet += log(StocProfCoeffs[j]);
			MNM[Mcounter+j][Mcounter+j] +=  1.0/StocProfCoeffs[j];
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);
			    
		logMargindet = Margindet;

		Marginlike = 0;
		for(int i =0; i < Msize; i++){
			Marginlike += TempdNM[i]*dNM[i];
		}


		double finaloffset = TempdNM[0];
		double finalamp = TempdNM[1];
		double finalJitter = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0)finalJitter = TempdNM[2];

		double *StocVec = new double[Nbins];
		
		for(int i =0; i < Nbins; i++){
			if(((MNStruct *)globalcontext)->numFitEQUAD > 0)Jittervec[i] = M[i][2];
			if(((MNStruct *)globalcontext)->numFitEQUAD == 0)Jittervec[i] = 0;
		}

		dgemv(M,TempdNM,shapevec,Nbins,Msize,'N');

		

		TempdNM[0] = 0;
		TempdNM[1] = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0)TempdNM[2] = 0;
		

		dgemv(M,TempdNM,StocVec,Nbins,Msize,'N');

		double StocSN = 0;

		if(writeprofiles == 1){
			std::ofstream profilefile;
			char toanum[15];
			sprintf(toanum, "%d", nTOA);
			std::string ProfileName =  toanum;
			std::string dname = longname+ProfileName+"-Profile.txt";
	
			profilefile.open(dname.c_str());
			double MLChisq = 0;
			for(int i =0; i < Nbins; i++){
				StocSN += StocVec[i]*StocVec[i]/profilenoise[i]/profilenoise[i];
				MLChisq += pow(((double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1] - shapevec[i])/profilenoise[i], 2);
				profilefile << i << " " << std::setprecision(10) << (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1] << " " << shapevec[i] << " " << profilenoise[i] << " " << finaloffset + finalamp*M[i][1] << " " << finalJitter*Jittervec[i] << " " << StocVec[i] << "\n";
//				if(fabs((double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1] - shapevec[i])/profilenoise[i] > 5)printf("5 sig %i %i \n", t, i);

			}
	    		profilefile.close();
			printf("Stoc SN: %i %g \n", t, StocSN);
		}
		delete[] profilenoise;
		delete[] StocVec;
		
		profilelike = -0.5*(detN + Chisq + logMargindet + StocProfDet + JitterDet - Marginlike);
		lnew += profilelike;

//		if(MLChisq > maxchisq){ printf("Max: %i %g \n", t, MLChisq); maxchisq=MLChisq;}
//		printf("Profile chisq and like: %g %g \n", MLChisq, profilelike);
//		printf("Like: %i %.15g %.15g %.15g %.15g %.15g %.15g %.15g\n", nTOA,lnew, detN, Chisq, logMargindet, Marginlike, StocProfDet , JitterDet);

		delete[] shapevec;
		delete[] Jittervec;
		delete[] ProfileModVec;
		delete[] ProfileJitterModVec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] JitterBasis[j];
		    delete[] AllHermiteBasis[j];
		    delete[] M[j];
		    delete[] NM[j];
		}
		delete[] Hermitepoly;
		delete[] AllHermiteBasis;
		delete[] JitterBasis;
		delete[] M;
		delete[] NM;

		for (int j = 0; j < Msize; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	}
	 
	 
	 
	 

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] numcoeff;
	delete[] CompSeps;
	delete[] NumCoeffForMult;
	lnew += uniformpriorterm;
	printf("End Like: %.10g \n", lnew);

	WriteProfileFreqEvo(longname, ndim,profiledimstart);


}

*/

/*
void  WriteExtraComp(std::string longname, int &ndim, int profiledimstart){


	int InterpIndex = 0;
        double *Cube = new double[ndim];
        int number_of_lines = 0;

        std::ifstream checkfile;
	std::string checkname;
	if(((MNStruct *)globalcontext)->sampler == 0){
	        checkname = longname+".txt";
	}
        if(((MNStruct *)globalcontext)->sampler == 1){
                checkname = longname+".txt";
        }
	//printf("%i %s \n", ((MNStruct *)globalcontext)->sampler, checkname.c_str());
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        checkfile.close();

        std::ifstream summaryfile;
	std::string fname;
	if(((MNStruct *)globalcontext)->sampler == 0){
		fname = longname+".txt";
	}
	if(((MNStruct *)globalcontext)->sampler == 1){
	        fname = longname+".txt";
	}

        summaryfile.open(fname.c_str());



        printf("Getting Mean with %i lines \n", number_of_lines);

	int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[0][1];	

	double *ECProfiles = new double[Nbins];
	for(int j = 0; j < Nbins; j++){
		ECProfiles[j] = 0;
	}

	double *EvoErrProfiles = new double[Nbins];
	for(int j = 0; j < Nbins; j++){
		ECErrProfiles[j] = 0;
	}

	double weightsum=0;
        for(int i=0;i<number_of_lines;i++){

		if(i%1000==0){printf("Line: %i \n", i);}

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);
                double weight = paramlist[0];
		weightsum+=weight;

		 for(int i = 0; i < ndim; i++){
		         Cube[i] = paramlist[i+2];
		 }
                
		int pcount = profiledimstart;
		double *ExtraCompProps;	
		double *ExtraCompBasis;
		int incExtraProfComp = ((MNStruct *)globalcontext)->incExtraProfComp;
		
		if(incExtraProfComp == 1){
			//Step Model, time, width, amp
			ExtraCompProps = new double[4];
			
			ExtraCompProps[0] = Cube[pcount]; //Position in time
			pcount++;
			ExtraCompProps[1] = Cube[pcount]; // position in phase
			pcount++;
			ExtraCompProps[2] = pow(10.0, Cube[pcount])*((MNStruct *)globalcontext)->ReferencePeriod; // comp width
			pcount++;
			ExtraCompProps[3] = pow(10.0, Cube[pcount]); //amp
			pcount++;
		
		}
		if(incExtraProfComp == 2){
			//Gaussian decay model: time, comp width, max comp amp, log decay width
			ExtraCompProps = new double[5];

			ExtraCompProps[0] = Cube[pcount]; //Position
			pcount++;
			ExtraCompProps[1] = Cube[pcount]; // position in phase
			pcount++;
			ExtraCompProps[2] = pow(10.0, Cube[pcount])*((MNStruct *)globalcontext)->ReferencePeriod; // comp width
			pcount++;
			ExtraCompProps[3] = pow(10.0, Cube[pcount]); //amp
			pcount++;
			ExtraCompProps[4] = pow(10.0, Cube[pcount]); //Event width
			pcount++;

		}
	//	printf("details: %i %i \n", incExtraProfComp, ((MNStruct *)globalcontext)->NumExtraCompCoeffs);
		if(incExtraProfComp == 3){
			//exponential decay: time, comp width, max comp amp, log decay timescale

			ExtraCompProps = new double[5+((MNStruct *)globalcontext)->NumExtraCompCoeffs];
			ExtraCompBasis = new double[((MNStruct *)globalcontext)->NumExtraCompCoeffs];

			ExtraCompProps[0] = Cube[pcount]; //Position
			pcount++;
			ExtraCompProps[1] = Cube[pcount]; // position in phase
			pcount++;
			ExtraCompProps[2] = pow(10.0, Cube[pcount])*((MNStruct *)globalcontext)->ReferencePeriod; // comp width
			pcount++;
			for(int i = 0; i < ((MNStruct *)globalcontext)->NumExtraCompCoeffs; i++){

				ExtraCompProps[3+i] = Cube[pcount]; //amp
				pcount++;
			}
			ExtraCompProps[3+((MNStruct *)globalcontext)->NumExtraCompCoeffs] = pow(10.0, Cube[pcount]); //Event width
			pcount++;
			ExtraCompProps[4+((MNStruct *)globalcontext)->NumExtraCompCoeffs] = Cube[pcount];
			pcount++;


		}
		if(incExtraProfComp == 4){
			ExtraCompProps = new double[5+2];
			ExtraCompProps[0] = Cube[pcount]; //Position
			pcount++;
			ExtraCompProps[1] = pow(10.0, Cube[pcount]); //Event width
			pcount++;
			for(int i = 0; i < 5; i++){
				ExtraCompProps[2+i] = Cube[pcount]; // Coeff Amp
				pcount++;

			}


		}





		if(incExtraProfComp > 0 && incExtraProfComp < 4){
			long double ExtraCompPos = ExtraCompProps[1]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
			long double timediff = 0;
			long double bintime = ProfileBats[t][0] + (j-1)*(ProfileBats[t][1] - ProfileBats[t][0])/(ProfNbins-1);
			if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - ExtraCompPos;
			}
			else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - ExtraCompPos;
			}
			else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - ExtraCompPos;
			}

			timediff=timediff*SECDAY;

			double Betatime = timediff/ExtraCompProps[2];
			
			double ExtraCompDecayScale = 1;

			if(incExtraProfComp == 1){
				if(((MNStruct *)globalcontext)->pulse->obsn[t].bat < ExtraCompProps[0]){
					ExtraCompDecayScale = 0;
				}
			}

			if(incExtraProfComp == 2){
				ExtraCompDecayScale = exp(-0.5*pow((ExtraCompProps[0]-((MNStruct *)globalcontext)->pulse->obsn[t].bat)/ExtraCompProps[4],2));
			}
			if(incExtraProfComp == 3){
				if(((MNStruct *)globalcontext)->pulse->obsn[t].bat < ExtraCompProps[0]){
					ExtraCompDecayScale = 0;
				}
				else{
					ExtraCompDecayScale = exp((ExtraCompProps[0]-((MNStruct *)globalcontext)->pulse->obsn[t].bat)/ExtraCompProps[3+((MNStruct *)globalcontext)->NumExtraCompCoeffs]);
				}
			}

			
			TNothpl(((MNStruct *)globalcontext)->NumExtraCompCoeffs, Betatime, ExtraCompBasis);


			double ExtraCompAmp = 0;
			for(int k =0; k < ((MNStruct *)globalcontext)->NumExtraCompCoeffs; k++){
				double Bconst=(1.0/sqrt(betas[0]))/sqrt(pow(2.0,k)*sqrt(M_PI));
				ExtraCompAmp += ExtraCompBasis[k]*Bconst*ExtraCompProps[3+k];
			}

			double ExtraCompSignal = ExtraCompAmp*exp(-0.5*Betatime*Betatime)*ExtraCompDecayScale;

			if(fabs(ExtraCompProps[1]) < 0.0005){ExtraCompSignal=0;}
			ECProfiles[j] += ExtraCompSignal*weight;
		    }

	}
}


*/




/*
void  WriteProfileFreqEvo(std::string longname, int &ndim, int profiledimstart){


	int InterpIndex = 0;
        double *Cube = new double[ndim];
        int number_of_lines = 0;

        std::ifstream checkfile;
	std::string checkname;
	if(((MNStruct *)globalcontext)->sampler == 0){
	        checkname = longname+".txt";
	}
        if(((MNStruct *)globalcontext)->sampler == 1){
                checkname = longname+".txt";
        }
	//printf("%i %s \n", ((MNStruct *)globalcontext)->sampler, checkname.c_str());
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        checkfile.close();

        std::ifstream summaryfile;
	std::string fname;
	if(((MNStruct *)globalcontext)->sampler == 0){
		fname = longname+".txt";
	}
	if(((MNStruct *)globalcontext)->sampler == 1){
	        fname = longname+".txt";
	}

        summaryfile.open(fname.c_str());



        printf("Getting Mean with %i lines \n", number_of_lines);

	int numsteps=5;
	int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[0][1];	

	double minfreq = 100000000.0;
	double maxfreq = 0;

	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
		double tfreq = (double)((MNStruct *)globalcontext)->pulse->obsn[t].freq;
		if(tfreq > maxfreq)maxfreq = tfreq;
		if(tfreq < minfreq)minfreq = tfreq;
	}

	printf("Using freqs: min %g max %g \n", minfreq, maxfreq);


	double bw = maxfreq-minfreq;
	double fstep = bw/(numsteps-1);		



	double **EvoProfiles = new double*[numsteps];
	for(int i = 0; i < numsteps; i++){
		EvoProfiles[i] = new double[Nbins];
		for(int j = 0; j < Nbins; j++){
			EvoProfiles[i][j] = 0;
		}
	}

	double **EvoErrProfiles = new double*[numsteps];
	for(int i = 0; i < numsteps; i++){
		EvoErrProfiles[i] = new double[Nbins];
		for(int j = 0; j < Nbins; j++){
			EvoErrProfiles[i][j] = 0;
		}
	}

	double weightsum=0;
        for(int i=0;i<number_of_lines;i++){

		if(i%1000==0){printf("Line: %i \n", i);}

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);
                double weight = paramlist[0];
		weightsum+=weight;

		 for(int i = 0; i < ndim; i++){
		         Cube[i] = paramlist[i+2];
		 }
                





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

		int cpos = 0;
		int epos = 0;
		int fpos = 0;
		int npos = 0;

		int pcount = profiledimstart;

		int maxshapecoeff = 0;
		int totshapecoeff = ((MNStruct *)globalcontext)->totshapecoeff; 

		int *numcoeff= new int[((MNStruct *)globalcontext)->numProfComponents];
		for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
			numcoeff[i] =  ((MNStruct *)globalcontext)->numshapecoeff[i];
		}

		int *numshapestoccoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;
		int totalshapestoccoeff =  ((MNStruct *)globalcontext)->totalshapestoccoeff;

		int *numEvoCoeff = ((MNStruct *)globalcontext)->numEvoCoeff;
		int totalEvoCoeff = ((MNStruct *)globalcontext)->totalEvoCoeff;

		int *numFitEvoCoeff = ((MNStruct *)globalcontext)->numEvoFitCoeff;
		int totalFitEvoCoeff  = ((MNStruct *)globalcontext)->totalEvoFitCoeff;

		int *numProfileFitCoeff = ((MNStruct *)globalcontext)->numProfileFitCoeff;
		int totalProfileFitCoeff = ((MNStruct *)globalcontext)->totalProfileFitCoeff;


		int totalCoeffForMult = 0;
		int *NumCoeffForMult = new int[((MNStruct *)globalcontext)->numProfComponents];
		for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
			NumCoeffForMult[i] = 0;
			if(numEvoCoeff[i] > NumCoeffForMult[i]){NumCoeffForMult[i]=numEvoCoeff[i];}
			if(numProfileFitCoeff[i] > NumCoeffForMult[i]){NumCoeffForMult[i]=numProfileFitCoeff[i];}
			totalCoeffForMult += NumCoeffForMult[i];
		}

		double shapecoeff[totshapecoeff];
		double StocProfCoeffs[totalshapestoccoeff];
		double EvoCoeffs[totalEvoCoeff];
		double ProfileFitCoeffs[totalProfileFitCoeff];
		double ProfileModCoeffs[totalCoeffForMult];


		for(int i =0; i < totshapecoeff; i++){
			shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
		}
		double *betas = new double[((MNStruct *)globalcontext)->numProfComponents]();
		for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
			betas[i] = ((MNStruct *)globalcontext)->MeanProfileBeta[i]*((MNStruct *)globalcontext)->ReferencePeriod;
		}

		for(int i =0; i < totalEvoCoeff; i++){
			EvoCoeffs[i]=((MNStruct *)globalcontext)->MeanProfileEvo[0][i];
//			printf("EC %i %g \n", i, EvoCoeffs[i]);
		}

		profiledimstart=pcount;
		for(int i =0; i < totalshapestoccoeff; i++){
			StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
			pcount++;
		}

		for(int i =0; i < totalProfileFitCoeff; i++){
			ProfileFitCoeffs[i]= Cube[pcount];
			pcount++;
		}
		double LinearProfileWidth=0;
		if(((MNStruct *)globalcontext)->FitLinearProfileWidth == 1){
			LinearProfileWidth = Cube[pcount];
			pcount++;
		}


		for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
			for(int i =0; i < numFitEvoCoeff[c]; i++){
				EvoCoeffs[i+cpos] += Cube[pcount];
//				printf("MEC %i %g \n", i, EvoCoeffs[i]);
				pcount++;
			}
			cpos += numEvoCoeff[c];
		}

		double EvoProfileWidth=0;
		if(((MNStruct *)globalcontext)->incProfileEvo == 2){
			EvoProfileWidth = Cube[pcount];
			pcount++;
		}

		maxshapecoeff = totshapecoeff + ((MNStruct *)globalcontext)->numProfComponents;

		long double *CompSeps = new long double[((MNStruct *)globalcontext)->numProfComponents];
		CompSeps[0] = 0;
		for(int i = 1; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		        CompSeps[i] = 0.336277178806697163*((MNStruct *)globalcontext)->ReferencePeriod;
		}


		for(int s = 0; s < numsteps; s++){

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


		

			double reffreq = ((MNStruct *)globalcontext)->EvoRefFreq;
			double freq = minfreq + s*fstep;
			double freqdiff =  (freq - reffreq)/1000.0;



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////Add in any Profile Changes///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



			double *shapevec  = new double[Nbins];
			double *ProfileModVec  = new double[Nbins];

			for(int i =0; i < totalCoeffForMult; i++){
				ProfileModCoeffs[i]=0;	
			}	

			
			cpos = 0;
			epos = 0;
			fpos = 0;
			npos = 0;

			for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){

							

				for(int i =0; i < numProfileFitCoeff[c]; i++){
					ProfileModCoeffs[i+cpos] += ProfileFitCoeffs[i+fpos];
				}


				for(int i =0; i < numEvoCoeff[c]; i++){
					//if(t==0){printf("fill evo coeff %i %i %g %g \n", NumCoeffForMult, i,  ProfileFitCoeffs[i], EvoCoeffs[i]*freqdiff);}
					ProfileModCoeffs[i+cpos] += EvoCoeffs[i+epos]*freqdiff;
		
				}

				cpos += NumCoeffForMult[c];
				epos += numEvoCoeff[c];
				fpos += numProfileFitCoeff[c];


			}

			if(totalCoeffForMult > 0){
				vector_dgemv(((MNStruct *)globalcontext)->InterpolatedShapeletsVec[InterpIndex], ProfileModCoeffs,ProfileModVec,Nbins,totalCoeffForMult,'N');
			}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////Fill Arrays with interpolated state//////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	
	
	
			for(int j =0; j < Nbins; j++){


				double widthTerm = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpIndex][j]*LinearProfileWidth; 
				double evoWidthTerm = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpIndex][j]*EvoProfileWidth*freqdiff;

				EvoProfiles[s][j] += (((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpIndex][j] + widthTerm + ProfileModVec[j] + evoWidthTerm)*weight;
				//printf("Evo: %g %i %g \n", freq, j, shapevec[j]);
			}

			delete[] shapevec;
			delete[] ProfileModVec;
		}

		delete[] numcoeff;

	}

	for(int s = 0; s < numsteps; s++){
		for(int j =0; j < Nbins; j++){
			EvoProfiles[s][j] = EvoProfiles[s][j]/weightsum;
		}
	}

	summaryfile.close();

	printf("Getting Errors with %i lines \n", number_of_lines);
	summaryfile.open(fname.c_str());	
	weightsum=0;
        for(int i=0;i<number_of_lines;i++){

		if(i%1000==0){printf("Line: %i \n", i);}

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);
                double weight = paramlist[0];
		weightsum+=weight;

		 for(int i = 0; i < ndim; i++){
		         Cube[i] = paramlist[i+2];
		 }
                





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

		int cpos = 0;
		int epos = 0;
		int fpos = 0;
		int npos = 0;


		int pcount = profiledimstart;

		int maxshapecoeff = 0;
		int totshapecoeff = ((MNStruct *)globalcontext)->totshapecoeff; 

		int *numcoeff= new int[((MNStruct *)globalcontext)->numProfComponents];
		for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
			numcoeff[i] =  ((MNStruct *)globalcontext)->numshapecoeff[i];
		}

		int *numshapestoccoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;
		int totalshapestoccoeff = ((MNStruct *)globalcontext)->totalshapestoccoeff;

		int *numEvoCoeff = ((MNStruct *)globalcontext)->numEvoCoeff;
		int totalEvoCoeff = ((MNStruct *)globalcontext)->totalEvoCoeff;

		int *numFitEvoCoeff = ((MNStruct *)globalcontext)->numEvoFitCoeff;
		int totalFitEvoCoeff  = ((MNStruct *)globalcontext)->totalEvoFitCoeff;

		int *numProfileFitCoeff = ((MNStruct *)globalcontext)->numProfileFitCoeff;
		int totalProfileFitCoeff = ((MNStruct *)globalcontext)->totalProfileFitCoeff;


		int totalCoeffForMult = 0;
		int *NumCoeffForMult = new int[((MNStruct *)globalcontext)->numProfComponents];
		for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
			NumCoeffForMult[i] = 0;
			if(numEvoCoeff[i] > NumCoeffForMult[i]){NumCoeffForMult[i]=numEvoCoeff[i];}
			if(numProfileFitCoeff[i] > NumCoeffForMult[i]){NumCoeffForMult[i]=numProfileFitCoeff[i];}
			totalCoeffForMult += NumCoeffForMult[i];
		}

		double shapecoeff[totshapecoeff];
		double StocProfCoeffs[totalshapestoccoeff];
		double EvoCoeffs[totalEvoCoeff];
		double ProfileFitCoeffs[totalProfileFitCoeff];
		double ProfileModCoeffs[totalCoeffForMult];


		for(int i =0; i < totshapecoeff; i++){
			shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
		}
		double *betas = new double[((MNStruct *)globalcontext)->numProfComponents]();
		for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
			betas[i] = ((MNStruct *)globalcontext)->MeanProfileBeta[i]*((MNStruct *)globalcontext)->ReferencePeriod;
		}

		for(int i =0; i < totalEvoCoeff; i++){
			EvoCoeffs[i]=((MNStruct *)globalcontext)->MeanProfileEvo[0][i];
		//	printf("EC %i %g \n", i, EvoCoeffs[i]);
		}

		profiledimstart=pcount;
		for(int i =0; i < totalshapestoccoeff; i++){
			StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
			pcount++;
		}

		for(int i =0; i < totalProfileFitCoeff; i++){
			ProfileFitCoeffs[i]= Cube[pcount];
			pcount++;
		}
		double LinearProfileWidth=0;
		if(((MNStruct *)globalcontext)->FitLinearProfileWidth == 1){
			LinearProfileWidth = Cube[pcount];
			pcount++;
		}


		for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
			for(int i =0; i < numFitEvoCoeff[c]; i++){
				EvoCoeffs[i+cpos] += Cube[pcount];
		//		printf("MEC %i %g \n", i, EvoCoeffs[i]);
				pcount++;
			}
			cpos += numEvoCoeff[c];
		}

		double EvoProfileWidth=0;
		if(((MNStruct *)globalcontext)->incProfileEvo == 2){
			EvoProfileWidth = Cube[pcount];
			pcount++;
		}

		maxshapecoeff = totshapecoeff + ((MNStruct *)globalcontext)->numProfComponents;

		long double *CompSeps = new long double[((MNStruct *)globalcontext)->numProfComponents];
		CompSeps[0] = 0;
		for(int i = 1; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		        CompSeps[i] = 0.336277178806697163*((MNStruct *)globalcontext)->ReferencePeriod;
		}
		



		for(int s = 0; s < numsteps; s++){

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


		

			double reffreq = ((MNStruct *)globalcontext)->EvoRefFreq;
			double freq = minfreq + s*fstep;
			double freqdiff =  (freq - reffreq)/1000.0;



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////Add in any Profile Changes///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



			double *shapevec  = new double[Nbins];
			double *ProfileModVec  = new double[Nbins];

			for(int i =0; i < totalCoeffForMult; i++){
				ProfileModCoeffs[i]=0;	
			}	

			
			cpos = 0;
			epos = 0;
			fpos = 0;
			npos = 0;

			for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){

							

				for(int i =0; i < numProfileFitCoeff[c]; i++){
					ProfileModCoeffs[i+cpos] += ProfileFitCoeffs[i+fpos];
				}


				for(int i =0; i < numEvoCoeff[c]; i++){
					//if(t==0){printf("fill evo coeff %i %i %g %g \n", NumCoeffForMult, i,  ProfileFitCoeffs[i], EvoCoeffs[i]*freqdiff);}
					ProfileModCoeffs[i+cpos] += EvoCoeffs[i+epos]*freqdiff;
		
				}

				cpos += NumCoeffForMult[c];
				epos += numEvoCoeff[c];
				fpos += numProfileFitCoeff[c];


			}

			if(totalCoeffForMult > 0){
				vector_dgemv(((MNStruct *)globalcontext)->InterpolatedShapeletsVec[InterpIndex], ProfileModCoeffs,ProfileModVec,Nbins,totalCoeffForMult,'N');
			}

			

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////Fill Arrays with interpolated state//////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	
	
	
			for(int j =0; j < Nbins; j++){


				double widthTerm = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpIndex][j]*LinearProfileWidth; 
				double evoWidthTerm = ((MNStruct *)globalcontext)->InterpolatedWidthProfile[InterpIndex][j]*EvoProfileWidth*freqdiff;

				double diff = (((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpIndex][j] + widthTerm + ProfileModVec[j] + evoWidthTerm - EvoProfiles[s][j]);
	
				EvoErrProfiles[s][j] += diff*diff*weight;
				//printf("Evo: %g %i %g \n", freq, j, shapevec[j]);
			}

			delete[] shapevec;
			delete[] ProfileModVec;
		}
		delete[] numcoeff;

	}

	for(int s = 0; s < numsteps; s++){
		for(int j =0; j < Nbins; j++){
			EvoErrProfiles[s][j] = sqrt(EvoErrProfiles[s][j]/weightsum);
		}
	}

	summaryfile.close();

	printf("Evo %g %g %g %g %g \n",minfreq + 0*fstep, minfreq + 1*fstep, minfreq + 2*fstep, minfreq + 3*fstep, minfreq + 4*fstep);
	for(int j =0; j < Nbins; j++){
		printf("Evo %i %g %g %g %g %g %g %g %g %g %g \n", j, EvoProfiles[0][j], EvoProfiles[1][j], EvoProfiles[2][j], EvoProfiles[3][j], EvoProfiles[4][j], EvoErrProfiles[0][j], EvoErrProfiles[1][j], EvoErrProfiles[2][j], EvoErrProfiles[3][j], EvoErrProfiles[4][j]);

	//	printf("Evo %.10g \n", EvoProfiles[0][j]);
	}



}



void Template2DProfLikeMNWrap(double *Cube, int &ndim, int &npars, double &lnew, void *context){



        for(int p=0;p<ndim;p++){
		//printf("PARray: %i %g %g \n", p,((MNStruct *)globalcontext)->PriorsArray[p],((MNStruct *)globalcontext)->PriorsArray[p+ndim]);
                Cube[p]=(((MNStruct *)globalcontext)->PriorsArray[p+ndim]-((MNStruct *)globalcontext)->PriorsArray[p])*Cube[p]+((MNStruct *)globalcontext)->PriorsArray[p];
        }


	double *DerivedParams = new double[npars];

	double result = Template2DProfLike(ndim, Cube, npars, DerivedParams, context);


	delete[] DerivedParams;

	lnew = result;

}

double  Template2DProfLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){


	int debug = 0;
        int pcount = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double FoldingPeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

	double *phases = new double[((MNStruct *)globalcontext)->numProfComponents]();
	//printf("Phase: %g \n",Cube[pcount] );
	phases[0] = Cube[pcount];
	pcount++;

	for(int i = 1; i < ((MNStruct *)globalcontext)->numProfComponents; i++){

		double compphase = phases[0] + Cube[pcount];
		//double WrappedCompPhase  = -FoldingPeriod/2 + fmod(FoldingPeriod + fmod(compphase + FoldingPeriod/2, FoldingPeriod), FoldingPeriod);

		phases[i] = compphase;//WrappedCompPhase;
		pcount++;
	}





	double *betas = new double[((MNStruct *)globalcontext)->numProfComponents]();
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		betas[i] = (pow(10.0, Cube[pcount]));
		pcount++;
	}


	int totalProfCoeff = 0;
	int *numcoeff= new int[((MNStruct *)globalcontext)->numProfComponents];
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		numcoeff[i] =  floor(pow(10.0, Cube[pcount]));
		totalProfCoeff += numcoeff[i];
		pcount++;
	}
	if(debug == 1){printf("Total coeff %i \n", totalProfCoeff);}

	double STau = 0;
	if(((MNStruct *)globalcontext)->incProfileScatter > 0){
		STau = pow(10.0, Cube[pcount]);	
		pcount++;
	}

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

		   
	int Nbins  = (int)((MNStruct *)globalcontext)->ProfileInfo[0][1];

	double *ProfileBats = new double[Nbins];
	double pulsesamplerate = 1.0/Nbins;

	for(int b =0; b < Nbins; b++){
		 ProfileBats[b] = pulsesamplerate*(b-Nbins/2);
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	


	int NFreqs = 0.5*Nbins;
	double *BetaFreqs = new double[NFreqs];

	double **HermiteFMatrix =  new double*[2*NFreqs];
	for(int i =0;i<2*NFreqs;i++){HermiteFMatrix[i]=new double[totalProfCoeff]();}


    	int cpos = 0;
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){

			
	

		for(int j =0; j < NFreqs; j++){
			BetaFreqs[j] = 2*M_PI*betas[i]*double(j+1);
	
		
		}
		for(int j =0; j < NFreqs; j++){

			
			TNothplMC(numcoeff[i],BetaFreqs[j],HermiteFMatrix[j], cpos);
			
			for(int c =0; c < numcoeff[i]; c++){

				if(c%4 == 1 || c%4 == 3){
					HermiteFMatrix[j+NFreqs][cpos+c] = -1*HermiteFMatrix[j][cpos+c];
					HermiteFMatrix[j][cpos+c] = 0;
				}

				if(c%4 == 2 || c%4 == 3){
					HermiteFMatrix[j+NFreqs][cpos+c] *= -1;
					HermiteFMatrix[j][cpos+c] *= -1;
				}
			}

				
		}




		for(int k =0; k < numcoeff[i]; k++){

			double Bconst=(1.0/sqrt(betas[i]*((MNStruct *)globalcontext)->ReferencePeriod))/sqrt(pow(2.0,k)*sqrt(M_PI));
			for(int j =0; j < NFreqs; j++){

				HermiteFMatrix[j][cpos+k]=HermiteFMatrix[j][cpos+k]*Bconst*exp(-0.5*BetaFreqs[j]*BetaFreqs[j])*Nbins*sqrt(2*M_PI*betas[i]*betas[i]);
				HermiteFMatrix[j+NFreqs][cpos+k]=HermiteFMatrix[j+NFreqs][cpos+k]*Bconst*exp(-0.5*BetaFreqs[j]*BetaFreqs[j])*Nbins*sqrt(2*M_PI*betas[i]*betas[i]);
			}

				for(int j = 0; j < NFreqs; j++){

					double freq = (j+1.0)/Nbins;
					double theta = 2*M_PI*phases[i]*Nbins*freq;
					double RealShift = cos(theta);
					double ImagShift = sin(-theta);

					double RData = HermiteFMatrix[j][cpos+k];
					double IData = HermiteFMatrix[j+NFreqs][cpos+k];

					double RealRolledData = RData*RealShift - IData*ImagShift;
					double ImagRolledData = RData*ImagShift + IData*RealShift;

					HermiteFMatrix[j][cpos+k] = RealRolledData;
					HermiteFMatrix[j+NFreqs][cpos+k] = ImagRolledData;


				}


		}





		cpos += numcoeff[i];
   	 }




	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


	double **M = new double*[2*NFreqs];



	int Msize = totalProfCoeff;


	if(debug ==1){printf("Made Basis Vectors %i \n", Msize);}

	for(int i =0; i < 2*NFreqs; i++){
		M[i] = new double[Msize];

		for(int j = 0; j < totalProfCoeff; j++){
			M[i][j] = HermiteFMatrix[i][j];
		}
	  
	}


	double **MNM = new double*[Msize];
	for(int i =0; i < Msize; i++){
	    MNM[i] = new double[Msize];
	}

	double **TempMNM = new double*[Msize];
	for(int i =0; i < Msize; i++){
	    TempMNM[i] = new double[Msize];
	}

	double lnew = 0;
	double Chisq=0;
	double detN = 0;

	if(((MNStruct *)globalcontext)->incProfileScatter == 0){



		dgemm(M, M , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');


		double priorval = 1000.0;

		//MNM[0][0] += 1.0/(0.01*0.01);
		for(int j = 1; j < Msize; j++){
			//printf("Prior: %i %g %g \n", j, MNM[j][j], 1.0/priorval);
		//	MNM[j][j] += 1.0/(priorval*priorval);

		}



		if(debug ==1){printf("Calculating Like \n");}

		for(int fc=0; fc < ((MNStruct *)globalcontext)->numTempFreqs; fc++){
	

			double *dNM = new double[Msize]();
			double *TempdNM = new double[Msize]();
			double *NDiffVec = new double[2*NFreqs]();
			double *shapevec = new double[2*NFreqs]();

			fftw_plan plan;
			fftw_complex *FFTData;
			double *OneData = new double[Nbins]();
			
			

			FFTData = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (Nbins/2+1));

			plan = fftw_plan_dft_r2c_1d(Nbins, OneData, FFTData, FFTW_ESTIMATE);

			for(int j = 0; j < Nbins; j++){
				OneData[j] = (double)((MNStruct *)globalcontext)->TemplateChans[fc*Nbins + j];
			}
			
			fftw_execute(plan);


			for(int j = 0; j < NFreqs; j++){
				NDiffVec[j] = FFTData[j+1][0];
				NDiffVec[j+NFreqs] = FFTData[j+1][1];
			}

			fftw_destroy_plan ( plan );
			fftw_free ( FFTData );
			delete[] OneData;

			dgemv(M,NDiffVec,dNM,2*NFreqs,Msize,'T');

			for(int i =0; i < Msize; i++){
				TempdNM[i] = dNM[i];
				for(int j =0; j < Msize; j++){
					TempMNM[i][j] = MNM[i][j];
				}
			}


			int info=0;
			double Margindet = 0;
			dpotrfInfo(TempMNM, Msize, Margindet, info);
			dpotrs(TempMNM, TempdNM, Msize);


			if(debug ==1){printf("Size: %i %g \n", Msize, Margindet);}

		
			dgemv(M,TempdNM,shapevec,2*NFreqs,Msize,'N');




			double TNoise = 0;
			double OneChisq = 0;
		

			for(int j =0; j < 2*NFreqs; j++){
				double datadiff =  NDiffVec[j] - shapevec[j];
				OneChisq += datadiff*datadiff;
			}

			TNoise = sqrt(OneChisq/((double)Nbins));
			detN += Nbins*log(TNoise*TNoise);
			Chisq += OneChisq/TNoise/TNoise;


			delete[] shapevec;
			delete[] NDiffVec;
			delete[] dNM;
			delete[] TempdNM;

			if(debug ==1 || debug ==2){printf("End Like: %.10g %g %g %g\n", lnew, detN, Chisq, OneChisq);}
		}

	}
	else{

		double **SM = new double*[2*NFreqs];
		for(int i =0; i < 2*NFreqs; i++){
			SM[i] = new double[Msize];
		}

		for(int fc=0; fc < ((MNStruct *)globalcontext)->numTempFreqs; fc++){



			double TFreq = ((MNStruct *)globalcontext)->TemplateFreqs[fc];
			double ScatterScale = pow(TFreq*pow(10.0,6),4)/pow(10.0,9.0*4.0);
			double STime = STau/ScatterScale;

			for(int c = 0; c < totalProfCoeff; c++){
				for(int b = 0; b < NFreqs; b++){

					double f = (b + 1.0)/((MNStruct *)globalcontext)->ReferencePeriod;
					double w = 2.0*M_PI*f;
					double RScatter = 1.0/(w*w*STime*STime+1); 
					double IScatter = -w*STime/(w*w*STime*STime+1);


					double RScattered = M[b][c]*RScatter - M[b+NFreqs][c]*IScatter;
					double IScattered = M[b+NFreqs][c]*RScatter + M[b][c]*IScatter;
					SM[b][c] = RScattered;
					SM[b+NFreqs][c] = IScattered;
				}

			}

			dgemm(SM, SM , MNM, 2*NFreqs, Msize, 2*NFreqs, Msize, 'T', 'N');


			double *dNM = new double[Msize]();
			double *TempdNM = new double[Msize]();
			double *NDiffVec = new double[2*NFreqs]();
			double *shapevec = new double[2*NFreqs]();

			fftw_plan plan;
			fftw_complex *FFTData;
			double *OneData = new double[Nbins]();
			
			

			FFTData = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (Nbins/2+1));

			plan = fftw_plan_dft_r2c_1d(Nbins, OneData, FFTData, FFTW_ESTIMATE);

			for(int j = 0; j < Nbins; j++){
				OneData[j] = (double)((MNStruct *)globalcontext)->TemplateChans[fc*Nbins + j];
			}
			
			fftw_execute(plan);


			for(int j = 0; j < NFreqs; j++){
				NDiffVec[j] = FFTData[j+1][0];
				NDiffVec[j+NFreqs] = FFTData[j+1][1];
			}

			fftw_destroy_plan ( plan );
			fftw_free ( FFTData );
			delete[] OneData;

			dgemv(SM,NDiffVec,dNM,2*NFreqs,Msize,'T');

			for(int i =0; i < Msize; i++){
				TempdNM[i] = dNM[i];
				for(int j =0; j < Msize; j++){
					TempMNM[i][j] = MNM[i][j];
				}
			}


			int info=0;
			double Margindet = 0;
			dpotrfInfo(TempMNM, Msize, Margindet, info);
			dpotrs(TempMNM, TempdNM, Msize);


			if(debug ==1){printf("Size: %i %g \n", Msize, Margindet);}

		
			dgemv(SM,TempdNM,shapevec,2*NFreqs,Msize,'N');




			double TNoise = 0;
			double OneChisq = 0;
		

			for(int j =0; j < 2*NFreqs; j++){
				double datadiff =  NDiffVec[j] - shapevec[j];
				OneChisq += datadiff*datadiff;
			}

			TNoise = sqrt(OneChisq/((double)Nbins));
			detN += Nbins*log(TNoise*TNoise);
			Chisq += OneChisq/TNoise/TNoise;


			delete[] shapevec;
			delete[] NDiffVec;
			delete[] dNM;
			delete[] TempdNM;
		}

		for (int j = 0; j < 2*NFreqs; j++){

		    delete[] SM[j];
		}

		delete[] SM;



	}


	lnew = -0.5*(Chisq+detN)-totalProfCoeff*((MNStruct *)globalcontext)->numTempFreqs;



	delete[] BetaFreqs;

	for (int j = 0; j < 2*NFreqs; j++){
	    delete[] HermiteFMatrix[j];
	    delete[] M[j];
	}
	delete[] HermiteFMatrix;
	delete[] M;

	for (int j = 0; j < Msize; j++){
	    delete[] MNM[j];
		delete[] TempMNM[j];
	}
	delete[] MNM;
	delete[] TempMNM;
	
	
	delete[] ProfileBats;
	delete[] numcoeff;
	delete[] betas;
	delete[] phases;

	if(debug ==1 || debug ==2){printf("End Like: %.10g %g %g\n", lnew, detN, Chisq);}

	if(debug ==1){sleep(5);}

	return lnew;

}


void WriteTemplate2DProfLike(std::string longname, int &ndim){

	//printf("In like \n");


        double *Cube = new double[ndim];
        int number_of_lines = 0;

        std::ifstream checkfile;
	std::string checkname;
	if(((MNStruct *)globalcontext)->sampler == 0){
	        checkname = longname+"phys_live.points";
	}
        if(((MNStruct *)globalcontext)->sampler == 1){
                checkname = longname+"_phys_live.txt";
        }
	printf("%i %s \n", ((MNStruct *)globalcontext)->sampler, checkname.c_str());
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        checkfile.close();

        std::ifstream summaryfile;
	std::string fname;
	if(((MNStruct *)globalcontext)->sampler == 0){
		fname = longname+"phys_live.points";
	}
	if(((MNStruct *)globalcontext)->sampler == 1){
	        fname = longname+"_phys_live.txt";
	}
        summaryfile.open(fname.c_str());



        printf("Getting ML \n");
        double maxlike = -1.0*pow(10.0,10);
        for(int i=0;i<number_of_lines;i++){

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);

                double like = paramlist[ndim];

                if(like > maxlike){
                        maxlike = like;
                         for(int i = 0; i < ndim; i++){
                                 Cube[i] = paramlist[i];
				printf("MLCube: %i %g \n", Cube[i]);
                         }
                }

        }
        summaryfile.close();
	printf("ML %g\n", maxlike);
	//printf("In like \n");



	int debug = 0;
        int pcount = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double FoldingPeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

	double *phases = new double[((MNStruct *)globalcontext)->numProfComponents]();
	//printf("Phase: %g \n",Cube[pcount] );
	phases[0] = Cube[pcount];
	pcount++;

	for(int i = 1; i < ((MNStruct *)globalcontext)->numProfComponents; i++){

		double compphase = phases[0] + Cube[pcount];
		//double WrappedCompPhase  = -FoldingPeriod/2 + fmod(FoldingPeriod + fmod(compphase + FoldingPeriod/2, FoldingPeriod), FoldingPeriod);

		phases[i] = compphase;//WrappedCompPhase;
		pcount++;
	}





	double *betas = new double[((MNStruct *)globalcontext)->numProfComponents]();
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		betas[i] = (pow(10.0, Cube[pcount]));
		pcount++;
	}


	int totalProfCoeff = 0;
	int *numcoeff= new int[((MNStruct *)globalcontext)->numProfComponents];
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		numcoeff[i] =  floor(pow(10.0, Cube[pcount]));
		totalProfCoeff += numcoeff[i];
		pcount++;
	}
	if(debug == 1){printf("Total coeff %i \n", totalProfCoeff);}

	double STau = 0;
	if(((MNStruct *)globalcontext)->incProfileScatter > 0){
		STau = pow(10.0, Cube[pcount]);	
		pcount++;
	}

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

		   
	int Nbins  = (int)((MNStruct *)globalcontext)->ProfileInfo[0][1];

	double *ProfileBats = new double[Nbins];
	double pulsesamplerate = 1.0/Nbins;

	for(int b =0; b < Nbins; b++){
		 ProfileBats[b] = pulsesamplerate*(b-Nbins/2);
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	


	int NFreqs = 0.5*Nbins;
	double *BetaFreqs = new double[NFreqs];

	double **HermiteFMatrix =  new double*[2*NFreqs];
	for(int i =0;i<2*NFreqs;i++){HermiteFMatrix[i]=new double[totalProfCoeff]();}


    	int cpos = 0;
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){

			
	

		for(int j =0; j < NFreqs; j++){
			BetaFreqs[j] = 2*M_PI*betas[i]*double(j+1);
	
		
		}
		for(int j =0; j < NFreqs; j++){

			
			TNothplMC(numcoeff[i],BetaFreqs[j],HermiteFMatrix[j], cpos);
			
			for(int c =0; c < numcoeff[i]; c++){

				if(c%4 == 1 || c%4 == 3){
					HermiteFMatrix[j+NFreqs][cpos+c] = -1*HermiteFMatrix[j][cpos+c];
					HermiteFMatrix[j][cpos+c] = 0;
				}

				if(c%4 == 2 || c%4 == 3){
					HermiteFMatrix[j+NFreqs][cpos+c] *= -1;
					HermiteFMatrix[j][cpos+c] *= -1;
				}
			}

				
		}




		for(int k =0; k < numcoeff[i]; k++){

			double Bconst=(1.0/sqrt(betas[i]*((MNStruct *)globalcontext)->ReferencePeriod))/sqrt(pow(2.0,k)*sqrt(M_PI));
			for(int j =0; j < NFreqs; j++){

				HermiteFMatrix[j][cpos+k]=HermiteFMatrix[j][cpos+k]*Bconst*exp(-0.5*BetaFreqs[j]*BetaFreqs[j])*Nbins*sqrt(2*M_PI*betas[i]*betas[i]);
				HermiteFMatrix[j+NFreqs][cpos+k]=HermiteFMatrix[j+NFreqs][cpos+k]*Bconst*exp(-0.5*BetaFreqs[j]*BetaFreqs[j])*Nbins*sqrt(2*M_PI*betas[i]*betas[i]);
			}

				for(int j = 0; j < NFreqs; j++){

					double freq = (j+1.0)/Nbins;
					double theta = 2*M_PI*phases[i]*Nbins*freq;
					double RealShift = cos(theta);
					double ImagShift = sin(-theta);

					double RData = HermiteFMatrix[j][cpos+k];
					double IData = HermiteFMatrix[j+NFreqs][cpos+k];

					double RealRolledData = RData*RealShift - IData*ImagShift;
					double ImagRolledData = RData*ImagShift + IData*RealShift;

					HermiteFMatrix[j][cpos+k] = RealRolledData;
					HermiteFMatrix[j+NFreqs][cpos+k] = ImagRolledData;


				}


		}





		cpos += numcoeff[i];
   	 }




	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


	double **M = new double*[2*NFreqs];



	int Msize = totalProfCoeff;


	if(debug ==1){printf("Made Basis Vectors %i \n", Msize);}

	for(int i =0; i < 2*NFreqs; i++){
		M[i] = new double[Msize];

		for(int j = 0; j < totalProfCoeff; j++){
			M[i][j] = HermiteFMatrix[i][j];
		}
	  
	}


	double **MNM = new double*[Msize];
	for(int i =0; i < Msize; i++){
	    MNM[i] = new double[Msize];
	}

	double **TempMNM = new double*[Msize];
	for(int i =0; i < Msize; i++){
	    TempMNM[i] = new double[Msize];
	}


	double **CoeffData = new double *[totalProfCoeff];
	for(int fc=0; fc < totalProfCoeff; fc++){
		CoeffData[fc] = new double[3*((MNStruct *)globalcontext)->numTempFreqs]();
	}



	double lnew = 0;
	double Chisq=0;
	double detN = 0;


	if(((MNStruct *)globalcontext)->incProfileScatter == 0){




		dgemm(M, M , MNM, 2*NFreqs, Msize,2*NFreqs, Msize, 'T', 'N');


		double priorval = 1000.0;

			//MNM[0][0] += 1.0/(0.01*0.01);
			for(int j = 1; j < Msize; j++){
				//printf("Prior: %i %g %g \n", j, MNM[j][j], 1.0/priorval);
				//MNM[j][j] += 1.0/(priorval*priorval);

			}



		if(debug ==1){printf("Calculating Like \n");}

		for(int fc=0; fc < ((MNStruct *)globalcontext)->numTempFreqs; fc++){
	

			double *dNM = new double[Msize]();
			double *TempdNM = new double[Msize]();
			double *NDiffVec = new double[2*NFreqs]();
			double *shapevec = new double[2*NFreqs]();
			double *CopyTempdNM = new double[Msize]();
			double **ProfShape = new double*[((MNStruct *)globalcontext)->numProfComponents];
			for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){ProfShape[i] = new double[2*NFreqs]();}

			fftw_plan plan;
			fftw_complex *FFTData;
			double *OneData = new double[Nbins]();
			
			

			FFTData = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (Nbins/2+1));

			plan = fftw_plan_dft_r2c_1d(Nbins, OneData, FFTData, FFTW_ESTIMATE);

			for(int j = 0; j < Nbins; j++){
				OneData[j] = (double)((MNStruct *)globalcontext)->TemplateChans[fc*Nbins + j];
			}
			
			fftw_execute(plan);


			for(int j = 0; j < NFreqs; j++){
				NDiffVec[j] = FFTData[j+1][0];
				NDiffVec[j+NFreqs] = FFTData[j+1][1];
			}

			fftw_destroy_plan ( plan );
			fftw_free ( FFTData );
			delete[] OneData;


			dgemv(M,NDiffVec,dNM,2*NFreqs,Msize,'T');

			for(int i =0; i < Msize; i++){
				TempdNM[i] = dNM[i];
				for(int j =0; j < Msize; j++){
					TempMNM[i][j] = MNM[i][j];
				}
			}

			int info=0;
			double Margindet = 0;
			dpotrfInfo(TempMNM, Msize, Margindet, info);
			dpotri(TempMNM, Msize);
			dgemv(TempMNM,dNM,TempdNM,Msize,Msize,'N');

		
			dgemv(M,TempdNM,shapevec,2*NFreqs,Msize,'N');


			for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){

				for(int j =0; j < Msize; j++){
					CopyTempdNM[j] = TempdNM[j];
				}
				cpos = 0;

				for(int j = 0; j < ((MNStruct *)globalcontext)->numProfComponents; j++){
		
					if(j != i){
						for(int k =0; k < numcoeff[j]; k++){
							CopyTempdNM[cpos+k] = 0;
						}
					}
					cpos += numcoeff[j];

				}

				dgemv(M,CopyTempdNM,ProfShape[i],2*NFreqs,Msize,'N');
			}





			double TNoise = 0;
			double OneChisq = 0;
		

			for(int j =0; j < 2*NFreqs; j++){
				double datadiff =  NDiffVec[j] - shapevec[j];
				OneChisq += datadiff*datadiff;
			}

			TNoise = sqrt(OneChisq/((double)Nbins));
			detN += Nbins*log(TNoise*TNoise);
			Chisq += OneChisq/TNoise/TNoise;


			for(int j = 0; j < Msize; j++){
				CoeffData[j][3*fc + 0] = ((MNStruct *)globalcontext)->TemplateFreqs[fc];
				CoeffData[j][3*fc + 1] = TempdNM[j]/TempdNM[0];
				CoeffData[j][3*fc + 2] = sqrt(TempMNM[j][j]*TNoise*TNoise)/TempdNM[0];

			}

			fftw_plan plan2;
	                fftw_complex *FFTData2;
			double *TimeShapeVec = new double[Nbins];
	                FFTData2 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (Nbins/2+1));
	
        	        plan2 = fftw_plan_dft_c2r_1d(Nbins, FFTData2, TimeShapeVec, FFTW_ESTIMATE);
			FFTData2[0][0] = 0;
			FFTData2[0][1] = 0;
	                for(int b = 1; b < NFreqs; b++){
                                FFTData2[b][0] = shapevec[b-1];
                                FFTData2[b][1] = shapevec[b+NFreqs-1];
                        }

                        fftw_execute(plan2);	


			std::ofstream profilefile;
			char toanum[15];
			sprintf(toanum, "%d", fc);
			std::string ProfileName =  toanum;
			std::string dname = longname+ProfileName+"-Template.txt";

			double MeanTS = 0;

			for(int j =0; j < Nbins; j++){
				MeanTS += (double)((MNStruct *)globalcontext)->TemplateChans[fc*Nbins + j];
			}
			MeanTS /= Nbins;
	
			profilefile.open(dname.c_str());
			for(int j =0; j < Nbins; j++){
				profilefile << j << " " << std::setprecision(10) << (double)((MNStruct *)globalcontext)->TemplateChans[fc*Nbins + j] << " " << TimeShapeVec[j]/Nbins+MeanTS << " " << TNoise << " ";
				for(int k = 0; k < ((MNStruct *)globalcontext)->numProfComponents; k++){
					profilefile << ProfShape[k][j] << " ";
				}
				profilefile << "\n";
			}
	    		profilefile.close();

			std::ofstream stdfile;
			char stdtoanum[15];
			sprintf(stdtoanum, "%d", fc);
			std::string stdProfileName =  toanum;
			std::string stdname = longname+stdProfileName+"-Template.std";
	
			stdfile.open(stdname.c_str());
			double mintemp = pow(10.0, 100);
			for(int j =0; j < Nbins; j++){
				if(shapevec[j] < mintemp){mintemp=shapevec[j];}
			}
			for(int j =0; j < Nbins; j++){
				stdfile << j << " " << std::setprecision(10) << " " << shapevec[j]-mintemp << "\n";
			}
	    		stdfile.close();


			fftw_free(FFTData2);
	                fftw_destroy_plan ( plan2 );
			delete[] CopyTempdNM;
			delete[] ProfShape;
			delete[] shapevec;
			delete[] NDiffVec;
			delete[] dNM;
			delete[] TempdNM;

			if(debug ==1 || debug ==2){printf("End Like: %.10g %g %g %g\n", lnew, detN, Chisq, OneChisq);}
		}

	}
	else{

                double **SM = new double*[2*NFreqs];
                for(int i =0; i < 2*NFreqs; i++){
                        SM[i] = new double[Msize];
                }

                for(int fc=0; fc < ((MNStruct *)globalcontext)->numTempFreqs; fc++){


                        double TFreq = ((MNStruct *)globalcontext)->TemplateFreqs[fc];
                        double ScatterScale = pow(TFreq*pow(10.0,6),4)/pow(10.0,9.0*4.0);
                        double STime = STau/ScatterScale;

                        for(int c = 0; c < totalProfCoeff; c++){
                                for(int b = 0; b < NFreqs; b++){

                                        double f = (b + 1.0)/((MNStruct *)globalcontext)->ReferencePeriod;
                                        double w = 2.0*M_PI*f;
                                        double RScatter = 1.0/(w*w*STime*STime+1);
                                        double IScatter = -w*STime/(w*w*STime*STime+1);


                                        double RScattered = M[b][c]*RScatter - M[b+NFreqs][c]*IScatter;
                                        double IScattered = M[b+NFreqs][c]*RScatter + M[b][c]*IScatter;
                                        SM[b][c] = RScattered;
                                        SM[b+NFreqs][c] = IScattered;
					if(c==0){printf("Scatter Comp: %i %g %g %g %g \n", b, M[b][c], M[b+NFreqs][c], RScattered, IScattered);}
                                }

                        }

                        dgemm(SM, SM , MNM, 2*NFreqs, Msize,2*NFreqs, Msize, 'T', 'N');


			double *dNM = new double[Msize]();
			double *TempdNM = new double[Msize]();
			double *NDiffVec = new double[2*NFreqs]();
			double *shapevec = new double[2*NFreqs]();
			double *CopyTempdNM = new double[Msize]();
			double **ProfShape = new double*[((MNStruct *)globalcontext)->numProfComponents];
			for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){ProfShape[i] = new double[2*NFreqs]();}

			fftw_plan plan;
			fftw_complex *FFTData;
			double *OneData = new double[Nbins]();
			
			

			FFTData = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (Nbins/2+1));

			plan = fftw_plan_dft_r2c_1d(Nbins, OneData, FFTData, FFTW_ESTIMATE);

			for(int j = 0; j < Nbins; j++){
				OneData[j] = (double)((MNStruct *)globalcontext)->TemplateChans[fc*Nbins + j];
			}
			
			fftw_execute(plan);


			for(int j = 0; j < NFreqs; j++){
				NDiffVec[j] = FFTData[j+1][0];
				NDiffVec[j+NFreqs] = FFTData[j+1][1];
			}

			fftw_destroy_plan ( plan );
			fftw_free ( FFTData );
			delete[] OneData;


			dgemv(SM,NDiffVec,dNM,2*NFreqs,Msize,'T');

			for(int i =0; i < Msize; i++){
				TempdNM[i] = dNM[i];
				for(int j =0; j < Msize; j++){
					TempMNM[i][j] = MNM[i][j];
				}
			}

			int info=0;
			double Margindet = 0;
			dpotrfInfo(TempMNM, Msize, Margindet, info);
			dpotri(TempMNM, Msize);
			dgemv(TempMNM,dNM,TempdNM,Msize,Msize,'N');

		
			dgemv(SM,TempdNM,shapevec,2*NFreqs,Msize,'N');


			for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){

				for(int j =0; j < Msize; j++){
					CopyTempdNM[j] = TempdNM[j];
				}
				cpos = 0;

				for(int j = 0; j < ((MNStruct *)globalcontext)->numProfComponents; j++){
		
					if(j != i){
						for(int k =0; k < numcoeff[j]; k++){
							CopyTempdNM[cpos+k] = 0;
						}
					}
					cpos += numcoeff[j];

				}

				dgemv(SM,CopyTempdNM,ProfShape[i],2*NFreqs,Msize,'N');
			}





			double TNoise = 0;
			double OneChisq = 0;
		

			for(int j =0; j < 2*NFreqs; j++){
				double datadiff =  NDiffVec[j] - shapevec[j];
				OneChisq += datadiff*datadiff;
			}

			TNoise = sqrt(OneChisq/((double)Nbins));
			detN += Nbins*log(TNoise*TNoise);
			Chisq += OneChisq/TNoise/TNoise;


			for(int j = 0; j < Msize; j++){
				CoeffData[j][3*fc + 0] = ((MNStruct *)globalcontext)->TemplateFreqs[fc];
				CoeffData[j][3*fc + 1] = TempdNM[j]/TempdNM[0];
				CoeffData[j][3*fc + 2] = sqrt(TempMNM[j][j]*TNoise*TNoise)/TempdNM[0];

			}

			fftw_plan plan2;
	                fftw_complex *FFTData2;
			double *TimeShapeVec = new double[Nbins];
	                FFTData2 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (Nbins/2+1));
	
        	        plan2 = fftw_plan_dft_c2r_1d(Nbins, FFTData2, TimeShapeVec, FFTW_ESTIMATE);
			FFTData2[0][0] = 0;
			FFTData2[0][1] = 0;
	                for(int b = 1; b < NFreqs; b++){
                                FFTData2[b][0] = shapevec[b-1];
                                FFTData2[b][1] = shapevec[b+NFreqs-1];
                        }

                        fftw_execute(plan2);	


			std::ofstream profilefile;
			char toanum[15];
			sprintf(toanum, "%d", fc);
			std::string ProfileName =  toanum;
			std::string dname = longname+ProfileName+"-Template.txt";

			double MeanTS = 0;

			for(int j =0; j < Nbins; j++){
				MeanTS += (double)((MNStruct *)globalcontext)->TemplateChans[fc*Nbins + j];
			}
			MeanTS /= Nbins;
	
			profilefile.open(dname.c_str());
			for(int j =0; j < Nbins; j++){
				profilefile << j << " " << std::setprecision(10) << (double)((MNStruct *)globalcontext)->TemplateChans[fc*Nbins + j] << " " << TimeShapeVec[j]/Nbins+MeanTS << " " << TNoise << " ";
				for(int k = 0; k < ((MNStruct *)globalcontext)->numProfComponents; k++){
					profilefile << ProfShape[k][j] << " ";
				}
				profilefile << "\n";
			}
	    		profilefile.close();

			std::ofstream stdfile;
			char stdtoanum[15];
			sprintf(stdtoanum, "%d", fc);
			std::string stdProfileName =  toanum;
			std::string stdname = longname+stdProfileName+"-Template.std";
	
			stdfile.open(stdname.c_str());
			double mintemp = pow(10.0, 100);
			for(int j =0; j < Nbins; j++){
				if(shapevec[j] < mintemp){mintemp=shapevec[j];}
			}
			for(int j =0; j < Nbins; j++){
				stdfile << j << " " << std::setprecision(10) << " " << shapevec[j]-mintemp << "\n";
			}
	    		stdfile.close();


			fftw_free(FFTData2);
	                fftw_destroy_plan ( plan2 );
			delete[] CopyTempdNM;
			delete[] ProfShape;
			delete[] shapevec;
			delete[] NDiffVec;
			delete[] dNM;
			delete[] TempdNM;

		}

		for (int j = 0; j < 2*NFreqs; j++){

		    delete[] SM[j];
		}

		delete[] SM;



	}


	lnew = -0.5*(Chisq+detN)-totalProfCoeff*((MNStruct *)globalcontext)->numTempFreqs;
	printf("End Like: %.10g %g %g\n", lnew, detN, Chisq);


	std::ofstream ProfilePolyFile;
	std::string Polyname = longname+"PolyFit.dat";

	ProfilePolyFile.open(Polyname.c_str());

 	int npoly = ((MNStruct *)globalcontext)->NProfileEvoPoly+1;
	double EvoRefFreq = ((MNStruct *)globalcontext)->EvoRefFreq;

	printf("Fitting polynomial using %i components with %g as ref freq \n", npoly, EvoRefFreq);


	gsl_matrix *X = gsl_matrix_alloc(((MNStruct *)globalcontext)->numTempFreqs, npoly);
	gsl_vector *y = gsl_vector_alloc(((MNStruct *)globalcontext)->numTempFreqs);
	gsl_vector *w = gsl_vector_alloc(((MNStruct *)globalcontext)->numTempFreqs);

	gsl_vector *c = gsl_vector_alloc(npoly);
	gsl_matrix *cov = gsl_matrix_alloc(npoly, npoly);

	double *PolyResults = new double[npoly*totalProfCoeff]();

	for(int coeff = 0; coeff < totalProfCoeff; coeff++){

		for (int i = 0; i < ((MNStruct *)globalcontext)->numTempFreqs; i++){
	      
			double freq = (CoeffData[coeff][i*3+0] - EvoRefFreq)/1000.0;
			for(int q = 0; q <  npoly; q++){
				gsl_matrix_set(X, i, q, pow(freq,q));
			}

			gsl_vector_set(y, i, CoeffData[coeff][i*3+1]);
			gsl_vector_set(w, i, 1.0/(CoeffData[coeff][i*3+2]*CoeffData[coeff][i*3+2]));

			if(fabs(freq) < 0.01/1000.0){
//				printf("This is the ref freq: %i %g %g \n", i, freq, EvoRefFreq);
				gsl_vector_set(w, i, pow(10.0,10));
			}
		}

		double gslchisq = 0;
		gsl_multifit_linear_workspace *work  = gsl_multifit_linear_alloc (((MNStruct *)globalcontext)->numTempFreqs, npoly);
		gsl_multifit_wlinear (X, w, y, c, cov, &gslchisq, work);
		gsl_multifit_linear_free (work);

#define COV(i,j) (gsl_matrix_get(cov,(i),(j)))

//	    printf ("# best fit: Y = %g + %g X + %g X^2\n", gsl_vector_get(c,(0)), gsl_vector_get(c,(1)), gsl_vector_get(c,(2)));

		for(int q = 0; q < npoly; q++){
			PolyResults[q*totalProfCoeff + coeff] = gsl_vector_get(c,(q));
		}

		for (int i = 0; i < ((MNStruct *)globalcontext)->numTempFreqs; i++){
			double freq = (CoeffData[coeff][i*3+0] - EvoRefFreq)/1000;
			double Signal=0;
			for(int q = 0; q < npoly; q++){
				Signal += gsl_vector_get(c,(q))*pow(freq,q);
//				gsl_vector_get(c,(0)) + gsl_vector_get(c,(1))*freq + gsl_vector_get(c,(2))*freq*freq;
			}
			ProfilePolyFile << freq << " " << CoeffData[coeff][i*3+1] << " " << 1.0/sqrt(gsl_vector_get(w,(i))) << " " << Signal << "\n";
	//		printf("Signal: %i %g %g %g %g \n", i, freq, CoeffData[coeff][i*3+1], 1.0/sqrt(gsl_vector_get(w,(i))), Signal);
		}
		ProfilePolyFile << "\n";

	}

	gsl_matrix_free (X);
	gsl_vector_free (y);
	gsl_vector_free (w);
	gsl_vector_free (c);
	gsl_matrix_free (cov);
	ProfilePolyFile.close();


	std::ofstream ProfileTemplateFile;
	std::string PolyTemplatename = longname+"ProfInfo.Template.dat";

	ProfileTemplateFile.open(PolyTemplatename.c_str());
	int EvoCoeffToWrite = 0;
	if(npoly > 0){ EvoCoeffToWrite = totalProfCoeff;}


	ProfileTemplateFile << totalProfCoeff << " " << EvoCoeffToWrite << "\n";

	double *MeanProf = new double[totalProfCoeff]();
	for(int c = 0; c < totalProfCoeff; c++){
		ProfileTemplateFile << PolyResults[c] << "\n";
		printf("Mean Prof O: %i %g \n", c,  PolyResults[c]);
		MeanProf[c] = PolyResults[c];		
	}
	((MNStruct *)globalcontext)->MeanProfileShape = MeanProf;

	double *MeanBeta= new double[((MNStruct *)globalcontext)->numProfComponents]();
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		printf("Mean Prof B: %i %g \n", i,  betas[i]);
                ProfileTemplateFile << betas[i] << "\n";
		MeanBeta[i] = betas[i];
        }
	((MNStruct *)globalcontext)->MeanProfileBeta = MeanBeta;
	double **MeanProfEvo = new double*[npoly-1];
	for(int q = 1; q < npoly; q++){
		MeanProfEvo[q-1] = new double[totalProfCoeff]();
		PolyResults[q*totalProfCoeff + 0] = 0;
		for(int c = 0; c < totalProfCoeff; c++){
			printf("Mean Prof E: %i %i %g \n",q,  c,  PolyResults[q*totalProfCoeff + c]);
			ProfileTemplateFile << PolyResults[q*totalProfCoeff + c] << "\n";
			MeanProfEvo[q-1][c] = PolyResults[q*totalProfCoeff + c];
		}
	}
	((MNStruct *)globalcontext)->MeanProfileEvo = MeanProfEvo;
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		((MNStruct *)globalcontext)->numshapecoeff[i] = numcoeff[i];
		((MNStruct *)globalcontext)->numProfileFitCoeff[i] = numcoeff[i];
	}
	((MNStruct *)globalcontext)->totshapecoeff = totalProfCoeff;

	for(int i = 1; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		((MNStruct *)globalcontext)->ProfCompSeps[i] = Cube[i];
	}

	if(((MNStruct *)globalcontext)->incProfileScatter == 1){
		((MNStruct *)globalcontext)->MeanScatter = STau;
	}

	ProfileTemplateFile.close();

	delete[] PolyResults;

	delete[] BetaFreqs;

	for (int j = 0; j < 2*NFreqs; j++){
	    delete[] HermiteFMatrix[j];
	    delete[] M[j];
	}
	delete[] HermiteFMatrix;
	delete[] M;

	for (int j = 0; j < Msize; j++){
	    delete[] MNM[j];
		delete[] TempMNM[j];
	}
	delete[] MNM;
	delete[] TempMNM;
	
	
	delete[] ProfileBats;
	delete[] numcoeff;
	delete[] betas;
	delete[] phases;

}

*/
/*
double FourierPhaseLike(double phase, int print){



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form the BATS///////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	int NEpochs = ((MNStruct *)globalcontext)->numProfileEpochs;
	int TotalProfs = ((MNStruct *)globalcontext)->pulse->nobs;


	 long double *ProfileBats=new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){

		ProfileBats[i] = ((MNStruct *)globalcontext)->ProfileData[i][0][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
	      
		ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY;

	 }





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	long double ldphase = (long double)phase;
	ldphase *= ((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){

		//if(i==0){printf("MB: %.15Lg %.15Lg %.15Lg\n", ModelBats[i] , ModelBats[i] - ldphase, ((MNStruct *)globalcontext)->pulse->obsn[i].residual);}
     		 ModelBats[i] -=  ldphase;
	}	





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get Profile Parameters//////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	int NFBasis = ((MNStruct *)globalcontext)->NFBasis;
	int totshapecoeff = ((MNStruct *)globalcontext)->totshapecoeff; 

	int *numcoeff= new int[((MNStruct *)globalcontext)->numProfComponents];
	for(int i = 0; i < ((MNStruct *)globalcontext)->numProfComponents; i++){
		numcoeff[i] =  ((MNStruct *)globalcontext)->numshapecoeff[i];
	}


	double **ProfCoeffs=new double*[1+((MNStruct *)globalcontext)->NProfileEvoPoly]; 
	for(int i = 0; i < 1+((MNStruct *)globalcontext)->NProfileEvoPoly; i++){ProfCoeffs[i] = new double[totshapecoeff]();}
	//printf("Total: %i \n",totshapecoeff );
	ProfCoeffs[0][0] = ((MNStruct *)globalcontext)->MeanProfileShape[0];
	for(int i = 1; i < totshapecoeff; i++){
		ProfCoeffs[0][i] = ((MNStruct *)globalcontext)->MeanProfileShape[i];
		//printf("Mean Prof: %i %g \n", i, ProfCoeffs[0][i]);
	}

	for(int p = 1; p < ((MNStruct *)globalcontext)->NProfileEvoPoly; p++){	
		for(int i =1; i < totshapecoeff; i++){
			ProfCoeffs[p][i] = ((MNStruct *)globalcontext)->MeanProfileEvo[p-1][i];
		}
	}



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////




	double lnew = 0;
	int GlobalNBins = (int)((MNStruct *)globalcontext)->LargestNBins;
	int ProfileBaselineTerms = ((MNStruct *)globalcontext)->ProfileBaselineTerms;


	int minep = 0;
	int maxep = ((MNStruct *)globalcontext)->numProfileEpochs;

	
	double *eplikes = new double[((MNStruct *)globalcontext)->numProfileEpochs]();

	for(int ep = minep; ep < maxep; ep++){

		double *TotalProfCoeffs = new double[totshapecoeff];

		int t = 0;

		for(int sep = 0; sep < ep; sep++){	
			t += ((MNStruct *)globalcontext)->numChanPerInt[sep];
		}
		

		int NChanInEpoch = ((MNStruct *)globalcontext)->numChanPerInt[ep];


		double EpochChisq = 0;	
		double EpochdetN = 0;
		double EpochLike = 0;







		for(int ch = 0; ch < NChanInEpoch; ch++){

			int nTOA = t;

			long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];


			int Nbins = GlobalNBins;
			int ProfNbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
			int BinRatio = Nbins/ProfNbins;


			long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

			double *RolledData  = new double[2*NFBasis]();
			double *ProfileVec  = new double[2*NFBasis]();



			/////////////////////////////////////////////////////////////////////////////////////////////  
			/////////////////////////Get and modify binpos///////////////////////////////////////////////
			/////////////////////////////////////////////////////////////////////////////////////////////
		    
			long double binpos = ModelBats[nTOA]; 


			long double binposSec = (ProfileBats[nTOA] - ModelBats[nTOA])*SECDAY;
			double DFP = double(FoldingPeriod);
			long double WrapBinPos = -FoldingPeriod/2 + fmod(FoldingPeriod + fmod(binposSec+FoldingPeriod/2, FoldingPeriod), FoldingPeriod);

			long double maxbinvalue = (FoldingPeriod/Nbins);
			double DNewInterpBin = double(fmod(maxbinvalue + fmod(-WrapBinPos, maxbinvalue), maxbinvalue)/((MNStruct *)globalcontext)->InterpolatedTime);
			int InterpBin = floor(DNewInterpBin+0.5);
			InterpBin = InterpBin%((MNStruct *)globalcontext)->NumToInterpolate;


			

			long double NewWholeBinTime = -WrapBinPos-InterpBin*((MNStruct *)globalcontext)->InterpolatedTime;
			int RollBin = int(round(NewWholeBinTime/maxbinvalue));
			int WrapRollBin = ((Nbins) + -RollBin % (Nbins)) % (Nbins);


			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//////////////////////////////////////////////////////Add in any Profile Changes///////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			double reffreq = ((MNStruct *)globalcontext)->EvoRefFreq;
			double freqdiff =  (((MNStruct *)globalcontext)->pulse->obsn[nTOA].freq - reffreq)/1000.0;


			for(int i =0; i < totshapecoeff; i++){
				TotalProfCoeffs[i]=0;	
			}				

			int cpos = 0;
	
			for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
				for(int p = 0; p < 1 + ((MNStruct *)globalcontext)->NProfileEvoPoly; p++){	
					for(int i = 0; i < numcoeff[c]; i++){
						TotalProfCoeffs[i+cpos] += ProfCoeffs[p][i+cpos]*pow(freqdiff, p);						
					}
				}

				cpos += numcoeff[c];
			}




			vector_dgemv(((MNStruct *)globalcontext)->InterpolatedFBasis[InterpBin], TotalProfCoeffs, ProfileVec,2*NFBasis,totshapecoeff,'N');



		///////////////////////////////////////////Rotate Data Vector////////////////////////////////////////////////////////////





			for(int j = 0; j < NFBasis; j++){

				double freq = (j+1.0)/ProfNbins;
				double theta = 2*M_PI*WrapRollBin*freq;
				double RealShift = cos(theta);
				double ImagShift = sin(-theta);

				double RData = ((MNStruct *)globalcontext)->FourierProfileData[t][1+j];
				double IData = ((MNStruct *)globalcontext)->FourierProfileData[t][1+j+ProfNbins/2+1];

				double RealRolledData = RData*RealShift - IData*ImagShift;
				double ImagRolledData = RData*ImagShift + IData*RealShift;

				RolledData[j] = RealRolledData;
				RolledData[j+NFBasis] = ImagRolledData;

			}

			if(((MNStruct *)globalcontext)->incProfileScatter == 1){

				double STau = ((MNStruct *)globalcontext)->MeanScatter;
				double TFreq = ((MNStruct *)globalcontext)->pulse->obsn[nTOA].freq;
				double ScatterScale = pow(TFreq*pow(10.0,6),4)/pow(10.0,9.0*4.0);
				double STime = STau/ScatterScale;


				for(int j = 0; j < NFBasis; j++){


					double f = (j+1.0)/((MNStruct *)globalcontext)->ReferencePeriod;
					double w = 2.0*M_PI*f;
					double RScatter = 1.0/(w*w*STime*STime+1); 
					double IScatter = -w*STime/(w*w*STime*STime+1);

					//if(c==0){printf("Scattered Basis: %i %g %g %g %g \n", b,RScatter, IScatter, FFTBasis[b][0], FFTBasis[b][1]);}

					double RScattered = ProfileVec[j]*RScatter - ProfileVec[j+NFBasis]*IScatter;
					double IScattered = ProfileVec[j+NFBasis]*RScatter + ProfileVec[j]*IScatter;
					ProfileVec[j] = RScattered;
					ProfileVec[j+NFBasis] = IScattered;

				}


			}



		///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////

			      
			double Chisq = 0;
			double detN = 0;
			double profilelike=0;
	


			double MNM = 0;
			double dNM = 0;
			for(int j = 0; j < 2*NFBasis; j++){

				
				MNM += ProfileVec[j]*ProfileVec[j];	
				dNM += ProfileVec[j]*RolledData[j];
			}

			double MaxAmp = dNM/MNM;

			
			for(int j = 0; j < 2*NFBasis; j++){

				//if(t==0){printf("Get Phase: %i %g %g \n", j, MaxAmp*ProfileVec[j], RolledData[j]);}

				double res = ProfileVec[j]*MaxAmp - RolledData[j];

				if(t==0 && print == 1){printf("Chisq: %i %i %i %g %g %g \n", j, InterpBin, WrapRollBin, (double) phase, RolledData[j], MaxAmp*ProfileVec[j]);}
				Chisq += res*res;
			}

			//printf("Max: %i %g %g \n", t, MaxAmp, sqrt(Chisq/(2*NFBasis)));
			double noise = sqrt(Chisq/(2*NFBasis));
			double OffPulsedetN = log(noise*noise);
			detN = 2*NFBasis*OffPulsedetN;

			Chisq = Chisq/(noise*noise);
			profilelike = -0.5*(detN + Chisq);



			EpochChisq+=Chisq;
			EpochdetN+=detN;
			EpochLike+=profilelike;

			delete[] ProfileVec;
			delete[] RolledData;




			t++;
		}



///////////////////////////////////////////


		if(((MNStruct *)globalcontext)->incWideBandNoise == 0){
			eplikes[ep] += EpochLike;
			lnew += EpochLike;
		}
		delete[] TotalProfCoeffs;

////////////////////////////////////////////
	
	}

	for(int j = 0; j < 1+((MNStruct *)globalcontext)->NProfileEvoPoly; j++){
	    delete[] ProfCoeffs[j];
	}
	delete[] ProfCoeffs;
	delete[] numcoeff;

	
	delete[] ProfileBats;
	delete[] ModelBats;





	double likelihood = 0;
	for(int ep = minep; ep < maxep; ep++){
		likelihood += eplikes[ep];
	}
	delete[] eplikes;

	return likelihood;



	
}

*/
/*
double FitPhase(){

	printf("Getting initial fit to phase using full data\n");

	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       


	std::vector<double> phases;
	std::vector<double> likes;

	double minphase = 0.0;
	double maxphase = 1.0;
	double mphase = 0;

	//double like =  FourierPhaseLike(0.28452636511326848297);


	for(int loop = 0; loop < 4; loop++){
		double stepsize = (maxphase-minphase)/100;
		double mlike = -pow(10.0, 100);
		mphase=0;
		for(int p = 0; p < 100; p++){ 

			double phase = minphase + p*stepsize;
			double like =  FourierPhaseLike(phase, 0);
			phases.push_back(phase);
			likes.push_back(like);
			
			//printf("Phases: %g %g \n", phase, like);	
	
			if(like > mlike){
				mlike = like;
				mphase = phase;
			}
		}


		minphase = mphase - stepsize;
		maxphase = mphase + stepsize;
	}


	FourierPhaseLike(mphase, 1);
	for(int i = 0; i < phases.size(); i++){
		printf("Phases: %g %.10g \n", phases[i], likes[i]);	
	}

	return mphase;

}

*/

/*

double negloglikelihood(const gsl_vector *pvP, void *context) {

	int ndims=((MNStruct *)globalcontext)->numdims;
	double *pdParameters = new double[ndims];
	int npars = 0;
	double *DerivedParams = new double[npars];
	int nParameters;
	double lval;

	printf("num dims in max %i \n", ndims);
	
	// Obtain the pointer to the model
	
	for(int i=0; i<ndims; i++) {
		pdParameters[i] = gsl_vector_get(pvP, i);
  	} // for i


	lval=ProfileDomainLike(ndims, pdParameters, npars, DerivedParams, context);

	printf("like %.10g \n", lval);

	delete[] pdParameters;
	delete[] DerivedParams;
	
	//printf("Like: %g \n",lval);
	// The loglikelihood function is virtual, so the correct one is called
	return -lval;
} // loglikelihood
*/
int llongturn_hms(long double turn, char *hms){
 
  /* Converts double turn to string " hh:mm:ss.ssss" */
  
  int hh, mm, isec;
  long double sec;

  hh = (int)(turn*24.);
  mm = (int)((turn*24.-hh)*60.);
  sec = ((turn*24.-hh)*60.-mm)*60.;
  isec = (int)((sec*10000. +0.5)/10000);
    if(isec==60){
      sec=0.;
      mm=mm+1;
      if(mm==60){
        mm=0;
        hh=hh+1;
        if(hh==24){
          hh=0;
        }
      }
    }

  sprintf(hms," %02d:%02d:%.12Lg",hh,mm,sec);
  return 0;
}

int llongturn_dms(long double turn, char *dms){
  
  /* Converts double turn to string "sddd:mm:ss.sss" */
  
  int dd, mm, isec;
  long double trn, sec;
  char sign;
  
  sign=' ';
  if (turn < 0.){
    sign = '-';
    trn = -turn;
  }
  else{
    sign = '+';
    trn = turn;
  }
  dd = (int)(trn*360.);
  mm = (int)((trn*360.-dd)*60.);
  sec = ((trn*360.-dd)*60.-mm)*60.;
  isec = (int)((sec*1000. +0.5)/1000);
    if(isec==60){
      sec=0.;
      mm=mm+1;
      if(mm==60){
        mm=0;
        dd=dd+1;
      }
    }
  sprintf(dms,"%c%02d:%02d:%.12Lg",sign,dd,mm,sec);
  return 0;
}

/* This function finds the maximum likelihood parameters for the stochastic
 * signal (here power-law power spectral density with a white high-frequency
 * tail)
 *
 * Input
 * nParameters:		The number of stochastic signal parameters
 * 
 * Output:
 * pdParameters:	The maximum likelihood value of the parameters
 * */

/*
void NelderMeadOptimum(int nParameters) {

	printf("\n\n Performing Minimisation over current model parameters \n\n");

	const gsl_multimin_fminimizer_type *pmtMinimiserType = gsl_multimin_fminimizer_nmsimplex;
	gsl_multimin_fminimizer *pmMinimiser = NULL;
	gsl_vector *vStepSize, *vStart;
	gsl_multimin_function MinexFunc;

	double *pdParameterEstimates;
	double dSize;
	int nIteration=0, nStatus;

	printf("\n\nAllocate: %i \n\n", nParameters);

	pdParameterEstimates = new double[nParameters];
	vStepSize = gsl_vector_alloc(nParameters);
	vStart = gsl_vector_alloc(nParameters);

	int pcount=0;
	// Obtain the starting point as rough parameter estimates

	printf("\n\n About to Set Start and StepSize \n\n");
	

 	 for(int p=0;p<1;p++){

                double val=0;
		double err=0.0001;
		if(((MNStruct *)globalcontext)->doLinear == 2){
			val = 0;
			err = 1;
		}
		printf("val err %i %g %g \n", p, val, err);
		gsl_vector_set(vStepSize, pcount, err);	
		gsl_vector_set(vStart, pcount, val);
		pcount++;
         }

 	 for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps;p++){

                double val=0;
		double err=1;
		printf("val err %i %g %g \n", p, val, err);
		gsl_vector_set(vStepSize, pcount, err);	
		gsl_vector_set(vStart, pcount, val);
		pcount++;

         }	

	if(((MNStruct *)globalcontext)->numFitEFAC == 1){
                double val=0;
		double err=0.001;
		printf("EFAC val err %i %g %g \n", pcount, val, err);
		gsl_vector_set(vStepSize, pcount, err);	
		gsl_vector_set(vStart, pcount, val);
		pcount++;
	}
        if(((MNStruct *)globalcontext)->numFitEFAC > 1){
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        double val=0;
			double err=0.01;
			printf("EFAC val err %i %g %g \n", pcount, val, err);
			gsl_vector_set(vStepSize, pcount, err);
			gsl_vector_set(vStart, pcount, val);

			pcount++;

                }
        }
	if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
                double val=-6.47735;
		double err=0.5;
		printf("val err %i %g %g \n", pcount, val, err);
		gsl_vector_set(vStepSize, pcount, err);	
		gsl_vector_set(vStart, pcount, val);
		pcount++;
	}
        if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			printf("Par file val: %i %g \n", o, ((MNStruct *)globalcontext)->pulse->TNEQVal[o]);
                        double val=-6.47735;
			double err=0.5;
			printf("val err %i %g %g \n", pcount, val, err);
			gsl_vector_set(vStepSize, pcount, err);
			gsl_vector_set(vStart, pcount, val);

			pcount++;

                }
        }

	if(((MNStruct *)globalcontext)->incDM > 4){

		int startpos = 0;
		double *SignalCoeff = new double[2*((MNStruct *)globalcontext)->numFitDMCoeff]();
			
		SignalCoeff[startpos+0] =   0.663901;
		SignalCoeff[startpos+1] =   -0.408302;
		SignalCoeff[startpos+2] =   -1.00818;
		SignalCoeff[startpos+3] =   0.0170304;
		SignalCoeff[startpos+4] =   0.00077749;
		SignalCoeff[startpos+5] =   -1.27488;
		SignalCoeff[startpos+6] =   -0.700241;
		SignalCoeff[startpos+7] =   -0.597241;
		SignalCoeff[startpos+8] =   -0.0218956;
		SignalCoeff[startpos+9] =   0.27582;
		SignalCoeff[startpos+10] =  0.0964869;
		SignalCoeff[startpos+11] =  -1.0958;
		SignalCoeff[startpos+12] =  0.892695;
		SignalCoeff[startpos+13] =  0.106757;
		SignalCoeff[startpos+14] =  -0.334975;
		SignalCoeff[startpos+15] =  0.556876;
		SignalCoeff[startpos+16] =  0.763701;
		SignalCoeff[startpos+17] =  2.3606;
		SignalCoeff[startpos+18] =  -0.541014;
		SignalCoeff[startpos+19] =  0.693078;
		SignalCoeff[startpos+20] =  -3.52692;
		SignalCoeff[startpos+21] =  0.318698;
		SignalCoeff[startpos+22] =  -1.02256;
		SignalCoeff[startpos+23] =  -1.01429;
		SignalCoeff[startpos+24] =  0.258241;
		SignalCoeff[startpos+25] =  0.151249;
		SignalCoeff[startpos+26] =  0.0286253;
		SignalCoeff[startpos+27] =  -0.0089317;
		SignalCoeff[startpos+28] =  0.616434;
		SignalCoeff[startpos+29] =  -0.35779;
		

		for(int i = 0; i < 2*((MNStruct *)globalcontext)->numFitDMCoeff; i++){

	                double val=SignalCoeff[i];
			double err=1.0;

			gsl_vector_set(vStepSize, pcount, err);	
			gsl_vector_set(vStart, pcount, val);
			pcount++;
		}
			
		if(((MNStruct *)globalcontext)->incDM==5){

		        double val=-10.7836;
			double err=1.0;
			gsl_vector_set(vStepSize, pcount, err);	
			gsl_vector_set(vStart, pcount, val);
			pcount++;


		        val=2.68;
			err=1.0;
			gsl_vector_set(vStepSize, pcount, err);	
			gsl_vector_set(vStart, pcount, val);

			pcount++;
		}

		delete[] SignalCoeff;
	}


	for(int i =0; i < ((MNStruct *)globalcontext)->totalshapestoccoeff; i++){

                double val=((MNStruct *)globalcontext)->MeanProfileStoc[i];
		double err=0.5;
		printf("val err %i %g %g \n", pcount, val, err);
		gsl_vector_set(vStepSize, pcount, err);	
		gsl_vector_set(vStart, pcount, val);
		pcount++;


	}

	for(int p2 = 0; p2 < ((MNStruct *)globalcontext)->numProfComponents; p2++){
		for(int p3 = 0; p3 < ((MNStruct *)globalcontext)->numProfileFitCoeff[p2]; p3++){

			gsl_vector_set(vStepSize, pcount, 0.01);
			gsl_vector_set(vStart, pcount, 0);


			printf("PFIT pcount %i \n", pcount);
			pcount++;
		}
	}


	if(((MNStruct *)globalcontext)->FitLinearProfileWidth == 1){
		gsl_vector_set(vStepSize, pcount, 0.01);
                gsl_vector_set(vStart, pcount, 0);


		printf("PFIT pcount %i \n", pcount);
		pcount++;
	}

	//Profile Evo Parameters
	for(int p1 = 0; p1 < ((MNStruct *)globalcontext)->NProfileEvoPoly; p1++){
		for(int p2 = 0; p2 < ((MNStruct *)globalcontext)->numProfComponents; p2++){
			for(int p3 = 0; p3 < ((MNStruct *)globalcontext)->numEvoFitCoeff[p2]; p3++){

//				if(p1 == 0){
					gsl_vector_set(vStepSize, pcount, 0.001);
//				}
//				if(p1 == 1){
//					 gsl_vector_set(vStepSize, pcount, 0.1);
//				}
				gsl_vector_set(vStart, pcount, 0);

			//	if(p2 ==0 && p3 == 0){
				//	gsl_vector_set(vStepSize, pcount, 0);
					//gsl_vector_set(vStart, pcount, 0);
				//}		
				printf("PEFIT pcount %i \n", pcount);
			        pcount++;
			}
		}

        }

	if(((MNStruct *)globalcontext)->incPrecession == 1){


		gsl_vector_set(vStepSize, pcount, 0);
                gsl_vector_set(vStart, pcount, 2.678475);
		pcount++;
		
		for(int p2 = 0; p2 < ((MNStruct *)globalcontext)->numProfComponents; p2++){
			for(int p3 = 0; p3 < ((MNStruct *)globalcontext)->numProfileFitCoeff[p2]; p3++){

				gsl_vector_set(vStepSize, pcount, 0.2);
				gsl_vector_set(vStart, pcount, 0);


				printf("PFIT pcount %i \n", pcount);
				pcount++;
			}
		}
	}

	if(((MNStruct *)globalcontext)->incPrecession == 2){


		for(int p2 = 0; p2 < ((MNStruct *)globalcontext)->numProfComponents; p2++){
//			printf("P2 %i %g %g \n", p2, ((MNStruct *)globalcontext)->PriorsArray[pcount],  ((MNStruct *)globalcontext)->PriorsArray[pcount+nParameters]);

			double val = 0.5*(((MNStruct *)globalcontext)->PriorsArray[pcount]+ ((MNStruct *)globalcontext)->PriorsArray[pcount+nParameters]);

			printf("P2 %i %g \n", p2, val);

                        gsl_vector_set(vStepSize, pcount,0.0);
                        gsl_vector_set(vStart, pcount, val);

			pcount++;
		}
		for(int p2 = 0; p2 < ((MNStruct *)globalcontext)->numProfComponents-1; p2++){
//			printf("P2 %i %g %g \n", p2, ((MNStruct *)globalcontext)->PriorsArray[pcount],  ((MNStruct *)globalcontext)->PriorsArray[pcount+nParameters]);

			double val = 0.5*(((MNStruct *)globalcontext)->PriorsArray[pcount]+ ((MNStruct *)globalcontext)->PriorsArray[pcount+nParameters]);

			printf("P2 %i %g \n", p2, val);


			gsl_vector_set(vStart, pcount,val);
			gsl_vector_set(vStepSize, pcount,0.00);

			pcount++;

		}
		for(int p2 = 0; p2 < (((MNStruct *)globalcontext)->numProfComponents-1)*(((MNStruct *)globalcontext)->FitPrecAmps+1); p2++){

			double val = 0.5*(((MNStruct *)globalcontext)->PriorsArray[pcount]+ ((MNStruct *)globalcontext)->PriorsArray[pcount+nParameters]);
			printf("P2 %i %g \n", p2, val);



			gsl_vector_set(vStart, pcount, val);
			gsl_vector_set(vStepSize, pcount, 0.1);

			pcount++;
		}

	}
	printf("\n\n Set Start and StepSize \n\n");

	// Initialise the iteration procedure

	MinexFunc.f = &negloglikelihood;
	MinexFunc.n = nParameters;
	MinexFunc.params = globalcontext;
	pmMinimiser = gsl_multimin_fminimizer_alloc(pmtMinimiserType, nParameters);
	gsl_multimin_fminimizer_set(pmMinimiser, &MinexFunc, vStart, vStepSize);


	printf("\n\n About to Iterate \n\n");


  // Iterate to the maximum likelihood
	do {
		nIteration++;
		nStatus = gsl_multimin_fminimizer_iterate(pmMinimiser);

		if(nStatus) break;

		// Check whether we are close enough to the minimum (1e-3 error)
		dSize = gsl_multimin_fminimizer_size(pmMinimiser);
		nStatus = gsl_multimin_test_size(dSize, 1e-3);

		for(int i=0; i<nParameters; i++) {
			pdParameterEstimates[i] = gsl_vector_get(pmMinimiser->x, i);
			//printf("%i %g \n", i,pdParameters[i]);
		} // for i

		if(nStatus == GSL_SUCCESS) {
			fprintf(stderr, "Converged to maximum likelihood with downhill simplex                               \n");
		} // if nStatus

		// Print iteration values
		if(nIteration % 100 ==0){
			printf("Step[%i]: Convergence: %g Current Minimum: %.10g \n", nIteration,dSize,gsl_multimin_fminimizer_minimum(pmMinimiser));
		}
  } while (nStatus == GSL_CONTINUE && nIteration < 1000);

	pcount=0;
	printf("\n");



	int incRED = 0;
	
	long double Tempo2Fit[((MNStruct *)globalcontext)->numFitTiming+((MNStruct *)globalcontext)->numFitJumps];
	std::string longname="MLVals";

	std::vector<double> paramlist(2*nParameters);

	double **paramarray = new double*[nParameters];
	for(int p =0;p < nParameters; p++){
		paramarray[p]=new double[4]();
	}


	


	int fitcount = 0;
	Tempo2Fit[fitcount] = 0;

	long double val = pdParameterEstimates[pcount]*(((MNStruct *)globalcontext)->LDpriors[0][1]) + (((MNStruct *)globalcontext)->LDpriors[0][0]);
	printf("Phase %g %.20Lg \n", pdParameterEstimates[pcount], val);
	pcount++;
	fitcount++;

	for(int i =1; i< ((MNStruct *)globalcontext)->numFitTiming; i++){
		long double val = pdParameterEstimates[pcount]*(((MNStruct *)globalcontext)->LDpriors[i][1]) + (((MNStruct *)globalcontext)->LDpriors[i][0]);

		if(strcasecmp(((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[i][0]].shortlabel[0],"RAJ")== 0){
			((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[i][0]].val[((MNStruct *)globalcontext)->TempoFitNums[i][1]] = val;
			char hmsstr[100];
		        llongturn_hms(((MNStruct *)globalcontext)->pulse->param[param_raj].val[0]/(2*M_PI), hmsstr);
	        	strcpy(((MNStruct *)globalcontext)->pulse->rajStrPost,hmsstr);

			printf("%s  %g    %s      1\n", ((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[i][0]].shortlabel[0], pdParameterEstimates[pcount],  ((MNStruct *)globalcontext)->pulse->rajStrPost);
		}

		

		else if(strcasecmp(((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[i][0]].shortlabel[0],"DECJ")== 0){
			((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[i][0]].val[((MNStruct *)globalcontext)->TempoFitNums[i][1]] = val;
			char hmsstr[100];
		        llongturn_dms(((MNStruct *)globalcontext)->pulse->param[param_decj].val[0]/(2*M_PI), hmsstr);
	                strcpy(((MNStruct *)globalcontext)->pulse->decjStrPost,hmsstr);

			printf("%s    %g    %s     1 \n", ((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[i][0]].shortlabel[0], pdParameterEstimates[pcount], ((MNStruct *)globalcontext)->pulse->decjStrPost);
		}
		else{
			((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[i][0]].val[((MNStruct *)globalcontext)->TempoFitNums[i][1]] = val;
			printf("%s   %g    %.20Lg       1\n", ((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[i][0]].shortlabel[0], pdParameterEstimates[pcount], val);

		}
		
		Tempo2Fit[fitcount] = ((MNStruct *)globalcontext)->LDpriors[i][0];

		pcount++;
		fitcount++;
	}

	for(int i =0; i<((MNStruct *)globalcontext)->numFitJumps; i++){
		long double val = pdParameterEstimates[pcount]*(((MNStruct *)globalcontext)->LDpriors[fitcount+i][1]) + (((MNStruct *)globalcontext)->LDpriors[fitcount+i][0]);
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[i]] = val;
		((MNStruct *)globalcontext)->pulse->jumpValErr[((MNStruct *)globalcontext)->TempoJumpNums[i]] = 0;

		Tempo2Fit[fitcount] = ((MNStruct *)globalcontext)->LDpriors[fitcount+i][0];
		printf("Jump %i %g %.20Lg \n", i, pdParameterEstimates[pcount], val);
		pcount++;
	}

	if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		printf("EF val %i %g \n", pcount, pdParameterEstimates[pcount]);
		pcount++;
	}

	if(((MNStruct *)globalcontext)->numFitEFAC > 1){
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			printf("EF %i val %i %g \n", o, pcount, pdParameterEstimates[pcount]);
	                pcount++;

		}
	}
	if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		printf("EQ val %i %g \n", pcount, pdParameterEstimates[pcount]);
		pcount++;
	}

	if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			printf("EQ %i val %i %g \n", o, pcount, pdParameterEstimates[pcount]);
	                pcount++;

		}
	}
	if(((MNStruct *)globalcontext)->incDM > 4){


		for(int i = 0; i < 2*((MNStruct *)globalcontext)->numFitDMCoeff; i++){

			printf("DMC %i %g \n", i, pdParameterEstimates[pcount]);
			pcount++;
		}
			
		if(((MNStruct *)globalcontext)->incDM==5){


			printf("DMA %i %g \n", pcount, pdParameterEstimates[pcount]);
			pcount++;


			printf("DMG %i %g \n", pcount, pdParameterEstimates[pcount]);
			pcount++;
		}

	}





	for(int i =0; i < ((MNStruct *)globalcontext)->totalshapestoccoeff; i++){
		printf("Stoc val %i %g \n", pcount, pdParameterEstimates[pcount]);
		((MNStruct *)globalcontext)->MeanProfileStoc[i] = pdParameterEstimates[pcount];
		pcount++;
	}
	std::ofstream profilefile;
	std::string dname = "NewProfInfo.dat";

	profilefile.open(dname.c_str());
	profilefile << ((MNStruct *)globalcontext)->totshapecoeff << " " << ((MNStruct *)globalcontext)->totalEvoCoeff <<"\n";

	int fitpsum=0;
	int nofitpsum=0;
	for(int p2 = 0; p2 < ((MNStruct *)globalcontext)->numProfComponents; p2++){
		int skipone=0;
		if(p2==0 && nofitpsum == 0){
			printf("P0: %i %g %g\n", nofitpsum, ((MNStruct *)globalcontext)->MeanProfileShape[nofitpsum], 0.0);
			profilefile << std::setprecision(10) << ((MNStruct *)globalcontext)->MeanProfileShape[nofitpsum] <<"\n";
			skipone=1;
			nofitpsum++;
		}
		for(int p3 = 0; p3 < ((MNStruct *)globalcontext)->numProfileFitCoeff[p2]; p3++){
			printf("PF: %i %g %g\n", nofitpsum+p3, ((MNStruct *)globalcontext)->MeanProfileShape[nofitpsum+p3]+pdParameterEstimates[pcount], pdParameterEstimates[pcount]);
			profilefile << std::setprecision(10) << ((MNStruct *)globalcontext)->MeanProfileShape[nofitpsum+p3]+pdParameterEstimates[pcount] <<"\n";
			pcount++;
			
		}
		for(int p3 = ((MNStruct *)globalcontext)->numProfileFitCoeff[p2]; p3 < ((MNStruct *)globalcontext)->numshapecoeff[p2]-skipone; p3++){
			printf("PNF: %i %g\n", nofitpsum+p3, ((MNStruct *)globalcontext)->MeanProfileShape[nofitpsum+p3]);
			profilefile << std::setprecision(10) << ((MNStruct *)globalcontext)->MeanProfileShape[nofitpsum+p3] <<"\n";
		}
		fitpsum+=((MNStruct *)globalcontext)->numProfileFitCoeff[p2];
		nofitpsum+=((MNStruct *)globalcontext)->numshapecoeff[p2]-skipone;
	}


	if(((MNStruct *)globalcontext)->FitLinearProfileWidth == 1){

		printf("Width pcount %i %g \n", pcount, pdParameterEstimates[pcount]);
		pcount++;
	}

	for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
		printf("B: %g \n", ((MNStruct *)globalcontext)->MeanProfileBeta[c]);
		profilefile << std::setprecision(10) << ((MNStruct *)globalcontext)->MeanProfileBeta[c] <<"\n";
	}
	
	double **EvoCoeffs=new double*[((MNStruct *)globalcontext)->NProfileEvoPoly]; 
	for(int i = 0; i < ((MNStruct *)globalcontext)->NProfileEvoPoly; i++){EvoCoeffs[i] = new double[((MNStruct *)globalcontext)->totalEvoCoeff];}

	for(int p = 0; p < ((MNStruct *)globalcontext)->NProfileEvoPoly; p++){	
		for(int i =0; i < ((MNStruct *)globalcontext)->totalEvoCoeff; i++){
			EvoCoeffs[p][i]=((MNStruct *)globalcontext)->MeanProfileEvo[p][i];
		}
	}


	for(int p = 0; p < ((MNStruct *)globalcontext)->NProfileEvoPoly; p++){	
		int cpos = 0;
		for(int c = 0; c < ((MNStruct *)globalcontext)->numProfComponents; c++){
			for(int i =0; i < ((MNStruct *)globalcontext)->numEvoFitCoeff[c]; i++){
				EvoCoeffs[p][i+cpos] += pdParameterEstimates[pcount];
				printf("Evo check: %i %i %i %i %g \n",p, c, i, pcount,  pdParameterEstimates[pcount]); 
				pcount++;
			}
			cpos += ((MNStruct *)globalcontext)->numEvoCoeff[c];
		
		}
	}



	if(((MNStruct *)globalcontext)->incPrecession == 2){


		for(int p2 = 0; p2 < ((MNStruct *)globalcontext)->numProfComponents; p2++){
			printf("PrecFreq: %i %i %g \n",pcount,  p2, pdParameterEstimates[pcount]);
			pcount++;
		}
		for(int p2 = 0; p2 < ((MNStruct *)globalcontext)->numProfComponents-1; p2++){
			printf("PrecPhase: %i %i %g \n",pcount,  p2, pdParameterEstimates[pcount]);
			pcount++;

		}
		for(int p2 = 0; p2 < (((MNStruct *)globalcontext)->numProfComponents-1)*(((MNStruct *)globalcontext)->FitPrecAmps+1); p2++){
			printf("PrecAmp: %i %i %g \n",pcount,  p2, pdParameterEstimates[pcount]);
			pcount++;
		}

	}
	for(int p = 0; p < ((MNStruct *)globalcontext)->NProfileEvoPoly; p++){	
		for(int i =0; i < ((MNStruct *)globalcontext)->totalEvoCoeff; i++){
			printf("E: %g \n", EvoCoeffs[p][i]);
			profilefile << std::setprecision(10) << EvoCoeffs[p][i] <<"\n";
		}
	}
	for(int i =0; i < ((MNStruct *)globalcontext)->totalshapestoccoeff; i++){
			printf("S: %g \n", ((MNStruct *)globalcontext)->MeanProfileStoc[i]);
			profilefile << std::setprecision(10) << ((MNStruct *)globalcontext)->MeanProfileStoc[i] <<"\n";	
	}
	profilefile.close();

	for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
		if(((MNStruct *)globalcontext)->pulse->fitJump[k] == 0){

			((MNStruct *)globalcontext)->pulse->jumpVal[k] = ((MNStruct *)globalcontext)->PreJumpVals[k];
		}
	}

	printf("   Max Like %.10g \n", gsl_multimin_fminimizer_minimum(pmMinimiser));
	TNtextOutput(((MNStruct *)globalcontext)->pulse, 1, 0, Tempo2Fit,  globalcontext, 0, nParameters, paramlist, 0.0, 0, 0, 0, longname, paramarray);

  gsl_vector_free(vStart);
  gsl_vector_free(vStepSize);
  gsl_multimin_fminimizer_free(pmMinimiser);
  delete[] pdParameterEstimates;
} // NelderMeadOptimum

*/

/*
double Smallnegloglikelihood(const gsl_vector *pvP, void *context) {

	int ndims=((MNStruct *)globalcontext)->numProfComponents;
	double *pdParameters = new double[ndims];
	double lval;
	double badamp = 0;
	double prior = 0;	
	for(int i=0; i<ndims; i++) {
	//	printf("Param: %i %g %g \n", i, gsl_vector_get(pvP, i), pow(10.0, gsl_vector_get(pvP, i)));
		pdParameters[i] = pow(10.0, gsl_vector_get(pvP, i));
		prior += log(pdParameters[i]);
		if(pdParameters[i] > 10000.0){ badamp = 1; }
  	} // for i


	int ProfNbins = 1024;
	double *PD=new double[ProfNbins];

	vector_dgemv(((MNStruct *)globalcontext)->PrecBasis, pdParameters, PD, ProfNbins, ((MNStruct *)globalcontext)->numProfComponents, 'N');
	double noise = ((MNStruct *)globalcontext)->PrecN*((MNStruct *)globalcontext)->PrecN;


	double *ModelVec = new double[2*ProfNbins](); 
	double *MM = new double[2*2](); 
	double *Md = new double[2](); 
	double *TempMd = new double[2](); 

	for(int i =0; i < ProfNbins; i++){ 
		ModelVec[i + 0*ProfNbins] = 1; 
		ModelVec[i + 1*ProfNbins] = PD[i]; 
	} 

	vector_dgemm(ModelVec, ModelVec, MM, ProfNbins, 2,ProfNbins, 2, 'T', 'N'); 
	vector_dgemv(ModelVec, ((MNStruct *)globalcontext)->PrecD, Md,ProfNbins,2,'T'); 

	for(int i =0; i < 2; i++){ 
		TempMd[i] = Md[i]; 
	} 

	int info=0; 
	double Margindet = 0; 
	vector_dpotrfInfo(MM, 2, Margindet, info); 
	vector_dpotrsInfo(MM, TempMd, 2, info); 


	for(int i = 0; i < ProfNbins; i++){
		double res = ((MNStruct *)globalcontext)->PrecD[i] - TempMd[1]*PD[i] - TempMd[0];
//		printf("res: %i %g %g \n", i, ((MNStruct *)globalcontext)->PrecD[i], PD[i]);
		lval += 0.5*res*res/noise;
	}
	
	delete[] ModelVec;
	delete[] MM;
	delete[] Md;
	delete[] TempMd;

	//printf("like %.10g \n", lval);

	delete[] pdParameters;
	delete[] PD;	

	if(badamp == 1){lval += pow(10.0, 100); }
	//printf("Like: %g \n",lval);
	// The loglikelihood function is virtual, so the correct one is called
	return lval-prior;
} // loglikelihood

*/

/*

void SmallNelderMeadOptimum(int nParameters, double *ML) {


	const gsl_multimin_fminimizer_type *pmtMinimiserType = gsl_multimin_fminimizer_nmsimplex;
	gsl_multimin_fminimizer *pmMinimiser = NULL;
	gsl_vector *vStepSize, *vStart;
	gsl_multimin_function MinexFunc;

	double *pdParameterEstimates;
	double dSize;
	int nIteration=0, nStatus;

//	printf("\n\nAllocate: %i \n\n", nParameters);

	pdParameterEstimates = new double[nParameters];
	vStepSize = gsl_vector_alloc(nParameters);
	vStart = gsl_vector_alloc(nParameters);

	int pcount=0;
	// Obtain the starting point as rough parameter estimates

//	printf("\n\n About to Set Start and StepSize \n\n");
	

	 double val=0;
	 double err=0;
  	 gsl_vector_set(vStepSize, pcount, err);
	 gsl_vector_set(vStart, pcount, val);
	 pcount++;

 	 for(int p=1;p<nParameters;p++){

                double val=0;
		double err=0.3;
		gsl_vector_set(vStepSize, pcount, err);	
		gsl_vector_set(vStart, pcount, val);
		pcount++;
         }

//	printf("\n\n Set Start and StepSize \n\n");

	// Initialise the iteration procedure

	MinexFunc.f = &Smallnegloglikelihood;
	MinexFunc.n = nParameters;
	MinexFunc.params = globalcontext;
	pmMinimiser = gsl_multimin_fminimizer_alloc(pmtMinimiserType, nParameters);
	gsl_multimin_fminimizer_set(pmMinimiser, &MinexFunc, vStart, vStepSize);


//	printf("\n\n About to Iterate \n\n");


  // Iterate to the maximum likelihood
	do {
		nIteration++;
		nStatus = gsl_multimin_fminimizer_iterate(pmMinimiser);

		if(nStatus) break;

		// Check whether we are close enough to the minimum (1e-3 error)
		dSize = gsl_multimin_fminimizer_size(pmMinimiser);
		nStatus = gsl_multimin_test_size(dSize, 1e-3);

		for(int i=0; i<nParameters; i++) {
			pdParameterEstimates[i] = gsl_vector_get(pmMinimiser->x, i);
			//printf("%i %g \n", i,pdParameters[i]);
		} // for i

		if(nStatus == GSL_SUCCESS) {
//			fprintf(stderr, "Converged to maximum likelihood with downhill simplex                               \n");
		} // if nStatus

		// Print iteration values
		if(nIteration % 100 ==0){
		//	printf("Step[%i]: Convergence: %g Current Minimum: %.10g \n", nIteration,dSize,gsl_multimin_fminimizer_minimum(pmMinimiser));
		}
  } while (nStatus == GSL_CONTINUE && nIteration < 1000);

	pcount=0;
//	printf("\n");


	for(int i =0; i< nParameters; i++){
		//printf("ML: %i %g \n", i, pdParameterEstimates[i]);
		ML[i] = pdParameterEstimates[i];
	}

  gsl_vector_free(vStart);
  gsl_vector_free(vStepSize);
  gsl_multimin_fminimizer_free(pmMinimiser);
  delete[] pdParameterEstimates;

//	sleep(5);

} // NelderMeadOptimum
*/
