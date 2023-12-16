//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <math.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Jiahao Yang";
const char *studentID = "A59024493";
const char *email = "jiy083@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

// define number of bits required for indexing the BHT here.
int ghistoryBits = 14; // Number of bits used for Global History
int bpType;            // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
// TODO: Add your own Branch Predictor data structures here
//
// gshare
uint8_t *bht_gshare;
uint64_t ghistory;

// alpha 21264
int localBits_alpha = 12;     // Number of local history bits for alpha 21264
int globalBits_alpha = 14;    // Number of global history bits for alpha 21264
uint16_t *lht_alpha;          // Local history table for alpha 21264
uint8_t *lp_alpha;            // Local prediction for alpha 21264
uint8_t *gp_alpha;            // Global prediction for alpha 21264
uint8_t *cp_alpha;            // Choice prediction for alpha 21264
uint64_t ghistory_alpha;      // Global history register for alpha 21264

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

// gshare functions
void init_gshare()
{
  int bht_entries = 1 << ghistoryBits;
  bht_gshare = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for (i = 0; i < bht_entries; i++)
  {
    bht_gshare[i] = WN;
  }
  ghistory = 0;
}

uint8_t gshare_predict(uint32_t pc)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;
  switch (bht_gshare[index])
  {
  case WN:
    return NOTTAKEN;
  case SN:
    return NOTTAKEN;
  case WT:
    return TAKEN;
  case ST:
    return TAKEN;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    return NOTTAKEN;
  }
}

void train_gshare(uint32_t pc, uint8_t outcome)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;

  // Update state of entry in bht based on outcome
  switch (bht_gshare[index])
  {
  case WN:
    bht_gshare[index] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    bht_gshare[index] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    break;
  }

  // Update history register
  ghistory = ((ghistory << 1) | outcome);
}

void cleanup_gshare()
{
  free(bht_gshare);
}

// alpha 21264 functions
void init_alpha()
{
  int lht_entries = 1 << localBits_alpha;
  int ght_entries = 1 << globalBits_alpha;
  lht_alpha = (uint16_t *)malloc(lht_entries * sizeof(uint16_t));
  lp_alpha = (uint8_t *)malloc(lht_entries * sizeof(uint8_t));
  gp_alpha = (uint8_t *)malloc(ght_entries * sizeof(uint8_t));
  cp_alpha = (uint8_t *)malloc(ght_entries * sizeof(uint8_t));
  int i = 0;
  for (i = 0; i < lht_entries; i++)
  {
    lht_alpha[i] = 0;
    lp_alpha[i] = WN;
  }
  for (i = 0; i < ght_entries; i++)
  {
    gp_alpha[i] = WN;
    cp_alpha[i] = WN;   // T for Local prediction, NT for global prediction
  }
  ghistory_alpha = 0;
}

uint8_t alpha_predict(uint32_t pc)
{
  // Get lower bits of pc and global history register
  int lht_entries = 1 << localBits_alpha;
  int ght_entries = 1 << globalBits_alpha;
  uint32_t pc_lower_bits = pc & (lht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory_alpha & (ght_entries - 1);

  // Choice prediction
  uint8_t choice = cp_alpha[ghistory_lower_bits];

  if (choice == ST || choice == WT) {
    // Local prediction
    uint32_t lp_index = lht_alpha[pc_lower_bits] & (lht_entries - 1);
    switch (lp_alpha[lp_index])
    {
      case WN:
        return NOTTAKEN;
      case SN:
        return NOTTAKEN;
      case WT:
        return TAKEN;
      case ST:
        return TAKEN;
      default:
        printf("Warning: Undefined state of entry in alpha local prediction!\n");
        return NOTTAKEN;
    }
  }
  // Global prediction
  switch (gp_alpha[ghistory_lower_bits])
  {
    case WN:
      return NOTTAKEN;
    case SN:
      return NOTTAKEN;
    case WT:
      return TAKEN;
    case ST:
      return TAKEN;
    default:
      printf("Warning: Undefined state of entry in alpha global prediction!\n");
      return NOTTAKEN;
  }
}

void train_alpha(uint32_t pc, uint8_t outcome)
{
  // Get lower bits of pc and global history register
  int lht_entries = 1 << localBits_alpha;
  int ght_entries = 1 << globalBits_alpha;
  uint32_t pc_lower_bits = pc & (lht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory_alpha & (ght_entries - 1);

  // Choice prediction
  uint8_t choice = cp_alpha[ghistory_lower_bits];

  // Get local and global prediction
  uint32_t lp_index = lht_alpha[pc_lower_bits] & (lht_entries - 1);
  uint8_t lp = lp_alpha[lp_index];
  uint8_t gp = gp_alpha[ghistory_lower_bits];

  if (true) {
    // Update local prediction
    switch (lp)
    {
      case WN:
        lp_alpha[lp_index] = (outcome == TAKEN) ? WT : SN;
        break;
      case SN:
        lp_alpha[lp_index] = (outcome == TAKEN) ? WN : SN;
        break;
      case WT:
        lp_alpha[lp_index] = (outcome == TAKEN) ? ST : WN;
        break;
      case ST:
        lp_alpha[lp_index] = (outcome == TAKEN) ? ST : WT;
        break;
      default:
        printf("Warning: Undefined state of entry in alpha local prediction!\n");
        break;
    }
  }
  if (true) {
    // Update global prediction
    switch (gp)
    {
      case WN:
        gp_alpha[ghistory_lower_bits] = (outcome == TAKEN) ? WT : SN;
        break;
      case SN:
        gp_alpha[ghistory_lower_bits] = (outcome == TAKEN) ? WN : SN;
        break;
      case WT:
        gp_alpha[ghistory_lower_bits] = (outcome == TAKEN) ? ST : WN;
        break;
      case ST:
        gp_alpha[ghistory_lower_bits] = (outcome == TAKEN) ? ST : WT;
        break;
      default:
        printf("Warning: Undefined state of entry in alpha global prediction!\n");
        break;
    }
  }

  // Update choice prediction
  if ((lp == ST || lp == WT) && (gp == SN || gp == WN)) {
    if (outcome == TAKEN) {
      // Local is correct
      switch (choice) {
        case WN:
          cp_alpha[ghistory_lower_bits] = WT;
          break;
        case SN:
          cp_alpha[ghistory_lower_bits] = WN;
          break;
        case WT:
          cp_alpha[ghistory_lower_bits] = ST;
          break;
        case ST:
          break;
        default:
          printf("Warning: Undefined state of entry in alpha choice prediction!\n");
          break;
      }
    }
    else {
      // Globla is correct
      switch (choice) {
        case WN:
          cp_alpha[ghistory_lower_bits] = SN;
          break;
        case SN:
          break;
        case WT:
          cp_alpha[ghistory_lower_bits] = WN;
          break;
        case ST:
          cp_alpha[ghistory_lower_bits] = WT;
          break;
        default:
          printf("Warning: Undefined state of entry in alpha choice prediction!\n");
          break;
      }
    }
  }
  if ((lp == SN || lp == WN) && (gp == ST || gp == WT)) {
    if (outcome == NOTTAKEN) {
      // Local is correct
      switch (choice) {
        case WN:
          cp_alpha[ghistory_lower_bits] = WT;
          break;
        case SN:
          cp_alpha[ghistory_lower_bits] = WN;
          break;
        case WT:
          cp_alpha[ghistory_lower_bits] = ST;
          break;
        case ST:
          break;
        default:
          printf("Warning: Undefined state of entry in alpha choice prediction!\n");
          break;
      }
    }
    else {
      // Globla is correct
      switch (choice) {
        case WN:
          cp_alpha[ghistory_lower_bits] = SN;
          break;
        case SN:
          break;
        case WT:
          cp_alpha[ghistory_lower_bits] = WN;
          break;
        case ST:
          cp_alpha[ghistory_lower_bits] = WT;
          break;
        default:
          printf("Warning: Undefined state of entry in alpha choice prediction!\n");
          break;
      }
    }
  }
  
  // Update history register
  ghistory_alpha = ((ghistory_alpha << 1) | outcome);

  // Update local history table
  lht_alpha[pc_lower_bits] = ((lht_alpha[pc_lower_bits] << 1) | outcome);
}

//
// 8-component TAGE 
//
int pc_lower_bits_t0 = 13;   // pc bits for T0
int pc_lower_bits_ti = 10;   // pc bits for T1-T7
int tag_bits = 11;           // tag bits for T1-T7
uint64_t ghistory_tage[2];   // gloabal history
uint8_t *bht_t0;             // T0: bimodal predictor
uint8_t *counter[7];         // 3 counter bits for T1-T7
uint16_t *tag[7];            // 11 tag bits for T1-T7
uint8_t *useful[7];          // 2 useful bits for T1-T7

void init_t0() {
  int bht_entries = 1 << pc_lower_bits_t0;
  bht_t0 = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for (i = 0; i < bht_entries; i++)
  {
    bht_t0[i] = WN;
  }
}

uint8_t t0_predict(uint32_t pc)
{
  int bht_entries = 1 << pc_lower_bits_t0;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  switch (bht_t0[pc_lower_bits])
  {
  case WN:
    return NOTTAKEN;
  case SN:
    return NOTTAKEN;
  case WT:
    return TAKEN;
  case ST:
    return TAKEN;
  default:
    printf("Warning: Undefined state of entry in T0 BHT!\n");
    return NOTTAKEN;
  }
}

void train_t0(uint32_t pc, uint8_t outcome)
{
  int bht_entries = 1 << pc_lower_bits_t0;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);

  // Update state of entry in bht based on outcome
  switch (bht_t0[pc_lower_bits])
  {
  case WN:
    bht_t0[pc_lower_bits] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    bht_t0[pc_lower_bits] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_t0[pc_lower_bits] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_t0[pc_lower_bits] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("Warning: Undefined state of entry in T0 BHT!\n");
    break;
  }
}

int pow_2(int n) {
  int res = 1;
  for (int i = 0; i < n; i++) {
    res *= 2;
  }
  return res;
}

uint32_t ghistory_folder(int index) {
  if (index == 0)
    return ghistory_tage[0] & ((1 << 2) - 1);
  if (index == 1)
    return ghistory_tage[0] & ((1 << 4) - 1);
  if (index == 2)
    return ghistory_tage[0] & ((1 << 8) - 1);
  uint32_t result = 0;
  if (index >= 3)
    result ^= ghistory_tage[0] & ((1 << 16) - 1);
  if (index >= 4)
    result ^= (ghistory_tage[0] >> 16) & ((1 << 16) - 1);
  if (index >= 5) {
    result ^= (ghistory_tage[0] >> 32) & ((1 << 16) - 1);
    result ^= (ghistory_tage[0] >> 48) & ((1 << 16) - 1);
  }
  if (index >= 6) {
    result ^= (ghistory_tage[1]) & ((1 << 16) - 1);
    result ^= (ghistory_tage[1] >> 16) & ((1 << 16) - 1);
    result ^= (ghistory_tage[1] >> 32) & ((1 << 16) - 1);
    result ^= (ghistory_tage[1] >> 48) & ((1 << 16) - 1);
  }
  return result;
}

uint32_t get_entry(uint32_t pc, int i) {           // map pc and history to 10 bits
  uint32_t history = ghistory_folder(i);
  return (pc ^ history) % 1021; 
}

uint32_t get_tag(uint32_t pc, int i) {             // map pc and history to 11 bits
  uint32_t history = ghistory_folder(i);
  return (pc & history ^ (history >> 2)) % 2039;
}

void init_ti() {
  int bht_entries = 1 << pc_lower_bits_ti;
  for (int i = 0; i < 7; i++) {
    counter[i] = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
    tag[i] = (uint16_t *)malloc(bht_entries * sizeof(uint16_t));
    useful[i] = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
    for (int j = 0; j < bht_entries; j++) {
      counter[i][j] = 4;       // 0-3 for NT, 4-7 for T
      tag[i][j] = 0;
      useful[i][j] = 0;
    }
  }
}

uint8_t ti_predict(uint32_t pc, int i) {
  if (counter[i][get_entry(pc, i)] >= 4)
    return TAKEN;
  else
    return NOTTAKEN;
}

void init_tage() {
  init_t0();
  init_ti();
  ghistory_tage[0] = 0;
  ghistory_tage[1] = 0;
}

uint8_t tage_predict(uint32_t pc) {
  int predictor = -1;
  for (int i = 0; i < 7; i++) {
    if (tag[i][get_entry(pc, i)] == get_tag(pc, i)) {
      predictor = i;
    }
  }
  if (predictor == -1) {
    return t0_predict(pc);
  }
  else {
    return ti_predict(pc, predictor);
  }
}

void train_tage(uint32_t pc, uint32_t outcome) {
  int predictor = -1, altpred = -1;
  for (int i = 0; i < 7; i++) {
    if (tag[i][get_entry(pc, i)] == get_tag(pc, i)) {
      altpred = predictor;
      predictor = i;
    }
  }

  if (predictor == -1) {
    train_t0(pc, outcome);
  }

  // update useful for predictor
  uint8_t predictor_result, altpred_result;
  if (predictor == -1)
    predictor_result = t0_predict(pc);
  else
    predictor_result = ti_predict(pc, predictor);
  if (altpred == -1)
    altpred_result = t0_predict(pc);
  else
    altpred_result = ti_predict(pc, altpred);

  if (predictor != -1 && altpred_result != predictor_result) {
    if (predictor_result == outcome && useful[predictor][get_entry(pc, predictor)] < 3)
      useful[predictor][get_entry(pc, predictor)]++;
    else if (predictor_result != outcome && useful[predictor][get_entry(pc, predictor)] > 0)
      useful[predictor][get_entry(pc, predictor)]--;
  }

  // update counter for predictor
  if (predictor != -1 && outcome == TAKEN && counter[predictor][get_entry(pc, predictor)] < 7)
    counter[predictor][get_entry(pc, predictor)]++;
  
  else if (predictor != -1 && outcome == NOTTAKEN && counter[predictor][get_entry(pc, predictor)] > 0)
    counter[predictor][get_entry(pc, predictor)]--;

  // set new entry
  if (predictor_result != outcome && predictor < 6) {
    bool allocated = false;
    int index;
    for (index = predictor + 1; index < 7; index++) {
      if (useful[index][get_entry(pc, index)] == 0) {
        allocated = true;
        break;
      }
      else {
        useful[index][get_entry(pc, index)]--;
      }
    }
    
    if (allocated) {
      // set a new entry at T-index
      tag[index][get_entry(pc, index)] = get_tag(pc, index);
      counter[index][get_entry(pc, index)] = 3;
      useful[index][get_entry(pc, index)] = 0;
    }
  }
  
  // Update history register
  ghistory_tage[1] = (ghistory_tage[1] << 1) | (ghistory_tage[0] >> 63);
  ghistory_tage[0] = (ghistory_tage[0] << 1) | outcome;
}

void init_predictor()
{
  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    init_gshare();
    break;
  case TOURNAMENT:
    init_alpha();
    break;
  case CUSTOM:
    init_tage();
    break;
  default:
    break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint32_t make_prediction(uint32_t pc, uint32_t target, uint32_t direct)
{

  // Make a prediction based on the bpType
  switch (bpType)
  {
  case STATIC:
    return TAKEN;
  case GSHARE:
    return gshare_predict(pc);
  case TOURNAMENT:
    return alpha_predict(pc);
  case CUSTOM:
    return tage_predict(pc);
  default:
    break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//

void train_predictor(uint32_t pc, uint32_t target, uint32_t outcome, uint32_t condition, uint32_t call, uint32_t ret, uint32_t direct)
{
  if (condition)
  {
    switch (bpType)
    {
    case STATIC:
      return;
    case GSHARE:
      return train_gshare(pc, outcome);
    case TOURNAMENT:
      return train_alpha(pc, outcome);
    case CUSTOM:
      return train_tage(pc, outcome);
    default:
      break;
    }
  }
}