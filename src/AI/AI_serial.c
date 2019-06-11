#include"AI.h"
#define MAXSTEP 3
//#define CHECK_SCORE

int num_nodes=0;
int max_num_nodes=0;

//the simulation function for the branches in the searching tree
int ai_serial_simulate(GameState *gameState, Player *player, int depth)
{
    num_nodes++;
    if(depth<=0)return ai_sum_scores(gameState,player);

    int MaxScore=-99999999;
    int score;
    int playerTurn=gameState->playerTurn;

    vector BestMovesID,MovesStart,MovesEnd;
    vector_init(&BestMovesID);
    vector_init(&MovesStart);
    vector_init(&MovesEnd);
    
    int cnt=0;
    vector CurLegalMoves;
    for(int i=0;i<64;i++)
    {
        CurLegalMoves=env_get_legal_moves(gameState,player,i);
        vector_cat(&MovesEnd,&CurLegalMoves);
        cnt=CurLegalMoves.count;
        for(int j=0;j<cnt;j++)
            vector_add(&MovesStart,i);
        vector_free(&CurLegalMoves);
    }

    assert(MovesStart.count==MovesEnd.count);
    cnt=MovesStart.count;
    for(int i=0;i<cnt;i++)
    {
        env_play(gameState,player,vector_get(&MovesStart,i),vector_get(&MovesEnd,i));
        score=playerTurn*ai_serial_simulate(gameState,player,depth-1);
        MaxScore=MAX(MaxScore,score);
        env_undo(gameState);
    }
    vector_free(&BestMovesID);
    vector_free(&MovesStart);
    vector_free(&MovesEnd);
    
    return MaxScore*playerTurn;
}

//the play function for the root in the searching tree, return the quit from check_end
int ai_serial_play(GameState *gameState, Player *player)
{
    int check_end=env_check_end(gameState,player);
    if(check_end!=0)
    {
        env_free_container(gameState);
        return check_end;
    }
    int MaxScore=-99999999;
    int score;
    vector MovesStart,MovesEnd,Scores;
    vector_init(&MovesStart);
    vector_init(&MovesEnd);
    vector_init(&Scores);
    
    
    int cnt=0;
    vector CurLegalMoves;
    int container_size=gameState->moves_vector_cnt;
    for(int i=0;i<container_size;i++)
    {
        CurLegalMoves=gameState->container[i].legal_moves;
        cnt=CurLegalMoves.count;
        vector_cat(&MovesEnd,&CurLegalMoves);
        int pos=gameState->container[i].pos;
        for(int j=0;j<cnt;j++)
            vector_add(&MovesStart,pos);
    }

    assert(MovesStart.count==MovesEnd.count);
    cnt=MovesStart.count;
    int playerTurn=gameState->playerTurn;
    num_nodes=0;
    for(int i=0;i<cnt;i++)
    {
        env_play(gameState,player,vector_get(&MovesStart,i),vector_get(&MovesEnd,i));
        score=playerTurn*ai_serial_simulate(gameState,player,MAXSTEP);
        vector_add(&Scores,score);
        MaxScore=MAX(MaxScore,score);
        env_undo(gameState);
    }
    max_num_nodes=MAX(max_num_nodes,num_nodes);
    printf("Max Num Nodes:%d\n",max_num_nodes);
    int BestMovesCnt=0;
    vector BestMovesID;
    vector_init(&BestMovesID);
    if(stack_check_repeated_move(gameState->moves_stack)){
        int MaxScoresArr[6];
        for(int i=0;i<6;i++)MaxScoresArr[i]=-60000;
        int MinScoreID,MinScoreArrValue;
        for(int i=0;i<cnt;i++){
            MinScoreArrValue=MaxScoresArr[0];
            MinScoreID=0;
            for(int j=1;j<6;j++){
                if(MaxScoresArr[j]<MinScoreArrValue){
                    MinScoreArrValue=MaxScoresArr[j];
                    MinScoreID=j;
                }
            }
            MaxScoresArr[MinScoreID]=MAX(MaxScoresArr[MinScoreID],vector_get(&Scores,i));
        }
        for(int i=0;i<cnt;i++){
            for(int j=0;j<6;j++){
                if(vector_get(&Scores,i)==MaxScoresArr[j]){
                    vector_add(&BestMovesID,i);
                    BestMovesCnt++;
                }
            }
        }
    }
    else{
        for(int i=0;i<cnt;i++){
            if(vector_get(&Scores,i)==MaxScore){
                vector_add(&BestMovesID,i);
                BestMovesCnt++;
            }
        }
    }
    int id=vector_get(&BestMovesID,rand()%BestMovesCnt);
    #ifdef CHECK_SCORE
    printf("It is %d playing\n",gameState->playerTurn);
    ai_print_board(gameState);
    printf("Current Score is %d\n",ai_sum_scores(gameState,player));
    #endif
    env_play(gameState,player,vector_get(&MovesStart,id),vector_get(&MovesEnd,id));
    #ifdef CHECK_SCORE
    printf("The player has decided to move from %d to %d\n",vector_get(&MovesStart,id),vector_get(&MovesEnd,id));
    ai_print_board(gameState);
    printf("After making the move, the score is %d\n",ai_sum_scores(gameState,player));
    #endif
    vector_free(&BestMovesID);
    vector_free(&MovesStart);
    vector_free(&MovesEnd);
    vector_free(&Scores);
    env_free_container(gameState);
    return 0;
}