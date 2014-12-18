

#include "CentralValuesAndUncertainties.h"
#include <set>
#include <string>
#include <iostream>
#include <sstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "TRandom3.h"

//using namespace std;

namespace NeutrinoFluxReweight{ 
  
  CentralValuesAndUncertainties* CentralValuesAndUncertainties::instance = 0;

  CentralValuesAndUncertainties::CentralValuesAndUncertainties(){
    r3=new TRandom3(0);
  }
  
  void CentralValuesAndUncertainties::readFromXML(const char* filename){
    using boost::property_tree::ptree;
    ptree top;
    
    read_xml(filename,top,2); // option 2 removes comment strings
    
    ptree& uncorrelated = top.get_child("pars.uncorrelated");
    ptree::iterator it = uncorrelated.begin();
    for(; it!=uncorrelated.end(); it++){
      // it->first is the name
      // it->second is the child property tree
      double cv=it->second.get<double>("cv");
      double err=it->second.get<double>("err");
      Parameter p(it->first,cv);
      CentralValuesAndUncertainties::addUncorrelated(p,err);
    }

    ptree& uncorrelated_list = top.get_child("pars.uncorrelated_list");
    it = uncorrelated_list.begin();
    for(; it!=uncorrelated_list.end(); it++){
      // it->first is the name
      // it->second is the child property tree

      std::string cvs_string=it->second.get<std::string>("cvs");
      std::string errs_string=it->second.get<std::string>("errs");
      //  std::cout<<"LIST "<<cvs_string<<" "<<errs_string<<std::endl;
      double cv=0;
      int ii=0;
      std::string name;
      std::stringstream ss(cvs_string);
      std::vector<Parameter> tmp_par;
      while(ss >> cv){
	std::stringstream sID;
	sID << ii;
	std::string nameID = sID.str();
	name = it->first + "_" + nameID;
	Parameter p(name,cv);
	tmp_par.push_back(p);
	ii++;
      };
      
      double err=0;
      std::stringstream sserr(errs_string);
      ii=0;
      while(sserr >> err){
	CentralValuesAndUncertainties::addUncorrelated(tmp_par[ii],err);
	ii++;
      }     
      
    }
    
    ptree& correlated = top.get_child("pars.correlated");
    it = correlated.begin();
    for(; it!=correlated.end(); it++){
      // it->first is the name
      // it->second is the child property tree
      std::string cvs_string=it->second.get<std::string>("cvs");
      std::string covmx_string=it->second.get<std::string>("covmx");

      //filling the Parameter table:
      std::stringstream ss(cvs_string);
      double cv=0;
      int ii=0;
      std::string name;
      ParameterTable ptable;
      while(ss >> cv){
	std::stringstream sID;
	sID << ii;
	std::string nameID = sID.str();
	name = it->first + "_" +nameID;
	Parameter p(name,cv);
	ptable.setParameter(p);
	ii++;
      };
      TMatrixD mcov(ii,ii);
      //filling the matrix:
      std::stringstream ssmx(covmx_string);
      double err;
      int idx = 0;
      while(ssmx >> err){
	mcov(idx/ii,idx%ii) = err;
	idx++;
      }
      CentralValuesAndUncertainties::addCorrelated(ptable,mcov);
    }
    
  }
  
  void CentralValuesAndUncertainties::addUncorrelated(Parameter& cv_par, double uncertainty){
    uncorrelated_pars.setParameter(cv_par);
    uncorrelated_errors[cv_par.first] = uncertainty;
  }
  
  //  void CentralValuesAndUncertainties::addCorrelated(ParameterTable& cv_pars, MatrixClass& cov_mx){
  void CentralValuesAndUncertainties::addCorrelated(ParameterTable& cv_pars, TMatrixD& cov_mx){
    correlated_par_tables.push_back(cv_pars);
    covariance_matrices.push_back(cov_mx);
  } 
  
  void CentralValuesAndUncertainties::setBaseSeed(int val){
    baseSeed = val;
  }
  ParameterTable CentralValuesAndUncertainties::calculateParsForUniverse(int universe){
    
    //If universe = 0, then it is the central value:
    double cvfactor = 1.0;
    if(universe==0)cvfactor = 0.0;
    int univ_seed = baseSeed + universe;
    r3->SetSeed(univ_seed);    
    
    ParameterTable ptable;
    
    std::map<std::string, double> table_uncorr_pars = uncorrelated_pars.table;
    std::map<std::string, double>::iterator it = table_uncorr_pars.begin();
    for(;it!=table_uncorr_pars.end();it++){
      double sigma = r3->Gaus(0.0,1.0);
      double new_val = it->second  + cvfactor*sigma*uncorrelated_errors[it->first];
      Parameter p(it->first,new_val); 
      ptable.setParameter(p);      
    }
    
    TDecompChol *decomp;
    
    for(int ii=0;ii<covariance_matrices.size();ii++){
      
      decomp=new TDecompChol(covariance_matrices[ii],0.0);
      
      bool isPosDef=decomp->Decompose();
      TMatrixD thisMx = decomp->GetU();
      delete decomp;
      int nmat = thisMx.GetNcols();
      TVectorD vsigma(nmat);
      for(int jj=0;jj<nmat;jj++){
	vsigma[jj]=cvfactor*(r3->Gaus(0.0,1.0));
      }   
      TVectorD vecDShift = thisMx*vsigma;
      
      std::map<std::string, double> tb = (correlated_par_tables[ii]).table;
      std::map<std::string, double>::iterator it_tb = tb.begin();

      for(;it_tb != tb.end();it_tb++){
	std::string tmp_name = it_tb->first;
	std::string snID = tmp_name.substr((it_tb->first).rfind("_")+1,(it_tb->first).length());
	std::stringstream ssID(snID);
	int nID;
	ssID >> nID;
	double new_val = it_tb->second + vecDShift[nID];	
	Parameter p(it_tb->first,new_val); 
	if(isPosDef)ptable.setParameter(p);
      }
      
    }
    
    return ptable; 
    
  }  
  
  ParameterTable CentralValuesAndUncertainties::getCVPars(){      

    ParameterTable ptableCV;
    
    std::map<std::string, double> table_pars;
    std::map<std::string, double>::iterator it;

    //Uncorrelated:
    table_pars = uncorrelated_pars.table;
    it = table_pars.begin();
    for(;it!=table_pars.end();it++){
      Parameter p(it->first,it->second); 
      ptableCV.setParameter(p);      
    }
    
    //Correlated:
    for(int ii=0;ii<covariance_matrices.size();ii++){
      table_pars = (correlated_par_tables[ii]).table;
      it = table_pars.begin();
      for(;it != table_pars.end();it++){
	Parameter p(it->first,it->second); 
	ptableCV.setParameter(p);
      }
      
    }
    
    return ptableCV;
    
  }
  
  CentralValuesAndUncertainties* CentralValuesAndUncertainties::getInstance(){
    if (instance == 0) instance = new CentralValuesAndUncertainties;
    return instance;
  }

}
