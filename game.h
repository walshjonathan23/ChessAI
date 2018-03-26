/*
 * game.h
 * This file contains header information for classes that define the behavior
 * of the game environment.
 */

#ifndef GAME_H_
#define GAME_H_

#include <iostream>
#include <time.h>
#include <queue>

using namespace std;

class myPlayer;

//The action class maintain information about a chess move including information
//about new and old positioning and additional flags and values used by the AI
//in its algorithms.
class action
{
    public:
        string type = "";
        string oldFile;
        int oldRank = 0;
        string newFile;
        int newRank = 0;
        //Optional element defining what piece replaces a promoting pawn
        string promotion;
        //Values defining the actions status in an AI history table
        int historyValue;
        int htAge;
        //Flags noting special cases of piece behavior
        bool isCastle;
        bool isEnPassant;

        action() {isCastle = false; isEnPassant = false; historyValue = 0; htAge = 0;}
        bool operator==(const action& rhs);
        bool operator<(const action& rhs)const {return historyValue < rhs.historyValue;}
        friend ostream& operator<<(ostream& os, const action& a);
};

//The myPiece class maintain information about a chess piece including
//its position, owner, and type
class myPiece
{
    public:
        string file;
        int rank;
        bool hasMoved;
        myPlayer* owner;
        string type;
};

//The myPlayer class tracks what the player's pieces, the direction pawns can go,
//and a reference to the opponent player class.
class myPlayer
{
    public:
        vector<myPiece*> pieces;
        int rankDirection;
        myPlayer* opponent;
};

//The state class maintains information about a single board arrangement and
//provides functions to produce the resulting states based on provided actions
//as well as functions to evaluate the overall value or utility of a given state
//to aid the AI in distinguishing what states are the best choices.
class state
{
    public:
		//constants for state value calculation
	    const int CHECKMATEVALUE = 100000;
	    const int DRAWVALUE = 0;

	    const int END_GAME_CUTOFF = 15000;

	    const int SAFETY_THRESH = 125;
	    const int QUEEN_BONUS_WEAK_KING = 200;

		const int QUEENVALUE = 9000;
	    const int ROOKVALUE = 5000;
	    const int BISHOPVALUE = 3200;
	    const int KNIGHTVALUE = 3200;
	    const int PAWNVALUE = 1000;

	    //The player who is in control of the next move
        myPlayer* currentPlayer;
        //The player who is the AI who's perspective is influencing a game tree search
        //This player remains constant at all depths while the currentPlayer switches
        //every other turn.
        myPlayer* maxPlayer;
        //References to the players in the game which consequently maintain the list
        //of pieces in play.
        vector<myPlayer*> players;
        //A list of the most recent actions taken by both players to determine if
        //a state ends in a draw because the players are making a cycle of the same moves
        vector<action> previousActions;
        //A value storing the difference between the sum of the piece values
        //of the maxPlayer and the opponent's pieces
        int materialDifference = 0;
        //A value storing the utility or overall value of the state
        int utilityValue;
        //A value storing how much the utility value changed since the previous state
        int quiescentChange = 0;
        //Flag to signal states where the game has ended and therefore has no
        //possible children states
        bool isTerminalState;
        //Flags signaling whether the game players have performed a castling move
        static bool whiteHasCastled;
        static bool blackHasCastled;

        //Functions related to generating possible children states in the game tree
        vector<action> actions(bool existenceCheck = false);
        state result(action& a, bool calcUtil, int strategy, bool calcTerminal, bool isOpening);

        state();
        ~state();

        //Functions related to evaluating the utility or overall value of a state
        int calculateUtility(bool isOpening, int strategy);
        void updateMaterialDifference();
        int openingUtility();
        int endingUtility();
        int rookUtility(vector<myPiece*>& myRooks, vector<myPiece*>& oppRooks);
        bool connected(vector<myPiece*>& rooks);
        int rookUtilitySub(myPlayer* player, vector<myPiece*>& rooks, int enemyRank);
        int kingSafetyUtility(myPiece* myKing, myPiece* oppKing);
        int kingSafetyBlack(myPlayer* black, myPiece* king);
        int kingSafetyWhite(myPlayer* white, myPiece* king);

        //Functions related to generating possible actions that can be taken
        //so the actions function can consider different piece behavior
        vector<action> generateKingMoves(myPiece* king);
        vector<action> generateQueenMoves(myPiece* queen);
        vector<action> generateKnightMoves(myPiece* knight);
        vector<action> generateRookMoves(myPiece* rook);
        vector<action> generateBishopMoves(myPiece* bishop);
        vector<action> generatePawnMoves(myPiece* pawn);

        //Various helper functions to determine information about the board state
        int occupied(const int rank, const int file, const myPlayer* player);
        bool inDanger(const int rank, const int file, const myPlayer* player);
        bool validForCheck(bool isOwnPiece, action a);
        bool isCheck(const myPlayer* player);
        bool isDraw();
        int isOpenFile(string targetFile, myPlayer* player);
        int pawnProgression(string targetFile, myPlayer* player);
        int pinnedSquares(const myPlayer* player);
        void removeTakenPiece(const int rank, const string file);
        string getType(const int rank, const string file);

        //Functions to convert the string for the file portion of a chess coordinate
        //to a more clear integer format
        int fileToInt(string file);
        string intToFile(int file);

        void operator=(const state& s);

        friend ostream& operator<<(ostream& os, const state& s);
};

//The game class maintains information about a full game of chess and
//oversees the interaction of the player's with the board.
class game
{
	public:
		//Fifteen minutes are allowed for each player for the duration of the game
		//for pondering their moves
		const double GAME_TIME_ALLOWED = 900;
		//The current_state maintains all information about the board
		state current_state;
		//The move_log tracks all of the final actions the AI's and players
		//decide to take.
		vector<action> move_log;
		//The time remaining for each player to ponder their remaining moves.
		//It is considered a loss if a player runs out of time.
		double whiteTimeRemaining = GAME_TIME_ALLOWED;
		double blackTimeRemaining = GAME_TIME_ALLOWED;
		//A turn counter
		int currentTurn = 0;

		//Sets up the current_state to have pieces in the correct starting positions
		void initializeBoard();
		//Validates moves returned by the players
		bool valid_move(action move);
		//Updates the game's characteristics after a valid action is selected
		void update(action move);
		//Evaluates an exit code if the game comes to an end or flags that the game
		//continues
		int is_game_over();
		//Prints the game status to the standard output
		void renderGame();
		//Prints a message related to the exit code generated by is_game_over()
		void printVictoryResults();
};

#endif /* GAME_H_ */
