#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <optional>
#include <fstream>
#include <filesystem>
#include <cstdlib>
/*#include <sstream>
#include <iomanip>
#include <cctype>
#include <stdexcept>*/

#include "nlohmann/json.hpp"

enum class LocationTokenType {
	Invalid = -1,
	GRASS, LUMBER, STONE, FIELD, BRICK, WATER
};

using TokensMap = std::unordered_map<LocationTokenType, unsigned int>;

const TokensMap all_tokens_initial = {
		{LocationTokenType::GRASS, 19},
		{LocationTokenType::LUMBER, 21},
		{LocationTokenType::STONE, 23},
		{LocationTokenType::FIELD, 19},
		{LocationTokenType::BRICK, 15},
		{LocationTokenType::WATER, 23}
};

NLOHMANN_JSON_SERIALIZE_ENUM(LocationTokenType, {
	{LocationTokenType::Invalid, nullptr},
	{LocationTokenType::GRASS, 'g'},
	{LocationTokenType::LUMBER, 'l'},
	{LocationTokenType::STONE, 's'},
	{LocationTokenType::FIELD, 'f'},
	{LocationTokenType::BRICK, 'b'},
	{LocationTokenType::WATER, 'w'}
})

struct Bowl {
	TokensMap tokens;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Bowl, tokens);

constexpr unsigned int c_bowl_market_capacity = 5;

struct GameState {
	TokensMap all_tokens;
	std::vector<Bowl> bowls_available = std::vector<Bowl>(c_bowl_market_capacity);
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GameState, all_tokens, bowls_available)

GameState game_state;

bool token_type_is_valid(const LocationTokenType& token_type) {
	return token_type != LocationTokenType::Invalid
		&& token_type >= LocationTokenType::GRASS
		&& token_type <= LocationTokenType::WATER;
}

bool tokens_are_valid(const TokensMap& tokens) {
	for (const auto& [token_type, count] : tokens) {
		if (!token_type_is_valid(token_type) || count > all_tokens_initial.at(token_type)) {
			return false;
		}
	}
	return true;
}

bool game_state_is_valid() {
	if (game_state.all_tokens.empty() || game_state.bowls_available.size() != c_bowl_market_capacity) {
		return false;
	}
	if (!tokens_are_valid(game_state.all_tokens)) {
		return false;
	}
	for (const auto& bowl : game_state.bowls_available) {
		if (!tokens_are_valid(bowl.tokens)) {
			return false;
		}
	}
	return true;
}

std::optional<Bowl> parse_bowl_string(std::string& bowl_string) {
	if (bowl_string.size() == 3) {
		Bowl bowl;
		for (char symbol : bowl_string) {
			LocationTokenType token_type = nlohmann::json(symbol).get<LocationTokenType>();

			if (token_type != LocationTokenType::Invalid) {
				bowl.tokens[token_type]++;
			}
			else {
				// didn't find key
				std::cout << "Error: wrong symbol in string" << std::endl;
				return std::nullopt;
			}
		}
		return bowl;
	}
	else {
		std::cout << "Error: wrong string size" << std::endl;
		return std::nullopt;
	}
}

void reduce_tokens_in_bag(const Bowl& bowl) {
	for (const auto& [token_type, count] : bowl.tokens) {
		auto it = game_state.all_tokens.find(token_type);
		if (it == game_state.all_tokens.end()) {
			std::cout << "Error: unknown token type in the bowl" << std::endl;
			continue;
		}
		if (it->second < count) {
			std::cout << "Error: not enough tokens of this type left in the bag" << std::endl;
			continue;
		}
		it->second -= count;
	}
}

std::optional<Bowl> init_new_bowl() {
	std::string init_bowl_string;
	std::optional<Bowl> bowl_opt = std::nullopt;
	do {
		std::cout << "Init new bowl" << std::endl;
		std::cin >> init_bowl_string;
		bowl_opt = parse_bowl_string(init_bowl_string);
	} while (!bowl_opt);
	reduce_tokens_in_bag(bowl_opt.value());
	return bowl_opt;
}

void bowl_setup() {
	std::cout << "Setup bowls" << std::endl;
	for (int i = 0; i < c_bowl_market_capacity; i++) {
		auto bowl_opt = init_new_bowl();
		game_state.bowls_available[i] = bowl_opt.value();
		std::cout << "Bowl added" << std::endl;
	}
	std::cout << "Bowls setup finished" << std::endl;
	std::cout << "---------------------------------------------------------------------" << std::endl;
}

const std::string RESET = "\033[0m";
const std::string GREEN = "\033[32m";
const std::string BROWN = "\033[33m";
const std::string GREY = "\033[90m";
const std::string YELLOW = "\033[93m";
const std::string RED = "\033[31m";
const std::string BLUE = "\033[34m";

const std::unordered_map<LocationTokenType, std::string> c_token_colors_map = {
		{LocationTokenType::GRASS, GREEN + "Grass" + RESET},
		{LocationTokenType::LUMBER, BROWN + "Lumber" + RESET},
		{LocationTokenType::STONE, GREY + "Stone" + RESET},
		{LocationTokenType::FIELD, YELLOW + "Field" + RESET},
		{LocationTokenType::BRICK, RED + "Brick" + RESET},
		{LocationTokenType::WATER, BLUE + "Water" + RESET}
};

void show_bowls() {
	for (int i = 0; i < game_state.bowls_available.size(); i++) {
		std::cout << "Bowl " << i + 1 << ":";
		for (auto& [key, value] : game_state.bowls_available[i].tokens) {
			for (int j = value; j > 0; j--) {
				std::cout << " " << c_token_colors_map.at(key);
			}
		}
		std::cout << std::endl;
	}
	std::cout << "---------------------------------------------------------------------" << std::endl;
}

double at_least_one_token(int count, int sum) {
	long long up = (static_cast<long long>(sum) - count) * (sum - count - 1) * (sum - count - 2);
	long long down = static_cast<long long>(sum) * (sum - 1) * (sum - 2);
	double result = 1 - static_cast<double>(up) / down;
	return result;
	
}

void show_bag_statistics() {
	unsigned int sum = 0;
	for (auto [key, count] : game_state.all_tokens) {
		sum += count;
	}
	for (auto [key, count] : game_state.all_tokens) {
		// main count 
		std::cout << c_token_colors_map.at(key) << ": " << count << std::endl;
		// Probability percents
		double probability = at_least_one_token(count, sum);
		
		std::stringstream ss;
		ss << std::fixed << std::setprecision(1) << (probability * 100.0) << "%";
		std::string percent_str = ss.str();
		
		std::cout << "Percentages for at least one: " << percent_str << std::endl;
	}
	std::cout << std::endl;
	std::cout << "Tokens left in the bag: " << sum << std::endl;
	std::cout << "---------------------------------------------------------------------" << std::endl;
}

void check_used_tokens() {
	std::cout << "****" << std::endl;
	std::cout << "Used tokens (for check purposes): " << std::endl;
	unsigned int total_sum = 0;
	for (auto [key, count] : all_tokens_initial) {
		total_sum += count;
	}
	unsigned int sum = 0;
	for (auto [key, count] : game_state.all_tokens) {
		sum += count;
	}
	for (auto [key, count] : all_tokens_initial) {
		// main count 
		std::cout << c_token_colors_map.at(key) << ": " << count - game_state.all_tokens.at(key) << std::endl;
	}
	std::cout << std::endl;
	std::cout << "Tokens used count: " << total_sum - sum << std::endl;
	std::cout << "---------------------------------------------------------------------" << std::endl;
}

// Returns false when the user chose to quit (or the input ended).
bool refresh_bowl() {
	std::cout << "Which bowl number was removed? (1-5, 'q' to quit)" << std::endl;
	std::string input;
	if (!(std::cin >> input)) {
		return false;
	}

	if (input == "q" || input == "Q") {
		return false;
	}

	int index = -1;
	try {
		index = std::stoi(input);
	}
	catch (const std::exception&) {
		std::cout << "Error: invalid input. Enter a number 1-5 or 'q'." << std::endl;
		return true;
	}

	if (index < 1 || index > static_cast<int>(game_state.bowls_available.size())) {
		std::cout << "Error: bowl number must be between 1 and 5" << std::endl;
		return true;
	}

	auto bowl_opt = init_new_bowl();
	game_state.bowls_available[index - 1] = bowl_opt.value();
	return true;
}

const std::string c_state_file_name = "state.txt";

bool save_state_to_file() {
	if (game_state.all_tokens.empty()) {
		return false;
	}

	std::ofstream file(c_state_file_name);

	// Always check if the file opened successfully
	if (!file.is_open()) {
		std::cerr << "Error: Could not open file for writing.\n";
		return false;
	}

	nlohmann::json serialized_json = game_state;
	file << serialized_json.dump() << std::endl;

	return true;
}

bool load_state_from_file() {
	std::ifstream file(c_state_file_name);

	if (!file.is_open()) {
		std::cerr << "Error: Could not open file for reading.\n";
		return false;
	}

	try {
		nlohmann::json serialized_json;

		// Parse directly from the file stream
		file >> serialized_json;

		// Reject files saved by other versions: the old format used numeric
		// char codes, the current one uses string keys.
		if (!serialized_json.is_object()
			|| !serialized_json.contains("all_tokens")
			|| !serialized_json.contains("bowls_available")
			|| !serialized_json["bowls_available"].is_array()) {
			std::cerr << "Error: state file has an incompatible format." << std::endl;
			return false;
		}
		for (const auto& bowl_json : serialized_json["bowls_available"]) {
			if (!bowl_json.is_object() || !bowl_json.contains("tokens")) {
				std::cerr << "Error: state file has an incompatible format." << std::endl;
				return false;
			}
		}

		// Convert JSON object back to C++ struct
		game_state = serialized_json.get<GameState>();
	}
	catch (const nlohmann::json::exception& e) {
		// Catches syntax errors in the JSON file or missing struct fields
		std::cerr << "JSON Parse Error: " << e.what() << '\n';
		return false;
	}

	return game_state_is_valid();
}

void basic_setup() {
	game_state.all_tokens = all_tokens_initial;
	bowl_setup();
}

void choose_token_map_initialization() {
	std::filesystem::path file_path = c_state_file_name;
	if (std::filesystem::exists(file_path) && std::filesystem::is_regular_file(file_path)) {
		std::cout << "Found previous file for state. Do you want to download it? (y/n)" << std::endl;
		bool invalid_choice = true;
		char choice;

		do {
			std::cin >> choice;
			// Convert input to lowercase to handle both 'Y' and 'y'
			choice = std::tolower(static_cast<unsigned char>(choice));

			if (choice == 'y') {
				if (load_state_from_file()) {
					std::cout << "State loaded" << std::endl;
				}
				else {
					std::cout << "Starting with a new setup." << std::endl;
					basic_setup();
				}
				invalid_choice = false;
			}
			else if (choice == 'n') {
				basic_setup();
				invalid_choice = false;
			}
			else {
				std::cout << "Error: Invalid choice format. Repeat, please" << std::endl;
			}
		} while (invalid_choice);
	}
	else {
		basic_setup();
	}
}

int main() {

	/*
	std::atexit([]() {
		save_state_to_file();
	});
	*/

	choose_token_map_initialization();
	
	while (true) { // I think about rectriction later
		
		save_state_to_file();

		show_bag_statistics();
		show_bowls();
		check_used_tokens();
		if (!refresh_bowl()) {
			break; // quit
		}
	}

	// Final save on exit. Note: don't use std::atexit for this — the enum-to-
	// JSON lookup tables are function-local statics initialized on the first
	// save, so at exit they would be destroyed BEFORE an atexit handler runs.
	save_state_to_file();

	return 0;
}