/**
 * AUTHOR: 
 * CONTACT: 
 * FIRST SUBMISSION: 12-28-2010
 *
 * REVISION:
 *
 * mm-dd-yyyy, What is changed, Whoami
 *
 * 12-28-2010, the switching from Geant4.9.2 to Geant4.9.3
 *             is finished starting from e+, Xiang
 */

// ---------------------------------------------------------------------------

#include "PhysListMessenger.hh"
#include "PhysList.hh"

#include <iomanip>

#include "globals.hh"

#include "G4PhysicsListHelper.hh"

#include "G4ParticleDefinition.hh"
#include "G4ParticleTypes.hh"
#include "G4ParticleTable.hh"

#include "G4ProcessManager.hh"
#include "G4ProcessVector.hh"

#include "G4ParticleWithCuts.hh"
#include "G4UserLimits.hh"
#include "G4ios.hh"

#include "G4StepLimiter.hh"
#include "G4OpAbsorption.hh"
#include "G4OpWLS.hh"
#include "G4OpRayleigh.hh"
#include "G4OpMieHG.hh"
#include "G4OpBoundaryProcess.hh"
#include "G4Scintillation.hh"

// EM processes (so particles deposit energy and can produce scintillation)
#include "G4eIonisation.hh"
#include "G4eMultipleScattering.hh"
#include "G4eplusAnnihilation.hh"
#include "G4MuIonisation.hh"
#include "G4MuMultipleScattering.hh"
#include "G4hIonisation.hh"
#include "G4hMultipleScattering.hh"

#include "G4SystemOfUnits.hh"
// Ensure basic particles for EM processes are available
#include "G4MuonMinus.hh"
#include "G4MuonPlus.hh"
#include "G4Electron.hh"
#include "G4Positron.hh"
#include "G4Gamma.hh"

//Instance and initialize all the static members
//G4ThreadLocal G4int PhysListOptPh::fVerboseLevel = 0;
//G4ThreadLocal G4int PhysListOptPh::fOpVerbLevel = 0;

G4ThreadLocal G4StepLimiter *PhysListOptPh::fStepLimiter = nullptr;
G4ThreadLocal G4OpAbsorption* PhysListOptPh::fAbsorptionProcess = nullptr;
G4ThreadLocal G4OpRayleigh* PhysListOptPh::fRayleighScatteringProcess = nullptr;
G4ThreadLocal G4OpWLS* PhysListOptPh::fWLSProcess = nullptr;
G4ThreadLocal G4OpMieHG* PhysListOptPh::fMieHGScatteringProcess = nullptr;
G4ThreadLocal G4OpBoundaryProcess* PhysListOptPh::fBoundaryProcess = nullptr;
G4ThreadLocal G4Scintillation* PhysListOptPh::fScintillation = nullptr;



PhysListOptPh::PhysListOptPh():
G4VUserPhysicsList(),
fVerboseLevel(PhysVerbosity::kSilent),
fDefaultCutValue(1. * mm),
fScintillationEnabled(true)
{
	
	fMessenger = new PhysListOptPhMessenger(this);
	
	/*
	cutForGamma = defaultCutValue;
	cutForElectron = defaultCutValue;
	cutForPositron = defaultCutValue;
	VerboseLevel = 0;
	OpVerbLevel = 0;
	*/
	SetVerboseLevel( static_cast<G4int>(PhysListOptPh::fVerboseLevel) );

}


PhysListOptPh::~PhysListOptPh()
{
	if(fMessenger) delete fMessenger;
}


void PhysListOptPh::ConstructParticle()
{
	G4Geantino::GeantinoDefinition();
	//G4ChargedGeantino::ChargedGeantinoDefinition();
	
	G4OpticalPhoton::OpticalPhotonDefinition();

	// Define standard muons here so particle table contains them
	// before messengers and macros try to reference "mu-" / "mu+".
	G4MuonMinus::MuonMinusDefinition();
	G4MuonPlus::MuonPlusDefinition();

	// Ensure electrons/positrons are available too
	G4Electron::ElectronDefinition();
	G4Positron::PositronDefinition();

	// Ensure gamma (photon) is defined for EM processes
	G4Gamma::GammaDefinition();
}


void PhysListOptPh::ConstructProcess()
{
	AddTransportation();
	ConstructOptical();
}





//#include "XeMaxTimeCuts.hh"
//#include "XeMinEkineCuts.hh"





// Optical Processes ////////////////////////////////////////////////////////

void PhysListOptPh::ConstructOptical()
{
	//I don't need all the scintillation and chernkov processses
	
	// optical processes
	fStepLimiter = new G4StepLimiter();
	fAbsorptionProcess = new G4OpAbsorption();
	fWLSProcess = new G4OpWLS();
	fRayleighScatteringProcess = new G4OpRayleigh();
	fMieHGScatteringProcess = new G4OpMieHG();
	fWLSProcess = new G4OpWLS();
	fBoundaryProcess = new G4OpBoundaryProcess();

	// scintillation process (create only if enabled)
	if(fScintillationEnabled){
		fScintillation = new G4Scintillation("Scintillation");
		fScintillation->SetTrackSecondariesFirst(true);
		// note: particle-specific yield can be enabled via SetScintillationByParticleType(true) if desired
	}
	
	//  theAbsorptionProcess->DumpPhysicsTable();
	//  theWLSProcess->DumpPhysicsTable();
	//  theRayleighScatteringProcess->DumpPhysicsTable();
	
	
	
	fStepLimiter->SetVerboseLevel(static_cast<G4int>(PhysListOptPh::fVerboseLevel));
	fAbsorptionProcess->SetVerboseLevel(static_cast<G4int>(PhysListOptPh::fVerboseLevel));
	fWLSProcess->SetVerboseLevel(static_cast<G4int>(PhysListOptPh::fVerboseLevel));
	fRayleighScatteringProcess->SetVerboseLevel(static_cast<G4int>(PhysListOptPh::fVerboseLevel));
	fMieHGScatteringProcess->SetVerboseLevel(static_cast<G4int>(PhysListOptPh::fVerboseLevel));
	fWLSProcess->SetVerboseLevel(static_cast<G4int>(PhysListOptPh::fVerboseLevel));
	fBoundaryProcess->SetVerboseLevel(static_cast<G4int>(PhysListOptPh::fVerboseLevel));
	if(fScintillation) fScintillation->SetVerboseLevel(static_cast<G4int>(PhysListOptPh::fVerboseLevel));
	
	
	//G4OpticalSurfaceModel themodel = unified;
	
	//The unified model for the G4OpBoundaryProcess is the standard one
	//fBoundaryProcess->SetModel( (G4OpticalSurfaceModel)unified ); //G4OpticalSurfaceModel is an enumerator!
	
	G4PhysicsListHelper* ph = G4PhysicsListHelper::GetPhysicsListHelper();
	
	G4ParticleTable::G4PTblDicIterator* theParticleIterator = GetParticleIterator();
	theParticleIterator->reset();
	
	while( (*theParticleIterator)() ){
		
		G4ParticleDefinition *particle = theParticleIterator->value();
		G4ProcessManager *pmanager = particle->GetProcessManager();
		G4String particleName = particle->GetParticleName();

		// Register basic EM processes so charged particles deposit energy
		// (ionisation + multiple scattering). This is required for
		// scintillation to be triggered by energy loss.
		if(particleName == "e-" || particleName == "e+"){
			ph->RegisterProcess(new G4eMultipleScattering(), particle);
			ph->RegisterProcess(new G4eIonisation(), particle);
			if(particleName == "e+") ph->RegisterProcess(new G4eplusAnnihilation(), particle);
		} else if(particleName == "mu-" || particleName == "mu+"){
			ph->RegisterProcess(new G4MuMultipleScattering(), particle);
			ph->RegisterProcess(new G4MuIonisation(), particle);
		} else if(particle->GetPDGCharge() != 0.0 && particleName != "opticalphoton"){
			// generic charged hadrons
			ph->RegisterProcess(new G4hMultipleScattering(), particle);
			ph->RegisterProcess(new G4hIonisation(), particle);
		}
		
		if(particleName == "opticalphoton"){
			
			if(fVerboseLevel>=PhysVerbosity::kInfo){
				G4cout << "Info --> PhysListOptPh::ConstructOptical: Adding DiscreteProcesses for \"opticalphoton\" particle" << G4endl;
			}
			
			
			ph->RegisterProcess(fAbsorptionProcess, particle);
			ph->RegisterProcess(fRayleighScatteringProcess, particle);
			ph->RegisterProcess(fWLSProcess, particle);
			ph->RegisterProcess(fMieHGScatteringProcess, particle);
			ph->RegisterProcess(fBoundaryProcess, particle);
		}

		// register scintillation for all charged particles if enabled
		if(fScintillationEnabled){
			// skip optical photon and neutral particles
			if( particle->GetPDGCharge() != 0.0 && particleName != "opticalphoton" ){
				if(fVerboseLevel>=PhysVerbosity::kInfo){
					G4cout << "Info --> PhysListOptPh::ConstructOptical: Adding Scintillation for \"" << particleName << "\"" << G4endl;
				}
				ph->RegisterProcess(fScintillation, particle);
			}
		}
		//ph->RegisterProcess(fStepLimiter, particle);
		pmanager->AddDiscreteProcess(new G4StepLimiter);
	}
}


// Cuts /////////////////////////////////////////////////////////////////////
void PhysListOptPh::SetCuts()
{
	/*
	if(verboseLevel > 1) G4cout << "PhysList::SetCuts:";

	
	SetCutsWithDefault();

	if(verboseLevel>0) DumpCutValuesTable();
	*/
}

