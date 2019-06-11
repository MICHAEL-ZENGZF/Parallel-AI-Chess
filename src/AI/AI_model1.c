#include"AI.h"
#include <omp.h>
#define MAXSTEP 3
//#define CHECK_SCORE

//This is for simple spawn

//the simulation function for the branches in the searching tree
int ai_model1_simulate(GameState *gameState, Player *player, int depth)
{
    if(depth<=0)return ai_sum_scores(gameState,player);

    int MaxScore=-60000;
    int playerTurn=gameState->playerTurn;

    int total_num_moves=0;
    vector MovesStart,MovesEnd;
    vector_init(&MovesStart);
    vector_init(&MovesEnd);
    
    int cnt=0;
    for(int i=0;i<64;i++)
    {
        vector CurLegalMoves=env_get_legal_moves(gameState,player,i);
        cnt=CurLegalMoves.count;
        if(cnt>0){
            vector_cat(&MovesEnd,&CurLegalMoves);
            for(int j=0;j<cnt;j++) vector_add(&MovesStart,i);
        }
        vector_free(&CurLegalMoves);
        total_num_moves+=cnt;
    }

    assert(MovesStart.count==MovesEnd.count);
    int *Scores=malloc(sizeof(int)*total_num_moves);
    omp_set_num_threads(2);
    #pragma omp parallel for shared(total_num_moves,gameState,player,MovesStart,MovesEnd,depth,Scores,playerTurn)
    for(int i=0;i<total_num_moves;i++)
    {
        GameState simulation=env_copy_State(gameState);
        env_play(&simulation,player,vector_get(&MovesStart,i),vector_get(&MovesEnd,i));
        int score=playerTurn*ai_model1_simulate(&simulation,player,depth-1);
        Scores[i]=score;
        env_free_state(&simulation);
    }
    
    for(int i=0;i<total_num_moves;i++)MaxScore=MAX(MaxScore,Scores[i]);
    vector_free(&MovesStart);
    vector_free(&MovesEnd);
    free(Scores);
    return MaxScore*playerTurn;
}

//the play function for the root in the searching tree, return the quit from check_end
int ai_model1_play(GameState *gameState, Player *player)
{
    int check_end=env_check_end(gameState,player);
    if(check_end!=0)
    {
        env_free_container(gameState);
        return check_end;
    }
    int MaxScore=-60000;
    int score;
    // vector MovesStart,MovesEnd,Scores;
    // vector_init(&BestMovesID);
    // vector_init(&MovesStart);
    // vector_init(&MovesEnd);
    // vector_init(&Scores);
    
    int container_size=gameState->moves_vector_cnt;
    int total_num_moves=0;
    int *accu_container_size_arr=malloc(sizeof(int)*gameState->moves_vector_cnt);
    for(int i=0;i<container_size;i++){
        total_num_moves+=gameState->container[i].legal_moves.count;
        if(i==0)accu_container_size_arr[0]=0;
        else accu_container_size_arr[i]=accu_container_size_arr[i-1]+gameState->container[i-1].legal_moves.count;
    }
    
    int *MovesStart=malloc(sizeof(int)*total_num_moves);
    int *MovesEnd=malloc(sizeof(int)*total_num_moves);
    int *Scores=malloc(sizeof(int)*total_num_moves);
    
    omp_set_num_threads(16);
    omp_set_nested(1);
    #pragma omp parallel for shared(MovesStart,MovesEnd)
    for(int i=0;i<container_size;i++)
    {
        vector CurLegalMoves=gameState->container[i].legal_moves;
        int cnt=CurLegalMoves.count;
        int pos=gameState->container[i].pos;
        for(int j=0;j<cnt;j++){
            MovesStart[accu_container_size_arr[i]+j]=pos;
            MovesEnd[accu_container_size_arr[i]+j]=vector_get(&CurLegalMoves,j);
        }
    }

    // assert(MovesStart.count==MovesEnd.count);
    int playerTurn=gameState->playerTurn;
    #pragma omp parallel for shared(Scores)
    for(int i=0;i<total_num_moves;i++)
    {
        GameState simulation=env_copy_State(gameState);
        env_play(&simulation,player,MovesStart[i],MovesEnd[i]);
        score=playerTurn*ai_model1_simulate(&simulation,player,MAXSTEP);
        Scores[i]=score;
        env_free_state(&simulation);
    }

    
    int BestMovesCnt=0;
    vector BestMovesID;
    vector_init(&BestMovesID);
    if(stack_check_repeated_move(gameState->moves_stack)){
        int MaxScoresArr[6];
        for(int i=0;i<6;i++)MaxScoresArr[i]=-60000;
        int MinScoreID,MinScoreArrValue;
        for(int i=0;i<total_num_moves;i++){
            MinScoreArrValue=MaxScoresArr[0];
            MinScoreID=0;
            for(int j=1;j<6;j++){
                if(MaxScoresArr[j]<MinScoreArrValue){
                    MinScoreArrValue=MaxScoresArr[j];
                    MinScoreID=j;
                }
            }
            MaxScoresArr[MinScoreID]=MAX(MaxScoresArr[MinScoreID],Scores[i]);
        }
        for(int i=0;i<total_num_moves;i++){
            for(int j=0;j<6;j++){
                if(Scores[i]==MaxScoresArr[j]){
                    vector_add(&BestMovesID,i);
                    BestMovesCnt++;
                }
            }
        }
    }
    else{
        for(int i=0;i<total_num_moves;i++)MaxScore=MAX(MaxScore,Scores[i]);
        for(int i=0;i<total_num_moves;i++){
            if(Scores[i]==MaxScore){
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
    env_play(gameState,player,MovesStart[id],MovesEnd[id]);
    #ifdef CHECK_SCORE
    printf("The player has decided to move from %d to %d\n",vector_get(&MovesStart,id),vector_get(&MovesEnd,id));
    ai_print_board(gameState);
    printf("After making the move, the score is %d\n",ai_sum_scores(gameState,player));
    #endif
    vector_free(&BestMovesID);
    free(MovesStart);
    free(MovesEnd);
    free(Scores);
    env_free_container(gameState);
    return 0;
}
