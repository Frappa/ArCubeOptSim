
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "PhysListMessenger.hh"
#include "PhysList.hh"

#include "G4UIdirectory.hh"
#include "G4UIcmdWithAnInteger.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PhysListOptPhMessenger::PhysListOptPhMessenger(PhysListOptPh* pPhys):
  G4UImessenger(), fPhysicsList(pPhys)
{
  fUiDir = new G4UIdirectory("/argoncube/physics/");
  fUiDir->SetGuidance("UI commands for ArgonCube physics list setup");

  fOptPhPhysDir = new G4UIdirectory("/argoncube/physics/optical/");
  fOptPhPhysDir->SetGuidance("Optical Physics List control ");
 
  fVerboseCmd = new G4UIcmdWithAnInteger("/argoncube/physics/optical/verbose",this);
  fVerboseCmd->SetGuidance("set verbose for optical physics processes");
  fVerboseCmd->SetParameterName("verbose",true);
  fVerboseCmd->SetDefaultValue(1);
  fVerboseCmd->SetRange("verbose>=0");
  fVerboseCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PhysListOptPhMessenger::~PhysListOptPhMessenger()
{
  delete fVerboseCmd;
  delete fOptPhPhysDir;
  delete fUiDir;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PhysListOptPhMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
  if( command == fVerboseCmd )
   {fPhysicsList->SetVerboseLevel(fVerboseCmd->GetNewIntValue(newValue));}
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
