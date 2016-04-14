#include "dominion.h"
#include "dominion_helpers.h"
#include "rngs.h"
#include "cardEffects.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

//modified
static int compare(const void* a, const void* b)
{
	return (*(int*)a - *(int*)b);
}

//modified
struct gameState* newGame()
{
	return (struct gameState*)malloc(sizeof(struct gameState));
}

int* kingdomCards(int k1, int k2, int k3, int k4, int k5, int k6, int k7,
		  int k8, int k9, int k10) {
  int* k = malloc(10 * sizeof(int));
  k[0] = k1;
  k[1] = k2;
  k[2] = k3;
  k[3] = k4;
  k[4] = k5;
  k[5] = k6;
  k[6] = k7;
  k[7] = k8;
  k[8] = k9;
  k[9] = k10;
  return k;
}

//modified
int initializeGame(int numPlayers, int kingdomCards[10], int randomSeed, struct gameState* state)
{
	int i, j;
	//set up random number generator
	SelectStream(1);
	PutSeed((long)randomSeed);

	//check number of players
	if(numPlayers > MAX_PLAYERS || numPlayers < 2)
		return -1;

	//set number of players
	state->numPlayers = numPlayers;

	//check selected kingdom cards are different
	for(i = 0; i < 9; ++i) //10 - 1
		for(j = i+1; j < 10; ++j)
			if(kingdomCards[j] == kingdomCards[i])
				return -1;

	//initialize supply
	///////////////////////////////

	//set number of Curse cards. f(x) = 10x - 10, so long as x > 1 (which we have)
	//f(2) = 10, f(3) = 20, f(4) = 30...
	state->supplyCount[curse] = 10*numPlayers - 10;

	//set number of Victory cards
	if(numPlayers == 2)
	{
		state->supplyCount[estate] = 8;
		state->supplyCount[duchy] = 8;
		state->supplyCount[province] = 8;
	}
	else
	{
		state->supplyCount[estate] = 12;
		state->supplyCount[duchy] = 12;
		state->supplyCount[province] = 12;
	}

	//set number of Treasure cards
	state->supplyCount[copper] = 60 - (7 * numPlayers);
	state->supplyCount[silver] = 40;
	state->supplyCount[gold] = 30;

	//set number of Kingdom cards
	for(i = adventurer; i <= treasure_map; ++i)       	//loop all cards
	{
		for(j = 0; j < 10; ++j)           		//loop chosen cards
		{
			if(kingdomCards[j] == i)
			{
				//check if card is a 'Victory' Kingdom card
				if(kingdomCards[j] == great_hall || kingdomCards[j] == gardens)
					state->supplyCount[i] = (numPlayers == 2)? 8: 12;
				else
					state->supplyCount[i] = 10;

				break;
			}
			else    //card is not in the set choosen for the game
				state->supplyCount[i] = -1;
		}
	}

	////////////////////////
	//supply intilization complete

	//set player decks
	for(i = 0; i < numPlayers; ++i)
	{
		state->deckCount[i] = 10;
		for(j = 0; j < 3; ++j) //first 3 are estate
			state->deck[i][j] = estate;
		for(; j < 10; ++j) //last 7 are copper
			state->deck[i][j] = copper;
	}

	//shuffle player decks
	for(i = 0; i < numPlayers; ++i)
		if(shuffle(i, state) < 0)
			return -1;

	//initialize hands and discards size's to zero
	memset(state->handCount, 0, numPlayers * sizeof(int));
	memset(state->discardCount, 0, numPlayers * sizeof(int));

	//set embargo tokens to 0 for all supply piles
	memset(state->embargoTokens, 0, treasure_map * sizeof(int));

	//initialize first player's turn
	state->outpostPlayed = 0;
	state->phase = 0;
	state->numActions = 1;
	state->numBuys = 1;
	state->playedCardCount = 0;
	state->whoseTurn = 0;
	state->handCount[state->whoseTurn] = 0;

	//draw cards here, only drawing at the start of a turn
	for(i = 0; i < 5; ++i)
		drawCard(state->whoseTurn, state);

	updateCoins(state->whoseTurn, state, 0);

	return 0;
}

//formatted
int shuffle(int player, struct gameState* state)
{
	int newDeck[MAX_DECK];
	int i, card, newDeckPos = 0;

	if(state->deckCount[player] < 1)
		return -1;

	qsort((void*)(state->deck[player]), state->deckCount[player], sizeof(int), compare);
	/* SORT CARDS IN DECK TO ENSURE DETERMINISM! */

	while(state->deckCount[player] > 0)
	{
		card = floor(Random() * state->deckCount[player]);
		newDeck[newDeckPos] = state->deck[player][card];
		newDeckPos++;
		for(i = card; i < state->deckCount[player]-1; ++i)
			state->deck[player][i] = state->deck[player][i+1];

		state->deckCount[player]--;
	}

	for(i = 0; i < newDeckPos; ++i)
	{
		state->deck[player][i] = newDeck[i];
		state->deckCount[player]++;
	}

	return 0;
}

//modified
int playCard(int handPos, int choice1, int choice2, int choice3, struct gameState* state)
{
	int card;
	int coin_bonus = 0; //tracks coins gain from actions

	//check if it is the right phase or player has enough actions
	if(state->phase != 0 || state->numActions < 1)
		return -1;

	//get card played
	card = handCard(handPos, state);

	//check if selected card is an action
	if(card < adventurer || card > treasure_map) //good idea?
		return -1;

	//play card
	if(cardEffect(card, choice1, choice2, choice3, state, handPos, &coin_bonus) < 0)
		return -1;

	//reduce number of actions
	state->numActions--;

	//update coins (Treasure cards may be added with card draws)
	updateCoins(state->whoseTurn, state, coin_bonus);

	return 0;
}

int buyCard(int supplyPos, struct gameState *state) {
  int who;
  if (DEBUG){
	printf("Entering buyCard...\n");
  }

  // I don't know what to do about the phase thing.

  who = state->whoseTurn;

  if (state->numBuys < 1){
	if (DEBUG)
	  printf("You do not have any buys left\n");
	return -1;
  } else if (supplyCount(supplyPos, state) <1){
	if (DEBUG)
	  printf("There are not any of that type of card left\n");
	return -1;
  } else if (state->coins < getCost(supplyPos)){
	if (DEBUG)
	  printf("You do not have enough money to buy that. You have %d coins.\n", state->coins);
	return -1;
  } else {
	state->phase=1;
	//state->supplyCount[supplyPos]--;
	gainCard(supplyPos, state, 0, who); //card goes in discard

	state->coins = (state->coins) - (getCost(supplyPos));
	state->numBuys--;
	if (DEBUG)
	  printf("You bought card number %d for %d coins. You now have %d buys and %d coins.\n", supplyPos, getCost(supplyPos), state->numBuys, state->coins);
  }

  //state->discard[who][state->discardCount[who]] = supplyPos;
  //state->discardCount[who]++;

  return 0;
}

int numHandCards(struct gameState *state) {
  return state->handCount[ whoseTurn(state) ];
}

int handCard(int handPos, struct gameState *state) {
  int currentPlayer = whoseTurn(state);
  return state->hand[currentPlayer][handPos];
}

//modified
int supplyCount(int card, struct gameState* state)
{
	if(card < 0 || card > treasure_map)
		return -1;
	return state->supplyCount[card];
}

//modified
int fullDeckCount(int player, int card, struct gameState *state)
{
	int i, count = 0;

	for(i = 0; i < state->deckCount[player]; ++i)
		if(state->deck[player][i] == card)
			count++;

	for(i = 0; i < state->handCount[player]; ++i)
		if(state->hand[player][i] == card)
			count++;

	for(i = 0; i < state->discardCount[player]; ++i)
		if(state->discard[player][i] == card)
			count++;

	return count;
}

int whoseTurn(struct gameState *state) {
  return state->whoseTurn;
}

//modified
int endTurn(struct gameState* state)
{
	int i;
	int currentPlayer = whoseTurn(state);

	//Discard hand
	for(i = 0; i < state->handCount[currentPlayer]; i++)
	{
		state->discard[currentPlayer][state->discardCount[currentPlayer]++] = state->hand[currentPlayer][i];//Discard
		state->hand[currentPlayer][i] = -1;//Set card to -1
	}
	state->handCount[currentPlayer] = 0;//Reset hand count

	//Code for determining the player
	state->whoseTurn = (currentPlayer + 1) % state->numPlayers;

	state->outpostPlayed = 0;
	state->phase = 0;
	state->numActions = 1;
	state->coins = 0;
	state->numBuys = 1;
	state->playedCardCount = 0;
	state->handCount[state->whoseTurn] = 0;

	//Next player draws hand
	for(i = 0; i < 5; i++)
		drawCard(state->whoseTurn, state);//Draw a card

	//Update money
	updateCoins(state->whoseTurn, state , 0);

	return 0;
}

//modified
int isGameOver(struct gameState *state)
{
	int i, j;

	//if stack of Province cards is empty, the game ends
	if(state->supplyCount[province] == 0)
		return 1;

	//if three supply pile are at 0, the game ends
	for(i = 0, j = 0; i <= treasure_map; ++i) //this is dangerous here! needs var
		if(state->supplyCount[i] == 0)
			j++;

	if(j >= 3)
		return 1;

	return 0;
}

static int scoreHelper(int card, int player, struct gameState* state)
{
	switch(card)
	{
		case curse:		 return -1;
		case estate:	 return 1;
		case duchy:		 return 3;
		case province:	 return 6;
		case great_hall: return 1;
		case gardens:	 return fullDeckCount(player, 0, state) / 10;
	}
	return 0; //if none do not do anything
}

//modified
int scoreFor(int player, struct gameState *state)
{
	int i, score = 0;

	//score from hand
	for(i = 0; i < state->handCount[player]; ++i)
		score += scoreHelper(state->hand[player][i], player, state);

	//score from discard
	for(i = 0; i < state->discardCount[player]; ++i)
		score += scoreHelper(state->discard[player][i], player, state);

	//score from deck
	for(i = 0; i < state->deckCount[player]; ++i)
		score += scoreHelper(state->deck[player][i], player, state);

	return score;
}

//modified
int getWinners(int players[MAX_PLAYERS], struct gameState* state)
{
	int i, highScore = -9999, lessTurns = 0;

	//get score for each player and set unused player scores to -9999
	for(i = 0; i < state->numPlayers; ++i)
	{
		players[i] = scoreFor(i, state);
		if(players[i] > highScore)
			highScore = players[i];
	}

	for(; i < MAX_PLAYERS; ++i)
		players[i] = -9999;

	//add 1 to players who had less turns
	for(i = whoseTurn(state)+1; i < state->numPlayers; ++i)
	{
		if(players[i] == highScore)
		{
			players[i]++;
			lessTurns = 1; //set to true
		}
	}

	if(lessTurns)
		highScore++;

	//set winners in array to 1 and rest to 0
	for(i = 0; i < state->numPlayers; ++i)
		players[i] = (players[i] == highScore);

	return 0;
}

//modified
int drawCard(int player, struct gameState *state)
{
	int count, deckCounter;

	//Deck is empty
	if(state->deckCount[player] <= 0)
	{
		//if deck and discard are empty then we cannot draw
		if(state->discardCount[player] <= 0)
			return -1;

		//Step 1 Shuffle the discard pile back into a deck
		//Move discard to deck & clear discard to -1 (for now)
		memcpy(state->deck[player], state->discard[player], state->discardCount[player] * sizeof(int));
		memset(state->discard[player], -1, state->discardCount[player] * sizeof(int)); //nbytes = count * int

		state->deckCount[player] = state->discardCount[player];
		state->discardCount[player] = 0;//Reset discard

		//Shuffle the deck
		shuffle(player, state);//Shuffle the deck up and make it so that we can draw
		#if DEBUG
			printf("Deck count now: %d\n", state->deckCount[player]);
		#endif
	}

	//Draw the Card
	count = state->handCount[player];//Get current player's hand count
	#if DEBUG
		printf("Current hand count: %d\n", count);
	#endif

	deckCounter = state->deckCount[player];//Create a holder for the deck count

	//can do this all in one line but will be long and not pretty
	state->hand[player][count] = state->deck[player][deckCounter - 1];//Add card to hand
	state->deckCount[player]--;
	state->handCount[player]++;//Increment hand count

	return 0;
}

//fixed
int getCost(int cardNumber)
{
	switch(cardNumber)
	{
		case curse:
		case copper: return 0;

		case estate:
		case embargo: return 2;

		case silver:
		case village:
		case great_hall:
		case steward:
		case ambassador: return 3;

		case feast:
		case gardens:
		case remodel:
		case smithy:
		case baron:
		case cutpurse:
		case salvager:
		case sea_hag:
		case treasure_map: return 4;

		case duchy:
		case council_room:
		case mine:
		case minion:
		case tribute:
		case outpost: return 5;

		case gold:
		case adventurer: return 6;

		case province: return 8;
	}

	return -1;
}

//modified
//everything that adds/messes with coins needs to put it into bonus
//Also must discard high pos cards first. Which is a problem almost everywhere in here
int cardEffect(int card, int choice1, int choice2, int choice3, struct gameState *state, int handPos, int *bonus)
{
	int i, j;
	int currentPlayer = whoseTurn(state);
	int nextPlayer = (currentPlayer + 1) % state->numPlayers;

	//uses switch to select card and perform actions
	switch(card)
	{
		case adventurer:
			return adventureEffect(state, currentPlayer, handPos);

		case council_room:
			return councilRoomEffect(state, currentPlayer, handPos);

		case feast:
			//gain card with cost up to 5
			if(supplyCount(choice1, state) <= 0 || getCost(choice1) > 5)
				return -1;

			if(gainCard(choice1, state, 0, currentPlayer) == -1)//Gain the card (in discard)
				return -1;

			discardCard(handPos, currentPlayer, state, 0);

			return 0;

		case mine: //trash treasure card and get another that costs up to 3 more
			//valid pos in hand
			if(choice1 < 0 || choice1 >= state->handCount[currentPlayer])
				return -1;

			//needs to be a treasure card. If mine is only card then this returns
			if(state->hand[currentPlayer][choice1] < copper || state->hand[currentPlayer][choice1] > gold)
				return -1;

			//need to gain a treasure card (nothing else). Handles out of bounds input. Handles empty supply
			if(supplyCount(choice2, state) <= 0 || choice2 < copper || choice2 > gold)
				return -1;

			//can trade a gold/silver for copper
			if((getCost(choice2) - 3) > getCost(state->hand[currentPlayer][choice1]))
				return -1;

			//puts card in hand
			gainCard(choice2, state, 2, currentPlayer); //don't need to check thanks to choice2 if

			//discard card from hand and trash treasure
			discardCard(handPos, currentPlayer, state, 0);
			discardCard(choice1, currentPlayer, state, 0);

			return 0;

		case remodel: //trash card from hand and get one that costs 3 more
			//valid pos in hand and remodel is not the only card
			if(choice1 < 0 || choice1 >= state->handCount[currentPlayer] || state->handCount[currentPlayer] <= 1)
				return -1;

			//check if that choice2 is available or valid
			if(supplyCount(choice2, state) <= 0)
				return -1;

			//can trade for a lower card (if they want to)
			if((getCost(choice2) - 2) > getCost(state->hand[currentPlayer][choice1]))
				return -1;

			//puts card in discard
			gainCard(choice2, state, 0, currentPlayer);  //don't need to check thanks to choice2 if

			//discard card from hand and trash other
			discardCard(handPos, currentPlayer, state, 0);
			discardCard(choice1, currentPlayer, state, 0);

			return 0;

		case smithy:
			//+3 Cards
			for(i = 0; i < 3; ++i)
				drawCard(currentPlayer, state);

			//discard card from hand
			discardCard(handPos, currentPlayer, state, 0);
			return 0;

		case village:
			//+1 Card
			drawCard(currentPlayer, state);

			//+2 Actions
			state->numActions += 2;

			//discard played card from hand
			discardCard(handPos, currentPlayer, state, 0);
			return 0;

		case baron: //can be simplified with goto
			if(choice1) //Boolean true or going to discard an estate
			{
				for(i = 0; i < state->handCount[currentPlayer]; ++i)
				{
					if(state->hand[currentPlayer][i] == estate)
					{
						state->numBuys++;//Increase buys by 1!
						state->coins += 4;
						discardCard(handPos, currentPlayer, state, 0); //discard baron
						discardCard(i, currentPlayer, state, 0); //discard estate
						return 0;
					}
				}

				//could not find estate. Warn them instead of giving them an estate
				if(DEBUG)
				{
					printf("No estate cards in your hand, invalid choice\n");
					printf("Must gain an estate if there are any\n");
				}
				return -1;
			}

			if(gainCard(estate, state, 0, currentPlayer) == -1) //for now it just continues to play
			{
				if(DEBUG)
					printf("No estates left in the supply. Cannot obtain one\n");
			}

			state->numBuys++;//Increase buys by 1!
			discardCard(handPos, currentPlayer, state, 0); //discard baron
			return 0;

		case great_hall:
			//+1 Card
			drawCard(currentPlayer, state);

			//+1 Actions
			state->numActions++;

			//discard card from hand
			discardCard(handPos, currentPlayer, state, 0);
			return 0;

		case minion: //what if choice1 is not 1 or 2
			//+1 action
			state->numActions++;

			//discard card from hand
			discardCard(handPos, currentPlayer, state, 0);

			if(choice1 == 1) //+2 coins
			{
				state->coins += 2;
				return 0;
			}

			//discard hand, redraw 4, other players with 5+ cards discard hand and draw 4
			//discard hand
			while(state->handCount[currentPlayer] > 0)
				discardCard(0, currentPlayer, state, 0);

			//draw 4
			for(i = 0; i < 4; ++i)
				drawCard(currentPlayer, state);

			//other players discard hand and redraw if hand size >= 5
			for(i = 0; i < state->numPlayers; ++i)
			{
				if(i != currentPlayer && state->handCount[i] >= 5)
				{
					//discard hand
					while(state->handCount[i] > 0)
						discardCard(0, i, state, 0);

					//draw 4
					for(j = 0; j < 4; ++j)
						drawCard(i, state);
				}
			}

			return 0;

		case steward: //there is a problem here possibly with how cards need to be discarded
			if(choice1 == 1) //+2 cards
			{
				drawCard(currentPlayer, state);
				drawCard(currentPlayer, state);
			}
			else if(choice1 == 2) //+2 coins
				state->coins += 2;
			else //need to check choice2 and choice3 here
			{
				//trash 2 cards in hand. Can trash 1 (or 0?) if have less than 2
				discardCard(choice2, currentPlayer, state, 1); //removes cards from play
				discardCard(choice3, currentPlayer, state, 1);
			}

			//discard card from hand
			discardCard(handPos, currentPlayer, state, 0);

			return 0;

		case tribute: //some code duplication here but works
			j = state->handCount[nextPlayer]; //pos of interest
			for(i = 0; i < 2; ++i) // try to draw/reveal 2 cards
				drawCard(nextPlayer, state);

			//handle drawing 2 identical cards
			if(state->handCount[nextPlayer] == (j+2) && state->hand[nextPlayer][j] == state->hand[nextPlayer][j+1])
			{
				//need to discard [j+1] without using discardCard (it puts it into the played area).
				state->discard[nextPlayer][state->discardCount[nextPlayer]] = state->hand[nextPlayer][j+1];
				state->discardCount[nextPlayer]++;
				//then need to decrement handCount[nextPlayer]
				discardCard(j+1, nextPlayer, state, 1); //removes from hand but does not move to played area
			}

			//decrementing so I can process and remove at the same time
			for(i = state->handCount[nextPlayer]-1; i >= j; --i)
			{
				if(state->hand[nextPlayer][i] >= copper && state->hand[nextPlayer][i] <= gold) //Treasure cards
					state->coins += 2;
				else if(state->hand[nextPlayer][i] == estate || state->hand[nextPlayer][i] == duchy || state->hand[nextPlayer][i] == province || state->hand[nextPlayer][i] == gardens || state->hand[nextPlayer][i] == great_hall)
				{
					//Victory Card Found
					drawCard(currentPlayer, state);
					drawCard(currentPlayer, state);
				}
				else //Action Card
					state->numActions += 2;

				//discard the processed card
				state->discard[nextPlayer][state->discardCount[nextPlayer]] = state->hand[nextPlayer][i];
				state->discardCount[nextPlayer]++;
				//then need to decrement handCount[nextPlayer]
				discardCard(i, nextPlayer, state, 1); //removes from hand but does not move to played area
			}

			//discard played card from hand
			discardCard(handPos, currentPlayer, state, 0);

			return 0;

		case ambassador:
			j = 0;		//used to check if player has enough cards to discard

			if(choice2 > 2 || choice2 < 0 || choice1 == handPos)
				return -1;

			for(i = 0; i < state->handCount[currentPlayer]; i++)
				if(i != handPos && i == state->hand[currentPlayer][choice1] && i != choice1)
					j++;

			if(j < choice2)
				return -1;

			if(DEBUG)
				printf("Player %d reveals card number: %d\n", currentPlayer, state->hand[currentPlayer][choice1]);

			//increase supply count for choosen card by amount being discarded
			state->supplyCount[state->hand[currentPlayer][choice1]] += choice2;

			//each other player gains a copy of revealed card
			for(i = 0; i < state->numPlayers; i++)
				if(i != currentPlayer)
					gainCard(state->hand[currentPlayer][choice1], state, 0, i);

			//discard played card from hand
			discardCard(handPos, currentPlayer, state, 0);

			//trash copies of cards returned to supply
			for(j = 0; j < choice2; j++)
			{
				for(i = 0; i < state->handCount[currentPlayer]; i++)
				{
					if(state->hand[currentPlayer][i] == state->hand[currentPlayer][choice1])
					{
						discardCard(i, currentPlayer, state, 1);
						break;
					}
				}
			}

			return 0;

		case cutpurse:
			return cutpurseEffect(state, currentPlayer, handPos);

		case embargo:
			//+2 Coins
			state->coins += 2;

			//see if selected pile is in play. This is a bug
			if(state->supplyCount[choice1] == -1)
				return -1;

			//add embargo token to selected supply pile
			state->embargoTokens[choice1]++;

			//trash card
			discardCard(handPos, currentPlayer, state, 1);
			return 0;

		case outpost:
			//set outpost flag
			state->outpostPlayed++;

			//discard card
			discardCard(handPos, currentPlayer, state, 0);
			return 0;

		case salvager: //if choice is less than player then discard problem
			//+1 buy
			state->numBuys++;

			if(choice1)
			{
				//gain coins equal to trashed card
				state->coins += getCost(handCard(choice1, state));
				//trash card
				discardCard(choice1, currentPlayer, state, 1);
			}

			//discard card
			discardCard(handPos, currentPlayer, state, 0);
			return 0;

		case sea_hag:
			return seaHagEffect(state, currentPlayer, handPos);

		case treasure_map:
			return treasureMapEffect(state, currentPlayer, handPos);
	}

	return -1;
}

//modified
int discardCard(int handPos, int currentPlayer, struct gameState *state, int trashFlag)
{
	//if card is not trashed, add to Played pile
	if(trashFlag < 1)
	{
		state->playedCards[state->playedCardCount] = state->hand[currentPlayer][handPos];
		state->playedCardCount++;
	}

	//remove card from player's hand (decrement hand)
	state->handCount[currentPlayer]--;
	//replace discarded card with last card in hand if not last
	if(handPos != state->handCount[currentPlayer])
		state->hand[currentPlayer][handPos] = state->hand[currentPlayer][state->handCount[currentPlayer]]; //safe cause dec

	//set last card to -1
	state->hand[currentPlayer][state->handCount[currentPlayer]] = -1; //safe cause dec

	return 0;
}

//modified
int gainCard(int supplyPos, struct gameState *state, int toFlag, int player)
{
	//Note: supplyPos is enum of choosen card

	//check if supply pile is empty (0) or card is not used in game (-1)
	if(supplyCount(supplyPos, state) < 1)
		return -1;

	//added card for [whoseTurn] current player:
	// toFlag = 0 : add to discard
	// toFlag = 1 : add to deck
	// toFlag = 2 : add to hand
	switch(toFlag)
	{
		case 0:
			state->discard[player][state->discardCount[player]] = supplyPos;
			state->discardCount[player]++;
			break;
		case 1:
			state->deck[player][state->deckCount[player]] = supplyPos;
			state->deckCount[player]++;
			break;
		case 2:
			state->hand[player][state->handCount[player]] = supplyPos;
			state->handCount[player]++;
			break;
	}

	//decrease number in supply pile
	state->supplyCount[supplyPos]--;

	return 0;
}

//modified
int updateCoins(int player, struct gameState* state, int bonus)
{
	int i;

	//reset coin count
	state->coins = 0;

	for(i = 0; i < state->handCount[player]; ++i)
	{
		switch(state->hand[player][i])
		{
			case copper: state->coins++; break;
			case silver: state->coins += 2; break;
			case gold:   state->coins += 3; break;
		}
	}

	//add bonus
	state->coins += bonus;

	return 0;
}

//end of dominion.c
