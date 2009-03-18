#include <stdio.h>
#include "macro_bunch_store.h"
#include "basic_toolkit/PhysicsConstants.h"

#include "BasErs_field.h"

using namespace boost::python;

int
apply_BasErs_kick(Macro_bunch_store &mbs, double sigmaX, double sigmaY, double tau)

{

    double gamma = -1 * mbs.ref_particle(5);
    double beta = sqrt(gamma * gamma - 1.0) / gamma;
    const  double c = PH_MKS_c;
    const  double pi = M_PI;
    const double eps0 = PH_MKS_eps0;

    double sigma[2];
    sigma[0] = sigmaX;
    sigma[1] = sigmaY;
    BasErs_field *myfield = new BasErs_field(sigma);

 
    for (int ipart = 0; ipart < mbs.local_num; ipart++) {
        // Get the field at the particle (x,y) location
        std::vector<double> Efield(3);
        Efield = myfield->NormalizedEField(mbs.local_particles(0, ipart), mbs.local_particles(2, ipart));
        //std::cout << " Ex = " << Efield[0] << " Ey = "<< Efield[1] << " Ez = " << Efield[2] << std::endl;

        // Now x and y kick

 	

        for (int n_axis = 0; n_axis < 2; n_axis++) {
           
//**************** old attempt **********************************
/*          double mass = mbs.mass * 1.0e9;          
            double Brho = gamma * beta * mass/ c;

            double perveance0 = mbs.total_current / (2 * pi * eps0 * Brho * gamma * gamma* \
                                beta * c * beta * c);
            double LengthScale = mbs.units(2 * n_axis);
            double factor = pi * perveance0 * gamma * beta * beta / LengthScale * 4.0 * pi;



             //(We think) this is for the Lorentz transformation of the transverse
             //E field.
             factor *= gamma;
             //the above fudge seemed to work for high energy, for low energy, I had to multiply current by
             //pi/betagamma (checked using channel).  Fudge factor recalculated below:

             factor *= pi / beta; */


//**************************************************************


/*       Alex: please check it, although I am almost sure it is right.....
          In the bunch frame,  

                    (Delta p) = q*E* (Delta t) =factor*Efiled*tau
                                             
           where Efiled=normalized field, see BasErs_field.h	    	

            q=p/Brho=PH_CNV_brho_to_p
	    because p unit is [GeV/c], the charge is measured in  q=c*10e-9

	    E= 1/(2*pi*eps0) *lambda*Efield, 
	    the line density of charge,lambda= current/v=current/beta*c
	
	   so far:  */

           double factor = PH_CNV_brho_to_p*mbs.total_current /(2.*pi*eps0* beta*c);
	
//	  the  arc length tau=beta*c* (Delta t), so (Delta t)= tau/(beta*c)

//	  in the bunch frame the factor before Efield is	 	
		
          factor=factor/(beta*c);

//        the Lorentz transformation to the accelerator
//                        frame introduces a factor of 1/gamma^2
//        where:
//        1/gamma is due to the time interval of the kick,
//        since time interval(in the acc frame)=time interval(in the bunch frame)*gamma
//
//        transversal coordinates x and y and momenta
//        px and py are the same in both frames, 
// 
//        and:
//        1/gamma is due to the charge density transformation between the frames
//        rho_charge (in the acc frame)=  rho_charge (in the bunch frame)*gamma
//        remember that the current I=lambda*beta*c is defined in the acc frame 
//   


	  factor = factor/(gamma * gamma);	

          int index = 2 * n_axis + 1; // for n_axis = (0,1,2) Cartesian coordinate x,y,z,
            // in particle store indexing, px,py,pz = (1,3,5)
          double kick = Efield[n_axis] * tau * factor ; // the SC kick with the 2D geometric field contribution
            // update the data structure
            mbs.local_particles(index, ipart) += kick;
           
        }

    }

    delete myfield;

    return 0;
}

BOOST_PYTHON_MODULE(GaussSC)
{
    def("apply_BasErs_kick", &apply_BasErs_kick);
}
