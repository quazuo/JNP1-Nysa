#include <iostream>
#include <regex>
#include <unordered_map>
#include <forward_list>
#include <unordered_set>

typedef std::unordered_map<uint32_t, std::pair<std::string, std::forward_list<uint32_t>>> gate_map;
typedef std::unordered_map<uint32_t, std::forward_list<uint32_t>> signal_map;

// Adds helper <IN, x> pairs to the `gates` map, where x is the input signal's id.
void add_in_gates(gate_map &gates) {
    std::unordered_set<uint32_t> in_signals;

    // Filling the `in_signals` set with all encountered signal id's.
    for (const auto &gate : gates) {
        for (auto signal : gate.second.second) {
            in_signals.insert(signal);
        }
    }

    // Removing from `in_signals` those signal id's, that are outputs of some logic gate.
    for (const auto &gate : gates) {
        in_signals.erase(gate.first);
    }

    // Adding helper pairs to the map.
    for (auto in_signal : in_signals) {
        std::forward_list<uint32_t> new_list;
        gates.emplace(in_signal, std::pair<std::string, std::forward_list<uint32_t>>("IN", new_list));
    }
}

void topoSortHelper(const gate_map &gates, uint32_t signal_id,
                    std::unordered_set<uint32_t> &visited, std::stack<uint32_t> &s) {
    visited.insert(signal_id);

    std::forward_list<uint32_t> signal_inputs = gates.at(signal_id).second;
    for (auto input : signal_inputs) {
        if (!visited.contains(input))
            topoSortHelper(gates, input, visited, s);
    }

    s.push(signal_id);
}

std::forward_list<uint32_t> topoSort(const gate_map &gates) {
    std::stack<uint32_t> s;
    std::unordered_set<uint32_t> visited;
    std::unordered_set<uint32_t> all_signals;

    for (const auto &gate: gates) {
        std::forward_list<uint32_t> curr_list = gate.second.second;
        for (auto signal: curr_list) {
            all_signals.insert(signal);
        }
        all_signals.insert(gate.first);
    }

    for (auto signal : all_signals) {
        if (!visited.contains(signal))
            topoSortHelper(gates, signal, visited, s);
    }

    // Inverting the stack and putting the contents into a forward_list.
    std::forward_list<uint32_t> result;
    while (!s.empty()) {
        uint32_t n = s.top();
        s.pop();
        result.push_front(n);
    }

    return result;
}

bool evaluate_signal(const gate_map &gates, std::map<uint32_t, bool> &signal_values,
                     uint32_t signal_id) {
    std::string curr_gate_type = gates.at(signal_id).first;
    std::forward_list<uint32_t> gate_inputs = gates.at(signal_id).second;
    bool result = true;

    if (curr_gate_type == "IN") {
        result = !signal_values[signal_id];
    } else if (curr_gate_type == "NOT") {
        result = !signal_values[gate_inputs.front()];
    } else if (curr_gate_type == "AND" || curr_gate_type == "NAND") {
        result = true;

        for (uint32_t input: gate_inputs)
            result &= signal_values[input];

    } else if (curr_gate_type == "OR" || curr_gate_type == "NOR") {
        result = false;

        for (uint32_t input: gate_inputs)
            result |= signal_values[input];

    } else if (curr_gate_type == "XOR") {
        auto i = gate_inputs.begin();
        bool first_input_value = signal_values.at(*i);
        bool second_input_value = signal_values.at(*(++i));
        result = first_input_value ^ second_input_value;
    }

    if (curr_gate_type == "NOR" || curr_gate_type == "NAND")
        result = !result;

    signal_values[signal_id] = result;

    return result;
}

// Evaluates all signal values in a specific case of initial (input) signal values.
std::map<uint32_t, bool> calc_signal_values(const std::vector<uint32_t> &input_signals,
                                             uint32_t line_index, const gate_map &gates,
                                             const std::forward_list<uint32_t> &list) {
    std::map<uint32_t, bool> signal_values;
    uint32_t n = line_index;

    // Adding input signal values to the `signal_values` map.
    for (uint32_t inputSignal : input_signals) {
        signal_values[inputSignal] = (n % 2 == 1);
        n /= 2;
    }

    for (auto signal : list) {
        evaluate_signal(gates, signal_values, signal);
    }

    return signal_values;
}

void print_truth_line(std::map<uint32_t, bool> &signal_values) {
    for (auto signal_value: signal_values) {
        std::cout << (signal_value.second ? 1 : 0);
    }

    std::cout << "\n";
}

void print_truth_table(const gate_map &gates,
                       const std::forward_list<uint32_t> &list) {
    std::vector<uint32_t> input_signals;
    uint32_t lines_count = 1;

    // Calculating how many rows the truth table should contain and also
    // filling the input signal id's vector.
    for (const auto &gate : gates) {
        if (gate.second.first == "IN") {
            input_signals.push_back(gate.first);
            lines_count *= 2;
        }
    }

    // Sorting the `input_signals` vector so that we can properly set
    // initial signal values before calculating the rest of the signal values.
    std::sort(input_signals.begin(), input_signals.end(), std::greater());

    // Printing each line of the truth table.
    for (uint32_t line_index = 0; line_index < lines_count; line_index++) {
        std::map<uint32_t, bool> signal_values =
            calc_signal_values(input_signals, line_index, gates, list);
        print_truth_line(signal_values);
    }
}

// Logs an error in case of an invalid line.
void log_error(const uint32_t line_number, std::string &line) {
    std::cerr << "Error in line " << line_number << ": " << line << "\n";
}

// Logs an error in case of a signal marked as an output of two logic gates.
void log_multi_outputs_error(const uint32_t line_number, uint32_t &output) {
    std::cerr << "Error in line " << line_number << ": signal "
              << output << " is assigned to multiple outputs.\n";
}

// Logs an error in case of a cycle.
void log_cycle_error() {
    std::cerr << "Error: sequential logic analysis has not yet been implemented.\n";
}

// Parses a string contaning signal values and outputs them as a vector.
std::vector<uint32_t> match_signals(std::string &text) {
    static const std::regex signal_regex("[1-9]\\d{0,8}");
    std::smatch match;
    std::regex_search(text, match, signal_regex);
    std::vector<uint32_t> signals;
    std::string::const_iterator search_start(text.cbegin());

    while (regex_search(search_start, text.cend(), match, signal_regex)) {
        signals.emplace_back(stoi(match[0]));
        search_start = match.suffix().first;
    }

    return signals;
}

bool check_if_multi_outputs(uint32_t signal, std::unordered_set<uint32_t> &exits) {
    if (exits.contains(signal)) {
        return true;
    }
    return false;
}

void parse_line(uint32_t line_number, std::string &line, std::unordered_set<uint32_t> &exits,
                signal_map &signals_map, bool &if_error, gate_map &gates) {
    static const std::regex not_regex(R"(^\s*NOT(\s+[1-9]\d{0,8}){2}\s*$)");
    static const std::regex xor_regex(R"(^\s*XOR(\s+[1-9]\d{0,8}){3}\s*$)");
    static const std::regex else_regex(R"(^\s*N?(AND|OR)(\s+[1-9]\d{0,8}){3,}\s*$)");
    static const std::regex signal_regex("[1-9]\\d{0,8}");
    static const std::regex key_regex("N?(AND|OR)");

    if (line.empty()) {
        return;
    } else if (!std::regex_match(line, not_regex) &&
               !std::regex_match(line, xor_regex) &&
               !std::regex_match(line, else_regex)) {
        log_error(line_number, line);
        if_error = true;
        return;
    }

    std::vector<uint32_t> signals = match_signals(line);
    if (check_if_multi_outputs(signals[0], exits))
        log_multi_outputs_error(line_number, signals[0]);
    exits.emplace(signals[0]);

    // Adding edges to the `gates` graph.
    for (size_t i = 1; i < signals.size(); ++i) {
        signals_map[signals[i]].push_front(signals[0]);
        gates[signals[0]].second.push_front(signals[i]);
    }

    // Adding logic gate names.
    if (std::regex_match(line, not_regex)) {
        gates[signals[0]].first = "NOT";
    } else if (std::regex_match(line, xor_regex)) {
        gates[signals[0]].first = "XOR";
    } else if (std::regex_match(line, else_regex)) {
        std::smatch match;
        std::regex_search(line, match, key_regex);
        gates[signals[0]].first = match.str(0);
    }

}

// Checks if a signal is part of a cycle.
bool is_in_cycle(signal_map &signals_map, uint32_t v, std::unordered_set<uint32_t> &visited,
                 std::unordered_set<uint32_t> &stack) {
    if (visited.contains(v)) {
        visited.emplace(v);
        stack.emplace(v);

        for (auto signal : signals_map[v]) {
            if ((visited.find(signal) == visited.end() &&
                 is_in_cycle(signals_map, signal, visited, stack)) ||
                (stack.find(signal) != stack.end()))
                return true;
        }
    }

    stack.erase(v);
    return false;
}

bool has_cycle(signal_map &signals_map) {
    std::unordered_set<uint32_t> visited;
    std::unordered_set<uint32_t> stack;

    for (const auto &signal : signals_map) {
        if (is_in_cycle(signals_map, signal.first, visited, stack)) {
            return true;
        }
    }
    return false;
}

int main() {
    
    std::string line;
    uint32_t line_number = 1;
    std::unordered_set<uint32_t> exits;
    signal_map signals_map;
    gate_map gates;
    bool if_error = false;

    while (std::getline(std::cin, line)) {
        parse_line(line_number++, line, exits, signals_map, if_error, gates);
    }
    if (if_error) {
        return 0;
    }

    if (has_cycle(signals_map)) {
        log_cycle_error();
        return 0;
    }

    add_in_gates(gates);
    std::forward_list<uint32_t> gates_path = topoSort(gates);
    print_truth_table(gates, gates_path);

    return 0;
}