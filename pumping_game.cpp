#include <algorithm>   // Gives std::equal for palindrome checks.
#include <cctype>      // Offers std::isdigit for validating numeric input.
#include <iostream>    // Handles CLI input/output.
#include <random>      // Provides RNG for pumping length and decomposition choices.
#include <regex>       // Lets us match simple regular-language patterns quickly.
#include <string>      // Core string container for witness handling.
#include <vector>      // Stores the catalog of languages.

// Simple alias so checker declarations stay compact.
using Checker = bool (*)(const std::string &);

// Each supported language bundles its name, a strategy tip, and a membership test.
struct Language {
    std::string name;
    std::string tip;
    Checker check;
};

// Returns true exactly for strings shaped like a^n b^n with n >= 1.
bool isAnBn(const std::string &text) {
    static const std::regex mask("^a+b+$");  // Restrict form to a-run followed by b-run.
    if (!std::regex_match(text, mask)) return false;
    const auto first_b = text.find('b');            // First b index splits the two runs.
    if (first_b == std::string::npos) return false; // Safety guard should never trigger.
    const size_t a_count = first_b;                 // Count of a's equals split position.
    const size_t b_count = text.size() - first_b;   // Count of b's is remaining length.
    return a_count == b_count;                      // Membership requires equal block sizes.
}

// Returns true exactly for strings shaped like a^n b^n c^n with n >= 1.
bool isAnBnCn(const std::string &text) {
    static const std::regex mask("^a+b+c+$");  // Enforce three runs with correct order.
    if (!std::regex_match(text, mask)) return false;
    const auto first_b = text.find('b');             // Locate boundary between a's and b's.
    const auto first_c = text.find('c', first_b);    // Locate boundary between b's and c's.
    if (first_b == std::string::npos || first_c == std::string::npos) return false;
    const size_t a_count = first_b;
    const size_t b_count = first_c - first_b;
    const size_t c_count = text.size() - first_c;
    return a_count == b_count && b_count == c_count;  // All three blocks must match.
}

// Returns true for non-empty palindromes (mirrored strings).
bool isPalindrome(const std::string &text) {
    return !text.empty() && std::equal(text.begin(), text.end(), text.rbegin());
}

// Static catalog of languages plus quick strategic hints.
const std::vector<Language> kLanguages = {
    {"L = { a^n b^n }", "Force y to sit inside the a-block.", isAnBn},
    {"L = { a^n b^n c^n }", "Make y touch two adjacent blocks.", isAnBnCn},
    {"L = { palindromes }", "Crack the symmetry by pumping one half.", isPalindrome},
};

// Prints the menu and returns the user's chosen language entry.
const Language &chooseLanguage() {
    std::cout << "Pick a language to attack (enter number):\n";
    for (size_t idx = 0; idx < kLanguages.size(); ++idx) {
        std::cout << "  " << (idx + 1) << ". " << kLanguages[idx].name << '\n';
    }
    while (true) {
        std::cout << "> ";
        std::string line;
        std::getline(std::cin, line);
        if (!line.empty() && std::all_of(line.begin(), line.end(), ::isdigit)) {
            const size_t pick = std::stoul(line);
            if (pick >= 1 && pick <= kLanguages.size()) return kLanguages[pick - 1];
        }
        std::cout << "Please enter a valid option number.\n";
    }
}

// Chooses a pumping length p using RNG but lets the user override it.
int pickP(std::mt19937 &rng) {
    std::uniform_int_distribution<int> dist(4, 10);  // Keep p modest.
    const int default_p = dist(rng);
    std::cout << "Auto-select pumping length p = " << default_p
              << ". Press Enter to accept or type another p.\n";
    std::cout << "> ";
    std::string line;
    std::getline(std::cin, line);
    if (!line.empty() && std::all_of(line.begin(), line.end(), ::isdigit)) {
        const int candidate = std::stoi(line);
        if (candidate > 1) return candidate;  // Pumping lemma only needs p > 1.
    }
    return default_p;
}

// Prompts for a witness string belonging to the language with |s| >= p.
std::string promptWitness(const Language &lang, int p) {
    std::cout << "Provide s ∈ L with length at least " << p << ".\n";
    while (true) {
        std::cout << "> ";
        std::string s;
        std::getline(std::cin, s);
        if (static_cast<int>(s.size()) < p) {
            std::cout << "Too short for the chosen p. Try again.\n";
            continue;
        }
        if (!lang.check(s)) {
            std::cout << "String not in the language. Try again.\n";
            continue;
        }
        return s;  // Valid witness.
    }
}

// Simple container for the adversary's decomposition.
struct Parts {
    std::string x;
    std::string y;
    std::string z;
};

// Randomly generate xyz with |xy| <= p and |y| > 0.
Parts adversarySplit(const std::string &text, int p, std::mt19937 &rng) {
    const int max_x = std::min(p - 1, static_cast<int>(text.size()) - 1);  // Leave room for y.
    std::uniform_int_distribution<int> pick_x(0, max_x);
    const int x_len = pick_x(rng);
    const int max_y = std::min(p - x_len, static_cast<int>(text.size()) - x_len);
    std::uniform_int_distribution<int> pick_y(1, max_y);
    const int y_len = pick_y(rng);
    return {text.substr(0, x_len), text.substr(x_len, y_len), text.substr(x_len + y_len)};
}

// Build xy^i z via direct string multiplication.
std::string pumpString(const Parts &parts, int i) {
    std::string pumped = parts.x;
    pumped.reserve(parts.x.size() + parts.y.size() * i + parts.z.size());  // Avoid reallocs.
    for (int k = 0; k < i; ++k) pumped += parts.y;  // Repeat y exactly i times.
    pumped += parts.z;
    return pumped;
}

// Prompts the user for a non-negative i value.
int pickI() {
    std::cout << "Pick an i value (0, 1, 2, ...).\n";
    while (true) {
        std::cout << "> ";
        std::string line;
        std::getline(std::cin, line);
        if (!line.empty() && std::all_of(line.begin(), line.end(), ::isdigit)) {
            return std::stoi(line);
        }
        std::cout << "i must be a non-negative integer.\n";
    }
}

// Prints success/failure with an optional improvement hint.
void reportResult(bool stillInLanguage, const Language &lang) {
    if (stillInLanguage) {
        std::cout << "Result still lies inside L → pumping attempt failed.\n";
        std::cout << "Suggestion: " << lang.tip << "\n";
    } else {
        std::cout << "Success! Pumped string left the language, proving non-regularity for this p.\n";
    }
}

// Executes one full proof attempt round.
void playRound(std::mt19937 &rng) {
    const Language &lang = chooseLanguage();
    std::cout << "Claim: This language is not regular. We will try to witness a contradiction.\n";
    const int p = pickP(rng);
    const std::string witness = promptWitness(lang, p);
    const Parts parts = adversarySplit(witness, p, rng);
    std::cout << "Adversary decomposition:\n";
    std::cout << "  x = '" << parts.x << "' (length " << parts.x.size() << ")\n";
    std::cout << "  y = '" << parts.y << "' (length " << parts.y.size() << ")\n";
    std::cout << "  z = '" << parts.z << "' (length " << parts.z.size() << ")\n";
    const int i = pickI();
    const std::string pumped = pumpString(parts, i);
    std::cout << "Pumped string xy^" << i << "z = '" << pumped << "' (length " << pumped.size() << ")\n";
    reportResult(lang.check(pumped), lang);
}

// Program entrypoint that keeps offering rounds until the player declines.
int main() {
    std::mt19937 rng(std::random_device{}());  // Seed RNG once.
    std::cout << "Pumping Lemma Practice Game — break the adversary's decomposition!\n\n";
    while (true) {
        playRound(rng);
        std::cout << "Play another round? (y/N) ";
        std::string line;
        std::getline(std::cin, line);
        if (line != "y" && line != "Y") {
            std::cout << "Thanks for playing. Keep pumping!\n";
            break;
        }
    }
    return 0;
}
