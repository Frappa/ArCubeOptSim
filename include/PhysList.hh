#ifndef __PHYS_LIST_HH__
#define __PHYS_LIST_HH__

#include <G4VUserPhysicsList.hh>
#include <globals.hh>


//Forward declarations
class G4StepLimiter;
class G4Scintillation;
class G4OpAbsorption;
class G4OpRayleigh;
class G4OpWLS;
class G4OpMieHG;
class G4OpBoundaryProcess;

//class G4ProductionCuts;

class PhysListOptPhMessenger;

enum class PhysVerbosity{kSilent, kInfo, kDetails, kDebug};

class PhysListOptPh: public G4VUserPhysicsList
{
	PhysListOptPhMessenger* fMessenger;
	
	PhysVerbosity fVerboseLevel;

	// Enable or disable scintillation generation (default: enabled)
	G4bool fScintillationEnabled;
	
	//All the static declaration here are made for speed and low memory consumption in addition they are thread safe using the G4 internal functionalities
	//Note: All this static members are not shared between different threads but shared between different instances of the class in the same thread
	
	static G4ThreadLocal G4StepLimiter *fStepLimiter;
	static G4ThreadLocal G4OpAbsorption* fAbsorptionProcess;
	static G4ThreadLocal G4OpRayleigh* fRayleighScatteringProcess;
	static G4ThreadLocal G4OpWLS* fWLSProcess;
	static G4ThreadLocal G4OpMieHG* fMieHGScatteringProcess;
	static G4ThreadLocal G4OpBoundaryProcess* fBoundaryProcess;
    static G4ThreadLocal G4Scintillation* fScintillation;
	
	G4double fDefaultCutValue;
	
	
public:
	PhysListOptPh();
	virtual ~PhysListOptPh();
	
	void SetCuts();
	
	virtual void ConstructParticle();
	virtual void ConstructProcess();
	
	//for the Messenger
	void SetVerbose(PhysVerbosity verb){ fVerboseLevel = verb; };
	void SetScintillationEnabled(G4bool enabled){ fScintillationEnabled = enabled; };
	

private:
	
	void ConstructMyBosons();
	void ConstructOptical();
	
	
	
};

#endif

