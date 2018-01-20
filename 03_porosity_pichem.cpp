#include "stdifm.h"
#include "porosity_2.h"
#include <algorithm>   /*VISUAL STUDIO 2013 requirement to use command std::min*/
//#include <vld.h>       /*Visual Leak Detector for Visual C++ 2008-2015 http://vld.codeplex.com/ */

#define _CRTDBG_MAP_ALLOC									/*memory leak START*/
#include <crtdbg.h>
#ifdef _DEBUG
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif
#include <stdlib.h>											/*memory leak END*/

using namespace::std; /*for the compiler 'string' == std::string "vector" == std::vector, comes from PHREEQC4FEFLOW*/

IfmModule g_pMod;  /* Global handle related to this plugin */

#pragma region IFM_Definitions
/* --- IFMREG_BEGIN --- */
/*  -- Do not edit! --  */

static IfmResult OnBeginDocument (IfmDocument);
static void OnEndDocument (IfmDocument);
static void Serialize (IfmDocument, IfmArchive);
static void OnEditDocument (IfmDocument, Widget);
static void PostTimeStep (IfmDocument);

/*
 * Enter a short description between the quotation marks in the following lines:
 */
static const char szDesc[] = 
  "Please, insert a plug-in description here!";

#ifdef __cplusplus
extern "C"
#endif /* __cplusplus */

IfmResult RegisterModule(IfmModule pMod)
{
  if (IfmGetFeflowVersion (pMod) < IFM_REQUIRED_VERSION)
    return False;
  g_pMod = pMod;
  IfmRegisterModule (pMod, "SIMULATION", "POROSITY_2", "porosity_2", 0x1000);
  IfmSetDescriptionString (pMod, szDesc);
  IfmSetCopyrightPath (pMod, "porosity_2.txt");
  IfmSetHtmlPage (pMod, "porosity_2.htm");
  IfmSetPrimarySource (pMod, "porosity_2.cpp");
  IfmRegisterProc (pMod, "OnBeginDocument", 1, (IfmProc)OnBeginDocument);
  IfmRegisterProc (pMod, "OnEndDocument", 1, (IfmProc)OnEndDocument);
  IfmRegisterProc (pMod, "Serialize", 1, (IfmProc)Serialize);
  IfmRegisterProc (pMod, "OnEditDocument", 1, (IfmProc)OnEditDocument);
  IfmRegisterProc (pMod, "PostTimeStep", 1, (IfmProc)PostTimeStep);
  return True;
}

static void Serialize (IfmDocument pDoc, IfmArchive pArc)
{
  CPorosity2::FromHandle(pDoc)->Serialize (pDoc, pArc);
}
static void OnEditDocument (IfmDocument pDoc, Widget wParent)
{
  CPorosity2::FromHandle(pDoc)->OnEditDocument (pDoc, wParent);
}
static void PostTimeStep (IfmDocument pDoc)
{
  CPorosity2::FromHandle(pDoc)->PostTimeStep (pDoc);
}

/* --- IFMREG_END --- */
#pragma endregion


static IfmResult OnBeginDocument (IfmDocument pDoc)
{
  if (IfmDocumentVersion (pDoc) < IFM_CURRENT_DOCUMENT_VERSION)
    return false;

  try {
    IfmDocumentSetUserData(pDoc, new CPorosity2(pDoc));
  }
  catch (...) {
    return false;
  }

  return true;
}

static void OnEndDocument (IfmDocument pDoc)
{
  delete CPorosity2::FromHandle(pDoc);
}

///////////////////////////////////////////////////////////////////////////
// Implementation of CPorosity2

// Constructor
CPorosity2::CPorosity2 (IfmDocument pDoc)
  : m_pDoc(pDoc)
{
  /*
   * TODO: Add your own code here ...
   */
}

// Destructor
CPorosity2::~CPorosity2 ()
{
  /*
   * TODO: Add your own code here ...
   */
}

// Obtaining class instance from document handle
CPorosity2* CPorosity2::FromHandle (IfmDocument pDoc)
{
  return reinterpret_cast<CPorosity2*>(IfmDocumentGetUserData(pDoc));
}

// Callbacks

/* Global variables to store data from OnEditDocument IFM Callback and use them on Presimulation IFM Callback */
double g1_stab;									// Days until mixing zone stabilization 
char*  g2_dissolved;							// Point distribution name for dissolved mineral data 

double g3_deltat_double;						// Short-term mass transport and reaction simulation runs
char*  g3_deltat_string;

double g4_sim_deltat = 0;						// Storage here the simulation delta_time to selectively decide if plug-in runs or lets flow run for more time.
char*  g5_Kxx_exp;								// Elemental distribution name for phi/Kxx relationship exponent in 2D 
char*  g5_Kyy_exp;								// Elemental distribution name for phi/Kyy relationship exponent in 2D 
int	   g6_message;								// Controls the number of messages in FeFlow GUI while running 
double g7_mg_mol;								// 1 for mol/L, calculate manually for mg/L input
char*  g8_Vm;									// Elemental distribution name for the Molar Volume in L/mol
char*  g9_init_por;								// Elemental distribution name for contains initial porosity to estimate permeability evolution
char*  g10_init_Kxx;							// Elemental distribution name for contains initial Kxx to estimate permeability evolution
char*  g10_init_Kyy;							// Elemental distribution name for contains initial Kyy to estimate permeability evolution

double g11_eff_timestep_double;					// Effective time step for porosity and permeability evolution estimates
char*  g11_eff_timestep_string;

double g12_accum_phiK;							// Total accumulated time of effective time step

double g13_PC_dt_double;						// Phreeqc reaction timestep (days)
char*  g13_PC_dt_string;


char*  g14_Kyy_0;								// Elemental distribution name for Initial hydraulic conductivity Kyy in 3D problems
char*  g15_Kzz_0;								// Elemental distribution name for Initial hydraulic conductivity Kzz in 3D problems
char*  g16_Kyy_exp;								// Elemental distribution name for the phi/Kyy relationship exponent in 3D problems
char*  g17_Kzz_exp;								// Elemental distribution name for the phi/Kzz relationship exponent in 3D problems
int    g18_ddb;									// Controls the chemistry input: 0 only dissolution, 1 only deposition, 2 both 

void CPorosity2::Serialize(IfmDocument pDoc, IfmArchive pArc)
{

	switch (IfmioGetMode(pArc)) {				// decide what's going on: INIT, STORE, LOAD, FREE? 
	case IfmIO_STORE:							// activated when click saving
		IfmioDouble(pArc, &g1_stab);						 
		IfmioString(pArc, &g2_dissolved,15);				
		IfmioString(pArc, &g3_deltat_string, 15);
		IfmioString(pArc, &g5_Kxx_exp, 20);
		IfmioString(pArc, &g5_Kyy_exp, 20);
		IfmioInt   (pArc, &g6_message);
		IfmioDouble(pArc, &g7_mg_mol);
		IfmioString(pArc, &g8_Vm, 20);
		IfmioString(pArc, &g9_init_por, 20);
		IfmioString(pArc, &g10_init_Kxx, 20);
		IfmioString(pArc, &g10_init_Kyy, 20);
		IfmioString(pArc, &g11_eff_timestep_string, 15);
		IfmioString(pArc, &g13_PC_dt_string, 15);
		IfmioString(pArc, &g14_Kyy_0, 20);
		IfmioString(pArc, &g15_Kzz_0, 20);
		IfmioString(pArc, &g16_Kyy_exp, 20);
		IfmioString(pArc, &g17_Kzz_exp, 20);
		IfmioInt   (pArc, &g18_ddb);
		break;	
	case IfmIO_LOAD:							// activated when file is open
		if (IfmioGetVersion(pArc) >= 0x1000) {
			// Serialize.
			IfmioDouble(pArc,&g1_stab);
			IfmioString(pArc,&g2_dissolved,15);
			IfmioString(pArc, &g3_deltat_string, 15);
			IfmioString(pArc, &g5_Kxx_exp, 20);
			IfmioString(pArc, &g5_Kyy_exp, 20);
			IfmioInt(pArc, &g6_message);
			IfmioDouble(pArc, &g7_mg_mol);
			IfmioString(pArc, &g8_Vm, 20);
			IfmioString(pArc, &g9_init_por, 20);
			IfmioString(pArc, &g10_init_Kxx, 20);
			IfmioString(pArc, &g10_init_Kyy, 20);
			IfmioString(pArc, &g11_eff_timestep_string, 15);
			IfmioString(pArc, &g13_PC_dt_string, 15);
			IfmioString(pArc, &g14_Kyy_0, 20);
			IfmioString(pArc, &g15_Kzz_0, 20);
			IfmioString(pArc, &g16_Kyy_exp, 20);
			IfmioString(pArc, &g17_Kzz_exp, 20);
			IfmioInt(pArc, &g18_ddb);
											}
		break;
	case IfmIO_INIT:							// initialize variables
		break;
	case IfmIO_FREE:							// clean memory when file is closed 
		break;
	}
}
void CPorosity2::OnEditDocument (IfmDocument pDoc, Widget wParent)
{
	const int ndim = IfmGetNumberOfDimensions(pDoc);	// number of dimensions 

	if (ndim == 2) {							// START of 2D plug-in properties window

		double	input1 = g1_stab;					// User input variable 1 initialization: DOUBLE TYPE
		char*	input2 = g2_dissolved;				// User input variable 2 initialization: STRING TYPE
		char*	input3 = g3_deltat_string;			// User input variable 3 initialization: DOUBLE TYPE	
		char*	input4 = g5_Kxx_exp;				// User input variable 5 initialization: STRING TYPE
		char*	input5 = g5_Kyy_exp;				// User input variable 5 initialization: STRING TYPE
		int		input6 = g6_message;				// User input variable 6 initialization: INT TPYE
		double	input7 = g7_mg_mol;					// User input variable 7 initialization: DOUBLE TYPE
		char*	input8 = g8_Vm;						// User input variable 8 initialization: STRING TYPE
		char*	input9 = g9_init_por;				// User input variable 9 initialization: STRING TYPE
		char*	input10 = g10_init_Kxx;				// User input variable 10 initialization: STRING TYPE
		char*	input11 = g10_init_Kyy;				// User input variable 11 initialization: STRING TYPE
		char*	input12 = g11_eff_timestep_string;	// User input variable 12 initialization: DOUBLE TYPE
		char*	input13 = g13_PC_dt_string;			// User input variable 13 initialization: DOUBLE TYPE
		int		input14 = g18_ddb;

		IfmProperty props[] = {
			{ "1 Stabilization time", IfmPROP_DOUBLE, &input1, &g1_stab, "INT, days until mixing zone is stabilized. Until then porosity starts to change" },
			{ "2 Reaction product concentration", IfmPROP_STRING, &input2, &g2_dissolved, "STRING, nodal distribution name for dissolved mineral concentration mol/L" },
			{ "3 Short-term mass transport and reaction simulation", IfmPROP_STRING, &input3, NULL, "DOUBLE, days until reactive transport stabilizes to estimate reaction rates for extrapolation" },
			{ "4 phi/Kxx relationship exponent", IfmPROP_STRING, &input4, NULL, "STRING, elemental distribution name for phi/Kxx relationship exponent" },
			{ "5 phi/Kyy relationship exponent", IfmPROP_STRING, &input5, NULL, "STRING, elemental distribution name for phi/Kyy relationship exponent" },
			{ "6 Message level", IfmPROP_INT, &input6, NULL, "INT, 0 all messages, 1 only Phi & K update messages, 2 no messages in log window" },
			{ "7 Molar Volume adjustment", IfmPROP_DOUBLE, &input7, NULL, "DOUBLE, use 1 for input in mol/L, calculate input when concentration input is in mg/L units" },
			{ "8 Molar Volume", IfmPROP_STRING, &input8, NULL, "STRING, elemental distribution name for Molar Volume of mineral in L/mol" },
			{ "9 Initial porosity elemental distribution", IfmPROP_STRING, &input9, NULL, "STRING, elemental distribution name for initial porosity" },
			{ "10 Initial Kxx elemental distribution", IfmPROP_STRING, &input10, NULL, "STRING, elemental distribution name for initial Kxx" },
			{ "11 Initial Kyy elemental distribution", IfmPROP_STRING, &input11, NULL, "STRING, elemental distribution name for initial Kyy" },
			{ "12 Effective phi/K timestep", IfmPROP_STRING, &input12, NULL, "DOUBLE, effective time step for phi/L evolution estimates (days)" },
			{ "13 PHREEQC timestep ", IfmPROP_STRING, &input13, NULL, "DOUBLE, PHREEQ-C timestep (days)" },
			{ "14 Dissolution, precipitation, both?", IfmPROP_INT, &input14, NULL, "INT, 0 only dissolution, 1 only precipitation, 2 both" }
							};
		IfmEditProperties(pDoc, "phi & K evolution plug-in (2D)", "My parameters: ", props, 14); // Last number is amount of input values.

		g1_stab = input1;						// Taking the values from variables in OnEditDocument callback to 
		g2_dissolved = input2;					// use them in another callback, i.e. PostTimeStep
		g3_deltat_string = input3;				// used in PostTimeStep
		g5_Kxx_exp = input4;					// used in section 4 exponent of porosity/permeability relationship
		g5_Kyy_exp = input5;					// used in section 4 exponent of porosity/permeability relationship
		g6_message = input6;					// used in section 0.1 for 0 = all messages 1 = only Phi & K update messages, 2 = no messages
		g7_mg_mol = input7;						// used when estimating porosity change in section 3.5
		g8_Vm = input8;							// converting input8 molar volume from string to float and use it to estimate porosity change in section 3.5
		g9_init_por = input9;					// used in section 4 for hydraulic conductivity evolution
		g10_init_Kxx = input10;					// used in section 4 for hydraulic conductivity evolution
		g10_init_Kyy = input11;					// used in section 4 for hydraulic conductivity evolution
		g11_eff_timestep_string = input12;				// used when estimating porosity change in section 3.5
		g13_PC_dt_string = input13;					// used in section 3.4 for aved_mine
		g18_ddb = input14;						// used in section 3.3

		long conc_ID = IfmGetRefDistrIdByName(pDoc, g2_dissolved);				// looks for dissolved mineral concentration distribution ID for name 'g2_dissolved' (string). Returs -1 if there's no distribution with given name
		long Kxx_exp_ID = IfmGetElementalRefDistrIdByName(pDoc, g5_Kxx_exp);	// looks for phi/k elemental distribution ID with string name 
		long Vm_ID = IfmGetElementalRefDistrIdByName(pDoc, g8_Vm);				// looks for Vm elemental distribution ID with string name 
		long init_por_ID = IfmGetElementalRefDistrIdByName(pDoc, g9_init_por);	// looks for initial porosity elemental distribution ID with string name 
		long init_Kxx_ID = IfmGetElementalRefDistrIdByName(pDoc, g10_init_Kxx);		// looks for initial hydraulic cond elemental distribution ID with string name 

		if (conc_ID == -1) IfmInfo(pDoc, "Reaction output elemental distribution name does not exist. Create/look for it in USER DATA and retype name");
		else{
			if (Kxx_exp_ID == -1) IfmInfo(pDoc, "phi/K relationship exponent elemental distribution name does not exist. Creat/look for it in USER DATA and retype name");
			else {
				if (Vm_ID == -1) IfmInfo(pDoc, "Molar volume elemental distribution name does not exist. Creat/look for it in USER DATA and retype name");
				else {
					if (init_por_ID == -1) IfmInfo(pDoc, "Initial porosity elemental distribution name does not exist. Creat/look for it in USER DATA and retype name");
					else {
						if (init_Kxx_ID == -1) IfmInfo(pDoc, "Initial hydraulic conductivity elemental distribution name does not exist. Look for it in USER DATA and retype");
						else IfmInfo(pDoc, "Reaction ND:'%i:%s', phi/K update: %s days, phi/K ED '%i:%s',Vm ED:'%i:%s',Initial phi ED: '%i:%s',init_K ED: '%i:%s", conc_ID, g2_dissolved, g3_deltat_string, Kxx_exp_ID, g5_Kxx_exp, Vm_ID, g8_Vm, init_por_ID, g9_init_por, init_Kxx_ID, g10_init_Kxx);
						IfmInfo(pDoc, "Effective porosity and permeability evolution timestep: %s days", g11_eff_timestep_string);
					}
				}
			}
		}
	}										// END of 2D plug-in properties window
	//*******************************************************************************************************************************************************
	else {										// START of 3D plug-in properties window

		double input1 = g1_stab;				// User input variable 1 initialization: DOUBLE TYPE
		char*  input2 = g2_dissolved;			// User input variable 2 initialization: STRING TYPE
		char* input3 = g3_deltat_string;		// User input variable 3 initialization: DOUBLE TYPE	
		char* input5 = g5_Kxx_exp;				// User input variable 5 initialization: STRING TYPE
		int input6 = g6_message;				// User input variable 6 initialization: INT TPYE
		double input7 = g7_mg_mol;				// User input variable 7 initialization: DOUBLE TYPE
		char* input8 = g8_Vm;					// User input variable 8 initialization: STRING TYPE
		char* input9 = g9_init_por;				// User input variable 9 initialization: STRING TYPE
		char* input10 = g10_init_Kxx;			// User input variable 10 initialization: STRING TYPE
		char* input11 = g11_eff_timestep_string;// User input variable 11 initialization: DOUBLE TYPE
		char* input13 = g13_PC_dt_string;		// User input variable 13 initialization: DOUBLE TYPE
		char* input14 = g14_Kyy_0;				// User input variable 14 initialization: STRING TYPE
		char* input15 = g15_Kzz_0;			    // User input variable 15 initialization: STRING TYPE
		char* input16 = g16_Kyy_exp;			// User input variable 15 initialization: STRING TYPE
		char* input17 = g17_Kzz_exp;			// User input variable 15 initialization: STRING TYPE
		int	  input18 = g18_ddb;

		IfmProperty props[] = {
			{ "1 Stabilization time", IfmPROP_DOUBLE, &input1, NULL, "INT, days until mixing zone is stabilized. Until then porosity starts to change" },
			{ "2 Reaction product concentration", IfmPROP_STRING, &input2, NULL, "STRING, nodal distribution name for dissolved mineral concentration mol/L" },
			{ "3 Short-term mass transport and reaction simulation", IfmPROP_STRING, &input3, NULL, "DOUBLE, days until reactive transport stabilizes to estimate reaction rates for extrapolation" },
			{ "4 phi/Kxx relationship exponent", IfmPROP_STRING, &input5, NULL, "STRING, elemental distribution name for phi/Kxx relationship exponent" },
			{ "5 Message level", IfmPROP_INT, &input6, NULL, "INT, 0 all messages, 1 only Phi & K update messages, 2 no messages in log window" },
			{ "6 Molar Volume adjustment", IfmPROP_DOUBLE, &input7, NULL, "DOUBLE, use 1 for input in mol/L, calculate input when concentration input is in mg/L units" },
			{ "7 Molar Volume", IfmPROP_STRING, &input8, NULL, "STRING, elemental distribution name for Molar Volume of mineral in L/mol" },
			{ "8 Initial porosity elemental distribution", IfmPROP_STRING, &input9, NULL, "STRING, elemental distribution name for initial porosity" },
			{ "9 Initial Kxx elemental distribution", IfmPROP_STRING, &input10, NULL, "STRING, elemental distribution name for initial Kxx hydraulic conductitivity" },
			{ "10 Effective phi/K timestep", IfmPROP_STRING, &input11, NULL, "DOUBLE, effective time step for phi/L evolution estimates (days)" },
			{ "11 PHREEQC timestep ", IfmPROP_STRING, &input13, NULL, "DOUBLE, PHREEQ-C timestep (days)" },
			{ "12 Initial Kyy elemental distribution", IfmPROP_STRING, &input14, NULL, "STRING, elemental distribution name for initial Kyy hydraulic conductitivity" },
			{ "13 Initial Kzz elemental distribution", IfmPROP_STRING, &input15, NULL, "STRING, elemental distribution name for initial Kyy hydraulic conductitivity" },
			{ "14 phi/Kyy relationship exponent", IfmPROP_STRING, &input16, NULL, "STRING, elemental distribution name for phi/Kyy relationship exponent" },
			{ "15 phi/Kzz relationship exponent", IfmPROP_STRING, &input17, NULL, "STRING, elemental distribution name for phi/Kzz relationship exponent" },
			{ "16 Dissolution, precipitation, both?", IfmPROP_INT, &input18, NULL, "INT, 0 only dissolution, 1 only precipitation, 2 both" }
		};
		IfmEditProperties(pDoc, "phi & K evolution plug-in (3D)", "My parameters: ", props, 16); // Last number is amount of input values.

		g1_stab = input1;						// Taking the values from variables in OnEditDocument callback to 
		g2_dissolved = input2;					// use them in another callback, i.e. PostTimeStep
		g3_deltat_string = input3;				// used in PostTimeStep
		g5_Kxx_exp = input5;					// used in section 4 exponent of porosity/permeability relationship
		g6_message = input6;					// used in section 0.1 for 0 = all messages 1 = only Phi & K update messages, 2 = no messages
		g7_mg_mol = input7;						// used when estimating porosity change in section 3.5
		g8_Vm = input8;							// converting input8 molar volume from string to float and use it to estimate porosity change in section 3.5
		g9_init_por = input9;					// used in section 4 for hydraulic conductivity evolution
		g10_init_Kxx = input10;					// used in section 4 for hydraulic conductivity evolution
		g11_eff_timestep_string = input11;		// used when estimating porosity change in section 3.5
		g13_PC_dt_string = input13;				// used in section 3.4 for aved_mine
		g14_Kyy_0 = input14;					// used in section 4 for 3D hydraulic conductivity evolution
		g15_Kzz_0 = input15;					// used in section 4 for 3D hydraulic conductivity evolution
		g16_Kyy_exp = input16;					// used in section 4 for 3D hydraulic conductivity evolution
		g17_Kzz_exp = input17;					// used in section 4 for 3D hydraulic conductivity evolution
		g18_ddb = input18;

		long conc_ID = IfmGetRefDistrIdByName(pDoc, g2_dissolved);				// looks for dissolved mineral concentration distribution ID for name 'g2_dissolved' (string). Returs -1 if there's no distribution with given name
		long Kxx_exp_ID = IfmGetElementalRefDistrIdByName(pDoc, g5_Kxx_exp);		// looks for phi/k elemental distribution ID with string name 
		long Vm_ID = IfmGetElementalRefDistrIdByName(pDoc, g8_Vm);				// looks for Vm elemental distribution ID with string name 
		long init_por_ID = IfmGetElementalRefDistrIdByName(pDoc, g9_init_por);	// looks for initial porosity elemental distribution ID with string name 
		long init_Kxx_ID = IfmGetElementalRefDistrIdByName(pDoc, g10_init_Kxx);		// looks for initial hydraulic cond elemental distribution ID with string name
		long init_Kyy_ID_3D = IfmGetElementalRefDistrIdByName(pDoc, g14_Kyy_0);
		long init_Kzz_ID = IfmGetElementalRefDistrIdByName(pDoc, g15_Kzz_0);
		long Kyy_exp_ID_3D = IfmGetElementalRefDistrIdByName(pDoc, g16_Kyy_exp);
		long Kzz_exp_ID = IfmGetElementalRefDistrIdByName(pDoc, g17_Kzz_exp);

		if (conc_ID == -1) IfmInfo(pDoc, "Reaction output elemental distribution name does not exist. Create/look for it in USER DATA and retype name");
		else{
			if (Kxx_exp_ID == -1) IfmInfo(pDoc, "phi/K relationship exponent elemental distribution name does not exist. Creat/look for it in USER DATA and retype name");
			else {
				if (Vm_ID == -1) IfmInfo(pDoc, "Molar volume elemental distribution name does not exist. Creat/look for it in USER DATA and retype name");
				else {
					if (init_por_ID == -1) IfmInfo(pDoc, "Initial porosity elemental distribution name does not exist. Creat/look for it in USER DATA and retype name");
					else {
						if (init_Kxx_ID == -1) IfmInfo(pDoc, "Initial hydraulic conductivity elemental distribution name does not exist. Look for it in USER DATA and retype");
						else IfmInfo(pDoc, "Reaction ND:'%i:%s', phi/K update: %s days, phi/K ED '%i:%s',Vm ED:'%i:%s',Initial phi ED: '%i:%s',init_K ED: '%i:%s", conc_ID, g2_dissolved, g3_deltat_string, Kxx_exp_ID, g5_Kxx_exp, Vm_ID, g8_Vm, init_por_ID, g9_init_por, init_Kxx_ID, g10_init_Kxx);
						IfmInfo(pDoc, "Effective porosity and permeability evolution timestep: %s days", g11_eff_timestep_string);
					}
				}
			}
		}
	}										// END of 3D plug-in properties window
}

void CPorosity2::PostTimeStep (IfmDocument pDoc)
{	
	g3_deltat_double = atof(g3_deltat_string);
	g11_eff_timestep_double = atof(g11_eff_timestep_string);
//	g13_PC_dt_double = atof(g13_PC_dt_string);    PETRUS_4

	/* 0 - CLOCK */

	double start_time = clock();

	/* 1 - Define mesh and simulation variables */
	const int ne = IfmGetNumberOfElements(pDoc);						// number of elements 
	const int nDOF = IfmGetNumberOfNodesPerElement(pDoc);				// nodes per element 
	const int ndim = IfmGetNumberOfDimensions(pDoc);					// number of dimensions 

	/* 2 - Utilities to know if it's working, know simulation time, log messages, which nodal distribution use to change porosity */
	double days = IfmGetAbsoluteSimulationTime(pDoc);					// variable days store simulation time  

	long conc_ID = IfmGetRefDistrIdByName(pDoc, g2_dissolved);				// looks for nodal distribution with name 'g2_dissolved' (string) 
	long Kxx_exp_ID = IfmGetElementalRefDistrIdByName(pDoc, g5_Kxx_exp);	// looks for phi/Kxx exponent elemental distribution ID with string name 
	long Kyy_exp_ID = IfmGetElementalRefDistrIdByName(pDoc, g5_Kyy_exp);	// looks for phi/Kyy exponent elemental distribution ID with string name
	long Vm_ID = IfmGetElementalRefDistrIdByName(pDoc, g8_Vm);				// looks for Vm elemental distribution ID with string name
	long init_por_ID = IfmGetElementalRefDistrIdByName(pDoc, g9_init_por);	// and returns it's ID (int). Returns -1 if there's no distribution with given name
	long init_Kxx_ID = IfmGetElementalRefDistrIdByName(pDoc, g10_init_Kxx);
	long init_Kyy_ID = IfmGetElementalRefDistrIdByName(pDoc, g10_init_Kyy);
	/* For 3D only */
	long init_Kyy_ID_3D = IfmGetElementalRefDistrIdByName(pDoc, g14_Kyy_0);
	long init_Kzz_ID = IfmGetElementalRefDistrIdByName(pDoc, g15_Kzz_0);
	long Kyy_exp_ID_3D = IfmGetElementalRefDistrIdByName(pDoc, g16_Kyy_exp);
	long Kzz_exp_ID = IfmGetElementalRefDistrIdByName(pDoc, g17_Kzz_exp);

	// IF nodal distribution does NOT exist, end simulation
	//	if (conc_ID == -1) { IfmInfo(pDoc, "Reaction ouput nodal distribution does NOT exist, simulation stop"); IfmSetSimulationControlFlag(pDoc, IfmCTL_BREAK); }   PETRUS_3
	if (Kxx_exp_ID == -1) { IfmInfo(pDoc, "phi/K exponent elemental distribution NOT exist, simulation stop"); IfmSetSimulationControlFlag(pDoc, IfmCTL_BREAK); }
	if (Vm_ID == -1) { IfmInfo(pDoc, "Molar volume elemental distribution does NOT Exist, simulation stop"); IfmSetSimulationControlFlag(pDoc, IfmCTL_BREAK); }
	if (init_por_ID == -1) { IfmInfo(pDoc, "Initial porosity elemental distribution does NOT exist, simulation stop"); IfmSetSimulationControlFlag(pDoc, IfmCTL_BREAK); }
	if (init_Kxx_ID == -1) { IfmInfo(pDoc, "Initial K elemental distribution does NOT exist, simulation stop"); IfmSetSimulationControlFlag(pDoc, IfmCTL_BREAK); }

	// Declare a number of days to let model mixing zone stabilize and until then start changing porosity.
	if (days < g1_stab) { goto unstable_mixing_zone; }

	/* 2.1 - Run the algorithm or not? Every PostTimeStep is slow, here is the input of an interval between calculations in days */

	double TS_dt = IfmGetCurrentTimeIncrement(pDoc);		//  Time increment in model in last timestep 

	g4_sim_deltat = g4_sim_deltat + TS_dt;				// Accumulated mass transport and reaction simulation time (g4_sim_deltat) + 206(TS_dt)
	if (g4_sim_deltat > g3_deltat_double) { goto calculate_new_por; }
	else { goto no_por_modif; }								// goto label going to line 292 (goto no_area_modif;)

calculate_new_por:										// goto label coming from line 209 (goto calculate_new_por;)

	/* 3 - Modify porosity based on dissolved calcite 'reaction product' */

	/* 3.1 - How many chemical species? and which one is the active chemical species */

	const int nspecies = IfmGetNumberOfSpecies(pDoc);				// Get total number of chemical species
	const int current_species = IfmGetMultiSpeciesId(pDoc);			// Get active species index (Id) among [0:nspecies-1]
	double* c = new double[nDOF];									// it allocates memory 'c' than can hold 'nDOF' number of variables, for squares is 4

	/* 3.2 */
	for (int sp = 0; sp < nspecies; sp++) {							// START of loop to visit all chemical species
		IfmSetMultiSpeciesId(pDoc, sp);								// Set active species index to force FEFLOW operating on species index sp

		for (int elem = 0; elem < ne; elem++) {							// START of loop to visit all mesh elements for current species sp 
			// Careful! elem = 1 in FeFlow GUI is elem = 0 here 	
			/* 3.3 - Get nodal concentrations of 'reaction product' for element 'elem' */

			for (int nodeindex = 0; nodeindex < nDOF; nodeindex++) {	// START of loop to get the dissolved calcite value 'reaction product' from all nodes in an element 
				const int nn = IfmGetNode(pDoc, elem, nodeindex);			// retrieves the node number from the element 'elem' and node index 'nodeindex' and store it in 'nn' 
				if (g18_ddb == 0)											// 0 = dissolution, 1 precipitation, 2 both	
					//c[nodeindex] = std::min(0.0, IfmGetRefDistrValue(pDoc, conc_ID, nn)); // DISSOLUTION: retrieves 'reaction product' from nodal distribution 'conc_ID' from the node 'nn' in the mesh 
					c[nodeindex] = IfmGetResultsTransportMassValue(pDoc, nn);   // PETRUS_1
				else{
					if (g18_ddb == 1) c[nodeindex] = std::max(0.0, IfmGetRefDistrValue(pDoc, conc_ID, nn)); // MINERAL PRECIPITATION
					else { c[nodeindex] = IfmGetRefDistrValue(pDoc, conc_ID, nn); }   // BOTH: dissolution + precipitation
				}
			}	//  END of loop to get the dissolved calcite value 'reaction product' from all nodes in an element. 
			//	When elements are squares, it loops from 0 to 3*/

			/* 3.4 Estimate elemental average 'reaction product' value from nodal values stored in array c[] of 'nDOF' size */
			double sum_mine = 0;											//variable to store the sum of 'reaction product' or other reacted mineral values

			for (int nodeindex = 0; nodeindex < nDOF; nodeindex++) {    // START of FOR that loops trough array c[] of nDOF size to accumulate values in 'sum_mine'
				sum_mine = c[nodeindex] + sum_mine;
			}	// END of loop that accumulates 'reaction product' values.								
			double aved_mine = sum_mine / nDOF;							// elemental 'aved_min' dissolved mineral average 
			//double rate_mine = aved_mine / g13_PC_dt_double;			// mineral dissolution rate in mol/(L*days) to extrapolate along effective phi/K timestep (g13)
			

			/* 3.5 Write new porosity 'p' values in element 'elem' */
			double e_por = IfmGetMatMassPorosity(pDoc, elem);			// get material porosity from element 'elem' and store it in 'por'
			double e_init_por = IfmGetElementalRefDistrValue(pDoc, init_por_ID, elem);// retrieves 'initial porosity' from elemental distribution 'init_por_ID' from element 'elem' in the mesh
			double e_Vm = IfmGetElementalRefDistrValue(pDoc, Vm_ID, elem);//retrieves 'Molar volume' from elemental distribution 'Vm_ID' from the element 'elem' in the mesh

			double rate_mine = 0.4 * ((2 - aved_mine)/1000) * ( 1 - e_por);	// PETRUS_2

			if (rate_mine < 0)
			{
				rate_mine = 0;
			}

			double p = e_por + (g7_mg_mol*e_Vm*rate_mine*g11_eff_timestep_double*e_init_por);// 'p' is the new porosity, g_7 is 1 for input in mol/L, calculate manually for inputs in mg/L PETRUS_5
			IfmSetMatMassPorosity(pDoc, elem, p);						// SetMatMassPorosity value p to element 'elem' 
		}							// END of FOR to visit all mesh element for current species 'sp' 

	}							// END of FOR to visit all chemical species 

	IfmSetMultiSpeciesId(pDoc, current_species);						// Restore originally active species index to avoid conflicts with potential other plugins
	g12_accum_phiK = g12_accum_phiK + g11_eff_timestep_double;				//effective phi_K simulation time accumulates.

	/* 4 - Estimate and write new hydraulic conductivity (K) values based on previously changed porosity (p) values */

	if (ndim == 2)														// checks variable ndim(number of dimensions) This is for 2D models.
	for (int elem = 0; elem < ne; elem++) {							// Careful! elem = 1 in FeFlow GUI is elem = 0 here 
		double e_por = IfmGetMatMassPorosity(pDoc, elem);				// get this timestep material porosity from element 'elem' and store it in 'por'
		double e_Kxx_exp = IfmGetElementalRefDistrValue(pDoc, Kxx_exp_ID, elem);  // retrieves 'phi/Kxx exponent' from elemental distribution 'Kxx_exp_ID' from the element 'elem' in the mesh
		double e_init_por = IfmGetElementalRefDistrValue(pDoc, init_por_ID, elem);// retrieves 'initial porosity' from elemental distribution 'init_por_ID' from element 'elem' in the mesh
		double e_init_Kxx = IfmGetElementalRefDistrValue(pDoc, init_Kxx_ID, elem);// retrieves 'initial Kxx' from elemental distribution 'init_Kxx_ID' element 'elem' in the mesh
		double new_Kxx = e_init_Kxx * pow(e_por / e_init_por, e_Kxx_exp);	// K = K_0 (Phi/Phi_0)^alpha porosity/permeability relationship model

		double e_Kyy_exp = IfmGetElementalRefDistrValue(pDoc, Kyy_exp_ID, elem);  // retrieves 'phi/Kxx exponent' from elemental distribution 'Kyy_exp_ID' from the element 'elem' in the mesh
		double e_init_Kyy = IfmGetElementalRefDistrValue(pDoc, init_Kyy_ID, elem);// retrieves 'initial Kyy' from elemental distribution 'init_Kyy_ID' element 'elem' in the mesh
		double new_Kyy = e_init_Kyy * pow(e_por / e_init_por, e_Kyy_exp);	// K = K_0 (Phi/Phi_0)^alpha porosity/permeability relationship model for Kyy
		double new_Anis = new_Kyy / new_Kxx;							// New anisotropy factor

		IfmSetMatConductivityValue2D(pDoc, elem, new_Kxx);				// sets Kxx value to 2D element
		IfmSetMatAnisotropyFactor2D(pDoc, elem, new_Anis);				// set 
	}

	else {
		for (int elem = 0; elem < ne; elem++) {							// New porosity for 3D models
			double e_por = IfmGetMatMassPorosity(pDoc, elem);				// get this timestep material porosity from element 'elem' and store it in 'e_por'
			double e_init_por = IfmGetElementalRefDistrValue(pDoc, init_por_ID, elem);// retrieves 'initial porosity' from elemental distribution 'init_por_ID' from element 'elem' in the mesh

			double e_Kxx_0 = IfmGetElementalRefDistrValue(pDoc, init_Kxx_ID, elem);	// retrieves 'initial Kxx' from elemental distribution 'init_Kxx_ID' from the element 'elem' in the mesh
			double e_Kyy_0 = IfmGetElementalRefDistrValue(pDoc, init_Kyy_ID_3D, elem);	// retrieves 'initial Kyy' from elemental distribution 'init_Kyy_ID_3D' 
			double e_Kzz_0 = IfmGetElementalRefDistrValue(pDoc, init_Kzz_ID, elem);   // retrieves 'initial Kzz' from elemental distribution 'init_Kzz_ID' 

			double e_phi_K_exp = IfmGetElementalRefDistrValue(pDoc, Kxx_exp_ID, elem);// retrieves 'Kxx exponent' from elemental distribution 'Kxx_exp_ID' from the element 'elem' in the mesh
			double e_Kyy_exp = IfmGetElementalRefDistrValue(pDoc, Kyy_exp_ID_3D, elem);  // retrieves 'Kyy exponent' from elemental distribution 'Kyy_exp_ID_3D' 
			double e_Kzz_exp = IfmGetElementalRefDistrValue(pDoc, Kzz_exp_ID, elem);  // retrieves 'Kzz exponent' from elemental distribution 'Kzz_exp_ID' 

			double new_Kxx = e_Kxx_0 * pow(e_por / e_init_por, e_phi_K_exp);	// K = K_0 (Phi/Phi_0)^alpha porosity/permeability relationship model
			double new_Kyy = e_Kyy_0* pow(e_por / e_init_por, e_Kyy_exp);		// K = K_0 (Phi/Phi_0)^alpha porosity/permeability relationship model
			double new_Kzz = e_Kzz_0* pow(e_por / e_init_por, e_Kzz_exp);		// K = K_0 (Phi/Phi_0)^alpha porosity/permeability relationship model

			IfmSetMatXConductivityValue3D(pDoc, elem, new_Kxx);		// Set new Kxx, Kyy,Kzz values		
			IfmSetMatYConductivityValue3D(pDoc, elem, new_Kyy);
			IfmSetMatZConductivityValue3D(pDoc, elem, new_Kzz);
		}
	}
	// END of FOR elem (elements) to change hydraulic conductivity K
	delete [] c;														// delete dynamic memory allocation double* c = new double[nDOF]

	/* 0.1 END of CLOCK */
	double end_time = clock();
	double clock_time = end_time - start_time;
	double timeInSeconds = clock_time / CLOCKS_PER_SEC;
	if (g6_message > 1);			// IF g6_messages > 1, do nothing.
	else{ IfmInfo(pDoc, "Effective phi & K simulation time. %g days, update of p & K values took %g clicks of clock() or %g seconds", g12_accum_phiK, clock_time, timeInSeconds); } // FeFlow log screen message 

	// goto that comes from the IF used to jump all the porosity increase process until mixing zone is in equilibrium

	g4_sim_deltat = 0 + (g4_sim_deltat - g3_deltat_double) ;					// reset delta t counter to 0 after porosity and permeability were modified, affects line 208. This line is before the no_area_modif: label
	// it is executed only when geometry is modified
	

unstable_mixing_zone:
	if (g6_message > 0);
	else{
		if (days < g1_stab)
		{
			IfmInfo(pDoc, "Porosity unchanged, simulation time %g days, time declared until mixing zone stabilizes %g days", days, g1_stab);
		}
	}

no_por_modif:
	if (init_Kxx_ID == -1) { ; }		// DUMMY line, i need to put a line to 'make no_por_modif:' label work. the IF is actually empty

	_CrtDumpMemoryLeaks(); /* Memory leak 2nd Component*/
}


