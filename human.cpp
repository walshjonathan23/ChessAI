/*
 * human.cpp
 * This file contains the function implementation of the humanPlayer class
 */

#include "human.h"
#include "ai.h"
#include "game.h"
#include <iostream>

//Prompts a human player to select moves based on standard input prompts
action humanPlayer::runTurn()
{
	action next_move;
	vector<action> possible_moves;
	unsigned int i = 0;

	//Calculates the moves that a player could select to validate a selection
	possible_moves = target_game->current_state.actions(false);

	//Prompts for an action
	cout << "Enter File (a-f) of Piece to be Moved: ";
	cin >> next_move.oldFile;
	cout << "Enter Rank (1-8) of Piece to be Moved: ";
	cin >> next_move.oldRank;
	cout << "Enter File (a-f) of Destination Space: ";
	cin >> next_move.newFile;
	cout << "Enter Rank (1-8) of Destination Space: ";
	cin >> next_move.newRank;

	//Identification of the piece index referenced by the user
	while(i < target_game->current_state.currentPlayer->pieces.size() &&
	      (target_game->current_state.currentPlayer->pieces[i]->rank != next_move.oldRank ||
	    		  target_game->current_state.currentPlayer->pieces[i]->file != next_move.oldFile))
	{
	    i++;
	}

	//Determination of the piece type to complete the instance of the action class
	if(i < target_game->current_state.currentPlayer->pieces.size())
		next_move.type = target_game->current_state.currentPlayer->pieces[i]->type;

	//Compares the chosen action to the list of possible actions
	for(unsigned int j = 0; j < possible_moves.size(); j++)
	{
		//Checks if the basic location and type information matches a possible move
		if(possible_moves[j] == next_move)
		{
			//Fills out the rest of information that the system generated move contains
			//These other variables are not expected to be known by the player and
			//would take up unnecessary time to enter.
			next_move = possible_moves[j];
			//If the move requires a promotion, the user is prompted for a choice instead
			//of automatically selecting the default promotion.
			if(next_move.promotion != "")
			{
				do
				{
					cout << "Enter Pawn Promotion Piece (Queen, Knight, Rook, Bishop): ";
					cin >> next_move.promotion;
				}while(next_move.promotion != "Queen" &&
						next_move.promotion != "Knight" &&
						next_move.promotion != "Rook" &&
						next_move.promotion != "Bishop");
			}
			//Returns a valid move that has been successfully expanded by an
			//existing system generated move.
			return next_move;
		}
	}
	//Returns a move that could not be validated and will most likely fail
	//the game class' validation process and require another execution of
	//this function.
	return next_move;
}
