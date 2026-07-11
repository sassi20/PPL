#include "tiny_ppl_core.h"

MachineState::MachineState(const std::vector<std::shared_ptr<Instr>>& controlStackinit,
    const std::vector<std::shared_ptr<Value>>& stackValueinit, 
    const std::map<std::string, std::shared_ptr<Value>>& environmentinit, 
    std::mt19937 randomNumberinit, double log_w_init)
  : controlStack(controlStackinit), stackValue(stackValueinit), environment(environmentinit), randomNumber(randomNumberinit), log_w(log_w_init) {}

MachineState::MachineState(const std::vector<std::shared_ptr<Instr>>& controlStackinit, std::mt19937 randomNumberinit)
  : controlStack(controlStackinit), randomNumber(randomNumberinit), log_w(0.0) {}

MachineState MachineState::fork(std::mt19937* new_rng) const {
  std::mt19937 rng_to_use = randomNumber;
  if (new_rng != nullptr) {
      rng_to_use = *new_rng;
  }
  return MachineState(controlStack, stackValue, environment, rng_to_use, log_w);
}



MachineState initMachine(const std::string& program_str, std::mt19937 randomNumber) {
    std::vector<std::shared_ptr<Expr>> forms = parse(program_str);
    std::map<std::string, std::shared_ptr<Value>> genv;
    std::shared_ptr<Expr> main_form = nullptr;
 
    for (auto& form : forms) {
        auto list = std::dynamic_pointer_cast<ListExpr>(form);
        bool isDefn = false;
        if (list && !list->elements.empty()) {
            auto headSym = std::dynamic_pointer_cast<SymbolExpr>(list->elements[0]);
            isDefn = headSym && headSym->name == "defn";
        }
        if (isDefn) {
            auto& elems = list->elements;
            auto nameSym = std::dynamic_pointer_cast<SymbolExpr>(elems[1]);
            auto paramsList = std::dynamic_pointer_cast<ListExpr>(elems[2]);
            std::vector<std::string> params;
            for (auto& p : paramsList->elements) {
                params.push_back(std::dynamic_pointer_cast<SymbolExpr>(p)->name);
            }
            std::vector<std::shared_ptr<Expr>> body(elems.begin() + 3, elems.end());
            genv[nameSym->name] = std::make_shared<Closure>(params, body, genv);
        } else {
            main_form = form;
        }
    }
    for (auto& kv : genv) {
        auto closure = std::dynamic_pointer_cast<Closure>(kv.second);
        if (closure) closure->env = genv;
    }
 
    std::vector<std::shared_ptr<Instr>> C;
    C.push_back(std::make_shared<EvInstr>(main_form, genv, Address{}));
    return MachineState(C, {}, genv, randomNumber, 0.0);
}

