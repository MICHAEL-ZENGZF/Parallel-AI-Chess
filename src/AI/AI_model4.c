#include"AI.h"
#include <omp.h>
#include <time.h> 
#include <stdio.h>

//#define CHECK_SCORE
#define MAX_STEP 5
#define MAX_NODES_2_COMPUTE 4000000
#define MAX_TASK_SIZE 6

//This is for simple spawn

int computed_nodes;

//computed_task_size=-resized_x+8
int linear_rearrange(int score, float range, int minScore){
    int computed_task_size=-(score-minScore)/range*MAX_TASK_SIZE+MAX_TASK_SIZE;
    return MIN(MAX(computed_task_size,1),MAX_TASK_SIZE);
}

//the simulation function for the branches in the searching tree
int ai_model4_simulate(GameState *gameState, Player *player, 
    int task_size, int depth)
{
    #pragma omp critical
    computed_nodes++;
    if(computed_nodes>=MAX_NODES_2_COMPUTE||depth>=MAX_STEP)
        return ai_sum_scores(gameState,player);

    int MaxScore=-60000,MinScore=60000;
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

    GameState *simulation=malloc(sizeof(GameState)*total_num_moves);
    int num_blocks=total_num_moves/task_size;
    for(int i=0;i<num_blocks;i++){
        #pragma omp task shared(Scores,playerTurn)
        {
            int id=i*task_size;
            for(int j=0;j<task_size;j++)
            {
                if(id+j>=total_num_moves)break;
                simulation[id+j]=env_copy_State(gameState);
                env_play(&simulation[id+j],player,vector_get(&MovesStart,id+j),vector_get(&MovesEnd,id+j));
                Scores[id+j]=player->color*ai_sum_scores(&simulation[id+j],player);
            }
        }
    }
    #pragma omp taskwait
    
    for(int i=0;i<total_num_moves;i++){
        MaxScore=MAX(MaxScore,Scores[i]);
        MinScore=MIN(MinScore,Scores[i]);
    }
    float range=MaxScore-MinScore;
    for(int i=0;i<num_blocks;i++){
        #pragma omp task shared(Scores)
        {
            int id=i*task_size;
            for(int j=0;j<task_size;j++)
            {
                if(id+j>=total_num_moves)break;
                int score=playerTurn*ai_model4_simulate(&simulation[i],player,
                    linear_rearrange(Scores[i],range,MinScore),depth+1);
                Scores[i]=score;
                env_free_state(&simulation[i]);
            }
        }
    }
    #pragma omp taskwait
    
    free(simulation);

    for(int i=0;i<total_num_moves;i++){
        MaxScore=MAX(MaxScore,Scores[i]);
    }
    
    vector_free(&MovesStart);
    vector_free(&MovesEnd);
    free(Scores);
    return MaxScore*playerTurn;
}

//the play function for the root in the searching tree, return the quit from check_end
int ai_model4_play(GameState *gameState, Player *player)
{
    computed_nodes=0;
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
    omp_set_nested(1);
    omp_set_num_threads(16);
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
    #pragma omp parallel for shared(gameState,player,MovesStart,MovesEnd,Scores,playerTurn)
    for(int i=0;i<total_num_moves;i++)
    {
        GameState simulation=env_copy_State(gameState);
        env_play(&simulation,player,MovesStart[i],MovesEnd[i]);
        score=playerTurn*ai_model4_simulate(&simulation,player,
            2,0);
        Scores[i]=score;
        env_free_state(&simulation);
    }
    printf("Computed Nodes:%d\n",computed_nodes);
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
