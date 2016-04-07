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

int initializeGame(int numPlayers, int kingdomCards[10], int randomSeed,
		   struct gameState *state) {

  int i;
  int j;
  int it;
  //set up random number generator
  SelectStream(1);
  PutSeed((long)randomSeed);

  //check number of players
  if (numPlayers > MAX_PLAYERS || numPlayers < 2)
	{
	  return -1;
	}

  //set number of players
  state->numPlayers = numPlayers;

  //check selected kingdom cards are different
  for (i = 0; i < 10; i++)
	{
	  for (j = 0; j < 10; j++)
		{
	  if (j != i && kingdomCards[j] == kingdomCards[i])
		{
		  return -1;
		}
		}
	}


  //initialize supply
  ///////////////////////////////

  //set number of Curse cards. 10x - 10 for x >= 2
  if (numPlayers == 2)
	{
	  state->supplyCount[curse] = 10;
	}
  else if (numPlayers == 3)
	{
	  state->supplyCount[curse] = 20;
	}
  else
	{
	  state->supplyCount[curse] = 30;
	}

  //set number of Victory cards
  if (numPlayers == 2)
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
  for (i = adventurer; i <= treasure_map; i++)       	//loop all cards
	{
	  for (j = 0; j < 10; j++)           		//loop chosen cards
	{
	  if (kingdomCards[j] == i)
		{
		  //check if card is a 'Victory' Kingdom card
		  if (kingdomCards[j] == great_hall || kingdomCards[j] == gardens)
		{
		  if (numPlayers == 2){
			state->supplyCount[i] = 8;
		  }
		  else{ state->supplyCount[i] = 12; }
		}
		  else
		{
		  state->supplyCount[i] = 10;
		}
		  break;
		}
	  else    //card is not in the set choosen for the game
		{
		  state->supplyCount[i] = -1;
		}
	}

	}

  ////////////////////////
  //supply intilization complete

  //set player decks
  for (i = 0; i < numPlayers; i++)
	{
	  state->deckCount[i] = 0;
	  for (j = 0; j < 3; j++)
	{
	  state->deck[i][j] = estate;
	  state->deckCount[i]++;
	}
	  for (j = 3; j < 10; j++)
	{
	  state->deck[i][j] = copper;
	  state->deckCount[i]++;
	}
	}

  //shuffle player decks
  for (i = 0; i < numPlayers; i++)
	{
	  if ( shuffle(i, state) < 0 )
	{
	  return -1;
	}
	}

  //draw player hands
  for (i = 0; i < numPlayers; i++)
	{
	  //initialize hand size to zero
	  state->handCount[i] = 0;
	  state->discardCount[i] = 0;
	  //draw 5 cards
	  // for (j = 0; j < 5; j++)
	  //	{
	  //	  drawCard(i, state);
	  //	}
	}

  //set embargo tokens to 0 for all supply piles
  for (i = 0; i <= treasure_map; i++)
	{
	  state->embargoTokens[i] = 0;
	}

  //initialize first player's turn
  state->outpostPlayed = 0;
  state->phase = 0;
  state->numActions = 1;
  state->numBuys = 1;
  state->playedCardCount = 0;
  state->whoseTurn = 0;
  state->handCount[state->whoseTurn] = 0;
  //int it; move to top

  //Moved draw cards to here, only drawing at the start of a turn
  for (it = 0; it < 5; it++){
	drawCard(state->whoseTurn, state);
  }

  updateCoins(state->whoseTurn, state, 0);

  return 0;
}

int shuffle(int player, struct gameState *state) {


  int newDeck[MAX_DECK];
  int newDeckPos = 0;
  int card;
  int i;

  if (state->deckCount[player] < 1)
	return -1;
  qsort ((void*)(state->deck[player]), state->deckCount[player], sizeof(int), compare);
  /* SORT CARDS IN DECK TO ENSURE DETERMINISM! */

  while (state->deckCount[player] > 0) {
	card = floor(Random() * state->deckCount[player]);
	newDeck[newDeckPos] = state->deck[player][card];
	newDeckPos++;
	for (i = card; i < state->deckCount[player]-1; i++) {
	  state->deck[player][i] = state->deck[player][i+1];
	}
	state->deckCount[player]--;
  }
  for (i = 0; i < newDeckPos; i++) {
	state->deck[player][i] = newDeck[i];
	state->deckCount[player]++;
  }

  return 0;
}

//modified
int playCard(int handPos, int choice1, int choice2, int choice3, struct gameState *state)
{
	int card;
	int coin_bonus = 0; //tracks coins gain from actions

	//check if it is the right phase
	if(state->phase != 0)
		return -1;

	//check if player has enough actions
	if(state->numActions < 1)
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
	gainCard(supplyPos, state, 0, who); //card goes in discard, this might be wrong.. (2 means goes into hand, 0 goes into discard)

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

int supplyCount(int card, struct gameState *state) {
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

int endTurn(struct gameState *state) {
  int k;
  int i;
  int currentPlayer = whoseTurn(state);

  //Discard hand
  for (i = 0; i < state->handCount[currentPlayer]; i++){
	state->discard[currentPlayer][state->discardCount[currentPlayer]++] = state->hand[currentPlayer][i];//Discard
	state->hand[currentPlayer][i] = -1;//Set card to -1
  }
  state->handCount[currentPlayer] = 0;//Reset hand count

  //Code for determining the player
  if (currentPlayer < (state->numPlayers - 1)){
	state->whoseTurn = currentPlayer + 1;//Still safe to increment
  }
  else{
	state->whoseTurn = 0;//Max player has been reached, loop back around to player 1
  }

  state->outpostPlayed = 0;
  state->phase = 0;
  state->numActions = 1;
  state->coins = 0;
  state->numBuys = 1;
  state->playedCardCount = 0;
  state->handCount[state->whoseTurn] = 0;

  //int k; move to top
  //Next player draws hand
  for (k = 0; k < 5; k++){
	drawCard(state->whoseTurn, state);//Draw a card
  }

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
	for(i = 0, j = 0; i < (treasure_map+1); ++i) //this is dangerous here! needs var
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

//modified. Really need to test!!!
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

int getWinners(int players[MAX_PLAYERS], struct gameState *state) {
  int i;
  int j;
  int highScore;
  int currentPlayer;

  //get score for each player
  for (i = 0; i < MAX_PLAYERS; i++)
	{
	  //set unused player scores to -9999
	  if (i >= state->numPlayers)
	{
	  players[i] = -9999;
	}
	  else
	{
	  players[i] = scoreFor (i, state);
	}
	}

  //find highest score
  j = 0;
  for (i = 0; i < MAX_PLAYERS; i++)
	{
	  if (players[i] > players[j])
	{
	  j = i;
	}
	}
  highScore = players[j];

  //add 1 to players who had less turns
  currentPlayer = whoseTurn(state);
  for (i = 0; i < MAX_PLAYERS; i++)
	{
	  if ( players[i] == highScore && i > currentPlayer )
	{
	  players[i]++;
	}
	}

  //find new highest score
  j = 0;
  for (i = 0; i < MAX_PLAYERS; i++)
	{
	  if ( players[i] > players[j] )
	{
	  j = i;
	}
	}
  highScore = players[j];

  //set winners in array to 1 and rest to 0
  for (i = 0; i < MAX_PLAYERS; i++)
	{
	  if ( players[i] == highScore )
	{
	  players[i] = 1;
	}
	  else
	{
	  players[i] = 0;
	}
	}

  return 0;
}

//modified
int drawCard(int player, struct gameState *state)
{
	int count, deckCounter;

	//Deck is empty
	if(state->deckCount[player] <= 0)
	{
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

		//Step 2 Draw Card
		count = state->handCount[player];//Get current player's hand count
		#if DEBUG
			printf("Current hand count: %d\n", count);
		#endif

		deckCounter = state->deckCount[player];//Create a holder for the deck count

		if(deckCounter == 0) //this shouldn't happen but okay
			return -1;
	}
	else //some code duplication here
	{
		count = state->handCount[player];//Get current hand count for player
		#if DEBUG
			printf("Current hand count: %d\n", count);
		#endif

		deckCounter = state->deckCount[player];//Create holder for the deck count
	}

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
int cardEffect(int card, int choice1, int choice2, int choice3, struct gameState *state, int handPos, int *bonus)
{
  int i, j, x;
  int currentPlayer = whoseTurn(state);
  int nextPlayer = currentPlayer + 1;

  int tributeRevealedCards[2] = {-1, -1};
  int temphand[MAX_HAND];

  if (nextPlayer > (state->numPlayers - 1)){
	nextPlayer = 0;
  }

  //uses switch to select card and perform actions
  switch( card )
	{
	case adventurer:
	  return adventureEffect(state, currentPlayer, handPos);

	case council_room:
	  return councilRoomEffect(state, currentPlayer, handPos);

	case feast:
	  //gain card with cost up to 5
	  //Backup hand
	  for (i = 0; i <= state->handCount[currentPlayer]; i++){
	temphand[i] = state->hand[currentPlayer][i];//Backup card
	state->hand[currentPlayer][i] = -1;//Set to nothing
	  }
	  //Backup hand

	  //Update Coins for Buy
	  updateCoins(currentPlayer, state, 5);
	  x = 1;//Condition to loop on
	  while( x == 1) {//Buy one card
	if (supplyCount(choice1, state) <= 0){
	  if (DEBUG)
		printf("None of that card left, sorry!\n");

	  if (DEBUG){
		printf("Cards Left: %d\n", supplyCount(choice1, state));
	  }
	}
	else if (state->coins < getCost(choice1)){
	  printf("That card is too expensive!\n");

	  if (DEBUG){
		printf("Coins: %d < %d\n", state->coins, getCost(choice1));
	  }
	}
	else{

	  if (DEBUG){
		printf("Deck Count: %d\n", state->handCount[currentPlayer] + state->deckCount[currentPlayer] + state->discardCount[currentPlayer]);
	  }

	  gainCard(choice1, state, 0, currentPlayer);//Gain the card
	  x = 0;//No more buying cards

	  if (DEBUG){
		printf("Deck Count: %d\n", state->handCount[currentPlayer] + state->deckCount[currentPlayer] + state->discardCount[currentPlayer]);
	  }

	}
	  }

	  //Reset Hand
	  for (i = 0; i <= state->handCount[currentPlayer]; i++){
	state->hand[currentPlayer][i] = temphand[i];
	temphand[i] = -1;
	  }
	  //Reset Hand

	  return 0;

	case gardens:
	  return -1;

	case mine:
	  j = state->hand[currentPlayer][choice1];  //store card we will trash

	  if (state->hand[currentPlayer][choice1] < copper || state->hand[currentPlayer][choice1] > gold)
	{
	  return -1;
	}

	  if (choice2 > treasure_map || choice2 < curse)
	{
	  return -1;
	}

	  if ( (getCost(state->hand[currentPlayer][choice1]) + 3) > getCost(choice2) )
	{
	  return -1;
	}

	  gainCard(choice2, state, 2, currentPlayer);

	  //discard card from hand
	  discardCard(handPos, currentPlayer, state, 0);

	  //discard trashed card
	  for (i = 0; i < state->handCount[currentPlayer]; i++)
	{
	  if (state->hand[currentPlayer][i] == j)
		{
		  discardCard(i, currentPlayer, state, 0);
		  break;
		}
	}

	  return 0;

	case remodel:
	  j = state->hand[currentPlayer][choice1];  //store card we will trash

	  if ( (getCost(state->hand[currentPlayer][choice1]) + 2) > getCost(choice2) )
	{
	  return -1;
	}

	  gainCard(choice2, state, 0, currentPlayer);

	  //discard card from hand
	  discardCard(handPos, currentPlayer, state, 0);

	  //discard trashed card
	  for (i = 0; i < state->handCount[currentPlayer]; i++)
	{
	  if (state->hand[currentPlayer][i] == j)
		{
		  discardCard(i, currentPlayer, state, 0);
		  break;
		}
	}


	  return 0;

	case smithy:
	  //+3 Cards
	  for (i = 0; i < 3; i++)
	{
	  drawCard(currentPlayer, state);
	}

	  //discard card from hand
	  discardCard(handPos, currentPlayer, state, 0);
	  return 0;

	case village:
	  //+1 Card
	  drawCard(currentPlayer, state);

	  //+2 Actions
	  state->numActions = state->numActions + 2;

	  //discard played card from hand
	  discardCard(handPos, currentPlayer, state, 0);
	  return 0;

	case baron:
	  state->numBuys++;//Increase buys by 1!
	  if (choice1 > 0){//Boolean true or going to discard an estate
	int p = 0;//Iterator for hand!
	int card_not_discarded = 1;//Flag for discard set!
	while(card_not_discarded){
	  if (state->hand[currentPlayer][p] == estate){//Found an estate card!
		state->coins += 4;//Add 4 coins to the amount of coins
		state->discard[currentPlayer][state->discardCount[currentPlayer]] = state->hand[currentPlayer][p];
		state->discardCount[currentPlayer]++;
		for (;p < state->handCount[currentPlayer]; p++){
		  state->hand[currentPlayer][p] = state->hand[currentPlayer][p+1];
		}
		state->hand[currentPlayer][state->handCount[currentPlayer]] = -1;
		state->handCount[currentPlayer]--;
		card_not_discarded = 0;//Exit the loop
	  }
	  else if (p > state->handCount[currentPlayer]){
		if(DEBUG) {
		  printf("No estate cards in your hand, invalid choice\n");
		  printf("Must gain an estate if there are any\n");
		}
		if (supplyCount(estate, state) > 0){
		  gainCard(estate, state, 0, currentPlayer);
		  state->supplyCount[estate]--;//Decrement estates
		  if (supplyCount(estate, state) == 0){
		isGameOver(state);
		  }
		}
		card_not_discarded = 0;//Exit the loop
	  }

	  else{
		p++;//Next card
	  }
	}
	  }

	  else{
	if (supplyCount(estate, state) > 0){
	  gainCard(estate, state, 0, currentPlayer);//Gain an estate
	  state->supplyCount[estate]--;//Decrement Estates
	  if (supplyCount(estate, state) == 0){
		isGameOver(state);
	  }
	}
	  }


	  return 0;

	case great_hall:
	  //+1 Card
	  drawCard(currentPlayer, state);

	  //+1 Actions
	  state->numActions++;

	  //discard card from hand
	  discardCard(handPos, currentPlayer, state, 0);
	  return 0;

	case minion:
	  //+1 action
	  state->numActions++;

	  //discard card from hand
	  discardCard(handPos, currentPlayer, state, 0);

	  if (choice1)		//+2 coins
	{
	  state->coins = state->coins + 2;
	}

	  else if (choice2)		//discard hand, redraw 4, other players with 5+ cards discard hand and draw 4
	{
	  //discard hand
	  while(numHandCards(state) > 0)
		{
		  discardCard(handPos, currentPlayer, state, 0);
		}

	  //draw 4
	  for (i = 0; i < 4; i++)
		{
		  drawCard(currentPlayer, state);
		}

	  //other players discard hand and redraw if hand size > 4
	  for (i = 0; i < state->numPlayers; i++)
		{
		  if (i != currentPlayer)
		{
		  if ( state->handCount[i] > 4 )
			{
			  //discard hand
			  while( state->handCount[i] > 0 )
			{
			  discardCard(handPos, i, state, 0);
			}

			  //draw 4
			  for (j = 0; j < 4; j++)
			{
			  drawCard(i, state);
			}
			}
		}
		}

	}
	  return 0;

	case steward:
	  if (choice1 == 1)
	{
	  //+2 cards
	  drawCard(currentPlayer, state);
	  drawCard(currentPlayer, state);
	}
	  else if (choice1 == 2)
	{
	  //+2 coins
	  state->coins = state->coins + 2;
	}
	  else
	{
	  //trash 2 cards in hand
	  discardCard(choice2, currentPlayer, state, 1);
	  discardCard(choice3, currentPlayer, state, 1);
	}

	  //discard card from hand
	  discardCard(handPos, currentPlayer, state, 0);
	  return 0;

	case tribute:
	  if ((state->discardCount[nextPlayer] + state->deckCount[nextPlayer]) <= 1){
	if (state->deckCount[nextPlayer] > 0){
	  tributeRevealedCards[0] = state->deck[nextPlayer][state->deckCount[nextPlayer]-1];
	  state->deckCount[nextPlayer]--;
	}
	else if (state->discardCount[nextPlayer] > 0){
	  tributeRevealedCards[0] = state->discard[nextPlayer][state->discardCount[nextPlayer]-1];
	  state->discardCount[nextPlayer]--;
	}
	else{
	  //No Card to Reveal
	  if (DEBUG){
		printf("No cards to reveal\n");
	  }
	}
	  }

	  else{
	if (state->deckCount[nextPlayer] == 0){
	  for (i = 0; i < state->discardCount[nextPlayer]; i++){
		state->deck[nextPlayer][i] = state->discard[nextPlayer][i];//Move to deck
		state->deckCount[nextPlayer]++;
		state->discard[nextPlayer][i] = -1;
		state->discardCount[nextPlayer]--;
	  }

	  shuffle(nextPlayer,state);//Shuffle the deck
	}
	tributeRevealedCards[0] = state->deck[nextPlayer][state->deckCount[nextPlayer]-1];
	state->deck[nextPlayer][state->deckCount[nextPlayer]--] = -1;
	state->deckCount[nextPlayer]--;
	tributeRevealedCards[1] = state->deck[nextPlayer][state->deckCount[nextPlayer]-1];
	state->deck[nextPlayer][state->deckCount[nextPlayer]--] = -1;
	state->deckCount[nextPlayer]--;
	  }

	  if (tributeRevealedCards[0] == tributeRevealedCards[1]){//If we have a duplicate card, just drop one
	state->playedCards[state->playedCardCount] = tributeRevealedCards[1];
	state->playedCardCount++;
	tributeRevealedCards[1] = -1;
	  }

	  for (i = 0; i <= 2; i ++){
	if (tributeRevealedCards[i] == copper || tributeRevealedCards[i] == silver || tributeRevealedCards[i] == gold){//Treasure cards
	  state->coins += 2;
	}

	else if (tributeRevealedCards[i] == estate || tributeRevealedCards[i] == duchy || tributeRevealedCards[i] == province || tributeRevealedCards[i] == gardens || tributeRevealedCards[i] == great_hall){//Victory Card Found
	  drawCard(currentPlayer, state);
	  drawCard(currentPlayer, state);
	}
	else{//Action Card
	  state->numActions = state->numActions + 2;
	}
	  }

	  return 0;

	case ambassador:
	  j = 0;		//used to check if player has enough cards to discard

	  if (choice2 > 2 || choice2 < 0)
	{
	  return -1;
	}

	  if (choice1 == handPos)
	{
	  return -1;
	}

	  for (i = 0; i < state->handCount[currentPlayer]; i++)
	{
	  if (i != handPos && i == state->hand[currentPlayer][choice1] && i != choice1)
		{
		  j++;
		}
	}
	  if (j < choice2)
	{
	  return -1;
	}

	  if (DEBUG)
	printf("Player %d reveals card number: %d\n", currentPlayer, state->hand[currentPlayer][choice1]);

	  //increase supply count for choosen card by amount being discarded
	  state->supplyCount[state->hand[currentPlayer][choice1]] += choice2;

	  //each other player gains a copy of revealed card
	  for (i = 0; i < state->numPlayers; i++)
	{
	  if (i != currentPlayer)
		{
		  gainCard(state->hand[currentPlayer][choice1], state, 0, i);
		}
	}

	  //discard played card from hand
	  discardCard(handPos, currentPlayer, state, 0);

	  //trash copies of cards returned to supply
	  for (j = 0; j < choice2; j++)
	{
	  for (i = 0; i < state->handCount[currentPlayer]; i++)
		{
		  if (state->hand[currentPlayer][i] == state->hand[currentPlayer][choice1])
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
	  state->coins = state->coins + 2;

	  //see if selected pile is in play
	  if ( state->supplyCount[choice1] == -1 )
	{
	  return -1;
	}

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

	case salvager:
	  //+1 buy
	  state->numBuys++;

	  if (choice1)
	{
	  //gain coins equal to trashed card
	  state->coins = state->coins + getCost( handCard(choice1, state) );
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

int discardCard(int handPos, int currentPlayer, struct gameState *state, int trashFlag)
{

  //if card is not trashed, added to Played pile
  if (trashFlag < 1)
	{
	  //add card to played pile
	  state->playedCards[state->playedCardCount] = state->hand[currentPlayer][handPos];
	  state->playedCardCount++;
	}

  //set played card to -1
  state->hand[currentPlayer][handPos] = -1;

  //remove card from player's hand
  if ( handPos == (state->handCount[currentPlayer] - 1) ) 	//last card in hand array is played
	{
	  //reduce number of cards in hand
	  state->handCount[currentPlayer]--;
	}
  else if ( state->handCount[currentPlayer] == 1 ) //only one card in hand
	{
	  //reduce number of cards in hand
	  state->handCount[currentPlayer]--;
	}
  else
	{
	  //replace discarded card with last card in hand
	  state->hand[currentPlayer][handPos] = state->hand[currentPlayer][ (state->handCount[currentPlayer] - 1)];
	  //set last card to -1
	  state->hand[currentPlayer][state->handCount[currentPlayer] - 1] = -1;
	  //reduce number of cards in hand
	  state->handCount[currentPlayer]--;
	}

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
int updateCoins(int player, struct gameState *state, int bonus)
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