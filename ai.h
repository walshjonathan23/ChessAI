/*
 * ai.h
 * This file contains the header information for the ai class.
 */

#ifndef AI_H_
#define AI_H_

#include "game.h"
#include "human.h"

//The ai class focuses on determining an optimal action through
//analysis of game trees and the utility of their states
class ai
{
    public:

		//History Table storage
		//The history table keeps track of moves that have been selected as optimal
		//in previous iterations so they can be evaluated before other states.
		//This method encourages pruning because a move that was the best in the past
		//is more likely to still be better than most in later searches creating
		//more opportunities where alpha and beta values cross over.
		static vector<action> ht;
		string htFile = "historyTable.txt";

		//strategy determines how the AI computes the utility of a state
		//strategy 0 is a complex strategy including the safety of the king,
		//an additional opening utility to encourage major piece development,
		//consideration for rook positioning, and pawn progression.

		//strategy 1 is a simple heuristic where all that is considered is
		//how many of each piece type is in play for each player

		//Strategy 0 understands chess strategy much better but has notable
		//overhead costs to evaluate so many factors

		//Strategy 1 is simplistic but is able to be calculated with minimal overhead
		//allowing deeper exploration of game trees in a given amount of time
		int strategy;

		//A reference to the game the AI is interacting with to obtain the current_state
		//for creating a new game tree
		game* target_game = NULL;

		ai(int new_strategy, char historyTableIndex) {strategy = new_strategy; htFile.insert(htFile.begin()+12,historyTableIndex);}

		//Sets up the necessary variables to run the move selection algorithm and return its results
		action runTurn();

		//Helper functions for interacting with the history table and its values
		void initializeHistoryTable();
        void getHistoryTable(state& s);
        void storeHistoryTable(bool maxHasCastled, bool oppHasCastled);
        void updateHistoryTable(action& a);
        void ageHistoryTable();
        int retrieveHistoryValue(action& a);
        void purgeHistoryTable();


        //This function determines if a state had an unusually large change in utility from the previous
        //state. If this state is on the event horizon, then the algorithm will extend its depth to
        //make sure the utility change isn't a false value that is followed up by a equal negative
        //response. This avoids letting a trap be set to trick the AI into starting down an overall
        //bad path to get a nearby reward.
        bool isNonquiescent(state& s);

        //Functions helping the AI relate how long it is taking to make moves and what depth of
        //game tree can it observe without over using its time allowance
        double timeHeuristic(state& s, clock_t startTime, double timeRemaining, bool isOpening);
        bool canCompleteNextDepth(state& s, clock_t startTime, double timeRemaining,  double endTime);

        //The primary move selection algorithm and its recursive sub-functions
        action IDABminimax(state& s, clock_t startTime, bool isOpening);
        action ABminimax(state& s, int depth, int quiescentDepth, bool isOpening);
        int ABmaxValue(state s, int depth, int quiescentDepth, int alpha, int beta, bool isOpening);
        int ABminValue(state s, int depth, int quiescentDepth, int alpha, int beta, bool isOpening);
};

#endif /* AI_H_ */
