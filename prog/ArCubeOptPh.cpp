#include "PhysList.hh"
#include "DetConstr.hh"
#include "AnalysisManager.hh"
#include "PrimGenAction.hh"
//#include "StackingAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"
#include "SteppingAction.hh"

#include "globals.hh"
#include "G4String.hh"
#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4UIterminal.hh"
#include "G4UItcsh.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

#include <string>
#include <fstream>
#include <sstream>
#include <unistd.h>

/*
#ifdef G4MULTITHREADED
#include "G4MTRunManager.hh"
#else
#include "G4RunManager.hh"
#endif
*/



using std::string;
using std::stringstream;
using std::ifstream;


void usage();

int main(int argc, char **argv)
{
	// switches
	int c = 0;

	stringstream hStream;
	
	bool bInteractive = true;
	bool bVisualize = false;
	bool bVrmlVisualize = false;
	bool bOpenGlVisualize = false;
	bool bUseGui = false;
	
	bool bGdmlFile = false;
	bool bPreinitMacroFile = false;
	bool bMacroFile = false;
	G4String hGdmlFileName, hPreinitMacroFileName, hMacroFileName, hOutFileName;
	int iNbEventsToSimulate = 0;

	// parse switches
	while((c = getopt(argc,argv,"h:g:p:m:o:n:G")) != -1)
	{
		switch(c)
		{
			case 'g':
				bGdmlFile = true;
				hGdmlFileName = optarg;
				break;
				
			case 'p':
				bPreinitMacroFile = true;
				hPreinitMacroFileName = optarg;
				break;
				
			case 'm':
				bMacroFile = true;
				hMacroFileName = optarg;
				if(bInteractive) bInteractive=false;
				break;
				
			case 'o':
				hOutFileName = optarg;
				break;
				
			case 'n':
				hStream.clear(); hStream.str("");
				hStream << optarg;
				iNbEventsToSimulate = atoi(hStream.str().c_str());
				break;
				
			case 'G':
				bUseGui=true;
				break;
				
			default:
				usage();
		}
	}
	
	
	if(!bGdmlFile){
		G4cout << "\nERROR: the gdml file must be provided!" << G4endl;
		usage();
	}
	
	// create the run manager
/*
#ifdef G4MULTITHREADED
	G4RunManager *pRunManager = new G4RunManager;
#else
	G4MTRunManager* runManager = new G4MTRunManager;
	runManager->SetNumberOfThreads(1);
#endif
*/
	
	G4RunManager *pRunManager = new G4RunManager;
	
	PhysListOptPh *pPhysList = new PhysListOptPh();
	DetConstrOptPh *pDetGeom = new DetConstrOptPh(hGdmlFileName);
	
	// set user-defined initialization classes
	pRunManager->SetUserInitialization(pDetGeom);
	pRunManager->SetUserInitialization(pPhysList);
	
	
	

	// create the primary generator action
	PrimaryGeneratorActionOptPh *pPrimaryGeneratorAction = new PrimaryGeneratorActionOptPh();

	// create an analysis manager object
	AnalysisManagerOptPh *pAnalysisManager = new AnalysisManagerOptPh(pPrimaryGeneratorAction);
	
	
	// set user-defined action classes
	pRunManager->SetUserAction(pPrimaryGeneratorAction);
	//pRunManager->SetUserAction(new StackingAction(pAnalysisManager));
	pRunManager->SetUserAction(new RunActionOptPh(pAnalysisManager));
	pRunManager->SetUserAction(new EventActionOptPh(pAnalysisManager));
	pRunManager->SetUserAction(new SteppingActionOptPh(pAnalysisManager));
	
	
	
	G4UImanager* pUImanager = G4UImanager::GetUIpointer();
	G4UIsession * pUIsession = NULL;
	G4UIExecutive* ui = NULL;
	G4VisManager* pVisManager = NULL;
	
	
	if(bPreinitMacroFile){
		G4String hCommand = "/control/execute " + hPreinitMacroFileName;
		pUImanager->ApplyCommand(hCommand);
	}
	
	//Initialize the RunManager
	pRunManager->Initialize();
	
	if(bInteractive){
		// Visualization Manager
		pVisManager = new G4VisExecutive;
		pVisManager->Initialize();
		
		//Let G4UIExecutive guess what is the best available UI
		ui = new G4UIExecutive(1,argv);
		if (ui->IsGUI() && bUseGui ){
			//pUImanager->ApplyCommand("/control/execute gui.mac");
			ui->SessionStart();
			delete ui;
		}else{
			pUIsession = new G4UIterminal(new G4UItcsh);
			pUIsession->SessionStart();
			delete pUIsession;
		}
		
	}else{
		if(bMacroFile)
		{
			G4String hCommand = "/control/execute " + hMacroFileName;
			pUImanager->ApplyCommand(hCommand);
		}
		if(!hOutFileName.empty()) pAnalysisManager->SetDataFilename(hOutFileName);
			
		if(iNbEventsToSimulate){
			pAnalysisManager->SetNbEventsToSimulate(iNbEventsToSimulate);
			hStream.clear(); hStream.str("");
			hStream << "/run/beamOn " << iNbEventsToSimulate;
			pUImanager->ApplyCommand(hStream.str());
		}
	}
	
	
	//delete pAnalysisManager;
	if(pVisManager) delete pVisManager;
	delete pRunManager;
	return 0;
}


void usage()
{
	G4cout << "\nUsage:" << G4endl;
	G4cout << "ArCubeOptPh -g gdmlfile [-p preinit.mac] [-m macro.mac] [-o output.root] [-n nevents]" << G4endl;
	G4cout << "ArCubeOptPh -g gdmlfile -G [-p preinit.mac]"<< G4endl;
	exit(0);
}

