#include"struct.h"
#include"constant.h"
#include"ENV.h"
#include<time.h> 
#include<stdio.h> 
#include<stdlib.h> 
#include <stdbool.h>

#ifndef AI_H
#define AI_H

#define AI_SERIAL 0
#define AI_MODEL1 1
#define AI_MODEL2 2
#define AI_MODEL3 3
#define AI_MODEL4 4

#define MIN(X,Y) ((X)>(Y)?(Y):(X))
#define MAX(X,Y) ((X)>(Y)?(X):(Y))

void ai_print_board(GameState *gameState);//for printing log

//scores for different pieces at different position
extern int pos_scores_pawn[2][64];
extern int pos_scores_knights[2][64];
extern int pos_scores_bishops[2][64];
extern int pos_scores_rooks[2][64];
extern int pos_scores_queens[2][64];
extern int pos_scores_kings_mid[2][64];
extern int pos_scores_kings_end[2][64];

//scores of different pieces
extern int piece_scores[7];

//the global function for ai to select model and play
int ai_play(GameState *gameState,Player *player, int model);
int ai_experiment(GameState *gameState,Player *player, 
    int model, double *time, int maxStep);

//sum up the scores including pieces and location scores
int ai_sum_scores(GameState *gameState, Player *player);

//different model of AI to play
int ai_serial_play(GameState *gameState, Player *player, int maxStep);
int ai_model1_play(GameState *gameState, Player *player, int maxStep);
int ai_model2_play(GameState *gameState, Player *player, int maxStep);
int ai_model3_play(GameState *gameState, Player *player, int maxStep);
int ai_model4_play(GameState *gameState, Player *player, int maxStep);
int ai_model2_AB_play(GameState *gameState, Player *player, int maxStep);
int ai_model3_AB_play(GameState *gameState, Player *player, int maxStep);
//Marshall`s experiment AI
int ai_easy_play(GameState *gameState, Player *player);

#endif