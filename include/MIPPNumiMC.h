#ifndef MIPPNUMIMC_H
#define MIPPNUMIMC_H

#include <utility>
#include <vector>
#include <string>
#include <iostream>
#include <iostream>
#include <sstream>

namespace NeutrinoFluxReweight{
  
  /*! \class MIPPNumiMC
   *  \brief A class to manage the MC value for MIPP NuMI. 
   */
  class MIPPNumiMC{
  public:
    
    MIPPNumiMC();
    static MIPPNumiMC* getInstance();
    
    //! Read a xml file name to get the mc value for pip
    void pip_mc_from_xml(const char* filename);

    //! Read a xml file name to get the mc value for pim
    void pim_mc_from_xml(const char* filename);

   //! Read a xml file name to get the mc value for kap
    void kap_mc_from_xml(const char* filename);

   //! Read a xml file name to get the mc value for kam
    void kam_mc_from_xml(const char* filename);

    double getMCval(double pz,double pt, int pdgcode);

  private:    
    std::vector<double> pip_cv,pim_cv,kap_cv,kam_cv;
    std::vector<double> v_pzmin,v_pzmax,v_ptmin,v_ptmax;
    
    static MIPPNumiMC* instance;
    
  };

  
  
};
#endif
