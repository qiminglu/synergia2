#!/usr/bin/env python

import physics_constants
import math
import Numeric

class Beam_parameters:
    def __init__(self, mass_GeV, charge_e, kinetic_energy_GeV,
                 initial_phase_rad, scaling_frequency_Hz):
        self.mass_GeV = mass_GeV
        self.charge_e = charge_e
        self.kinetic_energy_GeV = kinetic_energy_GeV
        self.initial_phase_rad = initial_phase_rad
        self.scaling_frequency_Hz = scaling_frequency_Hz
        self.default_correlation_coeffs()
        self.sigma_x_m = None
        self.lambda_x_GeVoc = None
        self.sigma_y_m = None
        self.lambda_y_GeVoc = None
        self.sigma_z_m = None
        self.lambda_z_GeVoc = None

    def x_params(self,sigma,lam,
                 r=None,mismatch=1,mismatch_p=1,offset=0,offset_p=0):
	"""The correlation coefficient r can also be set by the xpx argument
	to correlation_coeffs"""
        self.sigma_x_m = sigma
        self.lambda_x_GeVoc = lam
	if r != None: self.xpx = r
        self.mismatch_x = mismatch
        self.mismatch_px = mismatch_p
        self.offset_x_m = offset
        self.offset_px = offset_p
        
    def y_params(self,sigma,lam,
                 r=None,mismatch=1,mismatch_p=1,offset=0,offset_p=0):
	"""The correlation coefficient r can also be set by the ypy argument
	to correlation_coeffs"""
        self.sigma_y_m = sigma
        self.lambda_y_GeVoc = lam
	if r != None: self.ypy = r
        self.mismatch_y = mismatch
        self.mismatch_py = mismatch_p
        self.offset_y_m = offset
        self.offset_py = offset_p
        
    def z_params(self,sigma,lam,
                 r=None,mismatch=1,mismatch_p=1,offset=0,offset_p=0,
                 num_peaks=1):
	"""The correlation coefficient r can also be set by the zpz argument
	to correlation_coeffs"""
        self.sigma_z_m = sigma
        self.lambda_z_GeVoc = lam
	if r !=None : self.zpz = r
        self.mismatch_z = mismatch
        self.mismatch_pz = mismatch_p
        self.offset_z = offset
        self.offset_pz = offset_p
        self.num_zpeaks = num_peaks

    def omega(self):
        return self.scaling_frequency_Hz * 2* math.pi

    def gamma(self):
        return self.kinetic_energy_GeV/self.mass_GeV + 1.0

    def beta(self):
        """returns the velocity of the reference particle in natural units"""
        gamma = self.gamma()
        return math.sqrt(1.0 - 1.0/(gamma*gamma))

    def get_conversions(self):
        c = physics_constants.PH_MKS_c # m/s
        w = self.omega() # rad/s
        beta = self.beta()
        m = self.mass_GeV # GeV
        # Unit conversion: X^impact_i = C_i X^real_i
        Cxy   = w/c
        Cz    = w/(beta*c)
        Cxpyp = 1.0/m
        Czp   = 1.0/m # really a conversion from p_z to p_t, so includes an
                    # extra factor of beta, i.e., 1/(beta*m) * beta        
        return (Cxy, Cxpyp, Cz, Czp)
    
    def default_correlation_coeffs(self):
	"""Sets all correlation coefficients to zero."""
        self.xpx = 0.0
        self.xy = 0.0
        self.pxy = 0.0
        self.xpy = 0.0
        self.pxpy = 0.0
        self.ypy = 0.0
        self.xz = 0.0
        self.pxz = 0.0
        self.yz = 0.0
        self.pyz = 0.0
        self.xpz = 0.0
        self.pxpz = 0.0
        self.ypz = 0.0
        self.pypz = 0.0
        self.zpz = 0.0
	
    def correlation_coeffs(self, xpx=None, xy=None, pxy=None,
                           xpy=None, pxpy=None, ypy=None, xz=None,
                           pxz=None, yz=None, pyz=None, xpz=None,
                           pxpz=None, ypz=None, pypz=None, zpz=None):
        """Correlation coefficients for use the with gaussian covariance
        distribution"""
        if xpx: self.xpx = xpx
        if xy: self.xy = xy
        if pxy: self.pxy = pxy
        if xpy: self.xpy = xpy
        if pxpy: self.pxpy = pxpy
        if ypy: self.ypy = ypy
        if xz: self.xz = xz
        if pxz: self.pxz = pxz
        if yz: self.yz = yz
        if pyz: self.pyz = pyz
        if xpz: self.xpz = xpz
        if pxpz: self.pxpz = pxpz
        if ypz: self.ypz = ypz
        if pypz: self.pypz = pypz
        if zpz: self.zpz = zpz
        
    def distparam(self):
        (Cxy, Cxpyp, Cz, Czp) = self.get_conversions()
        # Unit conversion: X^impact_i = C_i X^real_i
        Cx = Cxy
        Cy = Cxy
        #Cz = Cz
        Cxp = Cxpyp
        Cyp = Cxpyp
        #Czp = Czp
        param = [self.sigma_x_m**2 * Cx**2,
                 (self.sigma_x_m*self.lambda_x_GeVoc*self.xpx)*Cx*Cxp,
                 self.lambda_x_GeVoc**2 * Cxp**2,
                 (self.sigma_x_m*self.sigma_y_m*self.xy)*Cx*Cy,
                 (self.lambda_x_GeVoc*self.sigma_y_m*self.pxy)*Cxp*Cy,
                 self.sigma_y_m**2 * Cy**2,
                 (self.sigma_x_m*self.lambda_y_GeVoc*self.xpy)*Cx*Cyp,
                 (self.lambda_x_GeVoc*self.lambda_y_GeVoc*self.pxpy)*Cxp*Cyp,
                 (self.sigma_y_m*self.lambda_y_GeVoc*self.ypy)*Cy*Cyp,
                 self.lambda_y_GeVoc**2 * Cyp**2,
                 (self.sigma_x_m*self.sigma_z_m*self.xz)*Cx*Cz,
                 (self.lambda_x_GeVoc*self.sigma_z_m*self.pxz)*Cxp*Cz,
                 (self.sigma_y_m*self.sigma_z_m*self.yz)*Cy*Cz,
                 (self.lambda_y_GeVoc*self.sigma_z_m*self.pyz)*Cyp*Cz,
                 self.sigma_z_m**2 * Cz**2,
                 (self.sigma_x_m*self.lambda_z_GeVoc*self.xpz)*Cx*Czp,
                 (self.lambda_x_GeVoc*self.lambda_z_GeVoc*self.pxpz)*Cxp*Czp,
                 (self.sigma_y_m*self.lambda_z_GeVoc*self.ypz)*Cy*Czp,
                 (self.lambda_y_GeVoc*self.lambda_z_GeVoc*self.pypz)*Cyp*Czp,
                 (self.sigma_z_m*self.lambda_z_GeVoc*self.zpz)*Cz*Czp,
                 self.lambda_z_GeVoc**2 * Czp**2,
                 self.offset_x_m * Cx,
                 self.offset_px * Cxp,
                 self.offset_y_m * Cy,
                 self.offset_py * Cyp,
                 self.offset_z * Cz,
                 self.offset_pz * Czp,
                 self.num_zpeaks,
                 666.0,
                 666.0]
        return Numeric.array(param,'d')
                  
