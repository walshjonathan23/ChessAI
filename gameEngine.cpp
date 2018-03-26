/*
 * gameEngine.cpp
 * This file contains the main function to initiate a game of chess with
 * provided parameters.
 */

/*
 * The Chess AI program defines an environment where a game of chess
 * can be played. The environment allows piece control by both human
 * players and artificial intelligence players.
 *
 * The artificial intelligence algorithm is a quiescent time-heuristic
 * iterative deepening depth limited alpha-beta pruned minimax method.
 * The algorithm detects major turning points in the game and deepens
 * its search in those areas. It also makes decisions for the depth
 * of its searches based on how much time is remaining. A history table
 * also maintains information about previous optimal moves to improve
 * the efficiency of future game tree searches.
 *
 * The program offers a choice between two different methods for the
 * AI to select moves. One method involves a complex set of
 * metrics that encourages moves that follow established chess
 * strategies. This method makes well-educated choices in the game
 * at the cost of long calculation times reducing game tree depth.
 * The second method focuses solely on the value of the pieces
 * each player has in play. This method focuses on only winning the
 * game and taking opponent pieces but can search deeper in a game
 * tree in a given amount of time.
 */

#include <iostream>
#include <time.h>
#include "game.h"
#include "ai.h"

using namespace std;

//The main function selects if humans or AI's control the pieces then runs a single game of chess
int main()
{
	game test_game;

	test_game.initializeBoard();

	string whitePlayerType = "none";
	string blackPlayerType = "none";

	vector<humanPlayer*> humans;
	vector<ai*> ais;

	time_t startOfTurn;
	double time_passed;

	action next_move;

	//Selection of AI or human control of white pieces
	do
	{
		int choice;
		cout << "Select Number of Player Type for White Pieces" << endl;
		cout << "1) Human Player\n2) AI Player (Advanced Heuristic)\n3) AI Player (Simple Heuristic)" << endl;
		cin >> choice;
		switch (choice)
		{
			case 1:
				whitePlayerType = "human";
				break;
			case 2:
				whitePlayerType = "ai 0";
				break;
			case 3:
				whitePlayerType = "ai 1";
				break;
			default:
				cout << "Invalid selection. Enter the number of one of the provided choices." << endl;
		}
	}while(whitePlayerType == "none");

	//Selection of AI or human control of black pieces
	do
	{
		int choice;
		cout << "Select Number of Player Type for Black Pieces" << endl;
		cout << "1) Human Player\n2) AI Player (Advanced Heuristic)\n3) AI Player (Simple Heuristic)" << endl;
		cin >> choice;
		switch (choice)
		{
			case 1:
				blackPlayerType = "human";
				break;
			case 2:
				blackPlayerType = "ai 0";
				break;
			case 3:
				blackPlayerType = "ai 1";
				break;
			default:
				cout << "Invalid selection. Enter the number of one of the provided choices." << endl;
		}
	}while(blackPlayerType == "none");

	if(whitePlayerType == "human")
	{
		//Initialize a human player
		humanPlayer* tmp;
		tmp = new humanPlayer;
		tmp->target_game = &test_game;
		humans.push_back(tmp);
	}
	else if(whitePlayerType == "ai 0")
	{
		//Initialize an AI player with strategy 0 and htFile 0
		ai* tmp;
		tmp = new ai(0,'0');
		tmp->target_game = &test_game;
		ais.push_back(tmp);
	}
	else if(whitePlayerType == "ai 1")
	{
		//Initialize an AI player with strategy 1 and htFile 0
		ai* tmp;
		tmp = new ai(1,'0');
		tmp->target_game = &test_game;
		ais.push_back(tmp);
	}

	if(blackPlayerType == "human")
	{
		//Initialize human player
		humanPlayer* tmp;
		tmp = new humanPlayer;
		tmp->target_game = &test_game;
		humans.push_back(tmp);
	}
	else if(blackPlayerType == "ai 0")
	{
		//Initialize AI player with strategy 0 and htFile 1
		ai* tmp;
		tmp = new ai(0,'1');
		tmp->target_game = &test_game;
		ais.push_back(tmp);
	}
	else if(blackPlayerType == "ai 1")
	{
		//Initialize AI player with strategy 1 and htFile 1
		ai* tmp;
		tmp = new ai(1,'1');
		tmp->target_game = &test_game;
		ais.push_back(tmp);
	}

	//Make sure AI's have initialized history tables
	for(unsigned int i = 0; i < ais.size(); i++)
	{
		ais[i]->initializeHistoryTable();
	}

	//Main game loop continuing until a terminal state is encountered
	while(!test_game.is_game_over())
	{
		//White's turn
		if(whitePlayerType == "human")
		{
			do
			{
				//Display game
				test_game.renderGame();
				//Begin move timer
				time(&startOfTurn);
				//Request move choice from human white player
				next_move = humans.front()->runTurn();
				//Calculate used time for turn
				time_passed = difftime(time(NULL),startOfTurn);
				test_game.whiteTimeRemaining -= time_passed;
			}while(!test_game.valid_move(next_move));
			//Store move choice and update board state
			test_game.move_log.push_back(next_move);
			test_game.update(next_move);
		}
		else
		{
			do
			{
				//Display game
				test_game.renderGame();
				//Begin move timer
				time(&startOfTurn);
				//Request move choice from AI white player
				next_move = ais.front()->runTurn();
				//Calculate used time for turn
				time_passed = difftime(time(NULL),startOfTurn);
				test_game.whiteTimeRemaining -= time_passed;
			}while(!test_game.valid_move(next_move));
			//Store move choice and update board state
			test_game.move_log.push_back(next_move);
			test_game.update(next_move);
		}
		//Check to see if white's move created a terminal state
		if(test_game.is_game_over())
			break;
		//Black's turn
		if(blackPlayerType == "human")
		{
			do
			{
				//Display game
				test_game.renderGame();
				//Begin move timer
				time(&startOfTurn);
				//Request move choice from human black player
				next_move = humans.back()->runTurn();
				//Calculate used time for turn
				time_passed = difftime(time(NULL),startOfTurn);
				test_game.blackTimeRemaining -= time_passed;
			}while(!test_game.valid_move(next_move));
			//Store move choice and update board state
			test_game.move_log.push_back(next_move);
			test_game.update(next_move);
		}
		else
		{
			do
			{
				//Display game
				test_game.renderGame();
				//Begin move timer
				time(&startOfTurn);
				//Request move choice from AI black player
				next_move = ais.back()->runTurn();
				//Calculate used time for turn
				time_passed = difftime(time(NULL),startOfTurn);
				test_game.blackTimeRemaining -= time_passed;
			}while(!test_game.valid_move(next_move));
			//Store move choice and update board state
			test_game.move_log.push_back(next_move);
			test_game.update(next_move);
		}
	}
	//Upon end of game, display final board and print what caused the end of game
	test_game.renderGame();
	test_game.printVictoryResults();

	return 0;
}


