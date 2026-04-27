#include "service/repl_service.hpp"
#include "service/analysis_service.hpp"
#include "engine/data_engine.hpp"
#include "ui/ui.hpp"
#include "ui/rendering.hpp"
#include "core/paths.hpp"
#include "linenoise.h"
#include <iostream>
#include <algorithm>

namespace Axiom::Service {

void ReplService::run() {
    char* line;
    std::string h_path = Paths::get_path("history");
    linenoiseHistoryLoad(h_path.c_str());

    while (true) {
        std::string prompt = "\033[38;2;26;158;110maxiom > \033[0m";
        line = linenoise(prompt.c_str());
        
        if (!line) break;

        std::string input(line);
        free(line);

        // Trim
        input.erase(0, input.find_first_not_of(" \t\n\r"));
        input.erase(input.find_last_not_of(" \t\n\r") + 1);

        if (input.empty()) continue;
        if (input == "exit" || input == "quit") break;
        if (input == "clear") {
            std::cout << "\033[2J\033[H";
            continue;
        }

        linenoiseHistoryAdd(input.c_str());
        linenoiseHistorySave(h_path.c_str());

        std::string cmd = "", target = input;
        if (input.starts_with("p ") || input.starts_with("predict ")) {
            cmd = "predict";
            target = input.substr(input.find(' ') + 1);
        } else if (input.starts_with("a ") || input.starts_with("analyze ")) {
            cmd = "analyze";
            target = input.substr(input.find(' ') + 1);
        } else if (input.starts_with("profile ")) {
            cmd = "profile";
            target = input.substr(input.find(' ') + 1);
        }

        // Fuzzy Resolution
        auto matches = DataEngine::resolve_symbol(target);
        if (matches.empty()) {
            std::cout << UI::fg(UI::CRIMSON) << "✖ No matches found for \"" << input << "\"" << UI::RESET << "\n";
            continue;
        }

        if (matches.size() == 1) {
            if (cmd == "predict") {
                auto res = AnalysisService::predict(matches[0].symbol);
                if (res) {
                    UI::print_panel(UI::render_markov(res.value()), "REGIME: " + matches[0].symbol, "MARKOV MLE");
                } else {
                    std::cout << UI::fg(UI::CRIMSON) << "✖ Prediction Error: " << error_to_string(res.error()) << UI::RESET << "\n";
                }
            } else {
                auto res = AnalysisService::analyze(matches[0].symbol);
                if (res) {
                    UI::print_panel(UI::render_analysis(res.value()), matches[0].symbol, matches[0].exchange);
                } else {
                    std::cout << UI::fg(UI::CRIMSON) << "✖ Analysis Error: " << error_to_string(res.error()) << UI::RESET << "\n";
                }
            }
        } else {
            std::cout << "\n  " << UI::fg(UI::MUTED) << "Multiple matches found — select one:" << UI::RESET << "\n";
            for (size_t i = 0; i < matches.size(); ++i) {
                std::string country_flag = "[" + matches[i].country + "]";
                std::transform(country_flag.begin(), country_flag.end(), country_flag.begin(), ::toupper);
                
                std::cout << "  " << (i + 1) << ". " << UI::BOLD << UI::fg(UI::EMERALD) << std::left << std::setw(6) << matches[i].symbol << UI::RESET
                          << " " << UI::DIM << matches[i].name << UI::RESET << " " 
                          << UI::fg(UI::MUTED) << country_flag << " " << matches[i].exchange << UI::RESET << "\n";
            }
            std::cout << "\n  " << UI::fg(UI::MUTED) << "Enter [1-" << matches.size() << "] or 'q' to cancel: " << UI::RESET;
            std::cout.flush();
            
            char* sel_line = linenoise("  > ");
            if (sel_line) {
                std::string sel(sel_line);
                free(sel_line);
                if (sel != "q" && !sel.empty() && std::isdigit(sel[0])) {
                    int idx = std::stoi(sel) - 1;
                    if (idx >= 0 && idx < static_cast<int>(matches.size())) {
                        if (cmd == "predict") {
                            auto res = AnalysisService::predict(matches[idx].symbol);
                            if (res) {
                                UI::print_panel(UI::render_markov(res.value()), "REGIME: " + matches[idx].symbol, "MARKOV MLE");
                            } else {
                                std::cout << UI::fg(UI::CRIMSON) << "✖ Prediction Error: " << error_to_string(res.error()) << UI::RESET << "\n";
                            }
                        } else {
                            auto res = AnalysisService::analyze(matches[idx].symbol);
                            if (res) {
                                UI::print_panel(UI::render_analysis(res.value()), matches[idx].symbol, matches[idx].exchange);
                            } else {
                                std::cout << UI::fg(UI::CRIMSON) << "✖ Analysis Error: " << error_to_string(res.error()) << UI::RESET << "\n";
                            }
                        }
                    }
                }
            }
        }
        std::cout << "\n";
    }
}

} // namespace Axiom::Service
