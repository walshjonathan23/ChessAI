/*
 * game.cpp
 * This file contains the function implementation of the action, state, and game classes
 */

#include "game.h"
#include "ai.h"
#include <iostream>
#include <time.h>
#include <fstream>
#include <queue>

using namespace std;

bool state::whiteHasCastled = false;
bool state::blackHasCastled = false;



//Returns a vector of all possible moves possible from state
//if existenceCheck is true, actions returns the first set of valid moves it finds
//This limits the number of evaluations needed to determine if any move exists at the
//current state
vector<action> state::actions(bool existenceCheck)
{
    vector<action> results;
    vector<action> newMoves;

    for(unsigned int i = 0; i < currentPlayer->pieces.size(); i++)
    {
        if(currentPlayer->pieces[i]->type == "King")
        {
            newMoves = generateKingMoves(currentPlayer->pieces[i]);
        }
        else if(currentPlayer->pieces[i]->type == "Queen")
        {
            newMoves = generateQueenMoves(currentPlayer->pieces[i]);
        }
        else if(currentPlayer->pieces[i]->type == "Knight")
        {
            newMoves = generateKnightMoves(currentPlayer->pieces[i]);
        }
        else if(currentPlayer->pieces[i]->type == "Rook")
        {
            newMoves = generateRookMoves(currentPlayer->pieces[i]);
        }
        else if(currentPlayer->pieces[i]->type == "Bishop")
        {
            newMoves = generateBishopMoves(currentPlayer->pieces[i]);
        }
        else if(currentPlayer->pieces[i]->type == "Pawn")
        {
            newMoves = generatePawnMoves(currentPlayer->pieces[i]);
        }
        //if a move exists return the set of moves found
        if(existenceCheck)
        {
            if(results.size() > 0)
                return results;
        }
        //add new moves to total results
        for(unsigned int j = 0; j < newMoves.size(); j++)
        {
            results.push_back(newMoves[j]);
        }

    }
    return results;
}

//returns a state that occurs from the calling state taking action a
//the resulting state will only calculate and store its utility if calcUtil is true
//if calcTerminal is true, the resulting state will do an abbreviated utility
//check to see if the state is terminal or not. This is stored in the state's isTerminalState.
state state::result(action& a, bool calcUtil, int strategy, bool calcTerminal, bool isOpening)
{
    state tmp;
    tmp = *this;
    unsigned int i = 0;

    //identify index of target piece
    while(i < tmp.currentPlayer->pieces.size() &&
          (tmp.currentPlayer->pieces[i]->rank != a.oldRank ||
          tmp.currentPlayer->pieces[i]->file != a.oldFile))
    {
        i++;
    }

    //save pointer to target piece to move
    myPiece* movedPiece = tmp.currentPlayer->pieces[i];

    movedPiece->hasMoved = true;

    //update type if promoted
    if(a.promotion != "")
        movedPiece->type = a.promotion;

    //remove taken piece for En Passant capture
    if(a.isEnPassant)
    {
        tmp.removeTakenPiece(a.oldRank, a.newFile);
    }

    if(!a.isCastle)
    {
        //if a piece is in the destination, it is captured and removed
        if(tmp.occupied(a.newRank, tmp.fileToInt(a.newFile), currentPlayer) == -1)
        {
            tmp.removeTakenPiece(a.newRank, a.newFile);
        }
    }
    else
    {
        string targetFile;
        i = 0;
        if(a.newFile > a.oldFile)
        {
            targetFile = "h";
        }
        else
        {
            targetFile = "a";
        }

        //find index of rook for castling
        while(i < tmp.currentPlayer->pieces.size() &&
             (tmp.currentPlayer->pieces[i]->type != "Rook" ||
              tmp.currentPlayer->pieces[i]->file != targetFile))
        {
            i++;
        }

        //update position of rook for castling
        if(a.newFile > a.oldFile)
        {
            tmp.currentPlayer->pieces[i]->file = intToFile(fileToInt(a.newFile) - 1);
        }
        else
        {
            tmp.currentPlayer->pieces[i]->file = intToFile(fileToInt(a.newFile) + 1);
        }
        if(tmp.currentPlayer->rankDirection == 1)
            tmp.whiteHasCastled = true;
        else
            tmp.blackHasCastled = true;
    }

    //update moved piece location on board
    movedPiece->file = a.newFile;
    movedPiece->rank = a.newRank;

    //store this action as the previous action by erasing the earliest action
    if(tmp.previousActions.size() >= 8)
        tmp.previousActions.erase(tmp.previousActions.begin());
    tmp.previousActions.push_back(a);

    //the resulting state is the opponent's move
    tmp.currentPlayer = tmp.currentPlayer->opponent;


    //calculates utility if prompted
    if(calcUtil)
    {
        tmp.utilityValue = tmp.calculateUtility(isOpening, strategy);
        tmp.quiescentChange = tmp.utilityValue - this->calculateUtility(isOpening, strategy);
    }
    //Optionally checks whether a state is the end of the game
    else if(calcTerminal)
    {
        vector<action> possibleMoves;
        possibleMoves = actions(true);
        isTerminalState = false;
        if(possibleMoves.size() == 0)
            isTerminalState = true;
        if(isDraw())
            isTerminalState = true;
    }

    return tmp;
}

state::state()
{
    isTerminalState = false;
    maxPlayer = NULL;
    currentPlayer = NULL;
    utilityValue = 10000;
    myPlayer* tmp;
    tmp = new myPlayer;
    players.push_back(tmp);
    tmp = new myPlayer;
    players.push_back(tmp);
    players[0]->rankDirection = 1;
    players[1]->rankDirection = -1;
    players[0]->opponent = players[1];
    players[1]->opponent = players[0];
}

state::~state()
{
    for(unsigned int i = 0; i < players[0]->pieces.size(); i++)
        delete players[0]->pieces[i];
    for(unsigned int i = 0; i < players[1]->pieces.size(); i++)
        delete players[1]->pieces[i];
    delete players[0];
    delete players[1];
}

void state::operator=(const state& s)
{
    //clear old memory
    for(unsigned int i = 0; i < players[0]->pieces.size(); i++)
        delete players[0]->pieces[i];
    for(unsigned int i = 0; i < players[1]->pieces.size(); i++)
        delete players[1]->pieces[i];
    players[0]->pieces.clear();
    players[1]->pieces.clear();
    utilityValue = s.utilityValue;
    materialDifference = s.materialDifference;
    quiescentChange = s.quiescentChange;
    isTerminalState = s.isTerminalState;

    whiteHasCastled = s.whiteHasCastled;
    blackHasCastled = s.blackHasCastled;

    //copy previous actions
    previousActions.clear();
    for(unsigned int i = 0; i < s.previousActions.size(); i++)
    {
        previousActions.push_back(s.previousActions[i]);
    }


    if(s.currentPlayer->rankDirection == 1)
        currentPlayer = players[0];
    else
        currentPlayer = players[1];

    if(s.currentPlayer == s.maxPlayer)
        maxPlayer = currentPlayer;
    else
        maxPlayer = currentPlayer->opponent;

    myPiece* tmp;

    //copy current player pieces
    for(unsigned int i = 0; i < s.currentPlayer->pieces.size(); i++)
    {
        tmp = new myPiece;
        tmp->file = s.currentPlayer->pieces[i]->file;
        tmp->rank = s.currentPlayer->pieces[i]->rank;
        tmp->hasMoved = s.currentPlayer->pieces[i]->hasMoved;
        tmp->owner = currentPlayer;
        tmp->type = s.currentPlayer->pieces[i]->type;
        currentPlayer->pieces.push_back(tmp);
    }

    //copy opponent pieces
    for(unsigned int i = 0; i < s.currentPlayer->opponent->pieces.size(); i++)
    {
        tmp = new myPiece;
        tmp->file = s.currentPlayer->opponent->pieces[i]->file;
        tmp->rank = s.currentPlayer->opponent->pieces[i]->rank;
        tmp->hasMoved = s.currentPlayer->opponent->pieces[i]->hasMoved;
        tmp->owner = currentPlayer->opponent;
        tmp->type = s.currentPlayer->opponent->pieces[i]->type;
        currentPlayer->opponent->pieces.push_back(tmp);
    }
}

//Returns the utility of the state. isOpening is a flag to determine
//if the state is early on in the game which requires an additional
//set of calculations to encourage early piece development.
//The strategy integer determines what process is used by the function
//to determine the utility. Currently the options are 0 for a complex
//strategy and 1 for a simplified version.
int state::calculateUtility(bool isOpening, int strategy)
{
    int totalUtility = 0;

    int playerMaterial = 0;
    int opponentMaterial = 0;

    myPiece* myKing = NULL;
    myPiece* oppKing = NULL;
    vector<myPiece*> myRooks;
    vector<myPiece*> oppRooks;
    bool myHasQueen = false;
    bool oppHasQueen = false;

    //generate possible actions to see if any exist
    vector<action> possibleMoves;
    possibleMoves = actions(true);

    //if the player is in check and has no valid moves, it is checkmate
    if(isCheck(currentPlayer) && possibleMoves.size() == 0)
    {
        //return the value of a win or a loss depending on the max player
        if(currentPlayer == maxPlayer)
            return (-CHECKMATEVALUE);
        else
            return CHECKMATEVALUE;
    }
    //evaluates whether a state is a draw
    else if(isDraw())
    {
        return DRAWVALUE;
    }
    //if there are no moves available but the opponent is not in check, it is a stalemate
    //the value of this state is treated the same as a draw state because there is no winner
    else if(possibleMoves.size() == 0)
    {
        return DRAWVALUE;
    }

    if(strategy == 1)
    {
    	return materialDifference;
    }
    else
    {
    	//piece value calculation for maxPlayer
    	for(unsigned int i = 0; i < maxPlayer->pieces.size(); i++)
    	{
    		if(maxPlayer->pieces[i]->type == "King")
    		{
    			myKing = maxPlayer->pieces[i];
    		}
    		else if(maxPlayer->pieces[i]->type == "Queen")
    		{
    			playerMaterial += QUEENVALUE;
    			myHasQueen = true;
    		}
    		else if(maxPlayer->pieces[i]->type == "Rook")
    		{
    			playerMaterial += ROOKVALUE;
    			myRooks.push_back(maxPlayer->pieces[i]);
    		}
    		else if(maxPlayer->pieces[i]->type == "Bishop")
    		{
    			playerMaterial += BISHOPVALUE;
    		}
    		else if(maxPlayer->pieces[i]->type == "Knight")
    		{
    			playerMaterial += KNIGHTVALUE;
    		}
    		else if(maxPlayer->pieces[i]->type == "Pawn")
    		{
    			playerMaterial += PAWNVALUE;
    		}
    	}

    	//piece value calculation for minPlayer
		for(unsigned int i = 0; i < maxPlayer->opponent->pieces.size(); i++)
		{
			if(maxPlayer->opponent->pieces[i]->type == "King")
			{
				oppKing = maxPlayer->opponent->pieces[i];
			}
			else if(maxPlayer->opponent->pieces[i]->type == "Queen")
			{
				opponentMaterial += QUEENVALUE;
				oppHasQueen = true;
			}
			else if(maxPlayer->opponent->pieces[i]->type == "Rook")
			{
				opponentMaterial += ROOKVALUE;
				oppRooks.push_back(maxPlayer->opponent->pieces[i]);
			}
			else if(maxPlayer->opponent->pieces[i]->type == "Bishop")
			{
				opponentMaterial += BISHOPVALUE;
			}
			else if(maxPlayer->opponent->pieces[i]->type == "Knight")
			{
				opponentMaterial += KNIGHTVALUE;
			}
			else if(maxPlayer->opponent->pieces[i]->type == "Pawn")
			{
				opponentMaterial += PAWNVALUE;
			}
		}

		totalUtility += (playerMaterial - opponentMaterial);

		//Reward states where the king is safe and the queen is still in play
		int safetyDifference = kingSafetyUtility(myKing, oppKing);

		if(safetyDifference > SAFETY_THRESH)
		{
			if(myHasQueen)
				totalUtility += QUEEN_BONUS_WEAK_KING;
		}
		else if(safetyDifference < -SAFETY_THRESH)
		{
			if(oppHasQueen)
				totalUtility -= QUEEN_BONUS_WEAK_KING;
		}

		totalUtility += safetyDifference;

		//Reward states with valuable rook placement
		totalUtility += rookUtility(myRooks, oppRooks);

		//Update utility during the opening and closing of the game
		if(isOpening)
		{
			totalUtility += openingUtility();
		}
		else if(opponentMaterial < END_GAME_CUTOFF)
		{
			totalUtility += endingUtility();
		}
		return totalUtility;
	}
}

//This function updates the materialDifference variable so
//that it reflects the perspective of the current maxPlayer
void state::updateMaterialDifference()
{
	int playerMaterial = 0;
	int opponentMaterial = 0;

	//piece value calculation for maxPlayer
	for(unsigned int i = 0; i < maxPlayer->pieces.size(); i++)
	{
		if(maxPlayer->pieces[i]->type == "Queen")
		{
			playerMaterial += QUEENVALUE;
		}
		else if(maxPlayer->pieces[i]->type == "Rook")
		{
			playerMaterial += ROOKVALUE;
		}
		else if(maxPlayer->pieces[i]->type == "Bishop")
		{
			playerMaterial += BISHOPVALUE;
		}
		else if(maxPlayer->pieces[i]->type == "Knight")
		{
			playerMaterial += KNIGHTVALUE;
		}
		else if(maxPlayer->pieces[i]->type == "Pawn")
		{
			playerMaterial += PAWNVALUE;
		}
	}

	//piece value calculation for minPlayer
	for(unsigned int i = 0; i < maxPlayer->opponent->pieces.size(); i++)
	{
		if(maxPlayer->opponent->pieces[i]->type == "Queen")
		{
			opponentMaterial += QUEENVALUE;
		}
		else if(maxPlayer->opponent->pieces[i]->type == "Rook")
		{
			opponentMaterial += ROOKVALUE;
		}
		else if(maxPlayer->opponent->pieces[i]->type == "Bishop")
		{
			opponentMaterial += BISHOPVALUE;
		}
		else if(maxPlayer->opponent->pieces[i]->type == "Knight")
		{
			opponentMaterial += KNIGHTVALUE;
		}
		else if(maxPlayer->opponent->pieces[i]->type == "Pawn")
		{
			opponentMaterial += PAWNVALUE;
		}
	}

	materialDifference = (playerMaterial - opponentMaterial);
}

//Opening utility penalizes major pieces for not moving
//It also penalizes states that cannot castle or have yet to do so
//while rewarding a successful castle attempt
//The queen will be punished if it enters play too early
int state::openingUtility()
{
    const int NUDGE = -10;
    const int UNMOVED_PENALTY = -100;
    const int QUEEN_IS_EARLY = -50;
    const int CANT_CASTLE = -300;
    const int NOT_YET_CASTLE = -150;
    const int CASTLED = 2000;

    int myUtility = 0;
    int oppUtility = 0;
    myPiece* queen = NULL;
    vector<myPiece*> minorPieces;
    vector<myPiece*> rooks;
    myPiece* king = NULL;
    bool allPiecesDeveloped = true;

    //max player utility
    for(unsigned int i = 0; i < maxPlayer->pieces.size(); i++)
    {
        myPiece* piece = maxPlayer->pieces[i];
        if(piece->type == "King")
        {
            king = piece;
        }
        else if(piece->type == "Rook")
        {
            rooks.push_back(piece);
        }
        else if(piece->type == "Queen")
        {
            queen = piece;
        }
        else if(piece->type == "Bishop" || piece->type == "Knight")
        {
            minorPieces.push_back(piece);
        }
        else if(piece->type == "Pawn" &&
                (piece->file == "d" || piece->file == "e"))
        {
            minorPieces.push_back(piece);
        }
    }

    for(unsigned int i = 0; i < minorPieces.size(); i++)
    {
        if(!minorPieces[i]->hasMoved)
        {
            if(fileToInt(minorPieces[i]->file) > 4)
                myUtility += NUDGE;
            myUtility += UNMOVED_PENALTY;
            allPiecesDeveloped = false;
        }
    }

    if(queen != NULL)
        if(queen->hasMoved && !allPiecesDeveloped)
            myUtility += QUEEN_IS_EARLY;

    if(maxPlayer->rankDirection == 1)
    {
    	if(!whiteHasCastled)
    	{
    		myUtility += NOT_YET_CASTLE;
    		for(unsigned int i = 0; i < rooks.size(); i++)
    		{
    			if(rooks[i]->hasMoved)
    		        myUtility += CANT_CASTLE;
    		}
    		if(king->hasMoved)
    		    myUtility += CANT_CASTLE;
    	}
    	else
    		myUtility += CASTLED;
    }
    else
    {
    	if(!blackHasCastled)
    	{
    	    myUtility += NOT_YET_CASTLE;
    	    for(unsigned int i = 0; i < rooks.size(); i++)
    	    {
    	    	if(rooks[i]->hasMoved)
    	    	    myUtility += CANT_CASTLE;
    	    }
    	    if(king->hasMoved)
    	    	myUtility += CANT_CASTLE;
    	}
    	else
    		myUtility += CASTLED;
    }

    //opponent player utility
    allPiecesDeveloped = true;
    minorPieces.clear();
    rooks.clear();
    for(unsigned int i = 0; i < maxPlayer->opponent->pieces.size(); i++)
    {
        myPiece* piece = maxPlayer->opponent->pieces[i];
        if(piece->type == "King")
        {
            king = piece;
        }
        else if(piece->type == "Rook")
        {
            rooks.push_back(piece);
        }
        else if(piece->type == "Queen")
        {
            queen = piece;
        }
        else if(piece->type == "Bishop" || piece->type == "Knight")
        {
            minorPieces.push_back(piece);
        }
        else if(piece->type == "Pawn" &&
                (piece->file == "d" || piece->file == "e"))
        {
            minorPieces.push_back(piece);
        }
    }

    for(unsigned int i = 0; i < minorPieces.size(); i++)
    {
        if(!minorPieces[i]->hasMoved)
        {
            oppUtility += UNMOVED_PENALTY;
            allPiecesDeveloped = false;
        }
    }

    if(queen != NULL)
        if(queen->hasMoved && !allPiecesDeveloped)
            oppUtility += QUEEN_IS_EARLY;

    if(maxPlayer->opponent->rankDirection == 1)
    {
        if(!whiteHasCastled)
        {
        	oppUtility += NOT_YET_CASTLE;
        	for(unsigned int i = 0; i < rooks.size(); i++)
        	{
        		if(rooks[i]->hasMoved)
        		    oppUtility += CANT_CASTLE;
        	}
        	if(king->hasMoved)
        		oppUtility += CANT_CASTLE;
        }
        else
        	oppUtility += CASTLED;
    }
    else
    {
        if(!blackHasCastled)
        {
        	oppUtility += NOT_YET_CASTLE;
        	for(unsigned int i = 0; i < rooks.size(); i++)
        	{
        	    if(rooks[i]->hasMoved)
        	    	oppUtility += CANT_CASTLE;
        	}
        	if(king->hasMoved)
        	    oppUtility += CANT_CASTLE;
        }
        else
        	oppUtility += CASTLED;
    }

    return myUtility - oppUtility;
}

//The ending utility is based on minimizing the distance of the opponent's
//king to a corner quickly to encourage checkmate in the end game.
int state::endingUtility()
{
    const int CORNER_CHASE_WEIGHT = -10;
    int i = 0;
    int x = 0;
    int y = 0;
    int minimumDistance = 100;
    myPiece* king = NULL;
    while(king == NULL)
    {
        if(maxPlayer->opponent->pieces[i]->type == "King")
            king = maxPlayer->opponent->pieces[i];
        i++;
    }
    for(int i = 0; i < 4; i++)
    {
        switch (i)
        {
            case 0:
                x = 1;
                y = 1;
                break;
            case 1:
                x = 1;
                y = 8;
                break;
            case 2:
                x = 8;
                y = 1;
                break;
            case 3:
                x = 8;
                y = 8;
                break;
        }
        if(abs(king->rank - x) + abs(fileToInt(king->file) - y) < minimumDistance)
            minimumDistance = abs(king->rank - x) + abs(fileToInt(king->file) - y);
    }
    return minimumDistance * CORNER_CHASE_WEIGHT;
}

//The rook utility functions determines the parameters for each player's utility function
int state::rookUtility(vector<myPiece*>& myRooks, vector<myPiece*>& oppRooks)
{
    int myUtility = 0;
    int oppUtility = 0;

    if(maxPlayer->rankDirection == 1)
    {
        myUtility = rookUtilitySub(maxPlayer, myRooks, 7);
        oppUtility = rookUtilitySub(maxPlayer->opponent, oppRooks, 2);
    }
    else
    {
        myUtility = rookUtilitySub(maxPlayer, myRooks, 2);
        oppUtility = rookUtilitySub(maxPlayer->opponent, oppRooks, 7);
    }

    return myUtility - oppUtility;
}

//Returns true if the rooks are connected
//Connected rooks can move to the other rook's position
//which provides better protection for either rook
bool state::connected(vector<myPiece*>& rooks)
{
    if(rooks[0]->file != rooks[1]->file && rooks[0]->rank != rooks[1]->rank)
    {
        return false;
    }
    else if(rooks[0]->rank == rooks[1]->rank)
    {
        int file0 = fileToInt(rooks[0]->file);
        int file1 = fileToInt(rooks[1]->file);
        if(file0 > file1)
        {
            int tmp = file1;
            file1 = file0;
            file0 = tmp;
        }
        for(int i = 1; i < file1-file0; i++)
        {
            if(occupied(rooks[0]->rank, file0 + i, maxPlayer) != 0)
                return false;
        }
    }
    else if(rooks[0]->file == rooks[1]->file)
    {
        int rank0 = rooks[0]->rank;
        int rank1 = rooks[1]->rank;
        if(rank0 > rank1)
        {
            int tmp = rank1;
            rank1 = rank0;
            rank0 = tmp;
        }
        for(int i = 0; i < rank1-rank0; i++)
        {
            if(occupied(rank0+i, fileToInt(rooks[0]->file), maxPlayer) != 0)
                return false;
        }
    }
    return true;
}

//The rook utility rewards rooks that are connected and if they have access to
//an open file or the opponent's back line because these conditions allow
//safe attacks against undeployed opponent pieces
int state::rookUtilitySub(myPlayer* player, vector<myPiece*>& rooks, int enemyRank)
{
    const int ROOK_ON_7 = 200;
    const int ROOK_CONNECTED_ON_7 = 100;
    const int ROOK_OPEN_FILE = 100;
    const int ROOK_CONNECTED_OPEN_FILE = 50;
    const int ROOK_HALF_OPEN_FILE = 50;
    int utility = 0;
    int openFileType;
    bool areConnected = false;

    if(rooks.size() == 2)
    {
        areConnected = connected(rooks);
    }

    for(unsigned int i = 0; i < rooks.size(); i++)
    {
        openFileType = isOpenFile(rooks[i]->file, player);
        if(openFileType == -2)
        {
            utility += ROOK_OPEN_FILE;
            if(areConnected)
                utility += ROOK_CONNECTED_OPEN_FILE;
        }
        else if(openFileType == 2 || openFileType == -1)
            utility += ROOK_HALF_OPEN_FILE;

        if(rooks[i]->rank == enemyRank)
        {
            utility += ROOK_ON_7;
            if(areConnected)
            {
                utility += ROOK_CONNECTED_ON_7;
            }
        }
    }

    return utility;
}

//Determines the parameters of the king safety functions
int state::kingSafetyUtility(myPiece* myKing, myPiece* oppKing)
{
    const int SAFETY_MODIFIER = -25;

    int mySafety;
    int oppSafety;

    if(maxPlayer->rankDirection == 1)
    {
        mySafety = kingSafetyWhite(maxPlayer, myKing);
        oppSafety = kingSafetyBlack(maxPlayer->opponent, oppKing);
    }
    else
    {
        mySafety = kingSafetyBlack(maxPlayer, myKing);
        oppSafety = kingSafetyWhite(maxPlayer->opponent, oppKing);
    }

    return SAFETY_MODIFIER * (mySafety - oppSafety);
}

//King safety rewards
int state::kingSafetyBlack(myPlayer* black, myPiece* king)
{
    const int IN_CENTER = 5;
    const int OPEN_FILE = 6;
    const int NOT_CORNER = 2;
    const int IN_CORNER = 3;
    const int PAWN_ATTACK_BARRIER = 10;

    const int ROOK_PAWN_1 = 3;
    const int ROOK_PAWN_2 = 6;
    const int ROOK_PAWN_MISSING = 8;
    const int ROOK_PAWN_FAR = 8;
    const int ROOK_PAWN_OPEN = 10;

    const int KNIGHT_PAWN_1 = 2;
    const int KNIGHT_PAWN_2 = 5;
    const int KNIGHT_PAWN_MISSING = 8;
    const int KNIGHT_PAWN_FAR = 8;
    const int KNIGHT_PAWN_OPEN = 10;

    const int BISHOP_PAWN_1 = 1;
    const int BISHOP_PAWN_2 = 2;
    const int BISHOP_PAWN_MISSING = 2;
    const int BISHOP_PAWN_FAR = 2;

    string rookFile;
    string knightFile;
    string bishopFile;
    int safetyIssues = 0;
    int fileInfo;
    int progression;
    int file;

    //sets the target rook, knight, and bishop files to the side the king is on
    if(fileToInt(king->file) < 4)
    {
        rookFile = "a";
        knightFile = "b";
        bishopFile = "c";
    }
    else if(fileToInt(king->file) > 5)
    {
        rookFile = "h";
        knightFile = "g";
        bishopFile = "f";
    }
    //If the king is in the center it is penalized
    else
    {
        safetyIssues += IN_CENTER;
        //Additional penalties are added if the king is near open files with easy access
        //for opponent's pieces
        if(king->file == "d")
        {
            if(isOpenFile("c", black) < 0)
                safetyIssues += OPEN_FILE;
            if(isOpenFile("d", black) < 0)
                safetyIssues += OPEN_FILE;
            if(isOpenFile("e", black) < 0)
                safetyIssues += OPEN_FILE;
        }
        else
        {
            if(isOpenFile("d", black) < 0)
                safetyIssues += OPEN_FILE;
            if(isOpenFile("e", black) < 0)
                safetyIssues += OPEN_FILE;
            if(isOpenFile("f", black) < 0)
                safetyIssues += OPEN_FILE;
        }
    }

    if(!safetyIssues)
    {
        //Rook Pawn
    	//If the pawn on the rook's file is absent or far away
    	//the state is penalized
        fileInfo = isOpenFile(rookFile, black);
        if(fileInfo < 0)
            safetyIssues += ROOK_PAWN_MISSING;
        if(fileInfo == -2)
            safetyIssues += ROOK_PAWN_OPEN;
        else
        {
            progression = pawnProgression(rookFile, black);
            if(progression == 1)
                safetyIssues += ROOK_PAWN_1;
            else if(progression == 2)
                safetyIssues += ROOK_PAWN_2;
            else if(progression > 2)
                safetyIssues += ROOK_PAWN_FAR;
        }

        //Knight Pawn
        //If the pawn on the knight's file is absent or far away
        //the state is penalized
        fileInfo = isOpenFile(knightFile, black);
        if(fileInfo < 0)
            safetyIssues += KNIGHT_PAWN_MISSING;
        if(fileInfo == -2)
            safetyIssues += KNIGHT_PAWN_OPEN;
        else
        {
            progression = pawnProgression(knightFile, black);
            if(progression == 1)
                safetyIssues += KNIGHT_PAWN_1;
            else if(progression == 2)
                safetyIssues += KNIGHT_PAWN_2;
            else if(progression > 2)
                safetyIssues += KNIGHT_PAWN_FAR;
        }

        //Bishop Pawn
        //If the pawn on the bishop's file is absent or far away
        //the state is penalized
        if(safetyIssues > 1)
        {
            fileInfo = isOpenFile(bishopFile, black);
            if(abs(fileInfo) == 2)
                safetyIssues += BISHOP_PAWN_MISSING;
            else
            {
                progression = pawnProgression(bishopFile, black);
                if(progression == 1)
                    safetyIssues += BISHOP_PAWN_1;
                else if(progression == 2)
                    safetyIssues += BISHOP_PAWN_2;
                else if(progression > 2)
                    safetyIssues += BISHOP_PAWN_FAR;
            }
        }

        //check for pawn at g6/b6 that can break barrier
        for(unsigned int i = 0; i < black->opponent->pieces.size(); i++)
        {
            if(black->opponent->pieces[i]->rank == 6 && black->opponent->pieces[i]->file == knightFile)
            {
                safetyIssues += PAWN_ATTACK_BARRIER;
                break;
            }
        }

        //penalize king for not being near the corner and for being exactly in corner
        file = fileToInt(king->file);
        if((file > 2) && (file < 7)) safetyIssues += NOT_CORNER;
        if((file > 3) && (file < 6)) safetyIssues += NOT_CORNER;

        if(king->rank < 8) safetyIssues += NOT_CORNER;
        if(king->rank < 7) safetyIssues += NOT_CORNER;
        if(king->rank < 6) safetyIssues += NOT_CORNER;
        if(king->rank == 8 && (file == 1 || file == 8))
            safetyIssues += IN_CORNER;
    }

    return safetyIssues;
}

//Offers the same calculations as kingSafetyBlack but from the
//white pieces perspective
int state::kingSafetyWhite(myPlayer* white, myPiece* king)
{
    const int IN_CENTER = 5;
    const int OPEN_FILE = 6;
    const int NOT_CORNER = 2;
    const int IN_CORNER = 3;
    const int PAWN_ATTACK_BARRIER = 10;

    const int ROOK_PAWN_1 = 3;
    const int ROOK_PAWN_2 = 6;
    const int ROOK_PAWN_MISSING = 8;
    const int ROOK_PAWN_FAR = 8;
    const int ROOK_PAWN_OPEN = 10;

    const int KNIGHT_PAWN_1 = 2;
    const int KNIGHT_PAWN_2 = 5;
    const int KNIGHT_PAWN_MISSING = 8;
    const int KNIGHT_PAWN_FAR = 8;
    const int KNIGHT_PAWN_OPEN = 10;

    const int BISHOP_PAWN_1 = 1;
    const int BISHOP_PAWN_2 = 2;
    const int BISHOP_PAWN_MISSING = 2;
    const int BISHOP_PAWN_FAR = 2;

    string rookFile;
    string knightFile;
    string bishopFile;
    int safetyIssues = 0;
    int fileInfo;
    int progression;
    int file;

    if(fileToInt(king->file) < 4)
    {
        rookFile = "a";
        knightFile = "b";
        bishopFile = "c";
    }
    else if(fileToInt(king->file) > 5)
    {
        rookFile = "h";
        knightFile = "g";
        bishopFile = "f";
    }
    else
    {
        safetyIssues += IN_CENTER;
        if(king->file == "d")
        {
            if(isOpenFile("c", white) < 0)
                safetyIssues += OPEN_FILE;
            if(isOpenFile("d", white) < 0)
                safetyIssues += OPEN_FILE;
            if(isOpenFile("e", white) < 0)
                safetyIssues += OPEN_FILE;
        }
        else
        {
            if(isOpenFile("d", white) < 0)
                safetyIssues += OPEN_FILE;
            if(isOpenFile("e", white) < 0)
                safetyIssues += OPEN_FILE;
            if(isOpenFile("f", white) < 0)
                safetyIssues += OPEN_FILE;
        }
    }

    if(!safetyIssues)
    {
        //Rook Pawn
        fileInfo = isOpenFile(rookFile, white);
        if(fileInfo < 0)
            safetyIssues += ROOK_PAWN_MISSING;
        if(fileInfo == -2)
            safetyIssues += ROOK_PAWN_OPEN;
        else
        {
            progression = pawnProgression(rookFile, white);
            if(progression == 1)
                safetyIssues += ROOK_PAWN_1;
            else if(progression == 2)
                safetyIssues += ROOK_PAWN_2;
            else if(progression > 2)
                safetyIssues += ROOK_PAWN_FAR;
        }

        //Knight Pawn
        fileInfo = isOpenFile(knightFile, white);
        if(fileInfo < 0)
            safetyIssues += KNIGHT_PAWN_MISSING;
        if(fileInfo == -2)
            safetyIssues += KNIGHT_PAWN_OPEN;
        else
        {
            progression = pawnProgression(knightFile, white);
            if(progression == 1)
                safetyIssues += KNIGHT_PAWN_1;
            else if(progression == 2)
                safetyIssues += KNIGHT_PAWN_2;
            else if(progression > 2)
                safetyIssues += KNIGHT_PAWN_FAR;
        }

        //Bishop Pawn
        if(safetyIssues > 1)
        {
            fileInfo = isOpenFile(bishopFile, white);
            if(abs(fileInfo) == 2)
                safetyIssues += BISHOP_PAWN_MISSING;
            else
            {
                progression = pawnProgression(bishopFile, white);
                if(progression == 1)
                    safetyIssues += BISHOP_PAWN_1;
                else if(progression == 2)
                    safetyIssues += BISHOP_PAWN_2;
                else if(progression > 2)
                    safetyIssues += BISHOP_PAWN_FAR;
            }
        }

        //check for pawn at g3/b3 that can break barrier
        for(unsigned int i = 0; i < white->opponent->pieces.size(); i++)
        {
            if(white->opponent->pieces[i]->rank == 3 && white->opponent->pieces[i]->file == knightFile)
            {
                safetyIssues += PAWN_ATTACK_BARRIER;
                break;
            }
        }

        //penalize king for not being near the corner and for being exactly in corner
        file = fileToInt(king->file);
        if((file > 2) && (file < 7)) safetyIssues += NOT_CORNER;
        if((file > 3) && (file < 6)) safetyIssues += NOT_CORNER;

        if(king->rank > 1) safetyIssues += NOT_CORNER;
        if(king->rank > 2) safetyIssues += NOT_CORNER;
        if(king->rank > 3) safetyIssues += NOT_CORNER;
        if(king->rank == 1 && (file == 1 || file == 8))
            safetyIssues += IN_CORNER;
    }

    return safetyIssues;
}

//Generates all moves that the pointed piece can make
vector<action> state::generateKingMoves(myPiece* king)
{
    vector<action> result;
    const int DIRECTIONS = 8;
    int modRank;
    int modFile;
    action tmp;
    bool isSafe = true;

    bool ownPiece;

    //determine owner of piece
    if(king->owner != currentPlayer)
    {
        currentPlayer = king->owner;
        ownPiece = false;
    }
    else
    {
        ownPiece = true;
    }

    //store file as integer for easy manipulation
    int tmpFile = fileToInt(king->file);

    tmp.oldFile = king->file;
    tmp.oldRank = king->rank;
    tmp.type = "King";
    tmp.promotion = "";
    //Check Castling
    if(king->hasMoved == false)
    {
        if(!inDanger(king->rank, tmpFile, currentPlayer))
        {
            int direction;
            int limit;
            bool clear = true;
            for(unsigned int i = 0; i < currentPlayer->pieces.size(); i++)
            {
                if(currentPlayer->pieces[i]->type == "Rook" &&
                   currentPlayer->pieces[i]->hasMoved == false)
                {
                    if(currentPlayer->pieces[i]->file == "h")
                    {
                        direction = 1;
                        limit = 3;
                    }
                    else
                    {
                        direction = -1;
                        limit = 4;
                    }
                    for(int inc = 1; inc < limit; inc++)
                    {
                        if(inDanger(king->rank, tmpFile+(direction*inc), currentPlayer) ||
                           occupied(king->rank, tmpFile+(direction*inc), currentPlayer) != 0)
                            clear = false;
                    }
                    if(clear)
                    {
                        //castling is valid so store new action
                        tmp.isCastle = true;
                        tmp.newFile = intToFile(tmpFile+(direction*2));
                        tmp.newRank = king->rank;
                        result.push_back(tmp);
                    }
                }
            }
        }
    }
    tmp.isCastle = false;

    //Check Typical Moves
    for(int i = 0; i < DIRECTIONS; i++)
    {
        switch (i)
        {
            case 0:
                modFile = -1;
                modRank = -1;
                break;
            case 1:
                modFile = 0;
                modRank = -1;
                break;
            case 2:
                modFile = 1;
                modRank = -1;
                break;
            case 3:
                modFile = 1;
                modRank = 0;
                break;
            case 4:
                modFile = 1;
                modRank = 1;
                break;
            case 5:
                modFile = 0;
                modRank = 1;
                break;
            case 6:
                modFile = -1;
                modRank = 1;
                break;
            case 7:
                modFile = -1;
                modRank = 0;
                break;
        }

        if(ownPiece)
            isSafe = !inDanger(king->rank + modRank, tmpFile + modFile, currentPlayer);

        if(tmpFile + modFile >= 1 &&
           tmpFile + modFile <= 8 &&
           king->rank + modRank >= 1 &&
           king->rank + modRank <= 8 &&
           occupied(king->rank + modRank, tmpFile + modFile, currentPlayer) < 1 && isSafe)
        {
            tmp.newFile = intToFile(tmpFile+modFile);
            tmp.newRank = king->rank+modRank;
            if(validForCheck(ownPiece, tmp))
                result.push_back(tmp);
        }
    }

    //return control to current player if evaluating opponent piece
    if(!ownPiece)
    {
        currentPlayer = king->owner->opponent;
    }

    return result;
}

//Generates all moves that the pointed piece can make
vector<action> state::generateQueenMoves(myPiece* queen)
{
    vector<action> actions;
    vector<action> newMoves;

    //generate Rook moves and add them to the total actions
    newMoves = generateRookMoves(queen);
    for(unsigned int i = 0; i < newMoves.size(); i++)
    {
        newMoves[i].type = "Queen";
        actions.push_back(newMoves[i]);
    }

    //generate Bishop moves and add them to the total actions
    newMoves = generateBishopMoves(queen);
    for(unsigned int i = 0; i < newMoves.size(); i++)
    {
        newMoves[i].type = "Queen";
        actions.push_back(newMoves[i]);
    }

    return actions;
}

//Generates all moves that the pointed piece can make
vector<action> state::generateKnightMoves(myPiece* knight)
{
    vector<action> result;
    const int DIRECTIONS = 8;
    int modRank;
    int modFile;
    action tmp;

    bool ownPiece;

    //determine if owner is current player
    if(knight->owner != currentPlayer)
    {
        currentPlayer = knight->owner;
        ownPiece = false;
    }
    else
    {
        ownPiece = true;
    }

    //store file as integer for easy manipulation
    int tmpFile = fileToInt(knight->file);

    tmp.oldFile = knight->file;
    tmp.oldRank = knight->rank;
    tmp.type = "Knight";
    tmp.promotion = "";

    for(int i = 0; i < DIRECTIONS; i++)
    {
        switch (i)
        {
            case 0:
                modFile = -2;
                modRank = -1;
                break;
            case 1:
                modFile = -1;
                modRank = -2;
                break;
            case 2:
                modFile = 1;
                modRank = -2;
                break;
            case 3:
                modFile = 2;
                modRank = -1;
                break;
            case 4:
                modFile = 2;
                modRank = 1;
                break;
            case 5:
                modFile = 1;
                modRank = 2;
                break;
            case 6:
                modFile = -1;
                modRank = 2;
                break;
            case 7:
                modFile = -2;
                modRank = 1;
                break;
        }

        if(tmpFile + modFile >= 1 &&
           tmpFile + modFile <= 8 &&
           knight->rank + modRank >= 1 &&
           knight->rank + modRank <= 8 &&
           occupied(knight->rank + modRank, tmpFile + modFile, currentPlayer) < 1)
        {
            tmp.newFile = intToFile(tmpFile+modFile);
            tmp.newRank = knight->rank+modRank;
            if(validForCheck(ownPiece, tmp))
                result.push_back(tmp);
        }
    }

    //return control to current player if evaluating opponent piece
    if(!ownPiece)
    {
        currentPlayer = knight->owner->opponent;
    }
    return result;
}

//Generates all moves that the pointed piece can make
vector<action> state::generateRookMoves(myPiece* rook)
{
    vector<action> result;
    const int DIRECTIONS = 4;
    int modRank;
    int modFile;
    action tmp;
    int inc;

    bool ownPiece;

    //determine if owner is current player
    if(rook->owner != currentPlayer)
    {
        currentPlayer = rook->owner;
        ownPiece = false;
    }
    else
    {
        ownPiece = true;
    }

    //store file as integer for easy manipulation
    int tmpFile = fileToInt(rook->file);

    tmp.oldFile = rook->file;
    tmp.oldRank = rook->rank;
    tmp.type = "Rook";
    tmp.promotion = "";

    for(int i = 0; i < DIRECTIONS; i++)
    {
        inc = 1;
        switch (i)
        {
            case 0:
                modFile = 0;
                modRank = -1;
                break;
            case 1:
                modFile = 1;
                modRank = 0;
                break;
            case 2:
                modFile = 0;
                modRank = 1;
                break;
            case 3:
                modFile = -1;
                modRank = 0;
                break;
        }

        //iterate in all directions
        while(tmpFile + (inc*modFile) >= 1 &&
              tmpFile + (inc*modFile) <= 8 &&
              rook->rank + (inc*modRank) >= 1 &&
              rook->rank + (inc*modRank) <= 8 &&
              occupied(rook->rank + (inc*modRank), tmpFile + (inc*modFile), currentPlayer) == 0)
        {
            tmp.newFile = intToFile(tmpFile+(inc*modFile));
            tmp.newRank = rook->rank+(inc*modRank);
            if(validForCheck(ownPiece, tmp))
                result.push_back(tmp);
            inc++;
        }

        if(occupied(rook->rank + (inc*modRank), tmpFile + (inc*modFile), currentPlayer) == -1)
        {
            tmp.newFile = intToFile(tmpFile+(inc*modFile));
            tmp.newRank = rook->rank+(inc*modRank);
            if(validForCheck(ownPiece, tmp))
                result.push_back(tmp);
        }
    }

    //return control to current player if evaluating opponent piece
    if(!ownPiece)
    {
        currentPlayer = rook->owner->opponent;
    }
    return result;
}

//Generates all moves that the pointed piece can make
vector<action> state::generateBishopMoves(myPiece* bishop)
{
    vector<action> result;
    const int DIRECTIONS = 4;
    int modRank;
    int modFile;
    action tmp;
    int inc;

    bool ownPiece;

    //determine if owner is current player
    if(bishop->owner != currentPlayer)
    {
        currentPlayer = bishop->owner;
        ownPiece = false;
    }
    else
    {
        ownPiece = true;
    }

    //store file as integer for easy manipulation
    int tmpFile = fileToInt(bishop->file);

    tmp.oldFile = bishop->file;
    tmp.oldRank = bishop->rank;
    tmp.type = "Bishop";
    tmp.promotion = "";

    for(int i = 0; i < DIRECTIONS; i++)
    {
        inc = 1;
        switch (i)
        {
            case 0:
                modFile = -1;
                modRank = -1;
                break;
            case 1:
                modFile = 1;
                modRank = -1;
                break;
            case 2:
                modFile = 1;
                modRank = 1;
                break;
            case 3:
                modFile = -1;
                modRank = 1;
                break;
        }

        //iterate in all directions
        while(tmpFile + (inc*modFile) >= 1 &&
              tmpFile + (inc*modFile) <= 8 &&
              bishop->rank + (inc*modRank) >= 1 &&
              bishop->rank + (inc*modRank) <= 8 &&
              occupied(bishop->rank + (inc*modRank), tmpFile + (inc*modFile), currentPlayer) == 0)
        {
            tmp.newFile = intToFile(tmpFile+(inc*modFile));
            tmp.newRank = bishop->rank+(inc*modRank);
            if(validForCheck(ownPiece, tmp))
                result.push_back(tmp);
            inc++;
        }

        if(occupied(bishop->rank + (inc*modRank), tmpFile + (inc*modFile), currentPlayer) == -1)
        {
            tmp.newFile = intToFile(tmpFile+(inc*modFile));
            tmp.newRank = bishop->rank+(inc*modRank);
            if(validForCheck(ownPiece, tmp))
                result.push_back(tmp);
        }
    }

    //return control to current player if evaluating opponent piece
    if(!ownPiece)
    {
        currentPlayer = bishop->owner->opponent;
    }
    return result;
}


//Generates all moves that the pointed piece can make
vector<action> state::generatePawnMoves(myPiece* pawn)
{
    vector<action> actions;
    action tmp;
    tmp.oldFile = pawn->file;
    tmp.oldRank = pawn->rank;
    tmp.type = "Pawn";
    state test;

    bool ownPiece;

    //determine if owner is current player
    if(pawn->owner != currentPlayer)
    {
        currentPlayer = pawn->owner;
        ownPiece = false;
    }
    else
    {
        ownPiece = true;
    }

    //store file as integer for easy manipulation
    int tmpFile = fileToInt(pawn->file);

    //en passant white
    if(currentPlayer->rankDirection == 1 && pawn->rank == 5)
    {
        if(previousActions.back().type == "Pawn" &&
           previousActions.back().oldRank == 7 && previousActions.back().newRank == 5 &&
           (fileToInt(previousActions.back().newFile) == tmpFile+1 ||
            fileToInt(previousActions.back().newFile) == tmpFile-1))
        {
            tmp.isEnPassant = true;
            tmp.newFile = previousActions.back().newFile;
            tmp.newRank = pawn->rank + currentPlayer->rankDirection;
            if(validForCheck(ownPiece, tmp))
                actions.push_back(tmp);
        }
    }
    tmp.isEnPassant = false;

    //en passant black
    if(currentPlayer->rankDirection == -1 && pawn->rank == 4)
    {
        if(previousActions.back().type == "Pawn" &&
           previousActions.back().oldRank == 2 && previousActions.back().newRank == 4 &&
           (fileToInt(previousActions.back().newFile) == tmpFile+1 ||
            fileToInt(previousActions.back().newFile) == tmpFile-1))
        {
            tmp.isEnPassant = true;
            tmp.newFile = previousActions.back().newFile;
            tmp.newRank = pawn->rank + currentPlayer->rankDirection;
            if(validForCheck(ownPiece, tmp))
                actions.push_back(tmp);
        }
    }
    tmp.isEnPassant = false;

    //forward movement check
    if(occupied(pawn->rank + currentPlayer->rankDirection, tmpFile, currentPlayer) == 0)
    {
        tmp.newFile = pawn->file;
        tmp.newRank = pawn->rank + currentPlayer->rankDirection;
        //promotion check
        if(tmp.newRank == 8 || tmp.newRank == 1)
        {
            tmp.promotion = "Queen";
        }
        else
        {
            tmp.promotion = "";
        }
        //if move doesn't result in check add to actions
        if(validForCheck(ownPiece, tmp))
            actions.push_back(tmp);

        //check if first move can be 2 spaces
        if(occupied(pawn->rank + (currentPlayer->rankDirection * 2), tmpFile, currentPlayer) == 0
           && pawn->hasMoved == false && ((currentPlayer->rankDirection == 1 && pawn->rank == 2) ||
           (currentPlayer->rankDirection == -1 && pawn->rank == 7)))
        {
            tmp.newFile = pawn->file;
            tmp.newRank = pawn->rank + (currentPlayer->rankDirection * 2);
            tmp.promotion = "";
            if(validForCheck(ownPiece, tmp))
                actions.push_back(tmp);
        }
    }

    //capture check right
    if((occupied(pawn->rank + currentPlayer->rankDirection, tmpFile + 1, currentPlayer) == -1 &&
       tmpFile + 1 <= 8) || !ownPiece)
    {
        tmp.newFile = intToFile(tmpFile+1);
        tmp.newRank = pawn->rank + currentPlayer->rankDirection;
        //promotion check
        if(tmp.newRank == 8 || tmp.newRank == 1)
        {
            tmp.promotion = "Queen";
        }
        else
        {
            tmp.promotion = "";
        }
        if(validForCheck(ownPiece, tmp))
            actions.push_back(tmp);
    }

    //capture check left
    if((occupied(pawn->rank + currentPlayer->rankDirection, tmpFile - 1, currentPlayer) == -1 &&
       tmpFile - 1 >= 1) || !ownPiece)
    {
        tmp.newFile = intToFile(tmpFile-1);
        tmp.newRank = pawn->rank + currentPlayer->rankDirection;
        //promotion check
        if(tmp.newRank == 8 || tmp.newRank == 1)
        {
            tmp.promotion = "Queen";
        }
        else
        {
            tmp.promotion = "";
        }
        if(validForCheck(ownPiece, tmp))
            actions.push_back(tmp);
    }

    //return control to current player if evaluating opponent piece
    if(!ownPiece)
    {
        currentPlayer = pawn->owner->opponent;
    }
    return actions;
}

//Returns 1 if the space contains a friendly piece
//Returns 0 if the space is empty
//Returns -1 if the space contains an enemy piece
int state::occupied(const int rank, const int file, const myPlayer* player)
{
    //search friendly pieces
    for(unsigned int i = 0; i < player->pieces.size(); i++)
    {
        if(player->pieces[i]->rank == rank &&
           fileToInt(player->pieces[i]->file) == file)
        {
            return 1;
        }
    }

    //search opponent pieces
    for(unsigned int i = 0; i < player->opponent->pieces.size(); i++)
    {
        if(player->opponent->pieces[i]->rank == rank &&
           fileToInt(player->opponent->pieces[i]->file) == file)
        {
            return -1;
        }

    }
    //no pieces on tile
    return 0;
}

//Returns true if an enemy piece can capture the given rank and file tile
//Returns false if the tile is safe for the player
bool state::inDanger(const int rank, const int file, const myPlayer* player)
{
    int inc;
    int modFile;
    int modRank;
    string attackerType;

    //check for opponent pieces along diagonals
    for(int i = 0; i < 4; i++)
    {
        inc = 1;
        switch (i)
        {
            case 0:
                modFile = -1;
                modRank = -1;
                break;
            case 1:
                modFile = 1;
                modRank = -1;
                break;
            case 2:
                modFile = 1;
                modRank = 1;
                break;
            case 3:
                modFile = -1;
                modRank = 1;
                break;
        }

        while(file + (inc*modFile) >= 1 &&
              file + (inc*modFile) <= 8 &&
              rank + (inc*modRank) >= 1 &&
              rank + (inc*modRank) <= 8 &&
              occupied(rank + (inc*modRank), file + (inc*modFile), player) == 0)
        {
            inc++;
        }

        if(occupied(rank + (inc*modRank), file + (inc*modFile), player) == -1)
        {
            attackerType = getType(rank + (inc*modRank), intToFile(file + (inc*modFile)));
            if((attackerType == "King" && inc == 1) ||
               attackerType == "Queen" ||
               attackerType == "Bishop")
            {
                //opponent pieces can capture this tile
                return true;
            }
            else if(attackerType == "Pawn" &&
                    modRank != player->opponent->rankDirection &&
                    inc == 1)
            {
                //opponent piece can capture this tile
                return true;
            }
        }
    }

    //check for opponent pieces along horizontal and verticals
    for(int i = 0; i < 4; i++)
    {
        inc = 1;
        switch (i)
        {
            case 0:
                modFile = 0;
                modRank = -1;
                break;
            case 1:
                modFile = 1;
                modRank = 0;
                break;
            case 2:
                modFile = 0;
                modRank = 1;
                break;
            case 3:
                modFile = -1;
                modRank = 0;
                break;
        }

        while(file + (inc*modFile) >= 1 &&
              file + (inc*modFile) <= 8 &&
              rank + (inc*modRank) >= 1 &&
              rank + (inc*modRank) <= 8 &&
              occupied(rank + (inc*modRank), file + (inc*modFile), player) == 0)
        {
            inc++;
        }

        if(occupied(rank + (inc*modRank), file + (inc*modFile), player) == -1)
        {
            attackerType = getType(rank + (inc*modRank), intToFile(file + (inc*modFile)));
            if((attackerType == "King" && inc == 1) ||
               attackerType == "Queen" ||
               attackerType == "Rook")
            {
                //opponent pieces can capture this tile
                return true;
            }
        }
    }

    //check for knights that can take the current tile
    for(int i = 0; i < 8; i++)
    {
        switch (i)
        {
            case 0:
                modFile = -2;
                modRank = -1;
                break;
            case 1:
                modFile = -1;
                modRank = -2;
                break;
            case 2:
                modFile = 1;
                modRank = -2;
                break;
            case 3:
                modFile = 2;
                modRank = -1;
                break;
            case 4:
                modFile = 2;
                modRank = 1;
                break;
            case 5:
                modFile = 1;
                modRank = 2;
                break;
            case 6:
                modFile = -1;
                modRank = 2;
                break;
            case 7:
                modFile = -2;
                modRank = 1;
                break;
        }

        if(file + modFile >= 1 &&
           file + modFile <= 8 &&
           rank + modRank >= 1 &&
           rank + modRank <= 8 &&
           occupied(rank + modRank, file + modFile, player) == -1)
        {
            if(getType(rank+modRank, intToFile(file+modFile)) == "Knight")
            {
                //an opponent knight can capture this tile
                return true;
            }
        }
    }

    return false;
}

//Returns true if the actions should be added to the list of all actions
//returns false if the action results in check for the current player
bool state::validForCheck(bool isOwnPiece, action a)
{
    state test;
    if(isOwnPiece)
    {
        test = result(a, false, 0, false, false);
        if(!test.isCheck(test.currentPlayer->opponent))
            return true;
    }
    else
    {
        return true;
    }
    return false;
}

//Returns true if the state is in check for player
//returns false if the state is out of check
bool state::isCheck(const myPlayer* player)
{
	unsigned int i = 0;
    //find index of current player king
    while(i < player->pieces.size() && player->pieces[i]->type != "King")
    {
        i++;
    }
    int file = fileToInt(player->pieces[i]->file);
    if(inDanger(player->pieces[i]->rank, file, player))
    {
        //king can be captured
        return true;
    }
    else
    {
        //king is not threatened
        return false;
    }
}

//returns true if in the last 8 moves, the first 4 match the last 4
bool state::isDraw()
{
    if(previousActions.size() < 8)
        return false;
    for(int i = 0; i < 4; i++)
    {
        if(previousActions[i].oldFile != previousActions[i+4].oldFile ||
           previousActions[i].oldRank != previousActions[i+4].oldRank ||
           previousActions[i].newFile != previousActions[i+4].newFile ||
           previousActions[i].newRank != previousActions[i+4].newRank ||
           previousActions[i].type != previousActions[i+4].type)
        {
            return false;
        }
    }
    return true;
}

//returns whether the targetFile is open
//return value of 2 is a file with only opponent pawns
//return value of 1 is a file with both player's pawns
//return value of -1 is a file with only the player's pawns
//return value of -2 is a file with no pawns
int state::isOpenFile(string targetFile, myPlayer* player)
{
    bool playerPawns = false;
    bool opponentPawns = false;
    for(unsigned int i = 0; i < player->pieces.size(); i++)
    {
        if(player->pieces[i]->type == "Pawn" && player->pieces[i]->file == targetFile)
        {
            playerPawns = true;
            break;
        }
    }
    for(unsigned int i = 0; i < player->opponent->pieces.size(); i++)
    {
        if(player->opponent->pieces[i]->type == "Pawn" && player->opponent->pieces[i]->file == targetFile)
        {
            opponentPawns = true;
            break;
        }
    }
    if(playerPawns && opponentPawns)
    {
        return 1;
    }
    if(!playerPawns && opponentPawns)
    {
        return 2;
    }
    if(playerPawns && !opponentPawns)
    {
        return -1;
    }
    if(!playerPawns && !opponentPawns)
    {
        return -2;
    }
    return 0;
}

//returns how far a pawn has progressed in a given file
int state::pawnProgression(string targetFile, myPlayer* player)
{
    for(unsigned int i = 0; i < player->pieces.size(); i++)
    {
        if(player->pieces[i]->type == "Pawn" && player->pieces[i]->file == targetFile)
        {
            if(player->rankDirection == 1)
                return player->pieces[i]->rank - 2;
            else
                return 7 - player->pieces[i]->rank;
        }
    }
    return 0;
}

//This function returns the number of squares around the king which are invalid for
//the king to move to. It is not yet used in the utility function.
int state::pinnedSquares(const myPlayer* player)
{
    int modFile;
    int modRank;
    int numberPinned=0;
    unsigned int i = 0;
    //find index of player king
    while(i < player->pieces.size() && player->pieces[i]->type != "King")
    {
        i++;
    }
    int file = fileToInt(player->pieces[i]->file);
    for(int j = 0; j < 8; j++)
    {
        switch (j)
        {
            case 0:
                modFile = -1;
                modRank = -1;
                break;
            case 1:
                modFile = 0;
                modRank = -1;
                break;
            case 2:
                modFile = 1;
                modRank = -1;
                break;
            case 3:
                modFile = 1;
                modRank = 0;
                break;
            case 4:
                modFile = 1;
                modRank = 1;
                break;
            case 5:
                modFile = 0;
                modRank = 1;
                break;
            case 6:
                modFile = -1;
                modRank = 1;
                break;
            case 7:
                modFile = -1;
                modRank = 0;
                break;
        }

        if(file+modFile >= 1 && file+modFile <= 8 &&
           player->pieces[i]->rank+modRank >= 1 && player->pieces[i]->rank+modRank <= 8 &&
           (inDanger(player->pieces[i]->rank+modRank, file+modFile, player) ||
            occupied(player->pieces[i]->rank+modRank, file+modFile, player) == 1))
        {
            numberPinned++;
        }
    }
    return numberPinned;
}

//Removes the piece that is at the given rank/file combination
void state::removeTakenPiece(const int rank, const string file)
{
	unsigned int i = 0;
    int positionRemoved = -1;
    int materialChange = 0;
    //find index of removed piece
    while(i < currentPlayer->opponent->pieces.size() && positionRemoved == -1)
    {
        if(currentPlayer->opponent->pieces[i]->rank == rank &&
           currentPlayer->opponent->pieces[i]->file == file)
        {
            positionRemoved = i;
        }
        i++;
    }

    switch(currentPlayer->opponent->pieces[positionRemoved]->type[0])
    {
    	case 'Q':
    		materialChange = QUEENVALUE;
    		break;
    	case 'R':
    		materialChange = ROOKVALUE;
    		break;
    	case 'P':
    		materialChange = PAWNVALUE;
    		break;
    	case 'K':
    		materialChange = KNIGHTVALUE;
    		break;
    	case 'B':
    		materialChange = BISHOPVALUE;
    }

    if(currentPlayer == maxPlayer)
    {
    	materialDifference += materialChange;
    }
    else
    {
    	materialDifference -= materialChange;
    }

    //delete removed piece from state
    delete currentPlayer->opponent->pieces[positionRemoved];

    currentPlayer->opponent->pieces.erase(currentPlayer->opponent->pieces.begin()+positionRemoved);

    return;
}

//Returns the type of the piece at the given rank/file pair
string state::getType(const int rank, const string file)
{
    for(unsigned int i = 0; i < currentPlayer->pieces.size(); i++)
    {
        if(currentPlayer->pieces[i]->file == file &&
           currentPlayer->pieces[i]->rank == rank)
            return currentPlayer->pieces[i]->type;
    }
    for(unsigned int i = 0; i < currentPlayer->opponent->pieces.size(); i++)
    {
        if(currentPlayer->opponent->pieces[i]->file == file &&
           currentPlayer->opponent->pieces[i]->rank == rank)
            return currentPlayer->opponent->pieces[i]->type;
    }
    //return "" if the space is empty
    return "";
}

//converts the file into an integer
int state::fileToInt(string file)
{
    if(file == "a")
        return 1;
    else if(file == "b")
        return 2;
    else if(file == "c")
        return 3;
    else if(file == "d")
        return 4;
    else if(file == "e")
        return 5;
    else if(file == "f")
        return 6;
    else if(file == "g")
        return 7;
    else if(file == "h")
        return 8;
    else
        return -1;
}

//converts the integer into a file string
string state::intToFile(int file)
{
    if(file == 1)
        return "a";
    else if(file == 2)
        return "b";
    else if(file == 3)
        return "c";
    else if(file == 4)
        return "d";
    else if(file == 5)
        return "e";
    else if(file == 6)
        return "f";
    else if(file == 7)
        return "g";
    else if(file == 8)
        return "h";
    else
        return "";
}

void game::initializeBoard()
{
	current_state.currentPlayer = current_state.players[0];
	current_state.previousActions.clear();
	current_state.whiteHasCastled = false;
	current_state.blackHasCastled = false;

	//Create White Pieces

	//Create Pawns
	for(int i = 1; i <= 8; i++)
	{
		myPiece* tmp;
		tmp = new myPiece;
		tmp->rank = 2;
		tmp->file = current_state.intToFile(i);
		tmp->hasMoved = false;
		tmp->owner = current_state.players[0];
		tmp->type = "Pawn";
		current_state.players[0]->pieces.push_back(tmp);
	}
	//Create Rooks
	myPiece* tmp;
	tmp = new myPiece;
	tmp->rank = 1;
	tmp->file = current_state.intToFile(1);
	tmp->hasMoved = false;
	tmp->owner = current_state.players[0];
	tmp->type = "Rook";
	current_state.players[0]->pieces.push_back(tmp);

	tmp = new myPiece;
	tmp->rank = 1;
	tmp->file = current_state.intToFile(8);
	tmp->hasMoved = false;
	tmp->owner = current_state.players[0];
	tmp->type = "Rook";
	current_state.players[0]->pieces.push_back(tmp);

	//Create Knights
	tmp = new myPiece;
	tmp->rank = 1;
	tmp->file = current_state.intToFile(2);
	tmp->hasMoved = false;
	tmp->owner = current_state.players[0];
	tmp->type = "Knight";
	current_state.players[0]->pieces.push_back(tmp);

	tmp = new myPiece;
	tmp->rank = 1;
	tmp->file = current_state.intToFile(7);
	tmp->hasMoved = false;
	tmp->owner = current_state.players[0];
	tmp->type = "Knight";
	current_state.players[0]->pieces.push_back(tmp);

	//Create Bishops
	tmp = new myPiece;
	tmp->rank = 1;
	tmp->file = current_state.intToFile(3);
	tmp->hasMoved = false;
	tmp->owner = current_state.players[0];
	tmp->type = "Bishop";
	current_state.players[0]->pieces.push_back(tmp);

	tmp = new myPiece;
	tmp->rank = 1;
	tmp->file = current_state.intToFile(6);
	tmp->hasMoved = false;
	tmp->owner = current_state.players[0];
	tmp->type = "Bishop";
	current_state.players[0]->pieces.push_back(tmp);

	//Create Queen
	tmp = new myPiece;
	tmp->rank = 1;
	tmp->file = current_state.intToFile(4);
	tmp->hasMoved = false;
	tmp->owner = current_state.players[0];
	tmp->type = "Queen";
	current_state.players[0]->pieces.push_back(tmp);

	//Create King
	tmp = new myPiece;
	tmp->rank = 1;
	tmp->file = current_state.intToFile(5);
	tmp->hasMoved = false;
	tmp->owner = current_state.players[0];
	tmp->type = "King";
	current_state.players[0]->pieces.push_back(tmp);

	//Create Black Pieces
	//Create Pawns
	for(int i = 1; i <= 8; i++)
	{
		myPiece* tmp;
		tmp = new myPiece;
		tmp->rank = 7;
		tmp->file = current_state.intToFile(i);
		tmp->hasMoved = false;
		tmp->owner = current_state.players[1];
		tmp->type = "Pawn";
		current_state.players[1]->pieces.push_back(tmp);
	}

	//Create Rooks
	tmp = new myPiece;
	tmp->rank = 8;
	tmp->file = current_state.intToFile(1);
	tmp->hasMoved = false;
	tmp->owner = current_state.players[1];
	tmp->type = "Rook";
	current_state.players[1]->pieces.push_back(tmp);

	tmp = new myPiece;
	tmp->rank = 8;
	tmp->file = current_state.intToFile(8);
	tmp->hasMoved = false;
	tmp->owner = current_state.players[1];
	tmp->type = "Rook";
	current_state.players[1]->pieces.push_back(tmp);

	//Create Knights
	tmp = new myPiece;
	tmp->rank = 8;
	tmp->file = current_state.intToFile(2);
	tmp->hasMoved = false;
	tmp->owner = current_state.players[1];
	tmp->type = "Knight";
	current_state.players[1]->pieces.push_back(tmp);

	tmp = new myPiece;
	tmp->rank = 8;
	tmp->file = current_state.intToFile(7);
	tmp->hasMoved = false;
	tmp->owner = current_state.players[1];
	tmp->type = "Knight";
	current_state.players[1]->pieces.push_back(tmp);

	//Create Bishops
	tmp = new myPiece;
	tmp->rank = 8;
	tmp->file = current_state.intToFile(3);
	tmp->hasMoved = false;
	tmp->owner = current_state.players[1];
	tmp->type = "Bishop";
	current_state.players[1]->pieces.push_back(tmp);

	tmp = new myPiece;
	tmp->rank = 8;
	tmp->file = current_state.intToFile(6);
	tmp->hasMoved = false;
	tmp->owner = current_state.players[1];
	tmp->type = "Bishop";
	current_state.players[1]->pieces.push_back(tmp);

	//Create Queen
	tmp = new myPiece;
	tmp->rank = 8;
	tmp->file = current_state.intToFile(4);
	tmp->hasMoved = false;
	tmp->owner = current_state.players[1];
	tmp->type = "Queen";
	current_state.players[1]->pieces.push_back(tmp);

	//Create King
	tmp = new myPiece;
	tmp->rank = 8;
	tmp->file = current_state.intToFile(5);
	tmp->hasMoved = false;
	tmp->owner = current_state.players[1];
	tmp->type = "King";
	current_state.players[1]->pieces.push_back(tmp);
}

//return true if the move is valid and false otherwise
bool game::valid_move(action move)
{
	vector<action> actions;
	actions = current_state.actions();
	for(unsigned int i = 0; i < actions.size(); i++)
	{
		if(move == actions[i])
			return true;
	}
	return false;
}

//Updates the game's current state and increments the turn counter
void game::update(action move)
{
	current_state = current_state.result(move, false, 0, false, false);
	currentTurn++;
	return;
}

//returns the associated return code for why a game ended or if it
//is not yet over
int game::is_game_over()
{
	//Return codes
	//0: Not terminal state
	//1: Draw by repeated move sequence
	//2: Checkmate
	//3: Stalemate
	//4: Timeout White
	//5: Timeout Black
	bool repeated_moves = true;
	//Check for 8 move repetition
	if(move_log.size() >= 8)
	{
		for(int i = 0; i < 4; i++)
		{
			if(!(move_log[move_log.size()-8+i] == move_log[move_log.size()-4+i]))
				repeated_moves = false;
		}
	}
	else
		repeated_moves = false;

	if(repeated_moves)
		return 1;

	//Check for possible moves
	vector<action> actions;
	actions = current_state.actions(true);

	if(actions.size() > 0)
	{
		if(whiteTimeRemaining <= 0)
			return 4;
		if(blackTimeRemaining <= 0)
			return 5;
		return 0;
	}

	if(current_state.isCheck(current_state.currentPlayer))
		return 2;
	else
		return 3;
}

//prints the current board, previous move, turn player's piece color, and
//remaining time for move planning for both players to the screen
void game::renderGame()
{
	cout << current_state << endl;
	cout << "Current Turn: " << currentTurn << endl;
	if(move_log.size() > 0)
		cout << "Previous Move: " << move_log.back() << endl;
	cout << "Current Player: ";
	if(current_state.currentPlayer->rankDirection == 1)
		cout << "White" << endl;
	else
		cout << "Black" << endl;
	int whiteT = whiteTimeRemaining;
	int blackT = blackTimeRemaining;
	cout << "White Time Remaining: " << whiteT/60 << " minutes " << whiteT%60 << " seconds" << endl;
	cout << "Black Time Remaining: " << blackT/60 << " minutes " << blackT%60 << " seconds" << endl;
	return;
}

//Prints a message to the screen based on the exit code provided
//by is_game_over() explaining the reason for a finished game
void game::printVictoryResults()
{
	int victoryCode = is_game_over();
	string winner;

	cout << "Game Over." << endl;

	if(current_state.currentPlayer->rankDirection == 1)
	{
		winner = "Black";
	}
	else
	{
		winner = "White";
	}

	switch(victoryCode)
	{
		case 1:
			cout << "Draw because of repeated move sequence." << endl;
			break;
		case 2:
			cout << winner << " Player Wins by Checkmate." << endl;
			break;
		case 3:
			cout << "Stalemate. No moves possible outside of check." << endl;
			break;
		case 4:
			cout << "Black wins because White time is exhausted." << endl;
			break;
		case 5:
			cout << "White wins because Black time is exhausted." << endl;
			break;
	}
}

bool action::operator==(const action& rhs)
{
    return oldFile == rhs.oldFile &&
           oldRank == rhs.oldRank &&
           newFile == rhs.newFile &&
           newRank == rhs.newRank &&
           type == rhs.type;
}

//Print out a state
ostream& operator<<(ostream&, const state& s)
{
    for (int rank = 9; rank >= -1; rank--)
    {
        string str = "";
        if (rank == 9 || rank == 0) // then the top or bottom of the board
        {
            str = "   +------------------------+";
        }
        else if (rank == -1) // then show the ranks
        {
            str = "     a  b  c  d  e  f  g  h";
        }
        else // board
        {
            str += " ";
            str += to_string(rank);
            str += " |";
            // fill in all the files with pieces at the current rank
            for (int fileOffset = 0; fileOffset < 8; fileOffset++)
            {
                string file(1, (char)(((int)"a"[0]) + fileOffset)); // start at a, with with file offset increasing the char;
                myPiece* currentPiece = nullptr;
                for (auto piece : s.currentPlayer->pieces)
                {
                    if (piece->file == file && piece->rank == rank) // then we found the piece at (file, rank)
                    {
                        currentPiece = piece;
                        break;
                    }
                }

                for (auto piece : s.currentPlayer->opponent->pieces)
                {
                    if (piece->file == file && piece->rank == rank)
                    {
                        currentPiece = piece;
                        break;
                    }
                }



                char code = '.'; // default "no piece";
                if (currentPiece != nullptr)
                {
                    code = currentPiece->type[0];

                    if (currentPiece->type == "Knight") // 'K' is for "King", we use 'N' for "Knights"
                    {
                        code = 'N';
                    }

                    if (currentPiece->owner->rankDirection == -1) // the second player (black) is lower case. Otherwise it's upppercase already
                    {
                        code = tolower(code);
                    }
                }

                str += " ";
                str += code;
                str += " ";
            }

            str += "|";
        }

        cout << str << endl;
    }
    return cout;
}

ostream& operator<<(ostream&, const action& a)
{
    return cout << a.type << " (" << a.oldFile << a.oldRank << ") to ("
             << a.newFile << a.newRank << ")" << endl;
}


