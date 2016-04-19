#ifndef _DOMINION_HELPERS_H
#define _DOMINION_HELPERS_H

#include "dominion.h"

#define PUSH(pile, player, val) state->pile[player][state->pile##Count[player]++] = val
#define TOP(pile, player) state->pile[player][state->pile##Count[player]-1]
#define POP(pile, player) state->pile##Count[player]--
#define POP_R(pile, player) state->pile[player][--state->pile##Count[player]]

int drawCard(int player, struct gameState *state);
int updateCoins(int player, struct gameState *state, int bonus);
int discardCard(int handPos, int currentPlayer, struct gameState *state, int trashFlag);
int safeDiscard(int card, int currentPlayer, struct gameState* state, int trashFlag);
int gainCard(int supplyPos, struct gameState *state, int toFlag, int player);
int getCost(int cardNumber);
int isVictory(int card);
int isTreasure(int card);
int cardEffect(int card, int choice1, int choice2, int choice3, struct gameState *state, int handPos, int *bonus);

#endif