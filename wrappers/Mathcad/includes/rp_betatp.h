LRESULT rp_Btp(
    LPCOMPLEXSCALAR     ret,
    LPCMCSTRING       fluid,
    LPCCOMPLEXSCALAR      t,
    LPCCOMPLEXSCALAR      p   )
{
    double tval, pval, Dval;
    double ttrip, tnbpt, tc, pc, Dc, Zc, acf, dip, Rgas;
    double Dl, Dv, Q, U, H, S, Cv, Cp, W, Pdum, hjt;
    double xkappa, beta = 0.0, xisenk, xkt, betas, bs, xkkt, thrott, pi, spht;
    double xl[20], xv[20];
    int ierr = 0, icomp = 1, kph = 1, kguess = 0;
    char herr[255];

    ierr = cSetup(fluid->str);
    if (ierr > 0)
        return MAKELRESULT(ierr,1);

    if( t->imag != 0.0 )
        return MAKELRESULT(MUST_BE_REAL,2);
    else
        tval = t->real;

    if (tval > Tmax*(1 + 0.5*extr)) return MAKELRESULT(T_OUT_OF_RANGE, 2);

    if( p->imag != 0.0 )
        return MAKELRESULT(MUST_BE_REAL,3);
    else
        pval = p->real * 1000.0;   // Convert from MPa to kPa for REFPROP inputs

    if (pval > Pmax*(1 + extr)) return MAKELRESULT(P_OUT_OF_RANGE, 3);

    // Get critical pressure
    if (ncomp > 1)
    {
        CRITPdll(&x[0], &tc, &pc, &Dc, &ierr, herr, errormessagelength);
        if (ierr > 0)
        {
            return MAKELRESULT(UNCONVERGED, 2);
        }
    }
    else
    {
        INFOdll(&icomp, &wmm, &ttrip, &tnbpt, &tc, &pc, &Dc, &Zc, &acf, &dip, &Rgas);
    }

    // If above critical pressure (Liquid) use TPRHO instead of TPFLSH
    if (tval > tc) kph = 2;
    if (pval > pc) kph = 1;
    if ((pval > pc) || (tval > tc))
    {
        // Get single-phase density
        TPRHOdll(&tval, &pval, &x[0], &kph, &kguess, &Dval, &ierr, herr, errormessagelength);
        // Have density, now call THERM to get Enthalpy
        if (ierr <=0) THERMdll(&tval, &Dval, &x[0], &Pdum, &U, &H, &S, &Cv, &Cp, &W, &hjt);
    }
    else
    {
        TPFLSHdll(&tval, &pval, &x[0], &Dval,   // [K], [kPa], [mol/L]
            &Dl, &Dv, &xl[0], &xv[0],           // Saturation terms
            &Q, &U, &H, &S, &Cv, &Cp, &W,       // Thermo properties
            &ierr, herr, errormessagelength);   // error code and string
    }

    if (ierr > 0) {
        if ((ierr == 1) || (ierr == 5) || (ierr == 9) || (ierr == 13))
            return MAKELRESULT(T_OUT_OF_RANGE, 2);  // Temperature out of bounds
        else if ((ierr == 4) || (ierr == 12))
            return MAKELRESULT(P_OUT_OF_RANGE, 1);  // Pressure out of bounds
        else if (ierr == 8)
            return MAKELRESULT(X_SUM_NONUNITY, 1);  // component and/or sum < 0 or > 1
        else
            return MAKELRESULT(UNCONVERGED, 2);     // one of many convergence errors
    }
    
    THERM3dll(&tval, &Dval, &x[0], &xkappa, 
              &beta,                                // Volume expansion coefficient (= 1/rho dD/dT) [1/K]
              &xisenk, &xkt, &betas, &bs, &xkkt, 
              &thrott, &pi, &spht);

    ret->real = beta;   // Return thermal expansion coefficient [1/K]

    return 0;               // return 0 to indicate there was no error
            
}

FUNCTIONINFO    rp_betatp = 
{ 
    (char *)("rp_betatp"),              // Name by which Mathcad will recognize the function
    (char *)("fluid,t,p"),              // rp_betatp will be called as rp_betatp(fluid,t,p)
    (char *)("Returns the volume expansion coefficient [1/K] given the temperature [K] and pressure [MPa]"),
                                        // description of rp_betatp(fluid,t,p)
    (LPCFUNCTION)rp_Btp,                // pointer to the executable code
    COMPLEX_SCALAR,                     // the return type is a complex scalar
    3,                                  // the function takes on 3 arguments
    { MC_STRING,                        // String argument
      COMPLEX_SCALAR,
      COMPLEX_SCALAR }                  // arguments are complex scalars
};
    
    
    