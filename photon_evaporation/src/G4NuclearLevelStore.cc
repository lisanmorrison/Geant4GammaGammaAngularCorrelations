//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
// $Id: G4NuclearLevelStore.cc 67983 2013-03-13 10:42:03Z gcosmo $
//
// 06-10-2010 M. Kelsey -- Drop static data members.
// 17-11-2010 V. Ivanchenko - make as a classical singleton. 
// 17-10-2011 V. L. Desorgher - allows to define separate datafile for 
//               given isotope
// 06-01-2012 V. Ivanchenko - cleanup the code; new method GetLevelManager  
//

#include "G4NuclearLevelStore.hh"
#include <sstream>
#include <fstream>

G4ThreadLocal G4NuclearLevelStore* G4NuclearLevelStore::theInstance = 0;

G4NuclearLevelStore* G4NuclearLevelStore::GetInstance()
{
  if(!theInstance) {
    static G4ThreadLocal G4NuclearLevelStore *store_G4MT_TLS_ = 0 ; if (!store_G4MT_TLS_) store_G4MT_TLS_ = new  G4NuclearLevelStore  ;  G4NuclearLevelStore &store = *store_G4MT_TLS_;
    theInstance = &store;
  }
  return theInstance;
}

G4NuclearLevelStore::G4NuclearLevelStore()
{
  userFiles = false;
  deconstructMultipole = false; // Evan Rand
  char* env = getenv("G4LEVELGAMMADATA");
  if (env == 0) 
    {
      G4cout << "G4NuclarLevelStore: please set the G4LEVELGAMMADATA environment variable\n";
      dirName = "";
    }
  else
    {
      dirName = env;
      dirName += '/';
    }
}

G4NuclearLevelStore::~G4NuclearLevelStore()
{
  ManagersMap::iterator i;
  for (i = theManagers.begin(); i != theManagers.end(); ++i)
    { delete i->second; }
  MapForHEP::iterator j;
  for (j = managersForHEP.begin(); j != managersForHEP.end(); ++j)
    { delete j->second; }
  if(userFiles) {
    std::map<G4int, G4String>::iterator k;
    for (k = theUserDataFiles.begin(); k != theUserDataFiles.end(); ++k)
      { delete k->second; }
  }
  // Evan Rand
  if(deconstructMultipole) {
    std::map<G4int, G4String>::iterator l;
    for (l = theUserDataFilesMultipole.begin(); l != theUserDataFilesMultipole.end(); ++l)
      { delete l->second; }
  }
}

void 
G4NuclearLevelStore::AddUserEvaporationDataFile(G4int Z, G4int A,
						const G4String& filename)
{ 
  if (Z<1 || A<2) {
    G4cout<<"G4NuclearLevelStore::AddUserEvaporationDataFile "
	  <<" Z= " << Z << " and A= " << A << " not valid!"<<G4endl;
  }

  std::ifstream DecaySchemeFile(filename);
  if (DecaySchemeFile){
    G4int ID_ion=Z*1000+A;//A*1000+Z;
    theUserDataFiles[ID_ion]=filename;
    userFiles = true;
  }
  else {
    G4cout<<"The file "<<filename<<" does not exist!"<<G4endl;
  }
}

// Evan Rand - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void G4NuclearLevelStore::AddUserMultipoleDataFile(G4int Z, G4int A,G4String filename)
{
    if (Z<1 || A<2) {
      G4cout<<"G4NuclearLevelStore::AddUserMultipoleDataFile "
        <<" Z= " << Z << " and A= " << A << " not valid!"<<G4endl;
    }

    std::ifstream DecaySchemeFileMultipole(filename);
    if (DecaySchemeFileMultipole){
      G4int ID_ion=Z*1000+A;//A*1000+Z;
      theUserDataFilesMultipole[ID_ion]=filename;
      userFilesMultipole[ID_ion] = true;
      deconstructMultipole = true;
    }
    else {
      G4cout<<"The file "<<filename<<" does not exist!"<<G4endl;
    }
}

G4bool G4NuclearLevelStore::GetUserFilesMultipole(G4int Z, G4int A)
{
    // Generate the key = filename
    G4int key = Z*1000+A; //GenerateKey(Z,A);
    return userFilesMultipole[key];
}

void G4NuclearLevelStore::SetGroundStateSpinAngularMomentum(G4ThreeVector value)
{
    G4int Z = (G4int)value.x();
    G4int A = (G4int)value.y();
    G4double GS = value.z();
    G4int key = Z*1000+A; //GenerateKey(Z,A);
    groundStateSpinAngularMomentum[key] = GS;
    boolGroundStateSpinAngularMomentum[key] = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

G4String 
G4NuclearLevelStore::GenerateFileName(G4int Z, G4int A) const 
{
  std::ostringstream streamName; 
  streamName << 'z' << Z << ".a" << A;
  G4String name(streamName.str());
  return name;
}

G4NuclearLevelManager* 
G4NuclearLevelStore::GetManager(G4int Z, G4int A) 
{
  G4NuclearLevelManager * result = 0; 
  if (A < 1 || Z < 1 || A < Z)
    {
      //G4cerr << "G4NuclearLevelStore::GetManager: Wrong values Z = " << Z 
      //	       << " A = " << A << '\n';
      return result;
    }

  // Generate the key = filename
  G4int key = Z*1000+A; //GenerateKey(Z,A);
    
  // Check if already exists that key
  ManagersMap::iterator idx = theManagers.find(key);
  // If doesn't exists then create it
  if ( idx == theManagers.end() )
    {
      G4String file = dirName + GenerateFileName(Z,A);

      //Check if data have been provided by the user
      if(userFiles) {
    G4String file1 = theUserDataFiles[key];//1000*A+Z];
	if (file1 != "") { file = file1; }
      }
      result = new G4NuclearLevelManager(Z,A,file);
      theManagers.insert(std::make_pair(key,result));
      // Evan Rand - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      G4double gsSpin = 0;
      if(userFilesMultipole[key]) {
          if(boolGroundStateSpinAngularMomentum[key]) {
              gsSpin = groundStateSpinAngularMomentum[key];
          }
          else {
            if(A%2 == 0) { // even-A, default is 0
                gsSpin = 0.0;
            }
            else { // odd-A, defaul is 1/2;
                gsSpin = 0.5;
            }
          }
          result->GroundStateSpinAngularMomentum(gsSpin);
          G4String file2 = theUserDataFilesMultipole[key];//1000*A+Z];
          if (file2 != "") { file = file2; }
          G4double num = result->NumberOfLevels();
          result->AddMulipole(Z,A,file);
      }
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    }
  // But if it exists...
  else
    {
      result = idx->second;
    }
    
  return result; 
}

G4LevelManager* 
G4NuclearLevelStore::GetLevelManager(G4int Z, G4int A) 
{
  G4LevelManager * result = 0; 
  // Generate the key = filename
  G4int key = Z*1000+A; 
    
  // Check if already exists that key
  MapForHEP::iterator idx = managersForHEP.find(key);
  // If doesn't exists then create it
  if ( idx == managersForHEP.end() ) {
    result = new G4LevelManager(Z,A,reader,
				dirName + GenerateFileName(Z,A));
    managersForHEP.insert(std::make_pair(key,result));

    // it exists
  } else {
    result = idx->second;
  }
    
  return result; 
}
