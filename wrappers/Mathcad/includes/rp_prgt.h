LRESULT rp_PRgt(
    LPCOMPLEXSCALAR     ret,
    LPCMCSTRING       fluid,
    LPCCOMPLEXSCALAR      t   )
{
    char herr[255];
    int kph = 2;       //  kph = 1 (saturated liquid conc.); kph = 2 (saturated vapor conc.)
    int ierr;
    double psat, tsat, rhol, rhov, xliq[20], xvap[20];
    double Cv, Cp, mu, cond;

    ierr = cSetup(fluid->str);
    if (ierr > 0)
        return MAKELRESULT(ierr,1);
    
    if( t->imag != 0.0 )
        return MAKELRESULT(MUST_BE_REAL,2);
    else
        tsat = t->real;

    SATTdll(&tsat, &x[0], &kph, &psat, &rhol, &rhov, &xliq[0], &xvap[0], &ierr, herr, errormessagelength);

    if (ierr > 0)
    {
        if ((ierr == 1) || (ierr == 9) || (ierr == 121) || (ierr == 125))
            return MAKELRESULT(T_OUT_OF_RANGE,2); // Temperature too low | negative | > Tcrit
        else
            return MAKELRESULT(UNCONVERGED,2); // failed to converge
    }

    CVCPdll(&tsat, &rhov, &x[0], &Cv, &Cp);

    TRNPRPdll(&tsat, &rhov, &x[0], &mu, &cond, &ierr, herr, errormessagelength);

    // check for errors and handle by returning MAKELRESULT(n,p)
    // Error codes for TRNPRPdll changed radically in REFPROP 10
    if (vMajor < 10)  // Can only be REFPROP 9.1.1 or 10+ if we are here
    {
        // REFPROP 9.1.1 Error Flags
        if (ierr > 0)
        {
            if ((ierr == 40) || (ierr == 49) || (ierr == 50))
                return MAKELRESULT(NO_TRANSPORT, 1);     // viscosity model not defined
            else if (ierr == 51)
                return MAKELRESULT(INFINITE_K, 3);
            else
                return MAKELRESULT(UNCONVERGED, 2);
        }
        else if (ierr < 0) {
            if ((ierr == -41) || (ierr == -43) || (ierr == -51) || (ierr == -53))
                return MAKELRESULT(T_OUT_OF_RANGE, 2);  // Temperature out of bounds
            else if ((ierr == -42) || (ierr == -52))
                return MAKELRESULT(D_OUT_OF_RANGE, 3);  // Pressure (density) out of bounds
            else if (ierr <= -58)
                return MAKELRESULT(UNCONVERGED, 2);     // did not converge
        }
    }
    else // Assume REFPROP 10 or greater
    {
        // REFPROP 10 error flags here
        if (ierr > 0)
        {
            if ((ierr == 502) || (ierr == 540) || (ierr == 541) || (ierr == 542) || (ierr == 543))
                return MAKELRESULT(NO_TRANSPORT, 1);     // viscosity model not defined
            else if ((ierr == 73) || (ierr == 74))
                return MAKELRESULT(T_OUT_OF_RANGE, 2);  // Temperature out of bounds
            else if (ierr == 561)
                return MAKELRESULT(UNCONVERGED, 2);     // Erroneous value returned for ETA or TCX
            else
                return MAKELRESULT(T_OUT_OF_RANGE, 2);  // One or more inputs to ETA or TCX out of bounds
        }
        else if (ierr < 0) return MAKELRESULT(UNCONVERGED, 2);  // Either ETA or TCX did not converge
    }
    if ((mu < 0) || (cond < 0))
        return MAKELRESULT(UNCONVERGED, 2);

    // � returned in [�Pa-s] = [mg / m-s]
    // k returned in [W/m-K] = [J/m-s-K]
    // Cp returned in [J/mol-K]
    // wmm returned in [g/mol]
    ret->real = mu * Cp / wmm / cond / 1000;   // Conversion: (�Pa-s) * (J/mol-K) / (g/mol) / (W/m*K) / (mg/g) = 
                                               //          (mg/m-s) * (J/mol-K) * (mol/g) * (m-s-K/J) * (g/mg) =
                                               //          [dimensionless]

    return 0;               // return 0 to indicate there was no error
            
}

FUNCTIONINFO    rp_prgt = 
{ 
    (char *)("rp_prgt"),                // Name by which mathcad will recognize the function
    (char *)("fluid,t"),                // rp_prgt will be called as rp_prgt(fluid,t)
    (char *)("Returns the saturated vapor Prandtl Number [-] given the temperature [K]"),
                                        // description of rp_prgt(fluid,t)
    (LPCFUNCTION)rp_PRgt,               // pointer to the executable code
    COMPLEX_SCALAR,                     // the return type is a complex scalar
    2,                                  // the function takes on 1 argument
    { MC_STRING,                        // argument is a MC_STRING
      COMPLEX_SCALAR }                  // argument is a complex scalar
};
