/*
 * human.h
 * This file contains the header information for the humanPlayer class
 */

#ifndef HUMAN_H_
#define HUMAN_H_

#include "game.h"
#include "ai.h"

//The humanPlayer class is responsible for prompting a user for chess moves
//and interacting with the game object to implement their selections.
class humanPlayer
{
	public:
		//A reference to the game being interacted with.
		game* target_game;
		//Function to prompt the user for all choices related to their turn
		action runTurn();
};

#endif /* HUMAN_H_ */
