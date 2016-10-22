#include "ClueReasoner.h"
using namespace std;

int ClueReasoner::GetPlayerNum(string player)
{
	if (player == case_file)
		return num_players;
	
	for (int i = 0; i < num_players; i++)
		if (player == players[i])
			return i;
			
	cout<<"Illegal player: "<<player<<endl;
	return -1;
}
int ClueReasoner::GetCardNum(string card)
{
	for (int i = 0; i < num_cards; i++)
		if (card == cards[i])
			return i;
			
	cout<<"Illegal card: "<<card<<endl;
	return -1;
}

string ClueReasoner::QueryString(int return_code)
{
	if (return_code == kFalse)
		return "n";
	else if (return_code == kTrue)
		return "Y";
	else
		return "-";
}

void ClueReasoner::PrintNotepad()
{
	for (int i = 0; i < num_players; i++)
		cout<<"\t"<<players[i];
	cout<<"\t"<<case_file<<endl;
	
	for (int i = 0; i < num_cards; i++)
	{
		cout<<cards[i]<<"\t";
		for (int j = 0; j < num_players; j++)
			cout<<QueryString(Query(players[j], cards[i]))<<"\t";
		
		cout<<QueryString(Query(case_file, cards[i]))<<endl;
	}
}
	
void ClueReasoner::AddInitialClauses()
{
	// Each card is in at least one place (including the case file).
	for (int c = 0; c < num_cards; c++)
	{
		Clause clause;
		for (int p = 0; p <= num_players; p++)
			clause.push_back(GetPairNum(p, c));
		solver->AddClause(clause);
	}

	// If a card is in one place, it cannot be in another place (including the case file)
	// for all cards...
	// c1 = card is one place
	// c2 = card in another place
	// c1 => ~c2
	// ~c1 v ~c2 
	for(int i = 0; i <= num_players; i++) {
		for(int j = 0; j < num_cards; j++) {
			for(int k = 0; k <= num_players; k++) {
				if(i != k) {
					Clause clause;
					clause.push_back(-GetPairNum(i, j));
					clause.push_back(-GetPairNum(k, j));
					solver->AddClause(clause);
				}
			}
		}
	}

	// At least one card of each category is in the case file.
	Clause clause;
	for(int i = 0; i < num_suspects; i++)
		clause.push_back(GetPairNum(case_file, suspects[i]));
	solver->AddClause(clause);
	clause.clear();

	for(int i = 0; i < num_weapons; i++)
		clause.push_back(GetPairNum(case_file, weapons[i]));
	solver->AddClause(clause);
	clause.clear();

	for(int i = 0; i < num_rooms; i++)
		clause.push_back(GetPairNum(case_file, rooms[i]));
	solver->AddClause(clause);
	clause.clear();


	// No two cards in each category can both be in the case file.
	// InPlace(cf, Y) => (!InPlace(cf, z) AND (y != z))
	// ...!InPlace(cf,Y) OR (!InPlace(cf, z) AND (y != z)
	// for all categories:
	// c1 = card is in case file
	// c2 = another card is in case file
	// c1 => ~c2
	// ~c1 v ~c2

	for(int i = 0; i < num_suspects; i++) {
		for(int j = 0; j < num_suspects; j++) {
			if(i != j) {
				Clause suspect_clause;
				suspect_clause.push_back(-GetPairNum(case_file, suspects[i]));
				suspect_clause.push_back(-GetPairNum(case_file, suspects[j]));
				solver->AddClause(suspect_clause); 
			}
		}
	}

	for(int i = 0; i < num_weapons; i++) {
		for(int j = 0; j < num_weapons; j++) {
			if(i != j) {
				Clause weapon_clause;
				weapon_clause.push_back(-GetPairNum(case_file, weapons[i]));
				weapon_clause.push_back(-GetPairNum(case_file, weapons[j]));
				solver->AddClause(weapon_clause); 
			}
		}
	}

	for(int i = 0; i < num_rooms; i++) {
		for(int j = 0; j < num_rooms; j++) {
			if(i != j) {
				Clause room_clause;
				room_clause.push_back(-GetPairNum(case_file, rooms[i]));
				room_clause.push_back(-GetPairNum(case_file, rooms[j]));
				solver->AddClause(room_clause); 
			}
		}
	}
}

void ClueReasoner::Hand(string player, string player_cards[3])
{
	player_num = GetPlayerNum(player);

	// add my cards to KB
	for(int i = 0; i < 3; i++) {
		Clause clause;
		clause.push_back(GetPairNum(player, player_cards[i]));
		solver->AddClause(clause);
		clause.clear();
	}
}

void ClueReasoner::Suggest(string suggester, string card1, string card2, string card3, string refuter, string card_shown)
{
	// all players but suggester do not have the three cards
	if(refuter == "") {
		Clause clause;
		for(int i = 0; i < num_players; i++) {
			if(i != GetPlayerNum(suggester)) {
				clause.push_back(-GetPairNum(players[i], card1)); solver->AddClause(clause); clause.clear();
				clause.push_back(-GetPairNum(players[i], card2)); solver->AddClause(clause); clause.clear();
				clause.push_back(-GetPairNum(players[i], card3)); solver->AddClause(clause); clause.clear();
			}
		}
	}
	else {
		// everybody between suggester and refuter do not have any of the cards
		int suggesterIndex = GetPlayerNum(suggester);
		int refuterIndex = GetPlayerNum(refuter);
		int i = ++suggesterIndex % num_players;

		while(i != refuterIndex) {
			Clause clause;
			clause.push_back(-GetPairNum(players[i], card1)); solver->AddClause(clause); clause.clear();
			clause.push_back(-GetPairNum(players[i], card2)); solver->AddClause(clause); clause.clear();
			clause.push_back(-GetPairNum(players[i], card3)); solver->AddClause(clause); clause.clear();
			i = (++i) % num_players;
		}

		// player did not see the card
		if(card_shown == "") { 
			// refuter has at least one of the three cards
			Clause clause;
			clause.push_back(GetPairNum(refuter, card1)); clause.push_back(GetPairNum(refuter, card2)); clause.push_back(GetPairNum(refuter, card3));
			solver->AddClause(clause); clause.clear();
		} 
		else {
			// refuter has this card
			Clause clause;
			clause.push_back(GetPairNum(refuter, card_shown));
			solver->AddClause(clause); clause.clear();
		}
	}
}

void ClueReasoner::Accuse(string suggester, string card1, string card2, string card3, bool is_correct)
{
}
