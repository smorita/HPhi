/* HPhi  -  Quantum Lattice Model Simulator */
/* Copyright (C) 2015 Takahiro Misawa, Kazuyoshi Yoshimi, Mitsuaki Kawamura, Youhei Yamaji, Synge Todo, Naoki Kawashima */

/* This program is free software: you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or */
/* (at your option) any later version. */

/* This program is distributed in the hope that it will be useful, */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/* GNU General Public License for more details. */

/* You should have received a copy of the GNU General Public License */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include "mltply.h"
#include "expec_cisajs.h"
#include "wrapperMPI.h"
#include "mltplyMPI.h"

/**
 * @file   expec_cisajs.c
 * 
 * @brief  File for calculation of one body green's function
 *
 * @version 0.1, 0.2
 *
 * @author Takahiro Misawa (The University of Tokyo)
 * @author Kazuyoshi Yoshimi (The University of Tokyo)
 * 
 */


/** 
 * @brief function of calculation for one body green's function
 * 
 * @param X  list for getting information to calculate one body green's function.
 * @param vec eigenvectors.
 * 
 * @version 0.2
 * @details add calculation one body green's functions for general spin
 *
 * @version 0.1
 * @author Takahiro Misawa (The University of Tokyo)
 * @author Kazuyoshi Yoshimi (The University of Tokyo)
 * @retval 0
 * @retval -1
 */
int expec_cisajs(struct BindStruct *X,double complex *vec){

  FILE *fp;
  char sdt[D_FileNameMax];

  long unsigned int i,j;
  long unsigned int iexchg;
  long unsigned int irght,ilft,ihfbit;
  long unsigned int isite1,isite2;
  long unsigned int org_isite1,org_isite2,org_sigma1,org_sigma2;
  long unsigned int tmp_org_isite1, tmp_org_isite2;
  long unsigned int Asum,Adiff;
  long unsigned int tmp_off=0;
  double complex dam_pr;
  long int i_max;
  int tmp_sgn, num1;
  long int ibit1, ibit;
  long unsigned int is1_up, is1_down, is;
  double complex tmp_OneGreen=1.0;
  //For TPQ
  int step=0;
  int rand_i=0;

  i_max = X->Check.idim_max;      
  if(GetSplitBitByModel(X->Def.Nsite, X->Def.iCalcModel, &irght, &ilft, &ihfbit)!=0){
    return -1;
  }
  X->Large.i_max    = i_max;
  X->Large.irght    = irght;
  X->Large.ilft     = ilft;
  X->Large.ihfbit   = ihfbit;
  X->Large.mode     = M_CORR;
 
  dam_pr=0.0;
  switch(X->Def.iCalcType){
  case Lanczos:
    if(X->Def.St==0){
      sprintf(sdt, cFileName1BGreen_Lanczos, X->Def.CDataFileHead);
      fprintf(stdoutMPI, "%s", cLogLanczosExpecOneBodyGStart);
    }else if(X->Def.St==1){
      sprintf(sdt, cFileName1BGreen_CG, X->Def.CDataFileHead);
      fprintf(stdoutMPI, "%s", cLogCGExpecOneBodyGStart);
    }
    //vec=v0;
    break;
  case TPQCalc:
    step=X->Def.istep;
    rand_i=X->Def.irand;
    TimeKeeperWithRandAndStep(X, cFileNameTimeKeep,  cTPQExpecOneBodyGStart, "a", rand_i, step);
    sprintf(sdt, cFileName1BGreen_TPQ, X->Def.CDataFileHead, rand_i, step);
    //vec=v0;
    break;
  case FullDiag:
    sprintf(sdt, cFileName1BGreen_FullDiag, X->Def.CDataFileHead, X->Phys.eigen_num);
    //vec=v0;
    break;
  }
  
  if(!childfopenMPI(sdt, "w", &fp)==0){
    return -1;
  } 
  switch(X->Def.iCalcModel){
  case HubbardGC:    
    for(i=0;i<X->Def.NCisAjt;i++){
      org_isite1 = X->Def.CisAjt[i][0]+1;
      org_isite2 = X->Def.CisAjt[i][2]+1;
      org_sigma1 = X->Def.CisAjt[i][1];
      org_sigma2 = X->Def.CisAjt[i][3];
      dam_pr=0;
      if (org_isite1  > X->Def.Nsite &&
          org_isite2  > X->Def.Nsite) {
	if(org_isite1==org_isite2 && org_sigma1==org_sigma2){
	  if(org_sigma1==0){
	    is   = X->Def.Tpow[2 * org_isite1 - 2];
	  }
	  else{
	    is = X->Def.Tpow[2 * org_isite1 - 1];
	  }
	  ibit = (unsigned long int)myrank & is;
	  if (ibit == is) {
#pragma omp parallel for default(none) reduction(+:dam_pr) shared(vec) \
  firstprivate(i_max) private(j) 
	    for (j = 1; j <= i_max; j++) dam_pr += vec[j]*conj(vec[j]);
	  }
	}
	else{
	  dam_pr =X_GC_child_general_hopp_MPIdouble(org_isite1-1, org_sigma1, org_isite2-1, org_sigma2, -tmp_OneGreen, X, vec, vec);
	}
      }
      else if (org_isite2  > X->Def.Nsite || org_isite1  > X->Def.Nsite){
	if(org_isite1<org_isite2){
	  dam_pr =X_GC_child_general_hopp_MPIsingle(org_isite1-1, org_sigma1, org_isite2-1, org_sigma2, -tmp_OneGreen, X, vec, vec);
	}
	else{
	  dam_pr =X_GC_child_general_hopp_MPIsingle(org_isite2-1, org_sigma2, org_isite1-1, org_sigma1, -tmp_OneGreen, X, vec, vec);
	  dam_pr = conj(dam_pr);
	}
      }
      else{
	if(child_general_hopp_GetInfo( X,org_isite1,org_isite2,org_sigma1,org_sigma2)!=0){
	  return -1;
	}
	dam_pr = GC_child_general_hopp(vec,vec,X,tmp_OneGreen);
      }
     
      dam_pr= SumMPI_dc(dam_pr);
      fprintf(fp," %4ld %4ld %4ld %4ld %.10lf %.10lf\n",org_isite1-1,org_sigma1,org_isite2-1,org_sigma2,creal(dam_pr),cimag(dam_pr));
    }
    break;
    
  case KondoGC:
  case Hubbard:
  case Kondo:
    for(i=0;i<X->Def.NCisAjt;i++){
      org_isite1 = X->Def.CisAjt[i][0]+1;
      org_isite2 = X->Def.CisAjt[i][2]+1;
      org_sigma1 = X->Def.CisAjt[i][1];
      org_sigma2 = X->Def.CisAjt[i][3];
      dam_pr=0.0;
      if (org_isite1  > X->Def.Nsite &&
          org_isite2  > X->Def.Nsite) {
	if(org_isite1==org_isite2 && org_sigma1==org_sigma2){//diagonal
	  if(org_sigma1==0){
	    is   = X->Def.Tpow[2 * org_isite1 - 2];
	  }
	  else{
	    is = X->Def.Tpow[2 * org_isite1 - 1];
	  }
	  ibit = (unsigned long int)myrank & is;
	  if (ibit == is) {
#pragma omp parallel for default(none) reduction(+:dam_pr) shared(vec)	\
  firstprivate(i_max) private(j) 
	    for (j = 1; j <= i_max; j++) dam_pr += vec[j]*conj(vec[j]);
	  }
	}
	else{
	  dam_pr =X_child_general_hopp_MPIdouble(org_isite1-1, org_sigma1, org_isite2-1, org_sigma2, -tmp_OneGreen, X, vec, vec);
	}
      }
        else if (org_isite2  > X->Def.Nsite || org_isite1  > X->Def.Nsite){
	  if(org_isite1 < org_isite2){
	    dam_pr =X_child_general_hopp_MPIsingle(org_isite1-1, org_sigma1,org_isite2-1, org_sigma2, -tmp_OneGreen, X, vec, vec);
	  }
	  else{
	    dam_pr = X_child_general_hopp_MPIsingle(org_isite2-1, org_sigma2, org_isite1-1, org_sigma1, -tmp_OneGreen, X, vec, vec);
	    dam_pr = conj(dam_pr);
	  }
	}
	else{     
	  if(child_general_hopp_GetInfo( X,org_isite1,org_isite2,org_sigma1,org_sigma2)!=0){
	    return -1;
	  }
	  if(org_isite1==org_isite2 && org_sigma1==org_sigma2){
	    if(org_sigma1==0){
	      is   = X->Def.Tpow[2 * org_isite1 - 2];
	    }
	    else{
	      is = X->Def.Tpow[2 * org_isite1 - 1];
	    }	    
#pragma omp parallel for default(none) shared(list_1, vec) reduction(+:dam_pr) firstprivate(i_max, is) private(num1, ibit)
	    for(j = 1;j <= i_max;j++){	      
	      ibit = list_1[j]&is;
	      num1  = ibit/is;	      
	      dam_pr += num1*conj(vec[j])*vec[j];
	    }

	  }
	  else{
	    dam_pr = child_general_hopp(vec,vec,X,tmp_OneGreen);
	  }
	}
      dam_pr= SumMPI_dc(dam_pr);
      fprintf(fp," %4ld %4ld %4ld %4ld %.10lf %.10lf\n",org_isite1-1,org_sigma1,org_isite2-1,org_sigma2,creal(dam_pr),cimag(dam_pr));
    }
    break;

  case Spin: // for the Sz-conserved spin system

    if(X->Def.iFlgGeneralSpin==FALSE){
    for(i=0;i<X->Def.NCisAjt;i++){
      org_isite1 = X->Def.CisAjt[i][0]+1;
      org_isite2 = X->Def.CisAjt[i][2]+1;
      org_sigma1 = X->Def.CisAjt[i][1];
      org_sigma2 = X->Def.CisAjt[i][3];
      
      if(org_sigma1 == org_sigma2){
	if(org_isite1==org_isite2){
	  if(org_isite1 > X->Def.Nsite){
	    is1_up = X->Def.Tpow[org_isite1 - 1];
	    ibit1 = ((unsigned long int)myrank& is1_up)^(1-org_sigma1);
	    dam_pr=0;
	    if(ibit1 !=0){
#pragma omp parallel for reduction(+:dam_pr)default(none) shared(vec)	\
  firstprivate(i_max, ibit1) private(j)
	    for (j = 1; j <= i_max; j++) dam_pr += ibit1*conj(vec[j])*vec[j];
	    }
	  }// org_isite1 > X->Def.Nsite 
	  else{
	    isite1     = X->Def.Tpow[org_isite1-1];
	    dam_pr=0.0;
#pragma omp parallel for default(none) reduction(+:dam_pr) private(j) firstprivate(i_max, isite1, org_sigma1, X) shared(vec)
	    for(j=1;j<=i_max;j++){
	      dam_pr+=X_Spin_CisAis(j,X, isite1,org_sigma1)*conj(vec[j])*vec[j]; 
	    }
	  }
	  dam_pr = SumMPI_dc(dam_pr);
	  fprintf(fp," %4ld %4ld %4ld %4ld %.10lf %.10lf\n",org_isite1-1, org_sigma1, org_isite2-1, org_sigma2, creal(dam_pr), cimag(dam_pr));
	}
	else{	  
	  fprintf(fp," %4ld %4ld %4ld %4ld %.10lf %.10lf\n",org_isite1-1, org_sigma1, org_isite2-1, org_sigma2,0.0,0.0);
	}	
      }else{
	// for the canonical case 
	fprintf(fp," %4ld %4ld %4ld %4ld %.10lf %.10lf\n",org_isite1-1, org_sigma1, org_isite2-1, org_sigma2,0.0,0.0);
      }
    }
    }//FlgGeneralSpin=FALSE
    else{
      for(i=0;i<X->Def.NCisAjt;i++){
	org_isite1 = X->Def.CisAjt[i][0]+1;
	org_isite2 = X->Def.CisAjt[i][2]+1;
	org_sigma1 = X->Def.CisAjt[i][1];
	org_sigma2 = X->Def.CisAjt[i][3];
	if(org_isite1 == org_isite2){ 
	  if(org_sigma1==org_sigma2){  
	    // longitudinal magnetic field
	    dam_pr=0.0;
#pragma omp parallel for default(none) reduction(+:dam_pr) private(j, num1) firstprivate(i_max, org_isite1, org_sigma1, X) shared(vec, list_1)
	    for(j=1;j<=i_max;j++){
	      num1 = BitCheckGeneral(list_1[j], org_isite1, org_sigma1, X->Def.SiteToBit, X->Def.Tpow);
	      dam_pr+=conj(vec[j])*vec[j]*num1; 
	    } 
	    fprintf(fp," %4ld %4ld %4ld %4ld %.10lf %.10lf\n",org_isite1-1, org_sigma1, org_isite2-1, org_sigma2,creal(dam_pr),cimag(dam_pr));
	  }else{
	    fprintf(fp," %4ld %4ld %4ld %4ld %.10lf %.10lf\n",org_isite1-1, org_sigma1, org_isite2-1, org_sigma2,0.0,0.0);
	  }
	}else{
	  // hopping is not allowed in localized spin system
	  fprintf(fp," %4ld %4ld %4ld %4ld %.10lf %.10lf\n",org_isite1-1, org_sigma1, org_isite2-1, org_sigma2, 0.0, 0.0);
	}
      }
    }

    break;
  case SpinGC: //for the Sz-nonconserved spin system

    if(X->Def.iFlgGeneralSpin==FALSE){
      for(i=0;i<X->Def.NCisAjt;i++){
	org_isite1 = X->Def.CisAjt[i][0]+1;
	org_isite2 = X->Def.CisAjt[i][2]+1;
	org_sigma1 = X->Def.CisAjt[i][1];
	org_sigma2 = X->Def.CisAjt[i][3];
	dam_pr=0.0;
	if(org_isite1 == org_isite2){ 
	  isite1 = X->Def.Tpow[org_isite1-1];
	  
	  if(org_isite1 > X->Def.Nsite){
	    if(org_sigma1==org_sigma2){  
	      ibit1 = ((unsigned long int)myrank& isite1)^(1-org_sigma1);
	      if(ibit1!=0){
#pragma omp parallel for reduction(+:dam_pr)default(none) shared(vec)	\
  firstprivate(i_max, ibit1) private(j)
		for (j = 1; j <= i_max; j++) dam_pr += conj(vec[j])*vec[j];
	      }
	    }
	    else{
	      ibit1 = ((unsigned long int)myrank& isite1);
	      if(ibit1 !=0){
		dam_pr=0.0;
#pragma omp parallel for default(none) reduction(+:dam_pr) private(j) firstprivate(i_max) shared(vec)
		for(j=1;j<=i_max;j++){
		  dam_pr  +=  conj(vec[j])*vec[j]; 
		}
	      }
	    }
	  }
	  else{	    
	    if(org_sigma1==org_sigma2){  
	      // longitudinal magnetic field
	      dam_pr=0.0;
#pragma omp parallel for default(none) reduction(+:dam_pr) private(j) firstprivate(i_max, isite1, org_sigma1, X) shared(vec)
	      for(j=1;j<=i_max;j++){
		dam_pr+=X_SpinGC_CisAis(j,X,isite1,org_sigma1)*conj(vec[j])*vec[j]; 
	      } 
	    }else{
	      // transverse magnetic field
	      dam_pr=0.0;
#pragma omp parallel for default(none) reduction(+:dam_pr) private(j, tmp_sgn) firstprivate(i_max, isite1, org_sigma2, X,tmp_off) shared(vec)
	      for(j=1;j<=i_max;j++){
		tmp_sgn  =  X_SpinGC_CisAit(j,X, isite1,org_sigma2,&tmp_off);   
		dam_pr  +=  tmp_sgn*conj(vec[tmp_off+1])*vec[j]; 
	      }
	    }
	  }
	  dam_pr = SumMPI_dc(dam_pr);
	  fprintf(fp," %4ld %4ld %4ld %4ld %.10lf %.10lf\n",org_isite1-1, org_sigma1, org_isite2-1, org_sigma2,creal(dam_pr),cimag(dam_pr));
	
	}else{
	  // hopping is not allowed in localized spin system
	  fprintf(fp," %4ld %4ld %4ld %4ld %.10lf %.10lf\n",org_isite1-1, org_sigma1, org_isite2-1, org_sigma2, 0.0, 0.0);
	}
      }
    }//FlgGeneralSpin=FALSE
    else{
      for(i=0;i<X->Def.NCisAjt;i++){
	org_isite1 = X->Def.CisAjt[i][0]+1;
	org_isite2 = X->Def.CisAjt[i][2]+1;
	org_sigma1 = X->Def.CisAjt[i][1];
	org_sigma2 = X->Def.CisAjt[i][3];
	if(org_isite1 == org_isite2){ 
	  if(org_sigma1==org_sigma2){  
	    // longitudinal magnetic field
	    dam_pr=0.0;
#pragma omp parallel for default(none) reduction(+:dam_pr) private(j, num1) firstprivate(i_max, org_isite1, org_sigma1, X) shared(vec)
	    for(j=1;j<=i_max;j++){
	      num1 = BitCheckGeneral(j-1, org_isite1, org_sigma1, X->Def.SiteToBit, X->Def.Tpow);
	      dam_pr+=conj(vec[j])*vec[j]*num1; 
	    } 
	    fprintf(fp," %4ld %4ld %4ld %4ld %.10lf %.10lf\n",org_isite1-1, org_sigma1, org_isite2-1, org_sigma2,creal(dam_pr),cimag(dam_pr));
	  }else{
	    // transverse magnetic field
	    dam_pr=0.0;
#pragma omp parallel for default(none) reduction(+:dam_pr) private(j, num1) firstprivate(i_max, org_isite1, org_sigma1, org_sigma2, X,tmp_off) shared(vec)
	    for(j=1;j<=i_max;j++){
	      num1 = GetOffCompGeneralSpin(j-1, org_isite1, org_sigma2, org_sigma1, &tmp_off, X->Def.SiteToBit, X->Def.Tpow);
	      if(num1 !=0){
		dam_pr  +=  conj(vec[tmp_off+1])*vec[j]*num1;
	      }
	    } 
	    fprintf(fp," %4ld %4ld %4ld %4ld %.10lf %.10lf\n",org_isite1-1, org_sigma1, org_isite2-1, org_sigma2,creal(dam_pr),cimag(dam_pr));
	  }
	}else{
	  // hopping is not allowed in localized spin system
	  fprintf(fp," %4ld %4ld %4ld %4ld %.10lf %.10lf\n",org_isite1-1, org_sigma1, org_isite2-1, org_sigma2, 0.0, 0.0);
	}
      }
    }
    break;
    
  default:
    return -1;
  }

  fclose(fp);
  if(X->Def.St==0){
    if(X->Def.iCalcType==Lanczos){
      TimeKeeper(X, cFileNameTimeKeep, cLanczosExpecOneBodyGFinish, "a");
      fprintf(stdoutMPI, "%s", cLogLanczosExpecOneBodyGEnd);
    }
    else if(X->Def.iCalcType==TPQCalc){
      TimeKeeperWithRandAndStep(X, cFileNameTimeKeep, cTPQExpecOneBodyGFinish, "a", rand_i, step);     
    }
  }else if(X->Def.St==1){
    TimeKeeper(X, cFileNameTimeKeep, cExpecOneBodyGFinish, "a");
    fprintf(stdoutMPI, "%s", cLogLanczosExpecOneBodyGEnd);
  }
  return 0;
}
