#include <string.h>
#include <stdio.h>
#include "fitsio.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>
#include <unistd.h>

using namespace std;

const int kMaxLine   = 2048;
int gVerbosity = 0;

bool fileExist(const char *fileName){
  ifstream in(fileName,ios::in);
  if(in.fail()){
    //cout <<"\nError reading file: " << fileName <<"\nThe file doesn't exist!\n\n";
    in.close();
    return false;
  }
  
  in.close();
  return true;
}

bool readStatsFile(const string &statsFile, map<string,float> &mCards){
  
  mCards.clear();
  ifstream in(statsFile.c_str());
  
  if(!fileExist(statsFile.c_str())){
    cout << "\nError reading stats file: " << statsFile.c_str() <<"\nThe file doesn't exist!\n";
    return false;
  }
  
  while(!in.eof()){
    char line[kMaxLine];
    in.getline(line,kMaxLine);
    
    string lineS(line);
    unsigned int found = lineS.find_first_not_of(" \t\v\r\n");
    if(found == string::npos) continue;
    else if( line[found] == '#') continue;
    
    std::stringstream iss(line);
    string aux;
    int nCols=0;
    while (iss >> aux) ++nCols;
    if(nCols!=2){
      cout << "Error in the number of columns in the stats file.\n";
      in.close();
      return false;
    }
    iss.clear();
    iss.seekg(ios_base::beg);
    
    string cardName = "";
    float value = -1000;
    iss >> cardName >> value;
    std::transform(cardName.begin(), cardName.end(),cardName.begin(), ::toupper);
    
    if(cardName.size()>8){
      cout << " Warnig! The card: " << cardName << " is too long." << flush;
      cout << " Will be trimed to: " << cardName.erase(8) << endl;
    }
    
    if(mCards.count(cardName) != 0){
      cout << " Warnig! The card: " << cardName << " is repeated. Will only use the first one.\n";
    }
    else{
      mCards[cardName] = value;
    }
  }
  in.close();
  
  return true;
}

void printHelp(){
  cout << "\nThis program reads a stats file and a FITS file and rewrites the FITS header.\n";
  cout << "All the non-essential entries are deleted and all the cards in the stats file are added.\n\n";
  cout << "Usage:\n";
  cout << "  writeDamicHeader.exe -s <stats file name> <FITS file>\n\n";
}


int processCommandLineArgs(const int argc, char *argv[], string &fitsFile, string &statsFile){
  
  if(argc == 1){
    printHelp();
    return 1;
  }
  
  bool statsFileFlag = false;
  int opt=0;
  while ( (opt = getopt(argc, argv, "s:vVhH?")) != -1) {
    switch (opt) {
    case 's':
      if(!statsFileFlag){
        statsFile = optarg;
        statsFileFlag = true;
      }
      else{
        cerr << "\nError, can not set more than one stats file!\n\n";
        return 2;
      }
      break;
    case 'V':
    case 'v':
      gVerbosity = 1;
      break;
    case 'h':
    case 'H':
    default: /* '?' */
      printHelp();
      return 1;
    }
  }
  
  if(!statsFileFlag){
    cerr << "\nStats filename missing.\n\n";
    return 2;
  }
  
  if((argc-optind) != 1){
    cerr << "\nError, can not read more than one fits file!\n\n";
    return 2;
  }
  
  fitsFile = argv[optind];
  if(!fileExist(fitsFile.c_str())){
      cout << "\nError reading input file: " << fitsFile <<"\nThe file doesn't exist!\n\n";
      return 2;
  }
  
  return 0;
}


bool keepThisCard(const char *card){
  
  vector<string> cardList;
  cardList.push_back("BZERO   ");
  cardList.push_back("BSCALE  ");
  cardList.push_back("TRIMSEC ");
  cardList.push_back("DATASEC ");
  
  for(unsigned int i=0;i<cardList.size();++i){
    if(strncmp(card,cardList[i].c_str(),8) ==0)
      return true;
  }
  
  return false;
}



int main(int argc, char *argv[])
{
  
  string fitsFile = "";
  string statsFile = "";
  int statusArg = processCommandLineArgs(argc, argv, fitsFile, statsFile);
  
  if(statusArg){
    return statusArg;
  }
  
  map<string,float> mCards;
  readStatsFile(statsFile, mCards);
  
  fitsfile *fptr;         /* FITS file pointer, defined in fitsio.h */
  int status = 0;   /*  CFITSIO status value MUST be initialized to zero!  */
  int iomode;
  
  int nhdu = 0;
    
  iomode = READWRITE;
  fits_open_file(&fptr, fitsFile.c_str(), iomode, &status);
  if (status != 0) return(status);
  fits_get_num_hdus(fptr, &nhdu, &status);
  if (status != 0) return(status);
  
  for (int n=1; n<=nhdu; ++n){  /* Main loop through each extension */
  
    int hdutype;
    fits_movabs_hdu(fptr, n, &hdutype, &status);
    
    int nkeys=0;
    fits_get_hdrspace(fptr, &nkeys, NULL, &status);
    
    /* delete all non-essential entries */
    for (int i = 1; i <= nkeys; ++i) {
      char card[FLEN_CARD];
      fits_read_record(fptr, i, card, &status);

      if (fits_get_keyclass(card) <= TYP_CMPRS_KEY) continue;
      if(keepThisCard(card)) continue;
      
      fits_delete_record(fptr, i,  &status);
      fits_get_hdrspace(fptr, &nkeys, NULL, &status);
      i=1;
    }
    
    { /* Add original hdu key */
      char keyname[] = "OHDU";
      char comment[FLEN_COMMENT] = "Original HDU";
      int datatype = TINT; //TFLOAT
      int value = n;
      fits_update_key(fptr, datatype, keyname, &value,comment, &status);
    }
    
    
    for ( map<string,float>::const_iterator mIt = mCards.begin();mIt != mCards.end(); ++mIt ){
      const char *keyname = mIt->first.c_str();
      char comment[FLEN_COMMENT] = "";
      int datatype = TFLOAT;
      float value = mIt->second;
      fits_update_key(fptr, datatype, keyname, &value,comment, &status);
    }
    
  }
  
  fits_close_file(fptr, &status);
  
  /* if error occured, print out error message */
  if (status) fits_report_error(stderr, status);
  
  return(status);
}

