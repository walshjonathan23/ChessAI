/*
 * ai.cpp
 * This file contains the function implementations for the ai class.
 */

#include "game.h"
#include "ai.h"
#include <iostream>
#include <time.h>
#include <fstream>
#include <queue>

using namespace std;

vector<action> ai::ht;

action ai::runTurn()
{
	action nextMove;

	//Determine the starting time of execution to calculate time passage
	clock_t startTime = clock();

	//Copy the current board state from the game hub
	state boardState;
	boardState = target_game->current_state;

	boardState.maxPlayer = boardState.currentPlayer;

	boardState.updateMaterialDifference();

	bool isOpening;

	srand(time(NULL));

	//Determine if the game is still in the opening phase
	if(target_game->currentTurn < 30)
	{
		if(boardState.currentPlayer->rankDirection == 1)
		{
			isOpening = (target_game->currentTurn < 20 || !boardState.whiteHasCastled);
		}
		else
		{
			isOpening = (target_game->currentTurn < 20 || !boardState.blackHasCastled);
		}
	}

	boardState.utilityValue = boardState.calculateUtility(isOpening, strategy);

	//Retreive the history table from a file on the first turn
	if(target_game->currentTurn < 2)
		getHistoryTable(boardState);

	//Determine the best action to take
	nextMove = IDABminimax(boardState, startTime, isOpening);
	//Update the static castling records if a castle move occurs
	if(nextMove.isCastle)
	{
		if(boardState.currentPlayer->rankDirection == 1)
			boardState.whiteHasCastled = true;
		else
			boardState.blackHasCastled = true;
	}

	//Store the history table in a file and age it's contents
	storeHistoryTable(boardState.whiteHasCastled, boardState.blackHasCastled);
	ageHistoryTable();

	return nextMove;
}

void ai::initializeHistoryTable()
{
	ofstream fout(htFile);
	fout << "0" << endl;
	fout << "0" << endl;
	fout << "0" << endl;
	fout.close();
}

//This function reads the history table stored in the ai's history table file and saves it to ht
void ai::getHistoryTable(state& s)
{
    ifstream fin;
    int numEntries;
    action tmp;
    fin.open(htFile);

    if(fin.is_open())
    {
        fin >> s.whiteHasCastled;
        fin >> s.blackHasCastled;
        fin >> numEntries;
        for(int i = 0; i < numEntries; i++)
        {
            fin >> tmp.type;
            fin >> tmp.oldRank;
            fin >> tmp.oldFile;
            fin >> tmp.newRank;
            fin >> tmp.newFile;
            fin >> tmp.historyValue;
            fin >> tmp.htAge;
            ht.push_back(tmp);
        }
    }
    fin.close();
    return;
}

//This function takes ht and writes it to the ai's history table file for the next turn
void ai::storeHistoryTable(bool whiteHasCastled, bool blackHasCastled)
{
    ofstream fout;
    fout.open(htFile);

    if(fout.is_open())
    {
        fout << whiteHasCastled << endl;
        fout << blackHasCastled << endl;
        fout << ht.size() << endl;
        for(unsigned int i = 0; i < ht.size(); i++)
        {
            fout << ht[i].type << " ";
            fout << ht[i].oldRank << " ";
            fout << ht[i].oldFile << " ";
            fout << ht[i].newRank << " ";
            fout << ht[i].newFile << " ";
            fout << ht[i].historyValue << " ";
            fout << ht[i].htAge + 1;
            fout << endl;
        }
    }
    fout.close();
    return;
}

//This function increases the history table value of action a or adds it to the table
//if it wasn't there. If the number of actions exceeds the maximum limit, it purges the table
void ai::updateHistoryTable(action& a)
{
    const int MAX_ACTIONS = 300;
    if(a.historyValue > 0)
    {
        unsigned int i = 0;
        while(i < ht.size())
        {
            if(ht[i] == a)
            {
                ht[i].historyValue++;
                return;
            }
            i++;
        }
    }

    if(ht.size() >= MAX_ACTIONS)
    {
        purgeHistoryTable();
    }

    a.historyValue++;
    ht.push_back(a);
    return;
}

//Increment the age of all entries in the history table to aid
//in purging unused entries
void ai::ageHistoryTable()
{
    for(unsigned int i = 0; i < ht.size(); i++)
    {
        ht[i].htAge++;
    }
    return;
}

//This function returns the history table value of action a
int ai::retrieveHistoryValue(action& a)
{
    for(unsigned int i = 0; i < ht.size(); i++)
    {
        if(ht[i] == a)
        {
            return ht[i].historyValue;
        }
    }
    return 0;
}

//This function removes entries that exceed the age barrier. The age of an action
//is the number of turns since it first entered the table. This keeps newer entries on
//the table when there are too many entries.
void ai::purgeHistoryTable()
{
    const int AGE_BARRIER = 6;

    for(unsigned int i = 0; i < ht.size(); i++)
    {
        if(ht[i].htAge >= AGE_BARRIER || ht[i].historyValue < 3)
        {
            ht.erase(ht.begin()+i);
        }
    }
    return;
}

//This function return true if the change in utility from the previous state exceeds
//the change barrier.
bool ai::isNonquiescent(state& s)
{
    const int CHANGE_BARRIER = 2000;
    return abs(s.quiescentChange) > CHANGE_BARRIER;
}

//calculates the time remaining when PERCENT_TIME_REMAINING percent of the starting time
//is used then returns the value. All times are measured in nanoseconds.
//The percentage of time remaining gives more time early in the game and less towards the end
//This allows the early turns which have more possible moves to go to deeper depths
//Late game depths are deeper because there are inherently few moves to be made by fewer pieces
double ai::timeHeuristic(state& s, clock_t startTime, double timeRemaining, bool isOpening)
{
    const int PERCENT_TIME_REMAINING = 8;
    const int OPENING_TIME = 7;
    double endTime;
    int percent = PERCENT_TIME_REMAINING;
    if(isOpening)
        percent = OPENING_TIME;

    //Calculate how much time should be remaining at the end of contemplation
    endTime = timeRemaining - (timeRemaining*percent/100);

    //outputs the expected threshold for calculation time
    //cout << endTime << endl;

    return endTime;
}

//estimates based on the startTime, the time since then, and the endTime whether
//another iterative depth search can be completed. The estimate averages the
//breadth of the max player moves and the next layer's min player moves. This average
//roughly estimates the number of moves at any given depth. The amount of time for a depth
//should be roughly equal to the time for the previous depth times the average breadth.
//returns true if the estimated time to search is finished before endTime
//returns false if it would exceed the bounds
bool ai::canCompleteNextDepth(state& s, clock_t startTime, double timeRemaining,  double endTime)
{
    double timeElapsed = float(clock() - startTime)/CLOCKS_PER_SEC;
    int averageBreadth;
    vector<action> possibleMoves1;
    vector<action> possibleMoves2;
    action nextAction;

    possibleMoves1 = s.actions();
    possibleMoves2 = s.result(possibleMoves1[0], false, strategy, false, false).actions();

    averageBreadth = (possibleMoves1.size()+possibleMoves2.size())/2;

    if(timeRemaining - (timeElapsed * averageBreadth) > endTime)
        return true;
    else
        return false;
}

//Time-limited Alpha Beta Iterative Deepening Depth-limited Minimax algorithm
//Runs ABminimax for as many dpeths as possible based on the timeHeuristic and
//the estimation of whether another depth can be completed within this time limit
action ai::IDABminimax(state& s, clock_t startTime, bool isOpening)
{
    const int QUIESCENT_DEPTH = 3;
    const int MAXDEPTH = 20;
    action result;
    int iterativeDepth = 1;
    double timeRemaining;

    if(target_game->current_state.currentPlayer->rankDirection == 1)
    {
    	timeRemaining = target_game->whiteTimeRemaining;
    }
    else
    {
    	timeRemaining = target_game->blackTimeRemaining;
    }

    //calculate the time by which calculations need to be finished
    double endTime = timeHeuristic(s, startTime, timeRemaining, isOpening);

    while(iterativeDepth <= MAXDEPTH && canCompleteNextDepth(s, startTime, timeRemaining, endTime))
    {
        cout << "Iterative Depth: " << iterativeDepth << endl;
        result = ABminimax(s, iterativeDepth, QUIESCENT_DEPTH, isOpening);
        iterativeDepth++;
    }


    return result;
}

//Returns the action that leads to the maximum value utility node at the passed depth
action ai::ABminimax(state& s, int depth, int quiescentDepth, bool isOpening)
{
    vector<action> possibleMoves;
    priority_queue<action> orderedMoves;
    int alpha = -100000;
    int beta = 100000;
    int currentUtility;
    action maxAction;
    action nextAction;

    //generate all actions possible for the current player
    possibleMoves = s.actions();
    for(unsigned int i = 0; i < possibleMoves.size(); i++)
    {
        possibleMoves[i].historyValue = retrieveHistoryValue(possibleMoves[i]);
        orderedMoves.push(possibleMoves[i]);
    }

    //assume that the first possible move is the maximum utility
    maxAction = orderedMoves.top();

    while(!orderedMoves.empty())
    {
        nextAction = orderedMoves.top();
        currentUtility = ABminValue(s.result(nextAction, false, strategy, true, false), depth-1, quiescentDepth, alpha, beta, isOpening);
        //if the current move improves the lower bound, set it as the best move
        if(currentUtility > alpha)
        {
            alpha = currentUtility;
            maxAction = nextAction;
        }
        orderedMoves.pop();
    }
    updateHistoryTable(maxAction);

    return maxAction;
}

//Returns the maximum utility that can be reached at depth away from this state
//using alpha beta pruning
int ai::ABmaxValue(state s, int depth, int quiescentDepth, int alpha, int beta, bool isOpening)
{
    bool calcUtil = false;
    if(depth == 1)
        calcUtil = true;
    if(s.isTerminalState)
    {
        return s.calculateUtility(isOpening, strategy);
    }
    if(depth == 0)
    {
        if(!isNonquiescent(s))
        {
            return s.utilityValue;
        }
        else
        {
            if(quiescentDepth > 0)
            {
                depth = 1;
                quiescentDepth--;
            }
            else
            {
                return s.utilityValue;
            }
        }
    }

    int currentUtility;
    vector<action> possibleMoves;
    priority_queue<action> orderedMoves;
    int bestUtility = -100000;
    action bestAction;
    action nextAction;

    //generate all possible actions from this board state
    possibleMoves = s.actions();
    for(unsigned int i = 0; i < possibleMoves.size(); i++)
    {
        possibleMoves[i].historyValue = retrieveHistoryValue(possibleMoves[i]);
        orderedMoves.push(possibleMoves[i]);
    }

    //for each action determine if it provides a new max utility
    while(!orderedMoves.empty())
    {
        nextAction = orderedMoves.top();
        currentUtility = ABminValue(s.result(nextAction, calcUtil, strategy, true, isOpening), depth-1, quiescentDepth, alpha, beta, isOpening);
        if(currentUtility > bestUtility)
        {
            bestAction = nextAction;
            bestUtility = currentUtility;
        }
        if(currentUtility > alpha)
        {
            //better move found, update lower bound
            alpha = currentUtility;
        }
        if(beta <= alpha)
        {
            updateHistoryTable(nextAction);
            //Prune
            return alpha;
        }
        orderedMoves.pop();
    }
    updateHistoryTable(bestAction);
    return alpha;
}

//returns the minimum utility that can be reached at depth away from this state
//using alpha beta pruning
int ai::ABminValue(state s, int depth, int quiescentDepth, int alpha, int beta, bool isOpening)
{
    bool calcUtil = false;
    if(depth == 1)
        calcUtil = true;
    if(s.isTerminalState)
    {
        return s.calculateUtility(isOpening, strategy);
    }
    if(depth == 0)
    {
        if(!isNonquiescent(s))
        {
            return s.utilityValue;
        }
        else
        {
            if(quiescentDepth > 0)
            {
                depth = 1;
                quiescentDepth--;
            }
            else
            {
                return s.utilityValue;
            }
        }
    }

    int currentUtility;
    int bestUtility = 100000;
    vector<action> possibleMoves;
    priority_queue<action> orderedMoves;
    action nextAction;
    action bestAction;

    possibleMoves = s.actions();
    for(unsigned int i = 0; i < possibleMoves.size(); i++)
    {
        possibleMoves[i].historyValue = retrieveHistoryValue(possibleMoves[i]);
        orderedMoves.push(possibleMoves[i]);
    }

    while(!orderedMoves.empty())
    {
        nextAction = orderedMoves.top();
        currentUtility = ABmaxValue(s.result(nextAction, calcUtil, strategy, true, isOpening), depth-1, quiescentDepth, alpha, beta, isOpening);
        if(currentUtility < bestUtility)
        {
            bestUtility = currentUtility;
            bestAction = nextAction;
        }
        if(currentUtility < beta)
        {
            //better move found, update upper bound
            beta = currentUtility;
        }
        if(beta <= alpha)
        {
            updateHistoryTable(nextAction);
            //Prune
            return beta;
        }
        orderedMoves.pop();
    }
    updateHistoryTable(bestAction);
    return beta;
}
