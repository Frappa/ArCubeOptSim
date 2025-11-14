#ifndef __PHYS_LIST_MESSENGER_HH__
#define __PHYS_LIST_MESSENGER_HH__ 1

#include "globals.hh"
#include "G4UImessenger.hh"

class PhysListOptPh;
class G4UIdirectory;
class G4UIcmdWithAnInteger;
class G4UIcmdWithABool;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class PhysListOptPhMessenger: public G4UImessenger
{
  public:
    PhysListOptPhMessenger(PhysListOptPh* PhysList);
    virtual ~PhysListOptPhMessenger();
 
    virtual void SetNewValue(G4UIcommand* command, G4String newValue);
 
  private:
    PhysListOptPh*  fPhysicsList;
 
    G4UIdirectory*        fPhysDir;
    G4UIdirectory*        fOptPhPhysDir;
    G4UIcmdWithAnInteger* fVerboseCmd;
    G4UIcmdWithABool*     fScintCmd;
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
