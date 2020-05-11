#include "DetConstr.hh"
#include "DetectorMessenger.hh"
//#include "SensitiveDetectors.hh"

#include "globals.hh"
#include "G4SystemOfUnits.hh"

#include "G4Material.hh"
#include "G4NistManager.hh"

#include "G4SDManager.hh"
#include "G4ThreeVector.hh"
#include "G4RotationMatrix.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"

#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Cons.hh"
#include "G4EllipticalCone.hh"
#include "G4Trd.hh"
#include "G4Sphere.hh"
#include "G4Torus.hh"
#include "G4Polyhedra.hh"
#include "G4Polycone.hh"
#include "G4Ellipsoid.hh"
#include "G4ExtrudedSolid.hh"
#include "G4UnionSolid.hh"
#include "G4SubtractionSolid.hh"
#include "G4IntersectionSolid.hh"

#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4PVParameterised.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4LogicalVolumeStore.hh"

#include "G4OpBoundaryProcess.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4LogicalSkinSurface.hh"


#include <vector>
#include <map>
#include <numeric>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cassert>

using std::vector;
using std::map;
using std::stringstream;
using std::max;
using std::ifstream;
using std::ofstream;


//map<G4String, G4double> DetConstrOptPh::fGeometryParams;


/////////////////////////////////////////////////////////////////////////////////////////
// constructor
DetConstrOptPh::DetConstrOptPh(G4String gdmlfilename)
{
	fWorld = NULL;
	fDetectorMessenger = NULL;
	
	fVerbose = DetConstrOptPh::kSilent;
	
	fGDMLParser = new G4GDMLParser;
	
	{
		ifstream gdmlfile(gdmlfilename.c_str());
		if(gdmlfile){
			fGDMLParser->Read(gdmlfilename);
			fWorld = fGDMLParser->GetWorldVolume();
		}
	}
	
	if(fWorld){
		fDetectorMessenger = new DetectorMessenger(this);
	}
	
	fOptPropManager = OptPropManager::GetInstance();
	fOptPropManager->SetDetConstr(this);
	
	if(!fOptPropManager){
		G4Exception("DetConstrOptPh::DetConstrOptPh(...)","Geom.001", FatalException,"Cannot get \"OptPropManager\" pointer.");
	}
	
	fOptSurfTab = G4SurfaceProperty::GetSurfacePropertyTable();
	
	fTpbThick = 0.001*mm; //Default value. Can be changed by a user command
}


/////////////////////////////////////////////////////////////////////////////////////////
// destructor
DetConstrOptPh::~DetConstrOptPh()
{
	if(fDetectorMessenger) delete fDetectorMessenger;
	delete fGDMLParser;
}


/////////////////////////////////////////////////////////////////////////////////////////
G4VPhysicalVolume *DetConstrOptPh::Construct()
{
	if(fVerbose>=DetConstrOptPh::kDebug) G4cout << "Debug --> DetConstrOptPh::Construct(): Entering the function."<<G4endl;
	if(fWorld){
		if( G4PhysicalVolumeStore::GetInstance()->GetVolume("volTPB_LAr_PV") ){
			BuildTPBlayer();
		}
		
		std::map<std::string, std::vector<G4VPhysicalVolume*> >::iterator iT;
		
		fPVolsMap.clear();
		
		if(fWorld){
			ScanVols(fWorld);
		}
		
		BuildDefaultOptSurf();
		BuildDefaultLogSurfaces();
		SetDefaultOptProperties();
	}
	
	if(fVerbose>=DetConstrOptPh::kInfo) G4cout << "Info --> DetConstrOptPh::Construct(): Finished construction "<<G4endl;
	
	
	if(fVerbose>=DetConstrOptPh::kDebug) G4cout << "Debug --> DetConstrOptPh::Construct(): Exiting the function."<<G4endl;
	return fWorld;
}


/////////////////////////////////////////////////////////////////////////////////////////
void DetConstrOptPh::BuildTPBlayer()
{
	
	if(fVerbose>=DetConstrOptPh::kDebug){
		G4cout << "Debug --> DetConstrOptPh::BuildTPBlayer(): Entering the function." << G4endl;
	}
	
	G4VPhysicalVolume* layerVol = G4PhysicalVolumeStore::GetInstance()->GetVolume("volTPB_LAr_PV");
	
	G4Box *layerGeom = dynamic_cast<G4Box*>( layerVol->GetLogicalVolume()->GetSolid() );
	
	if(!layerGeom){
		G4cout << "\nERROR --> DetConstrOptPh::BuildTPBlayer(): Cannot find the LAr layer volume <volTPB_LAr_PV>, where the TPB layer will be placed. TPB layer for ArCLight will not be build!\n" << G4endl;
		return;
	}
	
	if(fTpbThick<=0.){
		G4cout << "WARNING --> DetConstrOptPh::BuildTPBlayer(): the thickness of the TPB layer is not set to a positive value. TPB layer for ArCLight will not be build!" << G4endl;
		return;
	}
	
	
	//G4double layerDx = 280.255*mm;
	//G4double layerDy = 300.254*mm;
	//G4double layerDz = 0.01*mm;
	
	G4double layerDx_hl = layerGeom->GetXHalfLength();
	G4double layerDy_hl = layerGeom->GetYHalfLength();
	G4double layerDz_hl = layerGeom->GetZHalfLength();
	
	if(fTpbThick>2*layerDz_hl){
		G4cout << "\nERROR --> DetConstrOptPh::BuildTPBlayer(): The TPB layer thickness (" << fTpbThick << ") is larger than that of the LAr layer volume <volTPB_LAr_PV> (" << 2*layerDz_hl << "), where the TPB layer will be placed. TPB layer for ArCLight will not be build!\n" << G4endl;
		return;
	}
	
	
	G4Material* tpbMat = FindMaterial("TPB");
	if(!tpbMat){
		tpbMat = FindMaterial("LAr");
		G4cout << "WARNING --> DetConstrOptPh::BuildTPBlayer(): Building the TPB material as a copy of LAr material, with different name." << G4endl;
		if(!tpbMat){
			G4cout << "\nERROR --> DetConstrOptPh::BuildTPBlayer(): LAr material not found!. TPB layer for ArCLight will not be build, but this might be a major problem for the whole simulation!\n" << G4endl;
			return;
		}
		//Build the tpb as LAr copy, but change the name only. Only optical properties matter.
		tpbMat = new G4Material("TPB", tpbMat->GetDensity(), tpbMat, kStateSolid, 87.0);
	}
	
	G4Box *tpbGeom = new G4Box("tpbGeom",layerDx_hl,layerDy_hl,fTpbThick/2.);
	
	G4LogicalVolume* tpbLog= new G4LogicalVolume(tpbGeom,tpbMat,"volTPB");
	
	new G4PVPlacement(NULL, G4ThreeVector(0.,0., -layerDz_hl+fTpbThick/2.), tpbLog, "volTPB_PV", layerVol->GetLogicalVolume(), false, 0);
	
	
	if(fVerbose>=DetConstrOptPh::kDebug){
		G4cout << "Debug --> DetConstrOptPh::BuildTPBlayer(): TPB layer buit! Exiting the function." << G4endl;
	}
}


/////////////////////////////////////////////////////////////////////////////////////////
void DetConstrOptPh::BuildDefaultOptSurf()
{
	G4OpticalSurface* LAr2TPB_optsurf = new G4OpticalSurface("LAr2TPB_optsurf", unified, ground, dielectric_dielectric);
	LAr2TPB_optsurf -> SetMaterialPropertiesTable( new G4MaterialPropertiesTable() );
	
	G4OpticalSurface* TPB2LAr_optsurf = new G4OpticalSurface("TPB2LAr_optsurf", unified, ground, dielectric_dielectric);
	TPB2LAr_optsurf -> SetMaterialPropertiesTable( new G4MaterialPropertiesTable() );
	
	
	//Make the optical surface from EJ280 WLS to TPB
	G4OpticalSurface* TPB2EJ280_optsurf = new G4OpticalSurface("TPB2EJ280_optsurf", unified, polished, dielectric_dielectric);
	TPB2EJ280_optsurf -> SetMaterialPropertiesTable( new G4MaterialPropertiesTable() );


	//Make the optical surface from TPB to EJ280 WLS (trapping)
	G4OpticalSurface* EJ2802TPB_optsurf = new G4OpticalSurface("EJ2802TPB_optsurf", unified, polished, dielectric_metal);
	EJ2802TPB_optsurf -> SetMaterialPropertiesTable( new G4MaterialPropertiesTable() );
	
	
	G4OpticalSurface* EJ2802LAr_optsurf = new G4OpticalSurface("EJ2802LAr_optsurf", unified, polished, dielectric_metal);
	EJ2802LAr_optsurf -> SetMaterialPropertiesTable( new G4MaterialPropertiesTable() );
	
	
	G4OpticalSurface* EJ2802ESR_optsurf = new G4OpticalSurface("EJ2802ESR_optsurf", unified, polished, dielectric_metal);
	EJ2802ESR_optsurf -> SetMaterialPropertiesTable( new G4MaterialPropertiesTable() );
	
	
	G4OpticalSurface* EJ2802SiPM_optsurf = new G4OpticalSurface("EJ2802SiPM_optsurf", unified, polished, dielectric_metal);
	EJ2802SiPM_optsurf -> SetMaterialPropertiesTable( new G4MaterialPropertiesTable() );
}


/////////////////////////////////////////////////////////////////////////////////////////
void DetConstrOptPh::BuildDefaultLogSurfaces()
{
	if(fVerbose>=DetConstrOptPh::kDebug) G4cout << "Debug --> DetConstrOptPh::BuildDefaultLogSurfaces(): Entering the function."<<G4endl;
	//By default the EJ28 WLS does't have optical properties.
	//They can be defined later
	
	
	// --------------------------------------//
	//  Interface between LAr and TPB layer  //
	// --------------------------------------//
	
	//By the default the surfaces are with ground finish
	//The reflectivity is calculated by the Fresnel law
	//No reflection properties are defined ==> lambertian reflection is selected in this default
	
	fOptPropManager->BuildLogicalBorderSurface("LAr2TPB_logsurf", "volTPB_LAr_PV", "volTPB_PV", "LAr2TPB_optsurf");
	fOptPropManager->BuildLogicalBorderSurface("TPB2LAr_logsurf", "volTPB_PV", "volTPB_LAr_PV", "TPB2LAr_optsurf");
	
	
	
	// --------------------------------------//
	//  Interface between TPB and EJ280 WLS  //
	// --------------------------------------//
	fOptPropManager->BuildLogicalBorderSurface("TPB2EJ280_logsurf", "volTPB_PV", "volWLS_PV", "TPB2EJ280_optsurf");
	fOptPropManager->BuildLogicalBorderSurface("EJ2802TPB_logsurf", "volWLS_PV", "volTPB_PV", "EJ2802TPB_optsurf");
	
	
	
	// -----------------------------------------------------------//
	//        Interface between EJ280 WLS and volTPB_LAr_PV       //
	//  This is used only in the case the TPB layer is not built  //
	// -----------------------------------------------------------//
	fOptPropManager->BuildLogicalBorderSurface("EJ2802LArTPB_logsurf", "volWLS_PV", "volTPB_LAr_PV", "EJ2802LAr_optsurf");
	
	
	
	// -----------------------------------------//
	//  Interface between EJ280 WLS and Mirror  //
	// -----------------------------------------//
	
	fOptPropManager->BuildLogicalBorderSurface("EJ2802OptDet_logsurf", "volWLS_PV", "volOpticalDet_PV", "EJ2802ESR_optsurf");
	fOptPropManager->BuildLogicalBorderSurface("EJ2802SiPmMask_logsurf", "volWLS_PV", "volSiPM_Mask_PV", "EJ2802ESR_optsurf");
	fOptPropManager->BuildLogicalBorderSurface("EJ2802ArCLight_logsurf", "volWLS_PV", "volArCLight_PV", "EJ2802ESR_optsurf");
	fOptPropManager->BuildLogicalBorderSurface("EJ2802LAr_logsurf", "volWLS_PV", "volLAr_PV", "EJ2802ESR_optsurf");
	
	
	
	// ----------------------------------------//
	//  Interface between EJ280 WLS and SiPMs  //
	// ----------------------------------------//
	
	fOptPropManager->BuildLogicalBorderSurface("EJ2802SiPM0_logsurf", "volWLS_PV", "volSiPM_0_PV", "EJ2802SiPM_optsurf");
	fOptPropManager->BuildLogicalBorderSurface("EJ2802SiPM1_logsurf", "volWLS_PV", "volSiPM_1_PV", "EJ2802SiPM_optsurf");
	fOptPropManager->BuildLogicalBorderSurface("EJ2802SiPM2_logsurf", "volWLS_PV", "volSiPM_2_PV", "EJ2802SiPM_optsurf");
	fOptPropManager->BuildLogicalBorderSurface("EJ2802SiPM3_logsurf", "volWLS_PV", "volSiPM_3_PV", "EJ2802SiPM_optsurf");
	fOptPropManager->BuildLogicalBorderSurface("EJ2802SiPM4_logsurf", "volWLS_PV", "volSiPM_4_PV", "EJ2802SiPM_optsurf");
	fOptPropManager->BuildLogicalBorderSurface("EJ2802SiPM5_logsurf", "volWLS_PV", "volSiPM_5_PV", "EJ2802SiPM_optsurf");
	
	// -----------------------------------------//
	//  Interface between Fibre and LAr  //
	// -----------------------------------------//
	
	// LogSurface between Fibres and LAr (using same as EJ280 to ESR)
	fOptPropManager->BuildLogicalBorderSurface("Fib2LCM_logsurf","volFibre_PV","volLCM_PV","EJ2802ESR_optsurf");
	//fOptPropManager->BuildLogicalBorderSurface("Fib2LCM_logsurf","volFibre_PV","volLCM_PV","EJ2802ESR_optsurf");
	
	// LogSurface between Fibres and SiPMs
	fOptPropManager->BuildLogicalBorderSurface("Fibre2SiPM0_logsurf","volFibre_PV","volSiPM_LCM_0_PV","EJ2802SiPmLCM_optsurf");
	fOptPropManager->BuildLogicalBorderSurface("Fibre2SiPM1_logsurf","volFibre_PV","volSiPM_LCM_1_PV","EJ2802SiPmLCM_optsurf");
	fOptPropManager->BuildLogicalBorderSurface("Fibre2SiPM2_logsurf","volFibre_PV","volSiPM_LCM_2_PV","EJ2802SiPmLCM_optsurf");
	fOptPropManager->BuildLogicalBorderSurface("Fibre2SiPM3_logsurf","volFibre_PV","volSiPM_LCM_3_PV","EJ2802SiPmLCM_optsurf");
	fOptPropManager->BuildLogicalBorderSurface("Fibre2SiPM4_logsurf","volFibre_PV","volSiPM_LCM_4_PV","EJ2802SiPmLCM_optsurf");
	fOptPropManager->BuildLogicalBorderSurface("Fibre2SiPM5_logsurf","volFibre_PV","volSiPM_LCM_5_PV","EJ2802SiPmLCM_optsurf");
	
	
	if(fVerbose>=DetConstrOptPh::kDebug) G4cout << "Debug --> DetConstrOptPh::BuildDefaultOpticalSurfaces(): Exiting the function."<<G4endl;
}


/////////////////////////////////////////////////////////////////////////////////////////
void DetConstrOptPh::SetDefaultOptProperties()
{
	if(fVerbose>=DetConstrOptPh::kDebug) G4cout << "Debug --> DetConstrOptPh::SetDefaultOptProperties(): Entering the function."<<G4endl;
	
	if(!fOptPropManager){
		G4Exception("DetConstrOptPh::DefaultOptProperties()","Geom.002", FatalException,"\"OptPropManager\" pointer is null.");
	}
	
	if(fVerbose>=DetConstrOptPh::kDebug) G4cout << "Debug --> DetConstrOptPh::SetDefaultOptProperties(): Exiting the function."<<G4endl;
	
}


/////////////////////////////////////////////////////////////////////////////////////////
void DetConstrOptPh::PrintVolumeCoordinates(G4String hVolName)
{
	if(hVolName==G4String("")){
		G4cout << "\nERROR --> DetConstrOptPh::PrintVolumeCoordinates(G4String hVolName): Physical volume name not set!!!" << G4endl;
		return;
	}
	
	G4PhysicalVolumeStore* pPhysVolStore = G4PhysicalVolumeStore::GetInstance();
	
	G4int Nvolumes = pPhysVolStore->size();
	
	//Take the physical volume pointer you want
	std::vector<G4VPhysicalVolume *> vPhysVols;
	for(G4int i=0; i<Nvolumes; i++){
		G4String tmp_name = pPhysVolStore->at(i)->GetName();
		if(tmp_name==hVolName){
			vPhysVols.push_back(pPhysVolStore->at(i));
		}
	}
	
	if(vPhysVols.size()==0){
		G4cout << "Physical Volume \"" << hVolName << "\" not found!!!" << G4endl;
		return;
	}else{
		G4cout << "There are " << vPhysVols.size() << " instances of the physical volume \"" << hVolName << "\"" << G4endl;
	}
	
	G4cout << G4endl << G4endl;
	
	/*
	G4double mass = (vPhysVols..at(0)->GetLogicalVolume()->GetMass(false,false))/kg;
	G4double density = (pPhysVol->GetLogicalVolume()->GetMaterial()->GetDensity())/(kg/m3);
	G4double volume = mass/density;
	G4cout << "Mass of physical volume \"" << pPhysVol->GetName() << "\" = " << mass << " kg" << G4endl;
	G4cout << "Volume of physical volume \"" << pPhysVol->GetName() << "\" = " << volume << " m^3" << G4endl;
	G4cout << G4endl << G4endl;
	*/
	
	for(G4int iVol=0; iVol<vPhysVols.size(); iVol++){
		G4cout << "\n\nInstance " << iVol << ":" << G4endl;
		
		G4VPhysicalVolume *pPhysVol = vPhysVols.at(iVol);
		
		G4ThreeVector ShiftGlob = pPhysVol->GetTranslation();
		
		while( pPhysVol->GetMotherLogical() ){
			//Find the mother phys volume
			G4VPhysicalVolume *pAncPhysVol;
			
			for(G4int i=0; i<Nvolumes; i++){
				if(pPhysVolStore->at(i)->GetLogicalVolume()->IsDaughter(pPhysVol)){
					pAncPhysVol = pPhysVolStore->at(i);
				}
			}
			G4cout << "Shift of physical volume \"" << pPhysVol->GetName() << "\" with respect to \"" << pAncPhysVol->GetName() << "\" =  " << pPhysVol->GetTranslation()/mm << " mm" << G4endl;
			pPhysVol = pAncPhysVol;
			ShiftGlob = ShiftGlob + pPhysVol->GetTranslation();
		}
		
		G4cout << "\nGlobal shift of physical volume \"" << hVolName << "\" (inst. " << iVol << ") =  " << ShiftGlob/mm << " mm" << G4endl;
		
	}
	
	return;
}


/////////////////////////////////////////////////////////////////////////////////////////
void DetConstrOptPh::PrintListOfPhysVols()
{
	if(!fWorld) return;
	
	G4PhysicalVolumeStore* pPhysVolStore = G4PhysicalVolumeStore::GetInstance();
	
	if(!pPhysVolStore) return;
	
	G4int nvols = pPhysVolStore->size();
	
	
	G4cout << "\nList of Physical Volumes (PV, LV, MV)" << G4endl;
	
	for(G4int ivol=0; ivol<nvols; ivol++){
		G4LogicalVolume *pMotherVol = pPhysVolStore->at(ivol)->GetMotherLogical();
		if(pMotherVol){
			G4cout << "PV: " << pPhysVolStore->at(ivol)->GetName();
			if( pPhysVolStore->at(ivol)->IsReplicated() && pPhysVolStore->at(ivol)->IsParameterised()){
				G4cout << " (repl, param)";
			}else{
				if(pPhysVolStore->at(ivol)->IsReplicated()){
					G4cout << " (repl)";
				}
				if(pPhysVolStore->at(ivol)->IsParameterised()){
					G4cout << " (param)";
				}
			}
			G4cout << "\tLV: " << pPhysVolStore->at(ivol)->GetLogicalVolume()->GetName() << "\tMV: " << pPhysVolStore->at(ivol)->GetMotherLogical()->GetName() << G4endl;
		}else{
			G4cout << "PV: " << pPhysVolStore->at(ivol)->GetName() << "\tLV: " << pPhysVolStore->at(ivol)->GetLogicalVolume()->GetName() << "\tMV: None" << G4endl;
		}
	}
	
	G4cout << "\nTotal number of physical volumes registered: " << nvols << G4endl;
	
	
}


/////////////////////////////////////////////////////////////////////////////////////////
void DetConstrOptPh::PrintListOfLogVols()
{
	if(!fWorld) return;
	
	G4LogicalVolumeStore* pLogVolStore = G4LogicalVolumeStore::GetInstance();
	
	if(!pLogVolStore) return;
	
	G4int nvols = pLogVolStore->size();
	
	
	G4cout << "\nList of Logical Volumes (LV, Material)" << G4endl;
	
	for(G4int ivol=0; ivol<nvols; ivol++){
		G4cout << "LG: " << pLogVolStore->at(ivol)->GetName() << "\tMaterial: " << pLogVolStore->at(ivol)->GetMaterial()->GetName() << G4endl;
	}
	
	G4cout << "\nTotal number of logical volumes registered: " << nvols << G4endl;
	
	
}


G4Material* DetConstrOptPh::FindMaterial(G4String matname)
{
	
	G4MaterialTable *tab = G4Material::GetMaterialTable();
	
	if(!tab) return NULL;
	
	G4MaterialTable::iterator iT;
	for(iT=tab->begin(); iT!=tab->end(); ++iT){
		if( ((*iT)->GetName())==matname ) return (*iT);
	}
	
	//Here I did not find the material ==> returning NULL
	return NULL;
	
}


/////////////////////////////////////////////////////////////////////////////////////////
void DetConstrOptPh::ScanVols(G4VPhysicalVolume* mvol, std::map<G4String, std::set<G4VPhysicalVolume*> > *volsmap)
{
	if(!mvol) return;
	
	//This flag will be used to delete the volsmap instance and to copy the volumes of the world tree to the fPVolsMap object
	bool isRootVol = false;
	if(!volsmap){
		isRootVol = true;
		volsmap = new std::map<G4String, std::set<G4VPhysicalVolume*> >();
	}
	
	if( volsmap->find(mvol->GetName()) == volsmap->end() ){
		std::set<G4VPhysicalVolume*> volSet;
		volSet.insert(mvol);
		(*volsmap)[mvol->GetName()] = volSet;
	}else{
		((*volsmap)[mvol->GetName()]).insert(mvol);
	}
	
	G4int nDVols = mvol->GetLogicalVolume()->GetNoDaughters();
	
	if(fVerbose>=DetConstrOptPh::kDetails){
		G4cout << "Detail --> Scanning dauters of volume <" << mvol->GetName() << ">:" << G4endl;
		
		std::map<std::string, G4int> vols_map;
		
		for(G4int iVol=0; iVol<nDVols; iVol++){
			G4VPhysicalVolume* dVol = mvol->GetLogicalVolume()->GetDaughter(iVol);
			
			if(dVol){
				std::string name = dVol->GetName();
				if( vols_map.find(name) == vols_map.end() ){
					vols_map[name] = 1;
				}else{
					vols_map[name] += 1;
				}
			}
		}
	
		if(vols_map.size()==0){
			G4cout << "  No daughter volumes.\n" << G4endl;
		}else{
			std::map<std::string, int>::iterator it1;
			for(it1=vols_map.begin(); it1!=vols_map.end(); ++it1){
				G4cout << "  Volume <" << it1->first << ">, copies: " << it1->second << G4endl;
			}
			G4cout << G4endl;
		}
	}
	
	
	for(G4int iVol=0; iVol<nDVols; iVol++){
		if( mvol->GetLogicalVolume()->GetDaughter(iVol) ) ScanVols( mvol->GetLogicalVolume()->GetDaughter(iVol), volsmap );
	}
	
	if(isRootVol){
		//Only in this case I copy all the found 
		std::map<G4String, std::set<G4VPhysicalVolume*> >::iterator mpIt;
		for(mpIt=volsmap->begin(); mpIt!=volsmap->end(); ++mpIt){
			fPVolsMap[mpIt->first] = std::vector<G4VPhysicalVolume*>( (mpIt->second).begin(), (mpIt->second).end() );
		}
		
		
		delete volsmap;
	}
}


const std::vector<G4VPhysicalVolume* >* DetConstrOptPh::GetPvList(G4String pvname) const
{
	PVmap::const_iterator it = fPVolsMap.find(pvname);
	if( it==fPVolsMap.end() ) return NULL;
	
	const std::vector<G4VPhysicalVolume* > *vp = &(it->second);
	
	return vp;
}



