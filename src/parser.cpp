#include "tiny_ppl_core.h"
#include <cctype>     // std::isspace
#include <cstdlib>    // std::strtod
#include <stdexcept>  // std::runtime_error
 

std::vector<std::string> tokenize(const std::string& program) {
    std::vector<std::string> tokens;
    size_t i = 0;
    while (i < program.size()) {
        char c = program[i];
        if (std::isspace(static_cast<unsigned char>(c))) {
            ++i;
            continue;
        }
        if (c == '(' || c == ')' || c == '[' || c == ']') {
            tokens.push_back(std::string(1, c));
            ++i;
            continue;
        }
        size_t start = i;
        while (i < program.size() && !std::isspace(static_cast<unsigned char>(program[i])) && program[i] != '(' && program[i] != ')' && program[i] != '[' && program[i] != ']') {
            ++i;
        }
        tokens.push_back(program.substr(start, i - start));
    }
    return tokens;
}
 

bool is_number_token(const std::string& tok) {
    if (tok.empty()) return false;
    char* end = nullptr;
    std::strtod(tok.c_str(), &end);
    return end != tok.c_str() && *end == '\0';
}
 

std::shared_ptr<Expr> parseExprFromTokens(const std::vector<std::string>& tokens, size_t& pos) {
    if (pos >= tokens.size()) {
        throw std::runtime_error("Error de parseo: se termino el input inesperadamente");
    }
    const std::string& tok = tokens[pos];
 
    if (tok == "(" || tok == "[") {
        std::string closing;
        if (tok == "(") {
            closing = ")";
        } else {
            closing = "]";
        }   
        ++pos;
        std::vector<std::shared_ptr<Expr>> elements;
        while (pos < tokens.size() && tokens[pos] != closing) {
            elements.push_back(parseExprFromTokens(tokens, pos));
        }
        if (pos >= tokens.size()) {
            throw std::runtime_error("Error de parseo: falta cerrar '" + tok + "'");
        }
        ++pos; 
        return std::make_shared<ListExpr>(elements);
    }

    if (tok == ")" || tok == "]") {
        throw std::runtime_error("Error de parseo: '" + tok + "' inesperado");
    }

    ++pos;
    if (is_number_token(tok)) {
        return std::make_shared<NumberExpr>(std::strtod(tok.c_str(), nullptr));
    }
    return std::make_shared<SymbolExpr>(tok);
}
 

std::vector<std::shared_ptr<Expr>> parse(const std::string& program) {
    std::vector<std::string> tokens = tokenize(program);
    std::vector<std::shared_ptr<Expr>> forms;
    size_t pos = 0;
    while (pos < tokens.size()) {
        forms.push_back(parseExprFromTokens(tokens, pos));
    }
    return forms;
}