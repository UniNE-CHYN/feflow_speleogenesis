#include "stdifm.h"
#include "export_permeable_mc_porosityPlug-in1.h"

#include <fstream>						// to write in a text file
#include <stdlib.h>						// library for atof = string to double 
#include <string.h>						// library for std::to_string = double to string
#include <sstream>
#include <math.h>						// Linux compilation needs this header
#include <iomanip>						// to format column width in text file
#include <vector>						// count vector length, vector pushback, etc
#include <iostream>						// to read a text file
#include <algorithm>					// merge vectors and arrays

#include <ifm/document.h>

IfmModule g_pMod;  /* Global handle related to this plugin */

#pragma region IFM_Definitions
/* --- IFMREG_BEGIN --- */
/*  -- Do not edit! --  */

static IfmResult OnBeginDocument (IfmDocument);
static void OnEndDocument (IfmDocument);
static void Serialize (IfmDocument, IfmArchive);
static void OnEditDocument (IfmDocument, Widget);
static void OnLeaveSimulator (IfmDocument);
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
  IfmRegisterModule (pMod, "SIMULATION", "EXPORT_PERMEABLE_MC_POROSITYPLUG_IN1", "export_permeable_mc_porosityPlug-in1", 0x1000);
  IfmSetDescriptionString (pMod, szDesc);
  IfmSetCopyrightPath (pMod, "export_permeable_mc_porosityPlug-in1.txt");
  IfmSetHtmlPage (pMod, "export_permeable_mc_porosityPlug-in1.htm");
  IfmSetPrimarySource (pMod, "export_permeable_mc_porosityPlug-in1.cpp");
  IfmRegisterProc (pMod, "OnBeginDocument", 1, (IfmProc)OnBeginDocument);
  IfmRegisterProc (pMod, "OnEndDocument", 1, (IfmProc)OnEndDocument);
  IfmRegisterProc (pMod, "Serialize", 1, (IfmProc)Serialize);
  IfmRegisterProc (pMod, "OnEditDocument", 1, (IfmProc)OnEditDocument);
  IfmRegisterProc (pMod, "OnLeaveSimulator", 1, (IfmProc)OnLeaveSimulator);
  IfmRegisterProc (pMod, "PostTimeStep", 1, (IfmProc)PostTimeStep);
  return True;
}

static void Serialize (IfmDocument pDoc, IfmArchive pArc)
{
  CExportPermeableMcPorosityplugIn1::FromHandle(pDoc)->Serialize (pDoc, pArc);
}
static void OnEditDocument (IfmDocument pDoc, Widget wParent)
{
  CExportPermeableMcPorosityplugIn1::FromHandle(pDoc)->OnEditDocument (pDoc, wParent);
}
static void OnLeaveSimulator (IfmDocument pDoc)
{
  CExportPermeableMcPorosityplugIn1::FromHandle(pDoc)->OnLeaveSimulator (pDoc);
}
static void PostTimeStep (IfmDocument pDoc)
{
  CExportPermeableMcPorosityplugIn1::FromHandle(pDoc)->PostTimeStep (pDoc);
}

/* --- IFMREG_END --- */
#pragma endregion


static IfmResult OnBeginDocument (IfmDocument pDoc)
{
  if (IfmDocumentVersion (pDoc) < IFM_CURRENT_DOCUMENT_VERSION)
    return false;

  try {
    IfmDocumentSetUserData(pDoc, new CExportPermeableMcPorosityplugIn1(pDoc));
  }
  catch (...) {
    return false;
  }

  return true;
}

static void OnEndDocument (IfmDocument pDoc)
{
  delete CExportPermeableMcPorosityplugIn1::FromHandle(pDoc);
}

///////////////////////////////////////////////////////////////////////////
// Implementation of CExportPermeableMcPorosityplugIn1

// Constructor
CExportPermeableMcPorosityplugIn1::CExportPermeableMcPorosityplugIn1 (IfmDocument pDoc)
  : m_pDoc(pDoc)
{
  /*
   * TODO: Add your own code here ...
   */
}

// Destructor
CExportPermeableMcPorosityplugIn1::~CExportPermeableMcPorosityplugIn1 ()
{
  /*
   * TODO: Add your own code here ...
   */
}

// Obtaining class instance from document handle
CExportPermeableMcPorosityplugIn1* CExportPermeableMcPorosityplugIn1::FromHandle (IfmDocument pDoc)
{
  return reinterpret_cast<CExportPermeableMcPorosityplugIn1*>(IfmDocumentGetUserData(pDoc));
}

// Callbacks

/* GLobal varables here: values that can be used in all callbacks (serialize, onEdit, onLeave, PostTimeStep */
/* Global variables values are stored in the FEM file using the serialize callback */

char*	g1_input_filename;					// path and name of node list file
char*	g2_output_location;					// path and name of output file (hydraulic head, mass concentration)
char*	g3_export_timestep_filename;		// time step at which values will exported, values in days units

int		g5_IFM_access_counter;

void CExportPermeableMcPorosityplugIn1::Serialize (IfmDocument pDoc, IfmArchive pArc)
{
	switch (IfmioGetMode(pArc)) {				// decide what's going on: INIT, STORE, LOAD, FREE? 
	case IfmIO_STORE:							// activated when FEM files is saved on the GUI
		IfmioString(pArc, &g1_input_filename, 200);
		IfmioString(pArc, &g2_output_location, 200);
		IfmioString(pArc, &g3_export_timestep_filename, 200);
		break;
	case IfmIO_LOAD:							// activated when FEM file is opened on the GUI
		if (IfmioGetVersion(pArc) >= 0x1000) {
			// Serialize.
			IfmioString(pArc, &g1_input_filename, 200);
			IfmioString(pArc, &g2_output_location, 200);
			IfmioString(pArc, &g3_export_timestep_filename, 200);
		}
		break;
	case IfmIO_INIT:							// initialize variables will null values
		break;
	case IfmIO_FREE:							// clean memory when FEM file is closed 
		break;
	}
}

void CExportPermeableMcPorosityplugIn1::OnEditDocument (IfmDocument pDoc, Widget wParent)
{
	char*	input_1 = g1_input_filename;				// variables used on the onEdit window	
	char*	input_2 = g2_output_location;					
	char*	input_3 = g3_export_timestep_filename;
	
	IfmProperty props[] = {
		{ "1 Node list filename and location, example D:\\work\\nodes.dat", IfmPROP_STRING, &input_1, &g1_input_filename, "node list filename *.dat" },
		{ "2 Output file location, example D:\\work\\", IfmPROP_STRING, &input_2, &g2_output_location, "export directory pat " },
		{ "3 Time-step list filename and location, i.e. D:\\work\\timestep_list.dat ", IfmPROP_STRING, &input_3, &g3_export_timestep_filename, "export time-step list *.dat" }
	};
	IfmEditProperties(pDoc, "Export data settings", "Variable values: ", props, 3); // Last number is amount of input values.

	g1_input_filename = input_1;
	g2_output_location = input_2;
	g3_export_timestep_filename = input_3;
}

void CExportPermeableMcPorosityplugIn1::OnLeaveSimulator(IfmDocument pDoc)
{
	using namespace std;

	std::string node_list;								// create std::string structure for filename input
	node_list.append(g1_input_filename);				// append the name and location of file into std:string

	std::string export_location;						// create std::string structure for export filename
	export_location.append(g2_output_location);

	std::string timestep_filename;						// create std::string structure for export time step filename
	timestep_filename.append(g3_export_timestep_filename);

	//if (g5_IFM_access_counter < 1)	{					// initializing for 1st time the IFM access counter, only works the first time the IFM is used
	//	g5_IFM_access_counter = g5_IFM_access_counter + 1;
	//}

	ifstream inputfile3(timestep_filename, std::ios::binary | ios::in);			// open timestep list input file
	double line3;															// store here values of every line	
	std::vector<double> timestep_list;										// Vector for holding all lines in the file

	if (inputfile3.is_open())
	{
		//IfmInfo(pDoc, "Imported node numbers from input file (*.dat)");	 // message for the GUI log window 

		while (inputfile3 >> line3)
		{
			timestep_list.push_back(line3);                              // Save the line in the vector
			//IfmInfo(pDoc, "%i", line1);								// prints every export timestep on GUI log window 
		}
		inputfile3.close();
	}
	else
	{
		IfmInfo(pDoc, "A5: export time-step input  file (*.dat) file not found on given location");
		IfmSetSimulationControlFlag(pDoc, IfmCTL_BREAK);	// stop simulation
	}

	double simulation_time;
	simulation_time = IfmGetAbsoluteSimulationTime(pDoc);		// gets last time step length in days from FEFLOW simulation

	// ************ START of EXPORT ************

	/* Read node values from input file (*.dat) */

	ifstream inputfile1(node_list, std::ios::binary | ios::in);	// open BC input file
	int line1;													// store here values of every line	
	std::vector<int> export_node_list;							// Vector for holding all lines in the file

	if (inputfile1.is_open())
	{
		if (g5_IFM_access_counter < 3)	{
			IfmInfo(pDoc, "A5: Imported node numbers from input file (*.dat)");			// message for the GUI log window 
		}
		while (inputfile1 >> line1)
		{
			export_node_list.push_back(line1);                   // Save the line in the vector
			if (g5_IFM_access_counter < 3)	{					// put on the log the export nodes only the first time the export is done
				IfmInfo(pDoc, "A5: %i", line1);						// prints every read node on GUI log window 
			}
		}
		inputfile1.close();
	}
	else
	{
		IfmInfo(pDoc, "A5: input file (*.dat) file not found on given location");
		IfmSetSimulationControlFlag(pDoc, IfmCTL_BREAK);	// stop simulation
	}

	/* Write to export file (*.dat) based on nodes given in the input file (*.dat) */

	IfmInfo(pDoc, "A5: Data export made at simulation day: %g", simulation_time);

	int export_node_list_length = static_cast<int>(export_node_list.size());					// node list lenght controls the following FOR loop

	std::string export_filename = export_location;
	export_filename.append("export_final.dat");
	ofstream ofs(export_filename, ios::app);

	for (int n = 0; n < export_node_list_length; n++)
	{
		if (ofs.is_open())
		{
			int projection = IfmGetProblemProjection(pDoc);

			if (projection < 3)												// EXPORT FOR 2D models
			{
				ofs << "  Node  ,     X    ,     Y    ,   Time   ,    Head   , Saturation" << endl;  // Column names 3D

				double x_coord = IfmGetX(pDoc, export_node_list[n]);						// get X coordinate of node "n"
				double y_coord = IfmGetY(pDoc, export_node_list[n]);						// get Y coordinate of node "n"
				double head = IfmGetResultsFlowHeadValue(pDoc, export_node_list[n]);		// ge hydraulic head value of node "n"
				double saturation = IfmGetResultsFlowSaturationValue(pDoc, export_node_list[n]);

			//	double outflow = 0;
			//	IfmBudgetComponents BudgetComp;
			//	IfmBudget *bbb = IfmBudgetFlowCreate(pDoc);
			//	IfmBudgetComponentsQueryFlowAtNode(pDoc, bbb, export_node_list[n], &BudgetComp);
			//	outflow = BudgetComp.boundary_flux;
			//	IfmBudgetClose(pDoc, bbb);

				ofs << std::setfill(' ') << std::setw(7) << export_node_list[n] << " , ";
				ofs << std::setfill(' ') << std::setw(8) << x_coord << " , ";
				ofs << std::setfill(' ') << std::setw(8) << y_coord << " , ";
				ofs << std::setfill(' ') << std::setw(8) << simulation_time << " , ";
				ofs << std::setfill(' ') << std::setw(9) << head << " , ";
				ofs << std::setfill(' ') << std::setw(9) << saturation << endl;
			//	ofs << std::setfill(' ') << std::setw(9) << outflow << endl;

				ofs.close();
			}
			else															// EXPORT FOR 3D models
			{

				ofs << "  Node  ,     X    ,     Y    ,     Z    ,   Time   ,    Head   , Saturation " << endl;  // Column names 3D

				double x_coord = IfmGetX(pDoc, export_node_list[n]);						// get X coordinate of node "n"
				double y_coord = IfmGetY(pDoc, export_node_list[n]);						// get Y coordinate of node "n"
				double z_coord = IfmGetZ(pDoc, export_node_list[n]);						// get Z coordinate of node "n"
				double head = IfmGetResultsFlowHeadValue(pDoc, export_node_list[n]);		// ge hydraulic head value of node "n"
				double saturation = IfmGetResultsFlowSaturationValue(pDoc, export_node_list[n]);

		//		double outflow = 0;
		//		IfmBudgetComponents BudgetComp;
		//		IfmBudget *bbb = IfmBudgetFlowCreate(pDoc);
		//		IfmBudgetComponentsQueryFlowAtNode(pDoc, bbb, export_node_list[n], &BudgetComp);
		//		//double nodeflux_total = BudgetComp.total_flux;
		//		outflow = BudgetComp.boundary_flux;
		//		//double nodeflux_area = BudgetComp.area_flux;
		//		IfmBudgetClose(pDoc, bbb);

				ofs << std::setfill(' ') << std::setw(7) << export_node_list[n] << " , ";
				ofs << std::setfill(' ') << std::setw(8) << x_coord << " , ";
				ofs << std::setfill(' ') << std::setw(8) << y_coord << " , ";
				ofs << std::setfill(' ') << std::setw(8) << z_coord << " , ";
				ofs << std::setfill(' ') << std::setw(8) << simulation_time << " , ";
				ofs << std::setfill(' ') << std::setw(9) << head << " , ";
				ofs << std::setfill(' ') << std::setw(9) << saturation << endl;
		//		ofs << std::setfill(' ') << std::setw(9) << outflow << endl;

				ofs.close();
			}										// END of projection IF-ELSE
		}					// END OF is.open IF
		else
		{
			IfmInfo(pDoc, "A5: location for export file (*.dat) does not exist");
			IfmSetSimulationControlFlag(pDoc, IfmCTL_BREAK);	// stop simulation
		}
	}			// END OF FOR to visit node list vector


}		//	----------------- END of LEAVE SIMULATOR CALLBACK -------------------

void CExportPermeableMcPorosityplugIn1::PostTimeStep(IfmDocument pDoc)
{
	using namespace std;

	std::string node_list;								// create std::string structure for filename input
	node_list.append(g1_input_filename);				// append the name and location of file into std:string

	std::string export_location;						// create std::string structure for export filename
	export_location.append(g2_output_location);

	std::string timestep_filename;						// create std::string structure for export time step filename
	timestep_filename.append(g3_export_timestep_filename);

	if (g5_IFM_access_counter < 1)	{					// initializing for 1st time the IFM access counter, only works the first time the IFM is used
		g5_IFM_access_counter = g5_IFM_access_counter + 1;
	}

	ifstream inputfile3(timestep_filename, std::ios::binary | ios::in);			// open timestep list input file
	double line1;															// store here values of every line	
	std::vector<double> timestep_list;										// Vector for holding all lines in the file

	if (inputfile3.is_open())
	{
		//IfmInfo(pDoc, "Imported node numbers from input file (*.dat)");	 // message for the GUI log window 

		while (inputfile3 >> line1)
		{
			timestep_list.push_back(line1);                              // Save the line in the vector
			//IfmInfo(pDoc, "%i", line1);								// prints every export timestep on GUI log window 
		}
		inputfile3.close();
	}
	else
	{
		IfmInfo(pDoc, "A5: export time-step input  file (*.dat) file not found on given location");
		IfmSetSimulationControlFlag(pDoc, IfmCTL_BREAK);	// stop simulation
	}

	int export_timestep_list_length = static_cast<int>(timestep_list.size());
	double export_time_from_list;
	if (g5_IFM_access_counter > export_timestep_list_length){
		goto end_of_exports;					//  simulation time is larger than the export time-step list, END of exports
	}
	else{
		export_time_from_list = timestep_list[g5_IFM_access_counter - 1];
	}

	double simulation_time;
	simulation_time = IfmGetAbsoluteSimulationTime(pDoc);		// gets absolute simulation time from FEFLOW simulation
	IfmInfo(pDoc, "A5: Simulation time: %g ",simulation_time);

	if (simulation_time > export_time_from_list)	{			// ************ START of time step check IF ************

		g5_IFM_access_counter = g5_IFM_access_counter + 1;		// only adds one if there is an export

		/* Read node values from input file (*.dat) */

		ifstream inputfile1(node_list, std::ios::binary | ios::in);	// open BC input file
		int line1;													// store here values of every line	
		std::vector<int> export_node_list;							// Vector for holding all lines in the file

		if (inputfile1.is_open())
		{
			if (g5_IFM_access_counter < 3)	{
				IfmInfo(pDoc, "A5: Imported node numbers from input file (*.dat)");			// message for the GUI log window 
											}
			while (inputfile1 >> line1)
			{
				export_node_list.push_back(line1);                   // Save the line in the vector
				if (g5_IFM_access_counter < 3)	{					// put on the log the export nodes only the first time the export is done
					IfmInfo(pDoc, "A5: %i", line1);						// prints every read node on GUI log window 
												}
			}
			inputfile1.close();
		}
		else
		{
			IfmInfo(pDoc, "A5: input file (*.dat) file not found on given location");
			IfmSetSimulationControlFlag(pDoc, IfmCTL_BREAK);	// stop simulation
		}

		/* Write to export file (*.dat) based on nodes given in the input file (*.dat) */

		IfmInfo(pDoc, "A5: Data export made at simulation day: %g", simulation_time);

		int export_node_list_length = static_cast<int>( export_node_list.size() );					// node list lenght controls the following FOR loop
			
		for (int n = 0; n < export_node_list_length; n++)
		{

			int filenum = n + 1;
			string filenum_text = std::to_string(filenum);
			std::string export_filename = export_location;
			export_filename.append("export_");
			export_filename.append(filenum_text);
			export_filename.append(".dat");
			ofstream ofs(export_filename, ios::app);

			if (ofs.is_open())
			{
				int projection = IfmGetProblemProjection(pDoc);

				if (projection < 3)												// EXPORT FOR 2D models
				{
					if (g5_IFM_access_counter < 3)	{
						ofs << "  Node  ,     X    ,     Y    ,   Time   ,    Head   , Saturation,  Outflow   " << endl;  // Column names 3D
													}
					double x_coord = IfmGetX(pDoc, export_node_list[n]);						// get X coordinate of node "n"
					double y_coord = IfmGetY(pDoc, export_node_list[n]);						// get Y coordinate of node "n"
					double head = IfmGetResultsFlowHeadValue(pDoc, export_node_list[n]);		// ge hydraulic head value of node "n"
					double saturation = IfmGetResultsFlowSaturationValue(pDoc, export_node_list[n]);

					double outflow = 0;
					IfmBudgetComponents BudgetComp;
					IfmBudget *bbb = IfmBudgetFlowCreate(pDoc);
					IfmBudgetComponentsQueryFlowAtNode(pDoc, bbb, export_node_list[n], &BudgetComp);
					//double nodeflux_total = BudgetComp.total_flux;
					outflow = BudgetComp.boundary_flux;
					//double nodeflux_area = BudgetComp.area_flux;
					IfmBudgetClose(pDoc, bbb);

					ofs << std::setfill(' ') << std::setw(7) << export_node_list[n] << " , ";
					ofs << std::setfill(' ') << std::setw(8) << x_coord << " , ";
					ofs << std::setfill(' ') << std::setw(8) << y_coord << " , ";
					ofs << std::setfill(' ') << std::setw(8) << simulation_time << " , ";
					ofs << std::setfill(' ') << std::setw(9) << head << " , ";
					ofs << std::setfill(' ') << std::setw(9) << saturation << " , ";
					ofs << std::setfill(' ') << std::setw(9) << outflow << endl;

					ofs.close();

				}
				else															// EXPORT FOR 3D models
				{
					if (g5_IFM_access_counter < 3)	{
						ofs << "  Node  ,     X    ,     Y    ,     Z    ,   Time   ,    Head   , Saturation,  Outflow  " << endl;  // Column names 3D
												}

					double x_coord = IfmGetX(pDoc, export_node_list[n]);						// get X coordinate of node "n"
					double y_coord = IfmGetY(pDoc, export_node_list[n]);						// get Y coordinate of node "n"
					double z_coord = IfmGetZ(pDoc, export_node_list[n]);						// get Z coordinate of node "n"
					double head = IfmGetResultsFlowHeadValue(pDoc, export_node_list[n]);		// ge hydraulic head value of node "n"
					double saturation = IfmGetResultsFlowSaturationValue(pDoc, export_node_list[n]);

					double outflow = 0;
					IfmBudgetComponents BudgetComp;
					IfmBudget *bbb = IfmBudgetFlowCreate(pDoc);
					IfmBudgetComponentsQueryFlowAtNode(pDoc, bbb, export_node_list[n], &BudgetComp);
					//double nodeflux_total = BudgetComp.total_flux;
					outflow = BudgetComp.boundary_flux;
					//double nodeflux_area = BudgetComp.area_flux;
					IfmBudgetClose(pDoc, bbb);

					ofs << std::setfill(' ') << std::setw(7) << export_node_list[n] << " , ";
					ofs << std::setfill(' ') << std::setw(8) << x_coord << " , ";
					ofs << std::setfill(' ') << std::setw(8) << y_coord << " , ";
					ofs << std::setfill(' ') << std::setw(8) << z_coord << " , ";
					ofs << std::setfill(' ') << std::setw(8) << simulation_time << " , ";
					ofs << std::setfill(' ') << std::setw(9) << head << " , ";
					ofs << std::setfill(' ') << std::setw(9) << saturation << " , ";
					ofs << std::setfill(' ') << std::setw(9) << outflow << endl;

					ofs.close();
				}
			}

			else {
				IfmInfo(pDoc, "A5: location for export file (*.dat) does not exist");
				IfmSetSimulationControlFlag(pDoc, IfmCTL_BREAK);	// stop simulation
			}
		}   // END OF FOR from line 398
												}  // ********** END of time step check IF from line 362***********
	end_of_exports:
		if (g5_IFM_access_counter > export_timestep_list_length){ IfmInfo(pDoc, "A5:simulation time is larger than the export time-step list, END of exports"); }
	}



