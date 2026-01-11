#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <string>
#include <iomanip>
#include <chrono>


// 1. Definitions


enum Rank { TWO = 2, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, JACK, QUEEN, KING, ACE };
enum Suit { HEARTS, DIAMONDS, CLUBS, SPADES };

struct Card {
    int rank; // 2-14 (where 14 is Ace)
    int suit; // 0-3

    // checking whether cards are the same
    bool operator==(const Card& other) const {
        return rank == other.rank && suit == other.suit;
    }
    
    std::string toString() const {
        std::string r = std::to_string(rank);
        if(rank == 11) r = "J";
        if(rank == 12) r = "Q";
        if(rank == 13) r = "K";
        if(rank == 14) r = "A";
        
        char s = '?';
        if(suit == HEARTS) s = 'H'; // Hearts 
        if(suit == DIAMONDS) s = 'D'; // Diamonds 
        if(suit == CLUBS) s = 'C'; // Clubs 
        if(suit == SPADES) s = 'S'; // Spades 
        
        return r + s;
    }
};

enum HandStrength {
    HIGH_CARD, PAIR, TWO_PAIR, THREE_OF_A_KIND, STRAIGHT, FLUSH, FULL_HOUSE, FOUR_OF_A_KIND, STRAIGHT_FLUSH
};


// 2. Deck 


class Deck {
private:
    std::vector<Card> cards;
    std::mt19937 rng; 

public:
    Deck() {
        std::random_device rd;
        rng = std::mt19937(rd());
        reset();
    }

    void reset() {
        cards.clear();
        for (int s = 0; s < 4; s++) {
            for (int r = 2; r <= 14; r++) {
                cards.push_back({r, s});
            }
        }
    }

    void removeCards(const std::vector<Card>& toRemove) {
        for (const auto& removeCard : toRemove) {
            auto it = std::remove(cards.begin(), cards.end(), removeCard);
            if (it != cards.end()) {
                cards.erase(it, cards.end());
            }
        }
    }

    void shuffle() {
        std::shuffle(cards.begin(), cards.end(), rng);
    }

    std::vector<Card> deal(int count) {
        std::vector<Card> dealt;
        for (int i = 0; i < count; i++) {
            if (!cards.empty()) {
                dealt.push_back(cards.back());
                cards.pop_back();
            }
        }
        return dealt;
    }
};


// 3. Hand evaluator


class HandEvaluator {
public:
    static std::pair<HandStrength, int> evaluate(const std::vector<Card>& holeCards, const std::vector<Card>& board) {
        std::vector<Card> allCards = holeCards;
        allCards.insert(allCards.end(), board.begin(), board.end());

        int rankCounts[15] = {0};
        int suitCounts[4] = {0};

        for (const auto& card : allCards) {
            rankCounts[card.rank]++;
            suitCounts[card.suit]++;
        }

        bool flush = false;
        for (int s = 0; s < 4; s++) {
            if (suitCounts[s] >= 5) { flush = true; break; }
        }

        bool straight = false;
        int straightHighRank = 0;
        int consecutive = 0;
        for (int r = 14; r >= 2; r--) {
            if (rankCounts[r] > 0) consecutive++;
            else consecutive = 0;
            
            if (consecutive >= 5) {
                straight = true;
                straightHighRank = r + 4;
                break;
            }
        }
        if (!straight && rankCounts[14]>0 && rankCounts[2]>0 && rankCounts[3]>0 && rankCounts[4]>0 && rankCounts[5]>0) {
            straight = true;
            straightHighRank = 5;
        }

        bool fourKind = false;
        bool threeKind = false;
        int pairsCount = 0;
        int highRankMatch = 0;

        for (int r = 14; r >= 2; r--) {
            if (rankCounts[r] == 4) { fourKind = true; highRankMatch = r; }
            if (rankCounts[r] == 3) { threeKind = true; if(highRankMatch==0) highRankMatch = r; }
            if (rankCounts[r] == 2) { pairsCount++; if(highRankMatch==0) highRankMatch = r; }
        }

        if (straight && flush) return {STRAIGHT_FLUSH, straightHighRank};
        if (fourKind) return {FOUR_OF_A_KIND, highRankMatch};
        if (threeKind && pairsCount >= 1) return {FULL_HOUSE, highRankMatch}; 
        if (flush) return {FLUSH, 0}; 
        if (straight) return {STRAIGHT, straightHighRank};
        if (threeKind) return {THREE_OF_A_KIND, highRankMatch};
        if (pairsCount >= 2) return {TWO_PAIR, highRankMatch};
        if (pairsCount == 1) return {PAIR, highRankMatch};
        
        for(int r=14; r>=2; r--) {
            if(rankCounts[r] > 0) return {HIGH_CARD, r};
        }
        return {HIGH_CARD, 0};
    }
};


// 4. Monte Carlo


void runMonteCarlo(Card c1, Card c2, std::vector<Card> initialBoard, int iterations) {
    std::cout << "\n>>> Simulation (" << iterations << " games)...\n";
    std::cout << "Your hand: " << c1.toString() << " " << c2.toString() << "\n";
    std::cout << "Table (" << initialBoard.size() << " cards): ";
    if (initialBoard.empty()) std::cout << "Empty (Pre-flop)";
    else for (auto c : initialBoard) std::cout << c.toString() << " ";
    std::cout << "\n--------------------------------\n";

    // Timer start
    auto start = std::chrono::high_resolution_clock::now();

    int wins = 0;
    int ties = 0;

    // Statistics: Table of counters for each type of hand (0-8)
    int winStats[9] = {0}; 

    std::vector<Card> knownCards = initialBoard;
    knownCards.push_back(c1);
    knownCards.push_back(c2);

    Deck simulationDeck; 

    for (int i = 0; i < iterations; i++) {
        simulationDeck.reset();
        simulationDeck.removeCards(knownCards);
        simulationDeck.shuffle();

        std::vector<Card> oppHand = simulationDeck.deal(2);
        
        std::vector<Card> currentBoard = initialBoard;
        int cardsNeeded = 5 - currentBoard.size();
        if (cardsNeeded > 0) {
            std::vector<Card> newBoardCards = simulationDeck.deal(cardsNeeded);
            currentBoard.insert(currentBoard.end(), newBoardCards.begin(), newBoardCards.end());
        }

        auto myResult = HandEvaluator::evaluate({c1, c2}, currentBoard);
        auto oppResult = HandEvaluator::evaluate(oppHand, currentBoard);

        if (myResult.first > oppResult.first) {
            wins++;
            winStats[myResult.first]++; // How did we win?
        } else if (myResult.first == oppResult.first) {
            if (myResult.second > oppResult.second) {
                wins++;
                winStats[myResult.first]++;
            }
            else if (myResult.second == oppResult.second) ties++;
        }
    }

    // Timer stop
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    double speed = iterations / elapsed.count();

    double equity = (double)(wins + (ties / 2.0)) / iterations * 100.0;
    
    // Results
    std::cout << ">>> EQUITY: " << std::fixed << std::setprecision(2) << equity << "%\n";
    std::cout << ">>> Time: " << std::setprecision(3) << elapsed.count() << "s ";
    std::cout << "(" << (int)speed << " simulations per second)\n";
    std::cout << "--------------------------------\n";
    std::cout << "EQUITY COMPONENTS:\n"; 

    const std::string handNames[] = {
        "High Card", "Pair", "Two Pair", "3-of-Kind", 
        "Straight", "Flush", "Full House", "4-of-Kind", "Str. Flush"
    };

    for (int i = 0; i < 9; i++) {
        double pct = (double)winStats[i] / iterations * 100.0;
        
        // Display only if hand's chance > 0.1%
        if (pct >= 0.1) {
            std::cout << std::setw(12) << std::left << handNames[i] << ": ";
            
            // Histogram: Different scaling (e.g. 1 '#' = 0.5%)
            // 
            int bars = (int)(pct * 2); 
            for(int b=0; b<bars; b++) std::cout << "#";
            
            std::cout << " " << std::fixed << std::setprecision(2) << pct << "%\n";
        }
    }
    std::cout << "--------------------------------\n";
}

// 5. Main and interface


Card getCardFromUser(std::string prompt) {
    int rank, suit;
    while (true) {
        std::cout << prompt;
        std::cout << " (Rank: 2-14, Suit: 0-3): ";
        // e.g. 14 3 = Ace of Spades
        if (std::cin >> rank >> suit) {
            if (rank >= 2 && rank <= 14 && suit >= 0 && suit <= 3) {
                return {rank, suit};
            }
        } else {
            std::cin.clear(); 
            std::cin.ignore(10000, '\n'); 
        }
        std::cout << "   [!] Error. Try e.g.: 14 3 (Ace of Spades)\n";
    }
}

// Checking whether a card was already used
bool isCardUsed(Card c, Card my1, Card my2, const std::vector<Card>& board) {
    if (c == my1 || c == my2) return true;
    for (const auto& b : board) {
        if (c == b) return true;
    }
    return false;
}

int main() {
    std::cout << "===============================================\n";
    std::cout << "   TEXAS HOLD'EM MONTE CARLO ENGINE (C++)      \n";
    std::cout << "===============================================\n";
    std::cout << "Key (Suits): 0=Hearts(H), 1=Diamonds(D), 2=Clubs(C), 3=Spades(S)\n";
    std::cout << "Key (Ranks):   11=J, 12=Q, 13=K, 14=A\n";
    
    while (true) {
        // 1. Player's cards (user's input)
        std::cout << "\n--- YOUR HAND ---\n";
        Card c1 = getCardFromUser("Card 1");
        Card c2 = getCardFromUser("Card 2");

        if (c1 == c2) {
            std::cout << "   [!] Error: you have two identical cards\n";
            continue;
        }

        // 2. Cards on the table (user's input)
        int boardCount = 0;
        while(true) {
            std::cout << "\nHow many cards are on the table? (0=Preflop, 3=Flop, 4=Turn, 5=River): ";
            std::cin >> boardCount;
            if (boardCount == 0 || boardCount == 3 || boardCount == 4 || boardCount == 5) break;
            std::cout << "   [!] Wrong number of cards. Choose from 0, 3, 4, 5 cards.\n";
        }

        std::vector<Card> board;
        if (boardCount > 0) {
            std::cout << "--- CARDS ON THE TABLE (" << boardCount << ") ---\n";
            for(int i=0; i<boardCount; i++) {
                while(true) {
                    Card bc = getCardFromUser("Table " + std::to_string(i+1));
                    
                    // Checking for duplicats
                    if (isCardUsed(bc, c1, c2, board)) {
                        std::cout << "   [!] Error: This card is already on the table!\n";
                    } else {
                        board.push_back(bc);
                        break;
                    }
                }
            }
        }

        // 3. Simulation running
        runMonteCarlo(c1, c2, board, 1000000); 

        // 4. Restart?
        std::cout << "Another scenario? (y/n): ";
        char choice;
        std::cin >> choice;
        if (choice == 'n' || choice == 'N') break;
    }

    return 0;
}