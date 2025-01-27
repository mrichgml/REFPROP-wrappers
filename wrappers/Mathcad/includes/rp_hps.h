LRESULT rp_Hps(
    LPCOMPLEXSCALAR     ret,
    LPCMCSTRING       fluid,
    LPCCOMPLEXSCALAR      p,
    LPCCOMPLEXSCALAR      s   )
{
    char herr[255];
    double pval,hval,sval,dval,tval;
    double tc, pc, Dc, sc;
    double tsat,rhol,rhov,xliq[20],xvap[20],sliq,svap,hliq,hvap;
    int ierr;
    int kph = 1;

    ierr = cSetup(fluid->str);
    if (ierr > 0)
        return MAKELRESULT(ierr,1);

    if( p->imag != 0.0 )
        return MAKELRESULT(MUST_BE_REAL,2);
    else
        pval = p->real * 1000.0;  // Convert from MPa to kPa

    if (pval > Pmax*(1 + extr)) return MAKELRESULT(P_OUT_OF_RANGE, 2);

    // If pval > Pmax, return error 4
    if (pval > Pmax + extr * Pmax)
        return MAKELRESULT(P_OUT_OF_RANGE, 2);
    
    if( s->imag != 0.0 )
        return MAKELRESULT(MUST_BE_REAL,3);
    else
        sval = s->real * wmm;  // Convert from kJ/kg to J/mol

    // Are we above the critical pressure?
    CRITPdll(&x[0], &tc, &pc, &Dc, &ierr, herr, errormessagelength);
    if (ierr > 0)
        return MAKELRESULT(UNCONVERGED,1);

    if (pval < pc) // *** Below the Critical Point ***
    {
        // Below the critical pressure
        SATPdll(&pval, x, &kph, &tsat, &rhol, &rhov, xliq, xvap, &ierr, herr, errormessagelength);
        if (ierr > 0)
        {
            if ((ierr == 2)||(ierr == 4)||(ierr == 141)) 
                return MAKELRESULT(P_OUT_OF_RANGE,2); // Pressure too low | negative | > Pcrit
            else
                return MAKELRESULT(UNCONVERGED,1); // failed to converge
        }
        ENTROdll(&tsat, &rhol, &x[0], &sliq);                           // Get Saturated Liquid Entropy at T
        ENTROdll(&tsat, &rhov, &x[0], &svap);                           // Get Saturated Vapor Entropy at T
        if ((sval >= sliq) && (sval <= svap))                           // IF within saturation curve
        {                                    
            ENTHALdll(&tsat, &rhol, &x[0], &hliq);                      //     Get Saturated Liquid Enthalpy at T
            ENTHALdll(&tsat, &rhov, &x[0], &hvap);                      //     Get Saturated Vapor Enthalpy at T
            hval = hliq + (hvap - hliq)*(sval - sliq) / (svap - sliq);  //     Interpolate hval
            ret->real = hval / wmm;                                     //     convert to kJ/kg-
            return 0;                                                   //     RETURN w/ no errors
        }
        else if (sval < sliq)                                           // IF below liquid entropy
        {                                                               // initialize...
            kph = 1;                                                    //    liquid phase
            tval = tsat;                                                //    saturation temp
            dval = rhol;                                                //    saturation density
        }
        else                                                            // ELSE above vapor entropy
        {                                                               // initialize...
            kph = 2;                                                    //    vapor phase
            tval = tsat;                                                //    saturation temp
            dval = rhov;                                                //    saturation density
        }
        PSFL1dll(&pval, &sval, &x[0], &kph, &tval, &dval, &ierr, herr, errormessagelength);
        if (ierr > 0)
            return MAKELRESULT(UNCONVERGED,1);
        else
        {
            ENTHALdll(&tval,&dval, &x[0],&hval);                        // Get Enthalpy at T
            ret->real = hval / wmm;                                     // convert to kJ/kg-K
            return 0;
        }
    }                                    // *** above critical pressure ***
    kph = 2;                                                            // use vapor iterations
    tval = tc + 10.0;                                                   //   initialize t just above tc
    dval = Dc * 2.0;                                                    //   initialize rho to 2 X Dc
    ENTROdll(&tc, &Dc, &x[0], &sc);                                     //   check critical entropy
    if (sval < sc/2.0) tval = tc * 0.8;                                 //   if very low entropy, reduce t guess

    PSFL1dll(&pval, &sval, &x[0], &kph, &tval, &dval, &ierr, herr, errormessagelength);
    if (ierr > 0) {
        if (ierr <= 12)
            return MAKELRESULT(P_OUT_OF_RANGE, 2);
        else if (ierr == 249)
            return MAKELRESULT(H_OUT_OF_RANGE, 1);
        else if (ierr == 250)
            return MAKELRESULT(S_OUT_OF_RANGE, 3);
        else
            return MAKELRESULT(UNCONVERGED,2);
    }

    ENTHALdll(&tval, &dval, &x[0], &hval);                                // Get Enthalpy at T

    ret->real = hval / wmm;                                               // convert to kJ/kg-K

    return 0;               // return 0 to indicate there was no error
            
}

FUNCTIONINFO    rp_hps = 
{ 
    (char *)("rp_hps"),                 // Name by which mathcad will recognize the function
    (char *)("fluid,p,s"),              // rp_hps will be called as rp_hps(fluid,p,s)
    (char *)("Returns the enthalpy [kJ/kg] given the pressure [MPa] and entropy [kJ/kg-K]"),
                                        // description of rp_hps(fluid,p,s)
    (LPCFUNCTION)rp_Hps,                // pointer to the executable code
    COMPLEX_SCALAR,                     // the return type is a complex scalar
    3,                                  // the function takes on 2 arguments
    { MC_STRING,                        // String argument
      COMPLEX_SCALAR,
      COMPLEX_SCALAR }                  // arguments are complex scalars
};
    
    
    