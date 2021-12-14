//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"
#include <string.h>

//
// TODO:Student Information
//
const char *studentName = "NAME";
const char *studentID   = "PID";
const char *email       = "EMAIL";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History, for gshare, tournament
int lhistoryBits; // Number of bits used for Local History, for tour P1 simple BHT
int pcIndexBits;  // Number of bits used for PC index, for counter and tour P2 correlation, and custon
int choiceBits;   // Number of bits used for PC index, for custom
int bpType;       // Branch Prediction Type
int verbose;

// gshare global
uint32_t gshareHistory;  // history 10101...
uint8_t *gshareBHT;      // length * state, ST | WT | WN | SN

// tournament global
uint32_t tourHistory;    
uint8_t *globalBHT;        // P1, global
uint8_t *localBHT;         // P2, local
uint8_t *localCounter;     // P2, local
uint8_t *chooser;          // 0| 1| 2| 3, each time plus (P1 correct - P2 correct)
uint8_t globalPred, localPred;  // gloabal and local prediction results

// custom global, bi - mode
uint32_t bimodeHistory;
uint8_t *directionBHT1;
uint8_t *directionBHT2;
uint8_t *choiceTable;
uint8_t pred1, pred2; 
uint8_t choosed;


//------------------------------------//
//       Initialize Functions         //
//------------------------------------//

void init_gshare(){
  gshareHistory = 0;
  uint32_t length = 1 << ghistoryBits;  // 2 ^ ghistoryBits combinations
  gshareBHT = malloc(length * sizeof(uint8_t));
  memset(gshareBHT, SN, length * sizeof(uint8_t));
}

void init_tournament(){
  tourHistory = 0;
  chooser      = malloc((1 << ghistoryBits) * sizeof(uint8_t));
  globalBHT    = malloc((1 << ghistoryBits) * sizeof(uint8_t));
  localBHT     = malloc((1 << pcIndexBits)  * sizeof(uint8_t));
  localCounter = malloc((1 << lhistoryBits) * sizeof(uint8_t));
  memset(chooser,      3 , (1 << ghistoryBits) * sizeof(uint8_t));     // choose global when starts
  memset(globalBHT,    WN, (1 << ghistoryBits) * sizeof(uint8_t));
  memset(localBHT,     0 , (1 << pcIndexBits)  * sizeof(uint8_t));     // lhistory starts with 0
  memset(localCounter, WN, (1 << lhistoryBits) * sizeof(uint8_t));
}

void init_custom(){
  bimodeHistory = 0;
  directionBHT1 = malloc((1 << pcIndexBits) * sizeof(uint8_t));
  directionBHT2 = malloc((1 << pcIndexBits) * sizeof(uint8_t));
  choiceTable   = malloc((1 << choiceBits ) * sizeof(uint8_t));
  memset(directionBHT1, WN , (1 << pcIndexBits) * sizeof(uint8_t));
  memset(directionBHT2, WN , (1 << pcIndexBits) * sizeof(uint8_t));
  memset(choiceTable  , 1  , (1 << choiceBits ) * sizeof(uint8_t));  // 0, 1, 2, 3
  // printf("info custom: %d, %d\n", pcIndexBits, choiceBits);
}

// Initialize the predictor
//
void init_predictor(){
  //
  //TODO: Initialize Branch Predictor Data Structures
  init_gshare();
  init_tournament();
  init_custom();
}


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken

uint8_t pred_gshare(uint32_t pc){
  uint32_t mask = (1 << ghistoryBits) - 1;
  uint32_t idx = (pc ^ gshareHistory) & mask;         // index to find an entry in BTH
  if (gshareBHT[idx] == ST || gshareBHT[idx] == WT){
    return TAKEN;
  } else{
    return NOTTAKEN;
  }
}

uint8_t pred_tournament(uint32_t pc){
  uint32_t g_idx = tourHistory & ((1 << ghistoryBits) - 1);
  uint8_t choice = chooser[g_idx];

  // global
  if (globalBHT[g_idx] == ST || globalBHT[g_idx] == WT){
    globalPred = TAKEN;
  } else{
    globalPred = NOTTAKEN;
  }

  // local
  uint32_t pc_idx = pc & ((1 << pcIndexBits) - 1);
  uint32_t lhistory = localBHT[pc_idx];
  if (localCounter[lhistory] == ST || localCounter[lhistory] == WT){
    localPred = TAKEN;
  } else{
    localPred = NOTTAKEN;
  }

  // choose
  if (choice == 0 || choice == 1){
    return localPred;
  } else{
    return globalPred;
  }
}

uint8_t pred_custom(uint32_t pc){
  uint32_t dir_idx = (bimodeHistory ^ pc) & ((1 << pcIndexBits) - 1);
  if (directionBHT2[dir_idx] == ST || directionBHT2[dir_idx] == WT){
    pred2 = TAKEN;
  } else{
    pred2 = NOTTAKEN;
  }
  if (directionBHT1[dir_idx] == ST || directionBHT1[dir_idx] == WT){
    pred1 = TAKEN;
  } else{
    pred1 = NOTTAKEN;
  }

  uint32_t choice_idx = pc & ((1 << choiceBits) - 1);
  if (choiceTable[choice_idx] == 0 || choiceTable[choice_idx] == 1){
    // choose directionBHT 1
    choosed = 1;
    return pred1;
  } 
  else{
    // choose directionBHT 2
    choosed = 2;
    return pred2;
  }
  
}

uint8_t make_prediction(uint32_t pc){
  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return pred_gshare(pc);
      break;
    case TOURNAMENT:
      return pred_tournament(pc);
      break;
    case CUSTOM:
      return pred_custom(pc);
      break;
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

//------------------------------------//
//           Update Predictor         //
//------------------------------------//
// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)

void train_gshare(uint32_t pc, uint8_t outcome){
  uint32_t mask = (1 << ghistoryBits) - 1;
  uint32_t idx = (pc ^ gshareHistory) & mask;
  if (outcome == TAKEN && gshareBHT[idx] != 3){
    gshareBHT[idx] += 1;
  }
  else if (outcome == NOTTAKEN && gshareBHT[idx] != 0){
    gshareBHT[idx] -= 1;
  }
  gshareHistory <<= 1;
  gshareHistory |= outcome;
}

void train_tournament(uint32_t pc, uint8_t outcome){
  // global
  uint32_t g_idx = tourHistory & ((1 << ghistoryBits) - 1);
  if (outcome == TAKEN && globalBHT[g_idx] != 3){
    globalBHT[g_idx] += 1;
  }
  else if (outcome == NOTTAKEN && globalBHT[g_idx] != 0){
    globalBHT[g_idx] -= 1;
  }

  // local
  uint32_t pc_idx = pc & ((1 << pcIndexBits) - 1);
  uint32_t lhistory = localBHT[pc_idx];
  if (outcome == TAKEN && localCounter[lhistory] != 3){
    localCounter[lhistory] += 1;
  }
  else if (outcome == NOTTAKEN && localCounter[lhistory] != 0){
    localCounter[lhistory] -= 1;
  }
  localBHT[pc_idx] <<= 1;
  localBHT[pc_idx] |= outcome;

  // chooser
  uint8_t globalCorrect = (outcome == globalPred) ? 1 : 0;
  uint8_t localCorrect  = (outcome == localPred) ? 1 : 0;
  chooser[g_idx] += (globalCorrect - localCorrect);
  if (chooser[g_idx] > 3) chooser[g_idx] = 3;
  if (chooser[g_idx] < 0) chooser[g_idx] = 0;

  tourHistory <<= 1;
  tourHistory |= outcome;
}

void train_custom(uint32_t pc, uint8_t outcome){
  // direction predictors
  uint32_t dir_idx = (bimodeHistory ^ pc) & ((1 << pcIndexBits) - 1);
  uint8_t pred, oppositePred;
  if (choosed == 1){
    pred = pred1;
    oppositePred = pred2;
    if (outcome == TAKEN && directionBHT1[dir_idx] != 3){
      directionBHT1[dir_idx] += 1;
    }
    else if (outcome == NOTTAKEN && directionBHT1[dir_idx] != 0){
      directionBHT1[dir_idx] -= 1;
    }
  } else{
    pred = pred2;
    oppositePred = pred1;
    if (outcome == TAKEN && directionBHT2[dir_idx] != 3){
      directionBHT2[dir_idx] += 1;
    }
    else if (outcome == NOTTAKEN && directionBHT2[dir_idx] != 0){
      directionBHT2[dir_idx] -= 1;
    }
  }

  // choice predictor
  uint32_t choice_idx = pc & ((1 << choiceBits) - 1);
  if (pred != outcome && oppositePred == outcome){
    if (choosed == 1){ 
      // 1 wrong but 2 correct
      choiceTable[choice_idx] += 1;
      if (choiceTable[choice_idx] > 3) choiceTable[choice_idx] = 3;
    } else{
      // 2 wrong but 1 correct
      choiceTable[choice_idx] -= 1;
      if (choiceTable[choice_idx] < 0) choiceTable[choice_idx] = 0;
    }
  }


  bimodeHistory <<= 1;
  bimodeHistory |= outcome;
}

void train_predictor(uint32_t pc, uint8_t outcome){
  //
  //TODO: Implement Predictor training
  //
  switch (bpType) {
    case GSHARE:
      train_gshare(pc, outcome);
      break;
    case TOURNAMENT:
      train_tournament(pc, outcome);
      break;
    case CUSTOM:
      train_custom(pc, outcome);
      break;
    default:
      break;
  }
}
